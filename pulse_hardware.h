/* pulse_hardware.h
 *
 * This file specifies function prototype to communicate with the pulse programmer hardware
 *
 *  Xnmr software project
 *
 * UBC Physics
 * April, 2000
 * 
 * written by: Scott Nelson, Carl Michal
 */

#include "/usr/share/Xnmr/config/h_config.h"

/*
 *  Pulse hardware addresses
 */

#define CNTRL_ADDR 32
#define AD0_ADDR 34
#define AD1_ADDR 35
#define PPO_ADDR 193



/*
 *  Pulse hardware control port commands
 */

#define RESET_ROTOR 255
#define RESET_ALL 239
#define START 227
#define LOAD_TIMER 237
#define TCET 235
#define OEN 231

/*
 *  Memory chip addresses
 *
 *  The first four chips are the timer
 */


static const char chip_addr[ NUM_CHIPS ] = { 196, 197, 198, 199, 192,
					     128, 129, 130, 131,
					     132, 133, 134, 135,
					     136, 137, 138, 139,
					     140, 141, 142, 143 };

/*
 *  Public Method prototypes
 */


int init_pulse_hardware( int port );
  //This method initializes the EPP Port at the given address

int pulse_hardware_send( struct prog_shm_t* program );
  // Sends a pulse program, the port must be initialized first with init_pulse_hardware

int pulse_hardware_start(int start_address);
  // Sets the pulse programmer running

int free_pulse_hardware();
  // Releases the resources allocated by init_pulse_hardware

int pulse_hardware_load_timer();
  // Sends the pulse programmer a 'load timer' command.
int ph_clear_EPP_port();

int check_hardware_oen();
// checks to see if the outputs are still enabled (haven't missed an interrupt).



