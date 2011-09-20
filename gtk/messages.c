#include <math.h>

#include "config.h"
#include "global.h"
#include "messages.h"
#include "notices.h"
#include "ui.h"

void top()
{
	/* default prompt */

	/* clear the file info window */
	ui_textfield_clear(TEXT_FIELD_FILE_INFO_WINDOW);

	/* print the new file info */
	print_info(); 
	ui_textfield_normalize(TEXT_FIELD_FILE_INFO_WINDOW);

	sprintf(msg, "\n");
	print_msg(msg);
	set_left_footer("Type a command");
	set_cmd_prompt("Command: ");
	global_error = FALSE; /*reset error flag, which may have been true for last command*/
}


void name_col(
     int colnum)
{
	/* prompt for the name and unit (for naming) */
	sprintf(msg, "Type the name and unit for the new column (# %d)", colnum);
	set_left_footer(msg);
	set_cmd_prompt("Name, Unit: ");
}


void print_fileinfo(
	char *txt)
{
	/* prints the current data states in info window */
	ui_textfield_insert(TEXT_FIELD_FILE_INFO_WINDOW, txt, strlen(txt));
}


/* prints a message in the message subwindow */
void print_msg(
     char *txt)
{
	ui_textfield_insert(TEXT_FIELD_MESSAGE_WINDOW, txt, strlen(txt));

// don't think we care about this- I think we can have unlimited scrollbacks now?
#if FALSE    
  /*Menu m;*/
  /*int notice_msg;*/
  char filename[20];

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
#endif
}

      
void set_cmd_prompt(char *txt)
{
	ui_label_set(LABEL_COMMAND_PROMPT, txt);
}

void set_left_footer(char *txt)
{
	ui_label_set(LABEL_LEFT_FOOTER, txt);
}

void display_active_window(int aw)
{
	char string[128];

	if (aw > 0) sprintf(string, "Active Window: %d", aw);
	else sprintf(string, "Active Window: NONE");

	ui_label_set(LABEL_ACTIVE_WINDOW_COUNT, string);
}

void display_active_plot(int ap)
{
	char string[128];

	if (ap > 0) sprintf(string, "Active Plot: %d", ap);
	else sprintf(string, "Active Plot: NONE");

	ui_label_set(LABEL_ACTIVE_PLOT, string);
}

void display_active_file(int af)
{
	char string[128];

	if (af > 0) sprintf(string, "File: %s", head.title);
	else sprintf(string, "File: NONE");

	ui_label_set(LABEL_ACTIVE_FILENAME, string);
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
