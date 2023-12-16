import cv2

WIDTH = 320
HEIGHT = 240

cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, WIDTH)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, HEIGHT)
time.sleep(0.1)

num_images = 0

while(True):
    try:
        _, frame = cap.read()
        cv2.imshow("Frame", frame)

        key = cv2.waitKey(1) & 0xFF
        if key == ord('c'):
            path = "/home/pi/Desktop/capture/" + str(int(time.time() / 1000)) + ".png"
            cv2.imwrite(path, frame)
            num_images += 1

            print(f"Saved image #{num_images} to {path}")

            time.sleep(0.3)
        elif key == ord('q'):
            break
    except KeyboardInterrupt:
        break

cap.release()
cv2.destroyAllWindows()