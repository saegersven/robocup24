#clear
python3 /home/pi/robocup24/stop.py
cd /home/pi/robocup24/build
if ninja -j2 ;
then
    /home/pi/robocup24/build/robocup
fi
cd ..
