# wgc_python

English | [简体中文](README.md)

> **🚀 Window Capture Library Built for Python Automation**  
> 180+ FPS Performance · Zero-Resource Standby · Capture Behind Windows · Minimalist API

---

## Why wgc_python?

### 🎯 Designed for Automation Scenarios

Are you struggling with these problems?

- **mss/BitBlt**: Cannot capture occluded or background windows
- **PrintWindow**: Performance bottleneck, fixed 26ms+ latency
- **Other WGC wrappers**: Continuous resource consumption, huge overhead for frequent start/stop (50ms+)

**wgc_python solves this dilemma with its innovative Pause/Resume mechanism:**

```python
# Traditional approach: Either waste resources or suffer latency
start_capture()  # 50ms overhead
get_frame()      # Get screenshot
stop_capture()   # Destroy session
# Next screenshot needs to start all over...

# wgc_python approach: One-time init, on-demand capture, zero-overhead standby
start_capture()          # Initialize once
while running:
    resume_capture()     # <1ms resume
    frame = get_frame()  # Lightning fast
    pause_capture()      # <1ms pause, enter zero-power standby
    # Process your business logic...
stop_capture()
```

### 📊 Performance Comparison

| Solution | FPS | Background Capture | CPU Usage | Frequent Call Overhead |
|----------|-----|-------------------|-----------|----------------------|
| python-mss / BitBlt | ~60 | ❌ | High | Low |
| PrintWindow | ~38 | ✅ | Medium | Low |
| Other WGC wrappers | 180+ | ✅ | High (continuous) | Very High (50ms+) |
| **wgc_python** | **180+** | ✅ | **Near Zero (when paused)** | **Very Low (<1ms)** |

### ✨ Core Advantages

#### 1. Extreme Performance
- **180+ FPS** high frame rate capture, 5x faster than PrintWindow
- **Double-buffered Staging Texture**: GPU async copy, read/write non-blocking
- **Zero-copy Friendly**: `np.frombuffer` direct mapping, no extra memory copy

#### 2. Smart Resource Management
- **Pause/Resume Mechanism**: GPU stops copying when paused, CPU usage drops to zero
- **Auto Frame Skipping**: Automatically skips processing when window content is static
- **Session Reuse**: Avoids overhead of frequent D3D device creation/destruction

#### 3. Minimalist API
- **Synchronous Call Model**: No callbacks, events, or queues to handle
- **Thread-Safe Encapsulation**: C++ layer handles all multi-threading complexity
- **Ready to Use**: Start capturing with just 5 lines of code

#### 4. Capture Behind Windows
- Supports capturing occluded, minimized, and background windows
- Perfect for games, desktop apps, and various scenarios

---

## Quick Start

### Installation

```bash
pip install wgc-python
```

### Basic Usage

```python
from wgc_python import enumerate_windows, start_capture, get_frame, stop_capture

# Enumerate all windows
windows = enumerate_windows()
for title, class_name in windows:
    print(f"{title} ({class_name})")

# Start capture
if start_capture("Window Title", "WindowClass"):
    result = get_frame()
    if result:
        data, width, height = result
        # data: BGRA format bytes
    stop_capture()
```

### Best Practice for Automation

```python
from wgc_python import *
import numpy as np
import cv2

# Initialize once at program startup
start_capture("Game Window", "UnityWndClass")

while True:
    # Resume when screenshot is needed
    resume_capture()
    
    result = get_frame()
    if result:
        data, width, height = result
        img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
        # Process image...
    
    # Pause immediately after processing to release resources
    pause_capture()
    
    # Execute other business logic...
    time.sleep(1)

stop_capture()
```

### Mixed Scenario: Frequent Capture + Long Idle + Background Running

This is the most typical scenario for automation scripts: frequent screenshots needed, but most time spent waiting/processing, and target window may be occluded or in background.

```python
from wgc_python import *
import numpy as np
import cv2
import time

# Typical automation scenario: Game AI, RPA scripts, etc.
# Features: Frequent screenshots, long idle time, window may run in background

start_capture("Target Window", "WindowClass")

def find_target(img):
    """Template matching or other image processing"""
    # Your image recognition logic...
    return None

def execute_action(target):
    """Execute automation action"""
    # Your automation logic...
    time.sleep(0.5)  # Simulate action time

def wait_for_condition():
    """Wait for a condition to be met"""
    time.sleep(2)  # Simulate waiting

try:
    while True:
        # === Capture phase: Resume, fast acquisition ===
        resume_capture()
        
        result = get_frame()
        if result:
            data, width, height = result
            img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
            
            target = find_target(img)
            if target:
                # === Immediately pause, release resources ===
                pause_capture()
                
                # Execute action (capture paused, zero resource usage)
                execute_action(target)
            else:
                # Target not found, pause and wait
                pause_capture()
                wait_for_condition()
        else:
            pause_capture()
            time.sleep(0.1)
            
except KeyboardInterrupt:
    pass

stop_capture()
```

**Advantages of this pattern:**
- During capture: 180+ FPS lightning fast
- During idle: CPU/GPU usage drops to zero
- Background running: Window can be occluded or minimized
- Session reuse: No frequent start/stop, zero-latency switching

### Real-time Display

```python
from wgc_python import *
import numpy as np
import cv2

if start_capture("Window Title", "WindowClass"):
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

## API Reference

```python
from wgc_python import (
    enumerate_windows,    # Enumerate all visible windows
    start_capture,        # Start capture session
    get_frame,            # Get latest frame (BGRA format)
    stop_capture,         # Stop capture session
    is_capturing,         # Check if capturing
    get_frame_count,      # Get captured frame count
    get_last_error,       # Get last error message
    pause_capture,        # Pause capture (zero-resource standby)
    resume_capture,       # Resume capture
    is_paused,            # Check if paused
)
```

---

## Technical Architecture

```
WGC Capture → GPU Surface Texture
                  ↓
             CopyResource (GPU async copy)
                  ↓
    ┌─────────────────────────────┐
    │  Double-buffered Staging    │
    │  [0] Write ←→ [1] Read      │
    └─────────────────────────────┘
                  ↓
             Map/Unmap (CPU on-demand read)
                  ↓
             Python numpy array
```

### How Pause/Resume Works

```
┌─────────────────────────────────────────────────────┐
│                    FrameArrived Callback             │
├─────────────────────────────────────────────────────┤
│  if (m_isPaused) return;  // Skip when paused        │
│                                                      │
│  // Normal processing: GPU → CPU copy               │
│  CopyResource(stagingTexture, surfaceTexture);      │
└─────────────────────────────────────────────────────┘
         ↑                              ↑
    On Resume                      On Pause
  m_isPaused = false           m_isPaused = true
  Start processing              Skip all frame processing
```

---

## File Structure

```
wgc_python/
├── wgc_python_dll/               # C++ DLL Project
│   ├── WGCWindowCapture.h/cpp    # WGC capture core (double-buffered)
│   ├── WGCExport.h/cpp           # DLL exports
│   ├── D3DInterop.cpp            # D3D11 interop
│   ├── WindowEnumerator.h/cpp    # Window enumeration
│   └── packages/                 # NuGet packages
├── windows_capture/              # Python package
│   └── __init__.py               # Python API
├── wgc_python.py                 # Python API (alternative)
├── wgc_python.dll                # Compiled DLL
├── test.py                       # Real-time display test
├── test_api.py                   # API function test
├── BUILD.md                      # Build instructions
└── requirements.txt              # Python dependencies
```

---

## Requirements

- Windows 10 1903+ (Build 18362)
- Python 3.6+
- numpy, opencv-python

---

## Build DLL

See [BUILD.md](BUILD.md)

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