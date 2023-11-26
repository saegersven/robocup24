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

while True:
	time.sleep(1)
	ser.write(bytearray([0x01, 0xFF, 0xFF]))
	print("Wrote command")
	time.sleep(1)
	ser.write(bytearray([0x01, 0x00, 0x00]))
	print("Wrote stop command")

ser.close()
