#!/usr/bin/env python

import spidev

try:
    spi = spidev.SpiDev()
    spi.open(0, 0)

    spi.max_speed_hz = 500000
    spi.mode = 0
    spi.lsbfirst = False
    spi.cshigh = False

    spi.xfer([0x01, 0, 0]) # Send drive command with both speeds 0

    print("-- stop.py: Stopped motors --")
except:
    # Catching all exceptions because this script is not critical
    print("stop.py: An exception occured")