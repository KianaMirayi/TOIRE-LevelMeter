#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ====== PluginEditor (WebView-based UI) ======
//
// Data format: C++ → JS via emitEventIfBrowserIsVisible("levelData", [peak, peakHold, rms, rmsHold])
//   data[0] = instantaneous peak (dB)
//   data[1] = peak hold (dB)
//   data[2] = RMS (dB)
//   data[3] = RMS hold (dB)
//
// Anti-black-border: WebView shown immediately, bg colour matched across paint() / WebView2 / HTML

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

    TOIRELevelMeterAudioProcessor& audioProcessor;

    float displayPeakDb     = -96.0f;
    float displayPeakHoldDb = -96.0f;
    float displayRmsDb      = -96.0f;
    float displayRmsHoldDb  = -96.0f;

    // Frame-count detection for pause/bypass/deactivate
    uint64_t lastFrameCount = 0;

    // Pre-allocated to avoid heap allocation every tick @ 30Hz
    juce::Array<juce::var> payloadBuffer;
    juce::Path             clipPath;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TOIRELevelMeterAudioProcessorEditor)
};
