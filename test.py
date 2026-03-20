from wgc_python import *
import numpy as np
import cv2
import time

windows = enumerate_windows()
for title, cls in windows[:10]:
    print(f"标题: {title}, 类名: {cls}")

if start_capture("记事本", "Notepad"):
    print("捕获已启动")
    print(f"初始状态 - is_capturing: {is_capturing()}, is_paused: {is_paused()}")
    
    last_time = time.time()
    frame_count = 0
    paused = False
    
    while True:
        if not paused:
            result = get_frame()
            if result:
                data, width, height = result
                frame_count += 1
                
                img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
                img_bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
                
                if frame_count % 30 == 0:
                    fps = 30 / (time.time() - last_time)
                    cv2.setWindowTitle("Capture", f"FPS: {fps:.1f} (Paused: {paused})")
                    last_time = time.time()
                
                cv2.imshow("Capture", img_bgr)
        else:
            # 显示暂停状态
            cv2.setWindowTitle("Capture", f"Paused - FPS: 0.0")
            time.sleep(0.1)  # 减少CPU占用
        
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        elif key == ord('p'):  # 按P键暂停/恢复
            paused = not paused
            if paused:
                pause_capture()
                print(f"已暂停 - is_paused: {is_paused()}")
            else:
                resume_capture()
                print(f"已恢复 - is_paused: {is_paused()}")
                last_time = time.time()
                frame_count = 0
    
    stop_capture()
    cv2.destroyAllWindows()
else:
    print("捕获启动失败")
