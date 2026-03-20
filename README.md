# wgc_python

[English](README_EN.md) | 简体中文

> **🚀 为 Python 自动化而生的窗口捕获库**  
> 180+ FPS 极致性能 · 零资源待机 · 无视遮挡 · API 极简

---

## 为什么选择 wgc_python？

### 🎯 专为自动化场景设计

你是否在为以下问题困扰？

- **mss/BitBlt**：无法捕获被遮挡或后台窗口
- **PrintWindow**：性能瓶颈，固定 26ms+ 延迟
- **其他 WGC 封装**：持续运行占用资源，频繁启停开销巨大（50ms+）

**wgc_python 通过独创的 Pause/Resume 机制，完美解决了这个矛盾：**

```python
# 传统方式：要么持续空转浪费资源，要么频繁启停承受延迟
start_capture()  # 50ms 开销
get_frame()      # 获取截图
stop_capture()   # 销毁会话
# 下次截图又要重新开始...

# wgc_python 方式：一次启动，按需截图，零开销待机
start_capture()          # 仅一次初始化
while running:
    resume_capture()     # <1ms 恢复
    frame = get_frame()  # 极速获取
    pause_capture()      # <1ms 暂停，进入零功耗待机
    # 处理你的业务逻辑...
stop_capture()
```

### 📊 性能对比

| 方案 | FPS | 后台捕获 | CPU 占用 | 频繁调用开销 |
|------|-----|---------|---------|-------------|
| python-mss / BitBlt | ~60 | ❌ | 高 | 低 |
| PrintWindow | ~38 | ✅ | 中 | 低 |
| 其他 WGC 封装 | 180+ | ✅ | 高（持续空转） | 极高 (50ms+) |
| **wgc_python** | **180+** | ✅ | **极低（Pause时为0）** | **极低 (<1ms)** |

### ✨ 核心优势

#### 1. 极致性能
- **180+ FPS** 高帧率捕获，比 PrintWindow 快 5 倍
- **双缓冲 Staging 纹理**：GPU 异步拷贝，读写互不阻塞
- **零拷贝友好**：`np.frombuffer` 直接映射，无额外内存拷贝

#### 2. 智能资源管理
- **Pause/Resume 机制**：暂停时 GPU 停止拷贝，CPU 占用归零
- **静态窗口自动降帧**：窗口内容不变时自动跳过帧处理
- **会话复用**：避免频繁创建/销毁 D3D 设备的开销

#### 3. 极简 API
- **同步调用模型**：无需处理回调、事件、队列
- **线程安全封装**：C++ 层处理所有多线程复杂性
- **开箱即用**：5 行代码即可开始捕获

#### 4. 无视遮挡
- 支持捕获被遮挡、最小化、后台窗口
- 完美适配游戏、桌面应用等各种场景

---

## 快速开始

### 安装

```bash
pip install wgc-python
```

### 基础用法

```python
from wgc_python import enumerate_windows, start_capture, get_frame, stop_capture

# 枚举所有窗口
windows = enumerate_windows()
for title, class_name in windows:
    print(f"{title} ({class_name})")

# 启动捕获
if start_capture("窗口标题", "窗口类名"):
    result = get_frame()
    if result:
        data, width, height = result
        # data: BGRA 格式字节
    stop_capture()
```

### 自动化最佳实践

```python
from wgc_python import *
import numpy as np
import cv2

# 程序启动时初始化一次
start_capture("游戏窗口", "UnityWndClass")

while True:
    # 需要截图时恢复
    resume_capture()
    
    result = get_frame()
    if result:
        data, width, height = result
        img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
        # 处理图像...
    
    # 处理完后立即暂停，释放资源
    pause_capture()
    
    # 执行其他业务逻辑...
    time.sleep(1)

stop_capture()
```

### 混合场景示例：频繁截图 + 大量空闲 + 后台运行

这是自动化脚本最典型的场景：需要频繁截图，但大部分时间在等待/处理逻辑，且目标窗口可能被遮挡或在后台。

```python
from wgc_python import *
import numpy as np
import cv2
import time

# 典型自动化场景：游戏AI、RPA脚本等
# 特点：截图频繁、空闲时间长、窗口可能后台运行

start_capture("目标窗口", "WindowClass")

def find_target(img):
    """模板匹配等图像处理"""
    # 你的图像识别逻辑...
    return None

def execute_action(target):
    """执行自动化操作"""
    # 你的自动化逻辑...
    time.sleep(0.5)  # 模拟操作耗时

def wait_for_condition():
    """等待某个条件满足"""
    time.sleep(2)  # 模拟等待

try:
    while True:
        # === 截图阶段：恢复捕获，快速获取 ===
        resume_capture()
        
        result = get_frame()
        if result:
            data, width, height = result
            img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
            
            target = find_target(img)
            if target:
                # === 立即暂停，释放资源 ===
                pause_capture()
                
                # 执行操作（此时捕获已暂停，零资源占用）
                execute_action(target)
            else:
                # 未找到目标，暂停后等待
                pause_capture()
                wait_for_condition()
        else:
            pause_capture()
            time.sleep(0.1)
            
except KeyboardInterrupt:
    pass

stop_capture()
```

**这种模式的优势：**
- 截图时：180+ FPS 极速获取
- 空闲时：CPU/GPU 占用归零
- 后台运行：窗口可被遮挡、最小化
- 会话复用：无需频繁启停，零延迟切换

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

---

## API 参考

```python
from wgc_python import (
    enumerate_windows,    # 枚举所有可见窗口
    start_capture,        # 启动捕获会话
    get_frame,            # 获取最新帧 (BGRA格式)
    stop_capture,         # 停止捕获会话
    is_capturing,         # 检查是否正在捕获
    get_frame_count,      # 获取已捕获帧数
    get_last_error,       # 获取最后错误信息
    pause_capture,        # 暂停捕获 (零资源待机)
    resume_capture,       # 恢复捕获
    is_paused,            # 检查是否已暂停
)
```

---

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
         Python numpy数组
```

### Pause/Resume 工作原理

```
┌─────────────────────────────────────────────────────┐
│                    FrameArrived 回调                 │
├─────────────────────────────────────────────────────┤
│  if (m_isPaused) return;  // 暂停时直接跳过          │
│                                                      │
│  // 正常处理：GPU → CPU 拷贝                         │
│  CopyResource(stagingTexture, surfaceTexture);      │
└─────────────────────────────────────────────────────┘
         ↑                              ↑
    Resume 时                    Pause 时
  m_isPaused = false           m_isPaused = true
  开始处理帧                    跳过所有帧处理
```

---

## 文件结构

```
wgc_python/
├── wgc_python_dll/               # C++ DLL 项目
│   ├── WGCWindowCapture.h/cpp    # WGC 捕获核心 (双缓冲)
│   ├── WGCExport.h/cpp           # DLL 导出
│   ├── D3DInterop.cpp            # D3D11 互操作
│   ├── WindowEnumerator.h/cpp    # 窗口枚举
│   └── packages/                 # NuGet 包
├── windows_capture/              # Python 包
│   └── __init__.py               # Python API
├── wgc_python.py                 # Python API (备用)
├── wgc_python.dll                # 编译后的 DLL
├── test.py                       # 实时显示测试
├── test_api.py                   # API功能测试
├── BUILD.md                      # 构建说明
└── requirements.txt              # Python 依赖
```

---

## 系统要求

- Windows 10 1903+ (Build 18362)
- Python 3.6+
- numpy, opencv-python

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