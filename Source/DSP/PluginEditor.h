#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ====== PluginEditor (WebView-based UI) ======
// C++ -> JS: emitEventIfBrowserIsVisible("levelData", [peak, peakHold, rms, rmsHold]), all in dB.
// The WebView is built in the constructor so WebView2 startup overlaps the host showing the window.
// It stays 1x1 until the page reports itself composited, so its HWND cannot cover the placeholder.
// DarkWebBrowserComponent replaces JUCE's hard-coded white fallback paint with the Web UI colour.

// WebBrowserComponent that paints the Web UI background instead of JUCE's white, and tracks navigation.
class DarkWebBrowserComponent : public juce::WebBrowserComponent
{
public:
    explicit DarkWebBrowserComponent (const juce::WebBrowserComponent::Options& options);

    void paint (juce::Graphics& g) override;
    void pageFinishedLoading (const juce::String& url) override;

    // True once navigation completed; a fallback reveal trigger, not proof of visibility.
    bool hasNavigated() const noexcept { return navigated; }

private:
    bool navigated = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkWebBrowserComponent)
};

class TOIRELevelMeterAudioProcessorEditor
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit TOIRELevelMeterAudioProcessorEditor(TOIRELevelMeterAudioProcessor& processor);
    ~TOIRELevelMeterAudioProcessorEditor() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;

    void createWebView();

    TOIRELevelMeterAudioProcessor& audioProcessor;

    float displayPeakDb     = -96.0f;
    float displayPeakHoldDb = -96.0f;
    float displayRmsDb      = -96.0f;
    float displayRmsHoldDb  = -96.0f;

    uint64_t lastFrameCount = 0;                 // detects pause / bypass / deactivate
    juce::Array<juce::var> payloadBuffer;        // pre-allocated to avoid per-tick heap use

    std::atomic<bool> pageReadyFlag { false };   // declared before webView: its Options lambda captures `this`

    std::unique_ptr<DarkWebBrowserComponent> webView;

    bool  webViewRevealed  = false;              // has the WebView been grown to full size
    int   ticksSinceCreate = 0;
    int   ticksSinceNav    = 0;
    float loadingPhase     = 0.0f;               // drives the placeholder dots
    bool  nudgedWebView    = false;              // one-shot safety net that forces controller creation

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TOIRELevelMeterAudioProcessorEditor)
};
