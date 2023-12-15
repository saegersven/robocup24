if arduino-cli compile --fqbn arduino:avr:nano /home/pi/robocup24/src/arduino_firmware ;
then
	arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:nano /home/pi/robocup24/src/arduino_firmware --log-level debug -v
fi
