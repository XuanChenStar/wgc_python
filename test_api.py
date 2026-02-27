"""
wgc-python 测试脚本
测试: 单张截图、Base64编码、窗口枚举等
"""
from wgc_python import *
import numpy as np
import base64
import time
import os

def test_enumerate_windows():
    """测试窗口枚举"""
    print("=" * 50)
    print("测试: 窗口枚举")
    print("=" * 50)
    
    windows = enumerate_windows()
    print(f"找到 {len(windows)} 个窗口\n")
    
    for i, (title, cls) in enumerate(windows[:15]):
        print(f"{i+1:2}. [{cls:20}] {title[:40]}")
    
    return windows

def test_single_capture(title: str, class_name: str, save_path: str = None):
    """测试单张截图"""
    print("\n" + "=" * 50)
    print("测试: 单张截图")
    print("=" * 50)
    
    if not start_capture(title, class_name):
        print(f"启动捕获失败: {get_last_error()}")
        return None
    
    print(f"捕获已启动: {title}")
    
    result = None
    for _ in range(100):
        result = get_frame()
        if result:
            break
        time.sleep(0.01)
    
    stop_capture()
    
    if not result:
        print("获取帧失败")
        return None
    
    data, width, height = result
    print(f"截图成功: {width}x{height}, {len(data)} bytes")
    
    if save_path:
        import cv2
        img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
        img_bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
        success, buffer = cv2.imencode('.png', img_bgr)
        if success:
            with open(save_path, 'wb') as f:
                f.write(buffer)
            print(f"已保存: {save_path}")
        else:
            print(f"编码失败: {save_path}")
    
    return result

def test_base64_encode(title: str, class_name: str):
    """测试Base64编码"""
    print("\n" + "=" * 50)
    print("测试: Base64编码")
    print("=" * 50)
    
    if not start_capture(title, class_name):
        print(f"启动捕获失败: {get_last_error()}")
        return None
    
    result = None
    for _ in range(100):
        result = get_frame()
        if result:
            break
        time.sleep(0.01)
    
    stop_capture()
    
    if not result:
        print("获取帧失败")
        return None
    
    data, width, height = result
    
    import cv2
    img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
    img_bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
    
    _, buffer = cv2.imencode('.png', img_bgr)
    b64_str = base64.b64encode(buffer).decode('utf-8')
    
    print(f"Base64编码成功:")
    print(f"  图像尺寸: {width}x{height}")
    print(f"  PNG大小: {len(buffer)} bytes")
    print(f"  Base64长度: {len(b64_str)} chars")
    print(f"  前100字符: {b64_str[:100]}...")
    
    return b64_str

def test_frame_count(title: str, class_name: str, duration: float = 3.0):
    """测试帧计数"""
    print("\n" + "=" * 50)
    print(f"测试: 帧计数 ({duration}秒)")
    print("=" * 50)
    
    if not start_capture(title, class_name):
        print(f"启动捕获失败: {get_last_error()}")
        return
    
    print("捕获中...")
    start_time = time.time()
    
    while time.time() - start_time < duration:
        time.sleep(0.5)
        elapsed = time.time() - start_time
        frames = get_frame_count()
        fps = frames / elapsed if elapsed > 0 else 0
        print(f"  {elapsed:.1f}s: {frames} frames, {fps:.1f} FPS")
    
    total_frames = get_frame_count()
    avg_fps = total_frames / duration
    print(f"\n总计: {total_frames} frames, 平均 {avg_fps:.1f} FPS")
    
    stop_capture()

def test_capture_status(title: str, class_name: str):
    """测试捕获状态"""
    print("\n" + "=" * 50)
    print("测试: 捕获状态")
    print("=" * 50)
    
    print(f"捕获前: is_capturing() = {is_capturing()}")
    
    if start_capture(title, class_name):
        print(f"捕获中: is_capturing() = {is_capturing()}")
        time.sleep(1)
        stop_capture()
    
    print(f"捕获后: is_capturing() = {is_capturing()}")

def main():
    test_enumerate_windows()

    target_title = "记事本"
    target_class = "Notepad"
    
    print(f"\n目标窗口: {target_title} ({target_class})")
    
    output_dir = os.path.dirname(__file__)
    screenshot_path = os.path.join(output_dir, "screenshot.png")
    
    test_single_capture(target_title, target_class, screenshot_path)
    test_base64_encode(target_title, target_class)
    test_capture_status(target_title, target_class)
    test_frame_count(target_title, target_class, duration=3.0)
    
    print("\n" + "=" * 50)
    print("所有测试完成")
    print("=" * 50)

if __name__ == "__main__":
    main()
