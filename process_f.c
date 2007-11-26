#define GTK_DISABLE_DEPRECATED
/* process_f.c
 *
 * Implementation of the process panel page in Xnmr
 *
 * UBC Physics
 * April, 2000
 * 
 * written by: Scott Nelson, Carl Michal
 */

#include "param_utils.h"
#include "process_f.h"
#include "panel.h"
#include "xnmr.h"
#include "buff.h"
#include "nr.h"
#include "xnmr.h"


#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

process_button_t process_button[ MAX_PROCESS_FUNCTIONS ];
process_data_t* active_process_data;
process_data_t* acq_process_data;

GtkWidget *r_local_button,*r_global_button;

/*
 *  Data Processing Functions
 *
 *  If the function was called from gtk by pressing a button, the *widget pointer will
 *  point to the calling Gtk object.  If the process was called explicately by the process
 *  data function, the pointer will be NULL
 */

gint do_offset_cal_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  result = do_offset_cal( widget, unused );


  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}


gint do_offset_cal( GtkWidget *widget, double *unused )

{
  int i, j;
  int count;
  float offset;
  dbuff *buff;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  if (buff == NULL){
    popup_msg("do_offset_cal panic! buff is null!",TRUE);
    return 0;
  }

  for( j=0; j<buff->npts2; j++ ) {

    count = 0;
    offset = 0.0;

    //first determine the offset for the real channel

    for( i= (buff->npts*9/10)*2+j*2*buff->npts; i < (j+1)*2*buff->npts; i+=2 ) {
      offset += buff->data[i];
      count ++;
    }

    offset = offset / count;
    
    for( i=j*2*buff->npts; i < (j+1)*2*buff->npts; i+= 2 )
      buff->data[i] -= offset;
    
    //Now do the imaginary channel
    count = 0;
    offset = 0.0;

    for( i= (buff->npts*9/10)*2+1+j*2*buff->npts; i < (j+1)*2*buff->npts; i+=2 ) {
      offset += buff->data[i];
      count ++;
    }

    offset = offset / count;

    for( i=1+j*2*buff->npts; i < (j+1)*2*buff->npts; i+= 2 )
      buff->data[i] -= offset;
  }
  return 0;
}



gint do_offset_cal_a_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  result = do_offset_cal_a( widget, unused );


  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}


gint do_offset_cal_a( GtkWidget *widget, double *unused )

     /* this version does the BC by doing the ft, setting the center point to be the average 
	of those on either side, and then doing the reverse ft */

{
  int j,i;
  dbuff *buff;
  double spared;
  float scale;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  if (buff == NULL){
    popup_msg("do_offset_cal panic! buff is null!",TRUE);
    return 0;
  }


  // make sure we can do the ft:
  spared = 1.0;
  cursor_busy(buff);
  do_zero_fill(widget,&spared);

  scale = buff->npts;
  for ( j = 0;j<buff-> npts2; j++){
    four1(&buff->data[j*2*buff->npts]-1,buff->npts,-1);
    buff->data[2*j*buff->npts]= (buff->data[2*j*buff->npts+2] + buff->data[2*(j+1)*buff->npts-2])/2.0;
    buff->data[2*j*buff->npts+1]= (buff->data[2*j*buff->npts+3] + buff->data[2*(j+1)*buff->npts-1])/2.0;

    
    four1(&buff->data[j*2*buff->npts]-1,buff->npts,1);
    for (i=0;i<buff->npts;i++){
      buff->data[2*j*buff->npts+2*i] /= scale;
      buff->data[2*j*buff->npts+2*i+1] /= scale;
    }
    
	   
  }

  cursor_normal(buff);

  return 0;
}


gint do_zero_imag_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  result = do_zero_imag( widget, unused );

  draw_canvas( buff );
   return result;
}

gint do_zero_imag(GtkWidget *widget, double *unused)

{

  dbuff *buff;
  int i,j;

  if( widget == NULL ) {
    buff = buffp[ upload_buff ];
    // fprintf(stderr,"do_ft- on buffer %i\n",upload_buff );
  }
  else {
    buff = buffp[ current ];
    // fprintf(stderr,"do_ft- on buffer %i\n",current );
  }
  if (buff == NULL){
    popup_msg("do_zero_imag panic! buff is null!",TRUE);
    return 0;
  }


  for(i=0;i<buff->npts2;i++){
    for(j=0;j<buff->npts;j++)
      buff->data[2*j+1+i*buff->npts*2] = 0.;
  }


  

  return TRUE;
  
}

gint do_ft_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  result = do_ft( widget, unused );

  draw_canvas( buff );
   return result;
}

gint do_ft(GtkWidget *widget, double *unused)

{
  /* do fourier transform in one dimension */
  dbuff *buff;
  int i,j;
  float spare;
  double spared;
  float scale;

  if( widget == NULL ) {
    buff = buffp[ upload_buff ];
    // fprintf(stderr,"do_ft- on buffer %i\n",upload_buff );
  }
  else {
    buff = buffp[ current ];
    // fprintf(stderr,"do_ft- on buffer %i\n",current );
  }
  if (buff == NULL){
    popup_msg("do_ft panic! buff is null!",TRUE);
    return 0;
  }

  /*********

  if(buff->win.press_pend>0) {
    fprintf(stderr,"ft: press_pend>0, quitting\n");
    return TRUE;  //standard exit
  }

  **********/

  buff->flags ^= FT_FLAG; //toggle the ft flag
  
  /* do a zero fill with a factor of 1 to make 
     sure we have a power of two as npts */
  spared=1.0;
  do_zero_fill(widget,&spared);
  cursor_busy(buff);
  //    scale=sqrt((float) buff->npts);
  if (buff->flags & FT_FLAG){
    //    fprintf(stderr,"FT_FLAG is true\n");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check))){
    // if its symmetric, swap the first and second halves
      for(i=0;i<buff->npts2;i++)
	for(j=0;j<buff->npts;j++){
	  spare = buff->data[j+i*2*buff->npts];
	  buff->data[j+i*2*buff->npts]=
	    buff->data[j+i*2*buff->npts+buff->npts];
	  buff->data[j+i*2*buff->npts+buff->npts]=spare;
	}

    }

    scale = buff->npts/2.0;
  }
  else{
    //    fprintf(stderr,"FT_FLAG is false\n");
    scale = 2.0;
  }


  for(i=0;i<buff->npts2;i++){
    /* do ft for each 1d spectrum */
    if (buff->flags & FT_FLAG &&  
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check)) == FALSE){
      // don't do the /2 thing.  Not doing it only ever messes with the baseline.  Doing it can mess things
      // up worse.
      //      buff->data[i*2*buff->npts] /= 2;
      // buff->data[i*2*buff->npts+1] /= 2;
    }
    four1(&buff->data[i*2*buff->npts]-1,buff->npts,-1);
    for(j=0;j<buff->npts;j++){
      spare=buff->data[j+i*2*buff->npts]/scale;
      buff->data[j+i*2*buff->npts]=
	buff->data[j+i*2*buff->npts+buff->npts]/scale;
      buff->data[j+i*2*buff->npts+buff->npts]=spare;
    }
  }

  
  cursor_normal(buff);
  return TRUE;
  
}



gint do_bft_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  result = do_bft( widget, unused );

  draw_canvas( buff );
   return result;
}

gint do_bft(GtkWidget *widget, double *unused)

{
  /* do fourier transform in one dimension */
  dbuff *buff;
  int i,j;
  //  float spare;
  double spared;
  float scale;
  float *data;

  if( widget == NULL ) {
    buff = buffp[ upload_buff ];
    // fprintf(stderr,"do_ft- on buffer %i\n",upload_buff );
  }
  else {
    buff = buffp[ current ];
    // fprintf(stderr,"do_ft- on buffer %i\n",current );
  }
  if (buff == NULL){
    popup_msg("do_bft panic! buff is null!",TRUE);
    return 0;
  }

  /*********

  if(buff->win.press_pend>0) {
    fprintf(stderr,"ft: press_pend>0, quitting\n");
    return TRUE;  //standard exit
  }

  **********/

  buff->flags ^= FT_FLAG; //toggle the ft flag
  
  /* do a zero fill with a factor of 1 to make 
     sure we have a power of two as npts */
  spared=1.0;
  do_zero_fill(widget,&spared);
  cursor_busy(buff);
  //    scale=sqrt((float) buff->npts);
  if (buff->flags & FT_FLAG){
    //    fprintf(stderr,"FT_FLAG is true\n");
    scale = buff->npts/2.0;
  }
  else{
    //    fprintf(stderr,"FT_FLAG is false\n");
    scale = 2.0;
  }
  data = g_malloc(sizeof(float)* buff->npts*2*2);

  for(i=0;i<buff->npts2;i++){
    /* do ft for each 1d spectrum */
    // copy data out 
    memset(data,0,4*buff->npts*sizeof(float));

    for (j=0;j<buff->npts;j++){
      data[j*4]=buff->data[i*2*buff->npts + j*2];
      data[j*4+3] = -1.0*buff->data[i*2*buff->npts +j*2+1];
    }
    four1(data-1,buff->npts*2,-1);
    //    four1(&buff->data[i*2*buff->npts]-1,buff->npts,-1);
    for(j=0;j<buff->npts;j++){
      buff->data[i*2*buff->npts+j*2]=data[2*j+buff->npts]/scale;
      buff->data[i*2*buff->npts+j*2+1]=data[2*j+buff->npts+1]/scale;
    }
    /*    for(j=0;j<buff->npts;j++){
      spare=buff->data[j+i*2*buff->npts]/scale;
      buff->data[j+i*2*buff->npts]=
	buff->data[j+i*2*buff->npts+buff->npts]/scale;
      buff->data[j+i*2*buff->npts+buff->npts]=spare;
      }*/
  }

  
  cursor_normal(buff);
  g_free(data);
  return TRUE;
  
}






gint do_exp_mult_and_display( GtkWidget *widget, double *val )
{
  dbuff *buff;
  gint result;

  result = do_exp_mult( widget, val );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}

gint do_exp_mult( GtkWidget* widget, double* val )
{
  int i,j,i2;
  float factor;
  dbuff* buff;
  char is_symm;
  factor = (float) *val;

  // fprintf(stderr, "doing exp_mult by %f\n", factor );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];

  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_exp_mult panic! buff is null!",TRUE);
    return 0;
  }

  is_symm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check));

  // this is repeated in the fitting routine!

  for( j=0; j<buff->npts2; j++ )
    for( i=0; i<buff->npts; i++ ){
      if (is_symm){
	i2 = abs(i-buff->npts/2);
      }
      else i2 = i;
      
      buff->data[2*i+j*2*buff->npts] *= exp( -1.0 * factor * i2 * buff->param_set.dwell/1000000 * M_PI ); 
      buff->data[2*i+1+j*2*buff->npts] *= exp( -1.0 * factor * i2 * buff->param_set.dwell/1000000 * M_PI ); 
    }
  return 0;
}


gint do_gaussian_mult_and_display( GtkWidget *widget, double *val )
{
  dbuff *buff;
  gint result;

  result = do_gaussian_mult( widget, val );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}

gint do_gaussian_mult( GtkWidget* widget, double * val)

{
  int i,j,i2;
  float factor;
  float temp;
  dbuff* buff;
  char is_symm;

  factor = *val;

  // fprintf(stderr, "doing gaussian mult by %f\n",factor );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_gaussian_mult panic! buff is null!",TRUE);
    return 0;
  }
  // this is repeated in the fitting routine!
  is_symm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check))
;
  for( j=0; j<buff->npts2; j++ )
    for( i=0; i<buff->npts; i++ ) {
      if (is_symm){
	i2 = abs(i-buff->npts/2);
      }
      else i2 = i;

      temp = i2*buff->param_set.dwell/1000000 * M_PI * factor / 1.6651;
      buff->data[2*i+j*2*buff->npts] *= exp( -1 * temp * temp );
      buff->data[2*i+1+j*2*buff->npts] *= exp( -1 * temp * temp);
    }

  return 0;
}

gint do_zero_fill(GtkWidget * widget,double *val)
{
  int new_npts,old_npts,acq_points;
  float factor,temp;
  dbuff *buff;
  double ls;

  factor = *val;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_zero_fill panic! buff is null!",TRUE);
    return 0;
  }

  /* zero fill will always round up to the next power of 2
     so you can zero fill with factor of 1 to get next size up */

  temp=buff->npts*factor;
  new_npts = pow(2,ceil(log(temp-0.001)/log(2.0)));

  old_npts=buff->npts;
  acq_points=buff->acq_npts;
  
  if (new_npts ==old_npts) return 0;  
  if (old_npts>= MAX_DATA_NPTS) return 0;

  cursor_busy(buff);
  //let's try  something else
  buff_resize(buff,new_npts,buff->npts2);

  // ok, if we're here because the user pressed a button or we're actively running, then
  // change the npts.  
  // The other possibility is from an automatic upload_and_process while we're not focussed on the current buff.
  if (widget !=NULL || upload_buff == current) update_npts(new_npts); 



  // zero out the new stuff, but don't trust the new_npts

  new_npts = buff->npts;

  // reset this since the update_npts call may have messed it up.
  buff->acq_npts=acq_points;
  /* shouldn't have to do this anymore as buff resize zeros stuff out by itself
  for(j=0;j<buff->npts2;j++)
    for(i=old_npts;i<new_npts;i++){
      buff->data[2*i+2*j*new_npts] = 0.;
      buff->data[2*i+1+2*j*new_npts] = 0.;
      } */  


  // if we're symm, we want to add before and after the data set, not all at the end.

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check))){
    ls = -(buff->npts-old_npts)/2;
    do_left_shift((GtkWidget *)buff,(double *) &ls);
  }
      



  cursor_normal(buff);
  			    
  // fprintf(stderr,"do_zero_fill: got factor: %f, rounding to: %i\n",factor,new_npts);

 return 0;
}

gint do_zero_fill_and_display(GtkWidget * widget,double *val)
{
  dbuff *buff;
  gint result;
  gint old_npts;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  old_npts=buff->npts;
  result = do_zero_fill( widget, val );

  if (old_npts != buff->npts)
    draw_canvas( buff );

  return result;

}

gint do_left_shift(GtkWidget * widget,double *val)
{
  int i,j,shift;
  dbuff* buff;

  shift = (int) *val;
  

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_left_shift panic! buff is null!",TRUE);
    return 0;
  }

  // fprintf(stderr,"left_shift: shift is: %i\n",shift);
  if (shift ==0) return 0;
  if (shift > 0){
    for( j=0; j<buff->npts2; j++ ){
      for( i=shift; i<buff->npts; i++ ) {
	buff->data[2*(i-shift)+j*2*buff->npts]=
	  buff->data[2*i+j*2*buff->npts];
	buff->data[2*(i-shift)+1+j*2*buff->npts]=
	  buff->data[2*i+1+j*2*buff->npts];
      }
      for(i=buff->npts-shift-1;i<buff->npts;i++){
	buff->data[2*i+j*2*buff->npts]=0.;
	buff->data[2*i+1+j*2*buff->npts]=0.;
      }
    }
  }
  else
    for( j=0; j<buff->npts2; j++ ){
      for( i=buff->npts+shift-1;i>=0; i-- ) {
	buff->data[2*(i-shift)+j*2*buff->npts]=
	  buff->data[2*i+j*2*buff->npts];
	buff->data[2*(i-shift)+1+j*2*buff->npts]=
	  buff->data[2*i+1+j*2*buff->npts];
      }
      for(i=0;i<-shift;i++){
	buff->data[2*i+j*2*buff->npts]=0.;
	buff->data[2*i+1+j*2*buff->npts]=0.;
      }
    }



    

  return 0;


  }

gint do_left_shift_and_display(GtkWidget * widget,double *val)
{
  dbuff *buff;
  gint result;

  //  fprintf(stderr,"in do_left_shift_and_display with val = %lf\n",*val);
  result = do_left_shift( widget, val );

  if( widget == NULL ) {
    fprintf(stderr,"widget is nyull\n");
    buff = buffp[ upload_buff ];
  }
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;

  }

gint do_left_shift_2d(GtkWidget * widget,double *val)
{
  int i,j,shift;
  dbuff* buff;

  shift = (int) *val;
  

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_left_shift panic! buff is null!",TRUE);
    return 0;
  }

  // fprintf(stderr,"left_shift: shift is: %i\n",shift);
  if (shift ==0) return 0;
  if (shift*(1+buff->is_hyper) >= buff->npts2 ||
      shift*(1+buff->is_hyper) <= -1*buff->npts2 ){
    popup_msg("Left shift 2D too big!",TRUE);
    return 0;
  }
  if (shift > 0){ // left shifting - add zeros in at end.
    for(i=0;i<2*buff->npts;i++){
      for(j=shift*(1+buff->is_hyper);j<buff->npts2;j += (1+buff->is_hyper)){
	//	printf("dest is %i\n",j-shift*(1+buff->is_hyper));
	buff->data[i+(j-shift*(1+buff->is_hyper))*2*buff->npts] 
	  = buff->data[i+j*2*buff->npts];
	if (buff->is_hyper)
	  buff->data[i+(j+1-shift*(1+buff->is_hyper))*2*buff->npts] 
	    = buff->data[i+(j+1)*2*buff->npts];
      }
      // add in the zeros at the end
      for(j= buff->npts2-shift*(1+buff->is_hyper) ; j<buff->npts2;j++)
	buff->data[i+j*2*buff->npts] = 0.;
    }
  }
    else{ //right shifting.  resize buffer to keep all the data
      buff_resize(buff,buff->npts,buff->npts2-shift*(1+buff->is_hyper));
      for (i=0;i<2*buff->npts;i++){
	for (j=buff->npts2-1;j>=-shift*(1+buff->is_hyper);j--)
	  buff->data[i+j*2*buff->npts] 
	    = buff->data[i+(j+shift*(1+buff->is_hyper))*2*buff->npts];
	
	// add zeros in at beginning
	for(j=0;j<-shift*(1+buff->is_hyper);j++)
	  buff->data[i+j*2*buff->npts]=0.;
			    

      }
    }
	
  return 0;


  }

gint do_left_shift_2d_and_display(GtkWidget * widget,double *val)
{
  dbuff *buff;
  gint result;

  //  fprintf(stderr,"in do_left_shift_and_display with val = %lf\n",*val);
  result = do_left_shift_2d( widget, val );

  if( widget == NULL ) {
    fprintf(stderr,"widget is nyull\n");
    buff = buffp[ upload_buff ];
  }
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;

  }


gint do_shim_filter(GtkWidget * widget,double *val)
{
  int i,j,shift,imax;
  dbuff* buff;
  float sw;

  // this is a frequency in Hz

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_shim_filter panic! buff is null!",TRUE);
    return 0;
  }

  shift = (int) *val;
  sw = 1./buff->param_set.dwell*1e6;
  imax = sw/shift/4.;
  
  fprintf(stderr,"imax wants to be: %i\n",imax);
  if (imax > buff->npts) imax = buff->npts;
  for( j=0; j<buff->npts2; j++ )
    for( i=0; i<imax; i++ ) {
      buff->data[2*i+j*2*buff->npts] *= sin (2*M_PI*i*shift/sw);
      buff->data[2*i+1+j*2*buff->npts] *= sin(2*M_PI*i*shift/sw);
    }


    

  return 0;


  }

gint do_shim_filter_and_display(GtkWidget * widget,double *val)
{
  dbuff *buff;
  gint result;

  result = do_shim_filter( widget, val );

  if( widget == NULL ) {
    fprintf(stderr,"widget is nyull\n");
    buff = buffp[ upload_buff ];
  }
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;

  }

////


gint do_phase_wrapper( GtkWidget* widget, double *unused )

{
  dbuff* buff;
  int i;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_phase_wrapper panic! buff is null!",TRUE);
    return 0;
  }

  if ( ((int)buff->process_data[PH].val & GLOBAL_PHASE_FLAG)==0) {
    // fprintf(stderr, "Setting phase from %f, %f, to %f, %f, (local)\n", buff->phase0_app, buff->phase1_app, buff->phase0, buff->phase1 );

    for( i=0; i<buff->npts2; i++ )
      do_phase( &buff->data[i*2*buff->npts], &buff->data[ i*2*buff->npts ], 
		buff->phase0 - buff->phase0_app, buff->phase1 - buff->phase1_app, 
		buff->npts );
    buff->phase0_app = buff->phase0;
    buff->phase1_app = buff->phase1;
  }
  else {
    // fprintf(stderr, "Setting phase from %f, %f, to %f, %f, (global)\n", buff->phase0_app, buff->phase1_app, phase0, phase1 );
    for( i=0; i<buff->npts2; i++ )
      do_phase( &buff->data[i*2*buff->npts], &buff->data[ i*2*buff->npts ], 
		phase0 - buff->phase0_app, phase1 - buff->phase1_app, buff->npts );
    buff->phase0_app = phase0;
    buff->phase1_app = phase1;
  }

  return 0;

}

gint do_phase_and_display_wrapper( GtkWidget* widget, double *unused )

{
  dbuff *buff;
  gint result;

  result = do_phase_wrapper( widget,unused );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  draw_canvas( buff );
  return result;
}


///////
gint do_phase_2d_wrapper( GtkWidget* widget, double *unused )

{
  dbuff* buff;
  int i,j;
  float *pdat;
  float dp0,dp1;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_phase_wrapper panic! buff is null!",TRUE);
    return 0;
  }

  if(buff->is_hyper == 0){
    popup_msg("Can't phase 2d on non-hyper complex data",TRUE);
    return 0;
  }

  if(buff->npts2 %2 ==1){
    popup_msg("Can't phase 2d on odd number of npts2",TRUE);
    return 0;
  }

  pdat=g_malloc(buff->npts2*sizeof(float));
  // borrow 1-d global/local flag
  if ( ((int)buff->process_data[PH].val & GLOBAL_PHASE_FLAG)==0) {
    dp0=buff->phase20-buff->phase20_app;
    dp1=buff->phase21-buff->phase21_app;
    buff->phase20_app = buff->phase20;
    buff->phase21_app = buff->phase21;
  }
  else{
    dp0=phase20-buff->phase20_app;
    dp1=phase21-buff->phase21_app;
    buff->phase20_app = phase20;
    buff->phase21_app = phase21;
  }

  for( i=0; i<2*buff->npts; i++ ){
    for(j=0;j<buff->npts2;j++)
      pdat[j]=buff->data[i+j*2*buff->npts];
    do_phase(pdat,pdat,dp0,dp1,buff->npts2/2);
    for(j=0;j<buff->npts2;j++)
      buff->data[i+2*j*buff->npts] = pdat[j];
  }

  g_free(pdat);
  return 0;

}

gint do_phase_2d_and_display_wrapper( GtkWidget* widget, double *unused )

{
  dbuff *buff;
  gint result;

  result = do_phase_2d_wrapper( widget,unused );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  draw_canvas( buff );
  return result;
}

////////
gint process_button_toggle(GtkWidget *widget, int button )

{
  if (GTK_TOGGLE_BUTTON (widget)->active) {

    if( process_button[ button ].adj == NULL )
      active_process_data[button].status = PROCESS_ON;
    else
      active_process_data[ button ].status = SCALABLE_PROCESS_ON;
  }

  else {
    if( process_button[ button ].adj == NULL )
      active_process_data[ button ].status = PROCESS_OFF;
    else
      active_process_data[ button ].status = SCALABLE_PROCESS_OFF;
  }

  return 0;
}


gint process_local_global_toggle(GtkWidget *widget, int button )

{
  if (GTK_TOGGLE_BUTTON (widget)->active) {
    active_process_data[PH].val =0;
  }
  else {
    active_process_data[PH].val = GLOBAL_PHASE_FLAG;
    //    fprintf(stderr,"in toggle local/global, setting active to global\n");
  }

  return 0;
}



gint update_active_process_data( GtkAdjustment *adj, int button )

{

  double f;

  if( active_process_data == NULL )
    return -1;

  // fprintf(stderr, "updating active process data\n" );
  
  f = adj -> value;

  active_process_data[ button ].val = f;

  return 0;
}


GtkWidget* create_process_frame()

{

  char title[UTIL_LEN];
  GtkWidget *table, *frame, *button;
  GSList *group;
  int i;
  long nu;
  GtkWidget *hbox;

  active_process_data = NULL;

  for( i=0; i<MAX_PROCESS_FUNCTIONS; i++ ) {
    process_button[i].func = NULL;
    process_button[i].adj = NULL;
    process_button[i].button = NULL;
  }

  // fprintf(stderr, "creating process frame\n" );

  snprintf(title,UTIL_LEN,"Process");

  /* arguments are homogeneous and spacing */
  
  table = gtk_table_new(12,6, TRUE); /* rows, columns */
 
  /*
   *  This is where all the process buttons are set up
   */ 

  nu=CR;
  // default value actually set in buff.c 
  process_button[nu].adj= gtk_adjustment_new( 9, 1, PSRBMAX , 1, 2, 0 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00,0 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu,nu+1);
  gtk_widget_show( button );
  //  gtk_adjustment_set_value( GTK_ADJUSTMENT( process_button[nu].adj ), 11 );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), (void*) nu);
  gtk_widget_show(process_button[nu].button);

  button = gtk_button_new_with_label( "Cross correlation" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_cross_correlate_and_display), &(GTK_ADJUSTMENT(process_button[nu].adj) -> value) );
  process_button[nu].func = do_cross_correlate;
  gtk_widget_show(button);

  /*
   *  The first Baseline correct
   */

  nu=BC1;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), (void*) nu);
  gtk_widget_show(process_button[nu].button);

  button = gtk_button_new_with_label( "Baseline Correct" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_offset_cal_a_and_display), NULL);
  process_button[nu].func = do_offset_cal;
  gtk_widget_show(button);

  //

  /*
   * left shift
   */
  nu=LS;
  process_button[nu].adj= gtk_adjustment_new( 0, -10000, 10000, 1, 2, 0 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00, 0 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu,nu+1);
  gtk_widget_show( button );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), 
      (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Left Shift" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_left_shift_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_left_shift;
  gtk_widget_show(button);


  /*
   * shim-filter - should be removed...
   */
  nu=SF;
  process_button[nu].adj= gtk_adjustment_new( 0, 0, 10000000, 1, 2, 0 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00, 0 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu,nu+1);
  gtk_widget_show( button );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), 
      (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Shim Filter" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_shim_filter_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_shim_filter;
  gtk_widget_show(button);




  //
  /*
   *  Exp Mult
   */
  nu=EM;
  process_button[nu].adj =  gtk_adjustment_new( 0, -1e6, 1e6, 1, 10, 0 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT( process_button[nu].adj ), 1.00, 0 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu,nu+1);
  gtk_widget_show( button );
   
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Exp Mult" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_exp_mult_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_exp_mult;
  gtk_widget_show(button);

  /*
   *  Gaussian Mult
   */
  nu=GM;
  process_button[nu].adj= gtk_adjustment_new( 0, -1e6, 1e6, 1, 10, 1 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu );
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00, 0 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu,nu+1);
  gtk_widget_show( button );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle),  (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Gaussian Mult" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_gaussian_mult_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_gaussian_mult;
  gtk_widget_show(button);

  /*
   * Zero fill
   */
  nu=ZF;
  process_button[nu].adj= gtk_adjustment_new( 2, 1, 10, 1, 2, 0 );
  // the 2 is the default value but its actually set up in buff init in buff.c.
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00, 1 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu,nu+1);
  gtk_widget_show( button );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), 
      (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Zero Fill" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_zero_fill_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_zero_fill;
  gtk_widget_show(button);



  /*
   * ZI zero imaginary
   */
  nu=ZI;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle),  (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Zero Imag" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_zero_imag_and_display), NULL);
  process_button[nu].func = do_zero_imag;
  gtk_widget_show(button);

  /*
   * FT
   */
  nu=FT;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle),  (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "FT" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_ft_and_display), NULL);
  process_button[nu].func = do_ft;
  gtk_widget_show(button);


  /*  Bruker FT */
  nu=BFT;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle),  (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Bruker FT" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_bft_and_display), NULL);
  process_button[nu].func = do_bft;
  gtk_widget_show(button);




  /*
   * Another Baseline correct
   */  
  nu=BC2;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE( table ), process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT( process_button[nu].button ), "toggled", G_CALLBACK( process_button_toggle ),  (void*) nu);
  gtk_widget_show( process_button[nu].button );
  button = gtk_button_new_with_label( "Baseline Correct" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_offset_cal_and_display), NULL);
  process_button[nu].func = do_offset_cal;
  gtk_widget_show(button);

  /*
   *  Phase processing
   */
  nu=PH;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE( table ), process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT( process_button[nu].button ), "toggled", G_CALLBACK( process_button_toggle ),  (void*) nu);
  gtk_widget_show( process_button[nu].button );
  button = gtk_button_new_with_label( "Phase" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK( do_phase_and_display_wrapper ), NULL);
  process_button[nu].func = do_phase_wrapper;
  gtk_widget_show(button);

  /*
   *  Phase radio buttons
   */

  hbox = gtk_hbox_new(FALSE,1);
  
  button = gtk_radio_button_new_with_label( NULL, "local" );
  gtk_box_pack_start(GTK_BOX(hbox),button,FALSE,FALSE,1);
  g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(process_local_global_toggle),
		     (void *) 0);
  gtk_widget_show (button);
  r_local_button = button;

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button) );
  button = gtk_radio_button_new_with_label( group, "global" );
  gtk_box_pack_start(GTK_BOX(hbox),button,FALSE,FALSE,1);

  gtk_table_attach_defaults(GTK_TABLE(table),hbox,2,3,nu,nu+1);
  gtk_widget_show (button);
  gtk_widget_show(hbox);
  r_global_button=button;


//  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu+1,nu+2);

/*
  button = gtk_radio_button_new_with_label( NULL, "local" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(process_local_global_toggle),
		     (void *) 0);
  gtk_widget_show (button);

  r_local_button = button;

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button) );

  button = gtk_radio_button_new_with_label( group, "global" );
  r_global_button=button;
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu+1,nu+2);
  gtk_widget_show (button);
*/


  gtk_widget_show (table);

  snprintf(title,UTIL_LEN,"Process");

  frame = gtk_frame_new(title);

  gtk_container_set_border_width(GTK_CONTAINER (frame),5);
  //  gtk_widget_set_size_request(frame,800,300);

  gtk_container_add(GTK_CONTAINER(frame),table);
  gtk_widget_show(frame);

  return frame;
}


gint process_data( GtkWidget *widget, gpointer data )
{
  int i;
  process_data_t *p_data;

  //decide which process set to use

  if( widget == NULL ) {
    p_data = acq_process_data;
    // fprintf(stderr, "processing acq data\n" );
  }

  else {
    p_data = active_process_data;
    // fprintf(stderr, "processing active buffer data\n" );
  }

  for( i=0; i<MAX_PROCESS_FUNCTIONS; i++ ) {
    switch( p_data[i].status )
      {
	/*
	 *  Pass *widget to the process functions so they too know whether they are being called
	 *  by a button press or by an automatic processing sequence
	 */

      case PROCESS_ON:
	process_button[i].func( widget, NULL );
	break;

      case SCALABLE_PROCESS_ON:
	process_button[i].func( widget, &p_data[i].val );
	break;

      case PROCESS_OFF:
      case SCALABLE_PROCESS_OFF:
	break;
      default:
	fprintf(stderr, "Bad processing command\n" );
	break;
      }
  }

  // fprintf(stderr, "done processing data\n" );

  //draw canvas if this was a gtk button callback

  if( widget != NULL ) {
    draw_canvas( buffp[current] );
  }
  return 0;
}

void show_process_frame( process_data_t* process_set )
{
  int i;

  // fprintf(stderr, "Showing process frame\n" );

  active_process_data = process_set;

  for( i=0; i<MAX_PROCESS_FUNCTIONS; i++ ) {
    if( process_button[i].button != NULL ) {
      switch( process_set[i].status ) 
	{
	case NO_PROCESS:
	  break;
	  
	case PROCESS_ON:
	case SCALABLE_PROCESS_ON:
	  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( process_button[i].button ), TRUE ); 
	  break;
	  
	case PROCESS_OFF:
	case SCALABLE_PROCESS_OFF:
	  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( process_button[i].button ), FALSE ); 
	  break;
    
	default:
	  fprintf(stderr, "process button %i: invalid status is %i\n" ,i,process_set[i].status);
	  break;
	}
    }

    if( process_button[i].adj != NULL )
      gtk_adjustment_set_value( GTK_ADJUSTMENT( process_button[i].adj ), process_set[i].val );

  }

  // finally set the local/global phase flag to the value for this buff.

  if (((int)process_set[PH].val & GLOBAL_PHASE_FLAG) == 0)
    {
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(r_local_button  ), TRUE ); 
    }
  else{
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(r_global_button  ), TRUE ); 
  // fprintf(stderr, "done showing process frame\n" );
  }
  return;
}


gint do_cross_correlate_and_display( GtkWidget *widget, double *bits )
{
  dbuff *buff;
  gint result;

  result = do_cross_correlate( widget, bits );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}


gchar psrb(int bits,int init){
  // routine that generates a pseudo random bit sequence.  See Horowitz and Hill
  // pg 657 or CM-I-98

  static char inited = 0;
  static char mreg[PSRBMAX];
  static int inited_bits=0;
  static int m[PSRBMAX] = {0,0,2,3,3,5,6,0,5,7,9,0,0,0,14,0,14,11};
  static int position;

  int i;
  char first;

  if (bits < 3 || bits > PSRBMAX || bits == 8 || bits ==12 || bits == 13 || bits == 14 || bits == 16){
    fprintf(stderr,"psrb error: bits = %i. bailing\n",bits);
    return 0;
  }

			   

  if (inited == 0 && init == 0){
    fprintf(stderr,"psrb error.  Not init'ed yet, and init = 0.  Initing anyway\n");
    init = 1;
  }


  if (init == 0 && inited_bits != bits){
    fprintf(stderr,"psrb error.  Inited but with wrong number of bits.  Re-initing\n");
    init = 1;
  }
  

  if(init == 1){
    inited_bits= bits;
    for (i=0;i<PSRBMAX;i++)
      mreg[i]=1;
    inited = 1;
    position = 0;
    
  }
  /*
  if (position == len[bits-1]){
    position = 0;
    return -1;
  } // makes it so our sequence has an equal number of +1 and -1, 
  // and has a length of a power of 2. - I think this causes problems actually
  */
  position += 1;

    
  // do the shifting:


  first = mreg[bits-1]^mreg[m[bits-1]-1];
  for(i= bits-1;i>0;i--)
    mreg[i]=mreg[i-1];
  mreg[0] = first;
  return mreg[ bits-1]*2-1;


}

gint do_cross_correlate( GtkWidget *widget, double *bits )

{
  static int len[PSRBMAX] = {0,0,7,15,31,63,127,0,511,1023,2047,0,0,0,32767,0,131071,262143};

  int i, j,k;
  dbuff *buff;
  float *new_data;
  char *mreg;
  float avg1=0.,avg2=0.;
  int num_seq;
  int num_bits;
  int xsize;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_cross_correlate panic! buff is null!",TRUE);
    return 0;
  }
  
  num_bits = (int) *bits;

  if (len[num_bits-1] == 0 ){
    popup_msg("Cross Correlate: invalid number of bits",TRUE);
    return -1;
  }

  num_seq = (int) buff->npts/len[ num_bits-1 ] - 1;
  
  if (buff->npts < len[ num_bits-1 ]){
    popup_msg("do_cross_correlate: too few points for mlbs\n",TRUE);
    return 0;
  }
  /*  
      if (num_seq < 1){ 
      popup_msg("do_cross_correlate: num_seq < 1",TRUE);
      return 0;
      }
  */
  
  fprintf(stderr,"cross_correlate: num_seq is %i\n",num_seq);
      
  xsize = buff->npts;
  if (xsize < len[ num_bits - 1]*2){
    xsize = len[ num_bits - 1]*2;   
    num_seq = 1;
  }
    
  new_data = g_malloc(sizeof(float) * xsize);
  //reals are stored in: buff->data[2*i+j*2*buff->npts]
  //imag in:             buff->data[2*i+1+j*2*buff->npts]

  // array for storing bit coefficients
  mreg = g_malloc(sizeof(char) * xsize);


  //  fprintf(stderr,"in do_cross_correlate, bits: %i\n",(int) *bits);

  mreg[0] = psrb(num_bits,1);
  for (i=1;i<xsize;i++)
    mreg[i]=psrb(num_bits,0);


  for( j=0; j<buff->npts2; j++ ){  // j loops through the 2d records

    // do the reals:
    for(i=0;i< xsize ;i++) 
      new_data[i] = buff->data[2*(i%buff->npts)+j*2*buff->npts];


    // do the cross-correlation:
    for (i = 0 ; i < len[num_bits-1] ; i++){ 
      buff->data[2*i+j*2*buff->npts]=0.;
      for( k = 0 ; k < num_seq*len[num_bits-1] ; k++ ){
	buff->data[2*i+j*2*buff->npts] += mreg[k]*new_data[k+i];
      }
    }
    for (i=len[num_bits-1];i<buff->npts;i++)
      buff->data[2*i+j*2*buff->npts]= 0.;



    // now the imag:
    for(i=0;i<buff->npts;i++) 
      new_data[i] = buff->data[2*(i%buff->npts)+j*2*buff->npts+1];

    // do the cross-correlation:
    for (i=0;i< len[num_bits-1];i++){ 
      buff->data[2*i+j*2*buff->npts+1]=0.;
      for(k=0;k<num_seq* len[num_bits-1];k++){
	buff->data[2*i+j*2*buff->npts+1] += mreg[k]*new_data[k+i];
      }
    }
    for (i=len[num_bits-1];i<buff->npts;i++)
      buff->data[2*i+j*2*buff->npts+1]=0.;
 

  }
  fprintf(stderr,"cross_correlate: avg1/n %f 2: %f\n",avg1/len[num_bits-1],avg2/len[num_bits-1]);
  fprintf(stderr,"cross_correlate: len[num_bits]: %i\n",len[num_bits-1]);

  buff_resize(buff,len[num_bits-1],buff->npts2);
  if (widget !=NULL || upload_buff == current) update_npts(len[num_bits-1]); 

  g_free(new_data);
  g_free(mreg);
  return 0;
}


GtkWidget* create_process_frame_2d()

{

  char title[UTIL_LEN];
  GtkWidget *table, *frame, *button;
  //  GSList *group;
  //  int i;
  long nu;


  /* already done in create 1d 

  active_process_data = NULL;

  for( i=0; i<MAX_PROCESS_FUNCTIONS; i++ ) {
    process_button[i].func = NULL;
    process_button[i].adj = NULL;
    process_button[i].button = NULL;
  }

  */


  snprintf(title,UTIL_LEN,"Process 2D");

  /* arguments are homogeneous and spacing */
  
  table = gtk_table_new(12,6, TRUE); /* rows, columns */
 
  /*
   *  This is where all the process buttons are set up
   */ 


  /* first BC in 2nd D */


  nu=BC2D1;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE( table ), process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT( process_button[nu].button ), "toggled", G_CALLBACK( process_button_toggle ),  (void*) nu);
  gtk_widget_show( process_button[nu].button );
  button = gtk_button_new_with_label( "Baseline Correct" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_offset_cal_2D_a_and_display), NULL);
  process_button[nu].func = do_offset_cal_2D;
  gtk_widget_show(button);


  /*
   * Left shift 2d
   */
  nu=LS2D;
  process_button[nu].adj= gtk_adjustment_new( 0, -10000, 10000, 1, 2, 0 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00, 0 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu-P2D,nu-P2D+1);
  gtk_widget_show( button );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), 
      (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Left shift 2D" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_left_shift_2d_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_left_shift_2d;
  gtk_widget_show(button);

  /*
   * Exp multiply
   */
  nu=EM2D;
  process_button[nu].adj= gtk_adjustment_new( 2, -1E6, 1E6, 1, 2, 0 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00, 0 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu-P2D,nu-P2D+1);
  gtk_widget_show( button );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), 
      (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Exp Mult 2D" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_exp_mult_2d_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_exp_mult_2d;
  gtk_widget_show(button);

  /*
   * Gauss multiply
   */
  nu=GM2D;
  process_button[nu].adj= gtk_adjustment_new( 2, -1E6, 1E6, 1, 2, 0 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00, 0 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu-P2D,nu-P2D+1);
  gtk_widget_show( button );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), 
      (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Gauss Mult 2D" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_gaussian_mult_2d_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_gaussian_mult_2d;
  gtk_widget_show(button);




  /*
   * Zero fill
   */
  nu=ZF2D;
  process_button[nu].adj= gtk_adjustment_new( 2, 1, 100, 1, 2, 0 );
  g_signal_connect (G_OBJECT (process_button[nu].adj), "value_changed", G_CALLBACK (update_active_process_data), (void*) nu);
  button = gtk_spin_button_new( GTK_ADJUSTMENT(  process_button[nu].adj ), 1.00, 1 );
  gtk_spin_button_set_update_policy( GTK_SPIN_BUTTON( button ), GTK_UPDATE_IF_VALID );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu-P2D,nu-P2D+1);
  gtk_widget_show( button );

  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle), 
      (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Zero Fill 2D" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_zero_fill_2d_and_display), &(GTK_ADJUSTMENT( process_button[nu].adj ) -> value) );
  process_button[nu].func = do_zero_fill_2d;
  gtk_widget_show(button);

  /*
   * UNWIND 
   */


  nu=UNWIND;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle),  (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "Unwind phase" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(unwind_2d_and_display), NULL);
  process_button[nu].func = unwind_2d;
  gtk_widget_show(button);


  /*
   * FT
   */

  nu=FT2D;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle),  (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "FT2D" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_ft_2d_and_display), NULL);
  process_button[nu].func = do_ft_2d;
  gtk_widget_show(button);

  nu=MAG2D;
    /* 
     * convert hyper complex to simple 2d taking the magnitude in the second dimension
     */
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE(table),process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(process_button[nu].button),"toggled",G_CALLBACK(process_button_toggle),  (void*) nu);
  gtk_widget_show(process_button[nu].button);
  button = gtk_button_new_with_label( "MAG2D" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_mag_2d_and_display), NULL);
  process_button[nu].func = do_mag_2d;
  gtk_widget_show(button);





  
  /*
   *  Baseline correct in indirect dimension
   */  
  nu=BC2D2;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE( table ), process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT( process_button[nu].button ), "toggled", G_CALLBACK( process_button_toggle ),  (void*) nu);
  gtk_widget_show( process_button[nu].button );
  button = gtk_button_new_with_label( "Baseline Correct" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(do_offset_cal_2D_and_display), NULL);
  process_button[nu].func = do_offset_cal_2D;
  gtk_widget_show(button);
  /*
   *  Phase processing 2D
   */
  nu=PH2D;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE( table ), process_button[nu].button,0,1,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT( process_button[nu].button ), "toggled", G_CALLBACK( process_button_toggle ),  (void*) nu);
  gtk_widget_show( process_button[nu].button );
  button = gtk_button_new_with_label( "Phase 2D" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu-P2D,nu-P2D+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK( do_phase_2d_and_display_wrapper ), NULL);
  process_button[nu].func = do_phase_2d_wrapper;
  gtk_widget_show(button);




  /*
   *  Phase processing
   *
  nu=PH;
  process_button[nu].button = gtk_check_button_new();
  gtk_table_attach_defaults(GTK_TABLE( table ), process_button[nu].button,0,1,nu,nu+1);
  g_signal_connect(G_OBJECT( process_button[nu].button ), "toggled", G_CALLBACK( process_button_toggle ),  (void*) nu);
  gtk_widget_show( process_button[nu].button );
  button = gtk_button_new_with_label( "Phase" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,1,2,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK( do_phase_and_display_wrapper ), NULL);
  process_button[nu].func = do_phase_wrapper;
  gtk_widget_show(button);

  
  //   Phase radio buttons
   
  button = gtk_radio_button_new_with_label( NULL, "local" );
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu,nu+1);
  g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(process_local_global_toggle),
		     (void *) 0);
  gtk_widget_show (button);

  r_local_button = button;

  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button) );

  button = gtk_radio_button_new_with_label( group, "global" );
  r_global_button=button;
  gtk_table_attach_defaults(GTK_TABLE(table),button,2,3,nu+1,nu+2);
  gtk_widget_show (button);


*/
  gtk_widget_show (table);

  snprintf(title,UTIL_LEN,"Process 2D");

  frame = gtk_frame_new(title);

  gtk_container_set_border_width(GTK_CONTAINER (frame),5);
  //  gtk_widget_set_size_request(frame,800,300);

  gtk_container_add(GTK_CONTAINER(frame),table);
  gtk_widget_show(frame);

  return frame;
}



gint do_zero_fill_2d(GtkWidget * widget,double *val)
{
  int new_npts2,old_npts2,acq_points2;
  float factor,temp;
  dbuff *buff;
  int i,j,shift;

  factor = (float ) *val;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_zero_fill_2d panic! buff is null!",TRUE);
    return 0;
  }
  
  /* zero fill will always round up to the next power of 2
     so you can zero fill with factor of 1 to get next size up */

  temp=buff->npts2*factor; 
  new_npts2 = pow(2,ceil(log(temp-0.001)/log(2.0)));

  old_npts2=buff->npts2;
  acq_points2=buff->param_set.num_acqs_2d;
  
  if (new_npts2 ==old_npts2) return 0;  
  if (old_npts2>= MAX_DATA_NPTS) return 0;

  cursor_busy(buff);
  //let's try  something else
  buff_resize(buff,buff->npts,new_npts2);
  if (widget !=NULL || upload_buff == current) update_npts2(new_npts2); 



  // zero out the new stuff, but don't trust the new_npts

  new_npts2 = buff->npts2;

  // the update will have screwed this up, fix it:
  buff->param_set.num_acqs_2d=acq_points2; 

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check))){
    // move the data back to the center
    shift = (new_npts2-old_npts2)/2;
    for(i=0;i<buff->npts*2;i++)
      for(j=old_npts2-1;j>=0;j--){
	buff->data[i+(shift+j)*buff->npts*2]=buff->data[i+j*buff->npts*2];
	buff->data[i+j*buff->npts*2] = 0.;
      }

  }



      
  cursor_normal(buff);



  // fprintf(stderr,"do_zero_fill: got factor: %f, rounding to: %i\n",factor,new_npts);

 return 0;
}

gint do_zero_fill_2d_and_display(GtkWidget * widget,double *val)
{
  dbuff *buff;
  gint result;
  gint old_npts2;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  old_npts2=buff->npts2;
  result = do_zero_fill_2d( widget, val );

  if (old_npts2 != buff->npts2)
    draw_canvas( buff );

  return result;

}


//un

gint unwind_2d_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  result = unwind_2d( widget, unused );

  draw_canvas( buff );
   return result;
}

gint unwind_2d(GtkWidget *widget, double *unused)

{
  /* unwind some phase from the second dimension 
     for crackpot autoshimming feature. */

  dbuff *buff;
  int i,j;
  float re,im,freq,phase;
  float mag,ph;
  double tau;
  int npts,npts2;

  if( widget == NULL ) {
    buff = buffp[ upload_buff ];
    // fprintf(stderr,"do_ft- on buffer %i\n",upload_buff );
  }
  else {
    buff = buffp[ current ];
    // fprintf(stderr,"do_ft- on buffer %i\n",current );
  }
  if (buff == NULL){
    popup_msg("unwind_2d panic! buff is null!",TRUE);
    return 0;
  }
  
   
  if (buff->is_hyper == FALSE){
    popup_msg("Not hyper, can't unwind phase",TRUE);
    return TRUE;
  }
  
  // need to find a tau in the parameter set.

  i = pfetch_float(&buff->param_set,"tau",&tau,0);
  if (i == 0) {
    popup_msg("no parameter tau found for phase unwind",TRUE);
    return TRUE;
  }
  fprintf(stderr,"got tau: %lf\n",tau);
  
  npts = buff->npts;
  npts2 = buff->npts2;
  
  for (i=0;i<npts;i++){
    // what's the freq at this point?
    freq = - ( (double) i * 1./buff->param_set.dwell*1e6/npts
	       - (double) 1./buff->param_set.dwell*1e6/2.);
    // so the phase to unwind is:
    phase = freq*tau*2*M_PI;
    
    for(j=0;j<npts2/2;j++){
      re = buff->data[(2*j)*npts*2+2*i];
      im = buff->data[(2*j+1)*npts*2+2*i];
      mag = sqrt(re*re+im*im);
      ph = atan2(im,re);
      ph = ph-phase;
      buff->data[(2*j)*npts*2+2*i] = mag*cos(ph);
      buff->data[(2*j+1)*npts*2+2*i] = mag*sin(ph);
      
      re = buff->data[(2*j)*npts*2+2*i+1];
      im = buff->data[(2*j+1)*npts*2+2*i+1];
      mag = sqrt(re*re+im*im);
      ph = atan2(im,re);
      ph = ph-phase;
      buff->data[(2*j)*npts*2+2*i+1] = mag*cos(ph);
      buff->data[(2*j+1)*npts*2+2*i+1] = mag*sin(ph);
      
      
      
      
    }
  }
  
  
  return TRUE;
  
}




gint do_ft_2d_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  result = do_ft_2d( widget, unused );

  draw_canvas( buff );
   return result;
}

gint do_ft_2d(GtkWidget *widget, double *unused)

{
  /* do fourier transform in 2nd dimension */

  dbuff *buff;
  int i,j;
  float spare;
  float scale;
  float *new_data;
  double spared;
  char is_symm;
  char true_complex;

  if( widget == NULL ) {
    buff = buffp[ upload_buff ];
    // fprintf(stderr,"do_ft- on buffer %i\n",upload_buff );
  }
  else {
    buff = buffp[ current ];
    // fprintf(stderr,"do_ft- on buffer %i\n",current );
  }
  if (buff == NULL){
    popup_msg("do_ft_2d panic! buff is null!",TRUE);
    return 0;
  }

  is_symm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check));
  true_complex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.true_complex));
  

  // can't have hyper and true complex at the same time.
  // if true complex is active, then we say its true complex.

					      

  /*********

  if(buff->win.press_pend>0) {
    fprintf(stderr,"ft: press_pend>0, quitting\n");
    return TRUE;  //standard exit
  }

  **********/

  buff->flags ^= FT_FLAG2; //toggle the ft flag
  
  /* do a zero fill with a factor of 1 to make 
     sure we have a power of two as npts 

  - or, do a factor of 2 so we have room to put in the imaginaries - if not hyper*/
  if (buff->is_hyper || true_complex )
    spared= 1.0;
  else
    spared=2.0;
  do_zero_fill_2d(widget,&spared);

    //  fprintf(stderr,"in 2dft just did zero fill\n");
  cursor_busy(buff);

  if (buff->flags & FT_FLAG2)
    scale = buff->npts2/4.0;
  else
    scale = 2.0;
  
  if (true_complex){
    printf("doing true_complex FT2d\n");
    new_data = g_malloc(buff->npts2*sizeof(float)*2);
    if (new_data == NULL) fprintf(stderr,"failed to malloc!\n");
    
    
    // copy out
    for(i=0;i<buff->npts;i++){
      for (j=0;j<buff->npts2;j++){
	new_data[j*2] = buff->data[j*buff->npts*2+2*i];
	new_data[j*2+1] = buff->data[j*buff->npts*2+2*i+1];
      }
      // do the ft
      four1(new_data-1,buff->npts2,1);

      // descramble
      for(j=0;j<buff->npts2;j++){
	spare = new_data[j]/scale;
	new_data[j]=new_data[j+buff->npts2]/scale;
	new_data[j+buff->npts2]=spare;
      }
      // copy back:
      for (j=0;j<buff->npts2;j++){
	buff->data[j*buff->npts*2+2*i] = new_data[2*j];
	buff->data[j*buff->npts*2+2*i+1] = new_data[2*j+1];
      }
    }
  }
  else if (buff->is_hyper == FALSE){ 
    //    popup_msg("hypercomplex flag not set, doing real FT?",TRUE);
    fprintf(stderr,"hypercomplex flag not set, doing real FT\n");
    new_data = g_malloc(buff->npts2 * sizeof(float) );
    //  fprintf(stderr,"2dft did malloc, 2dnpts = %i\n",buff->npts2);
    if (new_data == NULL) fprintf(stderr,"failed to malloc!\n");

    // copy out
    for(i=0;i<buff->npts*2  ;i++){
      for(j=0;j<buff->npts2/2;j++){
	new_data[j*2] = buff->data[j*buff->npts*2+i];
	new_data[j*2+1] = 0.;
      }
      // do the ft


      // correct the first point if we're going forward... and not symmetric
      //      if (buff->flags & FT_FLAG2 & !is_symm)
      //	new_data[0] /= 2.;
      // naw dont.  Not doing this only ever introduces a baseline offset.

      if (is_symm)
	for(j=0;j<buff->npts2/2;j++){
	  spare=new_data[j]/scale;
	  new_data[j]=new_data[j+buff->npts2/2]/scale;
	  new_data[j+buff->npts2/2]=spare;
	}

      four1(new_data-1,buff->npts2/2,1);

      // descramble
      for(j=0;j<buff->npts2/2;j++){
	spare=new_data[j]/scale;
	new_data[j]=new_data[j+buff->npts2/2]/scale;
	new_data[j+buff->npts2/2]=spare;
      }

      //copy back
      for(j=0;j<buff->npts2/2;j++){
	buff->data[2*j*buff->npts*2+i] = new_data[2*j];
	buff->data[(2*j+1)*buff->npts*2+i] = new_data[2*j+1];
      }
      // turn on the is_hyper_flag!
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buff->win.hypercheck),TRUE);
    }
      

  }
  else {// is hypercomplex
    new_data = g_malloc(buff->npts2 * sizeof(float));
    //  fprintf(stderr,"2dft did malloc, 2dnpts = %i\n",buff->npts2);
    if (new_data == NULL) fprintf(stderr,"failed to malloc!\n");
    // copy out
    for(i=0;i<buff->npts*2  ;i++){
      for(j=0;j<buff->npts2/2;j++){
	new_data[j*2] = buff->data[2*j*buff->npts*2+i];
	new_data[j*2+1] = buff->data[(2*j+1)*buff->npts*2+i];
      }
      // do the ft
      if (buff->flags & FT_FLAG2 & !is_symm)
	new_data[0] /= 2.;

      if (is_symm)
	for(j=0;j<buff->npts2/2;j++){
	  spare=new_data[j]/scale;
	  new_data[j]=new_data[j+buff->npts2/2]/scale;
	  new_data[j+buff->npts2/2]=spare;
	}

      four1(new_data-1,buff->npts2/2,1);
      
      // descramble
      for(j=0;j<buff->npts2/2;j++){
	spare=new_data[j]/scale;
	new_data[j]=new_data[j+buff->npts2/2]/scale;
	new_data[j+buff->npts2/2]=spare;
      }
      
      // copy back 
      for(j=0;j<buff->npts2/2;j++){
	buff->data[2*j*buff->npts*2+i] = new_data[j*2];
	buff->data[(2*j+1)*buff->npts*2+i] = new_data[j*2+1];
      }
    }
  }
  //  fprintf(stderr,"2dft about to free buffer\n");

  cursor_normal(buff);
  g_free(new_data);
  return TRUE;
  
}



gint do_mag_2d_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  result = do_mag_2d( widget, unused );

  draw_canvas( buff );
   return result;
}

gint do_mag_2d(GtkWidget *widget, double *unused)

{
  /* take magnitude in second dimension */

  dbuff *buff;
  int i,j;

  if( widget == NULL ) {
    buff = buffp[ upload_buff ];
    // fprintf(stderr,"do_ft- on buffer %i\n",upload_buff );
  }
  else {
    buff = buffp[ current ];
  }
  if (buff == NULL){
    popup_msg("do_mag_2d panic! buff is null!",TRUE);
    return 0;
  }

  /*********

  if(buff->win.press_pend>0) {
    fprintf(stderr,"mag2d: press_pend>0, quitting\n");
    return TRUE;  //standard exit
  }

  **********/


  if (!buff->is_hyper){
    popup_msg("buff wasn't hyper, can't take mag2d",TRUE);
    return TRUE;
  }

  // turn off the is_hyper flag:
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buff->win.hypercheck),FALSE);

  for (j=0;j<buff->npts2/2;j++){
    for(i=0;i<buff->npts*2;i++)
      buff->data[j*buff->npts*2+i] = 
	sqrt(buff->data[2*j*buff->npts*2+i]*buff->data[2*j*buff->npts*2+i]
	     +buff->data[(2*j+1)*buff->npts*2+i]*buff->data[(2*j+1)*buff->npts*2+i]);
  }				     
    // free the extra memory:
    buff_resize(buff,buff->npts,buff->npts2/2);
  


  return TRUE;
  
}





gint do_exp_mult_2d_and_display( GtkWidget *widget, double* val )
{
  dbuff *buff;
  gint result;

  result = do_exp_mult_2d( widget, val );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}

gint do_exp_mult_2d( GtkWidget* widget, double * val )
{
  int i,j,i2;
  float factor;
  double dwell2,sw2;
  dbuff* buff;
  char is_symm;

  factor = (float) *val;

  // fprintf(stderr, "doing exp_mult by %f\n", factor );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];

  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_exp_mult_2d panic! buff is null!",TRUE);
    return 0;
  }

  is_symm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check));
  //eegads, what do I use as sw in second dimension? look for a parameter called dwell2

  i = pfetch_float(&buff->param_set,"dwell2",&dwell2,0);
  if (i == 1)
    sw2 =1.0/dwell2;
  else
    pfetch_float(&buff->param_set,"sw2",&sw2,0);
  if (i==1)
    dwell2=1./sw2;
  else {
    fprintf(stderr,"can't find dwell2 or sw2, bailing out of em2d\n");
    return 0;
  }



  for( j=0; j<buff->npts2; j++ ){
    i2 = floor(j/(1+buff->is_hyper));
    if (is_symm)
      i2 -= buff->npts2/(2*(1+buff->is_hyper));
    
    factor = exp ( - *val * i2 * dwell2 *M_PI);

    for( i=0; i<buff->npts; i++ ){
      buff->data[2*i+j*2*buff->npts] *= factor  ; 
      buff->data[2*i+1+j*2*buff->npts] *= factor ; 
    }
  }
  return 0;
}




gint do_gaussian_mult_2d_and_display( GtkWidget *widget, double* val )
{
  dbuff *buff;
  gint result;

  result = do_gaussian_mult_2d( widget, val );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}

gint do_gaussian_mult_2d( GtkWidget* widget, double * val )
{
  int i,j,i2;
  float factor;
  double dwell2,sw2;
  dbuff* buff;
  char is_symm;
  factor = (float) *val;

  // fprintf(stderr, "doing exp_mult by %f\n", factor );

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];

  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_exp_mult_2d panic! buff is null!",TRUE);
    return 0;
  }
  is_symm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buff->win.symm_check));

  //eegads, what do I use as sw in second dimension? look for a parameter called dwell2

  i = pfetch_float(&buff->param_set,"dwell2",&dwell2,0);
  if (i == 1)
    sw2 =1.0/dwell2;
  else
    pfetch_float(&buff->param_set,"sw2",&sw2,0);
  if (i==1)
    dwell2=1./sw2;
  else {
    fprintf(stderr,"can't find dwell2 or sw2, bailing out of em2d\n");
    return 0;
  }


  for( j=0; j<buff->npts2; j++ ){
    i2 = floor(j/(1+buff->is_hyper));
    if (is_symm)
      i2 -= buff->npts2/(2*(1+buff->is_hyper));
    //    fprintf(stderr,"gauss 2d %i %i\n",j,i2);

    factor = dwell2 *M_PI * *val/1.6651 * i2;

    for( i=0; i<buff->npts; i++ ){
      buff->data[2*i+j*2*buff->npts] *= exp(-1*factor*factor) ; 
      buff->data[2*i+1+j*2*buff->npts] *= exp(-1*factor*factor) ; 
    }
  }
  return 0;
}


gint do_offset_cal_2D_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  result = do_offset_cal_2D( widget, unused );


  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}


gint do_offset_cal_2D( GtkWidget *widget, double *unused )

{
  int i, j;
  int count;
  float offset;
  dbuff *buff;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_ft_offset_cal_2D panic! buff is null!",TRUE);
    return 0;
    
  }
  //  fprintf(stderr,"%d %d\n\n", buff->npts, buff->npts2);

  if (buff->is_hyper == 1){
    for( j=0;j<2*buff->npts;j++) {
      
      count = 0;
      offset = 0.0;
      
      //first determine the offset for the real channel (in indirect dimension)
      for(i=buff->npts2/2*0.9 ; i<buff->npts2/2;i++){
	offset += buff->data[4*i*buff->npts+j];
	count ++;
      }
      offset=offset/count;
      for(i=0;i<buff->npts2/2;i++)
	buff->data[4*i*buff->npts+j] -= offset;


      // now do imag
      count = 0;
      offset = 0.0;
      
      //first determine the offset for the real channel (in indirect dimension)
      for(i=buff->npts2/2*0.9 ; i<buff->npts2/2;i++){
	offset += buff->data[(2*i+1)*2*buff->npts+j];
	count ++;
      }
      offset=offset/count;
      for(i=0;i<buff->npts2/2;i++)
	buff->data[(2*i+1)*2*buff->npts+j] -= offset;
    }
  }


  else{ // isn't hypercomplex
    fprintf(stderr,"doing real baseline correct\n");
    for( j=0;j<2*buff->npts;j++) {
      count = 0;
      offset = 0.0;
      for (i=buff->npts2*0.9 ; i<buff->npts2; i++){
	offset += buff->data[2*i*buff->npts+j];
	count++;
      }
      offset = offset/count;
      for(i=0;i<buff->npts2;i++){
	buff->data[2*i*buff->npts+j] -= offset;
      }
    }
  }




  return 0;
}



gint do_offset_cal_2D_a_and_display( GtkWidget *widget, double *unused )
{
  dbuff *buff;
  gint result;

  result = do_offset_cal_2D_a( widget, unused );


  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];

  draw_canvas( buff );
  return result;
}


gint do_offset_cal_2D_a( GtkWidget *widget, double *unused )

{
  dbuff *buff;
  double spared;
  int i,j;
  float *new_data,scale;

  if( widget == NULL ) 
    buff = buffp[ upload_buff ];
  else 
    buff = buffp[ current ];
  if (buff == NULL){
    popup_msg("do_ft_offset_cal_2D panic! buff is null!",TRUE);
    return 0;
    
  }
  //  fprintf(stderr,"%d %d\n\n", buff->npts, buff->npts2);
  fprintf(stderr,"npts2: %i\n",buff->npts2);


  if (buff->npts2 < 8 ) return 0; // forget it!


  spared=1.0;
  do_zero_fill_2d(widget,&spared);

  if (buff->is_hyper)
    spared= 1.0;
  else
    spared=2.0;

  new_data = g_malloc(buff->npts2 * sizeof(float)*spared );
  //  fprintf(stderr,"2dft did malloc, 2dnpts = %i\n",buff->npts2);
  if (new_data == NULL) fprintf(stderr,"failed to malloc!\n");

    //  fprintf(stderr,"in 2dft just did zero fill\n");
  cursor_busy(buff);
  scale=(float) buff->npts2/2*spared;


  if (buff->is_hyper == FALSE){

    // copy out
    for(i=0;i<buff->npts*2  ;i++){
      for(j=0;j<buff->npts2;j++){
	new_data[j*2] = buff->data[j*buff->npts*2+i];
	new_data[j*2+1] = 0.;
      }
    
      // do the ft
      
      four1(new_data-1,buff->npts2,1);
      new_data[0] = (new_data[2]+new_data[2*buff->npts2-2])/2.0;
      new_data[1] = (new_data[3]+new_data[2*buff->npts2-1])/2.0;
      four1(new_data-1,buff->npts2,-1);
      for(j=0;j<buff->npts2;j++){
	 buff->data[j*buff->npts*2+i] = new_data[j*2]/scale;
      }	   
    }
  }
  else{
    // copy out
    for(i=0;i<buff->npts*2  ;i++){
      for(j=0;j<buff->npts2/2;j++){
	new_data[j*2] = buff->data[2*j*buff->npts*2+i];
	new_data[j*2+1] = buff->data[(2*j+1)*buff->npts*2+i];
      }
      // do the ft
      four1(new_data-1,buff->npts2/2,1);
      new_data[0] = (new_data[2]+new_data[buff->npts2-2])/2.0;
      new_data[1] = (new_data[3]+new_data[buff->npts2-1])/2.0;
      four1(new_data-1,buff->npts2/2,-1);

      
      // copy back 
      for(j=0;j<buff->npts2/2;j++){
	buff->data[2*j*buff->npts*2+i] = new_data[j*2]/scale;
	buff->data[(2*j+1)*buff->npts*2+i] = new_data[j*2+1]/scale;
      }


    }
  }
  //  fprintf(stderr,"2dft about to free buffer\n");

  cursor_normal(buff);
  g_free(new_data);

  
  return 0;
}



