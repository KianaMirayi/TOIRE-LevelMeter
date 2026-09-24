#pragma once
#include <JuceHeader.h>

// ====== WebViewPrewarm ======
// Holds a WebView2 environment open from plugin load, keeping the shared browser process alive,
// so that opening the editor does not have to start that process.
// Everything runs on a background thread and nothing here touches the message thread.
// Best-effort: any failure silently disables it, leaving behaviour as if it were absent.
class WebViewPrewarm
{
public:
    WebViewPrewarm();
    ~WebViewPrewarm();

private:
    class Worker;

    std::unique_ptr<Worker> worker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebViewPrewarm)
};
