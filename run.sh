#clear
python3 /home/pi/robocup24/stop.py
cd /home/pi/robocup24/build
if ninja -j2 ;
then
    v4l2-ctl --set-parm=90
    
    while true; do
        /home/pi/robocup24/build/robocup && break
    done
fi
cd ..