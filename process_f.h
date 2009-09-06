/* process_f.h
 *
 * header file for process.c 
 *
 * UBC Physics
 * April, 2000
 * 
 * written by: Scott Nelson, Carl Michal
 */

#ifndef PROCESS_F_H
#define PROCESS_F_H

#include <gtk/gtk.h>

#include "xnmr_types.h"

/*
 *  Globals
 */

extern process_data_t* acq_process_data;

#define PSRBMAX 18 


// max number of bits in the register for PSRB

/*
 *  Function Prototypes
 */

GtkWidget* create_process_frame();
GtkWidget* create_process_frame_2d();

/*
 *  Processing functions
 */

gint do_offset_cal( GtkWidget *widget, double *unused);
gint do_offset_cal_and_display( GtkWidget *widget, double *unused);

gint do_offset_cal_a( GtkWidget *widget, double *unused);
gint do_offset_cal_a_and_display( GtkWidget *widget, double *unused);

gint do_offset_cal_2D( GtkWidget *widget, double *unused);
gint do_offset_cal_2D_and_display( GtkWidget *widget, double *unused);

gint do_offset_cal_2D_a( GtkWidget *widget, double *unused);
gint do_offset_cal_2D_a_and_display( GtkWidget *widget, double *unused);

gint do_cross_correlate_mlbs( GtkWidget *widget, double *val);
gint do_cross_correlate_chu( GtkWidget *widget, double *val);
gint do_cross_correlate_frank( GtkWidget *widget, double *val);
gint do_cross_correlate( GtkWidget *widget, double *val);
gint do_cross_correlate_and_display( GtkWidget *widget, double *val);

gint do_zero_imag(GtkWidget *widget, double *spare);
gint do_zero_imag_and_display(GtkWidget *widget, double *spare);

gint do_ft(GtkWidget *widget, double *spare);
gint do_ft_and_display(GtkWidget *widget, double *spare);

gint do_bft(GtkWidget *widget, double *spare);
gint do_bft_and_display(GtkWidget *widget, double *spare);


gint do_exp_mult( GtkWidget* widget, double *val);
gint do_exp_mult_and_display( GtkWidget* widget, double *val);

gint do_gaussian_mult( GtkWidget* widget, double *val );
gint do_gaussian_mult_and_display( GtkWidget* widget, double *val );

gint do_zero_fill(GtkWidget * widget,double *val);
gint do_zero_fill_and_display(GtkWidget * widget,double *val);

gint do_left_shift(GtkWidget * widget,double *val);
gint do_left_shift_and_display(GtkWidget * widget,double *val);

gint do_left_shift_2d(GtkWidget * widget,double *val);
gint do_left_shift_2d_and_display(GtkWidget * widget,double *val);

gint do_shim_filter(GtkWidget * widget,double *val);
gint do_shim_filter_and_display(GtkWidget * widget,double *val);

gint do_phase_and_display_wrapper( GtkWidget* widget, double *data );
gint do_phase_wrapper( GtkWidget* widget, double *data );

gint do_phase_2d_and_display_wrapper( GtkWidget* widget, double *data );
gint do_phase_2d_wrapper( GtkWidget* widget, double *data );

gint process_data( GtkWidget *widget, gpointer data ); 
  //Performs multiple processing operations
  //does processing based on which processing functions are active (boxes are ticked on panel)

// 2d routines
gint do_exp_mult_2d( GtkWidget* widget, double *val);
gint do_exp_mult_2d_and_display( GtkWidget* widget, double *val);

gint do_gaussian_mult_2d( GtkWidget* widget, double *val);
gint do_gaussian_mult_2d_and_display( GtkWidget* widget, double *val);

gint do_zero_fill_2d(GtkWidget * widget,double *val);
gint do_zero_fill_2d_and_display(GtkWidget * widget,double *val);

gint do_ft_2d(GtkWidget *widget, double *spare);
gint do_ft_2d_and_display(GtkWidget *widget, double *spare);

gint do_mag_2d(GtkWidget *widget, double *spare);
gint do_mag_2d_and_display(GtkWidget *widget, double *spare);

gint unscramble_2d(GtkWidget *widget, double *spare);
gint unscramble_2d_and_display(GtkWidget *widget, double *spare);

gint do_hadamard1(GtkWidget *widget, double *spare);
gint do_hadamard1_and_display(GtkWidget *widget, double *spare);

gint do_hayashi1(GtkWidget *widget, double *spare);
gint do_hayashi1_and_display(GtkWidget *widget, double *spare);



gchar psrb(int bits,int init);


/*
 *  Visual methods
 */

gint update_active_process_data( GtkAdjustment *adj, int button );
  //Updates the processing data structure to reflect changes made by the user on the panel

void show_process_frame( process_data_t* process_set );
  //Updates the panel to display a particular set of processing options

#endif


