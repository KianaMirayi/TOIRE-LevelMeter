#include "PluginEditor.h"

static float linToDb(float lin)
{
    if (lin < 1.58e-5f) return -96.0f;   // below -96 dBFS clamp, so log10 never diverges
    return std::max(-96.0f, 20.0f * std::log10(lin));
}

// Walks up from the executable looking for the webui folder shipped next to the plugin.
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

// ====== DarkWebBrowserComponent ======

DarkWebBrowserComponent::DarkWebBrowserComponent(
    const juce::WebBrowserComponent::Options& options)
    : juce::WebBrowserComponent(options)
{
    setOpaque(true);   // paint() fills every pixel itself
}

void DarkWebBrowserComponent::paint(juce::Graphics& g)
{
    // Matches the Web UI body background, so the hand-off to the page is not a visible change.
    g.fillAll(juce::Colour(0xff0d0e11));
}

void DarkWebBrowserComponent::pageFinishedLoading(const juce::String& url)
{
    juce::WebBrowserComponent::pageFinishedLoading(url);

    navigated = true;   // fires before the first paint, so only used as a fallback reveal trigger
}

// ====== TOIRELevelMeterAudioProcessorEditor ======

TOIRELevelMeterAudioProcessorEditor::TOIRELevelMeterAudioProcessorEditor(
    TOIRELevelMeterAudioProcessor& processor)
    : AudioProcessorEditor(&processor)
    , audioProcessor(processor)
{
    setSize(480, 360);

    setOpaque(true);   // the editor fills itself completely below

    startTimerHz(30);

    payloadBuffer.resize(4);
    for (int i = 0; i < 4; ++i)
        payloadBuffer.set(i, juce::var(-96.0f));

    // Built here rather than from the timer, so controller creation overlaps the host showing the window.
    createWebView();
}

TOIRELevelMeterAudioProcessorEditor::~TOIRELevelMeterAudioProcessorEditor()
{
    stopTimer();
    webView.reset();
}

void TOIRELevelMeterAudioProcessorEditor::createWebView()
{
    jassert (webView == nullptr);

    auto winOpts = juce::WebBrowserComponent::Options::WinWebView2{};
    winOpts = winOpts.withUserDataFolder(juce::File::getSpecialLocation(
        juce::File::tempDirectory).getChildFile("TOIRE_WebView2"));
    winOpts = winOpts.withStatusBarDisabled();
    winOpts = winOpts.withBuiltInErrorPageDisabled();
    winOpts = winOpts.withBackgroundColour(juce::Colour(0xff0d0e11));   // JUCE defaults this to fully transparent

    auto opts = juce::WebBrowserComponent::Options{};
    opts = opts.withBackend(juce::WebBrowserComponent::Options::Backend::webview2);
    opts = opts.withWinWebView2Options(winOpts);
    opts = opts.withNativeIntegrationEnabled();
    opts = opts.withKeepPageLoadedWhenBrowserIsHidden();

    opts = opts.withEventListener("uiReady", [this](const juce::var&) { pageReadyFlag.store(true); });

    // Injects the page as base64 so about:blank, which loads instantly, holds the real UI.
    auto indexHtml = findWebui();
    if (indexHtml.existsAsFile())
    {
        const juce::String b64 = loadHtmlBase64(indexHtml);
        if (b64.isNotEmpty())
            opts = opts.withUserScript(
                "document.write(atob('" + b64 + "'));"
                "document.close();"
                "var __sigDone=false;"
                "function __sig(){"
                "  if(__sigDone)return; __sigDone=true;"
                "  try{ if(window.__JUCE__&&__JUCE__.backend)"
                "    __JUCE__.backend.emitEvent('uiReady',{}); }catch(e){}"
                "}"
                "requestAnimationFrame(function(){requestAnimationFrame(__sig);});"
                "setTimeout(__sig,1000);");
    }

    webView = std::make_unique<DarkWebBrowserComponent>(opts);

    // Starts at 1x1: the WebView2 HWND sits above every JUCE-drawn component, so at full size it
    // would cover the placeholder with its own unpainted surface until the page composites.
    webView->setBounds(0, 0, 1, 1);

    addAndMakeVisible(webView.get());
    webView->goToURL("about:blank");
}

void TOIRELevelMeterAudioProcessorEditor::resized()
{
    if (webView && webViewRevealed)
        webView->setBounds(getLocalBounds());
}

// Moves `val` toward `target` by `step` dB per tick; at 30 Hz step=2.0 is ~60 dB/s.
static void decayToward(float& val, float target, float step)
{
    if (val > target)
        val = std::max(target, val - step);
    else if (val < target)
        val = std::min(target, val + step);
}

void TOIRELevelMeterAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;   // createWebView() runs from the constructor

    // Safety net: JUCE only creates the WebView2 controller on a hierarchy or visibility change
    // once the component has a peer, so force one if the host never delivered it.
    if (!nudgedWebView && !webView->hasNavigated() && isShowing())
    {
        nudgedWebView = true;
        webView->setVisible(false);
        webView->setVisible(true);
    }

    if (!webViewRevealed)
    {
        ++ticksSinceCreate;
        if (webView->hasNavigated())
            ++ticksSinceNav;

        // Three triggers: the page's own composited signal, navigation plus a settle, and a hard cap.
        const bool pageReady  = pageReadyFlag.load();
        const bool navSettled = webView->hasNavigated() && ticksSinceNav > 6;   // ~200 ms
        const bool gaveUp     = ticksSinceCreate > 75;                          // ~2.5 s

        if (pageReady || navSettled || gaveUp)
        {
            webViewRevealed = true;
            webView->setBounds(getLocalBounds());
            webView->toFront(false);
        }
        else
        {
            loadingPhase += 0.08f;
            if (loadingPhase > 1.0f) loadingPhase -= 1.0f;

            repaint();   // the dots are drawn by this editor, not by the 1x1 WebView
            return;
        }
    }

    const uint64_t currentFrame = audioProcessor.getFrameCount();
    const bool audioIsRunning = (currentFrame != lastFrameCount);
    lastFrameCount = currentFrame;

    auto& meter = audioProcessor.getMeterData();

    if (audioIsRunning)
    {
        displayPeakDb     = linToDb(meter.getPeak());
        displayPeakHoldDb = linToDb(meter.getPeakHold());

        // While silent, hold off reading RMS: the ring buffer keeps 300 ms of history and would
        // feed stale non-zero values that fight the decay.
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
        // Paused, deactivated or bypassed: decay everything to the floor.
        constexpr float decayStep = 2.0f;
        constexpr float floorDb   = -96.0f;

        decayToward(displayPeakDb,     floorDb, decayStep);
        decayToward(displayPeakHoldDb, floorDb, decayStep);
        decayToward(displayRmsDb,      floorDb, decayStep);
        decayToward(displayRmsHoldDb,  floorDb, decayStep);
    }

    if (!webViewRevealed)
        return;   // the page has to be up before events can reach it

    payloadBuffer.set(0, displayPeakDb);
    payloadBuffer.set(1, displayPeakHoldDb);
    payloadBuffer.set(2, displayRmsDb);
    payloadBuffer.set(3, displayRmsHoldDb);
    webView->emitEventIfBrowserIsVisible("levelData", payloadBuffer);
}

void TOIRELevelMeterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Full-bleed and opaque: this panel covers the whole startup, and an unpainted client area
    // would show the DAW through the window.
    g.fillAll(juce::Colour(0xff0d0e11));

    if (webViewRevealed)
        return;

    // Loading indicator: three pulsing dots in the Web UI's accent blue.
    const auto  b  = getLocalBounds().toFloat();
    const float cx = b.getCentreX();
    const float cy = b.getCentreY();
    constexpr float r   = 2.5f;
    constexpr float gap = 11.0f;

    for (int i = 0; i < 3; ++i)
    {
        float t = loadingPhase - (float) i * 0.22f;
        if (t < 0.0f) t += 1.0f;

        const float pulse = std::abs(std::sin(t * juce::MathConstants<float>::pi));

        g.setColour(juce::Colour(0xff4a90d9).withAlpha(0.15f + 0.55f * pulse));
        g.fillEllipse(cx + ((float) i - 1.0f) * gap - r, cy - r, r * 2.0f, r * 2.0f);
    }
}
