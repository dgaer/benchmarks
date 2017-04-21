// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- // 
// C++ Source Code File
// C++ Compiler Used: GNU, Intel, Cray
// Produced By: Douglas.Gaer@noaa.gov
// File Creation Date: 10/17/2002
// Date Last Modified: 04/21/2017
// ----------------------------------------------------------- // 
// ------------- Program Description and Details ------------- // 
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
#include "gxdlcode.h"
#include "m_log.h"

// --------------------------------------------------------------
// Global variable initialzation
// --------------------------------------------------------------
LogConfig LogConfigSruct;
LogConfig *logcfg = &LogConfigSruct;
// --------------------------------------------------------------

LogConfig::LogConfig()
{
  Reset();
}

LogConfig::~LogConfig()
{

}

void LogConfig::Reset()
{
  service_name.Clear();
  logfile_name.Clear();
  display_is_locked = 0;
  logfile = 0;
  verbose = 0;
  debug = 0;
  debug_level = 1;
}

int LogConfig::GetProcessLock()
{
  display_lock.MutexLock();

  // Ensure that all threads have finished writing to the device
  int num_try = 0;
  while(display_is_locked != 0) {
    // Block this thread from its own execution if a another thread
    // is writing to the device
    if(++num_try < log_DISPLAY_THREAD_RETRIES) {
      display_cond.ConditionWait(&display_lock);
    }
    else {
      return 0; // Could not write string to the device
    }
  }

  // Tell other threads to wait until write is complete
  logcfg->display_is_locked = 1; 

  // ********** Enter Critical Section ******************* //

  return 1;
}

int LogConfig::ReleaseProcessLock() 
{
  // ********** Leave Critical Section ******************* //

  // Tell other threads that this write is complete
  display_is_locked = 0; 
 
  // Wake up the next thread waiting on this condition
  display_cond.ConditionSignal();
  display_lock.MutexUnlock();
  
  return 1;
}

int PrintMessage(const char *mesg1, const char *mesg2, const char *mesg3)
// Thread safe print function used to print and log a console message
{
  if(!logcfg->GetProcessLock()) return 0;
  int log_to_file = 1;
  if(!logcfg->logfile) log_to_file = 0;
  if(logcfg->logfile_name.is_null()) log_to_file = 0;
  int rv = print_block(mesg1, mesg2, mesg3, 0, 0, 1, 0, 
		       log_to_file, logcfg->verbose, logcfg->logfile, logcfg->service_name,
		       logcfg->logfile_name);
  logcfg->ReleaseProcessLock(); 
  return rv;
}

int PrintLines(const char *mesg1, const char *mesg2, const char *mesg3)
// Thread safe print function used to print and log a console message
{
  if(!logcfg->GetProcessLock()) return 0;
  int log_to_file = 1;
  if(!logcfg->logfile) log_to_file = 0;
  if(logcfg->logfile_name.is_null()) log_to_file = 0;
  int rv = print_block(mesg1, mesg2, mesg3, 1, 0, 1, 0, 
		       log_to_file, logcfg->verbose, logcfg->logfile, logcfg->service_name,
		       logcfg->logfile_name);
  logcfg->ReleaseProcessLock(); 
  return rv;
}

int ConsoleWrite(const char *mesg1, const char *mesg2, const char *mesg3)
// Thread safe print function used write a console message
{
  if(!logcfg->GetProcessLock()) return 0;
  int rv = print_block(mesg1, mesg2, mesg3, 0, 0, 0, 0, 0, 1, 0, logcfg->service_name
		       , logcfg->logfile_name);
  logcfg->ReleaseProcessLock(); 
  return rv;
}

int ConsoleWriteLines(const char *mesg1, const char *mesg2, const char *mesg3)
// Thread safe print function used write a console message
{
  if(!logcfg->GetProcessLock()) return 0;
  int rv = print_block(mesg1, mesg2, mesg3, 1, 0, 0, 0, 0, 1, 0, logcfg->service_name,
		       logcfg->logfile_name);
  logcfg->ReleaseProcessLock(); 
  return rv;
}

int ConsoleWriteErr(const char *mesg1, const char *mesg2, const char *mesg3)
// Thread safe print function used write a console message
{
  if(!logcfg->GetProcessLock()) return 0;
  int rv = print_block(mesg1, mesg2, mesg3, 0, 0, 1, 0, 0, 1, 0, logcfg->service_name,
		       logcfg->logfile_name);
  logcfg->ReleaseProcessLock(); 
  return rv;
}

int ConsoleWriteLinesErr(const char *mesg1, const char *mesg2, const char *mesg3)
// Thread safe print function used write a console message
{
  if(!logcfg->GetProcessLock()) return 0;
  int rv = print_block(mesg1, mesg2, mesg3, 1, 0, 1, 0, 0, 1, 0, logcfg->service_name,
		       logcfg->logfile_name);
  logcfg->ReleaseProcessLock(); 
  return rv;
}

int NT_print(const char *mesg1, const char *mesg2, const char *mesg3)
// Non-Thread safe print function used to print and log a console message
{
  int log_to_file = 1;
  if(!logcfg->logfile) log_to_file = 0;
  if(logcfg->logfile_name.is_null()) log_to_file = 0;
  return print_block(mesg1, mesg2, mesg3, 0, 0, 1, 0, 
		     log_to_file, logcfg->verbose, logcfg->logfile, logcfg->service_name,
		     logcfg->logfile_name);
}

int NT_printlines(const char *mesg1, const char *mesg2, const char *mesg3)
// Non-Thread safe print function used to print and log a console message
{
  int log_to_file = 1;
  if(!logcfg->logfile) log_to_file = 0;
  if(logcfg->logfile_name.is_null()) log_to_file = 0;
  return print_block(mesg1, mesg2, mesg3, 1, 0, 1, 0, 
		     log_to_file, logcfg->verbose, logcfg->logfile, logcfg->service_name,
		     logcfg->logfile_name);
}

int NT_console_write(const char *mesg1, const char *mesg2, const char *mesg3)
// Non-Thread safe print function used write a console message
{
  return print_block(mesg1, mesg2, mesg3, 0, 0, 0, 0, 0, 1, 0, 
		     logcfg->service_name, logcfg->logfile_name);
}

int NT_console_write_lines(const char *mesg1, const char *mesg2, const char *mesg3)
// Non-Thread safe print function used write a console message
{
  return print_block(mesg1, mesg2, mesg3, 1, 0, 0, 0, 0, 1, 0, 
		     logcfg->service_name, logcfg->logfile_name);
}

int NT_console_write_err(const char *mesg1, const char *mesg2, const char *mesg3)
// Non-Thread safe print function used write a console message
{
  return print_block(mesg1, mesg2, mesg3, 0, 0, 1, 0, 0, 1, 0, 
		     logcfg->service_name, logcfg->logfile_name);
}

int NT_console_write_lines_err(const char *mesg1, const char *mesg2, const char *mesg3)
// Non-Thread safe print function used write a console message
{
  return print_block(mesg1, mesg2, mesg3, 1, 0, 1, 0, 0, 1, 0, 
		     logcfg->service_name, logcfg->logfile_name);
}

int print_block(const char *mesg1, const char *mesg2, const char *mesg3, 
		int lines, int blob, int standard_error, int log_formatting,
		int log_to_file, int use_verbose_mode, LogFile *my_logfile, 
		gxString my_service_name, gxString my_logfile_name)
// Internal processing print function used to print and log a console message
{
  if(!mesg1) return 0;

  if(!my_logfile) log_to_file = 0;
  
  SysTime logtime;
  gxString output_message;

  if(lines) {
    if(log_formatting) {
      if(mesg1) output_message << logtime.GetSyslogTime() 
			       << " " << my_service_name << ": " << mesg1 << "\n";
      if(mesg2) output_message << logtime.GetSyslogTime() 
			       << " " << my_service_name << ": " << mesg2 << "\n";
      if(mesg3) output_message << logtime.GetSyslogTime() 
			       << " " << my_service_name << ": " << mesg3 << "\n";
    }
    else {
      if(mesg1) output_message << mesg1 << "\n";
      if(mesg2) output_message << mesg2 << "\n";
      if(mesg3) output_message << mesg3 << "\n";
    }
  }
  else if(blob) {
    if(log_formatting) {
      output_message << logtime.GetSyslogTime() 
		     << " " << my_service_name << ": " << "Message Block <<" 
		     << "\n";
      if(mesg1) output_message << mesg1;
      if(mesg2) output_message << mesg2; 
      if(mesg3) output_message << mesg3; 
      output_message << ">>" << "\n";
    }
    else {
      if(mesg1) output_message << mesg1;
      if(mesg2) output_message << mesg2; 
      if(mesg3) output_message << mesg3; 
    }
  }
  else {
    if(log_formatting) {
      output_message << logtime.GetSyslogTime() << " " << my_service_name << ": ";
      if(mesg1) output_message << mesg1;
      if(mesg2) output_message << " " << mesg2;
      if(mesg3) output_message << " " << mesg3;
      output_message << "\n";
    }
    else {
      if(mesg1) output_message << mesg1;
      if(mesg2) output_message << " " << mesg2;
      if(mesg3) output_message << " " << mesg3;
      output_message << "\n";
    }
  }

  if((log_to_file) && (my_logfile)) {
    if(my_logfile->Open(my_logfile_name.c_str()) == 0) {
      *(my_logfile) << output_message.c_str();
      my_logfile->df_Flush();
      my_logfile->Close();
    }
    else { // Could not write to log file so echo to STDERR
      gxString error_message;
      error_message << clear << logtime.GetSyslogTime() << " " << my_service_name << ": " 
		    << "Error writing to log file " << my_logfile_name.c_str()
		    << "\n";
      error_message << output_message.c_str();
      fprintf(stderr, "%s", error_message.c_str());
      fflush(stderr);
    }
  }

  if(use_verbose_mode) {
    if(standard_error) {
      fprintf(stderr, "%s", output_message.c_str());
      fflush(stderr);
    }
    else {
      GXSTD::cout.write(output_message.c_str(), output_message.length());
      GXSTD::cout.flush();
    } 
  }

  return 1;
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
