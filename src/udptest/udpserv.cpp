// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- // 
// C++ Source Code File
// Compiler Used: GNU, Intel, Cray
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

UDP benchmark server.
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

#if defined (__LINUX__)
#include <sys/signal.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "m_thread.h"

// --------------------------------------------------------------
// Constants
// --------------------------------------------------------------
const char *version_str = "2.01";
const char *program_name = "udpserv";
const char *author_email = "Douglas.Gaer@noaa.gov";
// --------------------------------------------------------------

// --------------------------------------------------------------
// Globals
// --------------------------------------------------------------
gxString logfile_name;
int enable_logging = 0;
gxsPort_t port = DEFAULT_PORT;
// --------------------------------------------------------------

class ConsoleThread : public gxThread
{
public:
  ConsoleThread() { }
  ~ConsoleThread() { }

private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};

void *ConsoleThread::ThreadEntryRoutine(gxThread_t *thread)
{
  const int cmd_len = 255;
  char sbuf[cmd_len];
  
  while(1) {
    for(int i = 0; i < cmd_len; i++) sbuf[i] = 0;
    cin >> sbuf;
    if(strcmp(sbuf, "quit") == 0) break;
    if(strcmp(sbuf, "exit") == 0) break;
    cout << "Invalid command" << "\n" << flush;
    cout << "Enter quit to exit" << "\n" << flush;
  }

  // Wake up the process thread waiting on this condition
  servercfg->accept_clients = 0; // Do not accept any more clients
  servercfg->kill_server = 1;
  servercfg->process_loop = 0; // Signal to stop the console thread

  // Wake up the thread and exit
  servercfg->process_cond.ConditionSignal(); 
  servercfg->process_lock.MutexUnlock();

  return (void *)0;
}

void HelpMessage() 
{
  cout << "UDP bandwidth benchmark server version" << version_str << "\n" 
       << flush;
  cout << "Producted by: " << author_email << "\n" << flush;
  cout << "\n" << flush;
  cout << "Usage: " << program_name << " [switches] server " 
       << "\n" << flush;
  cout << "Switches: " << "\n" << flush;
  cout << "-? = Display this help message." << "\n" << flush;
  cout << "-l[name] = Write output to log file." << "\n" << flush;
  cout << "-p = Specify a port number, ranges -p1024 to -p65535"
       << "\n" << flush;

    // End of list
  cout << "\n";
}

int ProcessArgs(char *arg)
{
  gxString sbuf;
  switch(arg[1]) {
    case 'p': case 'P':
      port = (unsigned short)atoi(arg+2);
      if((port <= 0) || (port > 65535)) {
	cout << "Bad port number specified: " << port << "\n";
	cout << "Port number should range between 1024 to 65535" << "\n";
	// http://www.iana.org/assignments/port-numbers
	return 0;
      }
      break;

    case 'l': case 'L':
      enable_logging = 1;
      logfile_name = arg+2;
      break;

    case 'h': case 'H': case '?':
      HelpMessage();
      return 0;

    default:
      cout << "\n" << flush;
      cout << "Unknown switch " << arg << "\n" << flush;
      cout << "Exiting..." << "\n" << flush;
      cout << "\n" << flush;
      return 0;
  }
  arg[0] = '\0';

  return 1; // All command line arguments were valid
}

int StartServer()
{
  SysTime systime;
  LogFile logfile;

  cout << "UDP LAN/WAN benchmark server" << "\n" 
       << flush;
  cout << "Initializing server @" << systime.GetSystemDateTime() 
       << "\n" << flush;
  ServerThread server;
  int rv;

  int retires = RESTART_RETRIES;
  while(retires--) {
    rv = server.InitServer();
    if(rv == 0) break; // Server initialized
    SleepFor(SLEEP_SECS_BEFORE_RETRY);
  }
  if(rv != 0) {
    cout << server.SocketExceptionMessage() << "\n" << flush;
    cout << "Exiting..." << "\n" << flush;
    return 1;
  }

  // Get the host name assigned to this machine
  UString hostname(gxsMAX_NAME_LEN);
  rv = server.GetHostName((char *)hostname.GetSPtr());
  if(rv != 0) {
    cout << "Cannot obtain hostname for local host" << "\n" << flush;
    cout << server.SocketExceptionMessage() << "\n" << flush;
    hostname << clear << "localhost";
  }
  else {
    hostname.SetLength(); // Set the string length after GetHostName() call
  }

  cout << "Opening " << program_name << " server on host " << hostname.c_str()
       << "\n" << flush;
  cout << "Enter quit to exit" << "\n" << flush;
  cout << "\n" << flush;
  cout << "Accepting clients on port " << servercfg->port << "\n" << flush;

  if(enable_logging) {
    if(logfile.Open(logfile_name.c_str()) == 0) {
      servercfg->logfile = &logfile;
    }
    else {
      cout << "Could not open the " << logfile_name.c_str() 
	   << " logfile" << "\n" << flush;
    }
  }

  gxThread_t *server_thread = server.CreateThread();
  if(!server_thread) {
    cout << "Encountered fatal error starting the server" << "\n" << flush;
    cout << "Could not create server thread" << "\n" << flush;
    return 1;
  }
  if(server_thread->GetThreadError() != gxTHREAD_NO_ERROR) {
    cout << "Encountered fatal error starting the server" << "\n" << flush;
    cout << server_thread->ThreadExceptionMessage() << "\n" << flush;
    delete server_thread;
    return 1;
  }
  
  ProcessThread process;
  gxThread_t *process_thread = process.CreateThread();
  if(!server_thread) {
    cout << "Encountered fatal error starting the server" << "\n" << flush;
    cout << "Could not create process thread" << "\n" << flush;
    delete server_thread;
    return 1;
  }
  if(process_thread->GetThreadError() != gxTHREAD_NO_ERROR) {
    cout << "Encountered fatal error starting the server" << "\n" << flush;
    cout << process_thread->ThreadExceptionMessage() << "\n" << flush;
    delete server_thread;
    delete process_thread;
    return 1;
  }
    
  ConsoleThread console; // Used for interactive console commands
  gxThread_t *console_thread = console.CreateThread();
  if(console_thread->GetThreadError() != gxTHREAD_NO_ERROR) {
    cout << "Encountered fatal error starting the server" << "\n" << flush;
    cout << console_thread->ThreadExceptionMessage() << "\n" << flush;
    delete server_thread;
    delete process_thread;
    return 1;
  }


  if(process.JoinThread(process_thread) != 0) {
    cout << "Encountered fatal error starting the server" << "\n" << flush;
    cout << "Could not join the process thread" << "\n" << flush;
    delete server_thread;
    delete process_thread;
    delete console_thread;
    return 1;
  }

  cout << "\n" << "Server shutdown @" << systime.GetSystemDateTime() 
       << "\n" << flush;

  cout << "Stopping the server thread..." << "\n" << flush;
  servercfg->accept_clients = 0;
  servercfg->echo_loop = 0;

  // Cannot the server thread due to the blocking accept() function, so
  // cancel the thread before it is destroyed
  server.CancelThread(server_thread);
  if(server_thread->GetThreadState() == gxTHREAD_STATE_CANCELED) {
    cout << "The server thread was stopped" << "\n" << flush;
  }

  // The shutdown was completed with no errors
  delete server_thread;
  delete process_thread;
  delete console_thread;
  return 0;
}

int main(int argc, char **argv)
{
#ifdef __MSVC_DEBUG__
  InitLeakTest();
#endif

#if defined (__LINUX__)
  // NOTE: Linux will kill a process that attempts to write to a pipe 
  // with no readers. This application may send data to client socket 
  // that has disconnected. If we do not tell the kernel to explicitly 
  // ignore the broken pipe the kernel will kill this process.
  signal(SIGPIPE, SIG_IGN);
#endif

  ServerThread server;
  int narg;
  char *arg = argv[narg = 1];
  LogFile logfile;

  if(argc >= 2) {
    while (narg < argc) {
      if (arg[0] != '\0') {
	if (arg[0] == '-') { // Look for command line arguments
	  if(!ProcessArgs(arg)) return 1; // Exit if argument is not valid
	}
	else { 
	  // Process the command line string after last - argument
	}
	arg = argv[++narg];
      }
    }
  }


  // Start the server and wait for a kill or restart comman
  SysTime systime;
  ProcessThread process; // Used only for sleep function
  while(!servercfg->kill_server) {
    if(StartServer() != 0) break;
    if(servercfg->restart_server) {
      cout << "Restarting the server @" << systime.GetSystemDateTime() 
	   << "\n" << flush;
      cout << "Sleeping for " << SLEEP_SECS_BEFORE_RETRY 
	   << " second(s) before restart" << "\n" << flush;
      process.sSleep(SLEEP_SECS_BEFORE_RETRY); // Allow for I/O recovery time

      // Reset all the server varaibles
      servercfg->reset_all();
      servercfg->port = port;

      // Restart the server
      continue;
    }
  }

  cout << "Exiting the server..." << "\n" << flush;
  return 0;
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //


  

