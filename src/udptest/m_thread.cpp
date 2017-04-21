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
  timeout = DEFAULT_TIMEOUT;
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
  GXSTD::cout.flush(); // Flush the ostream datagram to the stdio

  if(servercfg->logfile) {
    if(s1) *(servercfg->logfile) << s1;
    if(s2) *(servercfg->logfile) << s2;
    if(s3) *(servercfg->logfile) << s3;
    *(servercfg->logfile) << "\n";
    servercfg->logfile->df_Flush(); // Flush the disk datagrams
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
    if(InitSocket(SOCK_DGRAM, servercfg->port) < 0) return 1;
  }
  else {
    return 1;
  }

  // Bind the name to the socket
  if(Bind() < 0) {
    CheckSocketError((gxSocket *)this, "Error initializing server"); 
    Close();
    return 1;
  }
    
  return 0;
}

void *ServerThread::ThreadEntryRoutine(gxThread_t *thread)
{
  ComHeader ch;
  int bytes_read = 0;
  int hdr_size = sizeof(ComHeader);
  int err_count = 0;
  while(servercfg->accept_clients) { // Loop accepting client connections

    if(err_count >= 100) {
      PrintMessage("\nMaximum error count exceeded\n\
Shutting down the server\n");
      servercfg->kill_server = 1;
      servercfg->process_loop = 0; // Signal to stop the process thread
      // Wake up the thread and exit
      servercfg->process_cond.ConditionSignal(); 
      servercfg->process_lock.MutexUnlock();
      break;
    }

    // Block the server until a client requests service
    ch.Clear(); 
    memset(&remote_sin, 0, sizeof(remote_sin));
    bytes_read = RemoteRecvFrom(&ch, hdr_size);
    
    // NOTE: Getting client info for statistical purposes only
    char client_name[gxsMAX_NAME_LEN]; int r_port = -1;
    GetClientInfo(client_name, r_port);
    PrintMessage("Connecting to client: ", client_name,
		 "\nValidating client connection\n");
    // Record the client name
    hostname = client_name;

    if(bytes_read < 0) {
      ReportError("Error receiving client header");
      err_count++;
      continue;
    }
    if(bytes_read != hdr_size) {
      ReportError("Received invalid client header");
      err_count++;
      continue;
    }
    if(ch.checkword != net_checkword) {
      ReportError("Received invalid client header");
      err_count++;
      continue;
    }

    HandleClientRequest(ch);
    if(servercfg->restart_server) break;
    if(servercfg->kill_server) break;
  }    

  return (void *)0;
}

void ServerThread::ThreadExitRoutine(gxThread_t *thread)
// Thread exit function used to close the server thread.
{
  Close(); // Close the socket
}

void ServerThread::ThreadCleanupHandler(gxThread_t *thread)
// Thread cleanup handler used in the event that the thread is
// canceled.
{
  Close(); // Close the socket
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

int ServerThread::RecvDatagram(void *datagram, int bytes, int flags)
{
  if(bytes == 0) {
    PrintMessage("No bytes are being received");
    return -1;
  }

  memset(&remote_sin, 0, sizeof(remote_sin));
  int bytes_read = RemoteRecvFrom(datagram, bytes, servercfg->timeout, 0, 
				  flags);
  if(bytes_read < 0) {
    if(GetSocketError() != gxSOCKET_RECEIVE_ERROR) {
      CheckSocketError((gxSocket *)this, "Error reading client socket");
    }
    else {
      PrintMessage("Client has stopped sending during a socket read");
    }
    return -1;
  }
  if(bytes_read != bytes) {
    PrintMessage("Did not receive the expected number of bytes");
    return -1;
  }
  if(bytes_read == 0) {
    PrintMessage("Did not receive any bytes");
    return -1;
  }

  return bytes_read;
}

int ServerThread::SendDatagram(void *datagram, int bytes)
{
  int bytes_moved = RemoteSendTo(datagram, bytes);
  if(bytes_moved < 0) {
    if(GetSocketError() != gxSOCKET_TRANSMIT_ERROR) {
      CheckSocketError((gxSocket *)this, "Error writing to client socket");
    }
    else {
      PrintMessage("Client has stopped receiving during a socket write");
    }
    return -1;
  }
  return bytes_moved;
}

void ServerThread::HandleEchoRequest(ComHeader &ch)
{
  gxString sbuf, lbuf;
  SysTime systime;
  gxString byte_string;
  FAU_t byte_count = (FAU_t)0;
  double rx_time = (double)0;
  double tx_time = (double)0;
  double elapsed_time = (double)0;
  double avg_rx_time = (double)0;
  double avg_tx_time = (double)0;
  FAU_t datagram_count = (FAU_t)0;
  FAU_t curr_count = (FAU_t)0;
  FAU_t curr_percent = (FAU_t)0;
  FAU_t byps = (FAU_t)0;
  FAU_t bps = (FAU_t)0;
  int err = 0;
  const int report_percent = 10;
  time_t begin, begin_rx, end_rx, end;
  time_t begin_tx, end_tx;
  double r_percent, c_percent;
  int t_percent;
  FAU_t end_of_file = ch.bytes;

  // Construct a datagram for the incoming data
  FAU_t datagram_size = ch.datagram_size;
  if(datagram_size > (FAU_t)65507) {
    datagram_size = (FAU_t)65507;
  }
  char *datagram = new char[datagram_size];
  if(!datagram) {
    sbuf << clear << "Could not allocate memory for the receive datagram"
	 << "\n";
    PrintMessage(sbuf.c_str());
    return;
  }

  IntSHN(ch.bytes, byte_string);
  sbuf << clear << "Echoing " << byte_string 
       << " bytes of data to/from " << hostname
       << "\n" << "RX/TX datagram size = " 
       << ch.datagram_size << "\n" << "Starting download @" 
       << systime.GetSystemDateTime();
  PrintMessage(sbuf.c_str());

  // Get the current time before entering loop
  time(&begin);

  int bytes_read = 0; bytes_moved = 0;
  if(end_of_file <= ch.datagram_size) { 
    // The file is smaller than one datagram
    time(&begin_rx);
    bytes_read = RecvDatagram(datagram, end_of_file);
    time(&end_rx);
    time(&begin_tx);
    bytes_moved = SendDatagram(datagram, end_of_file);
    time(&end_tx);
    byte_count = end_of_file;  
    rx_time += difftime(end_rx, begin_rx);
    tx_time += difftime(end_tx, begin_tx);
    datagram_count++;
    curr_count++;
    if((bytes_moved < 0) || (bytes_read < 0)) err = 1;
  }
  else { // The file is larger than one datagram
    while((end_of_file > ch.datagram_size) && (servercfg->echo_loop)) {
      time(&begin_rx);
      bytes_read = RecvDatagram(datagram, ch.datagram_size);
      time(&end_rx);
      time(&begin_tx);
      bytes_moved = SendDatagram(datagram, ch.datagram_size);
      time(&end_tx);
      rx_time += difftime(end_rx, begin_rx);
      tx_time += difftime(end_tx, begin_tx);
      byte_count += bytes_read;
      end_of_file -= ch.datagram_size;
      datagram_count++;
      curr_count++;
      if((bytes_moved < 0) || (bytes_read < 0)) {
	err = 1;
	break;
      }
      if(end_of_file <= ch.datagram_size) {
	time(&begin_rx);
	bytes_read = RecvDatagram(datagram, end_of_file);
	time(&end_rx);
	time(&begin_tx);
	bytes_moved = SendDatagram(datagram, end_of_file);
	time(&end_tx);
	byte_count += end_of_file;
	rx_time += difftime(end_rx, begin_rx);
	tx_time += difftime(end_tx, begin_tx);
	datagram_count++;
	curr_count++;
	if((bytes_moved < 0) || (bytes_read < 0)) err = 1;
	break;
      }
      else {
	if(curr_count > 1) {
	  if((curr_count * ch.datagram_size) >= (ch.bytes/report_percent)) {
	    // Report the byte count every 10 percent
	    r_percent = (curr_percent++ * report_percent) / 100;
	    c_percent = ch.bytes * r_percent;
	    if((FAU_t)c_percent >= (ch.bytes - byte_count)) {
	      curr_count = 0;
	      IntSHN(byte_count, byte_string);
	      t_percent = curr_percent * report_percent;
	      t_percent -= 100;
	      if(t_percent >= 100) {
		sbuf << clear << "--% Complete - echoed "; 
	      }
	      else {
		sbuf << clear << t_percent << "% Complete - echoed "; 
	      }
	      lbuf.Clear();
	      lbuf.Precision(2); 
	      lbuf << double(rx_time+tx_time);
	      sbuf << byte_string << " bytes in " << lbuf << " secs";
	      PrintMessage(sbuf.c_str());
	    }
	  }
	}
	continue;
      }
    }
  }

  // Get the current time after loop is completed 
  time(&end);

  // Calculate the elapsed time in seconds. 
  elapsed_time = difftime(end, begin);
  if(err) {
    sbuf << clear << "Echo request partially completed!" << "\n";
    sbuf << "Echo failed @" << systime.GetSystemDateTime();
  }
  else {
    sbuf << clear << "Echo request completed @" << systime.GetSystemDateTime();
  }
  PrintMessage(sbuf.c_str());

  sbuf.Precision(2); 
  IntSHN(byte_count, byte_string);
  sbuf << clear << "Echoed " << byte_string << " bytes in " 
       << elapsed_time << " seconds" << "\n";
  sbuf << "Number of datagrams echoed = " << datagram_count << "\n";
  sbuf << "Average datagram size = " << byte_count/datagram_count 
       << " bytes" << "\n";
  avg_rx_time = (rx_time/(double)datagram_count) * 1000;
  avg_tx_time = (tx_time/(double)datagram_count) * 1000;
  sbuf.Precision(4); 
  sbuf << "Average buffered datagram echo time = " << avg_tx_time 
       << " milliseconds" << "\n";
  sbuf << "Average datagram echo time = " << avg_rx_time 
       << " milliseconds";
  PrintMessage(sbuf.c_str());
  
  // Calculate the number of bits per second  
  if(elapsed_time != (double)0) {
    byps = FAU_t(byte_count/(elapsed_time/2));
    bps = byps * 8;
    IntSHN(byps, byte_string);
    sbuf << clear << "UDP bytes per second rate = " << byte_string 
	 << "BytesPS";
    IntSHN(bps, byte_string);
    sbuf << "\n" << "UDP LAN/WAN Benchmark speed = " << byte_string 
	 << "BPS";
    PrintMessage(sbuf.c_str());
  }
  else {
    sbuf << clear << "UDP bytes per second rate = 100%";
    sbuf << "\n" << "UDP LAN/WAN Benchmark speed = 100%";
    PrintMessage(sbuf.c_str());
  }

  delete datagram;
  PrintMessage("\nClosing UDP echo session...\n");
}

void ServerThread::HandleClientRequest(ComHeader &ch)
// Function used to handle a client request.
{
  gxString sbuf, lbuf;
  SysTime systime;
  char command = char(ch.command & 0xFF);

  switch(command) {
    case net_kill:
      sbuf << clear << "Received kill request from " << hostname << "\n";
      sbuf << "Stopping the server @" 
	   << systime.GetSystemDateTime();
      PrintMessage(sbuf.c_str());
      // Wake up the process thread waiting on this condition
      servercfg->accept_clients = 0; // Do not accept any more clients
      servercfg->kill_server = 1;
      servercfg->process_loop = 0; // Signal to stop the process thread
      // Wake up the thread and exit
      servercfg->process_cond.ConditionSignal(); 
      servercfg->process_lock.MutexUnlock();
      return;
      
    case net_download:
      HandleEchoRequest(ch);
      break;

    case net_upload:
      HandleEchoRequest(ch);
      break;

    case net_restart:
      sbuf << clear << "Received restart request from " << hostname 
	   << "\n"; 
      sbuf << "Restarting server @" 
	   << systime.GetSystemDateTime();
      PrintMessage(sbuf.c_str());
      // Wake up the process thread waiting on this condition
      servercfg->accept_clients = 0; // Do not accept any more clients
      servercfg->restart_server = 1;
      servercfg->process_loop = 0; // Signal to stop the process thread
      // Wake up the thread and exit
      servercfg->process_cond.ConditionSignal(); 
      servercfg->process_lock.MutexUnlock();
      break;

    default:
      sbuf << clear << "Received an invalid client request from "
	   << hostname;
      PrintMessage(sbuf.c_str());
      break;
  }
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
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
