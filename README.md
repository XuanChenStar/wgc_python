# wgc-python

[English](README_EN.md) | 简体中文

> 🚀 高性能 Windows Graphics Capture Python 模块 - 180+ FPS 窗口捕获，双缓冲 Staging 纹理

高性能 Windows Graphics Capture (WGC) Python 模块。

## 功能

- 高帧率连续窗口捕获 (180+ FPS)
- 双缓冲 Staging 纹理异步读取
- 简洁的 Python API
- 支持单张截图、Base64编码

## 文件结构

```
wgc-python/
├── wgc_python_dll/               # C++ DLL 项目
│   ├── WGCWindowCapture.h/cpp    # WGC 捕获核心 (双缓冲)
│   ├── WGCExport.h/cpp           # DLL 导出
│   ├── D3DInterop.cpp            # D3D11 互操作
│   ├── WindowEnumerator.h/cpp    # 窗口枚举
│   └── packages/                 # NuGet 包
├── wgc_python.py                 # Python API
├── wgc_python.dll                # 编译后的 DLL
├── test.py                       # 实时显示测试
├── test_api.py                   # API功能测试
├── BUILD.md                      # 构建说明
└── requirements.txt              # Python 依赖
```

## 系统要求

- Windows 10 1903+ (18362)
- Python 3.6+
- numpy, opencv-python

## 安装

```bash
pip install wgc-python
```

## 构建 DLL

详见 [BUILD.md](BUILD.md)

## API

```python
from wgc_python import (
    enumerate_windows,    # 枚举窗口
    start_capture,        # 启动捕获
    get_frame,            # 获取帧
    stop_capture,         # 停止捕获
    is_capturing,         # 是否捕获中
    get_frame_count,      # 帧计数
    get_last_error,       # 错误信息
    pause_capture,        # 暂停捕获
    resume_capture,       # 恢复捕获
    is_paused,            # 是否已暂停
)
```

## 使用示例

### 基础用法

```python
from wgc_python import enumerate_windows, start_capture, get_frame, stop_capture

# 枚举窗口
windows = enumerate_windows()
for title, class_name in windows:
    print(f"{title} ({class_name})")

# 启动捕获
if start_capture("窗口标题", "窗口类名"):
    # 获取帧
    result = get_frame()
    if result:
        data, width, height = result
        # data: BGRA 格式字节
    
    # 停止捕获
    stop_capture()
```

### 实时显示

```python
from wgc_python import *
import numpy as np
import cv2

if start_capture("窗口标题", "窗口类名"):
    while True:
        result = get_frame()
        if result:
            data, width, height = result
            img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
            img_bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
            cv2.imshow("Capture", img_bgr)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    stop_capture()
    cv2.destroyAllWindows()
```

### 单张截图保存

```python
from wgc_python import *
import numpy as np
import cv2

if start_capture("窗口标题", "窗口类名"):
    result = get_frame()
    stop_capture()
    
    if result:
        data, width, height = result
        img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
        img_bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
        
        # 注意: cv2.imwrite 不支持中文路径
        success, buffer = cv2.imencode('.png', img_bgr)
        if success:
            with open("screenshot.png", 'wb') as f:
                f.write(buffer)
```

### Base64 编码

```python
from wgc_python import *
import numpy as np
import cv2
import base64

if start_capture("窗口标题", "窗口类名"):
    result = get_frame()
    stop_capture()
    
    if result:
        data, width, height = result
        img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
        img_bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
        
        _, buffer = cv2.imencode('.png', img_bgr)
        b64_str = base64.b64encode(buffer).decode('utf-8')
        print(f"Base64: {b64_str[:100]}...")
```

## 运行测试

```bash
# 安装依赖
pip install -r requirements.txt

# 实时显示测试
python test.py

# API功能测试 (截图、Base64、帧计数等)
python test_api.py
```

## 性能

| 场景 | FPS |
|------|-----|
| 游戏窗口 (动态) | 180+ |
| 静态窗口 | 1-2 (WGC自动跳过未变化帧) |

## 技术架构

```
WGC捕获 → GPU Surface纹理
              ↓
         CopyResource (GPU异步复制)
              ↓
    ┌─────────────────────────┐
    │  双缓冲Staging纹理       │
    │  [0] 写入 ←→ [1] 读取   │
    └─────────────────────────┘
              ↓
         Map/Unmap (CPU按需读取)
              ↓
         Python显示
```

## 故障排除

**DLL 未找到**: 确保 `wgc_python.dll` 在 `wgc_python/` 目录

**捕获失败**: 检查窗口是否可见，Windows 版本 >= 1903

**中文路径保存失败**: `cv2.imwrite` 不支持中文路径，使用 `cv2.imencode` + `open().write()` 代替

**依赖缺失**: `pip install numpy opencv-python`

## 鸣谢

本项目基于 [robmikh/Win32CaptureSample](https://github.com/robmikh/Win32CaptureSample) 开发。
