from picamera.array import PiRGBArray
from picamera import PiCamera
import numpy as np
import os
import time

from tensorflow.lite.python.interpreter import Interpreter


MODEL_PATH = '/home/pi/robocup24/runtime_data/victims.tflite'

interpreter = Interpreter(model_path=MODEL_PATH)
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
height = input_details[0]['shape'][1]
width = input_details[0]['shape'][2]

print(f"Model expects {width}x{height} image")

input_mean = 127.5
input_std = 127.5

camera = PiCamera()
camera.resolution = (320, 240)
camera.rotation = 0
camera.framerate = 90
rawCapture = PiRGBArray(camera, size=(320, 240))

for frame in camera.capture_continuous(rawCapture, format='bgr', use_video_port=True):
    image = frame.array
    
    input_data = (input_data - input_mean) / input_std

    interpreter.set_tensor(input_details[0]['index'], input_data)
    
    start_time = time.time()
    interpreter.invoke()
    end_time = time.time()

    print(f"Invoke took: {(end_time - start_time)/1000:.0f} ms")

    rawCapture.truncate(0)
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        camera.close()
        exit()