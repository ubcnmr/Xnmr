/* Xnmr.h include file with data structures and so forth 
 *
 *  UBC Physics
 * April, 2000
 * 
 * written by: Carl Michal, Scott Nelson
 */

#ifndef XNMR_H
#define XNMR_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <math.h>

#include "buff.h"


#define MAX_BUFFERS 50
//NUM_COLOURS should be n*4+1 (17,21,25 etc)
#define NUM_COLOURS 25
#define EXTRA_COLOURS 4


/* global variables */

extern GdkColor  colours[NUM_COLOURS+EXTRA_COLOURS];
extern GdkGC  *colourgc;
extern phase_data_struct  phase_data;
extern add_sub_struct add_sub;
extern fitting_struct fit_data;

extern GtkWidget  *phase_dialog,*freq_popup,*fplab1,*fplab2,*fplab3,*fplab4,*fplab5;
extern GtkObject  *phase0_ad,*phase1_ad;
extern float  phase0,phase1;
extern float  phase20,phase21;
extern GdkCursor  *cursorclock;
extern char no_acq;
/* function prototypes */

//void open_phase(dbuff *buff,int action,GtkWidget *widget);
void open_phase(GtkAction *action,dbuff *buff);

gint do_phase(float *source,float *dest,float phase0,float phase1,int npts);

gint phase_changed(GtkObject *widget,gpointer *data);

gint pivot_set_event (GtkWidget *widget, GdkEventButton *event,dbuff *buff);

void cursor_normal(dbuff *buff);

void cursor_busy(dbuff *buff);

gint destroy_all(GtkWidget *widget, gpointer data);

void do_destroy_all();

gint hide_phase( GtkWidget *widget, GdkEvent  *event, gpointer data );

gint popup_msg( char* msg,char modal);
gint popup_msg_mutex_wrap( char* msg );

void draw_vertical(dbuff *buff,GdkColor  *col, float xvalf,int xvali);

#define RED NUM_COLOURS
#define GREEN NUM_COLOURS+1
#define BLUE NUM_COLOURS+2
#define WHITE ((NUM_COLOURS-1)/2)
#define BLACK NUM_COLOURS+3
// WHITE is (NUM_COLOURS-1)/2

#endif







