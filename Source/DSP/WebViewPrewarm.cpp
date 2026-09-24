#include "WebViewPrewarm.h"

#if JUCE_WINDOWS
 #include <WebView2.h>
#endif

namespace
{
#if JUCE_WINDOWS
    constexpr int kPrewarmTimeoutMs = 15000;

    using CreateEnvironmentFn = HRESULT (STDMETHODCALLTYPE*) (PCWSTR,
                                                              PCWSTR,
                                                              ICoreWebView2EnvironmentOptions*,
                                                              ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

    // Resolves the DLL from the plugin binary's own folder first; the bare name matches JUCE's fallback.
    HMODULE loadWebView2Loader()
    {
        const auto dll = juce::File::getSpecialLocation (juce::File::invokedExecutableFile)
                             .getParentDirectory()
                             .getChildFile ("WebView2Loader.dll");

        if (dll.existsAsFile())
            if (auto* module = ::LoadLibraryExW (dll.getFullPathName().toWideCharPointer(),
                                                 nullptr,
                                                 LOAD_WITH_ALTERED_SEARCH_PATH))
                return module;

        return ::LoadLibraryA ("WebView2Loader.dll");
    }

    // Must match the folder the editor's WebBrowserComponent uses, or nothing is shared and the warm-up is wasted.
    juce::String getPrewarmUserDataFolder()
    {
        return juce::File::getSpecialLocation (juce::File::tempDirectory)
                   .getChildFile ("TOIRE_WebView2")
                   .getFullPathName();
    }

    // Hand-written so the project does not depend on the WRL headers of whichever WebView2 SDK is installed.
    class EnvironmentCompletedHandler final
        : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
    {
    public:
        EnvironmentCompletedHandler (std::atomic<bool>& completedFlag,
                                     ICoreWebView2Environment** environmentSlotIn)
            : completed (completedFlag)
            , environmentSlot (environmentSlotIn)
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void** ppvObject) override
        {
            if (ppvObject == nullptr)
                return E_POINTER;

            if (riid == __uuidof (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)
                || riid == __uuidof (IUnknown))
            {
                *ppvObject = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*> (this);
                AddRef();
                return S_OK;
            }

            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }

        ULONG STDMETHODCALLTYPE AddRef() override { return ++refCount; }

        ULONG STDMETHODCALLTYPE Release() override
        {
            const auto remaining = --refCount;

            if (remaining == 0)
                delete this;

            return remaining;
        }

        HRESULT STDMETHODCALLTYPE Invoke (HRESULT /*errorCode*/,
                                          ICoreWebView2Environment* createdEnvironment) override
        {
            if (createdEnvironment != nullptr)
            {
                createdEnvironment->AddRef();   // holding this reference is what keeps the browser process alive
                *environmentSlot = createdEnvironment;
            }

            completed.store (true);
            return S_OK;
        }

    private:
        ~EnvironmentCompletedHandler() = default;

        std::atomic<ULONG> refCount { 1 };
        std::atomic<bool>& completed;
        ICoreWebView2Environment** environmentSlot;
    };
#endif
}

// ====== WebViewPrewarm::Worker ======
// Owns the warmed environment, and must stay alive while the plugin is loaded: releasing it shuts
// the shared browser process back down.

class WebViewPrewarm::Worker final : public juce::Thread
{
public:
    Worker() : juce::Thread ("TOIRE WebView2 prewarm") {}

    ~Worker() override
    {
        signalThreadShouldExit();
        stopThread (kPrewarmTimeoutMs + 2000);
    }

    void run() override
    {
       #if JUCE_WINDOWS
        const auto comResult = ::CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED);

        auto* loader = loadWebView2Loader();
        auto* createEnvironment = loader != nullptr
            ? (CreateEnvironmentFn) ::GetProcAddress (loader, "CreateCoreWebView2EnvironmentWithOptions")
            : nullptr;

        if (createEnvironment != nullptr)
        {
            auto* handler = new EnvironmentCompletedHandler (completed, &liveEnvironment);
            const auto folder = getPrewarmUserDataFolder();

            if (SUCCEEDED (createEnvironment (nullptr, folder.toWideCharPointer(), nullptr, handler)))
            {
                // The call blocks until completion on this thread, but pumping costs nothing and keeps this correct regardless.
                MSG message {};
                const auto deadline = juce::Time::getMillisecondCounter() + (juce::uint32) kPrewarmTimeoutMs;

                while (! completed.load()
                       && ! threadShouldExit()
                       && juce::Time::getMillisecondCounter() < deadline)
                {
                    while (::PeekMessageW (&message, nullptr, 0, 0, PM_REMOVE))
                    {
                        ::TranslateMessage (&message);
                        ::DispatchMessageW (&message);
                    }

                    juce::Thread::sleep (10);
                }
            }

            handler->Release();
        }

        // Never exit early: this thread's COM apartment owns the environment, so the browser process
        // survives exactly as long as this loop runs.
        MSG message {};
        while (! threadShouldExit())
        {
            while (::PeekMessageW (&message, nullptr, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage (&message);
                ::DispatchMessageW (&message);
            }

            juce::Thread::sleep (50);
        }

        if (liveEnvironment != nullptr)
        {
            liveEnvironment->Release();
            liveEnvironment = nullptr;
        }

        if (comResult == S_OK || comResult == S_FALSE)
            ::CoUninitialize();
       #endif
    }

private:
    std::atomic<bool> completed { false };
    ICoreWebView2Environment* liveEnvironment = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Worker)
};

// ====== WebViewPrewarm ======

WebViewPrewarm::WebViewPrewarm()
{
    worker = std::make_unique<Worker>();
    worker->startThread (juce::Thread::Priority::low);
}

WebViewPrewarm::~WebViewPrewarm() = default;
