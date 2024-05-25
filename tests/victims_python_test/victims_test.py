import os
import cv2
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

cap = cv2.VideoCapture(0)
while True:
    cap.grab()
    _, image = cap.retrieve()

    image = cv2.cvtColor(cv2.resize(image, (320, 240)), cv2.COLOR_BGR2RGB)

    input_data = (input_data - input_mean) / input_std

    interpreter.set_tensor(input_details[0]['index'], input_data)
    
    start_time = time.time()
    interpreter.invoke()
    end_time = time.time()

    print(f"Invoke took: {(end_time - start_time)/1000:.0f} ms")