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

TCP benchmark client. 
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
#include <stdio.h>
#include "m_thread.h"

// --------------------------------------------------------------
// Constants
// --------------------------------------------------------------
const char *version_str = "2.01";
const char *program_name = "tcptest";
const char *author_email = "Douglas.Gaer@noaa.gov";
// --------------------------------------------------------------

// --------------------------------------------------------------
// Globals
// --------------------------------------------------------------
char *server_ip;
gxsPort_t port = DEFAULT_PORT;
int echo_loop = 1;
int num_clients = 1;
int buffer_size = DEFAULT_BUF_SIZE;
char buffer_fill = DEFAULT_BUF_FILL;
int num_to_send = DEFAULT_NUM_TO_SEND;
gxString logfile_name;
int enable_logging = 0;
int restarting = 0;
int stopping = 0;
gxString output_file;
int write_to_output_file = 0;
int display_processor_clock = 0;
// --------------------------------------------------------------

int CleanBeforeExit(gxSocket *client, int error_code) 
{
  if(client) {
    if(client->GetSocketError() != gxSOCKET_NO_ERROR) {
      cout << client->SocketExceptionMessage() << "\n" << flush;  
    }
    client->Close(); // Close the socket connection
    client->ReleaseSocketLibrary();
  }
  return error_code;
}

int RestartServer(int restart)
{
  gxString sbuf;
  ComHeader ch;
  int hdr_size = sizeof(ComHeader);
  int bytes_moved = 0;

  if(restart) {
    ch.command = (FAU_t)net_restart;
    sbuf << clear << "\n" << "Restarting the server";
  }
  else {
    ch.command = (FAU_t)net_kill;
    sbuf << clear << "\n" << "Shutting down the server";
  }
  sbuf << "\n" << "Connecting to: " << server_ip << " on port: " 
       << port << "\n";
  PrintMessage(sbuf.c_str());
  gxSocket client(SOCK_STREAM, port, server_ip);
  if(!client) {
    CheckSocketError(&client);
   return 0;
  }

  // Connect to the server
  if(client.Connect() < 0) {
    CheckSocketError(&client, "Could not connect to the server");
    client.Close();
   return 0;
  }

  // Send the client header
  bytes_moved = client.Send(&ch, hdr_size);
  if(bytes_moved != hdr_size) {
    sbuf << clear << "Cannot send message header to the server" << "\n";
    PrintMessage(sbuf.c_str());
    CleanBeforeExit(&client, 0);
   return 0;
  }


  if(restart) {
    sbuf << clear << "\n" << "Restarting command sent to the server" << "\n";
  }
  else {
    sbuf << clear << "\n" << "Kill command sent to the server" << "\n";
  }
  PrintMessage(sbuf.c_str());

  return 0;
}

int SendBuffer(gxSocket *client, void *buffer, int bytes)
{
  int bytes_moved = client->Send((const unsigned char *)buffer, bytes);
  if(bytes_moved < 0) {
    if(client->GetSocketError() != gxSOCKET_TRANSMIT_ERROR) {
      CheckSocketError(client, "Error writing to server socket");
    }
    else {
      PrintMessage("Server has disconnected during a socket write");
    }
    CleanBeforeExit(client, 0);
    return -1;
  }
  return bytes_moved;
}

int SendUploadRequest(int num) 
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
  ComHeader ch;
  int hdr_size = sizeof(ComHeader);
  FAU_t end_of_file = num_to_send;
  int bytes_moved;
  double r_percent, c_percent;
  int t_percent;
  double ps_elapsed_time;
  clock_t ps_begin, ps_end;

  sbuf << clear << "\n" << "Constructing TCP upload client #" << num
       << "\n" << "Connecting to: " << server_ip << " on port: " 
       << port << "\n";
  PrintMessage(sbuf.c_str());
  gxSocket client(SOCK_STREAM, port, server_ip);
  if(!client) {
    CheckSocketError(&client);
   return 0;
  }

  // Connect to the server
  if(client.Connect() < 0) {
    CheckSocketError(&client, "Could not connect to the server");
    client.Close();
   return 0;
  }

  char *buffer = new char[buffer_size];
  if(!buffer) {
    sbuf << clear << "Could not allocate memory for the transmit buffer"
	 << "\n";
    PrintMessage(sbuf.c_str());
    client.Close();
   return 0;
  }

  // Fill the buffer with some data
  memset(buffer, buffer_fill, buffer_size);

  ch.command = (FAU_t)net_upload;
  ch.bytes = num_to_send;
  ch.buffer_size = buffer_size;

  // Send the client header
  bytes_moved = client.Send(&ch, hdr_size);
  if(bytes_moved != hdr_size) {
    sbuf << clear << "Cannot send message header to the server" << "\n";
    PrintMessage(sbuf.c_str());
    CleanBeforeExit(&client, 0);
    delete buffer;
   return 0;
  }

  IntSHN(ch.bytes, byte_string);
  sbuf << clear << "Uploading " << byte_string 
       << " bytes of data to " << server_ip << "\n" 
       << "Transmit buffer size = " 
       << ch.buffer_size << "\n" << "Starting upload @" 
       << systime.GetSystemDateTime();
  PrintMessage(sbuf.c_str());

  // Get the current time before entering loop
  time(&begin);

  // Get CPU clock cycles before entering loop
  ps_begin = clock();

  bytes_moved = 0;
  if(end_of_file <= ch.buffer_size) { // The file is smaller than one buffer
    time(&begin_rx);
    bytes_moved = SendBuffer(&client, buffer, end_of_file);
    time(&end_rx);
    byte_count = end_of_file;  
    rx_time += difftime(end_rx, begin_rx);
    buffer_count++;
    curr_count++;
    if(bytes_moved < 0) err = 1;
  }
  else { // The file is larger than one buffer
    while((end_of_file > ch.buffer_size) && (echo_loop)) {
      time(&begin_rx);
      bytes_moved = SendBuffer(&client, buffer, ch.buffer_size);
      time(&end_rx);
      rx_time += difftime(end_rx, begin_rx);
      byte_count += bytes_moved;
      end_of_file -= ch.buffer_size;
      buffer_count++;
      curr_count++;
      if(bytes_moved < 0) {
	err = 1;
	break;
      }
      if(end_of_file <= ch.buffer_size) {
	time(&begin_rx);
	bytes_moved = SendBuffer(&client, buffer, end_of_file);
	time(&end_rx);
	byte_count += end_of_file;
	rx_time += difftime(end_rx, begin_rx);
	buffer_count++;
	curr_count++;
	if(bytes_moved < 0) err = 1;
	break;
      }
      else {
	if(curr_count > 1) {
	  if((curr_count * ch.buffer_size) >= (ch.bytes/report_percent)) {
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

  if(display_processor_clock) {
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

  PrintMessage("Disconnecting client...");
  client.Close();
  delete buffer;
  return 0;
}

int SendDownloadRequest(int num)
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

  DiskFileB testfile;

  if(write_to_output_file) {
    if(testfile.df_Create("testfile.dat") != 0) {
      sbuf << clear << "Error creating the " << output_file << " file";
      PrintMessage(sbuf.c_str());
      write_to_output_file = 0;
    }
  }

  ComHeader ch;
  int hdr_size = sizeof(ComHeader);

  sbuf << clear << "\n" << "Constructing TCP download client #" << num
       << "\n" << "Connecting to: " << server_ip << " on port: " 
       << port << "\n";
  PrintMessage(sbuf.c_str());
  gxSocket client(SOCK_STREAM, port, server_ip);
  if(!client) {
    CheckSocketError(&client);
   return 0;
  }

  // Connect to the server
  if(client.Connect() < 0) {
    CheckSocketError(&client, "Could not connect to the server");
    client.Close();
   return 0;
  }

  char *buffer = new char[buffer_size];
  if(!buffer) {
    sbuf << clear << "Could not allocate memory for the transmit buffer"
	 << "\n";
    PrintMessage(sbuf.c_str());
    client.Close();
   return 0;
  }

  // Fill the buffer with some data
  memset(buffer, buffer_fill, buffer_size);

  // Send the command header variables
  ch.command = (FAU_t)net_download;
  ch.bytes = num_to_send;
  ch.buffer_size = buffer_size;

  // Send the client header
  int bytes_moved = client.Send(&ch, hdr_size);
  if(bytes_moved != hdr_size) {
    sbuf << clear << "Cannot send message header to the server" << "\n";
    PrintMessage(sbuf.c_str());
    CleanBeforeExit(&client, 0);
    delete buffer;
   return 0;
  }

  IntSHN(ch.bytes, byte_string);
  sbuf << clear << "Downloading " << byte_string 
       << " bytes of data from " << server_ip
       << "\n" << "Receive buffer size = " 
       << ch.buffer_size << "\n" << "Starting download @" 
       << systime.GetSystemDateTime();
  PrintMessage(sbuf.c_str());

  // Get the current time before entering loop
  time(&begin);

  // Get CPU clock cycles before entering loop
  ps_begin = clock();

  int bytes_read = 0;
  while((bytes_read != ch.bytes) && (echo_loop)) {
    time(&begin_rx);
    bytes_read = client.RawRead(buffer, buffer_size);
    time(&end_rx);
    rx_time += difftime(end_rx, begin_rx);
    buffer_count++;
    curr_count++;

    if(write_to_output_file) {
      // Write buffer to a test file
      if(testfile.df_Write(buffer, bytes_read) != 0) {
	sbuf << clear << "Error writing to the " << output_file << " file";
	PrintMessage(sbuf.c_str());
	write_to_output_file = 0;
	testfile.df_Close();
      }
    }

    if(bytes_read == 0) break; // Data transfer complete
    if(bytes_read < 0) {
      if(client.GetSocketError() != gxSOCKET_RECEIVE_ERROR) {
	CheckSocketError(&client, "Error reading server socket");
      }
      else {
	PrintMessage("Server has disconnected during a socket read");
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
    sbuf << clear << "Download bytes per second rate = 100%";
    sbuf << "\n" << "LAN/WAN Benchmark speed = 100%";
    PrintMessage(sbuf.c_str());
  }

  if(display_processor_clock) {
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

  PrintMessage("Disconnecting client...");
  client.Close();
  if(write_to_output_file) {
    testfile.df_Close();
  }
  delete buffer;
  return 0;
}

void HelpMessage() 
{
  cout << "TCP bandwidth benchmark client version " << version_str << "\n" 
       << flush;
  cout << "Producted by: " << author_email << "\n" << flush;
  cout << "\n" << flush;
  cout << "Usage: " << program_name << " [switches] server " 
       << "\n" << flush;
  cout << "Switches: " << "\n" << flush;
  cout << "-? = Display this help message." << "\n" << flush;
  cout << "-b = Set buffer size, ranges = -b64 to -b65507"
       << "\n" << flush;
  cout << "-c = Display processor clock BPS rate" << "\n" << flush;
  cout << "-f[name] Write downloads to output file." << "\n" << flush;
  cout << "-k = Shutdown the server" << "\n" << flush;
  cout << "-l[name] = Write output to log file." << "\n" << flush;
  cout << "-n = Specify a number of clients, ranges -n1 to -n1000"
       << "\n" << flush;
  cout << "-p = Specify a port number, ranges -p1024 to -p65535"
       << "\n" << flush;
  cout << "-r = Restart the server" << "\n" << flush;
  cout << "-s = Specify the number of bytes, ranges -s1K to -s2G"
       << "\n" << flush;

    // End of list
  cout << "\n";
}

int ProcessArgs(char *arg)
{
  gxString sbuf;
  switch(arg[1]) {
    case 'b': case 'B':
      buffer_size = atoi(arg+2);
      if((buffer_size < 64) || (buffer_size > 65535)) {
	cout << "Bad buffer size specified: " << buffer_size << "\n";
	cout << "Buffer sizes should range between 64 to 65535" << "\n";
	return 0;
      }
      break;

    case 'c': case 'C':
      display_processor_clock = 1;
      break;

    case 'k': case 'K':
      stopping = 1;
      break;

    case 'r': case 'R':
      restarting = 1;
      break;

    case 'l': case 'L':
      enable_logging = 1;
      logfile_name = arg+2;
      break;

    case 'f': case 'F':
      write_to_output_file = 1;
      output_file = arg+2;
      break;

    case 'p': case 'P':
      port = (unsigned short)atoi(arg+2);
      if((port <= 0) || (port > 65535)) {
	cout << "Bad port number specified: " << port << "\n";
	cout << "Port number should range between 1024 to 65535" << "\n";
	// http://www.iana.org/assignments/port-numbers
	return 0;
      }
      break;

    case 'n': case 'N':
      num_clients = atoi(arg+2);
      if(num_clients <= 0) {
	cout << "Bad number of clients specified: " << num_clients << "\n";
	cout << "Number of clients should range between 1 and 1000" << "\n";
	return 0;
      }
      break;

    case 's': case 'S':
      sbuf = arg+2;
      if(sbuf.IFind("K") != -1) {
	sbuf.DeleteAfterIncluding("K");
	sbuf.TrimLeadingSpaces(); sbuf.TrimTrailingSpaces();
	num_to_send = (sbuf.Atoi() * x1K);
      }
      else if(sbuf.IFind("M") != -1) {
	sbuf.DeleteAfterIncluding("M");
	sbuf.TrimLeadingSpaces(); sbuf.TrimTrailingSpaces();
	num_to_send = (sbuf.Atoi() * x1M);
      }
      else if(sbuf.IFind("G") != -1) {
	sbuf.DeleteAfterIncluding("G");
	sbuf.TrimLeadingSpaces(); sbuf.TrimTrailingSpaces();
	num_to_send = (sbuf.Atoi() * x1G);
      }
      else {
	num_to_send = sbuf.Atoi();
      }
      if(num_to_send <= 0) {
	cout << "Bad number of bytes to send specified: " << num_to_send 
	     << "\n";
	cout << "The number of bytes to send should range between 1K and 2G"
	     << "\n";
	return 0;
      }
      if(num_to_send > (2.1 * x1G)) {
	cout << "A number of bytes over 2G was specified" << num_to_send 
	     << "\n";
	cout << "In order to support legacy systems the max number bytes \
is limited to 2G"
	     << "\n";
	return 0;
      }
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

  // Process command ling arguments and files 
  int narg;
  char *arg = argv[narg = 1];
  int num_servers = 0;
  LogFile logfile;

  if(argc < 2) {
    HelpMessage();
    return 1;
  }

  if(argc >= 2) {
    while (narg < argc) {
      if (arg[0] != '\0') {
	if (arg[0] == '-') { // Look for command line arguments
	  if(!ProcessArgs(arg)) return 1; // Exit if argument is not valid
	}
	else { 
	  // Process the command line string after last - argument
	  num_servers++;
	  server_ip = arg;
	}
	arg = argv[++narg];
      }
    }
  }
  else {
    HelpMessage();
    return 1;
  }

  if(num_servers == 0) {
    HelpMessage();
    return 1;
  }

  cout << "TCP bandwidth benchmark client" << "\n" 
       << flush;
  if(num_clients == 1) {
    cout << "Connecting " << num_clients << " client to " << server_ip << "\n" << flush;
  }
  else {
    cout << "Connecting " << num_clients << " clients to " << server_ip << "\n" << flush;
  }
  cout << "Press Ctrl-C at anytime to stop the test and exit program " 
       << "\n" << flush;

  if(enable_logging) {
    if(logfile.Open(logfile_name.c_str()) == 0) {
      servercfg->logfile = &logfile;
    }
    else {
      cout << "Could not open the " << logfile_name.c_str() 
	   << " logfile" << "\n" << flush;
    }
  }

  if(restarting) {
    RestartServer(1);
    return 0;
  }
  if(stopping) {
    RestartServer(0);
    return 0;
  }

  for(int i = 0; i < num_clients; i++) {
    SendDownloadRequest(i+1);
    SendUploadRequest(i+1);
  }

  cout << "\n";
  cout << "All tests complete" << "\n" << flush;
  cout << "Exiting..." << "\n" << flush;

  return 0;
}
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
