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

void *MemoryTestThread::ThreadEntryRoutine(gxThread_t *thread)
// Thread's entry function
{

  MemoryThreadParm *parm = (MemoryThreadParm *)thread->GetThreadParm();
  RunTestTree(parm->tree, parm->num_insertions, parm->output, parm->thread_number);
  ConsoleWrite(parm->output.c_str());
  delete parm;
  return 0;
}

void RunTestTreeFormatMessage(gxString &output, int thread_number)
{
  output << "Thread[" << thread_number << "]: ";
}

void RunTestTree(gxBStree<gxString> &tree, unsigned long num_insertions, gxString &output, int thread_number)
// Test the tree insertion and deletion times.
{
  output.Clear();
  gxString m_buf;

  if(logcfg->verbose) {
    ConsoleWriteErr("\nIn verbose mode echoing to stderr");
  }

  RunTestTreeFormatMessage(output, thread_number);
  output << "Inserting " << num_insertions << " keys..." << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Inserting " << num_insertions << " keys...";
    ConsoleWriteErr(m_buf.c_str());
  }

  unsigned long i, key_count = 0;
  unsigned long curr_count = 0;
  int status;
  int verify_deletions = 0; // Set to ture to verify all deletions
  double insert_time = 0;
  const char *name = "Item";
  char sbuf[255];
  gxString key;

  // Get CPU clock cycles before entering loop
  clock_t begin = clock();

  for(i = 0; i < num_insertions; i++) {
    sprintf(sbuf, "%s %d", name, (int)i);
    key = sbuf;
    clock_t begin_insert = clock();
    status = tree.Insert(key);
    clock_t end_insert = clock();
    insert_time += (double)(end_insert - begin_insert) / CLOCKS_PER_SEC;
    key_count++;
    curr_count++;

    if(status != 1) {
      RunTestTreeFormatMessage(output, thread_number);
      output << "Problem adding key - " << key << "\n";
      if(logcfg->verbose) {
	m_buf.Clear();
	RunTestTreeFormatMessage(m_buf, thread_number);
	m_buf << "Problem adding key - " << key;
	ConsoleWriteErr(m_buf.c_str());
      }
      return;
    }
    if(curr_count >= (.1*num_insertions)) {
      curr_count = 0;
      RunTestTreeFormatMessage(output, thread_number);
      output << "Inserted " << i << " keys in " << insert_time << " seconds" << "\n";
      if(logcfg->verbose) {
	m_buf.Clear();
	RunTestTreeFormatMessage(m_buf, thread_number);
	m_buf << "Inserted " << i << " keys in " << insert_time << " seconds";
	ConsoleWriteErr(m_buf.c_str());
      }
    }
  }

  // Get CPU clock cycles after loop is completed 
  clock_t end =clock();

  // Calculate the elapsed time in seconds. 
  double elapsed_time = (double)(end - begin) / CLOCKS_PER_SEC;
  output.Precision(3); 
  RunTestTreeFormatMessage(output, thread_number);
  output << "Inserted " << key_count << " values in " << elapsed_time << " seconds" << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Inserted " << key_count << " values in " << elapsed_time << " seconds";
    ConsoleWriteErr(m_buf.c_str());
  }
  double avg_insert_time = (insert_time/(double)key_count) * 1000;
  RunTestTreeFormatMessage(output, thread_number);
  output << "Average insert time = " << avg_insert_time << " milliseconds" << "\n"; 
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Average insert time = " << avg_insert_time << " milliseconds";
    ConsoleWriteErr(m_buf.c_str());
  }

  key_count = 0;
  double search_time = 0;
  RunTestTreeFormatMessage(output, thread_number);
  output << "Verifying the insertions..." << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Verifying the insertions...";
    ConsoleWriteErr(m_buf.c_str());
  }
  begin = clock();
  for(i = 0; i < num_insertions; i++) {
    sprintf(sbuf, "%s %d", name, (int)i);
    key = sbuf;
    clock_t begin_search = clock();
    status = tree.Find(key);
    clock_t end_search = clock();
    key_count++;
    search_time += (double)(end_search - begin_search) / CLOCKS_PER_SEC;
    
    if(status != 1) {
      RunTestTreeFormatMessage(output, thread_number);
      output << "Error finding key - " << key << "\n";
      if(logcfg->verbose) {
	m_buf.Clear();
	RunTestTreeFormatMessage(m_buf, thread_number);
	m_buf << "Error finding key - " << key;
	ConsoleWriteErr(m_buf.c_str());
      }
      return;
    }
  }

  end =clock();
  elapsed_time = (double)(end - begin) / CLOCKS_PER_SEC;
  output.Precision(3);
  RunTestTreeFormatMessage(output, thread_number);
  output << "Verified " << key_count << " values in " << elapsed_time << " seconds" << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Verified " << key_count << " values in " << elapsed_time << " seconds";
    ConsoleWriteErr(m_buf.c_str());
  }
  double avg_search_time = (search_time/(double)key_count) * 1000;
  RunTestTreeFormatMessage(output, thread_number);
  output << "Average search time = " << avg_search_time << " milliseconds" << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Average search time = " << avg_search_time << " milliseconds";
    ConsoleWriteErr(m_buf.c_str());
  }

  RunTestTreeFormatMessage(output, thread_number);
  output << "Deleting all the entries..." << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Deleting all the entries...";
    ConsoleWriteErr(m_buf.c_str());
  }
  key_count = 0;
  double delete_time = 0;
  begin = clock();
  for(i = 0; i < num_insertions; i++) {
    sprintf(sbuf, "%s %d", name, (int)i);
    key = sbuf;
    clock_t begin_delete = clock();
    status = tree.Delete(key);
    clock_t end_delete = clock();
    delete_time += (double)(end_delete - begin_delete) / CLOCKS_PER_SEC;
    key_count++;
    if(status != 1) {
      RunTestTreeFormatMessage(output, thread_number);
      output << "Error deleting key - " << key << "\n";
      if(logcfg->verbose) {
	m_buf.Clear();
	RunTestTreeFormatMessage(m_buf, thread_number);
	m_buf << "Error deleting key - " << key;
	ConsoleWriteErr(m_buf.c_str());
      }
      return;
    }

    if(verify_deletions) { // Verify the remaining key locations
      for(unsigned long j = num_insertions-1; j != i; j--) {
	sprintf(sbuf, "%s %d", name, (int)j);
	key = sbuf;
	status = tree.Find(key);
	if(status != 1) {
	  RunTestTreeFormatMessage(output, thread_number);
	  output << "Error finding key  - " << j << "\n";
	  if(logcfg->verbose) {
	    m_buf.Clear();
	    RunTestTreeFormatMessage(m_buf, thread_number);
	    m_buf << "Error finding key  - " << j;
	    ConsoleWriteErr(m_buf.c_str());
	  }
	  RunTestTreeFormatMessage(output, thread_number);
	  output << "After deleting key - " << i << "\n";
	  if(logcfg->verbose) {
	    m_buf.Clear();
	    RunTestTreeFormatMessage(m_buf, thread_number);
	    m_buf << "After deleting key - " << i;
	    ConsoleWriteErr(m_buf.c_str());
	  }
	  return;
	}
      }
    }
  }

  end =clock();
  output.Precision(3);
  elapsed_time = (double)(end - begin) / CLOCKS_PER_SEC;
  RunTestTreeFormatMessage(output, thread_number);
  output << "Deleted " << key_count << " values in " << elapsed_time << " seconds" << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Deleted " << key_count << " values in " << elapsed_time << " seconds";
    ConsoleWriteErr(m_buf.c_str());
  }
  double avg_delete_time = (delete_time/(double)key_count) * 1000;
  RunTestTreeFormatMessage(output, thread_number);
  output << "Average delete time = " << avg_delete_time << " milliseconds" << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Average delete time = " << avg_delete_time << " milliseconds";
    ConsoleWriteErr(m_buf.c_str());
  }

  RunTestTreeFormatMessage(output, thread_number);
  output << "Re-inserting " << num_insertions << " keys..." << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Re-inserting " << num_insertions << " keys...";
    ConsoleWriteErr(m_buf.c_str());
  }

  key_count = 0;
  insert_time = 0;
  begin = clock();
  for(i = 0; i < num_insertions; i++) {
    sprintf(sbuf, "%s %d", name, (int)i);
    key = sbuf;
    clock_t begin_insert = clock();
    status = tree.Insert(key);
    clock_t end_insert = clock();
    insert_time += (double)(end_insert - begin_insert) / CLOCKS_PER_SEC;
    key_count++;
    curr_count++;

    if(status != 1) {
      RunTestTreeFormatMessage(output, thread_number);
      output << "Problem adding key - " << key << "\n";
      if(logcfg->verbose) {
	m_buf.Clear();
	RunTestTreeFormatMessage(m_buf, thread_number);
	m_buf << "Problem adding key - " << key;
	ConsoleWriteErr(m_buf.c_str());
      }
      return;
    }
  }
  end =clock();
  elapsed_time = (double)(end - begin) / CLOCKS_PER_SEC;
  output.Precision(3); 
  RunTestTreeFormatMessage(output, thread_number);
  output << "Inserted " << key_count << " values in " << elapsed_time << " seconds" << "\n";
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Inserted " << key_count << " values in " << elapsed_time << " seconds";
    ConsoleWriteErr(m_buf.c_str());
  }
  avg_insert_time = (insert_time/(double)key_count) * 1000;
  RunTestTreeFormatMessage(output, thread_number);
  output << "Average insert time = " << avg_insert_time << " milliseconds" << "\n"; 
  if(logcfg->verbose) {
    m_buf.Clear();
    m_buf.Precision(3); 
    RunTestTreeFormatMessage(m_buf, thread_number);
    m_buf << "Average insert time = " << avg_insert_time << " milliseconds";
    ConsoleWriteErr(m_buf.c_str());
  }

  if(logcfg->verbose) {
    ConsoleWriteErr("End of verbose output\n");
  }
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
