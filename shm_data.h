/* shm_data.h
 *
 * This file defines the shared memory parameters and
 * the data structures in the shared memory
 *
 * Part of the Xnmr software project
 *
 * UBC Physics
 * April, 2000
 * 
 * written by: Scott Nelson, Carl Michal
 */


#ifndef SHM_DATA_H
#define SHM_DATA_H

#include <sys/types.h>

#include "param_utils.h"


// this is for shared memory checking to make sure versions match.
// this will ensure and acq and Xnmr get along.  

#define XNMR_ACQ_VERSION "0.99.2 FEB 20, 2003"

/*
 * memory related defs
 */

#define DATA_SHM_KEY 0x47
#define PARAMETER_LEN 16384

// max data points upped to 8192 for AD dsp board.
#define MAX_DATA_POINTS 8192
#define MIN_DATA_POINTS 128

/*
 * Flag defs
 */

#define NO_MODE 0
#define REPEAT_MODE 1
#define NORMAL_MODE 2
#define NORMAL_MODE_NOSAVE 3
/*
 *  Acquisition modes
 */

#define PHASE0   0
#define PHASE90  1
#define PHASE180 2
#define PHASE270 3

/*
 * This shared memory structure is used to share data, parameters and signal instructions
 * Between all three processes
 */

struct data_shm_t {
  
  char version[VERSION_LEN];
  pid_t pprog_pid;                               //Process IDs
  pid_t acq_pid;
  pid_t ui_pid;
  char ui_sig_acq_meaning;         //This is the meaning of a signal sent TO acq FROM ui
  char acq_sig_ui_meaning;         //As above, but in the other direction



  // The following parameters are for the acq process

  //unsigned int timeout_duration;  //In milliseconds
  unsigned char mode;
  char reset_dsp_and_synth; // if Xnmr sets this to 1, then acq will do it.
  unsigned long acqn;
  unsigned int acqn_2d;
  unsigned long last_acqn;
  unsigned int last_acqn_2d;
  unsigned long num_acqs;
  unsigned int num_acqs_2d;
  unsigned long ct;
  int npts;                        // the number of points in each acquisition
  float dwell;   // this goes from xnmr to pprog
  char parameters[ PARAMETER_LEN ];                     //A long string of parameters separated by '\n' 
  int parameter_count;
  char pulse_exec_path[ PATH_LENGTH ];          //The path to the pulse program
  char save_data_path[ PATH_LENGTH ];
  long long data_image[ MAX_DATA_POINTS *2 ];       //Data is stored in sequential xy pairs
  long long time_remaining; // time in pulse program remaining in 20MHz clock tics
  char ch1,ch2; // = A, B or C
  char force_synth; // flag to pulse utility routines 
  //that the current scan should be treated as a first scan - used by the synth set routines.

};

#endif











