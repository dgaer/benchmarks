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
# Script to watch total system usage.
#
# ----------------------------------------------------------- 

trap ExitProc SIGHUP SIGINT SIGTERM SIGQUIT SIGABRT

function EchoSystemStats() {
echo "START RELEASE"
if [ -e /etc/redhat-release ]; then cat /etc/redhat-release; fi
echo "END RELEASE"

echo "START VERSION"
if [ -e /proc/version ]; then cat /proc/version; fi
echo "END VERSION"

echo "START CPUINFO"
if [ -e /proc/cpuinfo ]; then cat /proc/cpuinfo; fi
echo "END CPUINFO"

echo "START STAT"
if [ -e /proc/stat ]; then cat /proc/stat; fi
echo "END STAT"

echo "START ZONEINFO"
if [ -e /proc/zoneinfo ]; then cat /proc/zoneinfo; fi
echo "END ZONEINFO"

echo "START MEMINFO"
if [ -e /proc/meminfo ]; then cat /proc/meminfo; fi
echo "END MEMINFO"

echo "START PARTITIONS"
if [ -e /proc/partitions ]; then cat /proc/partitions; fi
echo "END PARTITIONS"

echo "START MOUNTS"
if [ -e /proc/mounts ]; then cat /proc/mounts; fi
echo "END MOUNTS"

echo "START DISKSTATS"
if [ -e /proc/diskstats ]; then cat /proc/diskstats; fi
echo "END DISKSTATS"

echo "START DEVICES"
if [ -e /proc/devices ]; then cat /proc/devices; fi
echo "END DEVICES"
}

function ExitProc() {
    EchoSystemStats
    exit 0
}

EchoSystemStats
if [ "$1" == "STATSONLY" ]; then exit 0; fi

echo "Sampling system usage every 1 second"
echo "Press Ctrl-C to stop"
echo ""

top -b

# ----------------------------------------------------------- 
# ******************************* 
# ********* End of File ********* 
# ******************************* 
