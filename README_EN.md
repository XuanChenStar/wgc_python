# wgc-python

English | [简体中文](README.md)

> 🚀 High-performance Windows Graphics Capture for Python - 180+ FPS window capture with double-buffered staging texture

High-performance Windows Graphics Capture (WGC) Python module.

## Features

- High FPS continuous window capture (180+ FPS)
- Double-buffered staging texture async read
- Clean Python API
- Support for single screenshot, Base64 encoding

## File Structure

```
wgc-python/
├── wgc_python_dll/               # C++ DLL Project
│   ├── WGCWindowCapture.h/cpp    # WGC capture core (double-buffered)
│   ├── WGCExport.h/cpp           # DLL exports
│   ├── D3DInterop.cpp            # D3D11 interop
│   ├── WindowEnumerator.h/cpp    # Window enumeration
│   └── packages/                 # NuGet packages
├── wgc_python.py                 # Python API
├── wgc_python.dll                # Compiled DLL
├── test.py                       # Real-time display test
├── test_api.py                   # API function test
├── BUILD.md                      # Build instructions
└── requirements.txt              # Python dependencies
```

## Requirements

- Windows 10 1903+ (18362)
- Python 3.6+
- numpy, opencv-python

## Installation

```bash
pip install wgc-python
```

## Build DLL

See [BUILD.md](BUILD.md)

## API

```python
from wgc_python import (
    enumerate_windows,    # Enumerate windows
    start_capture,        # Start capture
    get_frame,            # Get frame
    stop_capture,         # Stop capture
    is_capturing,         # Is capturing
    get_frame_count,      # Frame count
    get_last_error,       # Error message
)
```

## Usage Examples

### Basic Usage

```python
from wgc_python import enumerate_windows, start_capture, get_frame, stop_capture

# Enumerate windows
windows = enumerate_windows()
for title, class_name in windows:
    print(f"{title} ({class_name})")

# Start capture
if start_capture("Window Title", "WindowClass"):
    # Get frame
    result = get_frame()
    if result:
        data, width, height = result
        # data: BGRA format bytes
    
    # Stop capture
    stop_capture()
```

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

### Save Screenshot

```python
from wgc_python import *
import numpy as np
import cv2

if start_capture("Window Title", "WindowClass"):
    result = get_frame()
    stop_capture()
    
    if result:
        data, width, height = result
        img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
        img_bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
        
        # Note: cv2.imwrite doesn't support non-ASCII paths
        success, buffer = cv2.imencode('.png', img_bgr)
        if success:
            with open("screenshot.png", 'wb') as f:
                f.write(buffer)
```

### Base64 Encoding

```python
from wgc_python import *
import numpy as np
import cv2
import base64

if start_capture("Window Title", "WindowClass"):
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

## Run Tests

```bash
# Install dependencies
pip install -r requirements.txt

# Real-time display test
python test.py

# API function test (screenshot, Base64, frame count, etc.)
python test_api.py
```

## Performance

| Scenario | FPS |
|----------|-----|
| Game window (dynamic) | 180+ |
| Static window | 1-2 (WGC auto-skips unchanged frames) |

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
             Python Display
```

## Troubleshooting

**DLL not found**: Ensure `wgc_python.dll` is in the `wgc_python/` directory

**Capture failed**: Check if window is visible, Windows version >= 1903

**Non-ASCII path save failed**: `cv2.imwrite` doesn't support non-ASCII paths, use `cv2.imencode` + `open().write()` instead

**Missing dependencies**: `pip install numpy opencv-python`

## Acknowledgments

This project is based on [robmikh/Win32CaptureSample](https://github.com/robmikh/Win32CaptureSample).
