/* pulse_hardware.c
 *
 * This file specifies function prototype to communicate with the pulse programmer hardware
 * Xnmr software
 *
 * UBC Physics
 * April, 2000
 * 
 * written by: Scott Nelson, Carl Michal
 */
//#define TIMING
#include "param_utils.h"
#include "p_signals.h"
#include "shm_prog.h"
#include "/usr/share/Xnmr/config/h_config.h"
#include "/usr/share/Xnmr/config/pulse_hardware.h"


#include <sys/io.h>   //for ioperm routine
//#include <asm/io.h>   //for inb, outb, etc.
#include <stdio.h>    //for printf
#include <sys/time.h>
#include <unistd.h>

/*
 *  Constants
 */


#define SPP_DATA 0x00
#define SPP_STAT 0x01
#define SPP_CTRL 0x02
#define EPP_ADDR 0x03
#define EPP_DATA 0x04

/*
 *  Global Variables
 */

int ph_base = -1;


int ph_check_EPP_timeout()
{
  unsigned char b;
  b = inb( ph_base + SPP_STAT );
  b = b & 0x01;                     //isolate bit 0

  if( b )
    return 1;
  else
    return 0;
}

int check_hardware_oen()
{ // looks to see if the outputs are enabled (ie, no PPO yet)
  unsigned char b;
  b = inb( ph_base + SPP_STAT );

  // actually, check for ttc, timer and outputs:
  if( (b&0x08) == 0 || (b&0x10) == 0 || (b&0x20) == 0) { 
    return -1;
  } 

  return 0; // if everything's ok, return a zero.
}



int ph_clear_EPP_timeout()

{
  unsigned char b;
  //clear the EPP timeout bit on the status register
  //this code is adapted from parport_pc.c

  if (ph_base >0){
    inb( ph_base + SPP_STAT );                 
    b=inb( ph_base + SPP_STAT );            //read in the status register
    
    outb( b | 0x01, ph_base + SPP_STAT );    // write bit 0 to 1
    outb( b & 0xfe, ph_base + SPP_STAT );    //write bit 0 to 0, leave the rest the same
    
    if( ph_check_EPP_timeout() ) {
      printf( "Could not reset EPP timeout bit on port 0x%x\n", ph_base );
      return -1;
    }
  } else printf("didn't reset EPP_timeout because no port\n");
  return 0;
}

int ph_clear_EPP_port()

{
  if (ph_base >0){
    outb(0x10,ph_base+SPP_CTRL);  // set reset to low
    outb (0x14,ph_base+SPP_CTRL); // set reset high, enable interrupt report
  } else printf("didn't clear EPP port because no port\n");
  return ph_clear_EPP_timeout();
}



int init_pulse_hardware( int port )
{
  int i;

  //check to see if port is already open

  if( ph_base != -1 )
    return -1;
 
  ph_base = port;

  
  i = ioperm( ph_base, 8, 1 );

  if( i<0 ) {
    printf( "Can't get permission to access the port 0x%x\n", ph_base );
    return -1;
  }

  return ph_clear_EPP_port();

}

int pulse_hardware_send( struct prog_shm_t* program )
{
  int chip = 0;
  int event = 0;
  //    unsigned int data;
   unsigned long *lp;
  int byte_count=0;

#ifdef TIMING
  int overhead=0;
  struct timeval start_time,end_time;
  struct timezone tz;
  float d_time;
#endif


      
 
#ifdef TIMING
     gettimeofday(&start_time,&tz);
#endif
  //write to Pulse hardware control port

  for( chip=0; chip<NUM_CHIPS; chip++ ) {
    if (program->chip_clean[chip] == DIRTY ){

      outb( AD0_ADDR, ph_base + EPP_ADDR );
      outb( 0, ph_base + EPP_DATA );
      //    overhead+=2;
      
      outb( AD1_ADDR, ph_base + EPP_ADDR );
      outb( 0, ph_base + EPP_DATA );
      //    overhead+=3;
      outb( chip_addr[chip], ph_base + EPP_ADDR );
      
      //Here we write the data four bytes at a time using 32bit EPP transfers
      
      if( chip < 4 ) {
	for( event=0; event< program->no_events; event+=4 ) {
	  /*	  data = program->prog_image[event][chip] + 
	    ( program->prog_image[event+1][chip] << 8) +
	    ( program->prog_image[event+2][chip] << 16) +
	    ( program->prog_image[event+3][chip] << 24);
	    outl( data, ph_base + EPP_DATA );  */
	  lp = (unsigned long *) &program->prog_image[chip][event];
	  outl (*lp, ph_base+EPP_DATA);
	  byte_count+=4;
	}
      }
      
      else {
	for( event=0; event< program->no_events; event+=4 ) {
	  /*	  data = program->prog_image[event][chip] + 
	    ( program->prog_image[event+1][chip] << 8) +
	    ( program->prog_image[event+2][chip] << 16) +
	    ( program->prog_image[event+3][chip] << 24);
	    outl( ~data, ph_base + EPP_DATA );  */
	  lp = (unsigned long *) &program->prog_image[chip][event];
	  outl (~(*lp), ph_base+EPP_DATA);
	  byte_count+=4;
	}
	
      }
      
      //This is the code for 8 bit EPP transfers
      
      /*************
		    if( chip < 4 ) {
		    for( event=0; event< program->no_events; event+=1 ) {
		    outb( program->prog_image[event][chip], ph_base + EPP_DATA ); 
		    }
		    }
		    
		    else {
		    for( event=0; event< program->no_events; event+=1 ) {
		    outb( ~program->prog_image[event][chip], ph_base + EPP_DATA ); 
		    }
		    
		    }
		    
      ***********/
    } // closes the dirty check
  } // loop through chips.


  outb( AD0_ADDR, ph_base + EPP_ADDR );
  outb( 0, ph_base + EPP_DATA );
  
  outb( AD1_ADDR, ph_base + EPP_ADDR );
  outb( 0, ph_base + EPP_DATA );

  program->downloaded = 1;
  
#ifdef TIMING
    gettimeofday(&end_time,&tz);

  //overhead+=4;
    d_time=(end_time.tv_sec-start_time.tv_sec)*1e6+(end_time.tv_usec-start_time.tv_usec);
    printf("download time: %.0f us\n",d_time);
    printf("downloaded: %i+%i bytes to programmer in %.0f us, rate: %.3f MB/s\n",
	 byte_count,
       	 overhead, d_time,
	 (byte_count+overhead)/d_time);  
#endif
  if( ph_check_EPP_timeout() == 1 ) {
    printf( "EPP timeout occurred on port %x\n", ph_base );
    return -1;
  }

  return 0;
}

int pulse_hardware_load_timer()
{

  outb( AD0_ADDR, ph_base + EPP_ADDR );
  outb( 0, ph_base + EPP_DATA );
  
  outb( AD1_ADDR, ph_base + EPP_ADDR );
  outb( 0, ph_base + EPP_DATA );

  //printf("about to load timer\n");

  outb( CNTRL_ADDR, ph_base + EPP_ADDR );
  outb( LOAD_TIMER, ph_base + EPP_DATA );

  outb( RESET_ROTOR, ph_base + EPP_DATA );
  //outb( RESET_ALL, ph_base + EPP_DATA );

  return 0;
}

int pulse_hardware_start(int start_address)
{
  char b;
  unsigned char ad_byte1,ad_byte2;

  /* start_address is where in the memory we start.  */


  b = inb( ph_base+SPP_STAT );
  /* should get 8 = no TTC_ERROR, (bit on = no error, bit off = error)
               16 = Timer running, bit on =running, maybe/maybe not,
	       32 = Outputs enabled, (bit on = enabled) should be low here */
     
  //     printf( "before start: Status register bits: %d, %d, %d\n", b & 0x08, b&0x10, b&0x20 );

  ad_byte1 = start_address & 255;
  //  printf("pulse_hardware_start, got start_address: %i, first byte: %u, ",start_address,ad_byte1);

  ad_byte2 = start_address  >>8;
  //  printf("second byte2: %u\n",ad_byte2);

  outb( AD0_ADDR, ph_base + EPP_ADDR );
  outb( 0, ph_base + EPP_DATA );
  
  outb( AD1_ADDR, ph_base + EPP_ADDR );
  outb( 0, ph_base + EPP_DATA );
 
  //printf("about to load PPO_ADDR\n");

  outb( PPO_ADDR, ph_base + EPP_ADDR ); //what is this for?
  outb( 255, ph_base + EPP_DATA );

  //Check for TTC Error

  b = inb( ph_base+SPP_STAT ); 
  //  printf( "before start: Status register bits: %d, %d, %d\n", b & 0x08, b&0x10, b&0x20 );
  if( (b&0x08) == 0 ) {
      outb( CNTRL_ADDR, ph_base + EPP_ADDR );
      outb( RESET_ALL, ph_base+EPP_DATA );
      printf( "TTC Error occurred, aborting pulse_hardware_start\n" );
      return -(TTC_ERROR);
  }

  outb( AD0_ADDR, ph_base + EPP_ADDR );
  outb( ad_byte1, ph_base + EPP_DATA );
  
  outb( AD1_ADDR, ph_base + EPP_ADDR );
  outb( ad_byte2, ph_base + EPP_DATA );


  outb( CNTRL_ADDR, ph_base + EPP_ADDR );
  outb( START, ph_base + EPP_DATA );

  //  printf("just wrote start to port\n");

  //Check for TTC Error
  b = inb( ph_base+SPP_STAT );
  /* here we should get 8 + 16 + 32 - no error, timer running, outputs on */
  //printf( "after start: Status register bits: %d, %d, %d\n", b & 0x08, b&0x10, b&0x20 );

  if( (b&0x08) == 0 ) {
    outb( CNTRL_ADDR, ph_base + EPP_ADDR );
    outb( RESET_ALL, ph_base+EPP_DATA );
    printf( "TTC Error occurred, aborting pulse_hardware_start\n" );
    return -(TTC_ERROR);
  } 
  if ( (b&0x10) == 0 ){
    outb( CNTRL_ADDR, ph_base + EPP_ADDR );
    outb( RESET_ALL, ph_base+EPP_DATA);
    printf("started, but timer wasn't running\n");
    return -(TTC_ERROR);
  }
  if ( (b&0x20) == 0 ){
    outb( CNTRL_ADDR, ph_base + EPP_ADDR );
    outb( RESET_ALL, ph_base+EPP_DATA);
    printf("started, but outputs weren't enabled\n");
    return -(TTC_ERROR);
  }

  


  return 0;
}

int free_pulse_hardware()
{
  int result;

  if( ph_base == -1 ) {
    //printf( "no need to free pulse hardware\n" );
    return 0;
  }

  outb( CNTRL_ADDR, ph_base+EPP_ADDR );
  outb( RESET_ALL, ph_base+EPP_DATA );

  //printf( "Freeing pulse_hardware\n" );

  outb(0x00,ph_base+SPP_CTRL);  // set reset to low, disable interrupt reporting
  outb (0x04,ph_base+SPP_CTRL); // set reset high, leave interrupts disabled
  result = ioperm( ph_base, 8, 0 );
  ph_base = -1;
  return result;
}





