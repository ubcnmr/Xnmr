#include <gtk/gtk.h>
#include <stdio.h>


gint popup_no_update_ok(gpointer *dialog) 
{ 
  
  gtk_widget_destroy(GTK_WIDGET(dialog)); 
  return FALSE; 
  
} 

gint popup_msg( char* msg )  
     
{  
  GtkWidget* dialog;  
  GtkWidget* button;  
  GtkWidget* label;  
  
  dialog = gtk_dialog_new(); 
  
  gtk_window_set_modal( GTK_WINDOW( dialog ), TRUE ); 
  label = gtk_label_new ( msg ); 
  button = gtk_button_new_with_label("OK"); 
  g_signal_connect_swapped (GTK_OBJECT (button), "clicked",  
  			     G_CALLBACK (popup_no_update_ok), GTK_OBJECT( dialog ) ); 
  gtk_box_pack_start (GTK_BOX ( GTK_DIALOG(dialog)->action_area ),button,FALSE,FALSE,0); 
  
  gtk_container_set_border_width( GTK_CONTAINER(dialog), 5 ); 
  
  gtk_box_pack_start ( GTK_BOX( (GTK_DIALOG(dialog)->vbox) ), label, FALSE, FALSE, 5 ); 
  
  gtk_widget_show_all (dialog); 
  
  return FALSE; 
}




gint adjust_callback(GtkAdjustment *adj,gpointer data){
  printf("in adjustment callback with value: %f\n",adj->value);
  if ( adj->value >1)
    popup_msg("value greater than 1");
  return 0; 
} 


gint delete_event(GtkWidget *widget, int * data) 
{ 
  printf("got delete event, with data: %i\n",(int) *data);
  gtk_main_quit(); 
  return FALSE; 
} 



int main(int argc,char *argv[]) 
     
{ 
  
  // build a simple dialog with a spin button 
  
  GtkWidget *window, *spin;  
  GtkObject *adj; 
  int data;

  data=5;
  
  gtk_init(&argc, &argv); 
  
  window=gtk_window_new( GTK_WINDOW_TOPLEVEL ); 
  g_signal_connect (G_OBJECT( window), "destroy", G_CALLBACK(delete_event),&data); 
  
  adj = gtk_adjustment_new(1,0.5,1000000,.01,10,0 );  
  
  spin = gtk_spin_button_new( GTK_ADJUSTMENT( adj ), 0.5, 4 );  
  
  
  g_signal_connect (G_OBJECT (adj), "value_changed",   
		      G_CALLBACK (adjust_callback), 0 );  
  
  
  gtk_container_add ( GTK_CONTAINER( window) , spin );  
  gtk_widget_show_all (window);  
  
  gtk_main(); 
  
  return FALSE;  
} 














