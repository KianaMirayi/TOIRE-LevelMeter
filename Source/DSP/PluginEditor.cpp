#include "PluginEditor.h"

static float linToDb(float lin)
{
    if (lin <= 0.0f) return -96.0f;
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

    const auto winOpts = juce::WebBrowserComponent::Options::WinWebView2{}
        .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)
                            .getChildFile("TOIRE_WebView2"))
        .withStatusBarDisabled()
        .withBuiltInErrorPageDisabled();

    const auto opts = juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(winOpts)
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden();

    webView = std::make_unique<juce::WebBrowserComponent>(opts);
    webView->setBounds(0, 0, 480, 360);
    webView->setOpaque(true);
    addAndMakeVisible(webView.get());
    webView->setVisible(true);
    webView->toFront(true);

    // Load from file — the index.html already has addEventListener for levelData
    auto index = findWebui();
    if (index.existsAsFile())
    {
        auto url = "file:///" + index.getFullPathName().replace("\\", "/");
        webView->goToURL(url);
    }
    else
    {
        webView->goToURL("about:blank");
    }

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
    displayPeakDb = linToDb(meter.getPeakHold());
    displayRmsDb  = linToDb(meter.getRms());
    juce::var payload = juce::var(juce::Array<juce::var>(
        { juce::var(displayPeakDb), juce::var(displayRmsDb) }));
    webView->emitEventIfBrowserIsVisible("levelData", payload);
}
