# wgc_python

English | [简体中文](README.md)

> **🚀 Window Capture Library Built for Python Automation**  
> High Frame Rate · Zero-Resource Standby · Capture Behind Windows · Minimalist API

---

## Why wgc_python?

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

```bash
pip install wgc-python
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
