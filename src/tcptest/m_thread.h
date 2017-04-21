// ------------------------------- //
// -------- Start of File -------- //
// ------------------------------- //
// ----------------------------------------------------------- //
// C++ Header File
// C++ Compiler Used: GNU, Intel, Cray
// Produced By: Douglas.Gaer@noaa.gov
// File Creation Date: 10/17/2002
// Date Last Modified: 04/21/2017
// ----------------------------------------------------------- // 
// ---------- Include File Description and Details  ---------- // 
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
#ifndef __GX_S_MTHREAD_SERVER_HPP__
#define __GX_S_MTHREAD_SERVER_HPP__

#include "gxdlcode.h"
#include "gxmutex.h"
#include "gxcond.h"
#include "gxthread.h"
#include "gxsocket.h"
#include "gxheader.h"
#include "gxstring.h"
#include "systime.h"
#include "logfile.h"

// --------------------------------------------------------------
// Constants and type definitions
// --------------------------------------------------------------
// Thread constants
const int DISPLAY_THREAD_RETRIES = 3;
const int RECONNECT_RETRIES = 5;
const int RESTART_RETRIES = 5;
const int SLEEP_SECS_BEFORE_RETRY = 5;

// Network integer types
typedef FAU netint;

// Checkwork constants
const netint net_checkword = 0XABABFEFE;

// Client/Server command constants
const char net_download = 'D';
const char net_kill = 'K';
const char net_restart = 'R';
const char net_upload = 'U';

// Multiplier constants
const FAU_t x1K = 1000;
const FAU_t x1M = 1000 * 1000;
const FAU_t x1G = 1000 * (1000 * 1000);

// Default settings
const int DEFAULT_PORT = 53897;
const int DEFAULT_BUF_SIZE = 1500;
const char DEFAULT_BUF_FILL = 'X';
const int DEFAULT_NUM_TO_SEND = 150 * x1M;
// --------------------------------------------------------------

struct ComHeader {
  ComHeader() { Clear(); checkword = net_checkword; }
  ~ComHeader() { }

  void Clear() { 
    checkword = (netint)0;
    bytes = 0;
    buffer_size = 1500;
    command = (netint)0;
    memset(reserved, 0, 32);
  }

  netint checkword;   // Checkword
  netint bytes;       // Size of data transfer
  netint buffer_size; // Size of the rx and tx buffers
  netint command;     // Commands use to talk to this process
  char reserved[32];  // Reserved for future use 

private: // Disallow copying and assignment
  ComHeader(const ComHeader &ob) { }
  void operator=(const ComHeader &ob) { }
};

// Server configuration
struct  ServerConfig {
  ServerConfig();
  ~ServerConfig();

  // Helper functions
  void reset_all();
  
  // Server configuration variables
  gxsPort_t port;     // Server's port number
  int accept_clients; // Ture while accepting 
  int echo_loop;      // True to keep all connection persistent

  // gxThread variables
  thrPool *client_request_pool; // Worker threads processing client requests

  // Logging variables
  LogFile *logfile;

  // Process thread condition variable used to block this thread from its
  // own execution
  gxMutex process_lock;
  gxCondition process_cond;

  // gxThread synchronization interface
  gxMutex display_lock;      // Mutex object used to lock the display
  gxCondition display_cond;  // Condition variable used with the display lock
  int display_is_locked;     // Display lock boolean predicate

  // Server stop and restart variables
  int kill_server;
  int restart_server;
  int process_loop;

  int display_processor_clock;
};

class ProcessThread : public gxThread
{
public:
  ProcessThread() { }
  ~ProcessThread() { }

private:
  void *ThreadEntryRoutine(gxThread_t *thread);
};


// Client socket type used to associate client sockets other
// data types.
struct ClientSocket_t
{
  gxsSocket_t client_socket; // Client socket
  gxString hostname; // Client hostname
};

class ClientRequestThread : public gxSocket, public gxThread
{
public:
  ClientRequestThread() { }
  ~ClientRequestThread() { }

public: // Client routines
  void HandleClientRequest(ClientSocket_t *s); 
  void HandleUploadRequest(ClientSocket_t *s, ComHeader &ch);
  void HandleDownloadRequest(ClientSocket_t *s, ComHeader &ch);
  int SendBuffer(ClientSocket_t *s, void *buffer, int bytes);
  int RecvHeader(ClientSocket_t *s, ComHeader &ch);

private: // gxThread Interface
  void *ThreadEntryRoutine(gxThread_t *thread);
  void ThreadExitRoutine(gxThread_t *thread);
  void ThreadCleanupHandler(gxThread_t *thread);
};

class ServerThread : public gxSocket, public gxThread
{
public:
  ServerThread() { }
  ~ServerThread() { }

public: // Server functions
  int InitServer(int max_connections = SOMAXCONN);

private: // gxThread Interface
  void *ThreadEntryRoutine(gxThread_t *thread);
  void ThreadExitRoutine(gxThread_t *thread);
  void ThreadCleanupHandler(gxThread_t *thread);
  
public: // Server data members
  ClientRequestThread request_thread;
};

// --------------------------------------------------------------
// Standalone functions
// --------------------------------------------------------------
void PrintMessage(const char *s1 = " ", const char *s2 = " ", 
		  const char *s3 = " ");
void ReportError(const char *s1 = " ", const char *s2 = " ", 
		 const char *s3 = " ");
int CheckSocketError(gxSocket *s, const char *mesg = 0, 
		     int report_error = 1);
int CheckThreadError(gxThread_t *thread, const char *mesg = 0, 
		     int report_error = 1);
void IntSHN(FAU_t bytes, gxString &byte_string);
int NumClientThreadsRunning(gxThread *requesting_thread);
void SleepFor(int seconds);
// --------------------------------------------------------------

// --------------------------------------------------------------
// Globals configuration variables
// --------------------------------------------------------------
extern ServerConfig ServerConfigSruct;
extern ServerConfig *servercfg;
// --------------------------------------------------------------

#endif // __GX_S_MTHREAD_SERVER_HPP__
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
