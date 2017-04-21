// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- // 
// C++ Source Code File
// C++ Compiler Used: GNU, Intel, Cray
// Produced By: Douglas.Gaer@noaa.gov
// File Creation Date: 10/25/2002
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

Multicast benckmark server.

This program uses a UDP socket to join a IP multicast group 
and send/receive datagrams from a multicast group. Multicast 
addresses range from 224.0.0.0 to 239.255.255.255 with addresses 
from 224.0.0.0 to 224.0.0.255 reserved for multicast routing 
information. A multicast address identifies a set of receivers 
for multicast messages. A sender can send datagrams to any group 
without being a member of that group. Clients must be member of
the multicast group in order to receive the multicast datagram
message.
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
const char *version_str = "2.0";
const char *program_name = "mcserv";
const char *author_email = "Douglas.Gaer@noaa.gov";
// --------------------------------------------------------------

// --------------------------------------------------------------
// Globals
// --------------------------------------------------------------
gxString logfile_name;
int enable_logging = 0;
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
  cout << "Multicast bandwidth benchmark server" << version_str << "\n" 
       << flush;
  cout << "Producted by: " << author_email << "\n" << flush;
  cout << "\n" << flush;
  cout << "Usage: " << program_name << " [switches] server " 
       << "\n" << flush;
  cout << "Switches: " << "\n" << flush;
  cout << "-? = Display this help message." << "\n" << flush;
  cout << "-b = Set datagram size, ranges = -b64 to -b65507"
       << "\n" << flush;
  cout << "-l[name] = Write output to log file." << "\n" << flush;
  cout << "-g = Specify a multicast group, ranges 224.0.0.0 to 239.255.255.255"
       << "\n" << flush;
  cout << "-p = Specify a port number, ranges -p1024 to -p65535"
       << "\n" << flush;
  cout << "-t = Specify a TTL, values: " << "\n" << flush;
  cout << "     0   Restricted to the same host" << "\n" << flush;
  cout << "     1   Restricted to the same subnet" << "\n" << flush; 
  cout << "     32  Restricted to the same site" << "\n" << flush; 
  cout << "     64  Restricted to the same region" << "\n" << flush; 
  cout << "     128 Restricted to the same continent" << "\n" << flush; 
  cout << "     255 Unrestricted" << "\n" << flush; 
  
    // End of list
  cout << "\n";
}

int ProcessArgs(char *arg)
{
  gxString sbuf;
  switch(arg[1]) {
    case 'b': case 'B':
      servercfg->datagram_size = atoi(arg+2);
      if((servercfg->datagram_size < 64) || (servercfg->datagram_size > 65535)) {
	cout << "Bad datagram size specified: " << servercfg->datagram_size 
	     << "\n";
	cout << "Datagram sizes should range between 64 to 65535" << "\n";
	return 0;
      }
      break;

    case 'p': case 'P':
      servercfg->port = (unsigned short)atoi(arg+2);
      if((servercfg->port <= 0) || (servercfg->port > 65535)) {
	cout << "Bad port number specified: " << servercfg->port << "\n";
	cout << "Port number should range between 1024 to 65535" << "\n";
	// http://www.iana.org/assignments/port-numbers
	return 0;
      }
      break;

    case 't': case 'T':
      servercfg->TTL = (unsigned short)atoi(arg+2);
      if((servercfg->TTL < 0) || (servercfg->TTL > 255)) {
	cout << "Bad TTL number specified: " << servercfg->TTL << "\n";
	return 0;
      }
      break;

    case 'g': case 'G':
      servercfg->group = arg+2;
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

  int narg;
  char *arg = argv[narg = 1];
  LogFile logfile;
  SysTime systime;

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

  cout << "Multicast LAN/WAN benchmark server" << "\n" 
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
  cout << "Sending multicast messages to group: " << servercfg->group.c_str() 
       << "\n" << flush;
  cout << "Sending datagrams on port: " << servercfg->port << "\n" << flush;
  cout << "TTL: " << servercfg->TTL << "\n" << flush;
  cout << "Datagram size: " << servercfg->datagram_size << "\n" << flush;

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

#ifndef __SOLARIS__ // 09/24/2002: Hangs under Solaris
  if(server.JoinThread(server_thread) != 0) {
    cout << "Encountered fatal error stopping the server" << "\n" << flush;
    cout << "Could not join the server thread" << "\n" << flush;
    delete server_thread;
    delete process_thread;
    delete console_thread;
    return 1;
  }
#endif

  // The shutdown was completed with no errors
  delete server_thread;
  delete process_thread;
  delete console_thread;

  cout << "Exiting the server..." << "\n" << flush;
  return 0;
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
