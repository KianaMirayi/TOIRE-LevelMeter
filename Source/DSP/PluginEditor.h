#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ====== PluginEditor (WebView-based UI) ======
//
// Loading strategy:
//   1. navigate to about:blank
//   2. read index.html → Base64 → inject via withUserScript
//   3. C++ → JS updates: emitEventIfBrowserIsVisible("levelData", [...])
//
// Why: file:/// fails with special chars in path (C++Hell). data: URI blocked.
//      Base64 + about:blank + userScript is the only reliable cross-context method.

class TOIRELevelMeterAudioProcessorEditor
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit TOIRELevelMeterAudioProcessorEditor(TOIRELevelMeterAudioProcessor& processor);
    ~TOIRELevelMeterAudioProcessorEditor() override;

    void resized() override;

private:
    void timerCallback() override;

    TOIRELevelMeterAudioProcessor& audioProcessor;

    float displayPeakDb = -96.0f;
    float displayRmsDb  = -96.0f;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TOIRELevelMeterAudioProcessorEditor)
};
