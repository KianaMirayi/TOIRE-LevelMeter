cmake_minimum_required(VERSION 3.15)
project(TOIRELevelMeter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 指向 JUCE 源码目录
add_subdirectory(../JUCE ${CMAKE_BINARY_DIR}/juce)

# 创建音频插件
juce_add_plugin(TOIRELevelMeter
    COMPANY_NAME "TOIRE"
    PLUGIN_MANUFACTURER_CODE Tore
    PLUGIN_CODE Lvm1
    FORMATS VST3 Standalone
    PRODUCT_NAME "TOIRE Level Meter"
    VERSION 1.0.0
    COPY_PLUGIN_AFTER_BUILD TRUE)

# 生成 JuceHeader.h
juce_generate_juce_header(TOIRELevelMeter)

# ====== 三层架构 ======
# DSP 层: 音频处理 (Cline)
# Bridge 层: DSP ↔ WebView 通信 (Cline)
# WebUI 层: React 前端 (Gemini, 编译产物放 Resources/webui/)

target_sources(TOIRELevelMeter PRIVATE
    Source/DSP/PluginProcessor.cpp
    Source/DSP/PluginEditor.cpp
    Source/DSP/LevelMeter/MeterComponent.cpp
    Source/Bridge/WebViewBridge.cpp)

# 添加 include 目录
target_include_directories(TOIRELevelMeter PRIVATE
    Source/DSP
    Source/Bridge)

# 设置编译定义 (WebView 启用)
target_compile_definitions(TOIRELevelMeter
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# ====== 复制 webui 到各输出目录 ======
add_custom_command(TARGET TOIRELevelMeter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/webui"
    "$<TARGET_FILE_DIR:TOIRELevelMeter>/webui"
    COMMENT "Copying webui to SharedCode output")

# Also copy to VST3/Standalone Contents dir for DAW loading
add_custom_command(TARGET TOIRELevelMeter_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/webui"
    "$<TARGET_FILE_DIR:TOIRELevelMeter_VST3>/../webui"
    COMMENT "Copying webui to VST3 Contents")
add_custom_command(TARGET TOIRELevelMeter_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/webui"
    "$<TARGET_FILE_DIR:TOIRELevelMeter_Standalone>/../webui"
    COMMENT "Copying webui to Standalone dir")

# Also copy webui to Common Files VST3 dir so DAW-loaded plugin finds the UI
add_custom_command(TARGET TOIRELevelMeter_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    "$ENV{CommonProgramFiles}/VST3/TOIRE Level Meter.vst3/Contents/webui"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/webui"
    "$ENV{CommonProgramFiles}/VST3/TOIRE Level Meter.vst3/Contents/webui"
    COMMENT "Copying webui to Common Files VST3 for DAW deployment")

# 链接 JUCE 模块
target_link_libraries(TOIRELevelMeter
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
