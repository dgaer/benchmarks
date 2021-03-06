#######################
#### Start of File ####
#######################
# --------------------------------------------------------------- 
# Makefile Contents: Makefile for example program
# C/C++ Compiler Used: GNU, Intel, Cray
# --------------------------------------------------------------- 
# Define a name for the executable
BUILD := $(BUILD)
PROJECT = tcpserv
64BITCFG = 1
FINAL = 1

# GXD/GXS/GXT library path
GCODE_LIB_DIR = ../3plibs/datareel

include linux.env

# Setup additional paths for includes and source code
APP_PATH = .

# Compile the files and build the executable
# ===============================================================
all:    $(PROJECT)

include project.mak

$(PROJECT):	$(OBJECTS)
	$(CXX) $(COMPILE_FLAGS) $(OBJECTS) $(LINK_LIBRARIES) \
	$(OUTPUT) $(PROJECT) $(LINKER_FLAGS)

# ===============================================================
# Set to your installation directory
install:
	mkdir -pv ../../bin
	cp tcpserv  ../../bin/tcp_test_server
	chmod 755 ../../bin/tcp_test_server

# Remove object files and the executable after running make 
# ===============================================================
clean:
	echo Removing all OBJECT files from working directory...
	rm -f *.o 

	echo Removing EXECUTABLE file from working directory...
	rm -f $(PROJECT)

	echo Removing all test LOG files from working directory...
	rm -f *.log 

	echo Removing all test OUT files from working directory...
	rm -f *.out 

	echo Removing all test EDS files from working directory...
	rm -f *.eds 

	echo Removing all test DATABASE files from working directory...
	rm -f *.gxd 

	echo Removing all test INDEX files from working directory...
	rm -f *.btx 
	rm -f *.gix

	echo Removing all test InfoHog files from working directory...
	rm -f *.ihd 
	rm -f *.ihx 
# --------------------------------------------------------------- 
#####################
#### End of File ####
#####################
