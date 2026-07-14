cmake_minimum_required(VERSION 3.15)
project(TOIRELevelMeter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(../JUCE ${CMAKE_BINARY_DIR}/juce)

juce_add_plugin(TOIRELevelMeter
    COMPANY_NAME "TOIRE"
    PLUGIN_MANUFACTURER_CODE Tore
    PLUGIN_CODE Lvm1
    FORMATS VST3 Standalone
    PRODUCT_NAME "TOIRE Level Meter"
    VERSION 1.0.0
    COPY_PLUGIN_AFTER_BUILD TRUE)

juce_generate_juce_header(TOIRELevelMeter)

target_sources(TOIRELevelMeter PRIVATE
    Source/DSP/PluginProcessor.cpp
    Source/DSP/PluginEditor.cpp
    Source/DSP/LevelMeter/MeterComponent.cpp
    Source/Bridge/WebViewBridge.cpp)

target_include_directories(TOIRELevelMeter PRIVATE
    Source/DSP
    Source/Bridge
    "C:/Users/Admin/.nuget/packages/Microsoft.Web.WebView2/build/native/include")

target_compile_definitions(TOIRELevelMeter
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# --- Copy WebView2Loader.dll to output dirs (needed for dynamic linking) ---
set(WEBVIEW2_DLL "C:/Users/Admin/.nuget/packages/Microsoft.Web.WebView2/build/native/x64/WebView2Loader.dll")

add_custom_command(TARGET TOIRELevelMeter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WEBVIEW2_DLL}"
    "$<TARGET_FILE_DIR:TOIRELevelMeter>/WebView2Loader.dll"
    COMMENT "Copy WebView2Loader.dll to SharedCode")

add_custom_command(TARGET TOIRELevelMeter_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WEBVIEW2_DLL}"
    "$<TARGET_FILE_DIR:TOIRELevelMeter_Standalone>/WebView2Loader.dll"
    COMMENT "Copy WebView2Loader.dll to Standalone")

add_custom_command(TARGET TOIRELevelMeter_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WEBVIEW2_DLL}"
    "$<TARGET_FILE_DIR:TOIRELevelMeter_VST3>/WebView2Loader.dll"
    COMMENT "Copy WebView2Loader.dll to VST3")

# --- Copy webui ---
add_custom_command(TARGET TOIRELevelMeter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/webui"
    "$<TARGET_FILE_DIR:TOIRELevelMeter>/webui"
    COMMENT "Copy webui to SharedCode")

add_custom_command(TARGET TOIRELevelMeter_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/webui"
    "$<TARGET_FILE_DIR:TOIRELevelMeter_VST3>/../webui"
    COMMENT "Copy webui to VST3 Contents")

add_custom_command(TARGET TOIRELevelMeter_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/webui"
    "$<TARGET_FILE_DIR:TOIRELevelMeter_Standalone>/../webui"
    COMMENT "Copy webui to Standalone dir")

add_custom_command(TARGET TOIRELevelMeter_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$ENV{CommonProgramFiles}/VST3/TOIRE Level Meter.vst3/Contents/webui"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/webui"
    "$ENV{CommonProgramFiles}/VST3/TOIRE Level Meter.vst3/Contents/webui"
    COMMENT "Copy webui to Common Files VST3")

target_link_libraries(TOIRELevelMeter
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
