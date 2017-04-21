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

TCP benchmark server classes.
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
  client_request_pool = new thrPool;
}

ServerConfig::~ServerConfig()
{
  if(client_request_pool) delete client_request_pool;
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
  display_processor_clock = 0;
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

int ServerThread::InitServer(int max_connections)
// Initialize the server. Returns a non-zero value if any
// errors ocurr.
{
  if(InitSocketLibrary() == 0) {
    if(InitSocket(SOCK_STREAM, servercfg->port) < 0) return 1;
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
    
  // Listen for connections with a specifed backlog
  if(Listen(max_connections) < 0) {
    CheckSocketError((gxSocket *)this, "Error initializing server");
    Close();
    return 1;
  }

  return 0;
}

void *ServerThread::ThreadEntryRoutine(gxThread_t *thread)
{
  while(servercfg->accept_clients) { // Loop accepting client connections

    // Block the server until a client requests service
    if(Accept() < 0) continue;

    // NOTE: Getting client info for statistical purposes only
    char client_name[gxsMAX_NAME_LEN]; int r_port = -1;
    GetClientInfo(client_name, r_port);
    PrintMessage("Connecting to client: ", client_name,
		 "\nValidating client connection\n");

    ClientSocket_t *s = new ClientSocket_t;
    if(!s) {
      ReportError("A fatal memory allocation error occurred\n \
Shutting down the server");
      CloseSocket();
      return (void *)0;
    }
    
    // Record the file descriptor assigned by the Operating System
    s->client_socket = GetRemoteSocket();
    s->hostname = client_name;

    // Destroy all the client threads that have exited
    RebuildThreadPool(servercfg->client_request_pool);

    // Create thread per cleint
    gxThread_t *rthread = \
      request_thread.CreateThread(servercfg->client_request_pool, (void *)s);
    if(CheckThreadError(rthread, "Error starting client request thread")) {
      delete s;
      return (void *)0;
    }
  }

  return (void *)0;
}

void ServerThread::ThreadExitRoutine(gxThread_t *thread)
// Thread exit function used to close the server thread.
{
  CloseSocket(); // Close the server side socket
}

void ServerThread::ThreadCleanupHandler(gxThread_t *thread)
// Thread cleanup handler used in the event that the thread is
// canceled.
{
  CloseSocket(); // Close the server side socket
}

void *ClientRequestThread::ThreadEntryRoutine(gxThread_t *thread)
{
  // Extract the client socket from the thread parameter
  ClientSocket_t *s = (ClientSocket_t *)thread->GetThreadParm();
  
  // Process the client request
  HandleClientRequest(s);

  return (void *)0;
}

void ClientRequestThread::ThreadExitRoutine(gxThread_t *thread)
// Thread exit function used to close the client thread.
{
  // Extract the client socket from the thread parameter
  ClientSocket_t *s = (ClientSocket_t *)thread->GetThreadParm();

  // Close the client socket and free its resources
  Close(s->client_socket);
  delete s;
}

void ClientRequestThread::ThreadCleanupHandler(gxThread_t *thread)
// Thread cleanup handler used in the event that the thread is
// canceled.
{
  // Extract the client socket from the thread parameter
  ClientSocket_t *s = (ClientSocket_t *)thread->GetThreadParm();

  // Close the client socket and free its resources
  Close(s->client_socket);
  delete s;
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

int ClientRequestThread::SendBuffer(ClientSocket_t *s, void *buffer, 
				    int bytes)
{
  int bytes_moved = Send(s->client_socket, 
			 (const unsigned char *)buffer, bytes);
  if(bytes_moved < 0) {
    if(GetSocketError() != gxSOCKET_TRANSMIT_ERROR) {
      CheckSocketError((gxSocket *)this, "Error writing to client socket");
    }
    else {
      PrintMessage("Client has disconnected during a socket write");
    }
    return -1;
  }
  return bytes_moved;
}

int ClientRequestThread::RecvHeader(ClientSocket_t *s, ComHeader &ch)
{
  // Read the client header
  int hdr_size = sizeof(ComHeader);
  ch.Clear(); // Reset the client header with each header read
  int bytes_read = Recv(s->client_socket, &ch, hdr_size);
  if(bytes_read < 0) {
    ReportError("Error receiving client header.\n \
Disconnecting the client...");
    return -1;
  }
  if(bytes_read != hdr_size) {
    ReportError("Received invalid client header.\n \
Disconnecting the client...");
    return -1;
  }
  if(ch.checkword != net_checkword) {
    ReportError("Received invalid client header.\n \
Disconnecting the client...");
    return -1;
  }

  return bytes_read;
}

void ClientRequestThread::HandleDownloadRequest(ClientSocket_t *s, 
					      ComHeader &ch)
{
  gxString sbuf, lbuf;
  SysTime systime;
  gxString byte_string;
  FAU_t byte_count = (FAU_t)0;
  double rx_time = (double)0;
  double elapsed_time = (double)0;
  double avg_insert_time = (double)0;
  FAU_t buffer_count = (FAU_t)0;
  FAU_t curr_count = (FAU_t)0;
  FAU_t curr_percent = (FAU_t)0;
  FAU_t byps = (FAU_t)0;
  FAU_t bps = (FAU_t)0;
  int err = 0;
  FAU_t end_of_file = ch.bytes;
  const int report_percent = 10;
  time_t begin, begin_rx, end_rx, end;
  int bytes_moved;
  char buffer_fill = DEFAULT_BUF_FILL;
  double r_percent, c_percent;
  int t_percent;
  double ps_elapsed_time;
  clock_t ps_begin, ps_end;

  // Construct a buffer for the incoming data
  FAU_t buffer_size = ch.buffer_size;
  if(buffer_size > (FAU_t)65507) {
    buffer_size = (FAU_t)65507;
  }
  char *buffer = new char[buffer_size];
  if(!buffer) {
    sbuf << clear << "Could not allocate memory for the receive buffer"
	 << "\n";
    PrintMessage(sbuf.c_str());
    return;
  }

  // Fill the buffer with some data
  memset(buffer, buffer_fill, buffer_size);

  IntSHN(ch.bytes, byte_string);
  sbuf << clear << "Downloading " << byte_string 
       << " bytes of data to " << s->hostname << "\n" 
       << "Transmit buffer size = " 
       << buffer_size << "\n" << "Starting download @" 
       << systime.GetSystemDateTime();
  PrintMessage(sbuf.c_str());

  // Get the current time before entering loop
  time(&begin);

  // Get CPU clock cycles before entering loop
  ps_begin = clock();

  bytes_moved = 0;
  if(end_of_file <= buffer_size) { // The file is smaller than one buffer
    time(&begin_rx);
    bytes_moved = SendBuffer(s, buffer, end_of_file);
    time(&end_rx);
    byte_count = end_of_file;  
    rx_time += difftime(end_rx, begin_rx);
    buffer_count++;
    curr_count++;
    if(bytes_moved < 0) err = 1; // Download failed
  }
  else { // The file is larger than one buffer
    while((end_of_file > buffer_size) && (servercfg->echo_loop)) {
      time(&begin_rx);
      bytes_moved = SendBuffer(s, buffer, buffer_size);
      time(&end_rx);
      rx_time += difftime(end_rx, begin_rx);
      byte_count += bytes_moved;
      end_of_file -= buffer_size;
      buffer_count++;
      curr_count++;
      if(bytes_moved < 0) {
	err = 1; // Download failed
	break;
      }
      if(end_of_file <= buffer_size) {
	time(&begin_rx);
	bytes_moved = SendBuffer(s, buffer, end_of_file);
	time(&end_rx);
	byte_count += end_of_file;
	rx_time += difftime(end_rx, begin_rx);
	buffer_count++;
	curr_count++;
	if(bytes_moved < 0) err = 1; // Download failed
	break;
      }
      else {
	if(curr_count > 1) {
	  if((curr_count * buffer_size) >= (ch.bytes/report_percent)) {
	    // Report the byte count every 10 percent
	    r_percent = (curr_percent++ * report_percent) / 100;
	    c_percent = ch.bytes * r_percent;
	    if((FAU_t)c_percent >= (ch.bytes - byte_count)) {
	      curr_count = 0;
	      IntSHN(byte_count, byte_string);
	      t_percent = curr_percent * report_percent;
	      t_percent -= 100;
	      if(t_percent >= 100) {
		sbuf << clear << "--% Complete - downloaded "; 
	      }
	      else {
		sbuf << clear << t_percent << "% Complete - downloaded "; 
	      }
	      lbuf.Clear();
	      lbuf.Precision(2); 
	      lbuf << rx_time;
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

  // Get CPU clock cycles after loop is completed 
  ps_end =clock();

  // Calculate the elapsed time in seconds. 
  elapsed_time = difftime(end, begin);
  ps_elapsed_time = (double)(ps_end - ps_begin) / CLOCKS_PER_SEC;

  if(err) {
    sbuf << clear << "Download partially completed!" << "\n";
    sbuf << "Download failed @" << systime.GetSystemDateTime();
  }
  else {
    sbuf << clear << "Download completed @" << systime.GetSystemDateTime();
  }
  PrintMessage(sbuf.c_str());

  sbuf.Precision(2); 
  IntSHN(byte_count, byte_string);
  sbuf << clear << "Downloaded " << byte_string << " bytes in " 
       << elapsed_time << " seconds" << "\n";
  sbuf << "Number of buffers downloaded = " << buffer_count << "\n";
  sbuf << "Average buffer size = " << byte_count/buffer_count 
       << " bytes" << "\n";
  avg_insert_time = (rx_time/(double)buffer_count) * 1000;
  sbuf.Precision(4); 
  sbuf << "Average buffer download time = " << avg_insert_time 
       << " milliseconds";
  PrintMessage(sbuf.c_str());
  
  // Calculate the number of bits per second
  if(elapsed_time != (double)0) {
    byps = FAU_t(byte_count/elapsed_time);
    bps = byps * 8;
    IntSHN(byps, byte_string);
    sbuf << clear << "Download bytes per second rate = " << byte_string 
	 << "BytesPS";
    IntSHN(bps, byte_string);
    sbuf << "\n" << "LAN/WAN Benchmark speed = " << byte_string 
	 << "BPS";
    PrintMessage(sbuf.c_str());
  }
  else {
    sbuf << clear << "Upload bytes per second rate = 100%";
    sbuf << "\n" << "LAN/WAN Benchmark speed = 100%";
    PrintMessage(sbuf.c_str());
  }

  if(servercfg->display_processor_clock) {
    if(ps_elapsed_time != 0) {
      byps = FAU_t(byte_count/ps_elapsed_time);
      bps = byps * 8;
      IntSHN(byps, byte_string);
      IntSHN(byps, byte_string);
      sbuf << clear << "Process rate bytes per second rate = " << byte_string 
	   << "BytesPS";
      IntSHN(bps, byte_string);
      sbuf << "\n" << "Process rate LAN/WAN Benchmark speed = " << byte_string 
	   << "BPS";
      PrintMessage(sbuf.c_str());
    }
    else {
      sbuf << clear << "Process rate bytes per second rate = 100%";
      sbuf << "\n" << "Process rate LAN/WAN Benchmark speed = 100%";
      PrintMessage(sbuf.c_str());
    }
  }

  delete buffer;
}

void ClientRequestThread::HandleUploadRequest(ClientSocket_t *s, 
					      ComHeader &ch)
{
  gxString sbuf, lbuf;
  SysTime systime;
  gxString byte_string;
  FAU_t byte_count = (FAU_t)0;
  double rx_time = (double)0;
  double elapsed_time = (double)0;
  double avg_insert_time = (double)0;
  FAU_t buffer_count = (FAU_t)0;
  FAU_t curr_count = (FAU_t)0;
  FAU_t curr_percent = (FAU_t)0;
  FAU_t byps = (FAU_t)0;
  FAU_t bps = (FAU_t)0;
  int err = 0;
  const int report_percent = 10;
  time_t begin, begin_rx, end_rx, end;
  double r_percent, c_percent;
  int t_percent;
  double ps_elapsed_time;
  clock_t ps_begin, ps_end;

  // Construct a buffer for the incoming data
  FAU_t buffer_size = ch.buffer_size;
  if(buffer_size > (FAU_t)65507) {
    buffer_size = (FAU_t)65507;
  }
  char *buffer = new char[buffer_size];
  if(!buffer) {
    sbuf << clear << "Could not allocate memory for the receive buffer"
	 << "\n";
    PrintMessage(sbuf.c_str());
    return;
  }

  IntSHN(ch.bytes, byte_string);
  sbuf << clear << s->hostname << " is uploading " << byte_string 
       << " bytes of data." << "\n" << "Receive buffer size = " 
       << buffer_size << "\n" << "Starting upload @" 
       << systime.GetSystemDateTime();
  PrintMessage(sbuf.c_str());

  // Get the current time before entering loop
  time(&begin);

  // Get CPU clock cycles before entering loop
  ps_begin = clock();

  bytes_read = 0;
  while((bytes_read != ch.bytes) && (servercfg->echo_loop)) {
    time(&begin_rx);
    bytes_read = RawRead(s->client_socket, buffer, buffer_size);
    time(&end_rx);
    rx_time += difftime(end_rx, begin_rx);
    buffer_count++;
    curr_count++;
    if(bytes_read == 0) break; // Data transfer complete
    if(bytes_read < 0) {
      if(GetSocketError() != gxSOCKET_RECEIVE_ERROR) {
	CheckSocketError((gxSocket *)this, "Error reading client socket");
      }
      else {
	PrintMessage("Client has disconnected during a socket read");
      }
      err = 1;
      break; // Break instead of returning to report the the stats
    }
    byte_count += bytes_read;

    if(curr_count > 1) {
      if((curr_count * buffer_size) >= (ch.bytes/report_percent)) {
	// Report the byte count every 10 percent
	r_percent = (curr_percent++ * report_percent) / 100;
	c_percent = ch.bytes * r_percent;
	if((FAU_t)c_percent >= (ch.bytes - byte_count)) {
	  curr_count = 0;
	  IntSHN(byte_count, byte_string);
	  t_percent = curr_percent * report_percent;
	  t_percent -= 100;
	  if(t_percent >= 100) {
	    sbuf << clear << "--% Complete - uploaded "; 
	  }
	  else {
	    sbuf << clear << t_percent << "% Complete - uploaded "; 
	  }
	  lbuf.Clear();
	  lbuf.Precision(2); 
	  lbuf << rx_time;
	  sbuf << byte_string << " bytes in " << lbuf << " secs";
	  PrintMessage(sbuf.c_str());
	}
      }
    }
  }

  // Get the current time after loop is completed 
  time(&end);

  // Get CPU clock cycles after loop is completed 
  ps_end =clock();

  // Calculate the elapsed time in seconds. 
  elapsed_time = difftime(end, begin);
  ps_elapsed_time = (double)(ps_end - ps_begin) / CLOCKS_PER_SEC;

  if(err) {
    sbuf << clear << "Upload partially completed!" << "\n";
    sbuf << "Upload failed @" << systime.GetSystemDateTime();
  }
  else {
    sbuf << clear << "Upload completed @" << systime.GetSystemDateTime();
  }
  PrintMessage(sbuf.c_str());

  sbuf.Precision(2); 
  IntSHN(byte_count, byte_string);
  sbuf << clear << "Uploaded " << byte_string << " bytes in " 
       << elapsed_time << " seconds" << "\n";
  sbuf << "Number of buffers uploaded = " << buffer_count << "\n";
  sbuf << "Average buffer size = " << byte_count/buffer_count 
       << " bytes" << "\n";
  avg_insert_time = (rx_time/(double)buffer_count) * 1000;
  sbuf.Precision(4); 
  sbuf << "Average buffer upload time = " << avg_insert_time 
       << " milliseconds";
  PrintMessage(sbuf.c_str());
  
  // Calculate the number of bits per second
  if(elapsed_time != (double)0) {
    byps = FAU_t(byte_count/elapsed_time);
    bps = byps * 8;
    IntSHN(byps, byte_string);
    sbuf << clear << "Upload bytes per second rate = " << byte_string 
	 << "BytesPS";
    IntSHN(bps, byte_string);
    sbuf << "\n" << "LAN/WAN Benchmark speed = " << byte_string 
	 << "BPS";
    PrintMessage(sbuf.c_str());
  }
  else {
    sbuf << clear << "Upload bytes per second rate = 100%";
    sbuf << "\n" << "LAN/WAN Benchmark speed = 100%";
    PrintMessage(sbuf.c_str());
  }

  if(servercfg->display_processor_clock) {
    if(ps_elapsed_time != 0) {
      byps = FAU_t(byte_count/ps_elapsed_time);
      bps = byps * 8;
      IntSHN(byps, byte_string);
      IntSHN(byps, byte_string);
      sbuf << clear << "Process rate bytes per second rate = " << byte_string 
	   << "BytesPS";
      IntSHN(bps, byte_string);
      sbuf << "\n" << "Process rate LAN/WAN Benchmark speed = " << byte_string 
	   << "BPS";
      PrintMessage(sbuf.c_str());
    }
    else {
      sbuf << clear << "Process rate bytes per second rate = 100%";
      sbuf << "\n" << "Process rate LAN/WAN Benchmark speed = 100%";
      PrintMessage(sbuf.c_str());
    }
  }

  delete buffer;
}

void ClientRequestThread::HandleClientRequest(ClientSocket_t *s)
// Function used to handle a client request.
{
  ComHeader ch;
  gxString sbuf, lbuf;
  SysTime systime;

  // Read the client header
  if(!RecvHeader(s, ch)) return;
  char command = char(ch.command & 0xFF);

  switch(command) {
    case net_kill:
      sbuf << clear << "Received kill request from " << s->hostname << "\n";
      // Report the number of running thread to monitor any data loss
      sbuf << "Number of client threads running: " 
	   << NumClientThreadsRunning((gxThread *)this);
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
      HandleDownloadRequest(s, ch);
      break;

    case net_upload:
      HandleUploadRequest(s, ch);
      break;

    case net_restart:
      sbuf << clear << "Received restart request from " << s->hostname 
	   << "\n"; 
      // Report the number of running thread to monitor any data loss
      sbuf << "Number of client threads running: " 
	   << NumClientThreadsRunning((gxThread *)this);
      PrintMessage(sbuf.c_str());
      while(NumClientThreadsRunning((gxThread *)this) > 1) {
	// Wait until all threads have stopped before restarting
	if(!servercfg->echo_loop) return;
	sSleep(SLEEP_SECS_BEFORE_RETRY); // Allow for I/O recovery time
      }
      sbuf << clear << "Restarting server @" 
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
	   << s->hostname << "\n" << "Disconnecting client";
      PrintMessage(sbuf.c_str());
      break;
  }

  // End of client request
  PrintMessage("\nDisconnecting client...");
}

int NumClientThreadsRunning(gxThread *requesting_thread)
{
  int num_running = 0;
  thrPool *thread_pool = servercfg->client_request_pool;

  if(thread_pool) {
    // Destroy all the engine threads that have exited
    requesting_thread->RebuildThreadPool(thread_pool);

    if(!thread_pool->IsEmpty()) {
      thrPoolNode *ptr;
      gxThread_t *thread;
      gxThreadType type;
      gxThreadState state;

      // Start checking at the tail since the higher priority threads
      // at the head of the pool would most likely have exited first.
      ptr = thread_pool->GetTail();

      while(ptr) {
	thread = ptr->GetThreadPtr();
	state = ptr->GetThreadPtr()->GetThreadState();
	type = ptr->GetThreadPtr()->GetThreadType();
	if(state == gxTHREAD_STATE_RUNNING) {
	  num_running++;
	}
	ptr = ptr->GetPrev();
      }
    }
  }
  return num_running;
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
