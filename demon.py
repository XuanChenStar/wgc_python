import threading
import time
import numpy as np
import cv2
from wgc_python import WindowCapture, enumerate_windows


class CaptureWorker:
    def __init__(self, title, cls):
        self.cap = WindowCapture(title, cls)
        self.lock = threading.Lock()
        self.frame = None
        self.running = True
        self.fps = 0
        self._fps_count = 0
        self._fps_timer = time.perf_counter()
        self._request = threading.Event()
        self._ready = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self):
        self._thread.start()

    def get_frame(self):
        self._request.set()
        self._ready.wait()
        self._ready.clear()
        with self.lock:
            return self.frame.copy() if self.frame is not None else None

    def _run(self):
        while self.running:
            self._request.wait()
            if not self.running:
                break
            self._request.clear()

            frame = self.cap.capture_one(timeout=1.0)
            if frame is not None:
                with self.lock:
                    self.frame = frame
                self._fps_count += 1

            now = time.perf_counter()
            if now - self._fps_timer >= 1.0:
                self.fps = self._fps_count / (now - self._fps_timer)
                self._fps_count = 0
                self._fps_timer = now

            self._ready.set()

    def stop(self):
        self.running = False
        self._request.set()
        self._thread.join(timeout=2)
        self.cap.close()


def main():
    wins = enumerate_windows()
    target = None
    for t, c in wins:
        if c == "UnityWndClass" and "崩坏3" in t:
            target = (t, c)
            break

    if not target:
        print("window not found")
        return

    title, cls = target
    print(f"thread: [{cls}] {title}")
    worker = CaptureWorker(title, cls)
    worker.start()
    time.sleep(0.5)

    t0 = time.perf_counter()
    duration = 30.0
    frame_n = 0
    print(f"running, Q to quit ({duration:.0f}s max)")

    try:
        while time.perf_counter() - t0 < duration:
            frame = worker.get_frame()
            if frame is not None:
                img = cv2.cvtColor(frame, cv2.COLOR_BGRA2BGR)
                cv2.imshow("崩坏3", img)

            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            frame_n += 1
    except KeyboardInterrupt:
        pass
    finally:
        worker.stop()
        cv2.destroyAllWindows()

    elapsed = time.perf_counter() - t0
    print(f"\n{elapsed:.1f}s, display {frame_n} frames, worker {worker.fps:.0f} FPS")


if __name__ == "__main__":
    main()
