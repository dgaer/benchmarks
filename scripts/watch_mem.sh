#!/bin/bash
# ----------------------------------------------------------- 
# UNIX Shell Script File
# Tested Operating System(s): RHEL 5,6,7
# Tested Run Level(s): 3, 5
# Shell Used: BASH shell
# Original Author(s): Douglas.Gaer@noaa.gov
# File Creation Date: 07/15/2009
# Date Last Modified: 04/21/2017
#
# Version control: 1.28
#
# Support Team:
#
# Contributors:
# ----------------------------------------------------------- 
# ------------- Program Description and Details ------------- 
# ----------------------------------------------------------- 
#
# Script to watch total memory usage.
#
# ----------------------------------------------------------- 

trap ExitProc SIGHUP SIGINT SIGTERM SIGQUIT SIGABRT

function ExitProc() {
    free
    if [ -e /proc/meminfo ]; then cat /proc/meminfo; fi
    exit 0
}


if [ "$1" == "STATSONLY" ]
    then 
    if [ -e /proc/meminfo ]; then cat /proc/meminfo; fi
    exit 0 
fi

echo "Sampling total memory usage every 1 second"
echo "Press Ctrl-C to stop"
echo ""

while :
do
  free
  if [ -e /proc/meminfo ]; then cat /proc/meminfo; fi
  sleep 1
done

# ----------------------------------------------------------- 
# ******************************* 
# ********* End of File ********* 
# ******************************* 
