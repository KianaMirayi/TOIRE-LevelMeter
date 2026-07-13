# Gemini — WebUI 工作区

这里是 TOIRE-LevelMeter 的前端 UI 层，使用 **React** 构建。

---

## 🎯 你的任务

用 React 实现电平表插件 UI，编译产物放入 `../Resources/webui/`，替换当前的原生 JUCE Graphics 临时渲染。

---

## 📡 通信接口（唯一的约定）

C++ 侧通过 `evaluateJavascript` 每 33ms（30Hz）调用一次：

```js
updateLevels(peakDb, rmsDb)
```

- `peakDb`: float，峰值电平，范围 -96.0 ~ 0.0 dB
- `rmsDb`:  float，RMS 电平，范围 -96.0 ~ 0.0 dB

**你只需要在 JS 里定义这个函数。** 其余全部由你自由发挥。

---

## 🎨 UI 需求

| 元素 | 说明 |
|---|---|
| PEAK 电平条 | 绿→黄→红渐变，`-96dB → 0%`, `0dB → 100%` |
| RMS 电平条 | 蓝色渐变，`-96dB → 0%`, `0dB → 100%` |
| CLIP 指示灯 | `peakDb >= -0.1` 时亮起红色 |
| 颜色参考 | 背景 `#1a1a2e`，面板 `#16213e`，轨 `#0f0f23`，绿 `#00c853`，黄 `#ffd600`，红 `#ff1744`，蓝起 `#2979ff`，蓝止 `#00e5ff` |

---

## 🗂️ 文件约定

| 文件 | 说明 |
|---|---|
| `Resources/webui/index.html` | **入口文件**，JUCE WebView 加载此文件 |
| `Resources/webui/` 其余文件 | JS/CSS 随意组织，index.html 引用即可 |
| 输出目标 | `../Resources/webui/`（相对于本 README） |

---

## ⚠️ 注意事项

1. **不要用 ES modules** — JUCE WebView 可能不支持 `import`，用 IIFE 或 `<script>` 直接引入
2. **不要依赖外部 CDN** — 离线环境下 DAW 加载不到
3. **保留 `updateLevels(peakDb, rmsDb)` 函数签名不变** — 这是 C++ → JS 的唯一桥接
4. **编译产物放 `Resources/webui/`** — CMake 的 POST_BUILD 会自动复制到 VST3/Standalone/Common Files

---

## 🔧 C++ 侧你不需要动的文件

- `Source/DSP/PluginProcessor.*` — DSP 核心
- `Source/DSP/LevelMeter/MeterComponent.*` — 电平检测算法
- `Source/Bridge/WebViewBridge.*` — 未来的 JS 桥接（暂时闲置）
- `CMakeLists.txt` — 构建系统

---

## 📦 交付

1. React 项目源码放在 `WebUI/` 下（或你另建仓库）
2. 编译后的静态文件放入 `Resources/webui/`
3. `index.html` 是入口，`updateLevels(peakDb, rmsDb)` 是桥接函数

完成后通知 Admin，让他跑一次 CMake 构建，webui 会自动部署到所有目标路径。
