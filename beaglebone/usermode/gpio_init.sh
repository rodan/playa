#!/bin/bash

num_retries=10

repeat_exec()
{
	retry=${num_retries}
	while [ "${retry}" != 0 ]; do
		retry=$((retry - 1))
		eval $@ 2>/dev/null
		if [ "$?" == "0" ]; then
			echo " [ok]  $@ - $(( ${num_retries} - ${retry} ))"
			return 0
		fi
	done
	echo " [err]  $@"
	return 1
}

repeat_exec "echo '31' > /sys/class/gpio/export"
repeat_exec "echo '50' > /sys/class/gpio/export"
repeat_exec "echo '51' > /sys/class/gpio/export"
repeat_exec "echo '60' > /sys/class/gpio/export"
repeat_exec "echo 'out' > /sys/class/gpio/gpio31/direction"
repeat_exec "echo 'out' > /sys/class/gpio/gpio50/direction"
repeat_exec "echo 'out' > /sys/class/gpio/gpio51/direction"
repeat_exec "echo 'in' > /sys/class/gpio/gpio60/direction"
