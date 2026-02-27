#pragma once

#ifdef WGC_CAPTURE_DLL_EXPORTS
#define WGC_API __declspec(dllexport)
#else
#define WGC_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 窗口枚举
WGC_API int EnumerateWindows(char*** titles, char*** classNames, int* count);
WGC_API void FreeStringArray(char** array, int count);

// 连续捕获 API
WGC_API int StartContinuousCapture(const char* title, const char* className);
WGC_API int GetLatestFrame(unsigned char** imageData, int* width, int* height);
WGC_API void FreeImageData(unsigned char* data);
WGC_API void StopContinuousCapture();
WGC_API int IsCapturing();
WGC_API int GetFrameCount();

// 错误信息
WGC_API const char* GetLastErrorMsg();

#ifdef __cplusplus
}
#endif
