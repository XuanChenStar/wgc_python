// wgc-python — Windows Graphics Capture for Python
// DLL export declarations
//
// Copyright (c) 2026 XuanChenxuan
// MIT License — see LICENSE file for details

#pragma once

#include <stdint.h>

#ifdef WGC_CAPTURE_DLL_EXPORTS
#define WGC_API __declspec(dllexport)
#else
#define WGC_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t CaptureHandle;

WGC_API int EnumerateWindows(char*** titles, char*** classNames, int* count);
WGC_API void FreeStringArray(char** array, int count);

WGC_API CaptureHandle StartCapture(const char* title, const char* className, int clientAreaOnly);
WGC_API CaptureHandle StartCaptureMonitorOfWindow(const char* title, const char* className);
WGC_API void SetCursorCaptureEnabled(CaptureHandle handle, int enabled);
WGC_API void DestroyCapture(CaptureHandle handle);
WGC_API int GetFrameMapped(CaptureHandle handle, unsigned char** imageData, int* width, int* height, int* rowPitch);
WGC_API void ReleaseMappedFrame(CaptureHandle handle);
WGC_API void StopCapture(CaptureHandle handle);
WGC_API void PauseCapture(CaptureHandle handle);
WGC_API void ResumeCapture(CaptureHandle handle);
WGC_API int IsCapturing(CaptureHandle handle);
WGC_API int IsPaused(CaptureHandle handle);
WGC_API int GetFrameCount(CaptureHandle handle);

WGC_API size_t GetActiveCaptureCount();
WGC_API const char* GetLastErrorMsg();

#ifdef __cplusplus
}
#endif
