#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ===================================================================
//  TOIRELevelMeterAudioProcessorEditor
//
//  Current renderer: native JUCE Graphics (paint + Timer 30Hz)
//  Future renderer:   WebView + React JS (evaluateJavascript)
//
//  GEMINI REPLACEABLE:
//    - paint() body (all drawing code)
//    - displayPeakDb/RmsDb/Clip members
//    - resized() layout calculations
//    - timerCallback() → evaluateJavascript path
//
//  KEEP ALWAYS:
//    - Timer base class + startTimerHz(30)
//    - linToDb() / dbToNorm() helpers
//    - meterData.getPeakHold() / getRms() data flow
//    - WebViewBridge (future JS bridge entry)
// ===================================================================

class TOIRELevelMeterAudioProcessorEditor
    : public juce::AudioProcessorEditor
    , private juce::Timer
{
public:
    explicit TOIRELevelMeterAudioProcessorEditor(TOIRELevelMeterAudioProcessor& processor);
    ~TOIRELevelMeterAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    TOIRELevelMeterAudioProcessor& audioProcessor;

    // ── DISPLAY STATE (Gemini replaceable) ──────────────
    float displayPeakDb = -96.0f;
    float displayRmsDb  = -96.0f;
    bool  displayClip   = false;

    // ── LAYOUT (Gemini replaceable) ─────────────────────
    juce::Rectangle<int> peakBarArea;
    juce::Rectangle<int> rmsBarArea;
    juce::Rectangle<int> clipArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TOIRELevelMeterAudioProcessorEditor)
};
