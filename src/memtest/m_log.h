// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File
// C++ Compiler Used: GNU, Intel, Cray
// Produced By: Douglas.Gaer@noaa.gov
// File Creation Date: 10/17/2002
// Date Last Modified: 04/21/2017
// ----------------------------------------------------------- // 
// ---------- Include File Description and Details  ---------- // 
// ----------------------------------------------------------- // 
/*
This software and documentation is distributed for the purpose of
gathering system statistics. This software can be freely modified and
distributed, but without a warranty of any kind. Use for any purpose
is not guaranteed. All third party libraries used to build this
application are subject to the licensing agreement stated within the
source code and any documentation supplied with the third party
library.

Application POSIX and/or WIN32 logging framework
*/
// ----------------------------------------------------------- //   
#ifndef __M_LOG_HPP__
#define __M_LOG_HPP__

// DataReel includes
#include "gxdlcode.h"
#include "gxmutex.h"
#include "gxcond.h"
#include "gxthread.h"
#include "gxstring.h"
#include "systime.h"
#include "logfile.h"

// --------------------------------------------------------------
// Constants and type definitions
// --------------------------------------------------------------
// Thread constants
const int log_DISPLAY_THREAD_RETRIES = 255;
// --------------------------------------------------------------

// Classes
struct  LogConfig {
  LogConfig();
  ~LogConfig();

  // Helper functions
  void Reset();

  // Locking functions
  int GetProcessLock();
  int ReleaseProcessLock();

  // Logging variables
  gxString service_name;
  gxString logfile_name;
  LogFile *logfile;
  int verbose;
  int debug;
  int debug_level;

private: // gxThread synchronization interface
  gxMutex display_lock;      // Mutex object used to lock the display
  gxCondition display_cond;  // Condition variable used with the display lock
  int display_is_locked;     // Display lock boolean predicate
};

// Thread safe console logging functions, with logging to log file
int PrintMessage(const char *mesg1, const char *mesg2 = 0, const char *mesg3 = 0);
int PrintLines(const char *mesg1, const char *mesg2 = 0, 
	       const char *mesg3 = 0);

// Thread safe console only write functions, without logging
int ConsoleWrite(const char *mesg1, const char *mesg2 = 0, const char *mesg3 = 0);
int ConsoleWriteLines(const char *mesg1, const char *mesg2 = 0, 
		      const char *mesg3 = 0);
int ConsoleWriteErr(const char *mesg1, const char *mesg2 = 0, const char *mesg3 = 0);
int ConsoleWriteLinesErr(const char *mesg1, const char *mesg2 = 0, 
			 const char *mesg3 = 0);

// Non-thread safe console logging functions, with logging to log file
int NT_print(const char *mesg1, const char *mesg2 = 0, const char *mesg3 = 0);
int NT_printlines(const char *mesg1, const char *mesg2 = 0, 
		  const char *mesg3 = 0);

// Non-thread safe console only write functions, without logging
int NT_console_write(const char *mesg1, const char *mesg2 = 0, const char *mesg3 = 0);
int NT_console_write_lines(const char *mesg1, const char *mesg2 = 0, 
			   const char *mesg3 = 0);
int NT_console_write_err(const char *mesg1, const char *mesg2 = 0, const char *mesg3 = 0);
int NT_console_write_lines_err(const char *mesg1, const char *mesg2 = 0, 
			       const char *mesg3 = 0);

// Internal processing function
int print_block(const char *mesg1, const char *mesg2, const char *mesg3, int lines, 
		int blob, int standard_error, int log_formatting, int log_to_file,
		int use_verbose_mode, LogFile *my_logfile, gxString my_serivce_name,
		gxString my_logfile_name);

// --------------------------------------------------------------
// Globals configuration variables
// --------------------------------------------------------------
extern LogConfig logConfigSruct;
extern LogConfig *logcfg;
// --------------------------------------------------------------

#endif // __M_LOG_HPP__
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
