/* xnmr_types.h
 *
 * Contains some type definitions for the Xnmr software project
 *
 * UBC Physics
 * April, 2000
 * 
 * written by: Scott Nelson, Carl Michal
 */

#ifndef XNMR_TYPES_H
#define XNMR_TYPES_H

#include <gtk/gtk.h>


#include "param_utils.h"

// these three also defined in param_utils.h - ick
#define MAX_PARAMETERS 100
#define PATH_LENGTH 200
#define UTIL_LEN 40


struct parameter_button_t {
  GtkWidget *label;
  GtkWidget *button;
  GtkObject *adj;
  GtkWidget *ent;
};

struct popup_data_t {
  parameter_t* param;
  GtkWidget **spin_hbox;
  GtkWidget **sp_button;
  GtkObject **adj;

  int button_number;

  GtkWidget *frame;
  GtkWidget *win;
  GtkObject *num_adj;
  GtkObject *start_adj;
  GtkWidget *start_button;
  GtkObject *inc_adj;
  GtkWidget *inc_button;
  GtkWidget *button_vbox;
  gint size;
  gint bnum;
};

typedef struct {
  gint (*func) ( GtkWidget *widget, double *data );
  GtkObject *adj;
  GtkWidget *button;
} process_button_t;

typedef struct {
  int status;
  double val;
} process_data_t;

/*
 *  possible values for process_data_t.status
 */

#define NO_PROCESS 0
#define PROCESS_OFF 1
#define SCALABLE_PROCESS_OFF 2
#define PROCESS_ON 3
#define SCALABLE_PROCESS_ON 4

#define MAX_PROCESS_FUNCTIONS 64

typedef struct {
  int npts; /* number of pts in dimension 1:  moved into param_set */
  int npts2; /* this is how many records there are in memory, num_acqs_2d is
		how many we'll acquire next go (in param_set) */
  int buffnum;
  unsigned long ct; // total number of transients completed
  int flags;
  unsigned int acq_npts; // this is how many points we will acquire
  float phase0,phase1;
  float phase0_app,phase1_app; /* these are what have been applied in this bu*/
  float phase20,phase21; /* for 2nd dimension */
  float phase20_app,phase21_app;
  float csshift; /* chemical shift reference */
  //  char comment[200]; // never used for anything.
  char is_hyper;
  char path_for_reload[PATH_LENGTH];
  struct{ /* stuff related to its window */
    GtkWidget *canvas; 
    GtkWidget *toggleb;
    GtkWidget *window;
    GtkWidget *autocheck,*hypercheck;
    GtkWidget *row_col_lab,*slice_2d_lab;
    GtkWidget *p1_label,*p2_label,*ct_label,*ct_box;
    GtkWidget *symm_check;
    int sizex,sizey; /* number of pixels for drawing, total is +2 */
    int buffid;
    int press_pend;
    float pend1,pend2,pend3,pend4;
    GdkPixmap *pixmap;
    GSList *grp1,*grp2;
    GtkWidget *but1a,*but1b,*but1c,*but2a,*but2b,*but2c;
  } win;
  struct{ /* related to display */
    float xx1,xx2;  /* limits of spectrum 0 -> 1 */
    float yy1,yy2; /* limits of spectrum for 2d */
    float yscale,yoffset;
    char real,imag,mag,base; /* toggles whether or not they are displayed */
    char dispstyle; /* slice,raster, stacked, contour */
    int record,record2;
  } disp;
  int overrun1;
  float *data;
  int overrun2;
  parameter_set_t param_set;
  process_data_t process_data[ MAX_PROCESS_FUNCTIONS ];
  GtkWidget *scales_dialog;
  char script_open;
} dbuff;


typedef struct {
  GtkWidget  
  *process_button,
    *acquire_button;
  int acquire_notify;
    } script_widget_type;


/* flags are: */

#define FT_FLAG 1
#define GLOBAL_PHASE_FLAG 2
#define FT_FLAG2 4

/*  symbols defining the positions of the processing functions */

#define CR 0
#define BC1 1
#define LS 2
#define SF 3
#define EM 4
#define GM 5
#define ZF 6
#define ZI 7
#define FT 8
#define BFT 9
#define BC2 10
#define PH 11

// this is the number of the first 2D processing function:
#define P2D 12
#define BC2D1 12
#define LS2D 13
#define EM2D 14
#define GM2D 15
#define ZF2D 16
#define UNWIND 17
#define FT2D 18
#define MAG2D 19
#define BC2D2 20
#define PH2D 21

#endif




