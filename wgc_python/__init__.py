"""
wgc-python — 基于 Windows Graphics Capture 的 Python 窗口捕获库

支持多窗口并发捕获、客户区裁剪、按需捕获（Pause/Resume）、关闭光标捕获。

Copyright (c) 2026 XuanChenxuan
MIT License — see LICENSE file for details
"""
import ctypes
import os
import time
from typing import List, Tuple, Optional
import numpy as np

_dll = ctypes.CDLL(os.path.join(os.path.dirname(__file__), 'wgc_python.dll'))

_dll.StartCapture.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
_dll.StartCapture.restype = ctypes.c_uint64
if hasattr(_dll, 'SetCursorCaptureEnabled'):
    _dll.SetCursorCaptureEnabled.argtypes = [ctypes.c_uint64, ctypes.c_int]
    _dll.SetCursorCaptureEnabled.restype = None
_dll.DestroyCapture.argtypes = [ctypes.c_uint64]
_dll.GetFrameMapped.argtypes = [ctypes.c_uint64, ctypes.POINTER(ctypes.POINTER(ctypes.c_ubyte)),
                                ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int),
                                ctypes.POINTER(ctypes.c_int)]
_dll.GetFrameMapped.restype = ctypes.c_int
_dll.ReleaseMappedFrame.argtypes = [ctypes.c_uint64]
_dll.StopCapture.argtypes = [ctypes.c_uint64]
_dll.PauseCapture.argtypes = [ctypes.c_uint64]
_dll.ResumeCapture.argtypes = [ctypes.c_uint64]
_dll.IsCapturing.argtypes = [ctypes.c_uint64]
_dll.IsCapturing.restype = ctypes.c_int
_dll.IsPaused.argtypes = [ctypes.c_uint64]
_dll.IsPaused.restype = ctypes.c_int
_dll.GetFrameCount.argtypes = [ctypes.c_uint64]
_dll.GetFrameCount.restype = ctypes.c_int
_dll.GetActiveCaptureCount.restype = ctypes.c_size_t
_dll.GetLastErrorMsg.restype = ctypes.c_char_p
_dll.EnumerateWindows.argtypes = [ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)),
                                ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)), ctypes.POINTER(ctypes.c_int)]
_dll.EnumerateWindows.restype = ctypes.c_int
_dll.FreeStringArray.argtypes = [ctypes.POINTER(ctypes.c_char_p), ctypes.c_int]


def enumerate_windows() -> List[Tuple[str, str]]:
    """枚举所有可见窗口，返回 (标题, 类名) 列表。失败时抛出 RuntimeError"""
    titles = ctypes.POINTER(ctypes.c_char_p)()
    classes = ctypes.POINTER(ctypes.c_char_p)()
    cnt = ctypes.c_int()
    if _dll.EnumerateWindows(ctypes.byref(titles), ctypes.byref(classes), ctypes.byref(cnt)) == 0:
        err = get_last_error()
        raise RuntimeError(err if err else "EnumerateWindows failed")
    result = [(titles[i].decode(), classes[i].decode()) for i in range(cnt.value)]
    _dll.FreeStringArray(titles, cnt.value)
    _dll.FreeStringArray(classes, cnt.value)
    return result


def get_last_error() -> str:
    msg = _dll.GetLastErrorMsg()
    return msg.decode() if msg else ""


def get_active_capture_count() -> int:
    return _dll.GetActiveCaptureCount()


class WindowCapture:
    """窗口捕获类 - 支持多线程多窗口并发捕获

    Args:
        title: 窗口标题
        class_name: 窗口类名
        client_area_only: 只捕获客户区（不含标题栏/边框），默认 True
        capture_cursor: 是否捕获鼠标光标，默认 True（需 Windows 10 2004+，旧系统自动忽略）
    """

    def __init__(self, title: str, class_name: str, client_area_only: bool = True,
                 capture_cursor: bool = True):
        self._h = _dll.StartCapture(title.encode(), class_name.encode(), int(client_area_only))
        if not self._h:
            raise RuntimeError(get_last_error())
        if not capture_cursor and hasattr(_dll, 'SetCursorCaptureEnabled'):
            _dll.SetCursorCaptureEnabled(self._h, 0)
    
    def __del__(self): self.close()
    def __enter__(self): return self
    def __exit__(self, *_): self.close()
    
    def get_frame(self) -> Optional[Tuple[int, int, int, int]]:
        """返回 (data_ptr, w, h, row_pitch) — 零拷贝 GPU 映射内存。

        用完必须调用 release_frame()，期间不要再次调用。"""
        if not self._h: return None
        data = ctypes.POINTER(ctypes.c_ubyte)()
        w, h, rp = ctypes.c_int(), ctypes.c_int(), ctypes.c_int()
        if _dll.GetFrameMapped(self._h, ctypes.byref(data), ctypes.byref(w), ctypes.byref(h), ctypes.byref(rp)) == 0:
            return None
        return (ctypes.cast(data, ctypes.c_void_p).value, w.value, h.value, rp.value)

    def release_frame(self):
        if self._h: _dll.ReleaseMappedFrame(self._h)
    
    def stop(self):
        if self._h: _dll.StopCapture(self._h)
    
    def pause(self):
        if self._h: _dll.PauseCapture(self._h)
    
    def resume(self):
        if self._h: _dll.ResumeCapture(self._h)

    def set_cursor_capture_enabled(self, enabled: bool):
        """运行时切换是否捕获鼠标光标"""
        if self._h and hasattr(_dll, 'SetCursorCaptureEnabled'):
            _dll.SetCursorCaptureEnabled(self._h, int(enabled))
    
    def is_capturing(self) -> bool:
        return bool(self._h and _dll.IsCapturing(self._h))
    
    def is_paused(self) -> bool:
        return bool(self._h and _dll.IsPaused(self._h))
    
    def get_frame_count(self) -> int:
        return _dll.GetFrameCount(self._h) if self._h else 0
    
    def close(self):
        if self._h:
            _dll.DestroyCapture(self._h)
            self._h = 0
    
    def capture_one(self, timeout: float = 0.5) -> Optional[np.ndarray]:
        """按需捕获一帧：Resume → 等待帧 → 拷贝 → Pause，捕获间隙零开销。

        返回 BGRA numpy 数组 (h, w, 4)，超时返回 None。"""
        self.resume()
        deadline = time.perf_counter() + timeout
        while time.perf_counter() < deadline:
            r = self.get_frame()
            if r:
                break
            time.sleep(0.002)
        else:
            self.pause()
            return None

        ptr, w, h, rp = r
        arr = np.ndarray((h, w, 4), dtype=np.uint8,
                        buffer=(ctypes.c_ubyte * (h * rp)).from_address(ptr),
                        strides=(rp, 4, 1))
        frame = arr.copy()
        self.release_frame()
        self.pause()
        return frame
    
    @property
    def handle(self) -> int: return self._h


__all__ = ['WindowCapture', 'enumerate_windows', 'get_last_error', 'get_active_capture_count']
