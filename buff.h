/* buff.h include file with data structures and so forth 
 *
 * UBC Physics
 * April, 2000
 * 
 * written by: Carl Michal, Scott Nelson
 */

#ifndef BUFF_H
#define BUFF_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <math.h>

#include "param_f.h"
#include "process_f.h"
#include "xnmr_types.h"

#define MAX_BUFFERS 50
#define BUFF_KEY "buffer key"
#define PATH_KEY "path key"

/* display styles */

#define SLICE_ROW 0
#define SLICE_COL 1
#define RASTER 2
#define STACKED 3
#define CONTOUR 4

/* for baseline_spline: */
#define PICK_SPLINE_POINTS 0
#define DO_SPLINE 1
#define SHOW_SPLINE_FIT 2
#define CLEAR_SPLINE_POINTS 3
#define UNDO_SPLINE 4
typedef struct {
  float ophase0,ophase1;
  float last_phase1;
  float pivot; /* between 0 and 1 */
  int buffnum;
  float *data,*data2;
  GtkWidget *ok,*apply_all,*update,*cancel;
  GtkWidget *pscroll0,*pscroll1;
  GtkWidget *pbut,*mbut;
  int is_open;
} phase_data_struct;

typedef struct {
  GtkWidget *dialog,*s_buff1,*s_buff2,*dest_buff,
    *s_record1,*s_record2,*dest_record,*apply,*close,*mult1,*mult2;
  char index[MAX_BUFFERS]; /* the index numbers tell you which buffer is 
			      in which spot in the list for the buffer number.
			      for the destination, there's always a 'new' at the top */
  int  s_rec_c1,s_rec_c2,dest_rec_c; 
  // number of records in the list for the combo box
  char shown ;
    } add_sub_struct;



/* global variables */

extern int current;
extern int last_current;
extern int num_buffs;
extern int active_buff;
extern dbuff *buffp[MAX_BUFFERS];
extern char data_path[PATH_LENGTH];
extern char from_do_destroy_all;
extern char no_update_open;
extern GtkWidget *panwindow;

/* function prototypes */
dbuff *create_buff();
void file_open(dbuff *buff,int action,GtkWidget *widget);
void file_close(dbuff *buff,int action,GtkWidget *widget);
void file_export(dbuff *buff,int action,GtkWidget *widget);
void file_save(dbuff *buff,int action,GtkWidget *widget);
void file_save_as(dbuff *buff,int action,GtkWidget *widget);
//void file_save_as(GtkAction *action,dbuff *buff);
void file_new(dbuff *buff,int action,GtkWidget *widget);
//void file_new(GtkAction *action,dbuff *buff);
void file_exit(dbuff *buff,int action,GtkWidget *widget);
void file_append(dbuff *buff,int action,GtkWidget *widget);


void do_scales(dbuff *buff,int action,GtkWidget *widget);
gint do_user_scales(GtkWidget *widget, float *range);
gint do_wrapup_user_scales(GtkWidget *widget, dbuff *buff);
gint do_update_scales(GtkWidget *widget, dbuff *buff);
void toggle_disp(dbuff *buff,int action,GtkWidget *widget);
gint expose_event(GtkWidget *widget,GdkEventExpose *event,dbuff *buff);
gint configure_event(GtkWidget *widget,GdkEventConfigure *event,
			 dbuff *buff);
void draw_oned(dbuff *buff,float extrayoff,float extraxoff,float *data
	       ,int npts);
void draw_oned2(dbuff *buff,float extrayoff,float extraxoff);
void draw_canvas(dbuff *buff);
gint destroy_buff(GtkWidget *widget,gpointer num);
gint full_routine(GtkWidget *widget,dbuff *buff);
gint expand_routine(GtkWidget *widget,dbuff *buff);
gint expandf_routine(GtkWidget *widget,dbuff *buff);
gint expand_press_event (GtkWidget *widget, GdkEventButton *event,dbuff *buff);
gint expandf_press_event (GtkWidget *widget, GdkEventButton *event,dbuff *buff);

gint Bbutton_routine(GtkWidget *widget,dbuff *buff);
gint bbutton_routine(GtkWidget *widget,dbuff *buff);
gint Sbutton_routine(GtkWidget *widget,dbuff *buff);
gint sbutton_routine(GtkWidget *widget,dbuff *buff);
gint auto_routine(GtkWidget *widget,dbuff *buff);
gint autocheck_routine(GtkWidget *widget,dbuff *buff);
gint do_auto(dbuff *buff);
gint do_auto2(dbuff *buff);
gint offset_routine(GtkWidget *widget,dbuff *buff);
gint offset_press_event (GtkWidget *widget, GdkEventButton *event,dbuff *buff);


gint press_in_win_event(GtkWidget *widget,GdkEventButton *event,dbuff *buff);

void show_active_border();
gint phase_buttons(GtkWidget *widget,gpointer data);

gint hyper_check_routine(GtkWidget *widget,dbuff *buff);

gint plus_button(GtkWidget *widget,dbuff *buff);
gint minus_button(GtkWidget *widget,dbuff *buff);
gint row_col_routine(GtkWidget *widget,dbuff *buff);

void draw_raster(dbuff *buff);
gint slice_2D_routine(GtkWidget *widget,dbuff *buff);
gint buff_resize( dbuff* buff, int npts1, int npts2 );

gint do_load( dbuff* buff, char* path);
gint do_load_wrapper( GtkWidget* widget, GtkFileSelection* fs );

gint do_save( dbuff* buff, char* fileN );
gint do_save_wrapper( GtkWidget* widget, GtkWidget* dialog );

gint check_overwrite( dbuff* buff, char* path );
gint check_overwrite_wrapper( GtkWidget* widget, GtkFileSelection* fs );

gint pix_to_x(dbuff * buff,int xval);
gint pix_to_y(dbuff * buff,int yval);

void set_window_title(dbuff *buff);
gint set_cwd(char *dir);
void update_param_win_title(parameter_set_t *param_set);
gint put_name_in_buff(dbuff *buff,char *fname);


void signal2noise(dbuff *buff,int action,GtkWidget *widget);
void s2n_press_event(GtkWidget *widget,GdkEventButton *event,dbuff *buff);
void signal2noiseold( dbuff *buff, int action, GtkWidget *widget );
void integrate(dbuff *buff,int action,GtkWidget *widget);
void integrate_press_event(GtkWidget *widget,GdkEventButton *event,dbuff *buff);
void integrateold( dbuff *buff, int action, GtkWidget *widget );
void integrate_from_file( dbuff *buff, int action, GtkWidget *widget );
void clone_from_acq(dbuff *buff, int action, GtkWidget *widget );
void set_sf1(dbuff *buff, int action, GtkWidget *widget);
void calc_rms(dbuff *buff, int action, GtkWidget *widget);
void add_subtract(dbuff *buff, int action, GtkWidget *widget);

void reset_dsp_and_synth(dbuff *buff, int action, GtkWidget *widget);

void baseline_spline(dbuff *buff, int action, GtkWidget *widget);


char get_ch1(dbuff *buff);
char get_ch2(dbuff *buff);

void set_ch1(dbuff *buff,char ch1);
void set_ch2(dbuff *buff,char ch2);

void check_for_overrun_timeout( gpointer data);
void make_active(dbuff *buff);
gint channel_button_change(GtkWidget *widget,dbuff *buff);

void add_sub_changed(GtkWidget *widget,gpointer data);
void add_sub_buttons(GtkWidget *widget,gpointer data);
gint hide_add_sub(GtkWidget *widget,gpointer data);
#endif

