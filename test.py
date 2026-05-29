"""
wgc-python 功能完整测试

覆盖：多开捕获 · 客户区/全窗口 · 零拷贝 · Pause/Resume · 状态查询
"""
import time
import sys
import ctypes
from wgc_python import WindowCapture, enumerate_windows, get_last_error, get_active_capture_count


def demo_list_windows():
    """① 枚举窗口"""
    print("=" * 50)
    print("1. 枚举窗口")
    print("=" * 50)
    wins = enumerate_windows()
    print(f"可见窗口数: {len(wins)}")
    for title, cls in wins[:10]:
        print(f"  [{cls}] {title}")
    if len(wins) > 10:
        print(f"  ... 还有 {len(wins) - 10} 个")
    return wins


def demo_client_area_capture(title, cls):
    """② 客户区捕获（不含标题栏/边框）"""
    print("=" * 50)
    print("2. 客户区捕获（client_area_only=True）")
    print("=" * 50)
    try:
        with WindowCapture(title, cls, client_area_only=True) as cap:
            print(f"  句柄: {cap.handle}")
            for i in range(3):
                r = cap.get_frame()
                if r:
                    ptr, w, h, rp = r
                    print(f"  帧 {i+1}: {w}x{h}  pitch={rp}")
                    cap.release_frame()
                else:
                    print(f"  帧 {i+1}: None")
                time.sleep(0.05)
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_full_window_capture(title, cls):
    """③ 全窗口捕获（含标题栏/边框）"""
    print("=" * 50)
    print("3. 全窗口捕获（client_area_only=False）")
    print("=" * 50)
    try:
        with WindowCapture(title, cls, client_area_only=False) as cap:
            print(f"  句柄: {cap.handle}")
            for i in range(3):
                r = cap.get_frame()
                if r:
                    ptr, w, h, rp = r
                    print(f"  帧 {i+1}: {w}x{h}  pitch={rp}")
                    cap.release_frame()
                else:
                    print(f"  帧 {i+1}: None")
                time.sleep(0.05)
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_zero_copy(title, cls):
    """④ 零拷贝路径：get_frame + release_frame + numpy 零构造"""
    print("=" * 50)
    print("4. 零拷贝路径（get_frame + numpy ndarray strides）")
    print("=" * 50)
    try:
        with WindowCapture(title, cls) as cap:
            r = cap.get_frame()
            if r:
                ptr, w, h, rp = r
                print(f"  映射指针: {ptr:#x}, 尺寸: {w}x{h}, pitch={rp}")
                try:
                    import numpy as np
                    arr = np.ndarray((h, w, 4), dtype=np.uint8,
                                     buffer=(ctypes.c_ubyte * (h * rp)).from_address(ptr),
                                     strides=(rp, 4, 1))
                    print(f"  numpy 零拷贝 shape: {arr.shape}, dtype: {arr.dtype}, strides: {arr.strides}")
                except ImportError:
                    print("  numpy 未安装，跳过零拷贝验证")
                cap.release_frame()
                print("  映射已释放")
            else:
                print("  获取帧失败")
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_multi_instance(title, cls):
    """⑤ 多开捕获：同一窗口并发两个会话"""
    print("=" * 50)
    print("5. 多开捕获（同一窗口并发两个会话）")
    print("=" * 50)
    try:
        cap1 = WindowCapture(title, cls)
        cap2 = WindowCapture(title, cls)
        print(f"  cap1 句柄: {cap1.handle}, cap2 句柄: {cap2.handle}")
        print(f"  活跃捕获数: {get_active_capture_count()}")

        r1 = cap1.get_frame()
        r2 = cap2.get_frame()
        if r1 and r2:
            print(f"  cap1 帧: {r1[1]}x{r1[2]}, cap2 帧: {r2[1]}x{r2[2]}")
            cap1.release_frame()
            cap2.release_frame()
        cap1.close()
        cap2.close()
        print(f"  close 后活跃捕获数: {get_active_capture_count()}")
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_pause_resume(title, cls):
    """⑥ Pause/Resume：暂停→恢复→验证帧计数增长"""
    print("=" * 50)
    print("6. Pause / Resume")
    print("=" * 50)
    try:
        with WindowCapture(title, cls) as cap:
            r = cap.get_frame()
            if r: cap.release_frame()
            cnt0 = cap.get_frame_count()
            print(f"  初始帧计数: {cnt0}")

            cap.pause()
            print(f"  暂停: is_paused={cap.is_paused()}")

            cnt1 = cap.get_frame_count()
            print(f"  暂停后帧计数: {cnt1} (增长: {cnt1 - cnt0})")

            cap.resume()
            print(f"  恢复: is_paused={cap.is_paused()}")
            time.sleep(0.1)

            r = cap.get_frame()
            if r: cap.release_frame()
            cnt2 = cap.get_frame_count()
            print(f"  恢复后帧计数: {cnt2} (增长: {cnt2 - cnt1})")

            cap.stop()
            print(f"  stop 后: is_capturing={cap.is_capturing()}")
    except RuntimeError as e:
        print(f"  失败: {e}")


def demo_error_handling():
    """⑦ 错误处理：无效窗口"""
    print("=" * 50)
    print("7. 错误处理")
    print("=" * 50)

    try:
        WindowCapture("__non_existent_window_12345__", "NoSuchClass")
        print("  无效窗口: 未抛出异常（异常）")
    except RuntimeError as e:
        print(f"  无效窗口: 正确抛出 RuntimeError")

    err = get_last_error()
    print(f"  get_last_error: {err[:60] if err else '(empty)'}...")


def demo_capture_one(title, cls):
    """⑧ capture_one 按需捕获 — 低 CPU 推荐用法"""
    print("=" * 50)
    print("8. capture_one 按需捕获（auto Pause/Resume，低 CPU）")
    print("=" * 50)
    try:
        cap = WindowCapture(title, cls)
        for i in range(5):
            frame = cap.capture_one(timeout=0.5)
            if frame is not None:
                print(f"  帧 {i+1}: {frame.shape}, dtype={frame.dtype}, mean={frame.mean():.1f}")
            else:
                print(f"  帧 {i+1}: None")
            time.sleep(0.03)
        cap.close()
    except RuntimeError as e:
        print(f"  失败: {e}")


if __name__ == "__main__":
    wins = demo_list_windows()
    if not wins:
        print("没有可捕获的窗口，跳过后续测试")
        sys.exit(0)

    title, cls = wins[0]
    print(f"\n使用窗口: [{cls}] {title}\n")

    demo_client_area_capture(title, cls)
    demo_full_window_capture(title, cls)
    demo_zero_copy(title, cls)
    demo_multi_instance(title, cls)
    demo_pause_resume(title, cls)
    demo_error_handling()
    demo_capture_one(title, cls)

    print("=" * 50)
    print(f"最终活跃捕获数: {get_active_capture_count()} (应为 0)")
    print("全部测试完成")
