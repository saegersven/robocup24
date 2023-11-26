import spidev
import time
import struct

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 10000

data = 0
while True:
	spi.xfer(struct.pack('BB', data, 0))
	data += 1
	time.sleep(1)
