#clear
python3 /home/pi/robocup24/stop.py
cd /home/pi/robocup24/build
if ninja -j2 ;
then
    v4l2-ctl --set-parm=90
    /home/pi/robocup24/build/robocup
fi
cd ..