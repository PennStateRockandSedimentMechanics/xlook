#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include <assert.h>

#include "config.h"
#include "global.h"
#include "messages.h"
#include "notices.h"

extern int action;

int write_show_warning(void)
{
	GtkWidget *dialog, *label, *content_area;
	int result= 0;

	dialog = gtk_dialog_new_with_buttons (NULL,
		GTK_WINDOW(ui_globals.main_window),
		GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_YES, // _OK
		GTK_RESPONSE_ACCEPT,
		GTK_STOCK_NO, // _CANCEL
		GTK_RESPONSE_REJECT,
		NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	label = gtk_label_new ("The file already exists.\n\nDo you want to overwrite the file?");

	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (content_area), label);
	gtk_widget_show_all(dialog);

	switch(gtk_dialog_run (GTK_DIALOG (dialog)))
	{
		case GTK_RESPONSE_ACCEPT:
			result= 1;
			break;
		case GTK_RESPONSE_REJECT:
		default:
			result= 0;
			break;
	}
	gtk_widget_destroy (dialog);

	return result;
} 

void all_show_warning_proc(char *cmd)
{
	GtkWidget *dialog, *label, *content_area;
	int result= 0;

	dialog = gtk_dialog_new_with_buttons (NULL,
		GTK_WINDOW(ui_globals.main_window),
		GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_YES, // _OK
		GTK_RESPONSE_ACCEPT,
		GTK_STOCK_NO, // _CANCEL
		GTK_RESPONSE_REJECT,
		NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	label = gtk_label_new ("CAUTION - no buffer for current data array.\nReallocation will not cause loss of overlapping space but if reallocation involves reduction, those parts no longer allocated will be LOST and GONE.");

	/* Add the label, and show everything we've added to the dialog. */
	gtk_container_add (GTK_CONTAINER (content_area), label);
	gtk_widget_show_all(dialog);

	switch(gtk_dialog_run (GTK_DIALOG (dialog)))
	{
		case GTK_RESPONSE_ACCEPT:
			set_left_footer("Input number of rows and columns for reallocation");
			set_cmd_prompt("NROW NCOL: ");
			ui_globals.action = ALL_NEED_ROW_COL;  
			break;
		case GTK_RESPONSE_REJECT:
		default:
			ui_globals.action = MAIN;
			set_left_footer("Aborted!");
			break;
	}
	gtk_widget_destroy (dialog);

	return result;
}



#if false
void warn_textsw_almost_full(void)
{
  extern Frame main_frame;
  Xv_notice notice;
  int notice_stat;
  
  notice = xv_create(main_frame, NOTICE,
		     NOTICE_MESSAGE_STRINGS,
		     "The message window is almost out of memory",
		     "You can either discard the whole content,",
		     "or you can save it in a file.",
		     "Click on the message window with the right button",
		     "to get the menu.",
		     NULL,
		     NOTICE_BUTTON_YES, "OK",
		     NOTICE_STATUS, &notice_stat,
		     XV_SHOW, TRUE,
		     NULL);

  switch(notice_stat)
    {
    case NOTICE_YES:
      xv_destroy_safe(notice);
      break;
    }
} 

int textsw_full_show_warning(
     char *filename)
{
  extern Frame main_frame;
  Xv_notice notice;
  int notice_stat;
  char name[256];
  
  sprintf(name, "Select Save to save the messages in %s", filename);
  
  
  notice = xv_create(main_frame, NOTICE,
		     NOTICE_MESSAGE_STRINGS,
		     "The message window is almost full.",
		     name,
		     "or select Discard to flush the messages. ",
		     NULL,
		     NOTICE_BUTTON, "Save", 100,
		     NOTICE_BUTTON, "Discard", 101,
		     NOTICE_STATUS, &notice_stat,
		     XV_SHOW, TRUE,
		     NULL);
  
  switch(notice_stat)
    {
    case 100:
      xv_destroy_safe(notice);
      return 0;
      break;

    case 101:
      xv_destroy_safe(notice);
      return 1;
      break;
    }
} 
#endif
