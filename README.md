# TOIRE-LevelMeter

JUCE C++ 音频电平表插件（VST3 / Standalone）。

## 架构

```
Source/DSP/              C++ 音频处理 + 原生 UI 渲染
Source/Bridge/           DSP ↔ WebView 通信桥（WebView 模式时启用）
Resources/webui/         WebUI 静态资源入口（Gemini React 编译产物）
WebUI/                   Gemini 工作区
```

## 构建

```bash
# 配置
cmake -B build -G "Visual Studio 17 2022"

# 编译
cmake --build build --config Debug
```

VST3 自动安装到 `C:\Program Files\Common Files\VST3\`。

## 数据流

```
音频线程 → LevelMeterData (atomic) → Timer 30Hz → paint() 原生 JUCE 渲染
```

## AI 协作

- **Admin** — 需求 & 决策
- **Cline** — C++ DSP / 编辑器实现
- **Hermes** — 构建系统 / 代码审计 / 基础设施
- **Gemini** — React WebUI（[任务说明](WebUI/README.md)）

协作日志：[chat.md](chat.md)
