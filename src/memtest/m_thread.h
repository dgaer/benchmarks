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
#include "gxbstree.h"
#include "bstreei.h"


// C library includes
#include <stdio.h>
#include <time.h>

// Application framework includes
#include "m_log.h"

struct MemoryThreadParm
{
  gxBStree<gxString> tree;
  unsigned long num_insertions;
  gxString output;
  long thread_number;
};

class MemoryTestThread : public gxThread
{
private: // Base class interface
  void *ThreadEntryRoutine(gxThread_t *thread);
};

// Stand alone benchmark functions
void RunTestTree(gxBStree<gxString> &tree, unsigned long num_insertions, gxString &output, int thread_number);

#endif // __M_THREAD_HPP__
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
