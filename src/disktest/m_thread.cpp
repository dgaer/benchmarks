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

Application POSIX and/or WIN32 thread framework
*/
// ----------------------------------------------------------- // 
#include "gxdlcode.h"
#include "m_thread.h"
#include "gxstring.h"
#include "dfileb.h"

int MyKeyClass::operator==(const DatabaseKeyB& key) const
{
  const MyKeyClass *kptr = (const MyKeyClass *)(&key);
  return data == *((gxUINT32 *)kptr->db_key);
}

int MyKeyClass::operator>(const DatabaseKeyB& key) const
{
  const MyKeyClass *kptr = (const MyKeyClass *)(&key);
  return data > *((gxUINT32 *)kptr->db_key);
}

int MyKeyClass::CompareKey(const DatabaseKeyB& key) const
// NOTE: This comparison function is only used if the                                                                                         
// __USE_SINGLE_COMPARE__ preprocessor directive is                                                                                           
// defined when the program is compiled.                                                                                                      
{
  const MyKeyClass *kptr = (const MyKeyClass *)(&key);
  if(data == *((gxUINT32 *)kptr->db_key)) return 0;
  if(data > *((gxUINT32 *)kptr->db_key)) return 1;
  return -1;
}

void BtreeStatus(gxBtree &btx)
{
  UString intbuf;
  std::cout << "\n";
  intbuf << clear << (FAU_t)btx.Root();
  std::cout << "Root address =      " << intbuf.c_str() << "\n" << std::flush;
  std::cout << "Number of trees =   " << btx.NumTrees() << "\n" << std::flush;
  std::cout << "Number of entries = " << btx.NumKeys() << "\n" << std::flush;
  std::cout << "Number of nodes =   " << btx.NumNodes() << "\n" << std::flush;
  std::cout << "B-Tree order =      " << btx.NodeOrder() << "\n" << std::flush;
  std::cout << "B-Tree height =     " << btx.BtreeHeight() << "\n" << std::flush;
}

void BuildTree(gxBtree &btx, unsigned long INSERTIONS, gxString plotdata_fname, int write_plot_data_file)
{
  MyKeyClass key;
  MyKeyClass compare_key;

  unsigned long i, key_count = 0;
  unsigned long curr_count = 0;
  int rv;
  int verify_deletions = 0; // Set to ture to verify all deletions
  double insert_time = 0.0;
  double read_time = 0.0;
  double write_time = 0.0;
  double remove_time = 0.0;

  gxString ibuf;
  gxString plot_data = "[DISKBENCHMARK]\n";
  plot_data << "num_inserts=" << INSERTIONS << "\n";
  IntSHN((FAU_t)INSERTIONS, ibuf);
  plot_data << "num_inserts_str=" << ibuf << "\n";
  gxString write_time_py = "write_time=";
  gxString write_time_avg_py = "write_time_avg=";
  gxString read_time_py = "read_time=";
  gxString read_time_avg_py = "read_time_avg=";
  gxString remove_time_py = "remove_time=";
  gxString remove_time_avg_py = "remove_time_avg=";
  gxString rewrite_time_py = "rewrite_time=";
  gxString rewrite_time_avg_py = "rewrite_time_avg=";
  gxString xticks_py = "xticks=";
  gxString xticks_str_py = "xticks_str=";

  write_time_py.Precision(6);
  write_time_avg_py.Precision(6);
  read_time_py.Precision(6);
  read_time_avg_py.Precision(6);
  remove_time_py.Precision(6);
  remove_time_avg_py.Precision(6);
  rewrite_time_py.Precision(6);
  rewrite_time_avg_py.Precision(6);

  std::cout << "Inserting " << INSERTIONS << " keys..." << "\n" << std::flush;

  // Get CPU clock cycles before entering loop
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
  __ULLWORD__ elapsed_nano_secs;
  struct timespec start_loop, end_loop, start_proc, end_proc;
  memset(&start_loop, 0, sizeof(start_loop));
  memset(&end_loop, 0, sizeof(end_loop));
  memset(&start_proc, 0, sizeof(start_proc));
  memset(&end_proc, 0, sizeof(end_proc));
  const __ULLWORD__ nanosec = 1000000000;
  __ULLWORD__ buf_times_total = (__ULLWORD__)0;
  __ULLWORD__ buf_times = (__ULLWORD__)0;
  std::cout << "Using CLOCK_MONOTONIC and USE_CLOCK_GETTIME" "\n" << std::flush;
  clock_gettime(CLOCK_MONOTONIC, &start_loop);
#else
  clock_t begin = clock();
  clock_t buf_times_total = (clock_t)0;
  clock_t buf_times = (clock_t)0;
#endif

  for(i = 0; i < INSERTIONS; i++) {
    key.data = i;

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
    clock_gettime(CLOCK_MONOTONIC, &start_proc);
#else
    clock_t begin_insert = clock();
#endif

    rv = btx.Insert(key, compare_key, 0);

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
    clock_gettime(CLOCK_MONOTONIC, &end_proc);
    elapsed_nano_secs = calc_time_diff(&end_proc, &start_proc);
    buf_times_total += elapsed_nano_secs;
    buf_times += elapsed_nano_secs;
#else
    clock_t end_insert = clock();
    buf_times_total += (end_insert - begin_insert);
    buf_times += (end_insert - begin_insert);
#endif

    key_count++;
    curr_count++;
    if(rv != 1) {
      std::cout << "\n" << "Problem adding key - " << i << "\n" << std::flush;
      std::cout << btx.DatabaseExceptionMessage() << "\n" << std::flush;
      return;
    }

    if(curr_count >= (.1*INSERTIONS)) {
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
      write_time = buf_times/(double)nanosec;
      insert_time = buf_times_total/(double)nanosec;
#else
      write_time = buf_times/(double)CLOCKS_PER_SEC;
      insert_time = buf_times_total/(double)CLOCKS_PER_SEC;
#endif
      double avg = (write_time/(double)curr_count) * 1000;
      std::cout << "Inserted " << i << " keys in " << insert_time
		<< " seconds" << "\n" << std::flush;
      write_time_py << write_time << ",";
      write_time_avg_py << avg << ",";
      xticks_py << (i+1) << ",";
      IntSHN((FAU_t)(i+1), ibuf);
      xticks_str_py << ibuf << ",";
      curr_count = 0;
      write_time = 0;
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
      buf_times = (__ULLWORD__)0;
#else
      buf_times = (clock_t)0;
#endif
    }
  }
  // Get CPU clock cycles after loop is completed 
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
  clock_gettime(CLOCK_MONOTONIC, &end_loop);
  elapsed_nano_secs = calc_time_diff(&end_loop, &start_loop);
  double elapsed_time = elapsed_nano_secs/(double)nanosec;
  buf_times_total = (__ULLWORD__)0;
  buf_times = (__ULLWORD__)0;
#else
  clock_t end =clock();
  // Calculate the elapsed time in seconds. 
  double elapsed_time = (end - begin) / (double)CLOCKS_PER_SEC;
  buf_times_total = (clock_t)0;
  buf_times = (clock_t)0;
#endif
  
  write_time_py.TrimTrailing(','); 
  write_time_py << "\n";
  write_time_avg_py.TrimTrailing(',');
  write_time_avg_py << "\n";
  xticks_py.TrimTrailing(',');
  xticks_py << "\n";
  xticks_str_py.TrimTrailing(',');
  xticks_str_py << "\n";
  plot_data << write_time_py << write_time_avg_py << xticks_py << xticks_str_py;

  std::cout.precision(3); 
  std::cout << "Inserted " << key_count << " values in " 
	    << elapsed_time << " seconds" << "\n" << std::flush;
  double avg_insert_time = (insert_time/(double)key_count) * 1000;
  std::cout << "Average insert time = " << avg_insert_time << " milliseconds"  
	    << "\n" << std::flush; 
  gxString fbuf;
  fbuf.Clear();
  fbuf.Precision(6);
  fbuf << elapsed_time;
  plot_data << "insert_time=" << fbuf << "\n";
  fbuf.Clear();
  fbuf.Precision(6);
  fbuf << avg_insert_time;
  plot_data << "insert_time_avg=" << fbuf << "\n";

  BtreeStatus(btx);
  curr_count = 0;
  key_count = 0;
  double search_time = 0;
  
  std::cout << "\n" << std::flush;
  std::cout << "Verifying the insertions..." << "\n" << std::flush;
  
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
  clock_gettime(CLOCK_MONOTONIC, &start_loop);
#else
  begin = clock();
#endif
  for(i = 0; i < INSERTIONS; i++) {
    key.data = i;
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
    clock_gettime(CLOCK_MONOTONIC, &start_proc);
#else
    clock_t begin_search = clock();
#endif

    rv = btx.Find(key, compare_key, 0);

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
    clock_gettime(CLOCK_MONOTONIC, &end_proc);
    elapsed_nano_secs = calc_time_diff(&end_proc, &start_proc);
    buf_times_total += elapsed_nano_secs;
    buf_times += elapsed_nano_secs;
#else
    clock_t end_search = clock();
    buf_times_total += (end_search - begin_search);
    buf_times += (end_search - begin_search);
#endif
    
    key_count++;
    curr_count++;
    if(rv != 1) {
      std::cout << "Error finding key - " << i << "\n" << std::flush;
      std::cout << btx.DatabaseExceptionMessage() << "\n" << std::flush;
      return;
    }
    
    if(curr_count >= (.1*INSERTIONS)) {
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
      read_time = buf_times/(double)nanosec;
      search_time = buf_times_total/(double)nanosec;
#else
      read_time = buf_times/(double)CLOCKS_PER_SEC;
      search_time = buf_times_total/(double)CLOCKS_PER_SEC;
#endif
      double avg = (read_time/(double)curr_count) * 1000;
      
      std::cout << "Read " << i << " keys in " << search_time
		<< " seconds" << "\n" << std::flush;
      
      read_time_py << read_time << ",";
      read_time_avg_py << avg << ",";
      curr_count = 0;
      read_time = 0;
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
      buf_times = (__ULLWORD__)0;
#else
      buf_times = (clock_t)0;
#endif
    }
  }
  // Get CPU clock cycles after loop is completed 
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
  clock_gettime(CLOCK_MONOTONIC, &end_loop);
  elapsed_nano_secs = calc_time_diff(&end_loop, &start_loop);
  elapsed_time = elapsed_nano_secs/(double)nanosec;
  buf_times_total = (__ULLWORD__)0;
  buf_times = (__ULLWORD__)0;
#else
  end = clock();
  // Calculate the elapsed time in seconds. 
  elapsed_time = (end - begin) / (double)CLOCKS_PER_SEC;
  buf_times_total = (clock_t)0;
  buf_times = (clock_t)0;
#endif

  read_time_py.TrimTrailing(','); 
  read_time_py << "\n";
  read_time_avg_py.TrimTrailing(',');
  read_time_avg_py << "\n";
  plot_data << read_time_py << read_time_avg_py;

  std::cout.precision(3);
  std::cout << "Verified " << key_count << " values in " 
	    << elapsed_time << " seconds" << "\n" << std::flush;
  double avg_search_time = (search_time/(double)key_count) * 1000;
  std::cout << "Average search time = " << avg_search_time << " milliseconds"
	    << "\n" << std::flush;

  fbuf.Clear();
  fbuf.Precision(6);
  fbuf << elapsed_time;
  plot_data << "search_time=" << fbuf << "\n";
  fbuf.Clear();
  fbuf.Precision(6);
  fbuf << avg_search_time;
  plot_data << "search_time_avg=" << fbuf << "\n";

  std::cout << "\n" << std::flush;
  std::cout << "Deleting all the entries..." << "\n" << std::flush;
  key_count = 0;
  curr_count = 0;
  double delete_time = 0;

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
  clock_gettime(CLOCK_MONOTONIC, &start_loop);
#else
  begin = clock();
#endif
  for(i = 0; i < INSERTIONS; i++) {
    key.data = i;

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
    clock_gettime(CLOCK_MONOTONIC, &start_proc);
#else
    clock_t begin_delete = clock();
#endif

    rv = btx.Delete(key, compare_key, 0);

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
    clock_gettime(CLOCK_MONOTONIC, &end_proc);
    elapsed_nano_secs = calc_time_diff(&end_proc, &start_proc);
    buf_times_total += elapsed_nano_secs;
    buf_times += elapsed_nano_secs;
#else
    clock_t end_delete = clock();
    buf_times_total += (end_delete - begin_delete);
    buf_times += (end_delete - begin_delete);
#endif

    key_count++;
    curr_count++;
    if(rv != 1) {
      std::cout << "Error deleting key - " << i << "\n" << std::flush;
      std::cout << btx.DatabaseExceptionMessage() << "\n" << std::flush;
      return;
    }

    if(curr_count >= (.1*INSERTIONS)) {
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
      remove_time = buf_times/(double)nanosec;
      delete_time = buf_times_total/(double)nanosec;
#else
      remove_time = buf_times/(double)CLOCKS_PER_SEC;
      delete_time = buf_times_total/(double)CLOCKS_PER_SEC;
#endif
      double avg = (remove_time/(double)curr_count) * 1000;
     
      std::cout << "Removed " << i << " keys in " << delete_time
		<< " seconds" << "\n" << std::flush;
      remove_time_py << remove_time << ",";
      remove_time_avg_py << avg << ",";
      remove_time = 0;
      curr_count = 0;
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
      buf_times = (__ULLWORD__)0;
#else
      buf_times = (clock_t)0;
#endif
    }

    if(verify_deletions) { // Verify the remaining key locations
      for(unsigned long j = INSERTIONS-1; j != i; j--) {
	key.data = j;
	rv = btx.Find(key, compare_key, 0);
	if(rv != 1) {
	  std::cout << "Error finding key  - " << j << "\n" << std::flush;
	  std::cout << "After deleting key - " << i << "\n" << std::flush;
	  std::cout << btx.DatabaseExceptionMessage() << "\n" << std::flush;
	  return;
	}
      }
    }
  }
  // Get CPU clock cycles after loop is completed 
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
  clock_gettime(CLOCK_MONOTONIC, &end_loop);
  elapsed_nano_secs = calc_time_diff(&end_loop, &start_loop);
  elapsed_time = elapsed_nano_secs/(double)nanosec;
  buf_times_total = (__ULLWORD__)0;
  buf_times = (__ULLWORD__)0;
#else
  end =clock();
  // Calculate the elapsed time in seconds. 
  elapsed_time = (end - begin) / (double)CLOCKS_PER_SEC;
  buf_times_total = (clock_t)0;
  buf_times = (clock_t)0;
#endif

  remove_time_py.TrimTrailing(','); 
  remove_time_py << "\n";
  remove_time_avg_py.TrimTrailing(',');
  remove_time_avg_py << "\n";
  plot_data << remove_time_py << remove_time_avg_py;

  std::cout.precision(3);
  std::cout << "Deleted " << key_count << " values in " 
	    << elapsed_time << " seconds" << "\n" << std::flush;
  double avg_delete_time = (delete_time/(double)key_count) * 1000;
  std::cout << "Average delete time = " << avg_delete_time << " milliseconds"
	    << "\n" << std::flush;

  fbuf.Clear();
  fbuf.Precision(6);
  fbuf << elapsed_time;
  plot_data << "delete_time=" << fbuf << "\n";
  fbuf.Clear();
  fbuf.Precision(6);
  fbuf << avg_delete_time;
  plot_data << "delete_time_avg=" << fbuf << "\n";

  std::cout << "\n" << std::flush;
  std::cout << "Re-inserting " << INSERTIONS << " keys..." << "\n" << std::flush;
  key_count = 0;
  curr_count = 0;
  insert_time = 0;
  write_time = 0;

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
  clock_gettime(CLOCK_MONOTONIC, &start_loop);
#else
  begin = clock();
#endif
  for(i = 0; i < INSERTIONS; i++) {
    key.data = i;

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
    clock_gettime(CLOCK_MONOTONIC, &start_proc);
#else
    clock_t begin_insert = clock();
#endif

    rv = btx.Insert(key, compare_key, 0);

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
    clock_gettime(CLOCK_MONOTONIC, &end_proc);
    elapsed_nano_secs = calc_time_diff(&end_proc, &start_proc);
    buf_times_total += elapsed_nano_secs;
    buf_times += elapsed_nano_secs;
#else
    clock_t end_insert = clock();
    buf_times_total += (end_insert - begin_insert);
    buf_times += (end_insert - begin_insert);
#endif

    key_count++;
    curr_count++;

    if(rv != 1) {
      std::cout << "\n" << "Problem adding key - " << i << "\n" << std::flush;
      std::cout << btx.DatabaseExceptionMessage() << "\n" << std::flush;
      return;
    }

    if(curr_count >= (.1*INSERTIONS)) {
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
      write_time = buf_times/(double)nanosec;
      insert_time = buf_times_total/(double)nanosec;
#else
      write_time = buf_times/(double)CLOCKS_PER_SEC;
      insert_time = buf_times_total/(double)CLOCKS_PER_SEC;
#endif
      double avg = (write_time/(double)curr_count) * 1000;
      
      std::cout << "Inserted " << i << " keys in " << insert_time
		<< " seconds" << "\n" << std::flush;
      rewrite_time_py << write_time << ",";
      rewrite_time_avg_py << avg << ",";
      write_time = 0;
      curr_count = 0;
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
      buf_times = (__ULLWORD__)0;
#else
      buf_times = (clock_t)0;
#endif
    }
  }
  // Get CPU clock cycles after loop is completed 
#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
  clock_gettime(CLOCK_MONOTONIC, &end_loop);
  elapsed_nano_secs = calc_time_diff(&end_loop, &start_loop);
  elapsed_time = elapsed_nano_secs/(double)nanosec;
  buf_times_total = (__ULLWORD__)0;
  buf_times = (__ULLWORD__)0;
#else
  end =clock();
  // Calculate the elapsed time in seconds. 
  elapsed_time = (end - begin) / (double)CLOCKS_PER_SEC;
  buf_times_total = (clock_t)0;
  buf_times = (clock_t)0;
#endif

  rewrite_time_py.TrimTrailing(','); 
  rewrite_time_py << "\n";
  rewrite_time_avg_py.TrimTrailing(',');
  rewrite_time_avg_py << "\n";
  plot_data << rewrite_time_py << rewrite_time_avg_py;

  std::cout.precision(3); 
  std::cout << "Inserted " << key_count << " values in " 
	    << elapsed_time << " seconds" << "\n" << std::flush;
  avg_insert_time = (insert_time/(double)key_count) * 1000;
  std::cout << "Average insert time = " << avg_insert_time << " milliseconds"  
	    << "\n" << std::flush; 

  fbuf.Clear();
  fbuf.Precision(6);
  fbuf << elapsed_time;
  plot_data << "reinsert_time=" << fbuf << "\n";
  fbuf.Clear();
  fbuf.Precision(6);
  fbuf << avg_insert_time;
  plot_data << "reinsert_time_avg=" << fbuf << "\n";
  plot_data << "\n";
  plot_data << "[PLOTINFO]" << "\n";
  plot_data << "DPI=75" << "\n";
  plot_data << "plot_type=times" << "\n";
  plot_data << "\n";

  std::cout << "\n" << std::flush;
  std::cout << plot_data.c_str();
  std::cout << "\n" << std::flush;

  if(write_plot_data_file) {
    DiskFileB dfile(plotdata_fname.c_str(), DiskFileB::df_READWRITE, DiskFileB::df_CREATE); 
    if(!dfile) {
      std::cout << "ERROR - Could not open the " << plotdata_fname.c_str() << "\n" << std::flush;
      return;
    }
    
    if(dfile.df_Write((const char *)plot_data.c_str(), plot_data.length()) != 0) {
      std::cout << "ERROR - Could write to " << plotdata_fname.c_str() << "\n" << std::flush;
      return;
    }
    dfile.df_Close();
  }
}

void IntSHN(FAU_t bytes, gxString &byte_string, int roundoff)
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
  if(roundoff) {
    if(lbuf[lbuf.length()-1] == '.') lbuf.DeleteAfterIncluding(".");
  }
  else {
    if(lbuf[lbuf.length()-1] == '.') lbuf += "0";
  }

  byte_string << clear << lbuf << shn_c;
}

#if defined (CLOCK_MONOTONIC) && defined (USE_CLOCK_GETTIME)
__ULLWORD__ calc_time_diff(struct timespec *a, struct timespec *b)
{
  __ULLWORD__ rv;
  const __ULLWORD__ nanosec = 1000000000;
  rv = ((a->tv_sec * nanosec) + a->tv_nsec) - ((b->tv_sec * nanosec) + b->tv_nsec);
  return rv;
}
#endif

// ----------------------------------------------------------- // 
// ------------------------------- //
// --------- End of File --------- //
// ------------------------------- //
