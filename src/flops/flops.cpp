// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- // 
// C++ Source Code File
// C++ Compiler Used: GNU
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

Program used for FLOPS testing.
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
const char *VersionString = "1.01";
const char *ProgramDescription = "FLOPS benchmark test";
const char *ProducedBy = "Douglas.Gaer@noaa.gov";
gxString ServiceName = "flops";
gxString ProgramName = ServiceName; // Default program name

// Set the global veriables here
unsigned long ITERATIONS = 999999999;
unsigned long NUM_THREADS = 1;
int run_dp_test = 0;
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

  FLOPSTestThread *t = new FLOPSTestThread;
  thrPool *pool = new thrPool;
  unsigned long i;

  // Set this program's name and service name
  ProgramName = argv[0];
  logcfg->service_name = ServiceName;

  VersionMessage(); // Display the program version information
  HelpMessage(1); // Display the short help menu

  unsigned long iterations = ITERATIONS/NUM_THREADS;

  ConsoleWrite("Starting FLOPS benchmark program.");
  ConsoleWrite("Benchmark represents floating point operations per second.\n");
  sbuf << clear << "Iterations for " <<  NUM_THREADS << " threads = " << iterations;
  ConsoleWrite(sbuf.c_str());
  ConsoleWrite("Beginning benchmark testing now...");
  ConsoleWrite("Please be patient while all threads complete.");
  ConsoleWrite("Press Ctrl-C if you need to exit this program before completion.\n");

  gxThreadPriority prio = gxTHREAD_PRIORITY_NORMAL;
  gxThreadType type = gxTHREAD_TYPE_JOINABLE;
  gxStackSizeType stack_size = 5000 * 1000;

  clock_t start, end;
  double cpu_time_used;
  double flop;

  // Get CPU clock cycles before starting test
  start = clock();

  for(i = 0; i < NUM_THREADS; i++) {
    sbuf << clear << "Starting thread: " << (i+1); 
    ConsoleWriteErr(sbuf.c_str());
    FLOPSThreadParm *parm = new FLOPSThreadParm;
    parm->iterations = iterations; 
    parm->thread_number = i;
    parm->run_dp_test = run_dp_test;
    t->CreateThread(pool, (void *)parm, prio, type, stack_size);
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
  
  // Get CPU clock cycles after test is complete
  end = clock();

  // Calculate the elapsed time in seconds. 
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;


  sbuf.Clear();
  sbuf.Precision(3); 
  sbuf << "All threads completed FLOPS test in " << cpu_time_used << " seconds";
  ConsoleWrite(sbuf.c_str());

  flop = ITERATIONS;
  flop *= 5;
  flop /= cpu_time_used;

  sbuf.Clear();
  sbuf.Precision(3); 
  if(flop/1000000000000>=1){
    flop/=1000000000000;
    sbuf << "FLOPS: " << flop << " TFLOPS";
  }
  else {
    flop/=1000000000;
    sbuf << "FLOPS: " << flop << " GFLOPS";
  }
  ConsoleWrite(sbuf.c_str());

  ConsoleWrite("\nBenchmark testing complete");
  ConsoleWriteErr("Exiting...");

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
  GXSTD::cout << "     --num-threads=NUM     Number of thread to run" << "\n" << GXSTD::flush;
  GXSTD::cout << "     --dp                  Double precision test (defaults to single  precision)" << "\n" << GXSTD::flush;
  GXSTD::cout << "     --iterations=NUM      Iterations (defaults to 999999999)" << "\n" << GXSTD::flush;
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

  if(arg == "dp") {
    run_dp_test = 1;
    has_valid_args = 1;
  }

  if(arg == "iterations") {
    if(equal_arg.is_null()) { 
      GXSTD::cout << "Error no number of iterations supplied with the --iterations swtich" 
		  << "\n" << flush;
      return 0;
    }
    ITERATIONS = equal_arg.Atol();
    if(ITERATIONS <= 0) {
      GXSTD::cout << "Error number of iterations is not valid" 
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
