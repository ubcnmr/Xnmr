// this will compile acq to not talk to any hardware:  
// it will wait for interrupts !!!
// it will also generate "simulated" data
//
#define NOHARDWARE
#define NO_PORT_INTERRUPT
#define NO_RT_SCHED



// we should read the first two of these out of /proc/pci

#define PULSE_PORT 0xb000
#define DSP_PORT 0xb400
#define AD9850_PORT 0x378

//#define MAX_ZERO_REDOS 20
#define MAX_ZERO_REDOS 0
// disable max_zero_redos while auto phase sync feature removed (July 11, 2002 CM)


/*  acq.c
 *
 * Xnmr Software Project
 * * Main controlling process for NMR experiments
 * UBC Physics
 *
 * This version uses a message queue for communication between the acq and pprog
 * processes and implements all error checking scenarios except timeouts. - later included  
 * It is also the first reliable version.
 *
 * Mar 2: Made this process a "real time" priority for the scheduler.  Note that it
 *        is so fast that if two signals are send to the ui in the same cycle 
 *        without a pause in between them, only the second signal will be received
 *        because the second signal meaning will overwrite the first before the first
 *        signal has been processed by the UI.  Perhaps a future versions should use
 *        message queues for this communication as well.
 *
 * written by: Scott Nelson, Carl Michal
 *
 * April, 2000
 *
 * March 2001, updates for using AD6640, AD6620 digitizer and dsp CM
 *
 * July 2001 Block averaging added CM
 *
 *
 * Feb 2002 Attempting deal with all zeros from receiver - redo a scan. CM
 *
 *
 *
 */

#include <math.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/mman.h>  //for mlockall, etc.
#include <sys/stat.h>  //for mkdir, etc
#include <sched.h>
#include <string.h>
#include <glib.h>
#include <syslog.h> // for system logger.
#include <sys/io.h>
#include "shm_data.h"
#include "shm_prog.h"          //indirectly dependant on h_config.h
#include "p_signals.h"
#include "acq.h"
#include "dsp.h"
#include "/usr/share/Xnmr/config/pulse_hardware.h"    //indirectly dependant on h_config.h
#include "param_utils.h"
#include "ad9850.h"

/*
 * possible states of variable done
 */

#define NOT_DONE 0
#define NICE_DONE 1
#define ERROR_DONE -1
#define KILL_DONE -2

/*
 *  Global variables
 */

struct data_shm_t* data_shm;
struct prog_shm_t* prog_shm;        //These are shared memory structures and must be initialized

int data_shm_id;
int prog_shm_id;
int msgq_id;
int errcode = 0;
volatile int running = 0;
int accum_phase;
char msg_wait_flag = 0;

volatile char done = NOT_DONE; 
int euid,egid;

void cant_open_file(){
  int result;

  done = ERROR_DONE;
  result = seteuid(euid);
  if (result != 0) perror("acq:");
  result = setegid(egid);
  if (result != 0) perror("acq:");
  send_sig_ui( ACQ_FILE_ERROR );
  running = 0;

}

int init_msgs()

{

  struct msgbuf message;
  int i = 0;

  msgq_id = msgget( MSG_KEY, IPC_CREAT|0660 );     

  if( msgq_id < 0 ){
    tell_xnmr_fail();
    shut_down();
  }


  // empty the message queue

  while( msgrcv ( msgq_id, &message, 1, 0, IPC_NOWAIT ) >= 0 )
    i++;

  if( i> 0 ) {
    //printf( "acq removed %d messages from the message queue\n", i );
  }

  return msgq_id;
}

int init_shm()
{

  int existed = 0; // stores if prog shared mem already existed

  data_shm = 0;  prog_shm = 0;
  data_shm_id = shmget( DATA_SHM_KEY, sizeof( struct data_shm_t ),0);

  if (data_shm_id < 0 ) perror("acq: error getting data shm");
  printf("acq: data_shm_id: %i\n",data_shm_id);

  prog_shm_id = shmget( PROG_SHM_KEY, sizeof( struct prog_shm_t ), IPC_CREAT|IPC_EXCL|0660); // fails if it already existed
  if (prog_shm_id  < 0) {  // call failed for some reason
    existed = 1;
    prog_shm_id = shmget( PROG_SHM_KEY, sizeof( struct prog_shm_t ), IPC_CREAT|0660);
  }
  else{
    shmctl(prog_shm_id,SHM_LOCK,NULL);
 }
    printf("data_shm_id: %i, prog_shm_id: %i\n",data_shm_id,prog_shm_id);


  if( data_shm_id < 0 || prog_shm_id < 0 ) {
    printf( "acq: Error getting shared memory segments\n" );
    printf("about to call tell_xnmr_fail\n");
    tell_xnmr_fail();
    shut_down();
  }

  shmctl(data_shm_id,SHM_LOCK,NULL); // Xnmr can't do it, isn't root

  data_shm = (struct data_shm_t*) shmat( data_shm_id, (char*)data_shm ,0 );
  prog_shm = (struct prog_shm_t*) shmat( prog_shm_id, (char*)prog_shm ,0 );

  if( (int)data_shm == -1 || (int)prog_shm == -1 ) {
    perror( "acq: Error attaching shared memory segments" );
    //    data_shm = NULL;
    //    prog_shm = NULL;
    tell_xnmr_fail();
    shut_down();
  }

  if (existed == 0){ // we just created prog_shm
    printf("acq: created prog_shm copying %s into version\n",PPROG_VERSION);
    strncpy(prog_shm->version,PPROG_VERSION,VERSION_LEN);
  }
  else{
    printf("acq: found %s in prog_shm version\n",prog_shm->version);
    if (strcmp(prog_shm->version,PPROG_VERSION) != 0){
      printf("acq: PPROG_VERSION mismatch\n");
      tell_xnmr_fail();
      shut_down();
    }
  }

  printf("acq: found %s in data_shm version\n",XNMR_ACQ_VERSION);
  if (strcmp(data_shm->version,XNMR_ACQ_VERSION) != 0){
    printf("acq: XNMR_ACQ_VERSION mismatch\n");
    printf("data_shm says %s, I say: %s\n",data_shm->version,XNMR_ACQ_VERSION);
    tell_xnmr_fail();
    shut_down();
  }
  

  if( data_shm->acq_pid > 0 ) {
    printf( "acq is already running\n" );
    tell_xnmr_fail();
    exit(1);
  }

  //Lock the memory pages as securely as possible

  mlockall( MCL_FUTURE );

  data_shm->acq_pid = getpid();

  return 0;
}

int send_sig_ui( char sig )

{
  pid_t pid;

  pid =  data_shm->ui_pid;
  data_shm->acq_sig_ui_meaning = sig;

  if( pid > 0 ){
    //    printf("in acq, sending a signal to Xnmr on pid %i\n",pid);
    kill( pid, SIG_UI_ACQ );
    return 0;
  }
  return -1;  
  
}

void ui_signal_handler()
    
{
  struct msgbuf message;
  //  printf( "acq: ui_signal_handler invoked\n" );

  switch( data_shm->ui_sig_acq_meaning ) {
    
  case ACQ_START:
    //    printf( "acq received signal ACQ_START\n" );
    data_shm->ui_sig_acq_meaning = NO_SIGNAL;
    if( running == 0) 
      run();
    else
      printf( "acq: can't start, already running\n" );
    break;

  case ACQ_STOP:
    //    printf( "acq received signal ACQ_STOP\n" );
    data_shm->ui_sig_acq_meaning = NO_SIGNAL;
    //    data_shm->mode = NO_MODE;  //this prevents last accumulation - don't do it
    done = NICE_DONE;
    break;

  case ACQ_KILL:
    //    printf("acq: got kill signal, running is: %i\n",running);
    //    printf("msg_wait_flag: %i\n",msg_wait_flag);
    ph_clear_EPP_port(); // stop the hardware immediately, if its allocated
    // printf("cleared EPP port\n");
    data_shm->ui_sig_acq_meaning = NO_SIGNAL; 

    done = KILL_DONE;

    // if run() is sleeping, waiting for pulse program, wake it up.
    if (msg_wait_flag == 1){ 

      message.mtext[0] = P_PROGRAM_ERROR;
      message.mtype = P_PROGRAM_READY;
      msgsnd ( msgq_id, &message, 1, 0 );
    }

    break;
  default:

    break;
  }
  //  printf("returning from acq's ui sig handler\n");
  return;
}

int init_signals()
{
  sigset_t sigset;
  struct sigaction sa1;
  struct sigaction sa2;


  //  printf("in acq init_signals\n");



  signal( SIGINT, shut_down  );
  signal( SIGTERM, shut_down );

  sigemptyset( &sigset );
  sa1.sa_handler = ui_signal_handler;
  sa1.sa_mask = sigset;
  sa1.sa_flags = SA_NOMASK; //This allows the signal handler to be interrupted by itself  
  sa1.sa_restorer = NULL;
  
  sigaction( SIG_UI_ACQ, &sa1, &sa2  );

  sigemptyset(&sigset);
  sigaddset(&sigset,SIG_UI_ACQ);
  sigaddset(&sigset,SIGINT);
  sigaddset(&sigset,SIGTERM);


  //now that signal handlers are set up, unblock the ones we care about.
  sigprocmask(SIG_UNBLOCK,&sigset,NULL);


  return 0;

}


int wait_for_pprog_msg(  )

{

  struct msgbuf message;
  int result;


  //  printf( "acq: retrieving message\n" );
  msg_wait_flag = 1;
  result = msgrcv ( msgq_id, &message, 1, P_PROGRAM_READY, MSG_NOERROR );
  msg_wait_flag = 0;

  /*   if(   prog_shm->prog_ready != READY)
       printf("acq: got a message, but shm doesn't say ready\n");;  */
     
  if( result<0 )
    printf( "Acq received a bad message\n" );

  //Now that we have received a real message, stop the timeout timer and check to see what this signal is

  switch ( message.mtext[0] )
    {
    case P_PROGRAM_READY :
      return P_PROGRAM_READY;
      break;
    case P_PROGRAM_ERROR:
    case P_PROGRAM_PARAM_ERROR:
    case P_PROGRAM_INTERNAL_TIMEOUT:
    case P_PROGRAM_RECOMPILE:
      errcode = message.mtext[0];
      //      printf("acq: got message that pulse program had an error\n");
      break;
    default:
      printf( "acq received an unusual message %ld\n", message.mtype );
      break;
    }

  return -1;
}


int start_pprog()

{
  pid_t pid;
  uid_t ruid;
  gid_t rgid;

  struct msgbuf message;
  int result;

  FILE *fs;
  char s[PATH_LENGTH];

  pid = fork();

  if( pid == 0 ) {  //start the pulse program

      data_shm->pprog_pid = getpid();



    /* don't want to be root inside the pulse program - want to inherit scheduling, but no
       special permissions */
      // hmm, this won't work though, because lock doesn't stay across exec
      mlockall(MCL_FUTURE);

    ruid=getuid();
    euid=geteuid();
    rgid=getgid();
    egid=getegid();
    //        printf("real: %i, eff: %i\n, setting effective uid to %i\n",ruid,euid,ruid);

    //    seteuid(ruid);  // these two will give up root privileges.
    //    setegid(rgid);

    path_strcpy(s,getenv("HOME"));
    path_strcat(s,"/Xnmr/prog/");
    path_strcat(s,data_shm->pulse_exec_path);

    fs = fopen(s,"r");
    if (fs == NULL){
      path_strcpy(s,"/usr/share/Xnmr/prog/");
      path_strcat(s,data_shm->pulse_exec_path);
      fs = fopen(s,"r");
    }

    if (fs != NULL){
      fclose(fs);
       //      printf("launching pprog using: %s\n",s);
      execl( s, NULL );
    }

    // shouldn't return from here
    printf( "failed to launch pulse program\n" );


  
    // let acq know we have a problem
    message.mtype = P_PROGRAM_READY;
    message.mtext[0] = P_PROGRAM_ERROR;
    
    //    data_shm->pprog_pid = -1;  // don't do this now - do it in pprog_post_timeout

    result=msgsnd ( msgq_id, &message, 1, 0 );
    if (result == -1) perror("pulse.c:msgsnd");
    
    exit(1);
  }

  //printf( "acq: starting pulse program: %s on pid: %d\n", data_shm->pulse_exec_path, (int)pid );

  return (int) pid;
}

int accumulate_data( int* buffer )

{
  int i;


  if (data_shm->npts > MAX_DATA_POINTS){
    printf("acq: accumulate_data, npts > MAX_DATA_POINTS, should NEVER HAPPEN\n");
    data_shm->npts = MAX_DATA_POINTS;
  }
  //data does not accumulate in repeat mode
  if( data_shm->mode == REPEAT_MODE ) {
    //{
    for( i=0; i<data_shm->npts*2; i++ )
      data_shm->data_image[i] = 0;
  }

  switch( accum_phase ) 
    {
    case PHASE0:
      for( i=0; i<data_shm->npts*2; i += 2 ) {
	data_shm->data_image[ i ] += buffer[i];
	data_shm->data_image[ i+1 ] += buffer[i+1];
      }
      return 0;
      break;

    case PHASE90:
      for( i=0; i<data_shm->npts*2; i += 2 ) {
	data_shm->data_image[ i ] -= buffer[i+1]; 
	data_shm->data_image[ i+1 ] += buffer[i];
      }
      return 0;
      break;

    case PHASE180:
      for( i=0; i<data_shm->npts*2; i += 2 ) {
	data_shm->data_image[ i ] -= buffer[i]; 
	data_shm->data_image[ i+1 ] -= buffer[i+1];
      }
      return 0;
      break;

    case PHASE270:
      for( i=0; i<data_shm->npts*2; i += 2 ) {
	data_shm->data_image[ i ] += buffer[i+1]; 
	data_shm->data_image[ i+1 ] -= buffer[i];
      }
      return 0;
      break;

      default:
	break;
    }
  printf( "acq: invalid acquisition phase: %i\n",accum_phase );
  return -1;
}


void post_pprog_timeout()
{
  int pid;
  pid = data_shm->pprog_pid;
  
  //  printf("in post_pprog_timeout, pid: %i\n",data_shm->pprog_pid);
  if( pid>0 ) {
    //    printf( "acq: terminating pulse program\n" );
    kill( pid, SIGKILL ); // if the pulse program was dead, our handler
    // was catching the SIGTERM and running shut_down ??
    //    printf("about to waitpid\n");
    waitpid(pid,NULL,0);
    data_shm->pprog_pid = -1; //carl added
    
  }

  done = ERROR_DONE;
  
  //  printf( "acq: Error, invalid pulse program or timeout occurred\n" );
  if (errcode == 0){
    //    printf("in post_pprog timeout with errcode 0\n");
    send_sig_ui( P_PROGRAM_ERROR );
  }
  else{
    //    printf("in post_pprog_timeout, sending errcode: %i\n",errcode);
    send_sig_ui( errcode );
  }
  errcode = 0;
}

void pprog_ready_timeout()

{
  struct msgbuf message;
  //  printf("acq: in pprog ready timeout, pid: %i\n",data_shm->pprog_pid);
  errcode = P_PROGRAM_ACQ_TIMEOUT;

  /*  don't do this here.  post_pprog_ready_timeout will set done = ERROR_DONE
  pid = data_shm->pprog_pid;
  if( pid>0 ) {
//      printf( "acq: terminating pulse program\n" );
      kill( pid, SIGTERM );
      data_shm->pprog_pid=0;  

  }

//   printf( "acq: Error, invalid pulse program or timeout occurred\n" );
   send_sig_ui( P_PROGRAM_ERROR );
  */

  //Send a mesage to wake up the sleeping run()
  if (msg_wait_flag == 1){

    message.mtext[0] = P_PROGRAM_ERROR;
    message.mtype = P_PROGRAM_READY;
    msgsnd ( msgq_id, &message, 1, 0 );
  }
  return;
}

int run()

{
  int buffer[ MAX_DATA_POINTS*2 ];
  int i;
  //  int j;
  pid_t pid;
  struct msgbuf message;
  struct itimerval time, old;
  int int_fd,result;
  int ruid,euid,rgid,egid;
  double freq;
  int sweep,extra_mult;
  int dgain;
  double dsp_ph;
  char force_setup,reset_data;
  int first_time; // first time is for hitting the load time button inside the pulse programmer, also used to crank up pp priority
  long long current_ppo_time,current_total_time,prev_ppo_time;

  int block_size,dummy_scans,num_dummy;
  static long long *block_buffer= NULL; //static to help prevent mem leaks.  Shouldn't need to be.
  int current_block,old_start_pos=0;
  int redo_zeros=0;


  FILE* fstream;
  char path[PATH_LENGTH],fileN[PATH_LENGTH],fileN2[PATH_LENGTH];
  float f;
  char end_1d_loop=0,end_2d_loop=0;
  int zeros[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  if (block_buffer != NULL){
    printf("acq: on enter, block_buffer was not NULL\n");
    g_free(block_buffer);
    block_buffer = NULL;
  }

  running = 1;
  done = NOT_DONE;

  //First we create the directory for the data
  path_strcpy(path,data_shm->save_data_path);
  make_path( path);
 
 // want to make sure the user has write access to create data file:
 ruid=getuid();
  euid=geteuid();
  rgid=getgid();
  egid=getegid();

  seteuid(ruid);
  setegid(rgid);



  if( mkdir( path, S_IRWXU | S_IRWXG | S_IRWXO ) < 0 ) {
    //    printf("acq: error in dir. creation\n");
    if( errno != EEXIST ) {
      perror( "acq couldn't make dir" );
      cant_open_file();
      return 0;
    }
    else
      //      printf( "already exists\n" );
    errno = 0;
  }



  //Now we create a file containing some parameters
  data_shm->last_acqn = 0;
  data_shm->ct = 0;
  if( data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE) {

    path_strcpy( fileN2, path);
    path_strcat( fileN2, "params" );

    //printf( "creating parameter file: %s\n", fileN2 );
    if (write_param_file(fileN2) == -1) return 0;


    path_strcpy( fileN, path);
    path_strcat( fileN, "proc_params" );
    fstream = fopen( fileN, "w" );
    if (fstream == NULL){
      cant_open_file();
      return 0;
    }
    fprintf(fstream,"PH: %f %f %f %f\n",0.0,0.0,0.0,0.0);
    fprintf(fstream,"FT: %i\n",0);
    fprintf(fstream,"FT2: %i\n",0);
    fclose(fstream);
    
    path_strcpy( fileN, path);
    path_strcat( fileN, "data" );
    
    //We have to empty the file 
    
    fstream = fopen( fileN, "w" );
    if (fstream == NULL){
      cant_open_file();
      return 0;
    }
    fclose( fstream );
    
    
  }
  
  result = seteuid(euid);
  if (result != 0) perror("acq:");
  result = setegid(egid);
  if (result != 0) perror("acq:");
  
  //printf( "starting pprog\n" );
  
  data_shm->acqn_2d = 0 ;
  data_shm->acqn = 0;
  
  // start_pprog used to be here
  
  // open the parallel port interrupt device:
#ifndef NO_PORT_INTERRUPT
  int_fd = open( "/dev/PP_irq0", O_RDONLY );
#endif

#ifndef NOHARDWARE
  i = iopl(3);
  //  printf("just called iopl\n");
  if (i != 0 ){
    perror("error on iopl");
  }
#else
  int_fd=zeros[0];
#endif

  /*
    
// first the old way with text freq
  result=sfetch_double( data_shm->parameters,"sf1",&freq,0);
  if (result == 0){
     freq *= 1e6;
  }
else{
    freq = 20e6;
    printf("acq: no sf1 found, using receiver freq of: %f\n",freq);
  }
  */


  // check to see if we're doing block averaging:

  result = sfetch_int(data_shm->parameters,"block_size",&block_size,0);
  if (result == 0){
    //printf("acq: got block_size: %i\n",block_size);
  }
  else
    block_size = 0;
  
  if (block_size > data_shm->num_acqs || data_shm->num_acqs_2d == 0) block_size = 0;
  
  if (block_size > 0){
    block_buffer = g_malloc(data_shm->num_acqs_2d * data_shm->npts * 2 * sizeof ( long long));
    if (block_buffer == NULL){
      printf("acq: malloc for block_buff failed\n");
      done = ERROR_DONE;
    }
    else {
      for (i = 0;i < data_shm->num_acqs_2d * data_shm->npts *2 ; i++ )
	block_buffer[i]=0;
    }
  }
  current_block = 0;
  data_shm->last_acqn_2d=0;

  sweep = (int) rint(1.0 / data_shm->dwell*1e6);


  /// look for dummy scans:
  result = sfetch_int(data_shm->parameters,"dummy_scans",&dummy_scans,0);
  if (result != 0 ) dummy_scans = 0;
  num_dummy = dummy_scans;
  //  else printf("acq: got %i dummy_scans\n",dummy_scans);



  result = sfetch_int( data_shm->parameters,"dgain",&dgain,0);
  if (result != 0) dgain=0;
  //  printf("acq: got dgain: %i\n",dgain);

  result = sfetch_double( data_shm->parameters,"dsp_ph",&dsp_ph,0);
  if (result != 0) dsp_ph=0.;
  //  printf("acq: got dsp_ph: %f\n",dsp_ph);


  //  ok, set up the DSP

  freq = 21093750.;
  //    freq = 20999999.99;

  force_setup = 0;
  
  if (data_shm->reset_dsp_and_synth == 1){
    //    printf("in acq, got reset_dsp_and_synth flag, resetting\n");
    force_setup = 1;

    data_shm->reset_dsp_and_synth = 0;
  }

#ifndef NOHARDWARE
  i = setup_dsp( sweep,DSP_PORT,freq ,dgain, dsp_ph, force_setup);
#else
  i = 0;
#endif

  // setup_dsp will tell us if we need to start up the 9850.
  if (i == 2) {
#ifndef NOHARDWARE
    reset_ad9850();
    setup_ad9850();
#endif
}

  if ( i == -1 ) {
    printf("acq: Couldn't find filter file\n");
    done = ERROR_DONE;
    send_sig_ui( DSP_FILE_ERROR );
  }
  else if (i == -2){ // some other error?
    printf("acq: error initializing dsp board\n");
    done = ERROR_DONE;
    send_sig_ui(DSP_INIT_ERROR);
  }
  else if (i == -3){
    printf("acq: setup_dsp: dgain too big\n");
    done = ERROR_DONE;
    send_sig_ui(DSP_DGAIN_OVERFLOW);
  }
  


  if (done == NOT_DONE){
#ifndef NOHARDWARE
    i = init_pulse_hardware( PULSE_PORT );
#else
    printf("would have init_pulse_hardware\n");
    i = 0;
#endif
    if( i < 0 ) {
      printf( "acq: Error Initializing pulse programmer hardware\n" );
      done = ERROR_DONE;
      send_sig_ui( PULSE_PP_ERROR );
    }
  }
  
  first_time = 1;

  // so other programs know the spectromter is actually in use.
  fstream=fopen("/var/run/Xnmr_acq_is_running","wb");

  if (fstream != NULL) // in case we're not root.
    fclose(fstream);

  data_shm->time_remaining = 0;

  if (done == NOT_DONE){
    data_shm->force_synth = 1;
    start_pprog();     //This will start the pulse program and calculate the first iteration
  }


  // ok this is where we try to calculate how long the whole thing will take:
  //extra mult counts for all the extra dummy scans in noisy mode.
  //  if (prog_shm->is_noisy == 1){ // can't know this till we calc once!
  if (block_size == 0)
    extra_mult = dummy_scans;
  else
    extra_mult = dummy_scans * ((data_shm->num_acqs + block_size-1) / block_size);
  //  printf("extra_mult = %i, dummy_scans is: %i\n",extra_mult,dummy_scans);
  
  for (i=0;i< data_shm->num_acqs_2d && done== NOT_DONE;i++){
    int j;
    for(j=0;j<2 && done == NOT_DONE ;j++){ /* we go through this twice because sometimes frequencies are set
			 on the first shot and sometimes maybe not... */

      time.it_interval.tv_sec = 0;
      time.it_interval.tv_usec = 0;
      time.it_value.tv_sec = 5;
      time.it_value.tv_usec = 0;
      //      printf("about to call signal for SIGALRM\n");
      signal( SIGALRM, pprog_ready_timeout ); 
      setitimer( ITIMER_REAL, &time, &old );
      
      //      printf("starting to wait for pprog in pre-calc, done = %i\n",done);
      if( wait_for_pprog_msg() <0) 
	  post_pprog_timeout(); //waits till a message comes back from pprog.
      
      if (done < 0){  //this may seem a little weird - reset the timer only if we're done.  Otherwise, we're
	//about to set it again immediately anyway.
	//reset the timer

	time.it_interval.tv_sec = 0;
	time.it_interval.tv_usec = 0;
	time.it_value.tv_sec = 0;
	time.it_value.tv_usec = 0;
	setitimer( ITIMER_REAL, &time, &old );
      }


      data_shm->force_synth = 0; // we only force frequencies on the first run of the dry run
      // and on the first real run (which may be a dummy scan).
      
      
      // so presumably, we now have the pulse program calculated
      //figure how long it is

      if (prog_shm->is_noisy == 0) extra_mult = 0;

      if ( j == 0 )
	data_shm->time_remaining +=  pp_time(); // on the first run through, only keep this time
      else if (j == 1 ){ // assume second run is the same as all the rest.
	data_shm->time_remaining +=  pp_time()*( data_shm->num_acqs-1 + extra_mult );
	if (data_shm->acqn_2d == 0 && prog_shm->is_noisy == 0 ) {  // take account of dummy scans
	  data_shm->time_remaining += pp_time()* dummy_scans;
	}
      }
      else printf("in acq, calc'ing pp length, got j = %i\n",j);
      //printf("after calc'ing: %d, total time is: %f\n",data_shm->acqn_2d,data_shm->time_remaining/20e6);
      
      if (j == 1){ // increment the 2d loop
	data_shm->acqn_2d += 1;
	if (data_shm->acqn_2d == data_shm->num_acqs_2d){
	  data_shm->time_remaining -= (long long) ppo_time(); // we don't wait for the ppo on the last scan.
	  data_shm->acqn_2d = 0;
	  data_shm->force_synth = 1;
	}
      }
      data_shm->acqn = ( j+1 )%2;

      message.mtype = P_PROGRAM_CALC;
      message.mtext[0] = P_PROGRAM_CALC;
      //    printf("acq: about to send message to calc next prog\n");
      msgsnd ( msgq_id, &message, 1, 0 );
      //    printf("acq: sent message to calc next prog\n");
    }
  }

  //  printf("Acq: pre-calc complete\n");

  // this is done above in the pre-calc
  //  data_shm->acqn_2d = 0 ;
  //  data_shm->acqn = 0;


  /*
   *  Start the main loop
   */


  // printf("going into main loop, block_size: %i\n",block_size);
  prev_ppo_time = 0;


  if (prog_shm->is_noisy == 0){
    while( done == NOT_DONE && end_2d_loop == 0 ) {
      //        printf("inside acq's main while loop, acqn_2d: %i of %i\n",data_shm->acqn_2d,
      //   data_shm->num_acqs_2d);
      
      //Reset the shared memory for a new record in 2d dimension
      reset_data=1;
      
      end_1d_loop = 0;
      data_shm->last_acqn_2d = data_shm->acqn_2d; 
      
      
      // this is the one-d loop:
      
      while( done == NOT_DONE && ( end_1d_loop == 0 || data_shm->mode == REPEAT_MODE )  ) {
	//            printf("inside acq's 1d while loop\n");
	
	//we sent out for start already
	
	//start the timer and wait for response from pulse program
	
	if( done >= 0 && data_shm->acqn_2d == 0 && data_shm->acqn == 0 ) {
	  time.it_interval.tv_sec = 0;
	  time.it_interval.tv_usec = 0;
	  time.it_value.tv_sec = 5;
	  time.it_value.tv_usec = 0;
	  //	printf("about to call signal for SIGALRM\n");
	  signal( SIGALRM, pprog_ready_timeout ); 
	  setitimer( ITIMER_REAL, &time, &old );
	  
	  
	}
	
	
	if( done >= 0 ) {
	  //	printf("starting to wait for pprog in real-calc\n");
	  if( wait_for_pprog_msg() <0) 
	    post_pprog_timeout(); //waits till a message comes back from pprog.
	  //      printf("acq: came back from waiting for msg\n");
	}
	accum_phase = prog_shm->phase;
	
	current_ppo_time = ppo_time();
	current_total_time = pp_time();
	
	//reset the timer
	
	time.it_interval.tv_sec = 0;
	time.it_interval.tv_usec = 0;
	time.it_value.tv_sec = 0;
	time.it_value.tv_usec = 0;
	setitimer( ITIMER_REAL, &time, &old );
	
	if( prog_shm->prog_ready == NOT_READY )
	  printf( "acq: warning: pulse program not ready!\n" );
	
	
	//  ok, if this is the first time through, then apparently we heard from the pprog ok, now crank up its priority:
	if (first_time == 1 && done >= 0 )
	  {
	    
#ifndef NO_RT_SCHED
	    struct sched_param sp;
	    int result,priority;
	    
	    result = sched_getparam( data_shm->pprog_pid, &sp );
	    priority = sched_get_priority_max(SCHED_FIFO);
	    sp.sched_priority = priority/2 - 2;  // set it to two less than acq's - shims goes in the middle
	    result = sched_setscheduler(data_shm->pprog_pid,SCHED_FIFO,&sp); // comment out to not
	    //	printf("in acq, set priority of: %i for pprog\n",priority/2);
	    
	    if( result!= 0) 
	      perror("acq: init_sched for pprog");
	    
	    result = sched_getscheduler(data_shm->pprog_pid);
	    if (result != SCHED_FIFO) printf("acq: scheduler of pprog is not SCHED_FIFO: %i\n",result);
#endif
	    data_shm->force_synth = 0;
	    
	    
	  }
	
	// check to see if there was a pulse_program event_error:
	if (done >= 0){
	  if (prog_shm->event_error == 1){
	    printf("acq: got pulse_program event_error\n");
	    done = ERROR_DONE;
	    send_sig_ui(EVENT_ERROR);
	  }      
	  
	  if (prog_shm->got_ppo == 0){
	    printf("acq: no ppo found\n");
	    done = ERROR_DONE;
	    send_sig_ui(PPO_ERROR);
	  }      
	}
	
	
	//download pulse program to pulse programmer hardware
	if( done >= 0 ) {
	  //	printf("acq: about to send to hardware\n");
#ifndef NOHARDWARE
	  i = pulse_hardware_send( prog_shm );
#else
	  i=0;
#endif
	  if( i < 0 ) {
	    printf( "acq: Error downloading pulse program to hardware\n" );
	    done = ERROR_DONE;
	    send_sig_ui( PULSE_PP_ERROR );
	  }
	}  // finished downloading pulse program
	
	
	if (first_time == 1 && done >=0 ){
	  first_time = 0;
	  
#ifndef NOHARDWARE
	  pulse_hardware_load_timer();
#endif
	}
	
	
	// last program is now sent to hardware, fix up variables to go calculate the next one.
#ifdef NOHARDWARE
	for (i=0;i<data_shm->npts*2;i++) buffer[i]=0;
	i = data_shm->acqn+data_shm->acqn_2d*data_shm->num_acqs;
	if (i >= data_shm->npts) i = i% data_shm->npts;
	buffer[i*2] = data_shm->acqn+data_shm->acqn_2d*data_shm->num_acqs+4;
#endif
	
	data_shm->acqn++;
	data_shm->last_acqn = data_shm->acqn; // used for rewrite of param file and by xnmr
	
	if (dummy_scans > 0 && (data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE) ) {
	  data_shm->acqn = 0;
	  data_shm->last_acqn = -dummy_scans + 1;
	}
	
	
	//      printf("acq:just incremented acqn: %li\n",data_shm->acqn);
	
	// next three lines to avoid executing % with block_size = 0
	i = 1;
	if (block_size == 0) i = 0;
      else if (data_shm->acqn % block_size != 0) i = 0;
	if (dummy_scans > 0) i = 0;
	
	if ((data_shm->acqn == data_shm->num_acqs || i == 1    )
	    && (data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE)){
	  
	  
	  if(data_shm->acqn_2d == data_shm->num_acqs_2d -1&& data_shm->acqn == data_shm->num_acqs  )
	    end_2d_loop = 1;
	  
	  end_1d_loop = 1;
	  data_shm->acqn_2d++;
	  
	  if (block_size > 0 && data_shm->acqn_2d == data_shm->num_acqs_2d ){
	    data_shm->acqn_2d =0;
	    current_block += 1;
	  }
	  
	  if (block_size > 0){
	    data_shm->acqn = block_size * current_block;
	    if (data_shm->acqn > data_shm->num_acqs) data_shm->acqn = data_shm->num_acqs; 
	    // should only happen at very end
	  }
	  else
	    data_shm->acqn = 0;
	  
	}
	
      // go calculate the next pulse program.
	if (done >=0 && (data_shm->acqn_2d < data_shm->num_acqs_2d || data_shm->mode == REPEAT_MODE )){
	  //		printf( "acq sending message P_PROGRAM_CALC with acqn, %li acq2d %i\n" ,
	  //			data_shm->acqn,data_shm->acqn_2d);
	message.mtype = P_PROGRAM_CALC;
	message.mtext[0] = P_PROGRAM_CALC;
	//	printf("acq: about to send message to calc next prog\n");
	msgsnd ( msgq_id, &message, 1, 0 );
	//	printf("acq: sent message to calc next prog\n");
	}
	

	do{ // repeating loop for receiver zeros...
	  
	  //start pulse programmer
	  if( done >= 0 ) {
	    //	printf( "acq: starting pulse programmer hardware\n" );
#ifndef NOHARDWARE
	    
	    if (redo_zeros > 0 )  { 
	      reset_ad9850();
	      i = setup_dsp( sweep,DSP_PORT,freq ,dgain, dsp_ph, 1); //force_setup = 1
	      setup_ad9850(); // this should do the resync of the AD9850 with AD6620
	    }
	    
	    i = pulse_hardware_start(0);
#else
	    i=0;
	    printf("didn't start pulse programmer\n");
#endif
	    
	    
	    if( i < 0 ) {
	      printf( "acq: Error starting pulse program\n" );
	      done = ERROR_DONE;
	      if (i == - (TTC_ERROR))
		send_sig_ui (TTC_ERROR);
	      else
		send_sig_ui( P_PROGRAM_ERROR );
	    }
	    
	  }
	  if (redo_zeros > 0) syslog(LOG_WARNING,"Got all zeros, redo_zeros = %i, max is %i\n",redo_zeros,MAX_ZERO_REDOS);
	  //wait for interrupt - note that a return value of -1 indicates that an interrupt was not
	  //received
	  
	  if( done >= 0 ) {
	    //	  printf( "acq: waiting for an interrupt\n" );
#ifndef NO_PORT_INTERRUPT
	    while( read( int_fd, buffer, 1 ) <= 0 && done >=0)
	      {
		//	    perror( "acq: PP_irq read broken" );
	      }
	    //	  printf( "acq: interrupt received\n" );
#endif
	  }
	  //      if (done == ERROR_DONE) printf("acq woken from read to find ERROR_DONE\n");
	  
	  
	  if( done >= 0 ) { // update time remaining, and read in data.
	    //	  printf( "acq doing acquisition %u of dimension %u\n", data_shm->acqn, data_shm->acqn_2d );
	    
	    //	printf("%f %f %f\n",current_total_time/20e6,current_ppo_time/20e6,prev_ppo_time/20e6);
	    if (redo_zeros == 0){
	      data_shm->time_remaining -= ( (long long)current_total_time - current_ppo_time + prev_ppo_time);
	      prev_ppo_time = current_ppo_time;
	    } // don't subtract off the time if we're repeating for all zeros.
	    
	    //	printf("time remaining: %f,\n",data_shm->time_remaining/20e6);
#ifndef NOHARDWARE
	    /*	{    
	    // this was a performance checking gizmo to test port speed.
	    struct timeval start_time,end_time;
	    struct timezone tz;
	    float d_time;
	    
	    gettimeofday(&start_time,&tz);
	    */
	    
	    i = read_fifo(data_shm->npts,buffer,STANDARD_MODE);
	    /*
	      gettimeofday(&end_time,&tz);
	      
	      d_time=(end_time.tv_sec-start_time.tv_sec)*1e6+(end_time.tv_usec-start_time.tv_usec);
	      printf("took: %f us to read in data from fifo\n",d_time); 
	      } */
	  
#else
	    i= data_shm->npts*2;
	    //	  printf("didn't read data from dsp\n");
#endif	
	    
	    if( i != data_shm->npts*2  ) {
	      printf( "acq: Error reading data from FIFO\n" );
	      done = ERROR_DONE;
	      send_sig_ui( FIFO_READ_ERROR );
	    }
	    
#ifndef NOHARDWARE
	    if (memcmp(buffer,zeros,20*4) == 0){ // then darn, all we got back from the receiver are 0's.
	      printf("acq: got first 10 points as all zeros\n");
	    // look the hard way
	      for( i = 0 ; i < data_shm->npts*2 ; i++ )
		if (buffer[i] != 0){
		  printf("acq: in further zero checking, found pt: %i\n",i);
		  i = data_shm->npts*2+5;
		}
	      if (i != data_shm->npts*2+6 ){ 
		// yes +6 because we added 5 above, then leaving the loop added one more!
		printf("acq: at scan ct: %li, next acqn: %li, acqn2d: %i.  Got zeros from receiver\n",data_shm->ct,data_shm->acqn,data_shm->acqn_2d);
		//	      printf("last successful scan was the one before this one\n");
		redo_zeros += 1;
	      }
	      else redo_zeros = 0; // not all 0's
	    }
	    else redo_zeros = 0; //not all 0's
	    
#endif

	    
	  } // update time remaining and read in data.
	}while ( redo_zeros > 0 && redo_zeros < MAX_ZERO_REDOS+1 && done >= 0);      
	
	if (redo_zeros >= MAX_ZERO_REDOS+1){
	  done = ERROR_DONE;
	  send_sig_ui(FIFO_ZERO_ERROR);
	}	
	
	if( done >= 0 ){
	  if (reset_data == 1){
	    reset_data = 0;
	    // either set the shared memory to 0, or recall the correct block from the block buffer
	    if (block_size > 0){
	      for (i=0;i<data_shm->npts*2;i++)
		data_shm->data_image[i] = block_buffer[i + data_shm->last_acqn_2d * data_shm->npts*2];
	    }
	    else{
	      for( i=0; i<data_shm->npts*2; i++ )
		data_shm->data_image[i] = 0;
	    }
	  }
	  accumulate_data( buffer );
	  data_shm->ct += 1;
	}
	
	if (dummy_scans > 0 && (data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE) ) {
	  dummy_scans--;
	  reset_data = 1;
	  //	  data_shm->ct -= 1;
	}      
	
	//signal UI that new data is ready 
	
	//      printf("at end of 1d loop, done = %i, acqn: %li\n",done,data_shm->acqn);
	if( done >= 0 ) send_sig_ui( NEW_DATA_READY );
	//      printf("coming up to end acq's 1d loop, just told ui, new data\n");
#ifdef NO_PORT_INTERRUPT
	{	    if (end_2d_loop == 1 && end_1d_loop == 1)
	  {
	    printf("not sleeping\n");
	  }
	else{
	  //	      printf("in acq with NO_PORT_INTERRUPT, sleeping %f s\n",pp_time()*1.0/CLOCK_SPEED);
	  usleep( pp_time()/(CLOCK_SPEED/1000000));
	}
	}
#endif
	
      
      } //End of 1d while loop
      //      printf("just out of acq's 1d loop\n");
      
      
      /*
       *  This is where the data for this set of acqusitions should be saved before
       *  advancing to the next dimension
       */
      // added kill_done in here to not touch file when we hit kill
      if(( data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE) && done != KILL_DONE) {
	
	// for blocks, store the data in the block buffer
	if ( block_size > 0)
	for ( i = 0 ; i < data_shm->npts * 2 ; i++ )
	   block_buffer[data_shm->last_acqn_2d*data_shm->npts *2 +i] = data_shm->data_image[i];
	
	
	//      printf( "acq appending file: %s\n", fileN );
	
	// need to change to fseek 
	fstream = fopen( fileN, "r+" );
	//      printf("seeking to: %i\n",data_shm->last_acqn_2d*data_shm->npts*2*sizeof(float));
	fseek(fstream,data_shm->last_acqn_2d*data_shm->npts*2 * sizeof (float),SEEK_SET);
	for( i=0; i<data_shm->npts*2; i++ ) {
	  f = (float) data_shm->data_image[i];
	  fwrite( &f, 4, 1, fstream );
	}
	
	
	fclose( fstream );
	// rewrite the param file
	write_param_file (fileN2);
      }
      
    }  //End of 2d while loop - normal operation
  }// end of normal operation 

      else{   // doing the noisy thing
	if (data_shm->mode == REPEAT_MODE ){ //simply life a little.
	  dummy_scans = 0;
	  num_dummy = 0;
	  block_size = 0;
	}
	while( done == NOT_DONE && end_2d_loop == 0 ) {
	  //        printf("inside acq's main while loop, acqn_2d: %i of %i\n",data_shm->acqn_2d,
	  //   data_shm->num_acqs_2d);
      

	  // for noisy, we reset completely each 2d block.


#ifndef NOHARDWARE
	  ph_clear_EPP_port(); // yes - every block
#else
	  printf("would have cleared pulse hardware\n");
#endif
	

	  //Reset the shared memory for a new record in 2d dimension
	  reset_data=1;
      
	  end_1d_loop = 0;
	  data_shm->last_acqn_2d = data_shm->acqn_2d; 
      


	  //we sent out for start already
	  //start the timer and wait for response from pulse program
	  
	  if( done >= 0 && data_shm->acqn_2d == 0 && data_shm->acqn == 0 ) {
	    time.it_interval.tv_sec = 0;
	    time.it_interval.tv_usec = 0;
	    time.it_value.tv_sec = 5;
	    time.it_value.tv_usec = 0;
	    //	printf("about to call signal for SIGALRM\n");
	    signal( SIGALRM, pprog_ready_timeout ); 
	    setitimer( ITIMER_REAL, &time, &old );
	  }
	
	
	  if( done >= 0 ) {
	    //	printf("starting to wait for pprog in real-calc\n");
	    if( wait_for_pprog_msg() <0) 
	      post_pprog_timeout(); //waits till a message comes back from pprog.
	    //      printf("acq: came back from waiting for msg\n");
	  }
	  accum_phase = prog_shm->phase;
	  
	  current_ppo_time = ppo_time();
	  current_total_time = pp_time();
	  
	  //reset the timer
	  
	  time.it_interval.tv_sec = 0;
	  time.it_interval.tv_usec = 0;
	  time.it_value.tv_sec = 0;
	  time.it_value.tv_usec = 0;
	  setitimer( ITIMER_REAL, &time, &old );
	  
	  if( prog_shm->prog_ready == NOT_READY )
	    printf( "acq: warning: pulse program not ready!\n" );

	  //  ok, if this is the first time through, then apparently we heard from the pprog ok, now crank up its priority:
	  if (first_time == 1 && done >= 0 )
	    {
	      
#ifndef NO_RT_SCHED
	      struct sched_param sp;
	      int result,priority;
	      
	      result = sched_getparam( data_shm->pprog_pid, &sp );
	      priority = sched_get_priority_max(SCHED_FIFO);
	      sp.sched_priority = priority/2 - 2;  // set it to two less than acq's - shims goes in the middle
	      result = sched_setscheduler(data_shm->pprog_pid,SCHED_FIFO,&sp); // comment out to not
	      //	printf("in acq, set priority of: %i for pprog\n",priority/2);
	      
	      if( result!= 0) 
		perror("acq: init_sched for pprog");
	      
	      result = sched_getscheduler(data_shm->pprog_pid);
	      if (result != SCHED_FIFO) printf("acq: scheduler of pprog is not SCHED_FIFO: %i\n",result);
#endif
	      data_shm->force_synth = 0;
	      
	      
	    }
	
	  // check to see if there was a pulse_program event_error:
	  if (done >= 0){
	    if (prog_shm->event_error == 1){
	      printf("acq: got pulse_program event_error\n");
	      done = ERROR_DONE;
	      send_sig_ui(EVENT_ERROR);
	    }      
	    
	    if (prog_shm->got_ppo == 0){
	      printf("acq: no ppo found\n");
	      done = ERROR_DONE;
	      send_sig_ui(PPO_ERROR);
	    }      
	  }
	  
	  
	  //download pulse program to pulse programmer hardware
	  if( done >= 0 ) {
	    //	printf("acq: about to send to hardware\n");
	    old_start_pos = prog_shm->noisy_start_pos; // save this for later!
#ifndef NOHARDWARE
	    i = pulse_hardware_send( prog_shm );
#else
	    i=0;
#endif
	    if( i < 0 ) {
	      printf( "acq: Error downloading pulse program to hardware\n" );
	      done = ERROR_DONE;
	      send_sig_ui( PULSE_PP_ERROR );
	    }
	  }  // finished downloading pulse program
	  
	  
	  if (first_time == 1 && done >=0 ){
	    first_time = 0;
	  }
  
#ifndef NOHARDWARE
	  if ( done >= 0);
	  pulse_hardware_load_timer(); //every block for noisy.
#endif
	  



	// go calculate the next pulse program.- it will be the next acqn_2d for sure.

	  // set last_acqn and last_acqn_2d to be correct.
	  // fake acqn_2d and acqn for the pulse program's benefit.
	  // noisy programs shouldn't care about acqn

	  data_shm->last_acqn_2d = data_shm->acqn_2d; 
	  data_shm->last_acqn = data_shm->acqn;

	  if (data_shm->last_acqn_2d == data_shm->num_acqs_2d-1)
	    data_shm->acqn_2d = 0;
	  else 	  
	    data_shm->acqn_2d++;




	  if (done >=0 && (data_shm->acqn_2d < data_shm->num_acqs_2d )){
	    //		printf( "acq sending message P_PROGRAM_CALC with acqn, %li acq2d %i\n" ,
	    message.mtype = P_PROGRAM_CALC;
	    message.mtext[0] = P_PROGRAM_CALC;
	    //	printf("acq: about to send message to calc next prog\n");
	    msgsnd ( msgq_id, &message, 1, 0 );
	    //	printf("acq: sent message to calc next prog\n");
	  }



	  // get the correct data ready to go if needed
	  if (block_size > 0){
	    for (i=0;i<data_shm->npts*2;i++)
	      data_shm->data_image[i] = block_buffer[i + data_shm->last_acqn_2d * data_shm->npts*2];
	  }
	  else{
	    for( i=0; i<data_shm->npts*2; i++ )
	      data_shm->data_image[i] = 0;
	  }
		



	  
	  //start pulse programmer
	  if( done >= 0 ) {
	    //	printf( "acq: starting pulse programmer hardware\n" );
#ifndef NOHARDWARE
	    
	    i = pulse_hardware_start(0); 
#else
	    i=0;
	    printf("didn't start pulse programmer\n");
#endif
	    
	    if( i < 0 ) {
	      printf( "acq: Error starting pulse program\n" );
	      done = ERROR_DONE;
	      if (i == - (TTC_ERROR))
		send_sig_ui (TTC_ERROR);
	      else
		send_sig_ui( P_PROGRAM_ERROR );
	    } // finished starting pulse programmer
	  }


	  dummy_scans = num_dummy; // reset number of dummy scans each block or increment

	  // this is the one-d loop:

	  while( done == NOT_DONE && ( end_1d_loop == 0 || data_shm->mode == REPEAT_MODE )  ) {
	      //            printf("inside acq's 1d while loop\n");
	


	  
	
#ifdef NOHARDWARE
	// gives some bogus data if we're not talking to hardware
	    for (i=0;i<data_shm->npts*2;i++) buffer[i]=0;
	    i = data_shm->acqn+data_shm->last_acqn_2d*data_shm->num_acqs;
	    if (i >= data_shm->npts) i = i% data_shm->npts;
	    buffer[i*2] = data_shm->acqn+data_shm->last_acqn_2d*data_shm->num_acqs+4;
	    
#endif

	// Figure out where we are in the 1d loop, will we restart or not?
	
	data_shm->acqn++;
	data_shm->last_acqn = data_shm->acqn; // used for rewrite of param file and by xnmr
	
	if (dummy_scans > 0 && (data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE) ) {
	  data_shm->acqn--;
	  data_shm->last_acqn = -dummy_scans + 1;
	}
	
	
	//      printf("acq:just incremented acqn: %li\n",data_shm->acqn);
	
	// next three lines to avoid executing % with block_size = 0
	i = 1;
	if (block_size == 0) i = 0;
	else if (data_shm->acqn % block_size != 0) i = 0;
	if (dummy_scans > 0) i = 0;
	// so i is only if we acqn % block_size == 0 and we're not doing dummy scans


	if ((data_shm->acqn == data_shm->num_acqs || i == 1    )
	    && (data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE)){
	  //	  printf("at end of block size\n");
	  
	  if(data_shm->last_acqn_2d == data_shm->num_acqs_2d -1 && data_shm->acqn == data_shm->num_acqs  )
	    end_2d_loop = 1;
	  
	  end_1d_loop = 1;
	  
	  if (block_size > 0 && data_shm->acqn_2d == 0 ){
	    current_block += 1;
	    //	    printf("incremented current_block\n");
	  }
	  
	  if (block_size > 0){
	    data_shm->acqn = block_size * current_block;
	    if (data_shm->acqn > data_shm->num_acqs) data_shm->acqn = data_shm->num_acqs; 
	    // should only happen at very end
	  }
	  else
	    data_shm->acqn = 0;	  
	}
	//	printf("about to wait for interrupt.\nHave set:\ndummy_scans: %i, acqn: %lu, acqn_2d: %i\n",dummy_scans,data_shm->acqn,data_shm->acqn_2d);
	


	    //wait for interrupt - note that a return value of -1 indicates that an interrupt was not
	    //received
	    
	    if( done >= 0  && check_hardware_oen() == 0) {
	      // if outputs are off, that means we already missed this interrupt, no point in waiting
	      // there is a race here - the interrupt could happen between the last call and when we get
	      // to wait for it.  But checking here helps reduce this.
	      //	  printf( "acq: waiting for an interrupt\n" );
#ifndef NO_PORT_INTERRUPT
	      while( read( int_fd, buffer, 1 ) <= 0 && done >=0)
		{
		  //	    perror( "acq: PP_irq read broken" );
		}
	      //	  printf( "acq: interrupt received\n" );
#endif
	    }
	    //      if (done == ERROR_DONE) printf("acq woken from read to find ERROR_DONE\n");




	    // ok, got the interrupt, as long as we're not going to end the 1d loop, we'll restart.
		
	    if (done >=0 && end_1d_loop == 0){
	      // restart !
#ifndef NOHARDWARE
	      i = pulse_hardware_start(old_start_pos);
#else
	      printf("would have restarted\n");
	      i=0;
#endif
	      if( i < 0 ) {
		printf( "acq: Error starting pulse program\n" );
		done = ERROR_DONE;
		if (i == - (TTC_ERROR))
		  send_sig_ui (TTC_ERROR);
		else
		  send_sig_ui( P_PROGRAM_ERROR );
	      }
	    }

	  
	  
	  
	  if( done >= 0 ) { // update time remaining, and read in data.
	    //	  printf( "acq doing acquisition %u of dimension %u\n", data_shm->acqn, data_shm->acqn_2d );
	    
	    //	printf("%f %f %f\n",current_total_time/20e6,current_ppo_time/20e6,prev_ppo_time/20e6);
	    
	    //	printf("time remaining: %f,\n",data_shm->time_remaining/20e6);
#ifndef NOHARDWARE
	    
	    i = read_fifo(data_shm->npts,buffer,NOISY_MODE);
	    
	  
#else
	    i= data_shm->npts*2;
	    //	  printf("didn't read data from dsp\n");
#endif	
	    
	    if( i != data_shm->npts*2  ) {
	      printf( "acq: Error reading data from FIFO\n" );
	      done = ERROR_DONE;
	      send_sig_ui( FIFO_READ_ERROR );
	    }
	    
#ifndef NOHARDWARE
	    if (memcmp(buffer,zeros,20*4) == 0){ // then darn, all we got back from the receiver are 0's.
	      printf("acq: got first 10 points as all zeros\n");
	    // look the hard way
	      for( i = 0 ; i < data_shm->npts*2 ; i++ )
		if (buffer[i] != 0){
		  printf("acq: in further zero checking, found pt: %i\n",i);
		  i = data_shm->npts*2+5;
		}
	      if (i != data_shm->npts*2+6 ){ 
		// yes +6 because we added 5 above, then leaving the loop added one more!
		printf("acq: at scan ct: %li, next acqn: %li, acqn2d: %i.  Got zeros from receiver\n",data_shm->ct,data_shm->acqn,data_shm->acqn_2d);
		done = ERROR_DONE;
		send_sig_ui(FIFO_ZERO_ERROR);
		//	      printf("last successful scan was the one before this one\n");
	      }
	    }
	    
#endif
	  } // done reading in data
	  //	  if (dummy_scans == 0 && data_shm->acqn == 0){ //only on first scane!
	  if (dummy_scans == 0 ){ 
	    accumulate_data( buffer );
	    data_shm->ct += 1;
	  }
	  
	  
	  if (dummy_scans > 0 && (data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE) ) {
	    dummy_scans--;
	    //	    data_shm->ct -= 1;
	  }      
	  
	  data_shm->time_remaining -= ( (long long)current_total_time - current_ppo_time + prev_ppo_time);
	  prev_ppo_time = current_ppo_time;


	  //signal UI that new data is ready 
	  
	  //      printf("at end of 1d loop, done = %i, acqn: %li\n",done,data_shm->acqn);

	  // only tell ui if ppo time is more than 905ms or we're at the end of a 1d loop.
	  if( done >= 0 && (current_ppo_time > 90e-3*CLOCK_SPEED || end_1d_loop == 1) ) send_sig_ui( NEW_DATA_READY );
	  //      printf("coming up to end acq's 1d loop, just told ui, new data\n");
#ifdef NO_PORT_INTERRUPT
	  {	    if (end_2d_loop == 1 && end_1d_loop == 1)
	    {
	      printf("not sleeping\n");
	    }
	  else{
	    //	      printf("in acq with NO_PORT_INTERRUPT, sleeping %f s\n",pp_time()*1.0/CLOCK_SPEED);
	    usleep( pp_time()/(CLOCK_SPEED/1000000));
	  }
	  }
#endif
	
	  
	  } //End of 1d while loop
	    //      printf("just out of acq's 1d loop\n");
	  //	  printf("out of 1d loop\n");
	    
	    
	    /*
	     *  This is where the data for this set of acqusitions should be saved before
	     *  advancing to the next dimension
	     */
	    // added kill_done in here to not touch file when we hit kill
	  if(( data_shm->mode == NORMAL_MODE || data_shm->mode == NORMAL_MODE_NOSAVE) && done != KILL_DONE) {
	      
	    // for blocks, store the data in the block buffer
	    if ( block_size > 0)
	      for ( i = 0 ; i < data_shm->npts * 2 ; i++ )
		block_buffer[data_shm->last_acqn_2d*data_shm->npts *2 +i] = data_shm->data_image[i];	
	  }
	 
	  //	  printf("writing out data\n");
	  // write out the data
	  fstream = fopen( fileN, "r+" );
	  
	  //      fseek(fstream,data_shm->last_acqn_2d*data_shm->npts*2 * sizeof (float),SEEK_SET);
	  fseek(fstream,data_shm->last_acqn_2d*data_shm->npts*2*sizeof (float), SEEK_SET);
	  
	  for( i=0; i<data_shm->npts*2; i++ ) {
	    f = (float) data_shm->data_image[i];
	    fwrite( &f, 4, 1, fstream );
	  }
	
	  	
	
	  fclose( fstream );
	  write_param_file (fileN2);

	  if (end_2d_loop == 0){ // guarantees we wait at least that long between scans.
	    //	    printf("acq: sleeping for pp_time between 2d blocks\n");
	    usleep( pp_time()/(CLOCK_SPEED/1000000));
	  }

	}  //End of 2d while loop - noisy style
	//	printf("out of 2d loop\n");
	
	
      } // end noisy







  // do some cleaning

  if (block_buffer != NULL){
    g_free(block_buffer);
    block_buffer = NULL;
  }
  unlink("/var/run/Xnmr_acq_is_running");

  //terminate the pulse program

  //  printf("acq: at terminate pp\n");
  pid = data_shm->pprog_pid;
  
  if( pid>0 ) {

    //    printf( "acq: terminating pulse program: pid is %i\n",pid );
      kill( pid, SIGTERM );
      
      // this is a normal pprog termination, causes pulse to call
      // wait for pulse program to terminate
      //      printf("acq:about to wait for child\n");
      wait( NULL );

  }
  //  printf("done killing pp\n");
  //  else printf("acq: not bothering to try to kill pprog, it claims to be dead\n");
 

  //Now manually empty the wait queue of any extra P_PROGRAM_READY messages that could be present
  //if a timeout error occurred

  while( msgrcv ( msgq_id, &message, 1, 0, IPC_NOWAIT ) >= 0 )
    i++;

#ifndef NO_PORT_INTERRUPT
  close( int_fd );
#endif
  //  scope_close_port();
  //  dsp_close_port();  this throws away the port bad...
  free_pulse_hardware();

  //send ACQ_DONE to ui
  
  if( done >= 0 )
    send_sig_ui( ACQ_DONE );
  running = 0;
  return 0;

}

void shut_down()

{
  pid_t c;

  //  printf( "acq starting shut down sequence\n" );

  if( data_shm != NULL ) {

    data_shm->acq_pid = -1;
    c = data_shm->pprog_pid;

   

    if( c > 0 ) {
      printf( "acq: attempting to shut down pprog\n" );
      kill( c, SIGKILL );
      //      printf("acq: about to wait for child(2)\n");
      wait( NULL );
      printf("returned from shutting down pprog\n");
    }

  }

  munlockall();

  free_pulse_hardware();
  //  scope_close_port();
  dsp_close_port();
  close_port_ad9850();
  closelog(); // system logger messages

  //printf( "acq: releasing memory\n" );

  release_mem();

  //remove the message queue

  //printf( "removing message queue\n" );

  msgctl( msgq_id, IPC_RMID, NULL );
  iopl(0);

  printf( "acq main terminated\n" );
  exit(1);
  return;
}


int init_sched()



{


  //  struct timespec tp;

#ifndef NO_RT_SCHED
  struct sched_param sp;
  int priority;
  int result;

  result = sched_getparam(0, &sp );
  priority = sched_get_priority_max(SCHED_FIFO);
   sp.sched_priority = priority/2;
   result = sched_setscheduler(0,SCHED_FIFO,&sp);  // comment out to not crank up
  //  printf("set priority of: %i\n",priority/2);

  if( result!= 0) {
    perror("acq: init_sched");
    return result;
  }

  // result = sched_rr_get_interval(0,&tp);
  //  printf("rr_interval: %li s, %li ns\n",tp.tv_sec,tp.tv_nsec);

  result = sched_getscheduler(0);

  if ( result == SCHED_FIFO) {
    //    printf("confirmed SCHED_FIFO\n");
  }
  else return -1;
#endif
  
  return 0;

}




void release_mem()
{
  // Mark the shared segments for removal - This will only remove the segment
  // once all user processes have detached from it.
  
  shmctl ( prog_shm_id, SHM_UNLOCK, NULL ); // if another Xnmr was running, 
  // the shm won't vanish till it exits, but no need to keep it locked in mem now
  shmctl ( data_shm_id, SHM_UNLOCK, NULL );
 
  shmctl ( prog_shm_id, IPC_RMID, NULL );  
  shmctl ( data_shm_id, IPC_RMID, NULL );

  shmdt( (char*) data_shm );
  shmdt( (char*) prog_shm );
  return;
}


int main(){

  pid_t pid;
  /*
   *  Initialization stuff
   */
#ifdef NO_RT_SCHED
  printf("\n\n********************************\n\n");
  printf("            Acq started with realtime scheduling Disabled\n");
  printf("\n\n********************************\n\n");
#endif
#ifdef NOHARDWARE
  printf("\n\n********************************\n\n");
  printf("            Acq started with NOHARDWARE\n");
  printf("\n\n********************************\n\n");
#endif
#ifdef NO_PORT_INTERRUPT
  printf("\n\n********************************\n\n");
  printf("            Acq started with NO_PORT_INTERRUPT \n");
  printf("\n\n********************************\n\n");
#endif

  init_shm();
  init_signals();
  init_msgs();

  data_shm->reset_dsp_and_synth = 1; // so that first time we try to do an acquisition, it will happen.
#ifndef NOHARDWARE
  init_port_ad9850(AD9850_PORT);
#endif
  init_sched();
  openlog("Xnmr acq",0,LOG_USER); //for system logging of all zeros events.

  /*
   *   Go to sleep and deal with the world through signal handlers
   */

  pid = data_shm->ui_pid;
  data_shm->acq_sig_ui_meaning = ACQ_LAUNCHED;

  if( pid > 0 ) {
    //    printf("acq startup:  sending signal to pid: %i\n",data_shm->ui_pid);
    kill( data_shm->ui_pid, SIG_UI_ACQ );
  }
  //printf( "acq main started\n" );
  
  while(1) {
    pause();
    //    printf( "acq main unpaused\n" );
  }
  //  printf( "acq: quitting main\n" );

  shut_down();
 
  return 0;
}


void tell_xnmr_fail(){
  int pid;

  pid = data_shm->ui_pid;
  printf("acq: ui_pid is: %i\n",pid);
  data_shm->acq_sig_ui_meaning = ACQ_LAUNCH_FAIL;
  if( pid > 0 ) {
    printf("acq: sending fail signal put: %i into sig\n",data_shm->acq_sig_ui_meaning);
    kill( data_shm->ui_pid, SIG_UI_ACQ );
  }

}

int write_param_file( char * fileN){

FILE *fstream;

 fstream = fopen( fileN, "w" );
 if (fstream == NULL){
   cant_open_file();
   return -1;
 }
 fprintf( fstream, "%s\n", data_shm->pulse_exec_path );
 //    printf("doing save from within acq, dwell: %f\n",data_shm->dwell);
 fprintf( fstream, 
	  "npts = %u\nacq_npts = %u\nna = %lu\nna2 = %u\nsw = %lu\ndwell = %f\nct = %lu\n", 
	  data_shm->npts, data_shm->npts,data_shm->num_acqs, 
	  data_shm->num_acqs_2d,(long unsigned int) 
	  (1./data_shm->dwell*1000000),data_shm->dwell,data_shm->ct);
 fprintf( fstream,"ch1 = %c\nch2 = %c\n",data_shm->ch1,data_shm->ch2);

 fprintf( fstream, data_shm->parameters );
 fclose( fstream );

return 0;

}

unsigned long long pp_time(){
  int i;
  unsigned long long how_long=0;

  for (i=0;i< prog_shm->no_events;i++){
    how_long += TIME_OF(i) + 1;
    //  printf("pp_time: how_long: %lld, %f\n",how_long,how_long/20e6);
  }
  return how_long;
}

unsigned long long ppo_time(){
  unsigned long long how_long=0;
  how_long += TIME_OF(prog_shm->no_events-1) + 1;
  //  printf("ppo_time: how_long: %lld, %f\n",how_long,how_long/20e6);
  return how_long;
}

