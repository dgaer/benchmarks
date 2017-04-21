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

Program used for CPU speed benchmark testing.
*/
// ----------------------------------------------------------- // 
#include "gxdlcode.h"

#if defined (__USE_ANSI_CPP__) // Use the ANSI Standard C++ library
#include <iostream>
using namespace std; // Use unqualified names for Standard C++ library
#else // Use the old iostream library by default
#include <iostream.h>
#endif // __USE_ANSI_CPP__

#ifdef __MSVC_DEBUG__
#include "leaktest.h"
#endif

#include "m_thread.h"
#include "m_log.h"

// Version number and program name
const char *VersionString = "1.28";
const char *ProgramDescription = "CPU speed benchmark test";
const char *ProducedBy = "Douglas.Gaer@noaa.gov";
gxString ServiceName = "cputest";
gxString ProgramName = ServiceName; // Default program name


// Buffer overhead includes space for compress header 
// and compression overhead
const unsigned BuffreOverhead = 100 * 1024; 

// Set our defaults here
unsigned long BUF_SIZE = 100000 * 1024;  // 100MEG test
long NUM_THREADS = 20;

// Set the global veriables here
int num_command_line_args = 0;

// Function declarations
void VersionMessage();
void HelpMessage(int short_help_menu = 0);
int ProcessArgs(int argc, char *argv[]);
int ProcessDashDashArg(gxString &arg);

// Start our program's main thread
int main(int argc, char **argv)
{
#ifdef __MSVC_DEBUG__
  InitLeakTest();
#endif

  int narg;
  char *arg = argv[narg = 1];
  gxString sbuf;
  int num_strings;

  // Load all the command line arguments
  while(narg < argc) {
    if(arg[0] != '\0') {
      if(arg[0] == '-') { // Look for command line arguments
	// Exit if argument is not valid or argument signals program to exit
	if(!ProcessArgs(argc, argv)) {
	  return 1; 
	}
      }
      else {
	// NOTE: Process all non -- or - arguments here
	sbuf = arg;
	num_strings++;
      }
    }
    arg = argv[++narg];
  }

  CPUTestThread *t = new CPUTestThread;
  thrPool *pool = new thrPool;
  long i;

  // Set this program's name and service name
  ProgramName = argv[0];
  logcfg->service_name = ServiceName;

  VersionMessage(); // Display the program version information
  HelpMessage(1); // Display the short help menu

  ConsoleWrite("Starting multi-threaded CPU benchmark program.");
  ConsoleWrite("Benchmarking using zlib compression.");
  ConsoleWrite("Benchmark represents tasking CPU utilization.\n");
  sbuf << clear << "Benchmark will be using " << BUF_SIZE << " memory buffer.";
  ConsoleWrite(sbuf.c_str());
  sbuf << clear << "Number of concurrent processing threads: " << NUM_THREADS;
  ConsoleWrite(sbuf.c_str());
  ConsoleWrite("Beginning benchmark testing now...");
  ConsoleWrite("Please be patient while all thread complete.");
  ConsoleWrite("Press Ctrl-C if you need to exit this program before completion.\n");

  ConsoleWrite("Initializing shared memory buffer...");
  unsigned total_buf_size = BUF_SIZE + (BuffreOverhead * NUM_THREADS);
  sbuf << clear << "Shared memory size = " << total_buf_size;
  ConsoleWrite(sbuf.c_str());
  char *shared_membuf = new char[total_buf_size];
  // Fill the buffer with random characters
  memset(shared_membuf, 0, total_buf_size);
  fillrand((unsigned char *)shared_membuf, total_buf_size);  
  ConsoleWrite("Share memory initialization complete.\n");

  unsigned buf_size_per_thread = BUF_SIZE / NUM_THREADS;
  unsigned buf_start = 0;

  gxThreadPriority prio = gxTHREAD_PRIORITY_NORMAL;
  gxThreadType type = gxTHREAD_TYPE_JOINABLE;
  gxStackSizeType stack_size = 5000 * 1000;

  // Get CPU clock cycles before starting CPU test
  clock_t begin = clock();

  global_tvars->Reset();
  
  for(i = 0; i < NUM_THREADS; i++) {
    sbuf << clear << "Starting thread: " << i; 
    ConsoleWriteErr(sbuf.c_str());
    CPUThreadParm *parm = new CPUThreadParm;
    parm->shared_membuf = shared_membuf;
    parm->buf_size = total_buf_size;
    parm->cytpro_overhead = BuffreOverhead;
    parm->buf_start = buf_start;
    parm->buf_len = buf_size_per_thread;
    parm->thread_number = i;
    t->CreateThread(pool, (void *)parm, prio, type, stack_size);
    buf_start += (buf_size_per_thread + BuffreOverhead);
  }
  
  // Wait for all threads to exit 
  ConsoleWriteErr("Waiting for all threads to complete");
  t->JoinThread(pool);

  // Destroying the thread pool
  if(!pool->IsEmpty()) {
    t->DestroyThreadPool(pool);
  }
  else {
    // If DestroyThreadPool() is not called the pool will not be deleted
    delete pool; 
  }
  
  // Get CPU clock cycles after test is complet
  clock_t end_time = clock();

  // Calculate the elapsed time in seconds. 
  double elapsed_time = (double)(end_time - begin) / CLOCKS_PER_SEC;
  sbuf.Clear();
  sbuf.Precision(3); 
  sbuf << "All threads completed CPU speed test in " << elapsed_time << " seconds";
  ConsoleWrite(sbuf.c_str());

  double average_time;
  global_tvars->GetAverageTime(average_time);
  average_time /= NUM_THREADS;
  sbuf.Clear();
  sbuf.Precision(3); 
  sbuf << "Average time per threads is " << average_time << " seconds";
  ConsoleWrite(sbuf.c_str());

  ConsoleWrite("\nBenchmark testing complete");
  ConsoleWriteErr("Exiting...");

  delete[] shared_membuf;
  delete t;
  
  return 0; // Exit the process, terminating all threads
}

void VersionMessage()
{
  GXSTD::cout << "\n" << GXSTD::flush;
  GXSTD::cout << ProgramDescription << " version " << VersionString  << "\n" << GXSTD::flush;
  GXSTD::cout << "Produced by: " << ProducedBy << "\n" << GXSTD::flush;
  GXSTD::cout << "\n" << GXSTD::flush;
}

void HelpMessage(int short_help_menu)
{
  if(short_help_menu) {
    if(num_command_line_args > 0) {
      GXSTD::cout << "Starting program with command line arguments." << "\n" << GXSTD::flush;
    }
    else {
      GXSTD::cout << "Starting program using default arguments." << "\n" << GXSTD::flush;
    }
    GXSTD::cout << "Run with the --help argument for list of switches." << "\n" << GXSTD::flush;
    GXSTD::cout << "\n" << GXSTD::flush;
    return;
  }
  
  // We are displaying the long help menu
  GXSTD::cout << "Usage: " << ProgramName << " [args]" << "\n" << GXSTD::flush;
  GXSTD::cout << "args: " << "\n" << GXSTD::flush;
  GXSTD::cout << "     --help                Print help message and exit" << "\n" << GXSTD::flush;
  GXSTD::cout << "     --version             Print version number and exit" << "\n" << GXSTD::flush;
  GXSTD::cout << "     --debug               Enable debug mode default level --debug=1" << "\n" << GXSTD::flush;
  GXSTD::cout << "     --verbose             Turn on verbose messaging to stderr" << "\n" << GXSTD::flush;
  GXSTD::cout << "     --num-threads=NUM     Number of thread to spawn" << "\n" << GXSTD::flush;
  GXSTD::cout << "     --buf-size=SIZE       Size of memory buffer: 24k 240m 2.4g" << "\n" << GXSTD::flush;
  GXSTD::cout << "\n" << GXSTD::flush;
  return;
}

int ProcessArgs(int argc, char *argv[])
{
  // process the program's argument list
  int i;
  gxString sbuf;
  num_command_line_args = 0;

  for(i = 1; i < argc; i++ ) {
    if(*argv[i] == '-') {
      char sw = *(argv[i] +1);
      switch(sw) {
	case '?':
	  HelpMessage();
	  return 0; // Signal program to exit

	case '-':
	  sbuf = &argv[i][2]; 
	  // Add all -- prepend filters here
	  sbuf.TrimLeading('-');
	  if(!ProcessDashDashArg(sbuf)) return 0;
	  break;

	default:
	  GXSTD::cout << "Error unknown command line switch " << argv[i] << "\n" << flush;
	  return 0;
      }
      num_command_line_args++;
    }
  }
  
  return 1; // All command line arguments were valid
}

int ProcessDashDashArg(gxString &arg)
{
  gxString sbuf, equal_arg;
  int has_valid_args = 0;
  
  if(arg.Find("=") != -1) {
    // Look for equal arguments
    // --log-file="/var/log/my_service.log"
    equal_arg = arg;
    equal_arg.DeleteBeforeIncluding("=");
    arg.DeleteAfterIncluding("=");
    equal_arg.TrimLeading(' '); equal_arg.TrimTrailing(' ');
    equal_arg.TrimLeading('\"'); equal_arg.TrimTrailing('\"');
    equal_arg.TrimLeading('\''); equal_arg.TrimTrailing('\'');
  }

  arg.ToLower();

  // Process all -- arguments here
  if(arg == "help") {
    HelpMessage();
    return 0; // Signal program to exit
  }
  if(arg == "?") {
    HelpMessage();
    return 0; // Signal program to exit
  }

  if((arg == "version") || (arg == "ver")) {
    VersionMessage();
    return 0; // Signal program to exit
  }

  if(arg == "debug") {
    logcfg->debug = 1;
    if(!equal_arg.is_null()) { 
      logcfg->debug_level = equal_arg.Atoi(); 
    }
    has_valid_args = 1;
  }
  if(arg == "verbose") {
    logcfg->verbose = 1;
    has_valid_args = 1;
  }

  if(arg == "num-threads") {
    if(equal_arg.is_null()) { 
      GXSTD::cout << "Error no number of threads supplied with the --num_threads swtich" 
		  << "\n" << flush;
      return 0;
    }
    NUM_THREADS = equal_arg.Atoi();
    if(NUM_THREADS <= 0) {
      GXSTD::cout << "Error number of threads is not valid" 
		  << "\n" << flush;
      return 0;
    }
    has_valid_args = 1;
  }

  if(arg == "buf-size") {
    if(equal_arg.is_null()) { 
      GXSTD::cout << "Error no number supplied with the --buf-size swtich" 
		  << "\n" << flush;
      return 0;
    }
    equal_arg.ToLower();
    gxString k = equal_arg.Right(1);
    gxString m = equal_arg.Right(1);
    gxString g = equal_arg.Right(1);
    if(k == "k") {
      equal_arg.DeleteAfterIncluding("k");
      if(equal_arg.Atoi() <= 0) {
	GXSTD::cout << "Error buffer size value is not valid" 
		    << "\n" << flush;
	return 0;
      }
      BUF_SIZE = 1000;
      BUF_SIZE *= equal_arg.Atoi();
    }
    else if(m == "m") {
      equal_arg.DeleteAfterIncluding("m");
      if(equal_arg.Atoi() <= 0) {
	GXSTD::cout << "Error buffer size value is not valid" 
		    << "\n" << flush;
	return 0;
      }
      BUF_SIZE = 1000 * 1000;
      BUF_SIZE *= equal_arg.Atoi();
    }
    else if(g == "g") {
      equal_arg.DeleteAfterIncluding("g");
      if(equal_arg.Atoi() <= 0) {
	GXSTD::cout << "Error buffer size value is not valid" 
		    << "\n" << flush;
	return 0;
      }
      BUF_SIZE = 1000000 * 1000;
      BUF_SIZE *= equal_arg.Atoi();
    }
    else {
      BUF_SIZE = equal_arg.Atoi();
    }
    if(BUF_SIZE <= 0) {
      GXSTD::cout << "Error buffer size value is not valid" 
		  << "\n" << flush;
      return 0;
    }
    has_valid_args = 1;
  }

  if(!has_valid_args) {
    GXSTD::cout << "Error unknown command line switch --" << arg << "\n" << flush;
  }
  arg.Clear();
  return has_valid_args;
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
