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

Application POSIX and/or WIN32 thread framework
*/
// ----------------------------------------------------------- //   
#ifndef __M_THREAD_HPP__
#define __M_THREAD_HPP__

// DataReel includes
#include "gxdlcode.h"
#include "gxdstats.h"
#include "gxbtree.h"
#include "ustring.h"
#include "gxheader.h"

// C library includes
#include <stdio.h>
#include <time.h>
#include <stdint.h>

// Application framework includes
#include "m_log.h"

#ifndef  __LLWORD__
typedef long long __LLWORD__;
#endif

#ifndef __ULLWORD__
typedef unsigned long long __ULLWORD__;	
#endif

// Set the Btree order
// const BtreeNodeOrder_t MyKeyClassOrder = 8;
// const BtreeNodeOrder_t MyKeyClassOrder = 16;
// const BtreeNodeOrder_t MyKeyClassOrder = 32;
// const BtreeNodeOrder_t MyKeyClassOrder = 64;
const BtreeNodeOrder_t MyKeyClassOrder = 128;
// const BtreeNodeOrder_t MyKeyClassOrder = 256;
// const BtreeNodeOrder_t MyKeyClassOrder = 1024; 

class MyKeyClass : public DatabaseKeyB
{
public:
  MyKeyClass() : DatabaseKeyB((char *)&data) { }
  MyKeyClass(gxUINT32 i) : DatabaseKeyB((char *)&data) { data = i; }

public:
  size_t KeySize() { return sizeof(data); }
  int operator==(const DatabaseKeyB& key) const;
  int operator>(const DatabaseKeyB& key) const;
  int CompareKey(const DatabaseKeyB& key) const;

public:
  gxUINT32 data;
};

// Stand alone benchmark functions
void BtreeStatus(gxBtree &btx);
void BuildTree(gxBtree &btx, unsigned long INSERTIONS, gxString plotdata_fname, int write_plot_data_file);
void IntSHN(FAU_t bytes, gxString &byte_string, int rountoff=1);

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
__ULLWORD__ calc_time_diff(struct timespec *a, struct timespec *b);
#endif

#endif // __M_THREAD_HPP__
// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
