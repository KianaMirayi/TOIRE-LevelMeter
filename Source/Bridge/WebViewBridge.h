#pragma once
#include <JuceHeader.h>

// ====== WebViewBridge ======
// Thread-safe bridge between DSP audio thread and WebView UI.
// Uses JUCE's AsyncUpdater — DSP pushes level data from audio thread,
// handleAsyncUpdate() delivers to WebView on the message thread.

class WebViewBridge : public juce::AsyncUpdater
{
public:
    WebViewBridge() = default;
    ~WebViewBridge() override = default;

    // Called from audio thread — stores latest level, triggers async update
    void pushLevel(float peak, float rms)
    {
        latestPeak.store(peak);
        latestRms.store(rms);
        triggerAsyncUpdate();
    }

    // Called on message thread by WebView JS via nativeFunction
    float getPeak() const { return latestPeak.load(); }
    float getRms()  const { return latestRms.load(); }

    // Override: called on message thread after triggerAsyncUpdate()
    void handleAsyncUpdate() override
    {
        // Notify listeners (e.g. PluginEditor) that new level data is ready
        if (onNewLevel)
            onNewLevel(latestPeak.load(), latestRms.load());
    }

    std::function<void(float, float)> onNewLevel;

private:
    std::atomic<float> latestPeak { 0.0f };
    std::atomic<float> latestRms  { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewBridge)
};
