#!/bin/sh

FLASK_APP=sleepmotion-ble
export FLASK_APP

cd /home/pi
python3 -m flask run -h 0.0.0.0  >/home/pi/ble.log 2>&1 &