import time
import serial
import struct

ser = serial.Serial(
	port='/dev/ttyUSB0',
	baudrate=115200,
	parity=serial.PARITY_NONE,
	stopbits=serial.STOPBITS_ONE,
	bytesize=serial.EIGHTBITS,
	timeout=1
)

LEFT_SPEED = 80
RIGHT_SPEED = -80

time.sleep(2)

while True:
	time.sleep(0.01)
	ser.write(struct.pack('BB', 0x04, 3))
	print("Wrote command")

	print(ser.read(2))

ser.close()
