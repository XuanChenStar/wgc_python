"""
wgc-python - Windows Graphics Capture Python 绑定
"""
import ctypes
import os
from typing import List, Tuple, Optional

class _WGCDLL:
    def __init__(self):
        dll_path = os.path.join(os.path.dirname(__file__), 'wgc_python.dll')
        if not os.path.exists(dll_path):
            raise FileNotFoundError(f"DLL not found: {dll_path}")
        
        self._dll = ctypes.CDLL(dll_path)
        self._setup_functions()
    
    def _setup_functions(self):
        self._dll.EnumerateWindows.argtypes = [
            ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)),
            ctypes.POINTER(ctypes.POINTER(ctypes.c_char_p)),
            ctypes.POINTER(ctypes.c_int)
        ]
        self._dll.EnumerateWindows.restype = ctypes.c_int

        self._dll.FreeStringArray.argtypes = [ctypes.POINTER(ctypes.c_char_p), ctypes.c_int]
        self._dll.FreeStringArray.restype = None

        self._dll.StartContinuousCapture.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        self._dll.StartContinuousCapture.restype = ctypes.c_int

        self._dll.GetLatestFrame.argtypes = [
            ctypes.POINTER(ctypes.POINTER(ctypes.c_ubyte)),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int)
        ]
        self._dll.GetLatestFrame.restype = ctypes.c_int

        self._dll.FreeImageData.argtypes = [ctypes.POINTER(ctypes.c_ubyte)]
        self._dll.FreeImageData.restype = None

        self._dll.StopContinuousCapture.argtypes = []
        self._dll.StopContinuousCapture.restype = None

        self._dll.IsCapturing.argtypes = []
        self._dll.IsCapturing.restype = ctypes.c_int

        self._dll.GetFrameCount.argtypes = []
        self._dll.GetFrameCount.restype = ctypes.c_int

        self._dll.GetLastErrorMsg.restype = ctypes.c_char_p

_dll = _WGCDLL()


def enumerate_windows() -> List[Tuple[str, str]]:
    """枚举所有可见窗口，返回 (标题, 类名) 列表"""
    titles_ptr = ctypes.POINTER(ctypes.c_char_p)()
    class_names_ptr = ctypes.POINTER(ctypes.c_char_p)()
    count = ctypes.c_int()

    if _dll._dll.EnumerateWindows(ctypes.byref(titles_ptr), ctypes.byref(class_names_ptr), ctypes.byref(count)) == 0:
        return []

    windows = []
    for i in range(count.value):
        title = titles_ptr[i].decode('utf-8')
        class_name = class_names_ptr[i].decode('utf-8')
        windows.append((title, class_name))

    _dll._dll.FreeStringArray(titles_ptr, count.value)
    _dll._dll.FreeStringArray(class_names_ptr, count.value)

    return windows


def start_capture(title: str, class_name: str) -> bool:
    """启动连续捕获"""
    return _dll._dll.StartContinuousCapture(title.encode('utf-8'), class_name.encode('utf-8')) != 0


def get_frame() -> Optional[Tuple[bytes, int, int]]:
    """获取最新帧，返回 (数据, 宽度, 高度) 或 None"""
    image_data_ptr = ctypes.POINTER(ctypes.c_ubyte)()
    width = ctypes.c_int()
    height = ctypes.c_int()

    if _dll._dll.GetLatestFrame(ctypes.byref(image_data_ptr), ctypes.byref(width), ctypes.byref(height)) == 0:
        return None

    if width.value <= 0 or height.value <= 0:
        _dll._dll.FreeImageData(image_data_ptr)
        return None

    image_size = width.value * height.value * 4
    image_data = ctypes.string_at(image_data_ptr, image_size)
    _dll._dll.FreeImageData(image_data_ptr)

    return image_data, width.value, height.value


def stop_capture():
    """停止捕获"""
    _dll._dll.StopContinuousCapture()


def is_capturing() -> bool:
    """是否正在捕获"""
    return _dll._dll.IsCapturing() != 0


def get_frame_count() -> int:
    """获取已捕获帧数"""
    return _dll._dll.GetFrameCount()


def get_last_error() -> str:
    """获取最后错误信息"""
    msg = _dll._dll.GetLastErrorMsg()
    return msg.decode('utf-8') if msg else ""


__all__ = [
    'enumerate_windows',
    'start_capture',
    'get_frame',
    'stop_capture',
    'is_capturing',
    'get_frame_count',
    'get_last_error'
]
