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

Application POSIX and/or WIN32 thread framework
*/
// ----------------------------------------------------------- // 
#include "gxdlcode.h"
#include "m_thread.h"

// --------------------------------------------------------------
// Global variable initialzation
// --------------------------------------------------------------
CPUTestThreadVars CPUTestThreadVarsGOB;
CPUTestThreadVars *global_tvars = &CPUTestThreadVarsGOB;
// --------------------------------------------------------------

CPUTestThreadVars::CPUTestThreadVars()
{
  reset_all();
}

CPUTestThreadVars::~CPUTestThreadVars()
{

}

void CPUTestThreadVars::reset_all()
{
  var_thread_retries = 255;
  var_is_locked = 0;
  average_time = (double)0;
}

int CPUTestThreadVars::GetProcessLock()
{
  var_lock.MutexLock();

  // Ensure that all threads have finished writing to the device
  int num_try = 0;
  while(var_is_locked != 0) {
    // Block this thread from its own execution if a another thread
    // is writing to the device
    if(++num_try < var_thread_retries) {
      var_cond.ConditionWait(&var_lock);
    }
    else {
      return 0; // Could not write string to the device
    }
  }

  // Tell other threads to wait until write is complete
  var_is_locked = 1; 

  // ********** Enter Critical Section ******************* //

  return 1;
}

int CPUTestThreadVars::ReleaseProcessLock() 
{
  // ********** Leave Critical Section ******************* //

  // Tell other threads that this write is complete
  var_is_locked = 0; 
 
  // Wake up the next thread waiting on this condition
  var_cond.ConditionSignal();
  var_lock.MutexUnlock();
  
  return 1;
}

int CPUTestThreadVars::Reset()
{
  if(!GetProcessLock()) return 0;
  reset_all();
  return  ReleaseProcessLock();
}

int CPUTestThreadVars::GetAverageTime(double &val)
{
  if(!GetProcessLock()) return 0;
  val = average_time;
  return  ReleaseProcessLock();
}

int CPUTestThreadVars::SetAverageTime(double val, int increment_time)
{
  if(!GetProcessLock()) return 0;
  if(increment_time) {
    average_time += val;
  }
  else {
    average_time = val;
  }
  return  ReleaseProcessLock(); 
}

void *CPUTestThread::ThreadEntryRoutine(gxThread_t *thread)
// Thread's entry function
{
  CPUThreadParm *parm = (CPUThreadParm *)thread->GetThreadParm();
  RunTest(parm);
  ConsoleWrite(parm->output.c_str());
  global_tvars->SetAverageTime(parm->run_time, 1);
  delete parm;
  return 0;
}

void RunTestFormatMessage(gxString &output, int thread_number)
{
  output << "Thread[" << thread_number << "]: ";
}

void RunTestEchoMessage(const char *mesg, int thread_number)
{
  if(!mesg) return;
  gxString m_buf;
  RunTestFormatMessage(m_buf, thread_number);
  m_buf << mesg;
  ConsoleWriteErr(m_buf.c_str());
}

int RunTest(CPUThreadParm *parm)
{
  parm->output.Clear();
  gxString m_buf;
  unsigned long buf_end = parm->buf_start + parm->buf_len;
  unsigned long j;

  if(logcfg->verbose) {
    RunTestEchoMessage("In verbose mode echoing to stderr", parm->thread_number);
  }

  if(parm->buf_start > buf_end) {
    m_buf << clear << "Invalid start buffer size" << "\n";
    m_buf << "Buffer start    = " << parm->buf_start << "\n"
	  << "Buffer end      = " << buf_end << "\n"
	  << "Buffer len      = " << parm->buf_len << "\n"
	  << "Buffer Overhead = " << parm->cytpro_overhead;
    RunTestEchoMessage(m_buf.c_str(), parm->thread_number);
    return 0;
  }
  if(buf_end > parm->buf_size) {
    m_buf << clear << "Invalid end buffer size" << "\n";
    m_buf << "Buffer start    = " << parm->buf_start << "\n"
	  << "Buffer end      = " << buf_end << "\n"
	  << "Buffer len      = " << parm->buf_len << "\n"
	  << "Buffer Overhead = " << parm->cytpro_overhead;
    RunTestEchoMessage(m_buf.c_str(), parm->thread_number);
    return 0;
  }

  if(parm->buf_len > buf_end) {
    m_buf << clear << "Invalid start and end buffer sizes" << "\n";
    m_buf << "Buffer start    = " << parm->buf_start << "\n"
	  << "Buffer end      = " << buf_end << "\n"
	  << "Buffer len      = " << parm->buf_len << "\n"
	  << "Buffer Overhead = " << parm->cytpro_overhead;
    RunTestEchoMessage(m_buf.c_str(), parm->thread_number);
    return 0;
  }

  m_buf.Clear();
  RunTestFormatMessage(m_buf, parm->thread_number);
  m_buf << "Buffer size is set to " << parm->buf_len << " bytes";
  parm->output << m_buf << "\n";
  if(logcfg->verbose) ConsoleWriteErr(m_buf.c_str());

  if(logcfg->verbose){ 
    m_buf << clear << "Memory buffer stats" << "\n";
    m_buf << "Buffer start    = " << parm->buf_start << "\n"
	  << "Buffer end      = " << buf_end << "\n"
	  << "Buffer len      = " << parm->buf_len << "\n"
	  << "Buffer Overhead = " << parm->cytpro_overhead;
    RunTestEchoMessage(m_buf.c_str(), parm->thread_number);
  }

  int num_inserts = 0;

  char *start = parm->shared_membuf + parm->buf_start;

  // Get CPU clock cycles before starting CPU test
  clock_t begin = clock();

  double insert_time = 0;
  clock_t begin_insert, end_insert;

  begin_insert = clock();
  fillrand((unsigned char *)start, parm->buf_len);
  m_buf.Clear();
  RunTestFormatMessage(m_buf, parm->thread_number);
  if(logcfg->verbose) ConsoleWriteErr(m_buf.c_str());

  Bytef *dest = 0;
  uLong destLen;
  int rv;

  // Compress the buffer
  begin_insert = clock();
  destLen = calc_compress_len(parm->buf_len);
  dest = new Bytef[destLen];
  rv = compress2(dest, &destLen, (const Bytef *)start, (uLong)parm->buf_len, Z_BEST_COMPRESSION);

  if(rv != Z_OK) {
    switch(rv) {
      case Z_MEM_ERROR:
	RunTestEchoMessage("There was not enough memory to compress buffer", parm->thread_number);
	break;
      case Z_BUF_ERROR:
	RunTestEchoMessage("There was not enough room in output buffer to compress", 
			   parm->thread_number);
	break;
      case Z_STREAM_ERROR:
	RunTestEchoMessage("The compression level parameter is invalid", parm->thread_number);
	break;
      default: 
	RunTestEchoMessage("Error uncompressing memory buffer", parm->thread_number);
	break;
    }
  }

  if(rv != Z_OK) {
    if(dest) delete[] dest;
    return 0;
  }

  if(!dest) {
    RunTestEchoMessage("Error after compressing memory buffer", parm->thread_number);
    return 0;
  }
  end_insert = clock();
  insert_time += (double)(end_insert - begin_insert) / CLOCKS_PER_SEC;
  num_inserts++;

  Bytef *dup = 0;
  uLong dupLen = parm->buf_len;

  // Uncompress the buffer
  begin_insert = clock();
  dup = new Bytef[parm->buf_len];
 
  rv = uncompress(dup, &dupLen, (const Bytef *)dest, destLen);

  end_insert = clock();
  insert_time += (double)(end_insert - begin_insert) / CLOCKS_PER_SEC;
  num_inserts++;

  // Free the duplicate sptr string
  delete[] dest;
  dest = 0;

  if(rv != Z_OK) {
    switch(rv) {
      case Z_MEM_ERROR:
	RunTestEchoMessage("There was not enough memory to uncompress buffer", parm->thread_number);
	break;
      case Z_BUF_ERROR:
	RunTestEchoMessage("There was not enough room in output buffer to uncompress", 
			   parm->thread_number);
	break;
      case Z_DATA_ERROR:
	RunTestEchoMessage("The compressed data is corrupted", parm->thread_number);
	break;
      default: 
	RunTestEchoMessage("Error uncompressing memory buffer", parm->thread_number);
	break;
    }
  }
  if(rv != Z_OK) {
    if(dup) delete[] dup;
    return 0;
  }

  if(!dup) {
    RunTestEchoMessage("Error after uncompressing memory buffer", parm->thread_number);
    return 0;
  }

  if(dupLen > (parm->buf_len+parm->cytpro_overhead)) {
    RunTestEchoMessage("Buffer size too big after uncompressing buffer", parm->thread_number);
    if(dup) delete[] dup;
  }

  delete[] dup;
  dup = 0;

  // Get CPU clock cycles after test is complete
  clock_t end_time = clock();

  // Calculate the elapsed time in seconds. 
  double elapsed_time = (double)(end_time - begin) / CLOCKS_PER_SEC;
  parm->run_time = elapsed_time;

  parm->output.Precision(3); 
  RunTestFormatMessage(parm->output, parm->thread_number);
  parm->output << "CPU speed test elapsed time " << elapsed_time << " seconds" << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestFormatMessage(m_buf, parm->thread_number);
    m_buf << "CPU speed test test elapsed time " << elapsed_time << " seconds";
    ConsoleWriteErr(m_buf.c_str());
  }
  double avg_insert_time = insert_time/num_inserts * 1000;
  parm->average_time = avg_insert_time;

  RunTestFormatMessage(parm->output, parm->thread_number);
  parm->output << "Average speed during test = " << avg_insert_time << " milliseconds" << "\n"; 
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestFormatMessage(m_buf, parm->thread_number);
    m_buf << "Average speed during test = " << avg_insert_time << " milliseconds";
    ConsoleWriteErr(m_buf.c_str());
  }

  if(logcfg->verbose) {
    RunTestEchoMessage("End of verbose output", parm->thread_number);
  }

  return 1;
}

int fillrand(unsigned char *buf, const unsigned long len)
{   
  unsigned long i, j;
  gxRandom random;
  unsigned char bytes[4];

  for(i = 0; i < len; ++i) {
    unsigned rand_num = random.RandomNumber();
    bytes[0] = (char)(rand_num & 0xFF);
    bytes[1] = (char)((rand_num & 0xFF00)>>8);
    bytes[2] = (char)((rand_num & 0xFF0000)>>16);
    bytes[3] = (char)((rand_num & 0xFF000000)>>24);
    for(j = 0; j < 4; j++) {
      buf[i++] = bytes[j];
    }
  }

  return len;
}

uLong calc_compress_len(uLong len)
{
  return ((len * 2) + (12+128));
  // NOTE: The following is from the zlib documentation
  // return (((.1/100 * len) * len) +12);
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
