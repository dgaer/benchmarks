##!/usr/bin/python
# *******************************
# ******** Start of File ********
# *******************************
# -----------------------------------------------------------
# Python Script
# Operating System(s): RHEL 6, 7
# Python version used: 2.6.x, 2.7.x, numpy, matplotlib
# Original Author(s): Douglas.Gaer@noaa.gov
# File Creation Date: 04/23/2016  
# Date Last Modified: 04/24/2016
#
# Version control: 1.01
#
# Support Team:
#
# Contributors: 
# -----------------------------------------------------------
# ------------- Program Description and Details -------------
# -----------------------------------------------------------
#
# Plot script for disk speed benchmaks
#
# -----------------------------------------------------------
import sys
import os
import time

# Output our program setup and ENV
PYTHON = os.environ.get('PYTHON')
PYTHONPATH = os.environ.get('PYTHONPATH')
print ' Python ploting program: ' + sys.argv[0]
if not PYTHON:
   print 'WARNING - PYTHON variable not set in callers ENV'
else:
   print ' Python interpreter: ' + PYTHON
if not PYTHONPATH:
   print 'WARNING - PYTHONPATH variable not set in callers ENV'
else:
   print ' Python path: ' + PYTHONPATH

import datetime
import numpy as np
import ConfigParser

# Generate images without having a window appear
# http://matplotlib.org/faq/howto_faq.html

INTERACTIVE = True
if len(sys.argv) > 2:
   INTERACTIVE = False

import matplotlib
if not INTERACTIVE:
   matplotlib.use('Agg')

from pylab import *
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.font_manager import fontManager, FontProperties 
from mpl_toolkits.basemap import Basemap
import matplotlib.dates as mdate
import datetime as dt

if len(sys.argv) < 2:
   print 'ERROR - You must supply the name of our input control config file'
   print 'Usage 1: ' +  sys.argv[0] + ' disk_benchmark.dat'
   print 'Usage 2: ' +  sys.argv[0] + ' disk_benchmark.dat disk_benchmark.png'
   sys.exit()

Config = ConfigParser.ConfigParser()
fname = sys.argv[1] 
if os.path.isfile(fname):
   print "Reding pyplot CFG file: " + fname
   Config.read(fname)
else:
   print 'ERROR - Cannot open pyplot CFG file ' + fname
   sys.exit()

if not Config.has_section("DISKBENCHMARK"):
   print  'ERROR - Pyplot CFG file missing DISKBENCHMARK section'
   sys.exit()

if not Config.has_section("PLOTINFO"):
   print  'ERROR - Pyplot CFG file missing PLOTINFO section'
   sys.exit()

num_inserts = Config.getint('DISKBENCHMARK', 'num_inserts')
num_inserts_str = Config.get('DISKBENCHMARK', 'num_inserts_str')
write_time = Config.get('DISKBENCHMARK', 'write_time').split(",")
write_time_avg = Config.get('DISKBENCHMARK', 'write_time_avg').split(",")
xticks = Config.get('DISKBENCHMARK', 'xticks').split(",")
xticks_str = Config.get('DISKBENCHMARK', 'xticks_str').split(",")
insert_time = Config.getfloat('DISKBENCHMARK', 'insert_time')
insert_time_avg = Config.getfloat('DISKBENCHMARK', 'insert_time_avg')
read_time = Config.get('DISKBENCHMARK', 'read_time').split(",")
read_time_avg = Config.get('DISKBENCHMARK', 'read_time_avg').split(",")
search_time = Config.getfloat('DISKBENCHMARK', 'search_time')
search_time_avg = Config.getfloat('DISKBENCHMARK', 'search_time_avg')
remove_time = Config.get('DISKBENCHMARK', 'remove_time').split(",")
remove_time_avg = Config.get('DISKBENCHMARK', 'remove_time_avg').split(",")
delete_time = Config.getfloat('DISKBENCHMARK', 'delete_time')
delete_time_avg = Config.getfloat('DISKBENCHMARK', 'delete_time_avg')
rewrite_time = Config.get('DISKBENCHMARK', 'rewrite_time').split(",")
rewrite_time_avg = Config.get('DISKBENCHMARK', 'rewrite_time_avg').split(",")
reinsert_time = Config.getfloat('DISKBENCHMARK', 'reinsert_time')
reinsert_time_avg = Config.getfloat('DISKBENCHMARK', 'reinsert_time_avg')
DPI = Config.getint('PLOTINFO', 'DPI')
plot_type = Config.get('PLOTINFO', 'plot_type')

if plot_type == 'averages':
   plt.plot(write_time_avg, color='blue', label="Write time")
   plt.plot(read_time_avg, color='green', label="Read time")
   plt.plot(remove_time_avg, color='red', label="Delete time")
   plt.plot(rewrite_time_avg, color='darkblue', label="Rewrite time")
   plt.ylabel('Average Write/Read/Delete (seconds)', fontsize=12)
else:
   plt.plot(write_time, color='blue', label="Write time")
   plt.plot(read_time, color='green', label="Read time")
   plt.plot(remove_time, color='red', label="Delete time")
   plt.plot(rewrite_time, color='darkblue', label="Rewrite time")
   plt.ylabel('Write/Read/Delete (seconds)', fontsize=12)

plt.legend(loc='upper right', fancybox=True, shadow=True, prop=dict(size=8,weight='bold'),ncol=4)
plt.grid(True, which="major", linestyle="dotted")
ind = np.arange(len(xticks_str))
plt.xticks(ind, xticks_str, rotation=75)
plt.xlabel('Number of Btree Index Keys', fontsize=12)

plot_title = "Btree Disk Speed Test For " + num_inserts_str + " Keys"
if Config.has_section("HOSTINFO"):
   host = Config.get('HOSTINFO', 'host')
   date = Config.get('HOSTINFO', 'date')
   build = Config.get('HOSTINFO', 'build')
   title = Config.get('HOSTINFO', 'title')
   plot_title = title +': '+ host +' '+ date +' '+ build +' build' 

plt.title(plot_title, fontsize=12,weight='bold')
plt.subplots_adjust(hspace=.23, left=.14, bottom=.16, right=.91, top=.90, wspace=.23)

if not INTERACTIVE:
   plotimg = sys.argv[2]
   print 'Plot file = ' + plotimg
   plt.savefig(plotimg,dpi=DPI,bbox_inches='tight',pad_inches=0.1)

if INTERACTIVE:
   plt.show()

print 'Plot complete'
plt.clf()
# -----------------------------------------------------------
# *******************************
# ********* End of File *********
# *******************************
