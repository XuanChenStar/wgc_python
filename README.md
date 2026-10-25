# wgc_python

[English](#english) | 简体中文

[![PyPI](https://img.shields.io/pypi/v/wgc-python)](https://pypi.org/project/wgc-python/)
[![Python](https://img.shields.io/pypi/pyversions/wgc-python)](https://pypi.org/project/wgc-python/)
[![License](https://img.shields.io/pypi/l/wgc-python)](LICENSE)

> **🚀 为 Python 自动化而生的窗口捕获库**  
> 高帧率捕获 · 零资源待机 · 无视遮挡 · API 极简

---

## 为什么选择 wgc_python？

### 🎯 专为自动化场景设计

你是否在为以下问题困扰？

- **mss/BitBlt**：无法捕获被遮挡或后台窗口
- **PrintWindow**：性能瓶颈，固定 26ms+ 延迟
- **其他 WGC 封装**：持续运行占用资源，频繁启停开销巨大（50ms+）

**wgc_python 通过 Pause/Resume 机制解决了这个矛盾：**

```python
# 传统方式：要么持续空转浪费资源，要么频繁启停承受延迟
start_capture()  # 50ms 开销
get_frame()      # 获取截图
stop_capture()   # 销毁会话（50ms）
# 下次截图又要重新开始...

# wgc_python 方式：一次启动，按需截图，零开销待机
with WindowCapture("窗口", "类名") as cap:
    while running:
        frame = cap.capture_one()  # auto Resume → 等待帧 → 拷贝 → Pause
        # 处理图像...
```

### 📊 性能对比

| 方案 | FPS | 后台捕获 | CPU 占用 | 频繁切换开销 | 暂停后 GPU 占用 |
|------|-----|---------|---------|-------------|----------------|
| python-mss / BitBlt | ~60 | ❌ | 高 | 低 | N/A (无暂停概念) |
| PrintWindow | ~38 | ✅ | 中 | 低 | N/A (每次调用即捕获) |
| 其他 WGC 封装 | 高 | ✅ | 高（持续空转） | 高 (启停会话开销大) | 高 (无法真正暂停) |
| **wgc_python** | **高** | ✅ | **极低（Pause时归零）** | **极低（原子标志位）** | **归零（无 D3D 操作）** |

> 表中为定性对比，具体数值因硬件、窗口内容与场景而异，建议以自己的实测为准。

### ✨ 核心优势

#### 1. 高帧率
- WGC 直接捕获 GPU 合成输出，不逐帧截屏，帧率上限远高于 PrintWindow 等 GDI 方案
- **双缓冲 Staging 纹理**：GPU 异步拷贝，读写互不阻塞
- **零拷贝友好**：`np.ndarray(strides=...)` 直接从 GPU 映射内存构造视图

#### 2. 智能资源管理
- **Pause/Resume 软暂停**：不销毁不重建 WGC session，仅原子标志位跳过帧处理
- **capture_one() 自动管理**：Resume → 等待帧 → 拷贝 → Pause，间隙 GPU 驱动零开销
- **会话复用**：避免频繁创建/销毁 D3D 设备的开销

#### 3. 极简 API
- **capture_one()**：一行代码完成按需捕获，返回 numpy 数组
- **get_frame()**：零拷贝裸指针路径（高级使用）
- **线程安全**：C++ 层处理所有多线程复杂性

#### 4. 多开并发
- 同一进程内可同时创建多个捕获会话，互不干扰
- 每个会话独立 D3D11 设备 + 独立纹理 + 独立 WinRT session，完全隔离
- 支持同窗口多路并发捕获

#### 5. 客户区精准裁剪（默认不截取标题栏/边框）
- **默认 `client_area_only=True`**：只捕获窗口客户区内容，自动裁剪标题栏和边框，直接输出有效像素
- **设置 `client_area_only=False`**：捕获整个窗口（含标题栏和边框），满足 UI 记录场景
- DPI 感知：自动修正高 DPI 缩放偏移，裁剪精度像素级
- GPU 级裁剪：`CopySubresourceRegion` 在 GPU 上完成裁剪，不浪费带宽和 CPU

#### 6. 光标捕获开关
- **默认 `capture_cursor=True`**：画面包含鼠标光标，与常规录屏行为一致
- **设置 `capture_cursor=False`**：画面不含鼠标指针，适合自动化 / 数据采集场景（也可用 `set_cursor_capture_enabled()` 运行时切换）
- 需 Windows 10 2004 (19041) 及以上系统，旧系统自动忽略该选项

#### 7. 无视遮挡
- 支持捕获被遮挡、最小化、后台窗口
- 完美适配游戏、桌面应用等各种场景

---

## 快速开始

### 安装

已发布至 PyPI，直接 pip 安装即可：

```bash
pip install wgc_python
```

### 基础用法

```python
from wgc_python import WindowCapture, enumerate_windows

# 枚举所有窗口
for title, class_name in enumerate_windows():
    print(f"{title} ({class_name})")

# 按需捕获（推荐 —— 零开销待机）
with WindowCapture("窗口标题", "窗口类名") as cap:
    frame = cap.capture_one()     # BGRA numpy 数组，shape (h, w, 4)
    if frame is not None:
        print(f"捕获成功: {frame.shape}")

# 客户区裁剪演示
# 默认 client_area_only=True：只截取客户区，不含标题栏/边框
cap_client = WindowCapture("记事本", "Notepad")                      # 只截内容
cap_full  = WindowCapture("记事本", "Notepad", client_area_only=False)  # 含标题栏
frame_client = cap_client.capture_one()  # 只有编辑区
frame_full  = cap_full.capture_one()    # 含标题栏 + 菜单 + 编辑区
cap_client.close()
cap_full.close()

# 不捕获鼠标光标（默认 capture_cursor=True，保持旧版行为）
cap = WindowCapture("记事本", "Notepad", capture_cursor=False)
frame = cap.capture_one()               # 画面不含鼠标指针
cap.set_cursor_capture_enabled(True)    # 也支持运行时切换
cap.close()
```

### 自动化最佳实践

```python
from wgc_python import WindowCapture

cap = WindowCapture("游戏窗口", "UnityWndClass")

while True:
    frame = cap.capture_one(timeout=1.0)
    if frame is not None:
        # frame 是 BGRA numpy 数组，直接用于 OpenCV/模板匹配
        pass
    time.sleep(1)

cap.close()
```

### 显示器捕获（推荐全屏游戏场景使用）

捕获目标窗口**所在的显示器**而非窗口本身：捕获会话绑定显示器，**目标窗口在全屏/窗口之间切换不会使捕获会话失效**（窗口捕获模式下此场景会冻结于最后一帧）。

```python
from wgc_python import WindowCaptureMonitor

# 定位目标窗口所在的显示器并捕获其全部内容（无客户区裁剪）
cap = WindowCaptureMonitor("游戏窗口", "UnityWndClass")
frame = cap.capture_one()   # 整屏 BGRA 帧
cap.close()
```

适用场景：全屏游戏的长期自动化采集、目标窗口显示模式频繁变化的场景。需要裁剪客户区或排除其他窗口内容时，仍请使用 `WindowCapture`。

### 零拷贝高级用法

```python
from wgc_python import WindowCapture
import numpy as np
import ctypes

with WindowCapture("窗口", "类名") as cap:
    cap.resume()
    r = cap.get_frame()  # (ptr, w, h, row_pitch) — GPU 映射裸指针
    if r:
        ptr, w, h, rp = r
        arr = np.ndarray((h, w, 4), dtype=np.uint8,
                         buffer=(ctypes.c_ubyte * (h * rp)).from_address(ptr),
                         strides=(rp, 4, 1))
        # arr 是 GPU 内存的零拷贝视图
        cap.release_frame()
    cap.pause()
```

### 实时显示

```python
from wgc_python import WindowCapture
import cv2

with WindowCapture("窗口标题", "窗口类名") as cap:
    while True:
        frame = cap.capture_one()
        if frame is not None:
            cv2.imshow("Capture", cv2.cvtColor(frame, cv2.COLOR_BGRA2BGR))
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    cv2.destroyAllWindows()
```

---

## API 参考

```python
from wgc_python import (
    WindowCapture,             # 窗口捕获类（上下文管理器支持）
    enumerate_windows,         # 枚举所有可见窗口
    get_last_error,            # 获取最后错误信息（线程安全）
    get_active_capture_count,  # 获取活跃捕获数
)

# WindowCapture 类方法:
#   cap = WindowCapture(title, class_name, client_area_only=True, capture_cursor=True)
#
#   cap.capture_one(timeout=0.5)  -> np.ndarray | None  ★ 推荐
#      自动 Resume → 等待帧 → 拷贝为 numpy → Pause
#      捕获间隙 WGC 完全休眠，GPU 驱动零开销
#
#   cap.get_frame()              -> (ptr, w, h, row_pitch) | None
#   cap.release_frame()           # 释放 GPU 映射
#   cap.pause()                   # 暂停捕获（零资源待机）
#   cap.resume()                  # 恢复捕获
#   cap.set_cursor_capture_enabled(enabled)  # 运行时切换光标捕获
#   cap.stop()                    # 停止帧到达
#   cap.close()                   # 销毁会话
#   cap.is_capturing()  -> bool
#   cap.is_paused()     -> bool
#   cap.get_frame_count()  -> int
#   cap.handle             -> int (DLL handle)
```

---

## 技术架构

```
WGC捕获 → GPU Surface纹理
              │
     ┌────────▼────────┐
     │  FrameArrived    │
     │  if pausing → ↑  │  ← Pause时直接返回，零 D3D 操作
     └────────┬─────────┘
              │
         CopyResource (GPU异步复制)
              ↓
    ┌─────────────────────────┐
    │  双缓冲Staging纹理       │
    │  [0] 写入 ←→ [1] 读取   │
    │  m_textureInUse 防冲撞   │
    └─────────────────────────┘
              ↓
         Map (永久映射 GPU 内存)
              ↓
    ┌────── 零拷贝输出 ───────┐
    │ get_frame()              │
    │ 返回裸指针 → numpy零拷贝  │
    │ 需手动 release_frame()    │
    └──────────────────────────┘

    ┌────── 一键捕获 ──────────┐
    │ capture_one()            │
    │ auto Pause/Resume        │
    │ 返回 numpy 数组          │
    │ 间隙 GPU 驱动零开销      │
    └──────────────────────────┘
```

### Pause/Resume 工作原理

```
  用户调用 cap.pause()
         │
    m_isPaused = true   ◄──── 原子标志位，微秒级
    m_readableStagingIndex = -1
         │
    ┌────▼────────────────────────────────────────────┐
    │                FrameArrived 回调（WGC 仍会触发）  │
    │                                                  │
    │  lock(mutex);                                    │
    │  if (m_isPaused) return;     // ← 纯CPU判断，跳过│
    │  // ↓ 以下只在 resume 后执行 ↓                   │
    │  CopyResource(staging, frame);                   │
    │  m_readableStagingIndex = idx;                   │
    │  unlock(mutex);                                  │
    └────▲────────────────┬───────────────────────────┘
         │                │
  用户调用 cap.resume()  MapFrame 检查 readableStagingIndex
  m_isPaused = false      <0 → 最近帧尚未就绪，返回 false
  
  不销毁 WGC session / 不重建 D3D 设备 / 不重新注册回调
  → 恢复零延迟，无突刺
```

---

## 文件结构

```
wgc_python/
├── wgc_python/                   # Python 包
│   ├── __init__.py               # Python API（ctypes FFI）
│   └── wgc_python.dll            # 编译后的 DLL
├── wgc_python_dll/               # C++ DLL 项目
│   ├── WGCWindowCapture.h/cpp    # WGC 捕获核心（双缓冲 + 零拷贝）
│   ├── WGCExport.h/cpp           # DLL 导出（含线程安全错误处理）
│   ├── D3DInterop.cpp            # D3D11 设备互操作
│   ├── WindowEnumerator.h/cpp    # 窗口枚举
│   ├── pch.h                     # 预编译头
│   └── packages/                 # NuGet 包
├── test.py                       # 功能测试
├── demon.py                      # 多线程实时显示示例
├── pyproject.toml                # pip 构建配置
├── BUILD.md / BUILD_EN.md        # 构建说明（中/英）
├── README.md / README_EN.md      # 使用文档（中/英）
├── CONTRIBUTING.md               # 贡献指南
├── CODE_OF_CONDUCT.md            # 行为准则
├── LICENSE                       # MIT 许可证
└── requirements.txt              # Python 依赖
```

---

## 系统要求

- Windows 10 1903+ (Build 18362)，光标捕获开关需 2004+ (Build 19041)
- Python 3.8+

---

## 构建 DLL

详见 [BUILD.md](BUILD.md)

---

## 故障排除

| 问题 | 解决方案 |
|------|---------|
| DLL 未找到 | 确保 `wgc_python.dll` 在正确位置 |
| 捕获失败 | 检查窗口是否可见，Windows 版本 >= 1903 |
| 中文路径保存失败 | 使用 `cv2.imencode` + `open().write()` 代替 `cv2.imwrite` |
| 依赖缺失 | `pip install numpy opencv-python` |

---

## 适用场景

- ✅ 游戏 AI / 自动化脚本
- ✅ RPA 流程自动化
- ✅ 屏幕录制 / 直播
- ✅ UI 自动化测试
- ✅ 计算机视觉应用

---

## 鸣谢

本项目基于 [robmikh/Win32CaptureSample](https://github.com/robmikh/Win32CaptureSample) 开发。

---

## License

MIT License

---

<a id="english"></a>
## English

### 🎯 Designed for Automation Scenarios

Are you struggling with these problems?

- **mss/BitBlt**: Cannot capture occluded or background windows
- **PrintWindow**: Performance bottleneck, fixed 26ms+ latency
- **Other WGC wrappers**: Continuous resource consumption, huge overhead for frequent start/stop (50ms+)

**wgc_python solves this dilemma with its Pause/Resume mechanism:**

```python
# Traditional approach: Either waste resources or suffer latency
start_capture()  # 50ms overhead
get_frame()      # Get screenshot
stop_capture()   # Destroy session (50ms)

# wgc_python approach: One-time init, on-demand capture, zero-overhead standby
with WindowCapture("Window", "Class") as cap:
    while running:
        frame = cap.capture_one()  # auto Resume → wait → copy → Pause
        # Process image...
```

### 📊 Performance Comparison

| Solution | FPS | Background Capture | CPU Usage | Toggle Overhead | GPU When Paused |
|----------|-----|-------------------|-----------|----------------|-----------------|
| python-mss / BitBlt | ~60 | ❌ | High | Low | N/A (no pause) |
| PrintWindow | ~38 | ✅ | Medium | Low | N/A (per-call) |
| Other WGC wrappers | High | ✅ | High (continuous) | High (session start/stop) | High (can't truly pause) |
| **wgc_python** | **High** | ✅ | **Near Zero (when paused)** | **Very Low (atomic flag)** | **Zero (no D3D ops)** |

> Qualitative comparison only; actual numbers vary by hardware, window content, and workload — benchmark on your own setup.

### ✨ Core Advantages

#### 1. High Frame Rate
- WGC captures GPU composition output directly instead of per-frame GDI screenshots — frame rate ceiling far above PrintWindow-style approaches
- **Double-buffered Staging Texture**: GPU async copy, read/write non-blocking
- **Zero-copy path**: `np.ndarray(strides=...)` directly from GPU-mapped memory

#### 2. Smart Resource Management
- **Pause/Resume soft-pause**: Atomic flag only, no WGC session teardown
- **capture_one() auto management**: Resume → wait → copy → Pause, zero GPU driver overhead between captures
- **Session Reuse**: No frequent D3D device creation/destruction

#### 3. Minimalist API
- **capture_one()**: One-line on-demand capture, returns numpy array
- **get_frame()**: Zero-copy raw pointer path (advanced)
- **Thread safe**: C++ handles all multi-threading complexity

#### 4. Multi-Instance Capture
- Create multiple capture sessions simultaneously within one process
- Each session has its own D3D11 device, staging textures, and WinRT session — fully isolated
- Supports concurrent capture of the same window

#### 5. Client Area Precision (No Title Bar by Default)
- **Default `client_area_only=True`**: captures only window client area, automatically crops title bar and borders
- **Set `client_area_only=False`**: captures entire window including title bar and borders, for UI recording
- DPI-aware: automatic high-DPI scaling correction for pixel-perfect cropping
- GPU-level cropping via `CopySubresourceRegion`, no wasted bandwidth or CPU

#### 6. Cursor Capture Toggle
- **Default `capture_cursor=True`**: frame includes the mouse cursor, same as typical screen recording
- **Set `capture_cursor=False`**: frame excludes the mouse pointer, ideal for automation / data collection (also toggleable at runtime via `set_cursor_capture_enabled()`)
- Requires Windows 10 2004 (19041) or later; silently ignored on older systems

#### 7. Capture Behind Windows
- Supports capturing occluded, minimized, and background windows
- Perfect for games, desktop apps, and various scenarios

---

## Quick Start

### Installation

Published on PyPI — install directly with pip:

```bash
pip install wgc_python
```

### Basic Usage

```python
from wgc_python import WindowCapture, enumerate_windows

# Enumerate all windows
for title, class_name in enumerate_windows():
    print(f"{title} ({class_name})")

# On-demand capture (recommended — zero-resource standby)
with WindowCapture("Window Title", "WindowClass") as cap:
    frame = cap.capture_one()     # BGRA numpy array, shape (h, w, 4)
    if frame is not None:
        print(f"Captured: {frame.shape}")

# Client area demo
# Default client_area_only=True: content only, no title bar/borders
cap_client = WindowCapture("Notepad", "Notepad")                       # content only
cap_full  = WindowCapture("Notepad", "Notepad", client_area_only=False) # with title bar
frame_client = cap_client.capture_one()  # edit area only
frame_full  = cap_full.capture_one()    # title bar + menu + edit area
cap_client.close()
cap_full.close()

# Disable mouse cursor capture (default capture_cursor=True, same as previous versions)
cap = WindowCapture("Notepad", "Notepad", capture_cursor=False)
frame = cap.capture_one()               # frame does not contain the mouse pointer
cap.set_cursor_capture_enabled(True)    # runtime toggle is also supported
cap.close()
```

### Best Practice for Automation

```python
from wgc_python import WindowCapture

cap = WindowCapture("Game Window", "UnityWndClass")

while True:
    frame = cap.capture_one(timeout=1.0)
    if frame is not None:
        # frame is BGRA numpy array, ready for OpenCV/template matching
        pass
    time.sleep(1)

cap.close()
```

### Zero-Copy Advanced Usage

```python
from wgc_python import WindowCapture
import numpy as np
import ctypes

with WindowCapture("Window", "Class") as cap:
    cap.resume()
    r = cap.get_frame()  # (ptr, w, h, row_pitch) — GPU mapped pointer
    if r:
        ptr, w, h, rp = r
        arr = np.ndarray((h, w, 4), dtype=np.uint8,
                         buffer=(ctypes.c_ubyte * (h * rp)).from_address(ptr),
                         strides=(rp, 4, 1))
        # arr is a zero-copy view into GPU-mapped memory
        cap.release_frame()
    cap.pause()
```

### Real-time Display

```python
from wgc_python import WindowCapture
import cv2

with WindowCapture("Window Title", "WindowClass") as cap:
    while True:
        frame = cap.capture_one()
        if frame is not None:
            cv2.imshow("Capture", cv2.cvtColor(frame, cv2.COLOR_BGRA2BGR))
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    cv2.destroyAllWindows()
```

---

## API Reference

```python
from wgc_python import (
    WindowCapture,             # Window capture class (context manager support)
    enumerate_windows,         # Enumerate all visible windows
    get_last_error,            # Get last error message (thread-safe)
    get_active_capture_count,  # Get active capture count
)

# WindowCapture methods:
#   cap = WindowCapture(title, class_name, client_area_only=True, capture_cursor=True)
#
#   cap.capture_one(timeout=0.5)  -> np.ndarray | None  ★ recommended
#      Auto Resume → wait for frame → copy to numpy → Pause
#      WGC fully dormant between captures, zero GPU driver overhead
#
#   cap.get_frame()              -> (ptr, w, h, row_pitch) | None
#   cap.release_frame()           # Release GPU mapping
#   cap.pause()                   # Pause capture (zero-resource standby)
#   cap.resume()                  # Resume capture
#   cap.set_cursor_capture_enabled(enabled)  # Toggle cursor capture at runtime
#   cap.stop()                    # Stop frame arrival
#   cap.close()                   # Destroy session
#   cap.is_capturing()  -> bool
#   cap.is_paused()     -> bool
#   cap.get_frame_count()  -> int
#   cap.handle             -> int (DLL handle)
```

---

## Technical Architecture

```
WGC Capture → GPU Surface Texture
                   │
         ┌─────────▼──────────┐
         │   FrameArrived     │
         │  if pausing → ↑   │  ← Paused: return directly, zero D3D ops
         └─────────┬──────────┘
                   │
              CopyResource (GPU async copy)
                   │
     ┌─────────────────────────────┐
     │  Double-buffered Staging    │
     │  [0] Write ←→ [1] Read      │
     │  m_textureInUse anti-collide │
     └─────────────────────────────┘
                   │
              Map (permanently mapped GPU memory)
                   │
     ┌────── Zero-Copy Output ────┐
     │ get_frame()                 │
     │ raw ptr → numpy zero-copy   │
     │ requires release_frame()    │
     └─────────────────────────────┘

     ┌────── One-Click Capture ───┐
     │ capture_one()               │
     │ auto Pause/Resume           │
     │ returns numpy array         │
     │ zero GPU overhead when idle │
     └─────────────────────────────┘
```

### How Pause/Resume Works

```
  User calls cap.pause()
         │
    m_isPaused = true   ◄──── atomic flag, microsecond-level
    m_readableStagingIndex = -1
         │
    ┌────▼────────────────────────────────────────────┐
    │           FrameArrived Callback (WGC still fires)│
    │                                                  │
    │  lock(mutex);                                    │
    │  if (m_isPaused) return;     // ← pure CPU, skip│
    │  // ↓ below runs only after resume ↓             │
    │  CopyResource(staging, frame);                   │
    │  m_readableStagingIndex = idx;                   │
    │  unlock(mutex);                                  │
    └────▲────────────────┬───────────────────────────┘
         │                │
  User calls cap.resume()  MapFrame checks readableStagingIndex
  m_isPaused = false       <0 → no frame ready → returns false

  No WGC session destroy / no D3D device recreate / no callback re-register
  → zero-latency resume, no spikes
```

---

## File Structure

```
wgc_python/
├── wgc_python/                   # Python package
│   ├── __init__.py               # Python API (ctypes FFI)
│   └── wgc_python.dll            # Compiled DLL
├── wgc_python_dll/               # C++ DLL Project
│   ├── WGCWindowCapture.h/cpp    # Capture core (double-buffered + zero-copy)
│   ├── WGCExport.h/cpp           # DLL exports (thread-safe error handling)
│   ├── D3DInterop.cpp            # D3D11 device interop
│   ├── WindowEnumerator.h/cpp    # Window enumeration
│   ├── pch.h                     # Precompiled header
│   └── packages/                 # NuGet packages
├── test.py                       # Functional tests
├── demon.py                      # Threaded realtime display example
├── pyproject.toml                # pip build config
├── BUILD.md / BUILD_EN.md        # Build instructions (CN/EN)
├── README.md / README_EN.md      # Usage docs (CN/EN)
├── CONTRIBUTING.md               # Contributing guide
├── CODE_OF_CONDUCT.md            # Code of conduct
├── LICENSE                       # MIT License
└── requirements.txt              # Python dependencies
```

---

## Build DLL

See [BUILD.md](BUILD.md)

---

## Requirements

- Windows 10 1903+ (Build 18362); cursor capture toggle requires 2004+ (Build 19041)
- Python 3.8+

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| DLL not found | Ensure `wgc_python.dll` is in the correct location |
| Capture failed | Check if window is visible, Windows version >= 1903 |
| Non-ASCII path save failed | Use `cv2.imencode` + `open().write()` instead of `cv2.imwrite` |
| Missing dependencies | `pip install numpy opencv-python` |

---

## Use Cases

- ✅ Game AI / Automation Scripts
- ✅ RPA Process Automation
- ✅ Screen Recording / Streaming
- ✅ UI Automation Testing
- ✅ Computer Vision Applications

---

## Acknowledgments

This project is based on [robmikh/Win32CaptureSample](https://github.com/robmikh/Win32CaptureSample).

---

## License

MIT License
