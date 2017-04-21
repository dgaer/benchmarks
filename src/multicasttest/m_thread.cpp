// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- // 
// C++ Source Code File Name: m_thread.cpp
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

UDP benchmark server classes.
*/
// ----------------------------------------------------------- // 
#include "gxdlcode.h"

#if defined (__USE_ANSI_CPP__) // Use the ANSI Standard C++ library
#include <iostream>
#else // Use the old iostream library by default
#include <iostream.h>
#endif // __USE_ANSI_CPP__

#include "m_thread.h"

// --------------------------------------------------------------
// Globals variable initialzation
// --------------------------------------------------------------
ServerConfig ServerConfigSruct;
ServerConfig *servercfg = &ServerConfigSruct;
// --------------------------------------------------------------

ServerConfig::ServerConfig()
{
  reset_all();
}

ServerConfig::~ServerConfig()
{

}

void ServerConfig::reset_all()
{
  port = DEFAULT_PORT;
  accept_clients = 1;
  echo_loop = 1;
  display_is_locked = 0;
  logfile = 0;
  kill_server = 0;
  restart_server = 0;
  process_loop = 1;
  TTL = DEFAULT_TTL;
  group = GetDefaultGroup();
  datagram_size = DEFAULT_DATAGRAM_SIZE;
}

void *ProcessThread::ThreadEntryRoutine(gxThread_t *thread)
{
  servercfg->process_lock.MutexLock();
  while(servercfg->process_loop) { 
    if(servercfg->kill_server) break;
    if(servercfg->restart_server) break;
    // Block this thread from its own execution 
    servercfg->process_cond.ConditionWait(&servercfg->process_lock);
  }

  SleepFor(1); // Allow for I/O recovery time before exiting the program
  return (void *)0;
}

int CheckSocketError(gxSocket *s, const char *mesg, int report_error)
// Test the socket for an error condition and report the message
// if the "report_error" variable is true. The "mesg" string is used
// display a message with the reported error. Returns true if an
// error was detected for false of no errors where detected. 
{
  if(s->GetSocketError() == gxSOCKET_NO_ERROR) {
    // No socket errors reported
    return 0;
  }
  if(report_error) { // Reporting the error to an output device
    if(mesg) {
      PrintMessage(mesg, "\n", s->SocketExceptionMessage());
    }
    else {
      PrintMessage(s->SocketExceptionMessage());
    }
  }
  return 1;
}

int CheckThreadError(gxThread_t *thread, const char *mesg, int report_error)
// Test the thread for an error condition and report the message
// if the "report_error" variable is true. The "mesg" string is used
// display a message with the reported error. Returns true if an
// error was detected for false of no errors where detected. 
{
  if(thread->GetThreadError() == gxTHREAD_NO_ERROR) {
    // No thread errors reported
    return 0;
  }
  if(report_error) { // Reporting the error to an output device
    if(mesg) {
      PrintMessage(mesg, "\n", thread->ThreadExceptionMessage()); 
    }
    else {
      PrintMessage(thread->ThreadExceptionMessage());
    }
  }
  return 1;
}

void ReportError(const char *s1, const char *s2, const char *s3)
// Thread safe error reporting function.
{
  PrintMessage(s1, s2, s3);
}

void PrintMessage(const char *s1, const char *s2, const char *s3)
// Thread safe write function that will not allow access to
// the critical section until the write operation is complete.
{
  servercfg->display_lock.MutexLock();

  // Ensure that all threads have finished writing to the device
  int num_try = 0;
  while(servercfg->display_is_locked != 0) {
    // Block this thread from its own execution if a another thread
    // is writing to the device
    if(++num_try < DISPLAY_THREAD_RETRIES) {
      servercfg->display_cond.ConditionWait(&servercfg->display_lock);
    }
    else {
      return; // Could not write string to the device
    }
  }

  // Tell other threads to wait until write is complete
  servercfg->display_is_locked = 1; 

  // ********** Enter Critical Section ******************* //
  if(s1) GXSTD::cout << s1;
  if(s2) GXSTD::cout << s2;
  if(s3) GXSTD::cout << s3;
  GXSTD::cout << "\n";
  GXSTD::cout.flush(); // Flush the ostream buffer to the stdio

  if(servercfg->logfile) {
    if(s1) *(servercfg->logfile) << s1;
    if(s2) *(servercfg->logfile) << s2;
    if(s3) *(servercfg->logfile) << s3;
    *(servercfg->logfile) << "\n";
    servercfg->logfile->df_Flush(); // Flush the disk buffers
  }
  // ********** Leave Critical Section ******************* //

  // Tell other threads that this write is complete
  servercfg->display_is_locked = 0; 
 
  // Wake up the next thread waiting on this condition
  servercfg->display_cond.ConditionSignal();
  servercfg->display_lock.MutexUnlock();
}

int ServerThread::InitServer()
// Initialize the server. Returns a non-zero value if any
// errors ocurr.
{
  if(InitSocketLibrary() == 0) {
    if(InitSocket(SOCK_DGRAM, SOCK_DGRAM, IPPROTO_UDP, 
		  servercfg->port, servercfg->group.c_str()) < 0) {
      return 1;
    }
  }
  else {
    return 1;
  }

  int optVal = 1;
  SetSockOpt(SOL_SOCKET, SO_REUSEADDR,
	     (char *)&optVal, sizeof(optVal));

  unsigned char multicast_TTL = (char)servercfg->TTL;
  int rv = SetSockOpt(IPPROTO_IP, IP_MULTICAST_TTL, 
		      (void *)&multicast_TTL, sizeof(multicast_TTL));
  if(rv < 0) {
    CheckSocketError((gxSocket *)this, "Cannot set TTL"); 
    Close();
    return 1;
  }

  return 0;
}

void *ServerThread::ThreadEntryRoutine(gxThread_t *thread)
{
  char *datagram = new char[servercfg->datagram_size];
  
  // Fill the datagram
  memset(datagram, DEFAULT_DATAGRAM_FILL, servercfg->datagram_size);

  while(servercfg->accept_clients) { // Loop accepting client connections
    // Flood the network with multicast datagrams 
    if(SendTo(datagram, servercfg->datagram_size) < 0) {
      CheckSocketError((gxSocket *)this, "A fatal send error occurred.\
Shutting down the server");
      delete datagram;
      return (void *)0;
    }
  }

  delete datagram;
  return (void *)0;
}

void ServerThread::ThreadExitRoutine(gxThread_t *thread)
// Thread exit function used to close the server thread.
{
  Close(); // Close the server side socket
}

void ServerThread::ThreadCleanupHandler(gxThread_t *thread)
// Thread cleanup handler used in the event that the thread is
// canceled.
{
  Close(); // Close the server side socket
}

void IntSHN(FAU_t bytes, gxString &byte_string)
{
  double shn; // Shorthand notation number
  gxString shn_c; // Shorthand notation nomenclature
  if(bytes > 1000) { // More than 1K bytes
    if(bytes < (1000*1000)) { // Less than 1M
      shn = (double)bytes/double(1000);
      shn_c = "K";
    }
    else if(bytes < ((1000*1000)*1000)) { // Less than 1G 
      shn = (double)bytes/double(1000*1000);
      shn_c = "M";
    }
    else { // 1G or greater
      shn = (double)bytes/double((1000*1000)*1000);
      shn_c = "G";
    }
  }
  else {
    shn_c = "bytes";
    byte_string << clear << bytes << " " << shn_c;
    return;
  }
  gxString lbuf;
  lbuf.Precision(2);
  lbuf << shn;
  while(lbuf[lbuf.length()-1] == '0') {
    lbuf.DeleteAt((lbuf.length()-1), 1);
  }
  if(lbuf[lbuf.length()-1] == '.') lbuf += "0";
  byte_string << clear << lbuf << " " << shn_c;
}

void SleepFor(int seconds)
{
#if defined (__WIN32__)
  int i = seconds * 1000; // Convert milliseconds to seconds
  Sleep((DWORD)i);
#elif defined (__UNIX__)
  sleep(seconds);
#else // No native sleep functions are defined
#error You must define a native API: __WIN32__ or __UNIX__
#endif
}

const char *GetDefaultGroup() {
  return (const char *)"225.0.0.39";
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
