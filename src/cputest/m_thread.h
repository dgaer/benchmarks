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

Application POSIX and/or WIN32 thread framework
*/
// ----------------------------------------------------------- //   
#ifndef __M_THREAD_HPP__
#define __M_THREAD_HPP__

// DataReel includes
#include "gxdlcode.h"
#include "gxthread.h"
#include "gxmutex.h"
#include "gxcond.h"
#include "membuf.h"
#include "gxstring.h"
#include "gxssl.h"

// ZLIB include files
#include <zlib.h>

// C library includes
#include <stdio.h>
#include <time.h>

// Application framework includes
#include "m_log.h"

struct CPUThreadParm
{
  char *shared_membuf;
  unsigned long buf_size;
  unsigned long cytpro_overhead;
  unsigned long buf_start;
  unsigned long buf_len;
  gxString output;
  long thread_number;
  double average_time;
  double run_time;
};

class CPUTestThreadVars
{
public:
  CPUTestThreadVars();
  ~CPUTestThreadVars();

public: // Locking functions
  int GetProcessLock();
  int ReleaseProcessLock();

public: // Thread safe variable access functions
  int Reset();
  int SetAverageTime(double val, int increment_time = 0);
  int GetAverageTime(double &val);

private: // Thread safe variables for this program
  double average_time;

private: // Internal processing functions
  void reset_all();

private: // gxThread synchronization interface
  gxMutex var_lock;
  gxCondition var_cond;
  int var_is_locked;
  int var_thread_retries;
};

class CPUTestThread : public gxThread
{
private: // Base class interface
  void *ThreadEntryRoutine(gxThread_t *thread);
};

// Standalone functions
int RunTest(CPUThreadParm *parm);
void RunTestFormatMessage(gxString &output, int thread_number);
void RunTestEchoMessage(const char *mesg, int thread_number);
int fillrand(unsigned char *buf, const unsigned long len);
uLong calc_compress_len(uLong len);

// Thread safe globals
extern CPUTestThreadVars CPUTestThreadVarsGOB;
extern CPUTestThreadVars *global_tvars;

#endif // __M_THREAD_HPP__
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
