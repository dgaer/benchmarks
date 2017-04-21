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

Multicast benchmark client.
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "m_thread.h"

// --------------------------------------------------------------
// Constants
// --------------------------------------------------------------
const char *version_str = "2.0";
const char *program_name = "mctest";
const char *author_email = "Douglas.Gaer@noaa.gov";
// --------------------------------------------------------------

// --------------------------------------------------------------
// Globals
// --------------------------------------------------------------
char *group_ip = (char *)GetDefaultGroup();
gxsPort_t port = DEFAULT_PORT;
int echo_loop = 1;
int datagram_size = DEFAULT_DATAGRAM_SIZE;
char datagram_fill = DEFAULT_DATAGRAM_FILL;
int num_to_send = DEFAULT_NUM_TO_SEND;
int num_clients = 1;
gxString logfile_name;
int enable_logging = 0;
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

int RecvDatagram(gxSocket *client, void *datagram, int bytes)
{
  if(bytes == 0) {
    PrintMessage("No bytes are being received");
    return -1;
  }
  
  int bytes_read = client->RecvFrom(datagram, bytes);
  if(bytes_read < 0) {
    if(client->GetSocketError() != gxSOCKET_RECEIVE_ERROR) {
      CheckSocketError(client, "Error reading server socket");
    }
    else {
      PrintMessage("Error during socket read");
    }
    CleanBeforeExit(client, 0);
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

int SendDownloadRequest(int num)
{
  gxString sbuf, lbuf;
  SysTime systime;
  gxString byte_string;
  FAU_t byte_count = (FAU_t)0;
  double rx_time = (double)0;
  double elapsed_time = (double)0;
  double avg_insert_time = (double)0;
  FAU_t datagram_count = (FAU_t)0;
  FAU_t curr_count = (FAU_t)0;
  FAU_t curr_percent = (FAU_t)0;
  FAU_t byps = (FAU_t)0;
  FAU_t bps = (FAU_t)0;
  int err = 0;
  const int report_percent = 10;
  time_t begin, begin_rx, end_rx, end;
  double r_percent, c_percent;
  int t_percent;
  FAU_t end_of_file = num_to_send;

  sbuf << clear << "\n" << "Constructing multicast client #" << num
       << "\n" << "Connecting to: " << group_ip << " on port: " 
       << port << "\n";
  PrintMessage(sbuf.c_str());

  char *datagram = new char[datagram_size];
  if(!datagram) {
    sbuf << clear << "Could not allocate memory for the transmit datagram"
	 << "\n";
    PrintMessage(sbuf.c_str());
   return 0;
  }

  gxSocket client(SOCK_DGRAM, SOCK_DGRAM, IPPROTO_UDP, port);
  if(!client) {
    CheckSocketError(&client);
   return 0;
  }

  // Set the socket options if needed
  int optVal = 1;
  int rv = client.SetSockOpt(SOL_SOCKET, SO_REUSEADDR,
			     (char *)&optVal, sizeof(optVal));
  if(rv < 0) return CleanBeforeExit(&client, 0);

  // Bind to the multicast port
  rv = client.Bind();
  if(rv < 0) return CleanBeforeExit(&client, 0);

  ip_mreq mreq; // Multicast address join structure
  mreq.imr_multiaddr.s_addr=inet_addr(group_ip);

  // Accept multicast from any interface
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);

  // Join the multicast group using the SetSockOpt() function to
  // request that the kernel join a multicast group.
  rv = client.SetSockOpt(IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq,
			 sizeof(mreq));
  if(rv < 0) return CleanBeforeExit(&client, 0);

  sbuf << clear << "Joining multicast group " << group_ip << "\n";
  sbuf << "Receiving datagrams on port " << port << "\n";
  sbuf << "Press Ctrl-C to exit" << "\n";
  PrintMessage(sbuf.c_str());

  IntSHN(end_of_file, byte_string);
  sbuf << clear << "Waiting to receive " << byte_string 
       << " bytes of data from " << group_ip
       << "\n" << "Receive datagram size = " 
       << datagram_size << "\n" << "Starting multicast receive @" 
       << systime.GetSystemDateTime();
  PrintMessage(sbuf.c_str());

  // Get the current time before entering loop
  time(&begin);

  int bytes_read = 0;
  while((echo_loop) && (bytes_read < end_of_file)) { 
    time(&begin_rx);
    bytes_read = RecvDatagram(&client, datagram, datagram_size);
    time(&end_rx);
    rx_time += difftime(end_rx, begin_rx);
    byte_count += bytes_read;
    end_of_file -= datagram_size;
    datagram_count++;
    curr_count++;
    if(bytes_read < 0) {
      err = 1;
      break;
    }
    if(curr_count > 1) {
      if((curr_count * datagram_size) >= (end_of_file/report_percent)) {
	// Report the byte count every 10 percent
	r_percent = (curr_percent++ * report_percent) / 100;
	c_percent = end_of_file * r_percent;
	if((FAU_t)c_percent >= (end_of_file - byte_count)) {
	  curr_count = 0;
	  IntSHN(byte_count, byte_string);
	  t_percent = curr_percent * report_percent;
	  t_percent -= 100;
	  if(t_percent >= 100) {
	    sbuf << clear << "--% Complete - received "; 
	  }
	  else {
	    sbuf << clear << t_percent << "% Complete - received "; 
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

  // Calculate the elapsed time in seconds. 
  elapsed_time = difftime(end, begin);
  if(err) {
    sbuf << clear << "Multicast test partially completed!" << "\n";
    sbuf << "Multicast test failed @" << systime.GetSystemDateTime();
  }
  else {
    sbuf << clear << "Multicast Download completed @" 
	 << systime.GetSystemDateTime();
  }
  PrintMessage(sbuf.c_str());

  sbuf.Precision(2); 
  IntSHN(byte_count, byte_string);
  sbuf << clear << "Received " << byte_string << " bytes in " 
       << elapsed_time << " seconds" << "\n";
  sbuf << "Number of multicast datagrams = " << datagram_count << "\n";
  sbuf << "Average datagram size = " << byte_count/datagram_count 
       << " bytes" << "\n";
  avg_insert_time = (rx_time/(double)datagram_count) * 1000;
  sbuf.Precision(4); 
  sbuf << "Average datagram download time = " << avg_insert_time 
       << " milliseconds";
  PrintMessage(sbuf.c_str());
  
  // Calculate the number of bits per second
  if(elapsed_time != (double)0) {
    byps = FAU_t(byte_count/elapsed_time);
    bps = byps * 8;
    IntSHN(byps, byte_string);
    sbuf << clear << "Multicast bytes per second rate = " << byte_string 
	 << "BytesPS";
    IntSHN(bps, byte_string);
    sbuf << "\n" << "LAN/WAN Benchmark multicast speed = " << byte_string 
	 << "BPS";
    PrintMessage(sbuf.c_str());
  }
  else {
    sbuf << clear << "Multicast bytes per second rate = 100%";
    sbuf << "\n" << "LAN/WAN Benchmark multicast speed = 100%";
    PrintMessage(sbuf.c_str());
  }

  PrintMessage("Closing client session...");
  client.Close();
  delete datagram;
  return 0;
}

void HelpMessage() 
{
  cout << "Multicast bandwidth benchmark client" << version_str << "\n" 
       << flush;
  cout << "Producted by: " << author_email << "\n" << flush;
  cout << "\n" << flush;
  cout << "Usage: " << program_name << " [switches] server " 
       << "\n" << flush;
  cout << "Switches: " << "\n" << flush;
  cout << "-? = Display this help message." << "\n" << flush;
  cout << "-b = Set datagram size, ranges = -b64 to -b65507"
       << "\n" << flush;
  cout << "-g = Specify a multicast group, ranges 224.0.0.0 to 239.255.255.255"
       << "\n" << flush;
  cout << "-l[name] = Write output to log file." << "\n" << flush;
  cout << "-n = Specify a number of clients, ranges -n1 to -n1000"
       << "\n" << flush;
  cout << "-p = Specify a port number, ranges -p1024 to -p65535"
       << "\n" << flush;
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
      datagram_size = atoi(arg+2);
      if((datagram_size < 64) || (datagram_size > 65535)) {
	cout << "Bad datagram size specified: " << datagram_size << "\n";
	cout << "Datagram sizes should range between 64 to 65535" << "\n";
	return 0;
      }
      break;

    case 'g': case 'G':
      group_ip = arg+2;
      break;

    case 'l': case 'L':
      enable_logging = 1;
      logfile_name = arg+2;
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

  // Process command ling arguments and files 
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

  cout << "UDP bandwidth benchmark client" << "\n" 
       << flush;
  cout << "Connecting " << num_clients << " to " << group_ip << "\n" 
       << flush;
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

  for(int i = 0; i < num_clients; i++) {
    SendDownloadRequest(i+1);
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
