/*
 *   scope_PP_handler.h
 * 
 *   This file specifies prototypes for handling the parallel port device that
 *    interfaces to the Nicolet Explorer III Oscilloscope
 *
 *    This module make calls to ioperm, so it must be run by a user with root permissions
 *
 *    Scott Nelson
 *    Jan, 2000
 */  

#ifndef SCOPE_PP_HANDLER_H
#define SCOPE_PP_HANDLER_H

int scope_open_port( int port );
  // This function opens an EPP Parallel port at the port address specified by the Parameter
  // This module can only open one port at a time
  // PRE: port is a valid parallel port address which is set in hardware EPP mode
  // returns <0 if an error results

int scope_close_port();
  // This function closes the port specified in open_port
  // also disables irq reporting on the parallel port
  // returns <0 if unsuccessful

int scope_check_active();
  // This function checks if the device is active
  // PRE Port must be opened with open_port, otherwise returns an error
  // returns 1 if active, 0 if inactive, <0 if error

int scope_check_live();
  // This function checks if the scope is in live mode
  // PRE Port must be opened
  // returns 1 if the scope is live

int scope_check_ioflag();
  // This function checks if the ioflag is active
  // returns 1 if active, 0 if inactive, <0 if error

int scope_read_ints ( int* buffer, int size, float* time_per_point, char do_norm );

  // reads in characters from the oscilloscope but does not do any data processing
  // PRE size must be less than 4096, which is the size of the entire oscilloscope memory
  // PRE scope_open_port must be called first
  // if do_norm = 1, then the horizontal normalizing data will be stored in the location of the pointer
  // and the vertical normalizations will be checked

int scope_set_live();

int scope_set_hold_next();

int scope_set_recall_last();

int check_EPP_timeout();

  // returns 1 if an EPP timout has occurred, 0 otherwise

int clear_EPP_port();

int scope_read_chars ( char* buffer, int size );

int convert_data( unsigned char* in_buf, int* out_buf, int size );

  // resets the EPP Port, including the timeout bit

int clear_EPP_timeout();

  //Clears the EPP timout bit, allowing port operation to continue

#endif
