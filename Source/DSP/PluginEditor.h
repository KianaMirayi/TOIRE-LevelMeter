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
// Anti-black-border: WebView hidden for first ~1s (方案二), bg colour set (方案一)

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

    float displayPeakDb     = -96.0f;
    float displayPeakHoldDb = -96.0f;
    float displayRmsDb      = -96.0f;
    float displayRmsHoldDb  = -96.0f;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 方案二：延迟显示 WebView
    int  loadDelayFrames = 0;
    bool webViewShown    = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TOIRELevelMeterAudioProcessorEditor)
};
