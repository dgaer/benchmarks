#######################
#### Start of File ####
#######################
# --------------------------------------------------------------- 
# Makefile
# Compiler: GNU, Intel, Cray
# Orginal Author: Douglas.Gaer@noaa.gov
# File Creation Date: 04/13/2016
# Date Last Modified: 04/21/2017
#
# Version: 1.28
#
# Contributors:
# --------------------------------------------------------------- 
# --------------- Makefile Description and Details -------------- 
# --------------------------------------------------------------- 
#
# Makefile used to build all the benchmark utilities
#
# --------------------------------------------------------------- 
SHELL = /bin/bash

# Compile the files and build the executable
# ===============================================================
all:
	cd cputest; make -f makefile all 64BITCFG=1 FINAL=1
	cd disktest; make -f makefile all 64BITCFG=1 FINAL=1
	cd memtest; make -f makefile all 64BITCFG=1 FINAL=1
	cd multicasttest; make -f makefile all 64BITCFG=1 FINAL=1
	cd tcptest; make -f makefile all 64BITCFG=1 FINAL=1
	cd udptest; make -f makefile all 64BITCFG=1 FINAL=1
	cd flops; make -f makefile all 64BITCFG=1 FINAL=1
# ===============================================================

# Install the new binaries
# ===============================================================
install:
	cd cputest; make -f makefile install 64BITCFG=1 FINAL=1
	cd disktest; make -f makefile install 64BITCFG=1 FINAL=1
	cd memtest; make -f makefile install 64BITCFG=1 FINAL=1
	cd multicasttest; make -f makefile install 64BITCFG=1 FINAL=1
	cd tcptest; make -f makefile install 64BITCFG=1 FINAL=1
	cd udptest; make -f makefile install 64BITCFG=1 FINAL=1
	cd flops; make -f makefile install 64BITCFG=1 FINAL=1

# Remove object files and the executable after running make 
# ===============================================================
clean:
	cd cputest; make -f makefile clean 64BITCFG=1 FINAL=1
	cd disktest; make -f makefile clean 64BITCFG=1 FINAL=1
	cd memtest; make -f makefile clean 64BITCFG=1 FINAL=1
	cd multicasttest; make -f makefile clean 64BITCFG=1 FINAL=1
	cd tcptest; make -f makefile clean 64BITCFG=1 FINAL=1
	cd udptest; make -f makefile clean 64BITCFG=1 FINAL=1
	cd flops; make -f makefile clean 64BITCFG=1 FINAL=1
# --------------------------------------------------------------- 
#####################
#### End of File ####
#####################
