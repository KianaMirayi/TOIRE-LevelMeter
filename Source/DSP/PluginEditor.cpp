#include "PluginEditor.h"

static float linToDb(float lin)
{
    // Clamp to -96 dBFS. Threshold = 10^(-96/20) ≈ 1.58e-5 prevents
    // log10 from producing values below -96 (e.g. 1.2e-6 → -118 dB).
    if (lin < 1.58e-5f) return -96.0f;
    return std::max(-96.0f, 20.0f * std::log10(lin));
}

static juce::File findWebui()
{
    const auto exeDir = juce::File::getSpecialLocation(
        juce::File::invokedExecutableFile).getParentDirectory();
    auto dir = exeDir;
    for (int i = 0; i < 8; ++i)
    {
        for (auto& sub : { "webui/index.html", "Contents/webui/index.html",
                           "../webui/index.html", "../../webui/index.html" })
        {
            auto f = dir.getChildFile(sub);
            if (f.existsAsFile()) return f;
        }
        dir = dir.getParentDirectory();
    }
    return {};
}

static juce::String loadHtmlBase64(const juce::File& f)
{
    juce::MemoryBlock mb;
    return f.loadFileAsData(mb) ? juce::Base64::toBase64(mb.getData(), mb.getSize()) : juce::String{};
}

TOIRELevelMeterAudioProcessorEditor::TOIRELevelMeterAudioProcessorEditor(
    TOIRELevelMeterAudioProcessor& processor)
    : AudioProcessorEditor(&processor)
    , audioProcessor(processor)
{
    setSize(480, 360);

    auto winOpts = juce::WebBrowserComponent::Options::WinWebView2{};
    winOpts = winOpts.withUserDataFolder(juce::File::getSpecialLocation(
        juce::File::tempDirectory).getChildFile("TOIRE_WebView2"));
    winOpts = winOpts.withStatusBarDisabled();
    winOpts = winOpts.withBuiltInErrorPageDisabled();
    winOpts = winOpts.withBackgroundColour(juce::Colour(0xff1a1a2e));

    auto opts = juce::WebBrowserComponent::Options{};
    opts = opts.withBackend(juce::WebBrowserComponent::Options::Backend::webview2);
    opts = opts.withWinWebView2Options(winOpts);
    opts = opts.withNativeIntegrationEnabled();
    opts = opts.withKeepPageLoadedWhenBrowserIsHidden();

    // Inject HTML as Base64 UserScript → about:blank loads instantly
    auto indexHtml = findWebui();
    if (indexHtml.existsAsFile())
    {
        const juce::String b64 = loadHtmlBase64(indexHtml);
        if (b64.isNotEmpty())
            opts = opts.withUserScript("document.write(atob('" + b64 + "')); document.close();");
    }

    webView = std::make_unique<juce::WebBrowserComponent>(opts);
    webView->setBounds(0, 0, 480, 360);
    webView->setOpaque(false);
    addAndMakeVisible(webView.get());
    webView->setVisible(true);
    webView->toFront(true);

    webView->goToURL("about:blank");
    startTimerHz(30);

    // Pre-allocate payload buffer (4 floats sent to JS every tick)
    payloadBuffer.resize(4);
    for (int i = 0; i < 4; ++i)
        payloadBuffer.set(i, juce::var(-96.0f));

    // Pre-build rounded-corner clip path
    constexpr float cornerRadius = 12.0f;
    clipPath.addRoundedRectangle(juce::Rectangle<float>(480.0f, 360.0f), cornerRadius);
}

TOIRELevelMeterAudioProcessorEditor::~TOIRELevelMeterAudioProcessorEditor()
{
    stopTimer();
    webView.reset();
}

void TOIRELevelMeterAudioProcessorEditor::resized()
{
    if (webView)
    {
        webView->setBounds(getLocalBounds());
        webView->toFront(true);
    }

    // Rebuild clip path when window size changes
    constexpr float cornerRadius = 12.0f;
    clipPath.clear();
    clipPath.addRoundedRectangle(getLocalBounds().toFloat(), cornerRadius);
}

// ====== Smooth decay helper ======
// Moves `val` toward `target` by `step` dB per tick.
// At 30 Hz, step=2.0 → ~60 dB/s, giving a smooth ~1.6 s fall from 0 to -96.
static void decayToward(float& val, float target, float step)
{
    if (val > target)
        val = std::max(target, val - step);
    else if (val < target)
        val = std::min(target, val + step);
}

void TOIRELevelMeterAudioProcessorEditor::timerCallback()
{
    if (!webView) return;

    const uint64_t currentFrame = audioProcessor.getFrameCount();
    const bool audioIsRunning = (currentFrame != lastFrameCount);
    lastFrameCount = currentFrame;

    auto& meter = audioProcessor.getMeterData();

    if (audioIsRunning)
    {
        // Audio is flowing — read peak from DSP (instant, no history)
        displayPeakDb     = linToDb(meter.getPeak());
        displayPeakHoldDb = linToDb(meter.getPeakHold());

        // When signal is present, read RMS from ring buffer.
        // When silent, DON'T read RMS — the ring buffer has 300 ms history
        // and would keep feeding stale non-zero values, fighting the decay.
        if (displayPeakDb < -80.0f)
        {
            constexpr float decayStep = 2.0f;
            decayToward(displayRmsDb,     -96.0f, decayStep);
            decayToward(displayRmsHoldDb,  -96.0f, decayStep);
        }
        else
        {
            displayRmsDb     = linToDb(meter.getRms());
            displayRmsHoldDb = linToDb(meter.getRmsHold());
        }
    }
    else
    {
        // Paused / deactivated / bypassed — smooth decay all values to -96 dB
        constexpr float decayStep = 2.0f;  // dB per tick at 30 Hz ≈ 60 dB/s
        constexpr float floorDb   = -96.0f;

        decayToward(displayPeakDb,     floorDb, decayStep);
        decayToward(displayPeakHoldDb, floorDb, decayStep);
        decayToward(displayRmsDb,      floorDb, decayStep);
        decayToward(displayRmsHoldDb,  floorDb, decayStep);
    }

    // Reuse pre-allocated buffer — avoid 6 heap allocations per tick @ 30Hz
    payloadBuffer.set(0, displayPeakDb);
    payloadBuffer.set(1, displayPeakHoldDb);
    payloadBuffer.set(2, displayRmsDb);
    payloadBuffer.set(3, displayRmsHoldDb);
    webView->emitEventIfBrowserIsVisible("levelData", payloadBuffer);
}

void TOIRELevelMeterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Use cached clip path (built in constructor, rebuilt in resized)
    g.reduceClipRegion(clipPath);
    g.fillAll(juce::Colour(0xff1a1a2e));
}
