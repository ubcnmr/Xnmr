#define GTK_DISABLE_DEPRECATED
/* Xnmr.c
 *
 * X windows NMR Control center - main program
 * 
 * UBC Physics
 * April, 2000
 * 
 * written by: Carl Michal, Scott Nelson
 */
/*

 'ported' for GTK+-2.0 Oct 27, 2002 CM
    not much to change but:

1)    array box didn't resize correctly,
 replaced:    
    gtk_window_set_policy( GTK_WINDOW( popup_data.win ), 0, 0, 1 );   //This makes the window autoshrink 
 with:
    gtk_window_set_resizable(GTK_WINDOW(popup_data.win),0);
in param_f.c

2) Also in param_f.c, param_spin_pressed type changed to gint and now returns false.  
It was preventing the cursor from landing in the box.

3) in show_2d_popup there was a break missing between i,I and f,F.
4) make_active moved to before do_load call in do_load_wrapper to avoid 
having the active buffer hiding the dialogs.

5) had to change a DIALOG to a POPUP to make it compile.
6) biggest changes in the Makefile, change from gtk-config --cflags and --libs
to pkg-config --cflags gtk+-2.0 and --libs gthread-2.0 gtk+-2.0 

7) changed gtk_accel_group_attach to gtk_window_add_accel_group


Dependencies: 
- gtk+-2.6 at least.
- four1 and spline routines from numerical recipes (included in here)
- the fitting package uses the AT&T PORT routine n2f.  If you don't have it,
  comment out the calls to n2f and ivset in buff.c and remove -lf2c and -lport in the makefile.
  OR, get the port library from AT&T (http://www.bell-labs.com/project/PORT/)
  To compile it on gentoo linux, change the makefile so F77=gcc, also may need
  to add flags -lf2c -lm.


*/
/*
  files that go where:

rm /usr/local/bin/xcomp
rm /usr/local/bin/Xnmr
rm /usr/local/bin/acq
rm /usr/local/bin/Xnmr_preproc

ln -s /usr/src/Xnmr/current/xcomp /usr/local/bin/xcomp
ln -s /usr/src/Xnmr/current/Xnmr /usr/local/bin/Xnmr
ln -s /usr/src/Xnmr/current/acq /usr/local/bin/acq
ln -s /usr/src/Xnmr/current/Xnmr_preproc /usr/local/bin/Xnmr_preproc
chown root /usr/local/bin/acq
chmod u+s /usr/local/bin/acq

rm /usr/local/lib/libxnmr.a
rm /usr/local/lib/libxnmr.so

ln -s /usr/src/Xnmr/current/libxnmr.a /usr/local/lib/libxnmr.a
ln -s /usr/src/Xnmr/current/libxnmr.so /usr/local/lib/libxnmr.so
/sbin/ldconfig

rm /usr/share/Xnmr/include/p_signals.h
rm /usr/share/Xnmr/include/param_utils.h
rm /usr/share/Xnmr/include/shm_data.h
rm /usr/share/Xnmr/include/pulse.h

ln -s /usr/src/Xnmr/current/p_signals.h /usr/share/Xnmr/include/p_signals.h
ln -s /usr/src/Xnmr/current/param_utils.h /usr/share/Xnmr/include/param_utils.h
ln -s /usr/src/Xnmr/current/shm_data.h /usr/share/Xnmr/include/shm_data.h
ln -s /usr/src/Xnmr/current/pulse.h /usr/share/Xnmr/include/pulse.h

rm /usr/share/Xnmr/config/h_config.h
rm /usr/share/Xnmr/config/pulse_hardware.h
rm /usr/share/Xnmr/config/xnmrrc

ln -s /usr/src/Xnmr/current/h_config.h /usr/share/Xnmr/config/h_config.h
ln -s /usr/src/Xnmr/current/pulse_hardware.h /usr/share/Xnmr/config/pulse_hardware.h
ln -s /usr/src/Xnmr/current/xnmrrc /usr/share/Xnmr/config/xnmrrc

  pulse_hardware.h  in /usr/share/Xnmr/config/
  h_config.h        
  xnmrrc

  libxnmr.a         in /usr/local/lib/

  p_signals.h       in /usr/share/Xnmr/include/
  param_utils.h
  pulse.h
  shm_data.h

  xcomp             in /usr/local/bin/
  Xnmr
  acq


  pulse programs    in /usr/share/Xnmr/prog/ or ~/Xnmr/prog/


Other software issues:

1) PP_irq (our kernel module that catches parallel port interrupts and
gives them to user-space) must be installed.  Best to compile it and
drop it into /lib/modules/2.x.y/misc/PP_irq.o then load it with
'insmod PP_irq irq=x' (this is currently done in /etc/rc.d/rc.local).

2) acq creates a file called /var/run/Xnmr_acq_is_running (this is
probably insecure...) when it is actually running an experiment.  This
is so that scripts (cron jobs!)  that might take a lot of cpu time
won't actually run.  To make it work, scripts that cron starts need to
be modified.  For example, in /etc/cron.weekly, makewhatis.cron starts
with: [ -e /var/run/Xnmr_acq_is_running ] && exit 0
which bails out if the file exists.

3) 



*/



#include "xnmr.h"
#include "buff.h"
#include "panel.h"
#include "xnmr_ipc.h"
#include "p_signals.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <getopt.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
/*
 *  Global Variables
 */


GdkColor colours[NUM_COLOURS+EXTRA_COLOURS]; // matches in xnmr.h
GdkGC *colourgc;
phase_data_struct phase_data;
add_sub_struct add_sub;
fitting_struct fit_data;

int phase_npts=0;
GtkWidget *phase_dialog,*freq_popup,*fplab1,*fplab2,*fplab3,*fplab4,*fplab5;

GtkObject *phase0_ad,*phase1_ad;
float phase0,phase1;
float phase20,phase21;
GdkCursor *cursorclock;
char no_acq=FALSE;
GtkWidget *panwindow;

char from_do_destroy_all=0; // global flags to figure out what to do on exit.


extern int main(int argc,char *argv[]);

int main(int argc,char *argv[])
//int MAIN__(int argc,char *argv[])
{

  int i,ic;
  int result,result_shm;
  GtkWidget  *pantable;
  GtkWidget *button,*hbox,*vbox;
  GtkObject *adjust;
  GtkWidget *label;
  char title[UTIL_LEN],command[PATH_LENGTH];
  int width,height;

  //  int timeout_tag;
  //  gpointer timeout_data;

  int ar,longindex;
  struct option cmds[]={
    {
      "noacq",
      0,
      NULL,
      'n'
      }
  }
;

  //first, we have to block SIGUSR1 in case ACQ is already launched and running
  // or just not enable the signals till later?
  //block_signal();

  /* initialize the gtk stuff */
  g_thread_init(NULL); // we don't really use threads, except to catch signals
  gdk_threads_init();
  // mutex to ensure that routines added with idle_add don't collide.
  gtk_init(&argc, &argv);
  /* look for command line arguments  that gtk didn't want*/
  do{
    ar=getopt_long(argc,argv,"n",cmds,&longindex);
    if (ar == 'n') {
      printf("got noacq\n");
      no_acq=TRUE;
    }
  }while (ar !=EOF );

  gtk_rc_parse("/usr/share/Xnmr/config/xnmrrc");
  path_strcpy(command,getenv("HOME"));
  path_strcat(command,"/.xnmrrc");
	 
  //  printf("looking for rc file: %s\n",command);
  gtk_rc_parse( command );
 
  /* initialize my stuff */
  for(i=0;i<MAX_BUFFERS;i++)
    {
    buffp[i]=NULL;
    }



  /*
   *  Start acq, etc
   */



  result_shm=-1;
  if (!no_acq){

    result_shm = xnmr_init_shm();  //this tells us whether acq is already launched


    if (result_shm != 0){                // shared mem exists
      printf("shm already exists, looking for running Xnmr\n");
      if ( data_shm->ui_pid >0 ){   // pid in shm is valid
	path_strcpy(command,"ps auxw|grep -v grep|grep Xnmr|grep ");
	snprintf(&command[strlen(command)],PATH_LENGTH-strlen(command),"%i", data_shm->ui_pid);
	//		printf("using command: %s\n",command);
	ic = system(command); // returns zero when it finds something, 256 if nothing?
	if (ic == 0 ) { // Xnmr is still running
	  no_acq = TRUE; 
	  g_idle_add((GtkFunction) popup_msg_mutex_wrap,"There appears to be another living Xnmr, started noacq");
	  // shmdt((char *)data_shm);  if we don't detach, then we should be able to clone from acq.
	  // not detaching does cause one problem - if we quit the active Xnmr after a second has attached,
	  // can't restart an active session because the shm exists.

	}
	else{ // try a -m on the ps command to show threads
	  path_strcpy(command,"ps auxwm|grep -v grep|grep Xnmr|grep ");
	  snprintf(&command[strlen(command)],PATH_LENGTH-strlen(command),"%i", data_shm->ui_pid);
	  ic = system(command); // returns zero when it finds something, 256 if nothing?
	  if (ic == 0 ) { // Xnmr is still running
	    no_acq = TRUE; 
	    g_idle_add((GtkFunction) popup_msg_mutex_wrap,"There appears to be another living Xnmr, started noacq");
	  }
	}
      }
    }
  }

  //  popup_msg("Caution: new version using Analog Devices DSP receiver");
  if ((strcmp(":0.0",getenv("DISPLAY")) != 0) && strcmp(":0",getenv("DISPLAY")) != 0){
    printf("Your display is set to go elsewhere???\n");
  }
  

	     
  if (!no_acq){ // want to have an acq running

    ic = 0;
    // now see if there is a living acq
    if ( data_shm->acq_pid > 0){ // there is a valid pid
      path_strcpy(command,"ps auxw|grep -v grep|grep [acq]|grep ");
      snprintf(&command[strlen(command)],PATH_LENGTH-strlen(command),"%i",data_shm->acq_pid);
      //      printf("using command: %s\n",command);
      ic = system(command); // returns zero when it finds something, 256 if nothing?
      if (ic == 0 ) { //acq is still alive
	g_idle_add((GtkFunction) popup_msg_mutex_wrap,"There appears to be a running acq");
      }
    }
    if (data_shm->acq_pid <1 || ic != 0){ 
	printf( "Xnmr will start acq\n" );
	data_shm->acq_pid = -1;
	
	init_ipc_signals();


	start_acq();
	/*	if ( wait_for_acq() != ACQ_LAUNCHED ){
	  printf("Acq not launched successfully???\n");
	  g_idle_add((GtkFunction) popup_msg_mutex_wrap,"Trouble starting up acq: started noacq");
	  no_acq = TRUE;
	  } */
    }
  }

  /* build the notebook/panel window */

  panwindow=gtk_window_new( GTK_WINDOW_TOPLEVEL );


  g_signal_connect(G_OBJECT(panwindow),"delete_event",
		     G_CALLBACK (destroy_all),NULL);

  pantable = create_panels( );

  gtk_container_add(GTK_CONTAINER(panwindow),pantable);

  //  gtk_window_set_position(GTK_WINDOW(panwindow),GTK_WIN_POS_NONE);
  gtk_window_set_gravity(GTK_WINDOW(panwindow),GDK_GRAVITY_NORTH_WEST);

  // want size of buffer window to set placement of panel window.
  // so postpone placing this window


  /* create a first buffer */


  path_strcpy(command,getenv("HOME"));
  make_path( command);
  path_strcat(command,"Xnmr/data/");
  result = chdir(command);

  if (result == -1){
    printf("creating directories...");
    path_strcpy (command,getenv("HOME"));
    path_strcat(command,"/Xnmr");
    result = mkdir(command,0755);
    if (result != 0) perror("making ~/Xnmr:");
    path_strcat(command,"/data");
    result = mkdir(command,0755);
    if (result != 0) perror("making ~/Xnmr/data:");
  }


  /* build the add/subtract window */

  add_sub.dialog = gtk_dialog_new();
  label=gtk_label_new("Add/Subtract");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_sub.dialog)->vbox),label,FALSE,FALSE,0);

  gtk_window_set_transient_for(GTK_WINDOW(add_sub.dialog),GTK_WINDOW(panwindow));
  gtk_window_set_position(GTK_WINDOW(add_sub.dialog),GTK_WIN_POS_CENTER_ON_PARENT);


  // The buffer line:
  hbox=gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_sub.dialog)->vbox),hbox,FALSE,FALSE,0);

  label=gtk_label_new("Source 1");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  label=gtk_label_new("Source 2");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  label=gtk_label_new("Destination");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  

  hbox=gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_sub.dialog)->vbox),hbox,FALSE,FALSE,0);

  label=gtk_label_new("Buffer");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);


  add_sub.s_buff1 = gtk_combo_box_new_text();
  //  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_buff1),"line1");
  //  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_buff1),"line2");
  g_signal_connect(G_OBJECT(add_sub.s_buff1),"changed",
		     G_CALLBACK (add_sub_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),add_sub.s_buff1,FALSE,FALSE,2);

  label=gtk_label_new("Buffer");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);


  add_sub.s_buff2 = gtk_combo_box_new_text();
  //  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_buff2),"line1");
  //  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_buff2),"line2");
  g_signal_connect(G_OBJECT(add_sub.s_buff2),"changed",
		     G_CALLBACK (add_sub_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),add_sub.s_buff2,FALSE,FALSE,2);


  label=gtk_label_new("Buffer");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  add_sub.dest_buff = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.dest_buff),"New");
  g_signal_connect(G_OBJECT(add_sub.dest_buff),"changed",
		     G_CALLBACK (add_sub_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),add_sub.dest_buff,FALSE,FALSE,2);


  // the record line:
  hbox = gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_sub.dialog)->vbox),hbox,FALSE,FALSE,0);
  

  label=gtk_label_new("record");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  add_sub.s_record1 = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_record1),"Each");
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_record1),"Sum All");
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_record1),"0");
  add_sub.s_rec_c1 = 1; 
  g_signal_connect(G_OBJECT(add_sub.s_record1),"changed",
		     G_CALLBACK (add_sub_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),add_sub.s_record1,FALSE,FALSE,2);


  label=gtk_label_new("record");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  add_sub.s_record2 = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_record2),"Each");
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_record2),"Sum All");
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.s_record2),"0");
  add_sub.s_rec_c2 = 1; 
  g_signal_connect(G_OBJECT(add_sub.s_record2),"changed",
		     G_CALLBACK (add_sub_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),add_sub.s_record2,FALSE,FALSE,2);


  label=gtk_label_new("record");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  add_sub.dest_record = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.dest_record),"Each");
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.dest_record),"Append");
  gtk_combo_box_append_text(GTK_COMBO_BOX(add_sub.dest_record),"0");
  add_sub.dest_rec_c = 1; 
  g_signal_connect(G_OBJECT(add_sub.dest_record),"changed",
		     G_CALLBACK (add_sub_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),add_sub.dest_record,FALSE,FALSE,2);


  // now the multiplier line:

  hbox = gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_sub.dialog)->vbox),hbox,FALSE,FALSE,0);

  label=gtk_label_new("multiplier");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  
  adjust = gtk_adjustment_new(1.0,-1e6,1e6,0.1,1.,0.);
  add_sub.mult1= gtk_spin_button_new(GTK_ADJUSTMENT(adjust),0,6);
  gtk_box_pack_start(GTK_BOX(hbox),add_sub.mult1,FALSE,FALSE,2);


  label=gtk_label_new("multiplier");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  
  adjust = gtk_adjustment_new(1.0,-1e6,1e6,0.1,1.,0.);
  add_sub.mult2= gtk_spin_button_new(GTK_ADJUSTMENT(adjust),0,6);
  gtk_box_pack_start(GTK_BOX(hbox),add_sub.mult2,FALSE,FALSE,2);

  label = gtk_label_new(" ");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  label = gtk_label_new(" ");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  add_sub.apply = gtk_button_new_with_label("Apply");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_sub.dialog)->action_area),add_sub.apply,TRUE,TRUE,2);
  g_signal_connect(G_OBJECT(add_sub.apply),"clicked",G_CALLBACK(add_sub_buttons),NULL);

  add_sub.close = gtk_button_new_with_label("Close");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_sub.dialog)->action_area),add_sub.close,TRUE,TRUE,2);
  g_signal_connect(G_OBJECT(add_sub.close),"clicked",G_CALLBACK(add_sub_buttons),NULL);

  g_signal_connect( G_OBJECT (add_sub.dialog), "delete_event", G_CALLBACK( hide_add_sub ), NULL );
  // defaults:
  add_sub.shown = 0;


  // now build the fitting window



  fit_data.dialog = gtk_dialog_new();
  gtk_window_set_resizable(GTK_WINDOW(fit_data.dialog),0);
  fit_data.add_dialog = NULL;

  gtk_window_set_transient_for(GTK_WINDOW(fit_data.dialog),GTK_WINDOW(panwindow));
  gtk_window_set_position(GTK_WINDOW(fit_data.dialog),GTK_WIN_POS_CENTER_ON_PARENT);


  label = gtk_label_new("Fitting");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(fit_data.dialog)->vbox),label,FALSE,FALSE,0);

  // the label line:

  hbox=gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(fit_data.dialog)->vbox),hbox,FALSE,FALSE,0);

  label=gtk_label_new("Data Source");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  label=gtk_label_new("Best Fit Destination");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  label=gtk_label_new("Store best fit?");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);




  // buffer line:
  hbox=gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(fit_data.dialog)->vbox),hbox,FALSE,FALSE,0);

  label=gtk_label_new("Buffer");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  fit_data.s_buff = gtk_combo_box_new_text();
  g_signal_connect(G_OBJECT(fit_data.s_buff),"changed",
		   G_CALLBACK(fit_data_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.s_buff,FALSE,FALSE,2);

  label=gtk_label_new("Buffer");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  
  fit_data.d_buff = gtk_combo_box_new_text();
  g_signal_connect(G_OBJECT(fit_data.d_buff),"changed",
		   G_CALLBACK(fit_data_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.d_buff,FALSE,FALSE,2);

  gtk_combo_box_append_text(GTK_COMBO_BOX(fit_data.d_buff),"New");

  fit_data.store_fit = gtk_check_button_new();
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.store_fit,FALSE,FALSE,2);




  // record line
  hbox=gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(fit_data.dialog)->vbox),hbox,FALSE,FALSE,0);

  label=gtk_label_new("record");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  
  fit_data.s_record = gtk_combo_box_new_text();
  //  gtk_combo_box_append_text(GTK_COMBO_BOX(fit_data.s_record),"Each");
  gtk_combo_box_append_text(GTK_COMBO_BOX(fit_data.s_record),"0");
  g_signal_connect(G_OBJECT(fit_data.s_record),"changed",
		   G_CALLBACK(fit_data_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.s_record,FALSE,FALSE,2);


  label=gtk_label_new("record");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);

  fit_data.d_record = gtk_combo_box_new_text();
  //  gtk_combo_box_append_text(GTK_COMBO_BOX(fit_data.d_record),"Each");
  gtk_combo_box_append_text(GTK_COMBO_BOX(fit_data.d_record),"Append");
  gtk_combo_box_append_text(GTK_COMBO_BOX(fit_data.d_record),"0");
  g_signal_connect(G_OBJECT(fit_data.d_record),"changed",
		   G_CALLBACK(fit_data_changed),NULL);
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.d_record,FALSE,FALSE,2);

  label=gtk_label_new("  \t");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);



  // next line:

  hbox=gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(fit_data.dialog)->vbox),hbox,FALSE,FALSE,0);


  label = gtk_label_new("# Components:");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
  


  
  adjust = gtk_adjustment_new(0,0,MAX_FIT,1,1,0);
  fit_data.components=gtk_spin_button_new(GTK_ADJUSTMENT(adjust),0,0);
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.components,FALSE,FALSE,2);
  g_signal_connect(G_OBJECT(fit_data.components),"value_changed",G_CALLBACK(fit_data_changed),NULL);

  label = gtk_label_new("       Enable\nProcess Broadening");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,3);

  fit_data.enable_proc_broad = gtk_check_button_new();
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.enable_proc_broad,FALSE,FALSE,0);
  


  fit_data.start_clicking = gtk_button_new_with_label("Add components");
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.start_clicking,FALSE,FALSE,2);
  g_signal_connect(G_OBJECT(fit_data.start_clicking),"clicked",G_CALLBACK(fitting_buttons),NULL);

  fit_data.precalc = gtk_button_new_with_label("Precalc");
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.precalc,FALSE,FALSE,2);
  g_signal_connect(G_OBJECT(fit_data.precalc),"clicked",G_CALLBACK(fitting_buttons),NULL);


  fit_data.run_fit = gtk_button_new_with_label("Run Fit\nFull Spectrum");
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.run_fit,FALSE,FALSE,2);
  g_signal_connect(G_OBJECT(fit_data.run_fit),"clicked",G_CALLBACK(fitting_buttons),NULL);

  fit_data.run_fit_range = gtk_button_new_with_label("Run Fit\nDisplayed Range");
  gtk_box_pack_start(GTK_BOX(hbox),fit_data.run_fit_range,FALSE,FALSE,2);
  g_signal_connect(G_OBJECT(fit_data.run_fit_range),"clicked",G_CALLBACK(fitting_buttons),NULL);

  fit_data.close = gtk_button_new_with_label("Close");
  gtk_box_pack_end(GTK_BOX(hbox),fit_data.close,FALSE,FALSE,2);
  g_signal_connect(G_OBJECT(fit_data.close),"clicked",G_CALLBACK(fitting_buttons),NULL);

  //  the labels for the start values
  vbox=gtk_vbox_new(TRUE,0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(fit_data.dialog)->action_area),vbox,FALSE,FALSE,0);
  

  hbox=gtk_hbox_new(TRUE,2);
  gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

  label=gtk_label_new("Num");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);

  label=gtk_label_new("center freq");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);

  label=gtk_label_new("amplitude");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);

  label=gtk_label_new("Gaussian width");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);

  label=gtk_label_new("Enable Gaussian");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);

  label=gtk_label_new("Lorentzian width");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);

  label=gtk_label_new("Enable Lorentzian");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);




  gtk_widget_show_all(GTK_WIDGET(GTK_DIALOG(fit_data.dialog)->vbox));
  gtk_widget_show_all(GTK_WIDGET(vbox));

  // start with zero components, let the user choose how to add them.
  fit_data.num_components = 0;
  fit_data.s_rec = 1;
  fit_data.d_rec = 1;

  // but build them all, just don't show them:
  for (i=0;i<MAX_FIT;i++){
    char stt[5];

    fit_data.hbox[i] = gtk_hbox_new(TRUE,2);
    gtk_box_pack_start(GTK_BOX(vbox),fit_data.hbox[i],FALSE,FALSE,0);
    
    sprintf(stt,"%i",i+1);
    label=gtk_label_new(stt);
    gtk_box_pack_start(GTK_BOX(fit_data.hbox[i]),label,FALSE,FALSE,0);
    gtk_widget_show(label);


  //center
    adjust = gtk_adjustment_new(0.0,-1e7,1e7,100,1000,0);
    fit_data.center[i] = gtk_spin_button_new(GTK_ADJUSTMENT(adjust),0.2,1);
    gtk_box_pack_start(GTK_BOX(fit_data.hbox[i]),fit_data.center[i],FALSE,FALSE,2);
    gtk_widget_show(GTK_WIDGET(fit_data.center[i]));

    // amplitude
    adjust = gtk_adjustment_new(0.0,-1e12,1e12,100,1000,0);
    fit_data.amplitude[i] = gtk_spin_button_new(GTK_ADJUSTMENT(adjust),0.2,1);
    gtk_box_pack_start(GTK_BOX(fit_data.hbox[i]),fit_data.amplitude[i],FALSE,FALSE,2);
    gtk_widget_show(GTK_WIDGET(fit_data.amplitude[i]));
    
    // gauss width
    adjust = gtk_adjustment_new(0.0,0.0,1e6,10,100,0);
    fit_data.gauss_wid[i] = gtk_spin_button_new(GTK_ADJUSTMENT(adjust),0.2,1);
    gtk_box_pack_start(GTK_BOX(fit_data.hbox[i]),fit_data.gauss_wid[i],FALSE,FALSE,2);
    gtk_widget_show(GTK_WIDGET(fit_data.gauss_wid[i]));


    // enable Gauss
    fit_data.enable_gauss[i] = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(fit_data.hbox[i]),fit_data.enable_gauss[i],FALSE,FALSE,2);
    gtk_widget_show(GTK_WIDGET(fit_data.enable_gauss[i]));


    // lorentz width
    adjust = gtk_adjustment_new(0.0,0.0,1e6,10,100,0);
    fit_data.lorentz_wid[i] = gtk_spin_button_new(GTK_ADJUSTMENT(adjust),0.2,1);
    gtk_box_pack_start(GTK_BOX(fit_data.hbox[i]),fit_data.lorentz_wid[i],FALSE,FALSE,2);
    gtk_widget_show(GTK_WIDGET(fit_data.lorentz_wid[i]));


    // enable Lorentz
    fit_data.enable_lorentz[i] = gtk_check_button_new();
    gtk_box_pack_start(GTK_BOX(fit_data.hbox[i]),fit_data.enable_lorentz[i],FALSE,FALSE,2);
    gtk_widget_show(GTK_WIDGET(fit_data.enable_lorentz[i]));



  }


 
  g_signal_connect(G_OBJECT(fit_data.dialog),"delete_event",G_CALLBACK(hide_fit),NULL);
  fit_data.shown = 0;





  buffp[0]=create_buff(0);


  gtk_combo_box_set_active(GTK_COMBO_BOX(add_sub.s_buff1),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(add_sub.s_buff2),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(add_sub.dest_buff),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(add_sub.s_record1),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(add_sub.s_record2),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(add_sub.dest_record),0);

  gtk_combo_box_set_active(GTK_COMBO_BOX(fit_data.s_buff),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(fit_data.d_buff),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(fit_data.s_record),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(fit_data.d_record),0);



  //  printf("did first buffer create, npts = %i\n",buffp[0]->param_set.npts);
  if (buffp[0] == NULL){
    printf("first buffer creation failed\n");
    exit(0);
  }
  // now a first buffer exists, find its size.
  gtk_window_get_size(GTK_WINDOW(buffp[0]->win.window),&width,&height);


  gtk_window_move(GTK_WINDOW(panwindow),0,height+50);
  gtk_widget_show(panwindow);

  current = 0;
  last_current = -1;



  /* do some color stuff */
  colourgc=gdk_gc_new(buffp[0]->win.canvas->window);

  colours[RED].red=65535;
  colours[RED].blue=0;
  colours[RED].green=0;
  colours[RED].pixel=(gulong) 255*256*256; /* red */
  //printf("pixel: %li\n",colours[RED].pixel);
  gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),&colours[RED],FALSE,TRUE);
  //printf("pixel: %li\n",colours[RED].pixel);
  
  colours[BLUE].red=0;
  colours[BLUE].blue=65535;
  colours[BLUE].green=0;
  colours[BLUE].pixel=(gulong) 255; /* blue */
  //printf("pixel: %li\n",colours[BLUE].pixel);
  gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),&colours[BLUE],FALSE,TRUE);
  //printf("pixel: %li\n",colours[BLUE].pixel);

  colours[GREEN].red=0;
  colours[GREEN].blue=0;
  colours[GREEN].green=65535;
  colours[GREEN].pixel=(gulong) 255*256; /* green */
  //printf("pixel: %li\n",colours[GREEN].pixel);
  gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),&colours[GREEN],FALSE,TRUE);
  //printf("pixel: %li\n",colours[GREEN].pixel);

  colours[WHITE].red=65535;
  colours[WHITE].blue=65535;
  colours[WHITE].green=65535;
  colours[WHITE].pixel=255+255*256+255*256*256;
  gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),&colours[WHITE],FALSE,TRUE);

  colours[BLACK].red=0;
  colours[BLACK].blue=0;
  colours[BLACK].green=0;
  colours[BLACK].pixel=0;
  gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),&colours[BLACK],FALSE,TRUE);


  ic=0;
  for (i=0;i<NUM_COLOURS/4;i++) {
    colours[ic].red = (int) 256*255.;
    colours[ic].blue = (int) 256*255.*0;
    colours[ic].green = (int) 256*255.*i/(NUM_COLOURS/4.);
    
    colours[ic].pixel= (gulong) colours[ic].green 
      +256*colours[ic].red+colours[ic].blue/256;
    //    printf("r g b %i %i %i %li\n",colours[ic].red,colours[ic].green, colours[ic].blue,colours[ic].pixel);
    
    gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),
		    &colours[ic],FALSE,TRUE);
    ic++;
  }

  for (i=0;i<NUM_COLOURS/4;i++){
    colours[ic].red = (int) 256*255.*(1.-(i/(NUM_COLOURS/4.)));
    colours[ic].blue = (int) 256*255.*0;
    colours[ic].green = (int) 256*255.;


    colours[ic].pixel= (gulong) colours[ic].green 
      +256*colours[ic].red+colours[ic].blue/256;
    //    printf("r g b %i %i %i %li\n",colours[ic].red,colours[ic].green, colours[ic].blue,colours[ic].pixel);
    gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),
		    &colours[ic],FALSE,TRUE);
    ic++;
  }

  //extra for white to be in the middle
  ic++;
  //  printf("just skipped colour: %i\n",ic-1);
  for (i=0;i<NUM_COLOURS/4;i++){
    colours[ic].red = (int) 256*255.*0;
    colours[ic].blue = (int) 256*255.*i/(NUM_COLOURS/4.);
    colours[ic].green = (int) 256*255.;
    

    colours[ic].pixel= (gulong) colours[ic].green 
      +256*colours[ic].red+colours[ic].blue/256;
    //    printf("r g b %i %i %i %li\n",colours[ic].red,colours[ic].green, colours[ic].blue,colours[ic].pixel);
    gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),
		    &colours[ic],FALSE,TRUE);
    ic++;
  }

  for (i=0;i<NUM_COLOURS/4;i++){
    colours[ic].red = (int) 256*255.*0;
    colours[ic].blue = (int) 256*255.;
    colours[ic].green = (int) 256*255.*(1.0-i/(NUM_COLOURS/4.));


    colours[ic].pixel= (gulong) colours[ic].green 
      +256*colours[ic].red+colours[ic].blue/256;
    //    printf("r g b %i %i %i %li\n",colours[ic].red,colours[ic].green, colours[ic].blue,colours[ic].pixel);
    gdk_colormap_alloc_color(gtk_widget_get_colormap(buffp[0]->win.canvas),
		    &colours[ic],FALSE,TRUE);
    ic++;
  }

  /*
  ic=NUM_COLOURS/2; // also white ?
  colours[ic].red=256*255;
  colours[ic].green=256*255;
  colours[ic].blue=256*255;
  colours[ic].pixel= (gulong) colours[ic].green 
    +256*colours[ic].red+colours[ic].blue/256;
  
  ic=NUM_COLOURS/2-1; //another white?
  colours[ic].red=256*255;
  colours[ic].green=256*255;
  colours[ic].blue=256*255;
  colours[ic].pixel= (gulong) colours[ic].green 
    +256*colours[ic].red+colours[ic].blue/256;

white set up below  */
  



  /* build some other dialogs */

  /* first the phase dialog */
  phase_dialog = gtk_dialog_new();
  phase0_ad=gtk_adjustment_new(0.0,-180.0,181.0,.1,.1,1);
  phase1_ad=gtk_adjustment_new(0.0,-180.0,181.0,.1,.1,1);

  hbox=gtk_hbox_new(FALSE,1);
  button=gtk_button_new_with_label("-360");
  gtk_box_pack_start(GTK_BOX(hbox),button,FALSE,FALSE,1);
  gtk_widget_show(button);
  phase_data.mbut=button;
  g_signal_connect(G_OBJECT(button),"clicked",
		     G_CALLBACK(phase_buttons),NULL);

  button=gtk_button_new_with_label("+360");
  gtk_box_pack_end(GTK_BOX(hbox),button,FALSE,FALSE,1);
  gtk_widget_show(button);
  phase_data.pbut=button;
  g_signal_connect(G_OBJECT(button),"clicked",
		     G_CALLBACK(phase_buttons),NULL);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(phase_dialog)->vbox),
		     hbox,TRUE,TRUE,1);

  snprintf(title,UTIL_LEN,"Phase 1");
  label=gtk_label_new(title);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox),
		     label,TRUE,TRUE,1);
  phase_data.pscroll1=gtk_hscale_new(GTK_ADJUSTMENT(phase1_ad));
  gtk_range_set_update_policy(GTK_RANGE(phase_data.pscroll1),
			      GTK_UPDATE_CONTINUOUS);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(phase_dialog)->vbox) ,
		     phase_data.pscroll1,TRUE,TRUE,1);
  g_signal_connect(G_OBJECT(phase1_ad),"value_changed",
		     G_CALLBACK(phase_changed),NULL);
  gtk_widget_set_size_request(phase_data.pscroll1,397,50);
  gtk_widget_show (phase_data.pscroll1);

  snprintf(title,UTIL_LEN,"Phase 0");
  label=gtk_label_new(title);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(phase_dialog)->vbox),
		     label,TRUE,TRUE,1);
  phase_data.pscroll0=gtk_hscale_new(GTK_ADJUSTMENT(phase0_ad));
  gtk_widget_set_size_request(phase_data.pscroll0,397,50);
  gtk_range_set_update_policy(GTK_RANGE(phase_data.pscroll0),
			      GTK_UPDATE_CONTINUOUS);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(phase_dialog)->vbox) ,
		     phase_data.pscroll0,TRUE,TRUE,0);
  g_signal_connect(G_OBJECT(phase0_ad),"value_changed",
		     G_CALLBACK(phase_changed),NULL);
  gtk_widget_show (phase_data.pscroll0);

  button=gtk_button_new_with_label("Ok");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(phase_dialog)->action_area),
		     button,TRUE,TRUE,1);
  g_signal_connect(G_OBJECT(button),"clicked",
		     G_CALLBACK(phase_buttons),(void *) 0);
  gtk_widget_show(button);
  phase_data.ok=button;

  button=gtk_button_new_with_label("Apply all");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(phase_dialog)->action_area),
		     button,TRUE,TRUE,1);
  g_signal_connect(G_OBJECT(button),"clicked",
		     G_CALLBACK(phase_buttons),(void *) 1);
  gtk_widget_show(button);
  phase_data.apply_all=button;


  button=gtk_button_new_with_label("Update Last");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(phase_dialog)->action_area),
		     button,TRUE,TRUE,1);
  g_signal_connect(G_OBJECT(button),"clicked",
		     G_CALLBACK(phase_buttons),(void *) 2);
  gtk_widget_show(button);
  phase_data.update=button;

  button=gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(phase_dialog)->action_area),
		     button,TRUE,TRUE,1);
  g_signal_connect(G_OBJECT(button),"clicked",
		     G_CALLBACK(phase_buttons),(void *) 3);
  gtk_widget_show(button);
  phase_data.cancel=button;
  phase_data.is_open=0;
  phase0=0.;
  phase1=0.;



  g_signal_connect( G_OBJECT (phase_dialog), "delete_event", G_CALLBACK( hide_phase ), NULL );
  

  /* now build frequency pop-up */

  freq_popup=gtk_window_new(GTK_WINDOW_POPUP);
  gtk_window_set_gravity(GTK_WINDOW(freq_popup),GDK_GRAVITY_NORTH_WEST);

  vbox=gtk_vbox_new(FALSE,1);
  gtk_container_add(GTK_CONTAINER(freq_popup),vbox);
  gtk_widget_show(vbox);
  
  fplab1=gtk_label_new("position");
  fplab2=gtk_label_new("and stuff");
  fplab3=gtk_label_new("dummy");
  fplab4=gtk_label_new("freqs");
  fplab5=gtk_label_new("freqs");

  gtk_widget_show(fplab1);
  gtk_widget_show(fplab2);
  gtk_widget_show(fplab3);
  gtk_widget_show(fplab4);
  gtk_widget_show(fplab5);

  gtk_box_pack_start(GTK_BOX(vbox),fplab1,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox),fplab2,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox),fplab3,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox),fplab4,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox),fplab5,FALSE,FALSE,0);










  /* setup a busy cursor */
  cursorclock=gdk_cursor_new(GDK_CLOCK);



  if( result_shm == 1 && !no_acq ) {
    printf("connecting to old shm\n");
    upload_buff = current;
    //printf( "initializing IPC signals and sending an idle_draw_canvas\n" );

    
    
    init_ipc_signals();
	
    
    path_strcpy(buffp[0]->path_for_reload,data_shm->save_data_path);
    set_window_title(buffp[0]);
    reload(NULL,NULL);

  }
  else
    draw_canvas(buffp[0]); 

  /*initialization complete */
  
  if (no_acq ==TRUE){
    signal( SIGINT,  do_destroy_all );
    signal( SIGTERM,  do_destroy_all );
    signal( SIGQUIT,  do_destroy_all ); // this is ordinary ^C or kill
  }

  show_active_border(); 

  last_draw();


  // add a timeout for debugging...

  //  timeout_tag = gtk_timeout_add(2000,(GtkFunction) check_for_overrun_timeout,timeout_data);
  gdk_threads_enter();
  gtk_main();
  gdk_threads_leave();
  //  printf("out of gtk_main\n");
  /* post main - clean up */
  //printf("post main cleanup\n");
  /* unalloc gc */
  gdk_gc_unref(colourgc);

  //  gtk_timeout_remove(timeout_tag);

  return 0;

}


void open_phase( dbuff *buff, int action, GtkWidget *widget )
{
  int buffnum,i;
  float old_low,old_up;
  buffnum=buff->buffnum;
  if (phase_data.is_open==1 || buff->win.press_pend >0 || 
      (buff->disp.dispstyle != SLICE_ROW && buff->disp.dispstyle !=SLICE_COL ))
      return;
  /* if its a column, has to be hyper */
  if(buff->disp.dispstyle ==SLICE_COL && !buff->is_hyper) return;

  buff->win.press_pend =1;
  g_signal_handlers_block_by_func(G_OBJECT(buff->win.canvas),
				   G_CALLBACK (press_in_win_event),
				   buff);
  g_signal_connect (G_OBJECT (buff->win.canvas), "button_press_event",
		      G_CALLBACK( pivot_set_event), buff);

  if(buff->disp.dispstyle==SLICE_ROW){
    phase_data.data=g_malloc(buff->param_set.npts*8);
    phase_data.data2=g_malloc(buff->param_set.npts*8);
    phase_npts = buff->param_set.npts;

    //copy the data to be phased to a safe place:
    
    for(i=0;i<phase_npts*2;i++) 
      phase_data.data[i]=
	buff->data[i+buff->disp.record*buff->param_set.npts*2];
    

  }
  else if(buff->disp.dispstyle==SLICE_COL){
    phase_data.data=g_malloc(buff->npts2*4);
    phase_data.data2=g_malloc(buff->npts2*4);
    phase_npts = buff->npts2/2;

    for(i=0;i<phase_npts;i++){
      phase_data.data[2*i]=buff->data[buff->param_set.npts*2*2*i+
				       2*buff->disp.record2];
      phase_data.data[2*i+1]=buff->data[buff->param_set.npts*2*(2*i+1)
					 +2*buff->disp.record2];
    }

  }
  /* copy the data into the first spot */


  phase_data.buffnum=buffnum;
  phase_data.pivot=0.0;

  if (((int)buff->process_data[PH].val & GLOBAL_PHASE_FLAG)!=0 ){
    if (buff->disp.dispstyle == SLICE_ROW){  //row
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase0_ad),phase0);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase1_ad),phase1);
    }
    else{   //col
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase0_ad),phase20);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase1_ad),phase21);
    }
  }
  else { /* not global */
    if(buff->disp.dispstyle == SLICE_ROW){  //row
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase0_ad),buff->phase0);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase1_ad),buff->phase1);
    }
    else{   //col
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase0_ad),buff->phase20);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase1_ad),buff->phase21);
    }
  }

  /* save old phase values from buffer */
  if(buff->disp.dispstyle ==SLICE_ROW ){
    phase_data.ophase0=buff->phase0_app;
    phase_data.ophase1=buff->phase1_app; 
  }
  else{
    phase_data.ophase0=buff->phase20_app;
    phase_data.ophase1=buff->phase21_app;
  }

  phase_data.last_phase1=GTK_ADJUSTMENT(phase1_ad)->value;

  /* check what the phase 1 value is */

  old_low=GTK_ADJUSTMENT(phase1_ad)->lower;
  old_up=GTK_ADJUSTMENT(phase1_ad)->upper;

  GTK_ADJUSTMENT(phase1_ad)->lower = 
      floor((GTK_ADJUSTMENT(phase1_ad)->value+180.0)/360.0)*360.-180. ;
  GTK_ADJUSTMENT(phase1_ad)->upper =GTK_ADJUSTMENT(phase1_ad)->lower+
    (old_up-old_low);

  gtk_window_set_transient_for(GTK_WINDOW(phase_dialog),GTK_WINDOW(panwindow));
  gtk_window_set_position(GTK_WINDOW(phase_dialog),GTK_WIN_POS_CENTER_ON_PARENT);

  gtk_widget_show(phase_dialog);
  phase_data.is_open=1;

  phase_changed(phase0_ad,NULL);
  
  return;
}

gint phase_buttons(GtkWidget *widget,gpointer data)
{
  int i, j,npts1;
  float lp0,lp1,dp0,dp1;
  dbuff *buff;
  /* first check to make sure buffer still exists */
  if (buffp[phase_data.buffnum] != NULL){

    buff=buffp[phase_data.buffnum];
    npts1=buff->param_set.npts;  

    lp0=GTK_ADJUSTMENT(phase0_ad)->value;
    lp1=GTK_ADJUSTMENT(phase1_ad)->value;

    /* do the ok button */
    if(widget == phase_data.ok || widget ==phase_data.apply_all){
      /* new phase values: */

      lp0 = lp0-(floor((lp0+180.)/360.))*360. ;

      /* differences */
      
      dp0=lp0-phase_data.ophase0;
      dp1=lp1-phase_data.ophase1;

      if(((int)buff->process_data[PH].val & GLOBAL_PHASE_FLAG)!=0){
	if (buff->disp.dispstyle == SLICE_ROW){
	  phase0=lp0; /* set new global values */
	  phase1=lp1;
	}
	else{
	  phase20=lp0;
	  phase21=lp1;
	}
      }
      /* either way the buffer's phases get it */
      if(buff->disp.dispstyle==SLICE_ROW){
	buff->phase0=lp0;
	buff->phase1=lp1;
	buff->phase0_app=lp0;
	buff->phase1_app=lp1;
      }
      else{ //its a col
	buff->phase20=lp0;
	buff->phase21=lp1;
	buff->phase20_app=lp0;
	buff->phase21_app=lp1;
      }

      /* actually do the phase on this data */

      if(buff->disp.dispstyle == SLICE_ROW){
	if (phase_npts == buff->param_set.npts){
	  //printf("in ok button, dealing with slice\n");
	  if(widget==phase_data.apply_all){
	    
	    for(j=0;j<buff->npts2;j++){
	      for(i=0;i<buff->param_set.npts*2;i++) 
		phase_data.data[i]=
		  buff->data[i+j*buff->param_set.npts*2];
	      do_phase(phase_data.data,&buff->data[npts1*2*j],
		       dp0,dp1,buff->param_set.npts);
	    }
	  }
	  else{  // not apply all 
	    do_phase(phase_data.data,&buff->data[npts1*2*buff->disp.record],
		     dp0,dp1,buff->param_set.npts);
	  }
	}// end if npts the same
	else popup_msg("npts changed, didn't apply phase",TRUE);
      }// end slice row
      else{ /* SLICE_COL */
	if (phase_npts == buff->npts2/2){
	  if(widget==phase_data.apply_all){
	    for(j=0;j<buff->param_set.npts*2;j++){// does real and imag of 1d's
	      for(i=0;i<buff->npts2/2;i++){
		phase_data.data[2*i]=buff->data[j+buff->param_set.npts*2*2*i];
		phase_data.data[2*i+1]=buff->data[j+
						  buff->param_set.npts*2*(2*i+1)];
	      }
	      do_phase(phase_data.data,phase_data.data2,dp0,dp1,buff->npts2/2);
	      for(i=0;i<buff->npts2/2;i++){
		buff->data[j+buff->param_set.npts*2*2*i]=phase_data.data2[2*i];
		buff->data[j+buff->param_set.npts*2*(2*i+1)]
		  =phase_data.data2[2*i+1];
	      }
	    }
	  }
	  else{ // not apply all 
	    for (j=2*buff->disp.record2;j<2*buff->disp.record2+2;j++){
	      for(i=0;i<buff->npts2/2;i++){
		phase_data.data[2*i]=buff->data[j+buff->param_set.npts*2*2*i];
		phase_data.data[2*i+1]=buff->data[j+
						  buff->param_set.npts*2*(2*i+1)];
	      }
	      do_phase(phase_data.data,phase_data.data2,dp0,dp1,buff->npts2/2);
	      for(i=0;i<buff->npts2/2;i++){
		buff->data[j+buff->param_set.npts*2*2*i]=phase_data.data2[2*i];
		buff->data[j+buff->param_set.npts*2*(2*i+1)]
		  =phase_data.data2[2*i+1];
	      }
	    }
	  }
	}// end npts the same
	else popup_msg("npts changed, didn't apply phase",TRUE);
      }// end slice col.


      /* close up */
      gtk_widget_hide(phase_dialog);
      phase_data.is_open=0;
      g_signal_handlers_unblock_by_func(G_OBJECT(buff->win.canvas),
				     G_CALLBACK (press_in_win_event),
				     buff);
      g_signal_handlers_disconnect_by_func (G_OBJECT (buff->win.canvas), 
                        G_CALLBACK( pivot_set_event), buff);
      buff->win.press_pend=0;
      draw_canvas(buff);
      if (buff->buffnum == current){
	gdk_window_raise(buff->win.window->window);
	//	printf("raised current window 4\n");
      }
      phase_npts=0;
      g_free(phase_data.data);
      g_free(phase_data.data2);
    }/* end button ok */
    else if(widget == phase_data.update){
      if(buff->disp.dispstyle ==SLICE_ROW){
	phase0=lp0;
	phase1=lp1;
	buff->phase0=lp0;
	buff->phase1=lp1;
	}
      else{
	phase20=lp0;
	phase21=lp1;
	buff->phase20=lp0;
	buff->phase21=lp1;
      }
      
    }
    else if(widget == phase_data.cancel){
      g_free(phase_data.data);
      g_free(phase_data.data2);
      phase_npts = 0;
      gtk_widget_hide(phase_dialog);
      phase_data.is_open=0;
      g_signal_handlers_disconnect_by_func (G_OBJECT (buff->win.canvas), 
                        G_CALLBACK( pivot_set_event), buff);
      g_signal_handlers_unblock_by_func(G_OBJECT(buff->win.canvas),
				     G_CALLBACK (press_in_win_event),
				     buff);
      buff->win.press_pend=0;
      if (buff->buffnum == current){
	gdk_window_raise(buff->win.window->window);
	//	printf("raised current window 5\n");
      }

      draw_canvas(buff);
    }
    else if (widget == phase_data.pbut){
      lp1+=360;
      GTK_ADJUSTMENT(phase1_ad)->lower +=360.0;
      GTK_ADJUSTMENT(phase1_ad)->upper +=360.;
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase1_ad),lp1);
      phase_changed(phase1_ad,NULL);
    }
    else if (widget == phase_data.mbut){
      lp1-=360;
      GTK_ADJUSTMENT(phase1_ad)->lower -=360.0;
      GTK_ADJUSTMENT(phase1_ad)->upper -=360.;
      gtk_adjustment_set_value(GTK_ADJUSTMENT(phase1_ad),lp1);
      phase_changed(phase1_ad,NULL);


    }

  }
  else{ /* if the buffer disappeared */
    g_free(phase_data.data);
    g_free(phase_data.data2);
    gtk_widget_hide(phase_dialog);
    phase_data.is_open=0;
    printf("in phase, buffer is gone\n");
  }
  return 0;
}

gint do_phase(float *source,float *dest,float phase0,float phase1,int npts)
{
  unsigned int i;
  float s1;
  float s2;

  phase0 *= M_PI/180.0;
  phase1 *= M_PI/180.0;

  for(i=0;i<npts;i++){

    s1 = source[i*2];
    s2 = source[i*2+1];

    if (npts >1 ){
      dest[2*i]=s1*cos(phase0+phase1*i/(npts-1))
	+s2*sin(phase0+phase1*i/(npts-1));
      dest[2*i+1]=-s1*sin(phase0+phase1*i/(npts-1))
	+s2*cos(phase0+phase1*i/(npts-1));
    }
    else{ // ignore lp1  might need this with a 2d data set with only 1 complex point.
      dest[2*i]=s1*cos(phase0)
	+s2*sin(phase0);
      dest[2*i+1]=-s1*sin(phase0)
	+s2*cos(phase0);


    }
 }

  return 0; 
}


gint phase_changed(GtkObject *widget,gpointer *data)
{
  float lp0,lp1;
  float dp0,dp1;
  dbuff *buff;
  int sizex,sizey,i;
  GdkRectangle rect;

  /* make sure buffer still exists and phase window is actually open */
  if(buffp[phase_data.buffnum]==NULL || phase_data.is_open==0) return -1;
  buff=buffp[phase_data.buffnum];
  lp0=GTK_ADJUSTMENT(phase0_ad)->value;
  lp1=GTK_ADJUSTMENT(phase1_ad)->value;

  if(G_OBJECT(widget) == G_OBJECT(phase1_ad)){
   /* all we do in here is make the corresponding change in phase0 */
    lp0 = lp0- (lp1-phase_data.last_phase1)*phase_data.pivot;
    lp0 = lp0-(floor ((lp0+180.)/360.))*360. ;
    phase_data.last_phase1=lp1;
    if (phase_data.pivot == 0.) phase_changed(GTK_OBJECT(phase0_ad),NULL);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(phase0_ad),lp0);
  }
  else /* is a phase 0 adjust */
    {
      /* so what do the adjustments say:*/
      dp0=lp0-phase_data.ophase0;
      dp1=lp1-phase_data.ophase1;
      /* copy the data over */
      if(buff->disp.dispstyle==SLICE_ROW){
	/*	do_phase(&buff->data[2*buff->disp.record*buff->param_set.npts],
		phase_data.data,dp0,dp1,buff->param_set.npts); */
	do_phase(phase_data.data,
		 phase_data.data2,dp0,dp1,phase_npts);
      }
      else{ /* is column */
	for(i=0;i<buff->npts2/2;i++){
	  /*	  phase_data.data2[2*i]=buff->data[buff->param_set.npts*2*2*i+
					  2*buff->disp.record2];
	  phase_data.data2[2*i+1]=buff->data[buff->param_set.npts*2*(2*i+1)
	  +2*buff->disp.record2]; */
	  do_phase(phase_data.data,phase_data.data2,dp0,dp1,phase_npts);
	}
      }      
      
      
      /* clear the canvas */
      sizex=buff->win.sizex;
      sizey=buff->win.sizey;
      rect.x=1;
      rect.y=1;
      rect.width=sizex;
      rect.height=sizey;
      gdk_draw_rectangle(buff->win.pixmap,
			 buff->win.canvas->style->white_gc,TRUE,
			 rect.x,rect.y,rect.width,rect.height);
      
      if (colourgc == NULL){
	printf("in draw_canvas, gc is NULL\n");
	return -1;           //??????? -SN
      } /* shouldn't need this */
      if(buff->disp.dispstyle==SLICE_ROW){
	draw_oned(buff,0.,0.,phase_data.data2,phase_npts);
      }
      else{ //column
	draw_oned(buff,0.,0.,phase_data.data2,phase_npts);
      }

      /* and draw in the pivot line */
      if ( phase_data.pivot >0 && phase_data.pivot <1){
	draw_vertical(buff,&colours[BLUE],phase_data.pivot,-1);
      }
      gtk_widget_queue_draw_area(buff->win.canvas,rect.x,rect.y,rect.width,rect.height);
    } /* end phase 0 adjust */
  return 0;  
}

gint pivot_set_event (GtkWidget *widget, GdkEventButton *event,dbuff *buff)
{
  int sizex,sizey,xval;
  float new_pivot,lp0,lp1,ll1,ll2;

  sizex=buff->win.sizex;
  sizey=buff->win.sizey;
  lp0=GTK_ADJUSTMENT(phase0_ad)->value;
  lp1=GTK_ADJUSTMENT(phase1_ad)->value;

  if(buff->win.press_pend<1){
    printf("in pivot set and press_pend <1\n");
    return TRUE;
  }

  if(buff->disp.dispstyle==SLICE_ROW){
    ll1=buff->disp.xx1;
    ll2=buff->disp.xx2;
  }
  else{
    ll1=buff->disp.yy1;
    ll2=buff->disp.yy2;
  }
  //printf("ll1: %f, ll2: %f\n",ll1,ll2);
  /* if there's an old pivot, erase it */

  if(phase_data.pivot>0 && phase_data.pivot<1){
    draw_vertical(buff,&colours[WHITE],phase_data.pivot,-1);
  }
  
  xval=event->x;
  if (xval == 0 ) xval=1; /* in case we clicked in the border, bring inside */
  if(xval == sizex+1) xval=sizex;

  new_pivot = (float) (xval-1) /(sizex-1) *(ll2-ll1)+ll1;

  //printf("pivot set to: %f\n",new_pivot);

  /*  
  lp0 += (new_pivot-phase_data.pivot)*
  GTK_ADJUSTMENT(phase1_ad)->value;   */


  lp0 = lp0-(floor((lp0+180.)/360.))*360. ;
  phase_data.pivot=new_pivot;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(phase0_ad),lp0);

  draw_vertical(buff,&colours[BLUE],new_pivot,-1);

  //printf("leaving pivot set\n");
  return TRUE;
    

}


void cursor_busy(dbuff *buff)
{
  //  printf("setting cursor busy\n");
  gdk_window_set_cursor(buff->win.window->window,cursorclock);

  return;
}

void cursor_normal(dbuff *buff)
{
  //  printf("setting cursor normal\n");
   gdk_window_set_cursor(buff->win.window->window,NULL); 
   return;
}


gint destroy_all(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *yes_b;
  GtkWidget *no_b;
  // this routine gets called if we do a file_exit, or if we kill the panel window
  //  printf("in destroy_all\n");
  
  if (acq_in_progress != ACQ_STOPPED){
    // need to build a "acq is running, are you sure you want to quit? box"
    
    dialog = gtk_dialog_new();
    gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );
    
    label = gtk_label_new ("Acquisition is in progress, are you sure you want to quit?");
    
    yes_b = gtk_button_new_with_label("Yes (Quit and leave Acq running)");
    no_b = gtk_button_new_with_label("No (Don't quit)");
    
    g_signal_connect (G_OBJECT (yes_b), "clicked", G_CALLBACK (do_destroy_all), dialog);
    g_signal_connect_swapped (G_OBJECT (no_b), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT( dialog ) );
    
    gtk_box_pack_start (GTK_BOX ( GTK_DIALOG(dialog)->action_area ),yes_b,FALSE,TRUE,0);
    gtk_box_pack_start (GTK_BOX ( GTK_DIALOG(dialog)->action_area ),no_b,FALSE,TRUE,0);
    
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);

    gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(buffp[current]->win.window));
    gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER_ON_PARENT);

    gtk_widget_show_all (dialog);
    return TRUE;
  }
  do_destroy_all();
  return FALSE; // seems hardly necessary
}

void do_destroy_all()
     // this routine does not fail - always destroys ( with gtk_main_quit )
{
 /* should run through all buffers and call file_close for each */
  int i;

  // this gets called from destroy_all, and also if we get a kill signal

  //  printf("in do_destroy_all\n");
  from_do_destroy_all = 1;
  for( i=0; i<MAX_BUFFERS; i++ ) {
    if( buffp[i] != NULL )
      destroy_buff( buffp[i]->win.window, NULL ); 
  }

  //release_ipc_stuff will be called when the last file window is destroyed,, so we don't have to 
  // call it explicitaly
  //  printf( "done closing buffers\n" );

  //  gtk_main_quit();  // this is done in destroy buff.
  //  printf("just called gtk_main_quit\n");
  return;
}

gint hide_phase( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
  phase_buttons( phase_data.cancel, NULL );
  //printf( "hiding window\n" );
  return TRUE;
}

gint popup_msg( char* msg ,char modal)

{
  GtkWidget* dialog;
  GtkWidget* button;
  GtkWidget* label;

  dialog = gtk_dialog_new();

  if (modal == TRUE)
    gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE );

  label = gtk_label_new ( msg );
  button = gtk_button_new_with_label("OK");
  g_signal_connect_swapped (G_OBJECT (button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT( dialog ) );
  gtk_box_pack_start (GTK_BOX ( GTK_DIALOG(dialog)->action_area ),button,FALSE,FALSE,0);

  gtk_container_set_border_width( GTK_CONTAINER(dialog), 5 );

  gtk_box_pack_start ( GTK_BOX( (GTK_DIALOG(dialog)->vbox) ), label, FALSE, FALSE, 5 );

  //  gtk_widget_set_uposition(dialog,(int) 50,(int) 200);
  gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(buffp[current]->win.window));
  gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER_ON_PARENT);

  gtk_widget_show_all (dialog);

  //  gdk_window_raise(dialog->window); // seems unnecessary
  //  printf("raised dialog?\n");
  return FALSE;
}

gint popup_msg_mutex_wrap(char *msg){
  gint i;
  gdk_threads_enter();
  i = popup_msg(msg,TRUE);
  gdk_threads_leave();
  return i;
}






void draw_vertical(dbuff *buff,GdkColor *col, float xvalf,int xvali){


  GdkRectangle rect;
  int sizex,sizey;
  float ll1,ll2;
  // routine will use either xvalf or xvali 
  // 0 < xvalf < 1, 0 < xvalf < number of pixels !

  sizex=buff->win.sizex;
  sizey=buff->win.sizey;

  if(buff->disp.dispstyle==SLICE_ROW){
    ll1=buff->disp.xx1;
    ll2=buff->disp.xx2;
  }
  else{
    ll1=buff->disp.yy1;
    ll2=buff->disp.yy2;
  }


  if (xvali == -1){
    rect.x=(int) ((xvalf-ll1)*(sizex-1)/
      (ll2-ll1)+1.5);

    //    printf("in draw_vertical with rect.x = %i from float\n",rect.x);
  }
  else{ 
    rect.x = xvali;
    //    printf("in draw_vertical with rect.x = %i from int\n",rect.x);
  }

    if (rect.x < 0 || rect.x >sizex){
      printf("in draw row_vert, xval is out of range\n");
    }

  
    rect.y=1;
    rect.width=1;
    rect.height=sizey;

    gdk_gc_set_foreground(colourgc,col);
    gdk_draw_line(buff->win.pixmap,colourgc,rect.x,1,rect.x,sizey);
    gtk_widget_queue_draw_area (buff->win.canvas, rect.x,rect.y,rect.width,rect.height);
}


