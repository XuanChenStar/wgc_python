"""
wgc-python 功能测试

覆盖：枚举窗口 · 客户区/全窗口 · 零拷贝 · 多开 · Pause/Resume · 错误处理 · capture_one
"""
import ctypes
import sys
import time
import numpy as np
from wgc_python import WindowCapture, enumerate_windows, get_active_capture_count


def list_windows():
    wins = enumerate_windows()
    print(f"可见窗口数: {len(wins)}")
    for title, cls in wins[:10]:
        print(f"  [{cls}] {title}")
    return wins


def get_frames(cap, n=3):
    for i in range(n):
        r = cap.get_frame()
        if r:
            ptr, w, h, rp = r
            print(f"  帧 {i + 1}: {w}x{h}  pitch={rp}")
            cap.release_frame()
        else:
            print(f"  帧 {i + 1}: None")
        time.sleep(0.05)


def wait_frame(cap, timeout=1.0):
    """启动后第一帧需要几毫秒~上百毫秒才到达，轮询等待直到超时"""
    deadline = time.perf_counter() + timeout
    while time.perf_counter() < deadline:
        r = cap.get_frame()
        if r:
            return r
        time.sleep(0.01)
    return None


def demo_capture(title, cls, client_area_only):
    mode = "客户区" if client_area_only else "全窗口"
    print(f"\n--- {mode}捕获 (client_area_only={client_area_only}) ---")
    try:
        with WindowCapture(title, cls, client_area_only=client_area_only) as cap:
            get_frames(cap)
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_zero_copy(title, cls):
    print("\n--- 零拷贝路径 (get_frame + numpy strides) ---")
    try:
        with WindowCapture(title, cls) as cap:
            r = wait_frame(cap)
            if not r:
                print("  获取帧失败（1s 内未收到首帧）")
                return
            ptr, w, h, rp = r
            arr = np.ndarray((h, w, 4), dtype=np.uint8,
                             buffer=(ctypes.c_ubyte * (h * rp)).from_address(ptr),
                             strides=(rp, 4, 1))
            print(f"  numpy 零拷贝 shape: {arr.shape}, strides: {arr.strides}")
            cap.release_frame()
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_multi_instance(title, cls):
    print("\n--- 多开捕获（同一窗口并发两个会话） ---")
    try:
        cap1 = WindowCapture(title, cls)
        cap2 = WindowCapture(title, cls)
        print(f"  活跃捕获数: {get_active_capture_count()}")
        r1, r2 = wait_frame(cap1), wait_frame(cap2)
        if r1 and r2:
            print(f"  cap1 帧: {r1[1]}x{r1[2]}, cap2 帧: {r2[1]}x{r2[2]}")
            cap1.release_frame()
            cap2.release_frame()
        else:
            print(f"  取帧结果: cap1={'OK' if r1 else 'None'}, cap2={'OK' if r2 else 'None'}")
        cap1.close()
        cap2.close()
        print(f"  close 后活跃捕获数: {get_active_capture_count()}")
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_pause_resume(title, cls):
    print("\n--- Pause / Resume ---")
    try:
        with WindowCapture(title, cls) as cap:
            r = cap.get_frame()
            if r:
                cap.release_frame()
            cnt0 = cap.get_frame_count()

            cap.pause()
            print(f"  暂停: is_paused={cap.is_paused()}, 帧计数增长: {cap.get_frame_count() - cnt0}")

            cap.resume()
            time.sleep(0.1)
            r = cap.get_frame()
            if r:
                cap.release_frame()
            print(f"  恢复: is_paused={cap.is_paused()}, 帧计数增长: {cap.get_frame_count() - cnt0}")

            cap.stop()
            print(f"  stop 后: is_capturing={cap.is_capturing()}")
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_error_handling():
    print("\n--- 错误处理 ---")
    try:
        WindowCapture("__non_existent_window_12345__", "NoSuchClass")
        print("  无效窗口: 未抛出异常（异常）")
    except RuntimeError as e:
        print(f"  无效窗口: 正确抛出 RuntimeError: {str(e)[:60]}")


def demo_capture_one(title, cls):
    print("\n--- capture_one 按需捕获（auto Pause/Resume） ---")
    try:
        with WindowCapture(title, cls) as cap:
            for i in range(5):
                frame = cap.capture_one(timeout=0.5)
                print(f"  帧 {i + 1}: " + (f"{frame.shape}, mean={frame.mean():.1f}" if frame is not None else "None"))
                time.sleep(0.03)
    except RuntimeError as e:
        print(f"  失败: {e}")


if __name__ == "__main__":
    wins = list_windows()
    if not wins:
        print("没有可捕获的窗口，跳过后续测试")
        sys.exit(0)

    title, cls = wins[0]
    print(f"\n使用窗口: [{cls}] {title}")

    demo_capture(title, cls, client_area_only=True)
    demo_capture(title, cls, client_area_only=False)
    demo_zero_copy(title, cls)
    demo_multi_instance(title, cls)
    demo_pause_resume(title, cls)
    demo_error_handling()
    demo_capture_one(title, cls)

    print(f"\n最终活跃捕获数: {get_active_capture_count()} (应为 0)")
    print("全部测试完成")
