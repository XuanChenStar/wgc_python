# WGC窗口捕获模块开发计划

## 项目结构

```
wgc_capture/
├── wgc_capture_dll/          # C++ DLL项目
│   ├── pch.h                 # 预编译头
│   ├── WindowEnumerator.h/cpp    # 窗口枚举器
│   ├── WGCWindowCapture.h/cpp   # WGC窗口捕获核心
│   ├── ImageEncoder.h/cpp       # 图像编码器（Base64）
│   ├── WGCExport.h              # DLL导出接口
│   ├── WGCExport.cpp            # DLL导出实现
│   ├── wgc_capture.vcxproj      # 项目文件
│   └── packages.config          # NuGet依赖
├── p_c_capture_api.py       # Python接口封装
└── test_capture.py          # 测试程序
```

## 核心功能模块

### 1. WindowEnumerator（窗口枚举器）

* 枚举所有可见窗口

* 返回窗口标题和类名列表

* 过滤不可用窗口（参考WindowList逻辑）

### 2. WGCWindowCapture（WGC窗口捕获）

* 基于CaptureSnapshot的简化版本

* 按标题+类名查找窗口

* 执行单次捕获（不保存本地）

* 返回D3D11Texture2D数据

### 3. ImageEncoder（图像编码器）

* 将D3D11Texture2D转换为字节数组

* Base64编码功能

* 支持BGRA格式（B8G8R8A8UIntNormalized）

### 4. WGCExport（DLL导出接口）

导出以下C接口：

```cpp
// 窗口枚举
extern "C" __declspec(dllexport) int EnumerateWindows(char** titles, char** classNames, int maxCount);

// 窗口捕获
extern "C" __declspec(dllexport) int CaptureWindow(const char* title, const char* className, 
    unsigned char** imageData, int* width, int* height);

// 释放内存
extern "C" __declspec(dllexport) void FreeImageData(unsigned char* data);

// Base64编码
extern "C" __declspec(dllexport) int EncodeBase64(const unsigned char* data, int size, char** base64Str);

// 释放Base64字符串
extern "C" __declspec(dllexport) void FreeBase64String(char* str);
```

### 5. p\_c\_capture\_api.py（Python接口）

* 使用ctypes加载DLL

* 封装所有导出函数

* 提供友好的Python API

* 支持from p\_c\_capture\_api import \*风格

### 6. test\_capture.py（测试程序）

* 测试窗口枚举

* 测试窗口捕获

* 测试Base64编码

* 显示捕获的图像

## 技术要点

1. **DLL配置**：

   * ConfigurationType: DynamicLibrary

   * 导出符号使用\_\_declspec(dllexport)

   * 使用C接口避免C++名称修饰

2. **内存管理**：

   * DLL分配的内存由DLL释放

   * 使用CoTaskMemAlloc分配跨边界的内存

3. **WGC捕获**：

   * 使用CreateFreeThreaded避免DispatcherQueue要求

   * 使用CaptureSnapshot::TakeAsync进行单次捕获

   * 使用CopyBytesFromTexture提取像素数据

4. **Base64编码**：

   * 使用CryptBinaryToStringA进行Base64编码

   * 确保字符串以null结尾

5. **Python集成**：

   * 使用ctypes.CDLL加载DLL

   * 正确处理字符串编码（UTF-8）

   * 自动内存管理

## 实施步骤

1. 创建wgc\_capture\_dll项目结构
2. 实现WindowEnumerator模块
3. 实现WGCWindowCapture模块
4. 实现ImageEncoder模块
5. 实现WGCExport导出接口
6. 编译生成DLL
7. 创建p\_c\_capture\_api.py
8. 创建test\_capture.py
9. 测试验证所有功能

