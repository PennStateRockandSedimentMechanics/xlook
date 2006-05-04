#include <math.h>
#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/sel_attrs.h>
#include <xview/textsw.h>
#include "global.h"

extern int active_window;
extern int old_active_window;
extern int total_windows;
extern int action;
extern char msg[MSG_LENGTH];

/* main_frame objects */
extern Frame main_frame;
extern Panel_item cmd_panel_item;
extern Panel main_frame_info_panel;
extern Panel_item active_win_info, active_plot_info, active_file_info;
extern Textsw msgwindow;
extern Textsw fileinfo_window;
extern int textsw_total_used;
extern int textsw_memory_limit;
extern int textsw_saves;


void print_msg(char *);
void set_cmd_prompt(char []);
void set_left_footer(char []);
void print_info(void);



void top()
{
  /* default prompt */

  						/* clear the file info window */
  textsw_reset(fileinfo_window, 0, 0);
 
  /* print the new file info */
  print_info(); 
  textsw_normalize_view(fileinfo_window, 1);
 
  sprintf(msg, "\n");
  print_msg(msg);
  set_left_footer("Type a command");
  set_cmd_prompt("Command: ");
  global_error = FALSE; /*reset error flag, which may have been true for last command*/
}


void name_col(colnum)
     int colnum;
{
  /* prompt for the name and unit (for naming) */
  sprintf(msg, "Type the name and unit for the new column (# %d)", colnum);
  set_left_footer(msg);
  set_cmd_prompt("Name, Unit: ");
}


/*print_fileinfo(txt)
     char txt[MSG_LENGTH];*/
void print_fileinfo(txt)
char *txt;
{
  /* prints the current data states in info window */

  int len = strlen(txt);
  if (len == 0) return;
  
  textsw_insert(fileinfo_window, txt, len);    
}


  			/* prints a message in the message subwindow */
void print_msg(txt)
     char *txt;
{
  int len;
  /*Menu m;*/
  /*int notice_msg;*/
  char filename[20];
  
  len = strlen(txt);
  if (len == 0) 
	return;
    
  		/* if buffer is half full, show a warning */
  if ((textsw_total_used <= 0.5*textsw_memory_limit)
      && textsw_total_used+len >= 0.5*textsw_memory_limit)
    warn_textsw_almost_full();
  
  textsw_total_used += len;
  
  /* if buffer is almost full, force a file save or clear buffer */
  if (textsw_total_used >= 0.8*textsw_memory_limit)
    {
      sprintf(filename, ".xlook_msgs_%d", textsw_saves);
      if (textsw_full_show_warning(filename) == 0)
	{
	  if(textsw_store_file(msgwindow, filename, 0, 0) != 0)
		printf("Error. Text could not be saved. See file messages.c\n");
	  else
	  	textsw_saves++;
	}
      textsw_reset(msgwindow, 0, 0);	
      /*textsw_erase(msgwindow, 0, TEXTSW_INFINITY);*/
      textsw_total_used = 0;
    }
  
  /* print txt in msg window */
/*printf("textsw_total_used = %d  textsw_memory_limit=%d\n",textsw_total_used,textsw_memory_limit);*/
  textsw_insert(msgwindow, txt, len);

}

      
void set_cmd_prompt(txt)
     char txt[256];
{
  xv_set(cmd_panel_item, PANEL_LABEL_STRING, txt, NULL);
}


void set_left_footer(txt)
     char txt[256];
{
  xv_set(main_frame, FRAME_LEFT_FOOTER, txt, NULL);
}


void display_active_window(aw)
     int aw;
{
  char string[20];
  
  if (aw > 0) sprintf(string, "Active Window: %d", aw);
  else sprintf(string, "Active Window: NONE");
  
  xv_set(active_win_info, PANEL_LABEL_STRING, string, NULL);
}


void display_active_plot(ap)
     int ap;
{
  char string[20];
  
  if (ap > 0) sprintf(string, "Active Plot: %d", ap);
  else sprintf(string, "Active Plot: NONE");
  
  xv_set(active_plot_info, PANEL_LABEL_STRING, string, NULL);
}

void display_active_file(af)
     int af;
{
  char string[50];
  
  if (af > 0) sprintf(string, "File: %s", head.title);
  else sprintf(string, "File: NONE");
  
  xv_set(active_file_info, PANEL_LABEL_STRING, string, NULL);
}



void print_info()
{
  int i ;
  char tmp1[MSG_LENGTH];
  char tmp2[MSG_LENGTH];
  
  sprintf(tmp1, "ALLOCATION: max_col = %d , max_row = %d\t",max_col,max_row);
  sprintf(tmp2, "number of records = %d\n ",head.nrec);
  strcat(tmp1, tmp2);
  print_fileinfo(tmp1);
  
  tmp1[0] = '\0';  
  for( i = 1; i < max_col ; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  sprintf(tmp2,"       col %1d",i) ;
	  strcat(tmp1, tmp2);
	}
    }
  strcat(tmp1, "\n");
  print_fileinfo(tmp1);
  
  tmp1[0] = '\0';
  for( i = 1; i < max_col ; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  sprintf(tmp2,"%12s",head.ch[i].name) ; 
	  strcat(tmp1, tmp2);
	}
    } 
  strcat(tmp1, "\n");
  print_fileinfo(tmp1);
  
  tmp1[0] = '\0';
  for( i = 1; i < max_col ; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  sprintf(tmp2,"%12s",head.ch[i].units) ;
	  strcat(tmp1, tmp2);
	}
    }
  strcat(tmp1, "\n");
  print_fileinfo(tmp1);
  
  tmp1[0] = '\0';
  for( i = 1; i < max_col ; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  sprintf(tmp2,"%7d recs",head.ch[i].nelem) ;
	  strcat(tmp1, tmp2);
	}
    }
  strcat(tmp1, "\n");  
  print_fileinfo(tmp1);
}






/******************************* error messages **************************/

void nea()
{
  global_error=TRUE;
  sprintf(msg, "Not enough arguments. Command aborted.\n");
  print_msg(msg);
}

void ne()
{
  global_error=TRUE;
  sprintf(msg, "Name Error! Aborted\n");
  print_msg(msg);
}

void coe()
{
  global_error=TRUE;
  sprintf(msg, "Error! Column not allocated. Use the \"all\" command. Note: you can use up to 17 columns.\n");
  print_msg(msg);
}

void cre()
{
  global_error=TRUE;
  sprintf(msg, "Error! Undefined row interval.\n");
  print_msg(msg);
}

void outdated_cmd()
{

  global_error=TRUE;
  sprintf(msg, "This command is no longer active\n");
  print_msg(msg);
}
