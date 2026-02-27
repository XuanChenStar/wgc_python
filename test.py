from wgc_python import *
import numpy as np
import cv2
import time

windows = enumerate_windows()
for title, cls in windows[:10]:
    print(f"标题: {title}, 类名: {cls}")

if start_capture("崩坏3", "UnityWndClass"):
    print("捕获已启动")
    
    last_time = time.time()
    frame_count = 0
    
    while True:
        result = get_frame()
        if result:
            data, width, height = result
            frame_count += 1
            
            img = np.frombuffer(data, dtype=np.uint8).reshape(height, width, 4)
            img_bgr = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
            
            if frame_count % 30 == 0:
                fps = 30 / (time.time() - last_time)
                cv2.setWindowTitle("Capture", f"FPS: {fps:.1f}")
                last_time = time.time()
            
            cv2.imshow("Capture", img_bgr)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    stop_capture()
    cv2.destroyAllWindows()
else:
    print("捕获启动失败")
