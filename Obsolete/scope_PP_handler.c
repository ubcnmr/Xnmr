/* scope_PP_handler.c
 *
 *  This file specifies implementations for handling the parallel port device that
 *  interfaces to the Nicolet Explorer III Oscilloscope
 *
 *
 *
 *  Scott Nelson, Jan 2000
 *
 *  Updated Feb 25, 2000 by Scott Nelson to collect normalizing data from the oscilloscope
 *  and to do error checking on buffer size in method read_ints
 */


#include <stdio.h>    //for printf, etc
#include <stdlib.h>   //for malloc, free
#include <unistd.h>   //for usleep
#include <sys/io.h>   //for ioperm routine
//#include <asm/io.h>   //for inb, outb, etc.
#include <math.h>
#include <glib.h>

#include "scope_PP_handler.h"
#include "shm_data.h"

#define SPP_DATA 0x00
#define SPP_STAT 0x01
#define SPP_CTRL 0x02
#define EPP_ADDR 0x03
#define EPP_DATA 0x04

#define SCOPE_IDLE 0x75   //0111 0101

int base = -1;

int scope_open_port( int port )

{
  int i;

  //check to see if port is already open

  if( base != -1 )
    return -1;
 
  base = port;

  
  i = ioperm( base, 8, 1 );

  if( i<0 ) {
    printf( "#Can't get permission to access the port 0x%x\n", base );
    return -1;
  }

 
  clear_EPP_port();

  //Make sure device is disabled

  outb( 0x74, base + EPP_DATA );            //0111 0100
  outb( SCOPE_IDLE, base+EPP_DATA );        //0111 0101

  return 0;
}


int scope_close_port()

{
  int result;

  if( base == -1 ) {
    //printf( "no need to close scope port\n" );
    return 0;
  }
  result = ioperm( base, 8, 0 );
  base = -1;
  return result;
}

int scope_check_active()

{
  unsigned char b;

  b = inb( base + SPP_STAT );

  //isolate bit 4
  b = b & 0x10;

  if( b == 0x10 ) 
    return 1;
  else
    return 0;
}


int scope_check_ioflag()

{
  unsigned char b;
  b = inb( base + SPP_STAT );

  //isolate bit5
  b = b & 0x20;

  //ioflag is active low, so this is the reverse of what you would expect

  if( b == 0x20 )
    return 0;

  else 
    return 1;
}

int scope_check_live()

{
  unsigned char b;

  b = inb( base + SPP_STAT );

  //isolate bit 3;
  b = b & 0x08;

  //again, this is active low

  if( b == 0x08 ) 
    return 0;
  else
    return 1;
}


int clear_EPP_timeout();


int scope_read_chars ( char * buffer, int size )

{
  int i=0;
  int result = 0;

  //set IO Active low, AC1 low
  outb( 0x73,base + EPP_DATA );             // 0111 0011
  
  //check scope for ready

  if( ( scope_check_live() != 0 ) || ( scope_check_ioflag() != 0 ) ) {
    outb( SCOPE_IDLE, base+ EPP_DATA );
    printf( "error, scope was live\n" );
    return -1;
  }

  //Now we must set load address mode to reset the counter by setting AC1 high
  //then reenter advance address mode

  outb( 0x7b, base + EPP_DATA ); // 0111 1011
  usleep(1);
  outb( 0x73, base + EPP_DATA ); // 0111 0011
  usleep(1);

  //Activate the device

  outb( 0x72, base+EPP_DATA );   //0111 0010

  for( i=0; i<10; i++ );

  outb( 0x73, base+EPP_DATA );   //0111 0011


  //the scope is ready, so now we can read in data

  i=0;
  for( i=0; i<size; i+=4) {
    unsigned long *temp;
    temp= (unsigned long *) &buffer[i];
    *temp= inl( base + EPP_DATA );
    if( check_EPP_timeout() ) {
      //printf( "EPP Timeout occured on read %d\n",i );      
      clear_EPP_timeout();
      result = -1;
    }
  }


  //Done reading, now deactivate and set IO active high again

  outb( 0x74, base + EPP_DATA );           //0111 0100
  outb( SCOPE_IDLE, base + EPP_DATA );     //0111 0101

  return result;
}

int scope_set_live()
{
  int result;

  if( scope_check_active() )
    return -1;

  outb( 0x55, base + EPP_DATA );        //0101 0101       
  //pause for 1 microsecond

  usleep( 1 );

  outb( SCOPE_IDLE, base + EPP_DATA );   // 0111 0101

  usleep( 1 );

  result = scope_check_live();
  if( result == 1 )
    return 0;
  else
    return -1;
}

int scope_set_recall_last()
{
  int result;

  if( scope_check_active()  )
    return -1;
  outb( 0x65, base + EPP_DATA );                // 0110 0101
  //pause for 1 microsecond
  usleep( 1 );
  outb( SCOPE_IDLE, base + EPP_DATA );   // 0111 0101
  usleep( 1 );
  result = scope_check_live();

  if( result == 0 )
    return 0;
  else
    return -1;
}

int scope_set_hold_next()

{
  int result;

  if( scope_check_active() )
    return -1;
  outb( 0x35, base + EPP_DATA );                // 0011 0101
  //pause for 1 microsecond
  usleep( 1 );
  outb( SCOPE_IDLE, base + EPP_DATA );   // 0111 0101
  usleep( 1 );
  result = scope_check_live();

  if( result == 0 )
    return 0;
  else
    return -1;
}

int check_EPP_timeout()
{
  unsigned char b;
  b = inb( base + SPP_STAT );
  b = b & 0x01;                     //isolate bit 0

  if( b )
    return 1;
  else
    return 0;
}

int clear_EPP_timeout()

{

  unsigned char b;
  //clear the EPP timeout bit on the status register
  //this code is adapted from parport_pc.c

  inb( base + SPP_STAT );                 
  b=inb( base + SPP_STAT );            //read in the status register

  outb( b & 0xfe, base + SPP_STAT );    //write bit 0 to 0, leave the rest the same


  if( check_EPP_timeout() ) {
    printf( "Could not reset EPP timeout bit on port 0x%x\n", base );
    return -1;
  }
  return 0;
}

int clear_EPP_port()

{

  //write 0x04 to control port

  outb ( 0x04,base+SPP_CTRL );
 

  clear_EPP_timeout();

  outb( SCOPE_IDLE, base + EPP_DATA );
  return 0;
}

int convert_data( unsigned char* in_buf, int* out_buf, int size )

{
  int i;
  unsigned char b1;
  unsigned char b2;
  if( size % 2 == 1 )
    return -1;

  for( i=0; i<size; i = i+2 ) {
    b1 = ~in_buf[i];
    //    printf( "%x -> %x,  ", in_buf[i], b1 );
    b2 = ~in_buf[i+1]; 
    //printf( "%x -> %x,  ", in_buf[i+1], b2 );
    b2 = b2 & 0x0f;        //isolates the least significant nibble
    out_buf[i/2] = b1 + 0x100 * b2;
    //printf( "%x + %x -> %x\n", b2, b1, out_buf[i/2]  );
    if( out_buf[i/2] > 2048 )
      out_buf[i/2] = out_buf[i/2] - 4096;
  }
  return 0;
}

int scope_read_ints ( int* buffer, int size, float* time, char do_norm )

{
  unsigned char cbuff[MAX_DATA_POINTS*4];
  int HORZA, HORZB, VERTA, VERTB;
  int i;
  unsigned char c;

  //  printf("in scope_read_ints do_norm is: %i\n",do_norm);
  if( size > MAX_DATA_POINTS*2 ) {
    printf( "scope_PP_handler: size of buffer exceeds maximum number of data points\n" );
    return -1;
  }

  if( size < MIN_DATA_POINTS*2 && do_norm > 0 ) {
    printf( "scope_PP_handler: data buffer is too small to read normalizing data\n" );
    return -1;
  }


  if( scope_read_chars( cbuff, size*2 ) < 0 ) {
    printf( "#scope_read_chars unsuccessfull\n" );
    return -1;
  }

  if( convert_data( cbuff, buffer, size*2 ) < 0 ) {
    printf( "#convert_data unsuccessfull\n" );
    return -1;
  }

  /*
   *  Now decode the normalizing data
   */

  if( do_norm > 0 ) {
    HORZA = 0;
    HORZB = 0;
    VERTA = 0;
    VERTB = 0;

    //first get the HORZ data

    for( i=0; i<16; i++ ) {
      c = ~cbuff[ 2 * (8*i) + 1];
      HORZA += ((c>>4)%2) << i;

      c = ~cbuff[2 * (8*i+1) + 1];
      HORZB += ((c>>4)%2) << i;
    }

    //now get the VERT data

    for( i=0; i<16; i++ ) {
      c = ~cbuff[ 2 * (8*i+128) + 1];
      VERTA += ((c>>4)%2) << i;

      c = ~cbuff[2 * (8*i+129) + 1];
      VERTB += ((c>>4)%2) << i;
    }

    //printf( "Collected normalizing data HA: %x HB: %x, VA: %x, VB: %x\n", HORZA, HORZB, VERTA, VERTB );

    if( VERTA != VERTB ) {
      printf( "Error, channels not on equal voltage scale\n" );
      return -1;
    }


    if( HORZA != HORZB ) {
      printf( "Error, channels not on equal time scale\n" );
      return -1;
    }

    if( HORZA > 4096 ) {
      printf( "Data not normalized - setting time_per_point to -1\n" );
      //      return -1; // removed for single point trigger mode.
      *time = -1.;
    }
    else{
    *time = pow( 2, ((HORZA>>8)%4) ) 
          / pow( 2, ((HORZA>>6)%4) ) 
          * pow( 10, ((HORZA>>4)%4)*-3 ) 
          * pow( 10, ((HORZA>>2)%4) ) 
          / pow( 10, (HORZA%4)*-1 );
    }
    //     printf( "time per point is %0.9f, *1e6 is: %f \n", *time,*time * 1e6 );

  }


  return 0;
}














