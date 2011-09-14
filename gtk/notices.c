#include <string.h>

#include "config.h"
#include "global.h"
#include "messages.h"
#include "notices.h"

extern int action;

int write_show_warning(void)
{
#if FIXME
  extern Frame main_frame;
  Xv_notice notice;
  int notice_stat;
  
  notice = xv_create(main_frame, NOTICE,
		     NOTICE_MESSAGE_STRINGS,
		     "The file already exist.",
		     "Do you want to overwrite the file?",
		     NULL,
		     NOTICE_BUTTON_YES, "Yes",
		     NOTICE_BUTTON_NO, "No",
		     NOTICE_STATUS, &notice_stat,
		     XV_SHOW, TRUE,
		     NULL);

  switch(notice_stat)
    {
    case NOTICE_YES:
      xv_destroy_safe(notice);
      return(1);
      break;
      
    case NOTICE_NO:
      xv_destroy_safe(notice);
      return(0);
      break;
    }
#endif
} 



void warn_textsw_almost_full(void)
{
#if FIXME
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
#endif
} 



int textsw_full_show_warning(
     char *filename)
{
#if FIXME
	
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
#endif
} 


     
void all_show_warning_proc(char *cmd)
{
#ifdef FIXME
  extern Frame main_frame;
  Xv_notice notice;
  int notice_stat;

    
  notice = xv_create(main_frame, NOTICE,
		     NOTICE_MESSAGE_STRINGS,
		     "CAUTION - no buffer for current data array.",
		     "Reallocation will not cause loss of overlapping space",
		     "but if reallocation involves reduction, those parts",
		     "no longer allocated will be LOST and GONE.",
		     "no longer allocated will be LOST and GONE.", 
		     NULL,
		     NOTICE_BUTTON_YES, "Yes",
		     NOTICE_BUTTON_NO, "No",
		     NOTICE_STATUS, &notice_stat,
		     XV_SHOW, TRUE,
		     NULL);

  switch (notice_stat)
    {
    case NOTICE_YES:
      /* continue processing */
      set_left_footer("Input number of rows and columns for reallocation");
      set_cmd_prompt("NROW NCOL: ");
      action = ALL_NEED_ROW_COL;  
      break;
    case NOTICE_NO:
      action = MAIN;
      set_left_footer("Aborted!");
      break;
    }
  
  xv_destroy_safe(notice);
#endif
}

