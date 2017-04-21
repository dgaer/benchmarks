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
# Script to log system memory usage.
#
# ----------------------------------------------------------- 

trap ExitProc SIGHUP SIGINT SIGTERM SIGQUIT SIGABRT

function ExitProc() {
    exit 0
}

while :
do
  free | grep Mem: | awk '{ print $3 }'
  if [ "$1" == "STATSONLY" ]; then exit 0; fi
  sleep 1
done

# ----------------------------------------------------------- 
# ******************************* 
# ********* End of File ********* 
# ******************************* 
                                                                       



