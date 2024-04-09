from picamera import PiCamera
import time

print("1) capture silver data (capture/silver)")
print("2) capture silver_continous (capture/silver)")
print("3) capture victim data (capture/victim)")
print("4) capture corner data (capture/corner)")

num = int(input("Enter number: "))

if num <= 0 or num > 4:
    print("You idiot.")
    exit()

resolution = (80, 48)

out_dirs = ['/home/pi/capture/silver/', '/home/pi/capture/silver/', '/home/pi/capture/victim/', '/home/pi/capture/corner/']

if num == 3 or num == 4:
    resolution = (320, 240)

cam = PiCamera()
cam.resolution = resolution
cam.hflip = True
cam.vflip = True
cam.framerate = 90
cam.zoom = (0.15, 0.15, 0.7, 0.7)

counter = 0

try:
    while True:
        cmd = ""
        print(f'{counter}')
        if num != 2:
            cmd = input('>')

        if 'q' in cmd:
            break
        else:
            cam.capture(out_dirs[num - 1] + str(int(time.time() * 1000)) + '.png')
            counter += 1
except KeyboardInterrupt:
    pass


"""
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
cv2.destroyAllWindows()"""