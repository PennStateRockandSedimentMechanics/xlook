#include "global.h"
#include <string.h>
#include <sys/file.h>
#include <xview/notice.h>
#include <xview/frame.h>
#include <xview/canvas.h>
#include <X11/Xlib.h>
#include <xview/xv_xrect.h>
#include <xview/panel.h>

char tmp_cmd[256];
char pathname[10][80];
char default_path[80];
char metapath[80];
char data_file[32];
char new_file[32];
char headline[32];

char qiparams[256];

FILE *data, *fopen(), *new;
int reed();

char plot_cmd[16], trig_cmd[16], qi_cmd[16], type_cmd[16];

extern int action;
extern char msg[MSG_LENGTH];
extern int total_windows;

int nargs;
char *args_left;

extern char mu_fit_mess_1[512];


command_handler(input) 
     char input[256]; 
{ 
  char cmd[256]; 
  char arg1[256], arg2[256], arg3[256], arg4[256]; 
  static char t_string[256];
  int nar, int1, i, intar[MAX_PLOTS], intar2[MAX_PLOTS];
  float farg1, farg2, farg3, farg4, farg5, farg6, farg7, farg8, farg9, farg10;
  char *strtok();
   

  if (action != MAIN)
    {
      process_action(input);
      return;
    }
  sscanf(input, "%s", cmd); 	/*take first part of input as cmd*/

  nargs = token_count(input,256);

  if (strcmp(cmd, "kp") == 0 || strcmp(cmd, "kill_plot") == 0)
    {
      if(total_windows == -1) 
      {
	sprintf(msg, "Error! There are no active plots \n");
	print_msg(msg);
	top();
	action = MAIN;
	return;
      }

      for(i=0; i < MAX_PLOTS; i++)
	intar[i]=intar2[i] = 0;

      if ((nar = sscanf(input, "%s %d %d %d %d %d %d %d %d %d %d", arg1, &intar[0],&intar[1],&intar[2], &intar[3], &intar[4], &intar[5], &intar[6], &intar[7], &intar[8], &intar[9])) >= 2)
	{
          for (i=0;i < nar-1; i++)
	  {
	    if(intar[i] > 0 && intar[i] < MAX_PLOTS)
          	intar2[intar[i]-1]=1;
	    else
	    {
		sprintf(msg, "Error, plot does not exist\n",i);
		print_msg(msg);
		top();
		action = MAIN;
		return;
	    }
	  }
          clr_mult_plots_proc(intar2);
	  action = MAIN;
	}
      else
	{
	  set_left_footer("Type the plot number to delete");
	  set_cmd_prompt("Plot Number: ");
	  action = KILL_PLOT;
	}
    }
  
  else if (strcmp(cmd, "kill_window") == 0 ||
	   strcmp(cmd, "kw") == 0)
    {
      if (sscanf(input, "%s %d", arg1, &int1) == 2)
	{
	  kill_win_proc(int1);
	  action = MAIN;
	}
      else
	{
	  set_left_footer("Type the window number to kill");
	  set_cmd_prompt("Window Number: ");
	  action = KILL_WIN;
	}
    }
  
  else if (strcmp(cmd, "new_window") == 0 ||
	   strcmp(cmd, "nw") == 0)
    {
      if (sscanf(input, "%s", arg1) == 1)
	{
	  new_win_proc();
	  action = MAIN;
	}
    }   
  
  else if (strcmp(cmd, "aw") == 0 ||
	   strcmp(cmd, "activate_window") == 0)
    {
      if (sscanf(input, "%s %d", arg1, &int1) == 2)
	{ 
	  set_active_window(int1-1);
	  action = MAIN;
	}
      else
	{
	  set_left_footer("Which window to be set active?");
	  set_cmd_prompt("Window Number: ");
	  action = SET_ACTIVE_WINDOW;
	}
    }

  else if (strcmp(cmd, "ap") == 0 ||
	   strcmp(cmd, "activate_plot") == 0)
    {
      if (sscanf(input, "%s %d", arg1, &int1) == 2)
	{ 
	  set_active_plot(int1-1);
	  action = MAIN;
	}
      else
	{
	  set_left_footer("Which plot to be set active?");
	  set_cmd_prompt("Plot Number: ");
	  action = SET_ACTIVE_PLOT;
	}
    }


  else if (strcmp(cmd, "q") == 0 ||
	   strncmp(cmd, "quit", 4) == 0)
    {
      /* may need to free all the memory allocations */
      quit_xlook();
    }


  else if (strncmp(cmd, "set_path", 8) == 0) 
    {
      if (sscanf(input, "%s %s", arg1, arg2) == 2)   
	{
	  set_path_proc(arg2); 	 	
	  action = MAIN;
	} 
      else 	 	
	{ 	 
	  set_cmd_prompt("Pathname: ");
	  set_left_footer("Type the pathname");
	  action = SET_PATH; 	
	}
    }

  
  else if (strncmp(cmd, "doit", 4) == 0)
    {
      if (sscanf(input, "%s %s", arg1, arg2) == 2) 	 	
	{
	  doit_proc(arg2); 	 	
	  action = MAIN;
	} 
      else 	 	
	{ 	 
	  set_cmd_prompt("Filename: ");
	  set_left_footer("Type the doit filename");
	  action=DOIT; 	
	}
    }  
  

  else if (strncmp(cmd, "all", 3) == 0)
    {
      if (nargs == 4)
	{
	  all_final_proc(input);
	  action = MAIN;
	  set_left_footer("Type a command");
	  set_cmd_prompt("Command: ");
	}
      
      else
	all_show_warning_proc(input);
    }
  

  else if (strncmp(cmd, "read", 4) == 0 || strncmp(cmd, "append", 6) == 0)
    {
      if (nargs == 2)
	{
	  read_proc(input);
	  action = MAIN;
	}
      else
	{
	  if (strncmp(cmd, "read", 4) == 0)
	    {
	      set_left_footer("Type the data file to read");	  
	      action = READ;
	    }
	  else 
	    {
	      set_left_footer("Type the file to append");
	      action = APPEND;
	    }
	  set_cmd_prompt("Filename: ");
	  return;
	}
    }


  else if (strncmp(cmd, "write", 5) == 0)
  {
      if (sscanf(input, "%s %s", arg1, arg2) == 2)
	{
	  write_proc(arg2);
	  action = MAIN;
	}
      else
	{
	  set_left_footer("Type the filename to write");
	  set_cmd_prompt("Filename: ");
	  action = WRITE;
	  
	}
  }

  /*  Tue Apr 15 21:25:02 EDT 1997 
      by park
      adding 'qi_tool' command */

  else if( (strncmp(cmd, "qi_tool", 7) == 0) || (strncmp(cmd, "fric_tool", 9) == 0)) 
  {
    strcpy(qiparams, input);
    if(nargs != 21)
    {
      sprintf(msg, "Error. 20 arguments are needed, only %d were entered. Relaunch tool or enter by hand\n", nargs-1);
      print_msg(msg); 
    }
    qi_win_proc();
  }
  
  else if(strncmp(cmd, "plot", 4) == 0 && (cmd[strlen((char *)cmd)-1] == 'h'))
  {
        sprintf(msg, "Plot help:  \n plotall: plots all rows for two columns, autoscales, enter col. numbers.\n plotover: fix rows and plot scales. Plots another set of cols at the same scale as the active plot.\n");
        print_msg(msg);
	sprintf(msg, " plotsr: plot the same row numbers but different cols, autoscales.\n plotauto: plot a data segment, autoscale: enter cols & rows.\n");
        print_msg(msg);
 	sprintf(msg, " plotlog: log plot, enter cols & rows.\n plotsame: plot at the same scale as the active plot but allow different rows and cols.\n plotscale: user enters everything. rows, cols, x, y scales.\n pa: same as plotauto, except different algorithm for determining tick placement.\n");
        print_msg(msg);
        top();
        action = MAIN;
        return;
  }
  else if (strncmp(cmd, "plotover", 8) == 0 || 
	   strncmp(cmd, "plotall", 7) == 0 || 
	   strncmp(cmd, "plotsr", 6) == 0)
    {
      strcpy(plot_cmd, cmd);
      
      if (nargs == 3)
	{
	  do_plot(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the x-axis and y-axis");
	  set_cmd_prompt("X-axis, Y-axis: "); 
	  action = PLOT_GET_XY;
	}
    }
  

  else if (strncmp(cmd, "plotauto", 8) == 0 || 
	   strncmp(cmd, "pa", 2) == 0 || 
	   strncmp(cmd, "plotlog", 7) == 0 || 
	   strncmp(cmd, "plotsame", 8) == 0)
    {
      strcpy(plot_cmd, cmd);

      if (nargs == 5)
	{
	  do_plot(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);
	  
	  set_left_footer("Type the x-axis and y-axis");
	  set_cmd_prompt("X-axis, Y-axis: ");
	  action = PLOT_GET_BE;
	}
    }


  else if (strncmp(cmd, "plotscale", 9) == 0)
    {
      strcpy(plot_cmd, cmd);

      if (nargs == 9)
	{
	  do_plot(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);
	  
	  set_left_footer("Type the x-axis and y-axis");
	  set_cmd_prompt("X-axis, Y-axis: ");
	  action = PLOT_GET_BE;
	}
    }  
  

  else if (strncmp(cmd, "examin", 6) == 0) 
    {
      if (sscanf(input, "%s %s", arg1, arg2) == 2) 	 	
	{
	  do_examin(arg2); 	 	
	  action = MAIN;
	} 
      else 	 	
	{ 	 
	  set_cmd_prompt("Filename: ");
	  set_left_footer("Type the input filename");	  action = EXAMIN_GET_FILENAME; 	
	}
    }


  else if (strncmp(cmd, "getaschead", 10) == 0) 
    {
       if (sscanf(input, "%s %s", arg1, arg2) == 2) 	 	
	{
	  do_getaschead(arg2); 	 	
	  action = MAIN;
	} 
      else 	 	
	{ 	 
	  set_cmd_prompt("Filename: ");
	  set_left_footer("Type the input filename");
	  action = GETASCHEAD_GET_FILENAME; 	
	}
    }


  else if (strncmp(cmd, "head", 4) == 0)
    {
       if (sscanf(input, "%s %s", arg1, arg2) == 2) 	 	
	{
	  do_head(arg2); 	 	
	  action = MAIN;
	} 
      else 
	{ 	 
	  set_cmd_prompt("Filename: ");
	  set_left_footer("Type the output filename or S for screen output.");
	  action = HEAD_GET_FILENAME; 	
	}
     }
  

  else if (strncmp(cmd, "tasc", 4) == 0)
    {
       if (nargs == 3)
	{
	  do_tasc(input); 	 	
	  action = MAIN;
	} 
      else 	 	
	{ 	
	  if (nargs > 1)
	    stripper(input, 1);
	
	  set_cmd_prompt("Filename: ");
	  set_left_footer("Type the input filename");
	  action = TASC_GET_FILENAME; 	
	}
     }
  

  else if (strncmp(cmd, "stdasc", 6) == 0)
    {
      if (nargs == 4)
	{
	  do_stdasc(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	      stripper(input, 1);
	
	  set_cmd_prompt("Filename: ");
	  set_left_footer("Type the input filename");
	  action = STDASC_GET_FILENAME;
	}
    }

  
  else if (strncmp(cmd, "simplex", 7) == 0) 
    {
      simplex_info();
      if (nargs == 6)
	{
	  do_simplex(input); 	 	
	} 
      else 	 	
	{ 	 
	  if (nargs > 1)
	    stripper(input, 1);

	  set_cmd_prompt("Function: ");
	  set_left_footer("Type the function to use");
	  action = SIMPLEX_GET_FN; 	
	}
    }
  

  else if (strncmp(cmd, "offset_int", 10) == 0) 
    {
      if (nargs == 5)
	{
	  do_offset_int(input); 	 	
	} 
      else 	 	
	{ 
	  sprintf(msg, "Offset COL (from REC2 to the end) by the difference between REC2 and REC1");
	  print_msg(msg);
	  if (nargs > 1)
	    stripper(input, 1);

	  set_cmd_prompt("Col, Rec1, Rec2: ");
	  set_left_footer("Type the column (COL), record1 (REC1) and record2 (REC2)");
	  action = OFFSET_INT; 	
	}
    }


  else if (strncmp(cmd, "offset", 6) == 0) 
    {
      if (nargs == 5)
	{
	  do_offset(input); 	 	
	  action = MAIN;
	} 
      else 	 	
	{ 
	  sprintf(msg, "Offset COL1 by the difference between COL2, REC2 and COL1, REC1\n");
	  print_msg(msg);
	  if (nargs > 1)
	    stripper(input, 1);

	  set_cmd_prompt("Col1, Rec1, Col2, Rec2: ");
	  set_left_footer("Type COL1, REC1, COL2 and REC2");
	  action = OFFSET; 	
	}
    }
 

  else if (strncmp(cmd, "zero", 4) == 0)
    { 
      if (nargs == 3)
	{
	  do_zero(input);
	  action = MAIN;
	} 
      else 	 	
	{ 	 
	  if (nargs > 1)
	    stripper(input, 1);

	  set_cmd_prompt("Col, Rec: ");
	  set_left_footer("Type the column and record number");
	  action = ZERO; 	
	}
    }


 else if (strncmp(cmd, "r_row_col", 9) == 0 ||
	  strncmp(cmd, "rrc", 3) == 0)
    {
       if (nargs == 4)
	{
	  do_r_row_col(input);
	  action = MAIN;
	} 
      else 	 	
	{ 
	  if (nargs > 1)
	    stripper(input, 1);

	  set_cmd_prompt("Col: ");
	  set_left_footer("Which column to remove?");
	  action = R_ROW_COL; 	
	}
    }


  else if (strncmp(cmd, "r_row", 5) == 0 || strncmp(cmd, "rr", 2) == 0) 
    {
       if (nargs == 3)
	{
	  do_r_row(input);
	  action = MAIN;
	} 
      else 	 	
	{ 	
	  if (nargs > 1)
	    stripper(input, 1);

	  set_cmd_prompt("First, Last: ");
	  set_left_footer("Type the first and last row to remove");
	  action = R_ROW_F_L; 	
	}
    }


  else if (strncmp(cmd, "r_col", 5) == 0)
    { 
      if (sscanf(input, "%s %d", arg1, &int1) == 2)
	{
	  do_r_col(int1);
      	  action = MAIN;
	} 
      else 	 	
	{ 	
	  if (nargs > 1)
	    stripper(input, 1);

	  set_cmd_prompt("Col: ");
	  set_left_footer("Remove which column?");
	  action = R_COL; 	
	}
    }


  else if (strncmp(cmd, "name", 4) == 0)
    { 
      if (nargs == 4)
	  do_name(input);
      else 	 	
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the column number to name, the name and the unit");
	  set_cmd_prompt("Col, Name, Unit: ");
	  action = NAME; 	
	}
    }


  else if (strncmp(cmd, "comment", 7) == 0) 
    {
       if (nargs == 3)
	{
	  do_comment(input); 	 	
	  action = MAIN;
	} 
      else 	 	
	{ 
	  if (nargs > 1)
	    stripper(input, 1);

	  set_cmd_prompt("Col: ");
	  set_left_footer("Comment which column?");
	  action = COMMENT_COL; 	
	}
    }


  else if (strncmp(cmd, "pdfauto", 7) == 0)
    {
      if (nargs == 5)
	{ 
	  do_pdfauto(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);
	
	  set_left_footer("Type the Col, NewCol Prob, NewCol Bin, #Bins");
	  set_cmd_prompt("Col, NCP, NCB, #Bins: ");
	  action = PDFAUTO;
	}
    }

  else if (strncmp(cmd, "pdf", 3) == 0)
    {
      if (nargs == 7)
	{ 
	  do_pdf(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the Col, NewCol Prob, NewCol Bin, #Bins, Max, Min");
	  set_cmd_prompt("Col, NCP, NCB, #Bins, Max, Min: ");
	  action = PDF;
	}
    }  
      
  else if (strncmp(cmd, "decimat", 7) == 0)
    {
      if (cmd[strlen((char *)cmd)-1] == 'h')
      {  
        sprintf(msg, "Decimate help.\nUsage: col, new_col, interval, first, last, name, units.\nRemove points from a segment of a column.\nInterval controls how often to save points. Higher intervals remove more points.\nUse decimate_r to remove rows after the last decimated row.\n\n");
        print_msg(msg);
        action=MAIN;
      }  
      else
      {   
         if (nargs == 8)
        {
          do_decimat(input);
        }
        else
        {
          if (nargs > 1)
            stripper(input, 1);

          set_left_footer("Type the Col, New Col and Increment");
          set_cmd_prompt("Col, New_Col, Inc: ");
 
          if (strncmp(cmd, "decimat_r", 9) == 0)
            action = DECIMAT_R;
          else
            action = DECIMAT;
        }
      }
    }
 
  else if (strncmp(cmd, "peak", 4) == 0)
    {
      if (nargs == 5)
	{ 
	  do_peak(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the Col and New Col");
	  set_cmd_prompt("Col, New_Col: ");
	  action = PEAK;
	}
    }
  
  else if (strncmp(cmd, "curv", 4) == 0)
    {
      if (nargs == 6)
	{ 
	  do_curv(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the column number, new column number and dx");
	  set_cmd_prompt("Col, New_Col, dx: ");
	  action = CURV;
	}
    }

  else if (strncmp(cmd, "summation", 9) == 0)
    {
      if (nargs == 5)
	{ 
	  do_summation(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the Col and New Col");
	  set_cmd_prompt("Col, New_Col: ");
	  action = SUMMATION;
	}
    }

  else if (strncmp(cmd, "findint", 7) == 0)
    {
      if (nargs == 4)
	{ 
	  do_findint(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the Column, First and Last record number");
	  set_cmd_prompt("Col, First, Last: ");
	  action = FINDINT;
	}
    }

  else if (strncmp(cmd, "trend_a", 7) == 0)
    {
      if (nargs == 3)
	{ 
	  do_trend_a(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

          sprintf(msg, "Calculate a best fit line.\n");
      	  print_msg(msg);
	  set_left_footer("Type X-column and Y-column");
	  set_cmd_prompt("X-col, Y-col: ");
	  action = TREND_A;
	}
    }

  else if (strncmp(cmd, "trend", 5) == 0)
    {
      if (nargs == 5)
	{ 
	  do_trend(input);
	  action = MAIN;
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

          sprintf(msg, "Calculate a best fit line.\n");
      	  print_msg(msg);
	  set_left_footer("Type X-column and Y-column");
	  set_cmd_prompt("X-col, Y-col: ");
	  action = TREND_XY;
	}
    }


  else if (strncmp(cmd, "r_trend", 7) == 0)
    {

      if (nargs == 10)
	{ 
	  strcpy(arg3,"\t ,");
	  strcpy(t_string,input);
	  strtok(t_string,arg3);	/*call twice so that we're pointing at 3rd tok*/
          strtok(NULL,arg3);
	  strcpy(arg4, strtok(NULL,arg3));
	  if(strncmp(arg4, "i", 1) == 0 || strncmp(arg4, "I", 1) == 0)
	  	do_r_trend_input(input);
	  else
	  	do_r_trend_comp(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

          sprintf(msg, "Remove a trend from data.\n");
      	  print_msg(msg);
	  set_left_footer("Detrend X or Y values?");
	  set_cmd_prompt("X/Y: ");
	  action = R_TREND;
	}
    }

  else if (strncmp(cmd, "z_max", 5) == 0)
    {
      if (nargs == 5)
	{ 
	  do_z_max(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type Col and New Col");
	  set_cmd_prompt("Col, New_Col: ");
	  action = Z_MAX;
	}
    }

  else if (strncmp(cmd, "z_min", 5) == 0)
    {
      if (nargs == 5)
	{ 
	  do_z_min(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type COL and NEW_COL");
	  set_cmd_prompt("Col, New_Col: ");
	  action = Z_MIN;
	}
    }


  else if (strncmp(cmd, "r_mean", 6) == 0)
    {
      if (nargs == 5)
	{ 
	  do_r_mean(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type COL and NEW_COL");
	  set_cmd_prompt("Col, New_Col: ");
	  action = R_MEAN;
	}
    }

  else if (strncmp(cmd, "compress", 8) == 0)
    {
      if (nargs == 5)
	{ 
	  do_compress(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type COL and NEW_COL");
	  set_cmd_prompt("Col, New_Col: ");
	  action = COMPRESS;
	}
    }

  else if (strncmp(cmd, "positive", 8) == 0)
    {
      if (nargs == 5)
	{ 
	  do_positive(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type COL and NEW_COL");
	  set_cmd_prompt("Col, New_Col: ");
	  action = POSITIVE;
	}
    }

  else if (strncmp(cmd, "r_spike", 7) == 0)
    {
      if (nargs == 3)
	{ 
	  do_r_spike(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the Col and Row number to remove the spike");
	  set_cmd_prompt("Col, Row#: ");
	  action = R_SPIKE;
	}
    }

  else if (strncmp(cmd, "mathint", 7) == 0 ||
	   strncmp(cmd, "math_int", 8) == 0)
    {
      if (nargs == 10)
	{ 
	  do_mathint(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the math command: X-col $ Y-col @ New_col");
	  set_cmd_prompt("X-col, $, Y-col, @, New_Col: ");
	  action = MATHINT;
	}
    }
  
  else if (strncmp(cmd, "math", 4) == 0)
    {
      if (nargs == 8)
	{ 
	  do_math(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the math command: X-col $ Y-col @ New_col");
	  set_cmd_prompt("X-col, $, Y-col, @, New_Col: ");
	  action = MATH;
	}
    }
  
  else if (strncmp(cmd, "interpolate", 11) == 0)
    {
      if (nargs == 12)
	{ 
	  do_interpolate(input);
	}
      else
	{
	  if (cmd[strlen((char *)cmd)-1] == 'h')
	    {
	      sprintf(msg, "Mid point interpolation for y.\nRows from the end of interpolated data can be removed using interpolate_r.\n");
	      print_msg(msg);
	      }

	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the x-col, y-col, new x-col and new y-col");
	  set_cmd_prompt("X-col, Y-col, New_X-col, New_Y-col: ");

	  if (cmd[strlen((char *)cmd)-1] == 'r')
	    action = INTERPOLATE_R;
	  else	  
	    action = INTERPOLATE;
	}
    }

  else if (strncmp(cmd, "sort", 4) == 0 || strncmp(cmd, "order", 5) == 0)
    {
      if (nargs == 4)
	{ 
	  do_sort(input);
	}
      else
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type Col, Start_row and End_row");
	  set_cmd_prompt("Col, Start, End: ");
	  action = SORT;
	}
    }

  else if (strncmp(cmd, "polyfit", 7) == 0)
    {
      if (nargs == 11 || nargs == 10)	/*it will be 11 if fit is extended --the extra arg being a row number in position 9, do_polyfit --in cmd.c will deal with this*/
	{ 
	  do_polyfit(input);
	}
      
      else
	{
	  sprintf(msg, "Polynomial fit of order n.\n");
	  print_msg(msg);

	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X-col, Y-col, New_col, Order");
	  set_cmd_prompt("X-col, Y-col, New_col, Order: ");

	  if (strncmp(cmd, "polyfit_i", 9) == 0)
	    action = POLYFIT_I;
	  else
	    action = POLYFIT;
	}
    }

  else if (strncmp(cmd, "cs", 2) == 0 || strncmp(cmd, "strain", 6) == 0)
    {
      if (nargs == 8)
	{ 
	  do_cs(input);
	}
      
      else
	{
	  sprintf(msg, "Incremental shear strain calculation.\n");
	  print_msg(msg);
	  
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the axial_disp col, layer_thick col and output col");
	  set_cmd_prompt("Axial_disp, Layer_thick, Output Col: ");
	  action = CS;
	}
    }

  else if (strncmp(cmd, "ec", 2) == 0)
    {
      if (nargs == 9)
	{ 
	  do_ec(input);
	}
      else
	{
	  sprintf(msg, "Correct displacement for elastic distortion within piston and sample column.\nThe algorithm builds a new displacement col. by summing intial disp. and incremental actual displacements  (measured - elastic).\n");
	  print_msg(msg);

	  if (nargs > 1)
	    stripper(input, 1);
	
	  set_left_footer("Type the disp. col, force (or stress) col and new col");
	  set_cmd_prompt("disp_col, force_col, new_col: ");
	  action = EC;
	}
    }

  else if (strncmp(cmd, "cgt", 3) == 0 || strncmp(cmd, "calc_geometric_thinning", 23) == 0)
    {
      if (nargs == 7)
	{ 
	  do_cgt(input);
	}
      else
	{
	  if (cmd[strlen((char *)cmd)-1] == 'h')
	    {
	      sprintf(msg,"Calculate geometric thinning\n  calculation is: (1)  del_h = h dx/2L ; where h is input thickness, dx is slip increment and L is length of the sliding block parallel to slip \n");
	      print_msg(msg);	    
	      sprintf(msg, "(1) Assumes an h appropriate for *one* layer (not both!) and thus the calculation is for one layer\n");
	      print_msg(msg);	    
	      sprintf(msg, "Also, make sure your equation is dimensionally correct!     -see also rgt\n");
	      print_msg(msg);
	    }
	  
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the Length, initial_h, Disp_col and New_col");
	  set_cmd_prompt("L, h, Disp_col, New_col: ");
	  action = CGT;
	}
    }
  
  else if ((strncmp(cmd, "rgt", 3) == 0) || (strncmp(cmd, "geometric_thinning", 18) == 0))
    {
      if (nargs == 7)
	{ 
	  do_rgt(input);
	}
      else
        { 
          if(cmd[strlen((char *)cmd)-1] == 'h')
	    {
	      sprintf(msg, " Correct horizontal displacement measurement during direct shear test for geometric thinning\n  correction is: (1)  del_h = h dx/2L ; where h is thickness, dx is slip increment and L is length of the sliding block parallel to slip \n"); 
	      print_msg(msg);
	      sprintf(msg, "(1) Assumes an h appropriate for *one* layer (not both!) and thus the H. disp. measurements being corrected should be for one layer\nAlso, make sure your equation is dimensionally correct!   --see also cgt\n");
	      print_msg(msg);
	    }
	  
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the Length, Disp_col, Gouge_thickness col and New_col");
	  set_cmd_prompt("L, Disp_col, Thick_col, New_col: ");
	  action = RGT;
	}
    }

  else if ((strncmp(cmd, "vc", 2) == 0) || (strncmp(cmd, "vol_cor", 7) == 0))
   {
     if (nargs == 9)
	{ 
	  do_vc(input);
	}
      else
        {
	  if(cmd[strlen((char *)cmd)-1] == 'h')
	    {
	      sprintf(msg, "Corrrect porosity/volume strain (during loading/unloading) for the effect of confining pressure on the material at the edges of the layer.\n");
	      print_msg(msg);
	      sprintf(msg, "User provides a compressibility (dv/V/dP) * the ratio of the affected volume (eg. at the edges) to the total volume.\n");
	      print_msg(msg);
	      sprintf(msg, "A first guess for the volume of material affected by Pc during load/unload is an anular -elliptical area- of width = layer thickness * thickness\n\n"); 
	      print_msg(msg);
	    }
	  
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the Vol strain, Shear stress col and New col");
	  set_cmd_prompt("Vol_strain, Shear_stress_col, New_col: ");
	  action = VC;
      	}
   }
  
  else if ((strncmp(cmd, "deriv", 5) == 0))
    {
      if (nargs == 8)
	{ 
	  do_deriv(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, Y col and New col for dy/dx derivative");
	  set_cmd_prompt("X-col, Y-col, New-col: ");
	  action = DERIV;
      	}
    }

  else if ((strncmp(cmd, "exp", 3) == 0))
    {
      if (nargs == 5)
	{ 
	  do_exp(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col and New col");
	  set_cmd_prompt("X-col, New-col: ");
	  action = EXP;
      	}
    }

  else if ((strncmp(cmd, "ln", 2) == 0))
    {
      if (nargs == 5)
	{ 
	  do_ln(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col and New col");
	  set_cmd_prompt("X-col, New-col: ");
	  action = LN;
      	}
    }

  else if ((strncmp(cmd, "Power1", 6) == 0))
    {
      if (nargs == 7)
	{ 
	  do_Power1(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	  set_left_footer("Type the X col, New col, A and B");
	  set_cmd_prompt("X-col, New-col, A, B: ");
	  action = POWER1;
      	}
    }

  else if ((strncmp(cmd, "Power2", 6) == 0))
    {
      if (nargs == 8)
	{ 
	  do_Power2(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, New col, A, B, C");
	  set_cmd_prompt("X-col, New-col, A, B, C: ");
	  action = POWER2;
      	}
    }

  else if ((strncmp(cmd, "normal", 6) == 0))
    {
      if (nargs == 7)
	{ 
	  do_normal(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, New col, mean an stddev");
	  set_cmd_prompt("X-col, New-col, Mean, Stddev: ");
	  action = NORMAL;
      	}
    }

  else if ((strncmp(cmd, "chisqr", 6) == 0))
    {
      if (nargs == 6)
	{ 
	  do_chisqr(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, New col and ndf");
	  set_cmd_prompt("X-col, New-col, ndf: ");
	  action = CHISQR;
      	}
    }

  else if ((strncmp(cmd, "scchisqr", 8) == 0))
    {
      if (nargs == 8)
	{ 
	  do_chisqr(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, New col, sigma, ndf and offset");
	  set_cmd_prompt("X-col, New-col, sigma, ndf, offset: ");
	  action = SCCHISQR;
      	}
    }

  else if ((strncmp(cmd, "rclow", 5) == 0))
    {
      if (nargs == 7)
	{ 
	  do_rclow(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, New col, sigma, A and B");
	  set_cmd_prompt("X-col, New-col, A, B: ");
	  action = RCLOW;
      	}
    }

  else if ((strncmp(cmd, "genexp", 6) == 0))
    {
      if (nargs == 9)
	{ 
	  do_genexp(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, New col, A, B, C and D");
	  set_cmd_prompt("X-col, New-col, A, B, C, D: ");
	  action = GENEXP;
      	}
    }

  else if ((strncmp(cmd, "gensin", 6) == 0))
    {
      if (nargs == 9)
	{ 
	  do_gensin(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, New col, A, B, C and D");
	  set_cmd_prompt("X-col, New-col, A, B, C, D: ");
	  action = GENSIN;
      	}
    }

  else if ((strncmp(cmd, "Poly4", 5) == 0))
    {
      if (nargs == 10)
	{ 
	  do_Poly4(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	  set_left_footer("Type the X col, New col, A, B, C, D and E");
	  set_cmd_prompt("X-col, New-col, A, B, C, D, E: ");
	  action = POLY4;
      	}
    }

  else if ((strncmp(cmd, "ExpLin", 6) == 0))
    {
      if (nargs == 8)
	{ 
	  do_ExpLin(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col, New col, A, B and C");
	  set_cmd_prompt("X-col, New-col, A, B, C: ");
	  action = EXPLIN;
      	}
    }

  else if ((strncmp(cmd, "log", 3) == 0))
    {
      if (nargs == 5)
	{ 
	  do_log(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col and New col");
	  set_cmd_prompt("X-col, New-col: ");
	  action = LOG;
      	}
    }

  else if ((strncmp(cmd, "recip", 5) == 0))
    {
      if (nargs == 5)
	{ 
	  do_recip(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);

	  set_left_footer("Type the X col and New col");
	  set_cmd_prompt("X-col, New-col: ");
	  action = RECIP;
      	}
    }

  else if ((strncmp(cmd, "power", 5) == 0))
    {
      if (nargs == 6)
	{ 
	  do_power(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	    
	  set_left_footer("Type the Power, X col and New col");
	  set_cmd_prompt("Power, X-col, New-col: ");
	  action = POWER;
      	}
    }

  else if ((strncmp(cmd, "col_power", 9) == 0))
    {
      if (nargs == 6)
	{ 
	  do_col_power(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	  
	  set_left_footer("Type the Power col, X col and New col");
	  set_cmd_prompt("Power-col, X-col, New-col: ");
	  action = COL_POWER;
      	}
    }

  else if ((strncmp(cmd, "rcph", 4) == 0))
    {
      if (nargs == 7)
	{ 
	  do_rcph(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	    
	  set_left_footer("Type the X col, New col, A and B");
	  set_cmd_prompt("X-col, New-col, A, B: ");
	  action = RCPH;
      	}
    }


  else if (strncmp(cmd,"trig", 4) == 0 || 
	   strncmp(cmd,"sin", 3) == 0 || 
	   strncmp(cmd,"cos", 3) ==  0 || 
	   strncmp(cmd,"tan", 3) == 0 || 
	   strncmp(cmd,"asin", 4) == 0 ||
	   strncmp(cmd,"acos", 4) == 0 || 
	   strncmp(cmd,"atan", 4) == 0 )
    {
      strcpy(trig_cmd, cmd);
      
      if (nargs == 6)
	{ 
	  do_col_power(input);
	}
      else  
	{
	  if (nargs > 1)
	      stripper(input, 1);

	  set_left_footer("Type the Column, New col and Function");
	  set_cmd_prompt("Col, New-col, Func: ");
	  action = TRIG;
	}
    }

  else if ((strncmp(cmd, "slope", 5) == 0) || (strncmp(cmd, "ras", 3) == 0) )  
    {
      /* used to be called o_slope */
      if (nargs == 9)
	{ 
	  do_slope(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	  
	  sprintf(msg, "Calculate running average slope, dY/dX, and put in \"New Col\"\n");
	  print_msg(msg);
	  sprintf(msg, "Window size is forced to be odd.\nEnds are padded with first reasonable value (half_window from start/end)\n");
	  print_msg(msg);
	  set_left_footer("Type the X col, Y Col, New Col, Start Row, End Row and Window size");
	  set_cmd_prompt("X-col, Y-Col, New-col, Start, End, Win-size: ");
	  action = SLOPE;
      	}
    }

  else if ((strncmp(cmd, "rsm", 3) == 0))
    {
      if (cmd[strlen((char *)cmd)-1] == 'h')
	print_rsm_help_info();
   
      sprintf(msg, "One or two state variable friction model calculated at displacements specified by disp col.\n");
      print_msg(msg);
      sprintf(msg, "rsm_h gives a detailed description of this function. See also cm_h \n");
      print_msg(msg);
      sprintf(msg, mu_fit_mess_1);
      print_msg(msg);
      
      if (nargs == 16)
	{ 
	  do_rsm(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	  
	  sprintf(msg, "Enter: disp. col., mu col, row # of vel. step and end of data to model, col for model_mu,  stiffness (k), Sigma_n, v_initial (v_o), v_final, mu_o, mu_f, a, b1, Dc1, and Dc2\n");
	  print_msg(msg);
	  set_left_footer("");
	  set_cmd_prompt("Cmds: ");
	  action = RSM;
	}
    }

  else if ((strncmp(cmd, "cm", 2) == 0))
    {
      if (cmd[strlen((char *)cmd)-1] == 'h')
	print_cm_help_info();
      
      if (nargs == 17)
	{ 
	  do_cm(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	    
	  sprintf(msg, "Forward modelling of rate/state variable friction. (cm_h gives a detailed description of this function) \n");
	  print_msg(msg);
	  sprintf(msg, "Enter: model_disp_col, (data) disp. col., (data) mu col,  vel_step_row, end_row, model_mu_col, \n\tstiffness (k), Sigma_n,v_initial (v_o), v_final, mu_o, mu_f, a, b1, Dc1, and Dc2\n");
	  print_msg(msg);
	  sprintf(msg, "\tmodel_disp_col, disp_col, mu_col, vs_row, end_row, model_mu_col, k, Sigma_n, v_o, vf, mu_o, mu_f, a, b1, Dc1, Dc2 \n ") ;
	  print_msg(msg);

	  set_left_footer("");
	  set_cmd_prompt("Inputs: ");
	  action = CM;
	}
    }


  else if ((strncmp(cmd, "mem", 3) == 0))
    {
      if (cmd[strlen((char *)cmd)-1] == 'h')
	{
	  sprintf(msg, "Command line interpretation: \n ycol = signal for which you want power spectra.\n xcol = spacing of signal (i.e., time, dist)\n");
	  print_msg(msg);
	  sprintf(msg,"  2 cols are output: freq. and power.\n the spectra is calculated between records 'first_row' and 'last row'\n  #freqs is the # of frequencies  (beginning at the nyquist) at which to calculate power.\n");
	  print_msg(msg);
	  sprintf(msg,"  and #poles is the # of poles to use.   The frequencies can either be distributed linearly (n) or \n logarithmically (l=> linear on a log plot), a welsh taper may be applied to the data (w)\n\n");
	  print_msg(msg);
	  top();
          action = MAIN;
          return;
	}
      
      if (nargs == 11)
	{ 
	  do_mem(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	    
	  sprintf(msg, "Power spectra estimate using maximum entropy method, n=linear freq. series, l=log, w=welsh taper, n=no taper (mem_h  for help)\n");
	  print_msg(msg);
	  sprintf(msg, "Inputs for mem: xcol, ycol, freq_col, power_col, first_row, last_row, #freqs, #poles, l/n, w/n\n") ;
	  print_msg(msg);
	  set_left_footer("");
	  set_cmd_prompt("Inputs: ");
	  action = MEM;
	}
    }


  else if ((strncmp(cmd, "median_smooth", 13) == 0) ||
	   (strncmp(cmd, "ms", 2) == 0))
    {
      if (cmd[strlen((char *)cmd)-1] == 'h')
	{ 
	  sprintf(msg, "Calculate the running average *median* value of COL within a moving window between start_row and end_row.\n\t*the length of `window' is forced to be odd*\n");
	  print_msg(msg);
	}
      
      if (nargs == 8)
	{ 
	  do_median_smooth(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	    
	  sprintf(msg, "Inputs for median smooth: Col, New_Col , Start_Row, End_Row, Window_Size\n") ;
	  print_msg(msg);
	  set_left_footer("");
	  set_cmd_prompt("Inputs: ");
	  action = MEDIAN_SMOOTH;
	}
    }


  else if ((strncmp(cmd, "smooth", 6) == 0))
    {
      if (nargs == 8)
	{ 
	  do_smooth(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	    
	  sprintf(msg, "Inputs for smooth: Col, New_Col , Start_Row, End_Row, Window_Size\n") ;
	  print_msg(msg);
	  set_left_footer("");
	  set_cmd_prompt("Inputs: ");
	  action = SMOOTH;
	}
    }
  
  else if ((strncmp(cmd, "typeall", 7) == 0) || 
	   (strncmp(cmd, "type_all", 8) == 0))
    {
      if (nargs == 2)
	{ 
	  do_typeall(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);
	    
	  set_left_footer("Input the filename or type 'S' for screen output");
	  set_cmd_prompt("Input: ");
	  action = TYPEALL;
	}
    }
  
  else if ((strncmp(cmd, "type", 4) == 0))
    {
      if (nargs == 6)
	{ 
	  do_type(input);
	}
      else  
	{
	  sscanf(cmd, "%s", type_cmd);
	  
	  if (nargs > 1)
	    stripper(input, 1);
	    
	  set_left_footer("Input the starting and ending record numbers");
	  set_cmd_prompt("Start End: ");
	  action = TYPE;
	}
    }

  else if ((strncmp(cmd, "stat", 4) == 0))
    {
      if (nargs == 4 || ((strncmp(cmd, "stat_a", 6) == 0) && nargs == 2) )
	{ 
	  do_stats(input);
	}
      else  
	{
	  if (nargs > 1)
	    stripper(input, 1);	 	/*this is in strcmd.c*/

	  if(strncmp(cmd, "stat_a", 6) == 0)
	  {
		set_left_footer("Type the col number");
	  	set_cmd_prompt("Col: ");
	  	action = STAT_A;
	  }
	  else
	  {
		set_left_footer("Type the starting and ending record numbers");
	  	set_cmd_prompt("Col Start End: ");
	  	action = STAT;
	  }
	}
    }

  else if (strncmp(cmd, "qi", 2) == 0)
  {
      if(cmd[(strlen((char *)cmd)-1)] == 'h')
      {
	  do_qi_help();
          top();
          action = MAIN;
          return;
      }
      else if (nargs == 21)
	  do_qi(input);
      else 
      { 
	  if (nargs > 21)
	  {  
	    stripper(input,21);
	    do_qi(input);
	  }
      	  else 
	  {
	    strcpy(qi_cmd, cmd);
	  
	    sprintf(msg, "Linearized inversion of one or two state variable friction model.\n\t(qi_h gives a detailed description of this function) \n");
	    print_msg(msg);
	    sprintf(msg, "\nNeed disp. col, mu col, model_mu col, beginning row for fit, row # of vel. step, end row, weight_row, lin_term(c), converg_tol, lambda,  wc, stiffness, v_initial, v_final, mu_init, a, b1, Dc1, b2 and Dc2 for input.\n ");
	    print_msg(msg);
	    sprintf(msg, "Input:\nd_col, mu_col, mod_col, first_row, vs_row, end_row, weight_row, c, tol, lambda, wc, k, v_o, vf, mu_o, a, b1, Dc1, b2, Dc2 \n");
	    print_msg(msg);
	    set_left_footer("");
	    set_cmd_prompt("QI Input: ");
	    action = QI;
	  }
      }
  }
  else if (strncmp(cmd, "scm", 3) == 0) 
    {
      if (nargs > 16)       
	{
	  stripper(input, 16);
	  do_scm(input); 	 	
	} 
      else 	 	
	{ 
	  if ((strlen(cmd) >= 5) && (strncmp(cmd, "scm_h", 5) == 0))
	    do_scm_help();
	  do_scm_info();
	  set_cmd_prompt("Input: ");
	  set_left_footer("Type all the arguments for scm");
	  action = SCM_GET_ARGS; 	
	}
    }


}



process_action(arg)
     char arg[256];
{
  int nargs;
  
  nargs = token_count(arg,256);
  
  switch(action)
    {

    case SIMP_FUNC_GET_MAX_ITER:
      if (nargs > 1) stripper(arg, 1);
      sprintf(tmp_cmd, "%s ", arg);
      sprintf(msg, "Type the initial guesses for the %d free parameters", n_param);
      set_left_footer(msg);
      set_cmd_prompt("Initial guesses: ");
      action = SIMP_FUNC_GET_INIT_GUESSES;
      break;
      
    case SIMP_FUNC_GET_INIT_GUESSES:
      if (nargs > n_param) stripper(arg, n_param);
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      sprintf(msg, "Type the initial step size for the %d free parameters", n_param);
      set_left_footer(msg);
      set_cmd_prompt("Initial step size: ");
      action = SIMP_FUNC_GET_INIT_STEP_SIZE;
      break;

    case SIMP_FUNC_GET_INIT_STEP_SIZE:
      if (nargs > n_param) stripper(arg, n_param);
      strcat(tmp_cmd, arg);
      do_get_initial_values(tmp_cmd);
      break;
      

    case SIMP_FUNC:
      do_simp_func(arg);
      break;
      
    case SCM_GET_ARGS:
      sprintf(tmp_cmd, "scm %s", arg);
      do_scm(tmp_cmd);
      break;
   
    case SCM_SIMP_WEIGHT_L2:
      do_simp_weight_l2(arg);
      break;
      
    case SCM_SIMP_WEIGHT:
      do_simp_weight(arg);
      break;
      
    case SCM_1:
      do_scm_1(arg);
      break;
      
    case QI_MVS:
      do_qi_mvs(arg);
      /*printf("\n\n QI_MVS: %s\n", arg);*/
      break;      

    case QI_WC:
      sscanf(arg, "%d", &rs_param.end_weight_row);
     /*printf("\n\n QI_WC: %s\n", arg);*/
      do_qi_final();
      break;
      
    case QI:
      sprintf(tmp_cmd, "%s %s", qi_cmd, arg);
      do_qi(tmp_cmd);
      break;
      
    case STAT:
      sprintf(tmp_cmd, "stat %s", arg);
      do_stats(tmp_cmd);
      break;
      
    case STAT_A:
      sprintf(tmp_cmd, "stat_a %s", arg);
      do_stats(tmp_cmd);
      break;
      
    case TYPE:
      /*printf("type cmd: %s\n", type_cmd);*/
      
      if (nargs == 5)
	{
	  sprintf(tmp_cmd, "%s %s", type_cmd, arg);
	  do_type(tmp_cmd);
	}
      else
	{
	  if (nargs > 2) stripper(arg, 2);
	  sprintf(tmp_cmd, "%s %s ", type_cmd, arg);
	  set_left_footer("Input the first and last column");
	  set_cmd_prompt("First Last: ");
	  action = TYPE_FL;
	}
      break;
      
    case TYPE_FL:
      if (nargs == 3)
	{
	  strcat(tmp_cmd, arg);
	  do_type(tmp_cmd);
	}
      else
	{
	  strcat(tmp_cmd, arg);
	  strcat(tmp_cmd, " ");
	  set_left_footer("Input the filename or type 'S' for screen output");
	  set_cmd_prompt("Input: ");
	  action = TYPE_S;
	}
      break;

    case TYPE_S:
      strcat(tmp_cmd, arg);
      do_type(tmp_cmd);
      break;
      
    case TYPEALL:
       sprintf(tmp_cmd, "typeall %s", arg);
       do_typeall(tmp_cmd);
       break;
       
    case SMOOTH:
      if (nargs == 7)
	{
	  sprintf(tmp_cmd, "smooth %s", arg);
	  do_smooth(tmp_cmd);
	}
      else
	{
	  if (nargs > 5) stripper(arg, 5);
	  sprintf(tmp_cmd, "smooth %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = SMOOTH_NAME;
	}
      break;
      
    case SMOOTH_NAME:
      strcat(tmp_cmd, arg);
      do_smooth(tmp_cmd);
      break;

    case MEDIAN_SMOOTH:
      if (nargs == 7)
	{
	  sprintf(tmp_cmd, "median_smooth %s", arg);
	  do_median_smooth(tmp_cmd);
	}
      else
	{
	  if (nargs > 5) stripper(arg, 5);
	  sprintf(tmp_cmd, "median_smooth %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = MEDIAN_SMOOTH_NAME;
	}
      break;
      
    case MEDIAN_SMOOTH_NAME:
      strcat(tmp_cmd, arg);
      do_median_smooth(tmp_cmd);
      break;

    case MEM:
      sprintf(tmp_cmd, "mem %s", arg);
      do_mem(tmp_cmd);
      break;

    case CM:
      sprintf(tmp_cmd, "cm %s", arg);
      do_cm(tmp_cmd);
      break;

    case RSM:
      sprintf(tmp_cmd, "rsm %s", arg);
      do_rsm(tmp_cmd);
      break;

    case SLOPE:
      if (nargs == 8)
	{
	  sprintf(tmp_cmd, "slope %s ", arg);
	  do_slope(tmp_cmd);
	}
      else
	{
	  if (nargs > 6) stripper(arg, 6);
	  sprintf(tmp_cmd, "slope %s ", arg);
	  name_col(getcol(tmp_cmd, 4));
	  action = SLOPE_NAME;
	}
      break;
      
    case SLOPE_NAME:
      strcat(tmp_cmd, arg);
      do_slope(tmp_cmd);
      break;

    case TRIG:
      if (nargs == 5)
	{
	  sprintf(tmp_cmd, "%s %s ", trig_cmd, arg);
	  do_trig(tmp_cmd);
	}
      else
	{
	  if (nargs > 3) stripper(arg, 3);
	  sprintf(tmp_cmd, "%s %s ", trig_cmd, arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = TRIG_NAME;
	}
      break; 

    case TRIG_NAME:
      strcat(tmp_cmd, arg);
      do_trig(tmp_cmd);
      break;

    case RCPH:
      if (nargs == 6)
	{
	  sprintf(tmp_cmd, "rcph %s ", arg);
	  do_rcph(tmp_cmd);
	}
      else
	{
	  if (nargs > 4)
	    stripper(arg, 4);
	  sprintf(tmp_cmd, "rcph %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = RCPH_NAME;
	}
      break; 
      
    case RCPH_NAME:
      strcat(tmp_cmd, arg);
      do_rcph(tmp_cmd);
      break;
      
    case COL_POWER:
      sprintf(tmp_cmd, "col_power %s", arg);
      do_col_power(tmp_cmd);
      break;
      
    case POWER:
      if (nargs == 6)
	{
	  sprintf(tmp_cmd, "rcph %s ", arg);
	  do_power(tmp_cmd);
      	}
      else
	{
	  if (nargs > 4)
	    stripper(arg, 4);
	  sprintf(tmp_cmd, "power %s ", arg);
	  name_col(getcol(tmp_cmd, 4));
	  action = POWER_NAME;
	}      
      break; 

    case POWER_NAME:
      strcat(tmp_cmd, arg);
      do_power(tmp_cmd);
      break;

    case RECIP:
      if (nargs == 4)
	{
	  sprintf(tmp_cmd, "recip %s ", arg);
	  do_recip(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "recip %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = RECIP_NAME;
	}      
      break; 
      
    case RECIP_NAME:
      strcat(tmp_cmd, arg);
      do_recip(tmp_cmd);
      break;
      
    case LOG:
     if (nargs == 4)
	{
	  sprintf(tmp_cmd, "log %s ", arg);
	  do_log(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "log %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = LOG_NAME;
	}
      
      break; 

    case LOG_NAME:
      strcat(tmp_cmd, arg);
      do_log(tmp_cmd);
      break;

    case EXPLIN:
      if (nargs == 7)
	{
	  sprintf(tmp_cmd, "ExpLin %s ", arg);
	  do_ExpLin(tmp_cmd);
      	}
      else
	{
	  if (nargs > 5)
	    stripper(arg, 5);
	  sprintf(tmp_cmd, "ExpLin %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = EXPLIN_NAME;
	}
      
      break; 

    case EXPLIN_NAME:
      strcat(tmp_cmd, arg);  
      do_ExpLin(tmp_cmd);
      break;

    case POLY4:
      if (nargs == 9)
	{
	  sprintf(tmp_cmd, "Poly4 %s ", arg);
	  do_Poly4(tmp_cmd);
      	}
      else
	{
	  if (nargs > 7)
	    stripper(arg, 7);
	  sprintf(tmp_cmd, "Poly4 %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = POLY4_NAME;
	}
      
      break; 

    case POLY4_NAME:
      strcat(tmp_cmd, arg);
      do_Poly4(tmp_cmd);
      break;

    case GENSIN:
     if (nargs == 8)
	{
	  sprintf(tmp_cmd, "gensin %s ", arg);
	  do_gensin(tmp_cmd);
      	}
      else
	{
	  if (nargs > 6)
	    stripper(arg, 6);
	  sprintf(tmp_cmd, "gensin %s ", arg);
      name_col(getcol(tmp_cmd, 3));
      action = GENSIN_NAME;
	}
      
      break; 

    case GENSIN_NAME:
      strcat(tmp_cmd, arg);
      do_gensin(tmp_cmd);
      break;

    case GENEXP:
       if (nargs == 8)
	{
	  sprintf(tmp_cmd, "genexp %s ", arg);
	  do_genexp(tmp_cmd);
      	}
      else
	{
	  if (nargs > 6)
	    stripper(arg, 6);
	  sprintf(tmp_cmd, "genexp %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = GENEXP_NAME;
	}
      
      break; 

    case GENEXP_NAME:
      strcat(tmp_cmd, arg);
      do_genexp(tmp_cmd);
      break;

    case RCLOW:
      if (nargs == 6)
	{
	  sprintf(tmp_cmd, "rclow %s ", arg);
	  do_rclow(tmp_cmd);
      	}
      else
	{
	  if (nargs > 4)
	    stripper(arg, 4);
	  sprintf(tmp_cmd, "rclow %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = RCLOW_NAME;
	}
      
      break; 

    case RCLOW_NAME:
      strcat(tmp_cmd, arg);
      do_rclow(tmp_cmd);
      break;

    case SCCHISQR:
      if (nargs == 7)
	{
	  sprintf(tmp_cmd, "scchisqr %s ", arg);
	  do_scchisqr(tmp_cmd);
      	}
      else
	{
	  if (nargs > 5)
	    stripper(arg, 5);
	  sprintf(tmp_cmd, "scchisqr %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = SCCHISQR_NAME;
	}
      
      break; 

    case SCCHISQR_NAME:
      strcat(tmp_cmd, arg);
      do_scchisqr(tmp_cmd);
      break;

    case CHISQR:
      if (nargs == 5)
	{
	  sprintf(tmp_cmd, "chisqr %s ", arg);
	  do_chisqr(tmp_cmd);
      	}
      else
	{
	  if (nargs > 3)
	    stripper(arg, 3);
	  sprintf(tmp_cmd, "chisqr %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = CHISQR_NAME;
	}
      
      break; 

    case CHISQR_NAME:
      strcat(tmp_cmd, arg);
      do_chisqr(tmp_cmd);
      break;

    case NORMAL:
      if (nargs == 6)
	{
	  sprintf(tmp_cmd, "normal %s ", arg);
	  do_normal(tmp_cmd);
      	}
      else
	{
	  if (nargs > 4)
	    stripper(arg, 4);
	  sprintf(tmp_cmd, "normal %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = NORMAL_NAME;
	}
      
      break; 

    case NORMAL_NAME:
      strcat(tmp_cmd, arg);
      do_normal(tmp_cmd);
      break;

    case POWER2:
      if (nargs == 7)
	{
	  sprintf(tmp_cmd, "Power2 %s ", arg);
	  do_Power2(tmp_cmd);
      	}
      else
	{
	  if (nargs > 5)
	    stripper(arg, 5);
	  sprintf(tmp_cmd, "Power2 %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = POWER2_NAME;
	}
      
      break; 

    case POWER2_NAME:
      strcat(tmp_cmd, arg);
      do_Power2(tmp_cmd);
      break;

    case POWER1:
      if (nargs == 6)
	{
	  sprintf(tmp_cmd, "Power1 %s ", arg);
	  do_Power1(tmp_cmd);
      	}
      else
	{
	  if (nargs > 4)
	    stripper(arg, 4);
	  sprintf(tmp_cmd, "Power1 %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = POWER1_NAME;
	}
      
      break; 

    case POWER1_NAME:
      strcat(tmp_cmd, arg);
      do_Power1(tmp_cmd);
      break; 

    case LN:
      if (nargs == 4)
	{
	  sprintf(tmp_cmd, "ln %s ", arg);
	  do_ln(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "ln %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = LN_NAME;
	}
      
      break; 

    case LN_NAME:
      strcat(tmp_cmd, arg);
      do_ln(tmp_cmd);
      break;

    case EXP:
       if (nargs == 4)
	{
	  sprintf(tmp_cmd, "exp %s ", arg);
	  do_exp(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "exp %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = EXP_NAME;
	}
      
      break; 

    case EXP_NAME:
      strcat(tmp_cmd, arg);
      do_exp(tmp_cmd);
      break;
      
    case DERIV:
      if (nargs == 7)
	{
	   sprintf(tmp_cmd, "deriv %s, ", arg);
	   do_deriv(tmp_cmd);
	 }
      else
	{
	  if (nargs > 5)
	    {
	      stripper(arg, 5);
	      sprintf(tmp_cmd, "deriv %s, ", arg);
	      name_col(getcol(tmp_cmd, 5));
	      action = DERIV_NAME;
	    }
	  else 
	    { 
	      if (nargs > 3) stripper(arg, 3);
	      sprintf(tmp_cmd, "deriv %s, ", arg);
	      set_left_footer("Type the first and last record for the interval");
	      set_cmd_prompt("First, Last: ");
	      action = DERIV_GET_INT;
	    }
	}
      break;
      
    case DERIV_GET_INT:
      if (nargs == 4)
	{
	  strcat(tmp_cmd, arg);
	  do_deriv(tmp_cmd);
	}
      else
	{
	  if (nargs > 2) stripper(arg, 2);
	  strcat(tmp_cmd, arg);
	  strcat(tmp_cmd, " ");
	  name_col(getcol(tmp_cmd, 4));
	  action = DERIV_NAME;
	}
      break; 

    case DERIV_NAME:
      strcat(tmp_cmd, arg);
      do_deriv(tmp_cmd);
      break;
      
    case VC:
      if (nargs == 8)
	{
	  sprintf(tmp_cmd, "vc %s", arg);
	  do_vc(tmp_cmd);
	}
      else
	{
	  if (nargs > 3) stripper(arg, 3);
	  sprintf(tmp_cmd, "vc %s, ", arg);
	  action = VC_GET_INT;
	  set_left_footer("Type the first and last record for the interval");
	  set_cmd_prompt("First, Last: ");
	}
      
      break;

    case VC_GET_INT:
      if (nargs > 2) stripper(arg, 2);
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, ", ");
      action = VC_GET_F;
      sprintf(msg, "(S*Va/V)  (supply the compressibility ([dv/V]/dP) and the ratio of the volume of material effected to the total volume -in the same units as tau \n:");
      print_msg(msg);
      set_left_footer("Type the compressability");
      set_cmd_prompt("[dv/V]/dP: ");
      break;

    case VC_GET_F:
      if (nargs == 3)
	{
	  strcat(tmp_cmd, arg);
	  do_vc(tmp_cmd);
	}
      else
	{
	  if (nargs > 1) stripper(arg, 1);
	  strcat(tmp_cmd, arg);
	  strcat(tmp_cmd, " ");
	  name_col(getcol(tmp_cmd, 4));
	  action = VC_NAME;
	}
      break; 

    case VC_NAME:
      strcat(tmp_cmd, arg);
      do_vc(tmp_cmd);
      break;

   case RGT:
      if (nargs == 6)
	{
	  sprintf(tmp_cmd, "rgt %s ", arg);
	  do_rgt(tmp_cmd);
      	}
      else
	{
	  if (nargs > 4)
	    stripper(arg, 4);
	  sprintf(tmp_cmd, "rgt %s ", arg);
	  name_col(getcol(tmp_cmd, 5));
	  action = RGT_NAME;
	}
      
      break; 

    case RGT_NAME:
      strcat(tmp_cmd, arg);
      do_rgt(tmp_cmd);
      break;

    case CGT:
      if (nargs == 6)
	{
	  sprintf(tmp_cmd, "cgt %s ", arg);
	  do_cgt(tmp_cmd);
      	}
      else
	{
	  if (nargs > 4)
	    stripper(arg, 4);
	  sprintf(tmp_cmd, "cgt %s ", arg);
	  name_col(getcol(tmp_cmd, 5));
	  action = CGT_NAME;
	}
      
      break; 

    case CGT_NAME:
      strcat(tmp_cmd, arg);
      do_cgt(tmp_cmd);
      break;

    case EC:
      if (nargs == 8)
	{
	  sprintf(tmp_cmd, "ec %s", arg);
	  do_ec(tmp_cmd);
	}
      else if (nargs >= 6)
	{
	  if (nargs > 6) stripper(arg, 6);
	  sprintf(tmp_cmd, "ec %s, ", arg);
	  name_col(getcol(tmp_cmd, 4));
	  action = EC_NAME;
	}
      else if (nargs == 5)
	{
	  sprintf(tmp_cmd, "ec %s, ", arg);
	  set_left_footer("Type the stiffness (k) in the same units as force and disp");
	  set_cmd_prompt("k : ");	  
	  action = EC_GET_F;
	}      
      else 
	{
	  if (nargs > 3) stripper(arg, 3);
	  sprintf(tmp_cmd, "ec %s, ", arg);
	  action = EC_GET_INT;
	  set_left_footer("Type the first and last record for the interval");
	  set_cmd_prompt("First, Last: ");
	}
      break;

    case EC_GET_INT:
      if (nargs == 5)
	{
	  strcat(tmp_cmd, arg);
	  do_ec(tmp_cmd);
	}
      else if (nargs >= 3)
	{
	  if (nargs > 3) stripper(arg, 3);
	  strcat(tmp_cmd, arg);
	  name_col(getcol(tmp_cmd, 4));
	  action = EC_NAME;
	}
      else
	{
	  strcat(tmp_cmd, arg);
	  action = EC_GET_F;
	  set_left_footer("Type the ratio (k) in the same units as force and disp");
	  set_cmd_prompt("k : ");
	}
      break;

    case EC_GET_F:
      if (nargs == 3)
	{
	  strcat(tmp_cmd, arg);
	  do_ec(tmp_cmd);
	}
      else
	{
	  if (nargs > 1) stripper(arg, 1);
	  strcat(tmp_cmd, ", ");
	  strcat(tmp_cmd, arg);
	  strcat(tmp_cmd, ", ");
	  name_col(getcol(tmp_cmd, 4));
	  action = EC_NAME;
	}
      break; 

    case EC_NAME:
      strcat(tmp_cmd, arg);
      do_ec(tmp_cmd);
      break;
      
    case CS:
      if (nargs > 3) stripper(arg, 3);
      sprintf(tmp_cmd, "cs %s, ", arg);
      action = CS_GET_INT;
      set_left_footer("Type the first and last record for the interval");
      set_cmd_prompt("First, Last: ");
      break;
      
    case CS_GET_INT:
      if (nargs > 2) stripper(arg, 2);
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      name_col(getcol(tmp_cmd, 4));
      action = CS_NAME;
      break; 

    case CS_NAME:
      strcat(tmp_cmd, arg);
      do_cs(tmp_cmd);
      break;

    case POLYFIT:
      if (nargs > 4) stripper(arg, 4);
      sprintf(tmp_cmd, "polyfit %s, ", arg);
      action = POLYFIT_GET_INT;
      set_left_footer("Type the first and last record for the interval");

      set_cmd_prompt("First, Last: ");
      break;
      
    case POLYFIT_GET_INT:
      if (nargs > 2) stripper(arg, 2);
      strcat(tmp_cmd, arg);
      action = POLYFIT_GET_INC;
      set_left_footer("Extend the fit past the interval (y/n) OR apply to entire col (a) ?");
      set_cmd_prompt("y/n/a: ");
      break;

    case POLYFIT_GET_INC:
      if (arg[0] == 'y' || arg[0] == 'Y')
	{
	  if (nargs > 1) stripper(arg, 1);
	  strcat(tmp_cmd, ", ");
	  strcat(tmp_cmd, arg);
	  set_left_footer("Extend to what row number?");
	  set_cmd_prompt("Row #: ");
	  action = POLYFIT_I_GET_ROW;
	}
      else
	{
	  strcat(tmp_cmd, ", ");
	  strcat(tmp_cmd, arg);
	  strcat(tmp_cmd, " 0 ");	  
          name_col(getcol(tmp_cmd, 4));
	  action = POLYFIT_NAME;
	}
      break;

    case POLYFIT_NAME:
      strcat(tmp_cmd, arg);
      do_polyfit(tmp_cmd);
      break;

    case POLYFIT_I:
      if (nargs > 4) stripper(arg, 4);
      sprintf(tmp_cmd, "polyfit_i %s, ", arg);
      action = POLYFIT_I_GET_INT;
      set_left_footer("Type the first and last record for the interval");
      set_cmd_prompt("First, Last: ");
      break;

    case POLYFIT_I_GET_INT:
      if (nargs > 2) stripper(arg, 2);
      strcat(tmp_cmd, arg);
      action = POLYFIT_I_GET_INC;
      set_left_footer("Type the dx increment or 'd' for default (max_x-min_x/#of rows");
      set_cmd_prompt("dx increment: ");
      break;

    case POLYFIT_I_GET_INC:
      if (arg[0] == 'd')
	{
	  strcat(tmp_cmd, ", ");
	  strcat(tmp_cmd, arg);
	  strcat(tmp_cmd, ", 0");
	  do_polyfit(tmp_cmd);
	}
      else
	{
	  if (nargs > 1) stripper(arg, 1);
	  strcat(tmp_cmd, ", ");
	  strcat(tmp_cmd, arg);
	  set_left_footer("Extend to what row number?");
	  set_cmd_prompt("Row #: ");
	  action = POLYFIT_I_GET_ROW;
	}
      break;

    case POLYFIT_I_GET_ROW:
      strcat(tmp_cmd, " ");
      strcat(tmp_cmd, arg);
      do_polyfit(tmp_cmd);
      break;

    case SORT:
      sprintf(tmp_cmd, "sort %s", arg);
      do_sort(tmp_cmd);
      break;

    case INTERPOLATE_R:
      if (nargs > 4) stripper(arg, 4);
      sprintf(tmp_cmd, "interpolater %s ", arg);
      action = INTERPOLATE_GET_F_L;
      set_left_footer("Type the first and last record for the interval");
      set_cmd_prompt("First, Last: ");
      break;

    case INTERPOLATE:
      if (nargs > 4) stripper(arg, 4);
      sprintf(tmp_cmd, "interpolate %s ", arg);
      action = INTERPOLATE_GET_F_L;
      set_left_footer("Type the first and last record for the interval");
      set_cmd_prompt("First, Last: ");
      break;

    case INTERPOLATE_GET_F_L:
      if (nargs > 2) stripper(arg, 2);
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      action = INTERPOLATE_GET_INC;
      set_left_footer("Type the dx increment or 'd' to get a default");
      set_cmd_prompt("Increment: ");
      break;

    case INTERPOLATE_GET_INC:
      if (nargs > 1) stripper(arg, 1);
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      name_col(getcol(tmp_cmd, 4));
      action = INTERPOLATE_NAME1;
      break; 

    case INTERPOLATE_NAME1:
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      name_col(getcol(tmp_cmd, 5));
      action = INTERPOLATE_NAME2;
      break;

    case INTERPOLATE_NAME2:
      strcat(tmp_cmd, arg);
      do_interpolate(tmp_cmd);
      break;

    case MATHINT :
      if (nargs > 5) stripper(arg, 5);
      sprintf(tmp_cmd, "mathint %s ", arg);
      action = MATHINT_F_L;
      set_left_footer("Type the first and last record for the interval");
      set_cmd_prompt("First, Last: ");
      break;
      
    case MATHINT_F_L:
      if (nargs > 2) stripper(arg, 2);
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, ", ");
      name_col(getcol(tmp_cmd, 6));
      action = MATHINT_NAME;
      break; 

    case MATHINT_NAME:
      strcat(tmp_cmd, arg);
      do_mathint(tmp_cmd);
      break;

    case MATH:
      if (nargs == 7)
	{
	  sprintf(tmp_cmd, "math %s ", arg);
	  do_math(tmp_cmd);
      	}
      else
	{
	  if (nargs > 5)
	    stripper(arg, 5);
	  sprintf(tmp_cmd, "math %s ", arg);
	  name_col(getcol(tmp_cmd, 6));
	  action = MATH_NAME;
	}
      
      break; 

    case MATH_NAME:
      strcat(tmp_cmd, arg);
      do_math(tmp_cmd);
      break;
      
    case POSITIVE:
        if (nargs == 4)
	{
	  sprintf(tmp_cmd, "positive %s ", arg);
	  do_positive(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "positive %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = POSITIVE_NAME;
	}
      
      break; 

    case POSITIVE_NAME:
      strcat(tmp_cmd, arg);
      do_positive(tmp_cmd);
      break;

    case COMPRESS:
       if (nargs == 4)
	{
	  sprintf(tmp_cmd, "compress %s ", arg);
	  do_compress(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "compress %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = COMPRESS_NAME;
	}
      
      break; 

    case COMPRESS_NAME:
      strcat(tmp_cmd, arg);
      do_compress(tmp_cmd);
      break;

    case R_MEAN:
       if (nargs == 4)
	{
	  sprintf(tmp_cmd, "r_mean %s ", arg);
	  do_r_mean(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "r_mean %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = R_MEAN_NAME;
	}
      
      break; 

    case R_MEAN_NAME:
      strcat(tmp_cmd, arg);
      do_r_mean(tmp_cmd);
      break;

    case R_SPIKE:
      sprintf(tmp_cmd, "r_spike %s", arg);
      do_r_spike(tmp_cmd);
      break;

    case Z_MAX:
    if (nargs == 4)
	{
	  sprintf(tmp_cmd, "z_max %s ", arg);
	  do_z_max(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "z_max %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = Z_MAX_NAME;
	}
      
      break; 

    case Z_MAX_NAME:
      strcat(tmp_cmd, arg);
      do_z_max(tmp_cmd);
      break;

    case Z_MIN:
      if (nargs == 4)
	{
	  sprintf(tmp_cmd, "z_min %s ", arg);
	  do_z_min(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "z_min %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = Z_MIN_NAME;
	}
      
      break; 

    case Z_MIN_NAME:
      strcat(tmp_cmd, arg);
      do_z_min(tmp_cmd);
      break;



      /* r_trend -> r_trend_type -> r_trend_input -> r_trend_input_name */
      /* r_trend -> r_trend_type -> r_trend_comp -> r_trend_comp_fl -> r_trend_comp_name */      
    case R_TREND:
      sprintf(tmp_cmd, "r_trend %s ", arg);
      set_left_footer("Input slope and intercept or compute from least squares?");
      set_cmd_prompt("Input/Comp: ");
      action = R_TREND_TYPE;
      break;

    case R_TREND_TYPE:
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      if (strncmp(arg, "input", 5) == 0 || strncmp(arg, "i", 1) == 0)
	{
	  set_left_footer("Input the X col, Y col, New col, Slope and Y-intercept");
	  set_cmd_prompt("X-col Y-col New-col Slope Y-int: ");
	  action = R_TREND_INPUT;
	}
      else
	{
	  set_left_footer("Input the X col, Y col and New col");
	  
	  set_cmd_prompt("X-col Y-col New-col: ");
	  action = R_TREND_COMP;
	}
      break;
      
    case R_TREND_INPUT:
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      name_col(getcol(tmp_cmd, 6));
      action = R_TREND_INPUT_NAME;
      break;

    case R_TREND_INPUT_NAME:
      strcat(tmp_cmd, arg);
      do_r_trend_input(tmp_cmd);
      break;

    case R_TREND_COMP:
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      set_left_footer("Input the interval for linear trend analysis");
      set_cmd_prompt("First Last: ");
      action = R_TREND_COMP_FL;
      break;
      
    case R_TREND_COMP_FL:
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      name_col(getcol(tmp_cmd, 6));
      action = R_TREND_COMP_NAME;
      break;
      
    case  R_TREND_COMP_NAME:
      strcat(tmp_cmd, arg);
      do_r_trend_comp(tmp_cmd);
      break;

    case TREND_XY:
      if (nargs > 2) stripper(arg, 2);
      sprintf(tmp_cmd, "trend %s, ", arg);
      action = TREND_F_L;
      set_left_footer("Type the first and last record number");
      set_cmd_prompt("First, Last: ");
      break;

    case TREND_F_L:
      strcat(tmp_cmd, arg);
      do_trend(tmp_cmd);
      break;

    case TREND_A:
      sprintf(tmp_cmd, "trend_a %s", arg);
      do_trend_a(tmp_cmd);
      break;

    case FINDINT:
      sprintf(tmp_cmd, "findint %s", arg);
      do_findint(tmp_cmd);
      break;

    case SUMMATION:
       if (nargs == 4)
	{
	  sprintf(tmp_cmd, "summation %s ", arg);
	  do_summation(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "summation %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = SUMMATION_NAME;
	}
      
      break; 

    case SUMMATION_NAME:
      strcat(tmp_cmd, arg);
      do_summation(tmp_cmd);
      break;

    case CURV:
      if (nargs == 4)
	{
	  sprintf(tmp_cmd, "curv %s ", arg);
	  do_curv(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "curv %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = CURV_NAME;
	}
      
      break; 

    case CURV_NAME:
      strcat(tmp_cmd, arg);
      do_curv(tmp_cmd);
      break;

    case PEAK:
        if (nargs == 4)
	{
	  sprintf(tmp_cmd, "peak %s ", arg);
	  do_peak(tmp_cmd);
      	}
      else
	{
	  if (nargs > 2)
	    stripper(arg, 2);
	  sprintf(tmp_cmd, "peak %s ", arg);
	  name_col(getcol(tmp_cmd, 3));
	  action = PEAK_NAME;
	}
      
      break; 

    case PEAK_NAME:
      strcat(tmp_cmd, arg);
      do_peak(tmp_cmd);
      break;

    case DECIMAT:
      if (nargs > 3) stripper(arg, 3);
      sprintf(tmp_cmd, "decimat %s, ", arg);
      set_left_footer("Interval for decimation? (first_row, last_row)");
      set_cmd_prompt("First, Last: ");
      action = DECIMAT_F_L;
      break;
      
    case DECIMAT_R:
      if (nargs > 3) stripper(arg, 3);
      sprintf(tmp_cmd, "decimat_r %s, ", arg);
      set_left_footer("Interval for decimation? (first_row, last_row)");
      set_cmd_prompt("First, Last: ");
      action = DECIMAT_F_L;
      break;

    case DECIMAT_F_L:
      if (nargs > 2) stripper(arg, 2);
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, " ");
      name_col(getcol(tmp_cmd, 3));
      action = DECIMAT_NAME;
      break; 

    case DECIMAT_NAME:
      strcat(tmp_cmd, arg);
      do_decimat(tmp_cmd);
      break;
      
    case PDF:
      sprintf(tmp_cmd, "pdf %s", arg);
      do_pdf(tmp_cmd);
      action = MAIN;
      break;

    case PDFAUTO:
      sprintf(tmp_cmd, "pdfauto %s", arg);
      do_pdfauto(tmp_cmd);
      action = MAIN;
      break;

    case R_ROW_COL:
      if (nargs > 1) stripper(arg, 1);
      sprintf(tmp_cmd, "r_row_col, %s, ", arg);
      set_left_footer("Type the first and last row to remove");
      set_cmd_prompt("First, Last: ");
      action = R_ROW_COL_F_L;
      break;
      
    case R_ROW_COL_F_L:
      strcat(tmp_cmd, arg);
      do_r_row_col(tmp_cmd);
      action = MAIN;
      break;

    case R_ROW_F_L:
      sprintf(tmp_cmd, "r_row %s", arg);
      do_r_row(tmp_cmd);
      action = MAIN;
      break;

    case COMMENT_COL:
      if (nargs > 2)
	{
	  sprintf(tmp_cmd, "comment %s", arg);
	  do_comment(tmp_cmd);
	  action = MAIN;
	}
      else
	{
	  sprintf(tmp_cmd, "comment %s ", arg);
	  set_left_footer("Type comment (up to 50 chars)");
	  set_cmd_prompt("Comment: ");
	  action = COMMENT_STR;
	}
      break;
      
    case COMMENT_STR:
      strcat(tmp_cmd, arg);
      do_comment(tmp_cmd);
      break;

    case NAME:
      sprintf(tmp_cmd, "cmd %s", arg);
      do_name(tmp_cmd);
      break;

    case R_COL:
      do_r_col(atoi(arg));
      break;

    case ZERO_ALL:
      sprintf(tmp_cmd, "zero_all %s", arg); 
      do_zero_all(tmp_cmd);
      break;

    case ZERO:
      sprintf(tmp_cmd, "zero %s", arg); 
      do_zero(tmp_cmd);
      break;

    case OFFSET:
      sprintf(tmp_cmd, "offset %s", arg); 
      do_offset(tmp_cmd);
      break;
      
    case OFFSET_INT:
      if (nargs == 4)
	{
	   sprintf(tmp_cmd, "offset_int %s", arg); 
	   do_offset_int(tmp_cmd);
	 }
      else
	{
	/*  if (nargs > 3)
	    stripper(arg, 3); */
	  sprintf(tmp_cmd, "offset_int %s, ", arg); 
	  set_left_footer("Set COL values between REC1 and REC2 equal to REC1?");
	  set_cmd_prompt("y/n: ");
	  action = OFFSET_INT_GET_YN;
	}
      
      break;

    case OFFSET_INT_GET_YN:
      strcat(tmp_cmd, arg);
      do_offset_int(tmp_cmd);
      break;
      
    case SIMPLEX_GET_FN:
      if (nargs > 1)
	stripper(arg, 1);
      sprintf(tmp_cmd, "simplex %s, ", arg); 
      set_left_footer("Enter the first and last records for fit");
      set_cmd_prompt("First, Last: ");
      action = SIMPLEX_GET_F_L;
      break;

    case SIMPLEX_GET_F_L:
      if (nargs > 2)
	stripper(arg, 2);
      strcat(tmp_cmd, arg);
      set_left_footer("Enter the X-col and Y-col");
      set_cmd_prompt("X-col, Y-col: ");
      action = SIMPLEX_GET_COLS;
      break;

    case SIMPLEX_GET_COLS:
      strcat(tmp_cmd, ", ");
      strcat(tmp_cmd, arg);
      do_simplex(tmp_cmd);
      break;

    case EXAMIN_GET_FILENAME:
      do_examin(arg);
      action = MAIN;
      break;

    case HEAD_GET_FILENAME:
      do_head(arg);
      action = MAIN;
      break;

    case GETASCHEAD_GET_FILENAME:
      do_head(arg);
      action = MAIN;
      break;

    case TASC_GET_FILENAME:
      if (nargs == 2)
	{
	  sprintf(tmp_cmd, "tasc %s, ", arg); 
	  do_tasc(tmp_cmd);
	}
      else
	{
	  if (nargs > 1)
	    stripper(arg, 1);
	  sprintf(tmp_cmd, "tasc %s, ", arg); 
	  set_left_footer("Integer or float data?");
	  set_cmd_prompt("integer/float: ");
	  action = TASC_GET_FI;
	}
      break;
      
    case TASC_GET_FI:
      strcat(tmp_cmd, arg);
      do_tasc(tmp_cmd);
      break;
      
    case STDASC_GET_FILENAME:
      if (nargs > 1)
	stripper(arg, 1);
      sprintf(tmp_cmd, "stdasc %s, ", arg); 
      set_left_footer("Read complex or real data?");
      set_cmd_prompt("complex/real: ");
      action = STDASC_GET_CR;
      break;
      
    case STDASC_GET_CR:
      if (nargs > 1)
	stripper(arg, 1);
      strcat(tmp_cmd, arg);
      set_left_footer("Use float or scaled integer format?");
      set_cmd_prompt("float/int: ");
      action = STDASC_GET_FI;
      break;
      
    case STDASC_GET_FI:
      strcat(tmp_cmd, ", ");
      strcat(tmp_cmd, arg);
      do_stdasc(tmp_cmd);
      break;
      
    case DOIT:
      doit_proc(arg);
      break;
      
    case READ:
      sprintf(tmp_cmd, "read  %s", arg);
      read_proc(tmp_cmd);
      break;
      
    case APPEND:
      sprintf(tmp_cmd, "append  %s", arg);
      read_proc(tmp_cmd);
      break;
      
    case WRITE:
      write_proc(arg);
      break;

    case ALL_NEED_ROW_COL:
      sprintf(tmp_cmd, "all yes, %s", arg);
      all_final_proc(tmp_cmd);
      action = MAIN;
      break;
      
    case SET_PATH:
      set_path_proc(arg);
      action = MAIN;
      break;

    case PLOT_GET_XY:
      sprintf(tmp_cmd, "%s %s", plot_cmd, arg);
      do_plot(tmp_cmd);
      break;
      
    case PLOT_GET_BE:
      if (nargs > 2) stripper(arg, 2);
      sprintf(tmp_cmd, "%s %s, ", plot_cmd, arg);
      set_left_footer("Type the first and last records to plot.");
      set_cmd_prompt("Begin, End: ");
      if (strncmp(plot_cmd, "plotscale", 9) == 0)
	action = PLOT_SCALE;
      else
	action = PLOT_OTHERS;
      break;
      
    case PLOT_SCALE:
      if (nargs > 2) stripper(arg, 2);
      strcat(tmp_cmd, arg);
      strcat(tmp_cmd, ", ");
      set_left_footer("Type the limits X_max, X_min, Y_max, Y_min");
      set_cmd_prompt("X_max, X_min, Y_max, Y_min: ");
      action = PLOT_OTHERS;
      break;
      
    case PLOT_OTHERS:
      strcat(tmp_cmd, arg);
      do_plot(tmp_cmd);
      break;

    case SET_ACTIVE_PLOT:
      set_active_plot(atoi(arg)-1);
      action = MAIN;
      top();
      break;
            
    case SET_ACTIVE_WINDOW:
      set_active_window(atoi(arg)-1);
      action = MAIN;
      top();
      break;

    case KILL_WIN:
      kill_win_proc(atoi(arg));
      action = MAIN;
      break;

    case KILL_PLOT:
      del_plot_proc(atoi(arg));
      action = MAIN;
      break;
      
    default:
      action = MAIN;
      top();
      break;
      
    }
}
      
