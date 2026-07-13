#include "WebViewBridge.h"

// WebViewBridge implementation — see header for architecture notes.
// The bridge is intentionally thin: DSP pushes, WebView pulls.
// Any additional JS ↔ native methods should be registered in PluginEditor.
