#!/usr/bin/env python

import serial
import time

try:
    ser = serial.Serial(
        port='/dev/ttyUSB0', #Replace ttyS0 with ttyAM0 for Pi1,Pi2,Pi0
        baudrate = 115200,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
    )

    #time.sleep(2)
    #ser.write(bytearray([0x01, 0, 0]))
    time.sleep(0.2)
    ser.close()

    print("-- stop.py: Stopped motors --")
except Exception as e:
    # Catching all exceptions because this script is not critical
    print("stop.py: ", e)