# WGC Capture DLL 构建说明

[English](BUILD_EN.md) | 简体中文

## 项目结构

```
wgc-python/
├── wgc_python.py            # Python API
├── test.py                  # 测试脚本
├── requirements.txt         # Python 依赖
├── wgc_python.dll           # 编译后的 DLL (需复制到此目录)
└── wgc_python_dll/          # C++ DLL 源码
    ├── WGCWindowCapture.h/cpp   # 核心捕获类 (双缓冲Staging纹理)
    ├── WGCExport.h/cpp          # DLL 导出接口
    ├── D3DInterop.cpp           # D3D11 互操作
    ├── WindowEnumerator.h/cpp   # 窗口枚举
    ├── pch.h                    # 预编译头
    └── packages/                # NuGet 包
```

## 前置条件

1. **Visual Studio 2022** (支持 C++20)
2. **Windows 11 SDK** (10.0.26100)
3. **NuGet 包** (已包含在 `packages/` 目录):
   - Microsoft.Windows.CppWinRT.2.0.240405.15
   - Microsoft.Windows.ImplementationLibrary.1.0.240803.1
   - Microsoft.Windows.SDK.CPP.10.0.26100.1

## 构建步骤

### 方法一：Visual Studio GUI

1. 打开 `wgc_python_dll/wgc_python_dll.sln`
2. 配置: `Release` | `x64`
3. 右键项目 → 生成
4. 复制 DLL 到 `wgc-python/` 目录

### 方法二：命令行 (MSBuild)

```powershell
# 在 wgc_python_dll 目录执行
msbuild wgc_python_dll.sln /p:Configuration=Release /p:Platform=x64

# 复制 DLL
copy "x64\Release\wgc_python.dll" "..\wgc_python.dll"
```

## 输出位置

| 配置 | DLL 路径 |
|------|----------|
| Release | `wgc_python_dll\x64\Release\wgc_python.dll` |
| Debug | `wgc_python_dll\x64\Debug\wgc_python.dll` |

## Python 使用

```bash
# 安装依赖
pip install numpy opencv-python

# 运行测试
python test.py
```

## DLL 导出函数

| 函数 | 说明 |
|------|------|
| `EnumerateWindows` | 枚举所有可见窗口 |
| `StartContinuousCapture` | 启动连续捕获 |
| `GetLatestFrame` | 获取最新帧 (BGRA) |
| `FreeImageData` | 释放图像数据 |
| `StopContinuousCapture` | 停止捕获 |
| `IsCapturing` | 是否正在捕获 |
| `GetFrameCount` | 已捕获帧数 |
| `GetLastErrorMsg` | 获取错误信息 |

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

## 常见问题

### 编译错误 C2065/C3536

检查 lambda 表达式中的类型声明，使用显式 WinRT 类型而非 `auto`。

### 找不到 DLL

确保 `wgc_python.dll` 与 `wgc_python.py` 在同一目录。

### 捕获失败

1. 确保目标窗口可见
2. 确保窗口标题/类名正确
3. 检查 `get_last_error()` 返回的错误信息
