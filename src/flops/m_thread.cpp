// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- // 
// C++ Source Code File
// C++ Compiler Used: GNU, Intel, Cray
// Produced By: Douglas.Gaer@noaa.gov
// File Creation Date: 12/03/2018
// Date Last Modified: 01/08/2019
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

Application POSIX and/or WIN32 thread framework
*/
// ----------------------------------------------------------- // 
#include "gxdlcode.h"
#include "m_thread.h"

void *FLOPSTestThread::ThreadEntryRoutine(gxThread_t *thread)
// Thread's entry function
{
  FLOPSThreadParm *parm = (FLOPSThreadParm *)thread->GetThreadParm();
  if(parm->run_dp_test == 1) {
    RunDPTest(parm);
  }
  else {
    RunSPTest(parm);
  }
  delete parm;
  return 0;
}

int RunSPTest(FLOPSThreadParm *parm)
{
  unsigned long i;
  unsigned long iterations = parm->iterations;
  float a, b, var;
  a = 1.0;
  b = 2.0;
  
  for(i = 0; i < iterations; i++) {
    var = (a*b) + (a/b);
    var = 0;
  }

  return 1;
}

int RunDPTest(FLOPSThreadParm *parm)
{
  unsigned long i;
  unsigned long iterations = parm->iterations;
  double a, b, var;
  a = 1.0;
  b = 2.0;
  
  for(i = 0; i < iterations; i++) {
    var = (a*b) + (a/b);
    var = 0;
  }

  return 1;
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
