import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BOARD)

GPIO.setup(19, GPIO.OUT)

while True:
	time.sleep(2)
	s = time.time()
	GPIO.output(19, GPIO.HIGH)
	time.sleep(0.001)
	GPIO.output(19, GPIO.LOW)
	print("Took: ", (time.time() - s))

GPIO.cleanup()
