# WGC Capture DLL Build Instructions

English | [简体中文](BUILD.md)

## Project Structure

```
wgc-python/
├── wgc_python.py            # Python API
├── test.py                  # Test script
├── requirements.txt         # Python dependencies
├── wgc_python.dll           # Compiled DLL (copy to this directory)
└── wgc_python_dll/          # C++ DLL source
    ├── WGCWindowCapture.h/cpp   # Core capture class (double-buffered staging texture)
    ├── WGCExport.h/cpp          # DLL export interface
    ├── D3DInterop.cpp           # D3D11 interop
    ├── WindowEnumerator.h/cpp   # Window enumeration
    ├── pch.h                    # Precompiled header
    └── packages/                # NuGet packages
```

## Prerequisites

1. **Visual Studio 2022** (C++20 support)
2. **Windows 11 SDK** (10.0.26100)
3. **NuGet Packages** (included in `packages/` directory):
   - Microsoft.Windows.CppWinRT.2.0.240405.15
   - Microsoft.Windows.ImplementationLibrary.1.0.240803.1
   - Microsoft.Windows.SDK.CPP.10.0.26100.1

## Build Steps

### Method 1: Visual Studio GUI

1. Open `wgc_python_dll/wgc_python_dll.sln`
2. Configuration: `Release` | `x64`
3. Right-click project → Build
4. Copy DLL to `wgc-python/` directory

### Method 2: Command Line (MSBuild)

```powershell
# Run from wgc_python_dll directory
msbuild wgc_python_dll.sln /p:Configuration=Release /p:Platform=x64

# Copy DLL
copy "x64\Release\wgc_python.dll" "..\wgc_python.dll"
```

## Output Location

| Configuration | DLL Path |
|---------------|----------|
| Release | `wgc_python_dll\x64\Release\wgc_python.dll` |
| Debug | `wgc_python_dll\x64\Debug\wgc_python.dll` |

## Python Usage

```bash
# Install dependencies
pip install numpy opencv-python

# Run test
python test.py
```

## DLL Export Functions

| Function | Description |
|----------|-------------|
| `EnumerateWindows` | Enumerate all visible windows |
| `StartContinuousCapture` | Start continuous capture |
| `GetLatestFrame` | Get latest frame (BGRA) |
| `FreeImageData` | Free image data |
| `StopContinuousCapture` | Stop capture |
| `IsCapturing` | Is currently capturing |
| `GetFrameCount` | Captured frame count |
| `GetLastErrorMsg` | Get error message |

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

## Common Issues

### Compile Error C2065/C3536

Check type declarations in lambda expressions, use explicit WinRT types instead of `auto`.

### DLL Not Found

Ensure `wgc_python.dll` is in the same directory as `wgc_python.py`.

### Capture Failed

1. Ensure target window is visible
2. Ensure window title/class name is correct
3. Check error message from `get_last_error()`
