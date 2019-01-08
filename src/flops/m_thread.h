// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File
// C++ Compiler Used: GNU
// Produced By: Douglas.Gaer@noaa.gov
// File Creation Date: 12/03/2018
// Date Last Modified: 01/08/2019
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

// C library includes
#include <stdio.h>
#include <time.h>

// Application framework includes
#include "m_log.h"

struct FLOPSThreadParm
{
  FLOPSThreadParm() {
    run_dp_test = 0;
    iterations = 0;
    thread_number = 0;
  }

  int thread_number;
  int run_dp_test;
  unsigned long iterations;
};

class FLOPSTestThread : public gxThread
{
private: // Base class interface
  void *ThreadEntryRoutine(gxThread_t *thread);
};

// Standalone functions
int RunSPTest(FLOPSThreadParm *parm);
int RunDPTest(FLOPSThreadParm *parm);

#endif // __M_THREAD_HPP__
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
