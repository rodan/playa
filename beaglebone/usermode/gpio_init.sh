#!/bin/bash
echo '31' > /sys/class/gpio/export
sleep 1
echo '50' > /sys/class/gpio/export
sleep 1
echo '51' > /sys/class/gpio/export
sleep 1
echo 'out' >  /sys/class/gpio/gpio31/direction
sleep 1
echo 'out' >  /sys/class/gpio/gpio50/direction
sleep 1
echo 'out' >  /sys/class/gpio/gpio51/direction
