#include "PluginEditor.h"

static float linToDb(float lin)
{
    // Clamp tiny values to silence to avoid floating-point drift in RMS ring buffer
    if (lin < 1e-6f) return -96.0f;
    return 20.0f * std::log10(lin);
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

    webView = std::make_unique<juce::WebBrowserComponent>(opts);
    webView->setBounds(0, 0, 480, 360);
    webView->setOpaque(true);
    addAndMakeVisible(webView.get());
    webView->setVisible(true);
    webView->toFront(true);

    auto index = findWebui();
    if (index.existsAsFile())
        webView->goToURL("file:///" + index.getFullPathName().replace("\\", "/"));
    else
        webView->goToURL("about:blank");

    startTimerHz(30);
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
}

void TOIRELevelMeterAudioProcessorEditor::timerCallback()
{
    if (!webView) return;

    auto& meter = audioProcessor.getMeterData();
    displayPeakDb     = linToDb(meter.getPeak());
    displayPeakHoldDb = linToDb(meter.getPeakHold());
    displayRmsDb      = linToDb(meter.getRms());
    displayRmsHoldDb  = linToDb(meter.getRmsHold());

    // When DAW stops, processBlock isn't called. If peak is silent,
    // force RMS values to -96 to prevent stale readings.
    if (displayPeakDb < -90.0f)
    {
        displayRmsDb     = -96.0f;
        displayRmsHoldDb = -96.0f;
    }
    juce::var payload = juce::var(juce::Array<juce::var>({
        juce::var(displayPeakDb),
        juce::var(displayPeakHoldDb),
        juce::var(displayRmsDb),
        juce::var(displayRmsHoldDb)
    }));
    webView->emitEventIfBrowserIsVisible("levelData", payload);
}
