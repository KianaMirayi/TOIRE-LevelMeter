# TOIRE-LevelMeter

JUCE 8 C++ 音频电平表插件（VST3 / Standalone）。WebView2 驱动赛博朋克风格 UI。

## 功能

- **四值电平检测**：瞬时 Peak、Peak Hold、RMS、RMS Hold
- **平滑衰减**：暂停 / deactivate / bypass 时所有电平值平滑归零（~1.6s）

## 架构

```
Source/
├── DSP/
│   ├── PluginProcessor.cpp/h    # 音频处理器
│   ├── PluginEditor.cpp/h       # WebView2 编辑器（30Hz Timer → JS）
│   └── LevelMeter/
│       └── MeterComponent.cpp/h # 纯 DSP 电平计算（lock-free atomic）
└── Bridge/
    └── WebViewBridge.cpp/h      # 预留通信桥（当前未使用）

Resources/webui/
    └── index.html               # WebUI
```

## 数据流

```
音频线程                         消息线程（30Hz）
────────                        ──────────
processBlock()                  timerCallback()
  ├─ meterData.setLatestPeak()    ├─ meterData.getPeak()      atomic →
  ├─ meterData.updatePeakHold()   ├─ meterData.getPeakHold()  atomic →
  ├─ meterData.updateRmsHold()    ├─ meterData.getRms()       atomic →
  ├─ meterData.processRms()       ├─ meterData.getRmsHold()   atomic →
  └─ ++frameCounter               ├─ frameCounter             检测暂停
                                  └─ emitEventIfBrowserIsVisible("levelData", [p, pH, r, rH])
```

## 构建

```bash
# 配置（MSVC）
cmake -B build -G "Visual Studio 17 2022"

# 编译 Debug
cmake --build build --config Debug

# 编译 Release
cmake --build build --config Release
```

VST3 自动安装到 `C:\Program Files\Common Files\VST3\`。

### 依赖

- JUCE 8（路径由 `CMakeLists.txt` 中的 `add_subdirectory` 指定）
- WebView2 Runtime（Windows 10+ 自带）
- `WebView2Loader.dll`（NuGet 1.0.1901.177，构建时自动复制）

## 已知限制

- **加载延迟**：首次打开插件 UI 有 ~1s WebView2 冷启动延迟（Edge WebView2 进程初始化，平台层限制）
- **仅支持立体声**：`isBusesLayoutSupported` 限制为 stereo ↔ stereo
