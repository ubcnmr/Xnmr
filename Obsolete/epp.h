// File epp.h

#ifndef _EPP_
#define _EPP_

#include <stdio.h>
#include <sys/io.h>

// Define addresses of various devices addresed by the EPP data lines
// D0-D2 via 3-8 decoder
#define FIFO = 1                 // 00000001
#define FIFO_FLIP_FLOP = 2       // 00000010
#define DSP_MICROPORT = 3        // 00000011
#define DSP_DATA_LINES = 4       // 00000100



void select_device(int address);
// Selects the device at "address" for reading and/or writing by writing
// "address" to EPP address register.  All other devices will have 
// their control lines in the high-impedance state

void initialize_epp();
// Must be called before any epp related functions

char read_fifo();
// Pre: Fifo control lines are active
// Post: reads 1 byte from data lines coming from fifo

char read_micro_addr();
// Pre: DSP microport address selected (control lines are active)
// Post: Reads the contents of the address lines A0-A2 of DSP

void write_micro_addr(const int data);

char read_micro(); 
// Post: reads microport data lines D0-D7 of DSP 

void write_micro();
// Pre: DSP microport data lines selected (control lines are active)
// Post: epp data lines D0-D7 are sent to an internal address location
// in the DSP specified by A0-A2

void reset(int device);
// Post: Resets the given device to default value.  If no device is specified
// then it resets all devices.

#endif






