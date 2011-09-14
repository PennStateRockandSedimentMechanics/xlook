#include <string.h>
#include <unistd.h>
#include <sys/file.h>

#include "can.h"
#include "cmds1.h"
#include "config.h"
//#include "drawwin.h"
#include "event.h"
#include "filtersm.h"
#include "global.h"
#include "look_funcs.h"
#include "messages.h"
#include "notices.h"
#include "rs_fric_tool.h"
#include "special.h"
#include "strcmd.h"

/* cjm 14.5.07; to solve problem with doit files: increased the size of path names. 1024's used to be 80, 512 used to be 32 */
char pathname[10][1024];
char default_path[1024];
char metapath[1024];
char data_file[512];
char new_file[512];
char headline[512];

FILE *data, *new;

int doit_des, doit_f_open, meta_fd;
int plot_error;
int read_flag = 0; 
int qi_flag = 0;

extern char plot_cmd[256], trig_cmd[256];

extern char msg[MSG_LENGTH];


void new_win_proc()
{
  ui_globals.total_windows++;
  create_plot_window();
  sprintf(msg, "Total windows = %d\n", ui_globals.total_windows+1);
  print_msg(msg);
}

void qi_win_proc()
{

 if (read_flag != 0) {
#ifdef FIXME
    if (qi_flag == 1) 
      update_params();
    else {
      create_qi_canvas();
      qi_flag = 1;
    }
#endif
  } 
 else {
   sprintf(msg, "File has not been read yet. File must be read first.\n");
   print_msg(msg);
   set_left_footer("Type the data file to read");	  
   ui_globals.action = READ;
   set_cmd_prompt("Filename: ");
 }

}


void set_active_window(win_num)
     int win_num;
{
  if (wininfo.windows[win_num] != 1)
    {
      sprintf(msg, "Window #%d does not exist.\n", win_num+1);
      print_msg(msg);
      ui_globals.action = MAIN;
      top();
      return;
    }
  
  if (ui_globals.active_window != win_num)
    {
      if (win_num == ui_globals.old_active_window)
	{
	  ui_globals.old_active_window = ui_globals.active_window;
	  ui_globals.active_window = win_num;
	}
      else
	{
	  ui_globals.old_active_window = ui_globals.active_window;
	  ui_globals.active_window = win_num;
	}
    }

  display_active_window(ui_globals.active_window+1);
  display_active_plot(wininfo.canvases[ui_globals.active_window]->active_plot+1);  
  display_active_file(1);
}


void set_active_plot(int i)
{
	canvasinfo *can_info;
	fprintf(stderr, "set_active_plot(%d)\n", i);
	can_info = wininfo.canvases[ui_globals.active_window];
	set_active_plot_in_window(can_info->plot_window, i);
}

void del_plot_proc(int pn)
{
	canvasinfo *can_info;

	can_info = wininfo.canvases[ui_globals.active_window];
	remove_plot_in_window(can_info->plot_window, pn);
}

void kill_win_proc(int wn)
{
	canvasinfo *can_info;

	wn--;

	if (wn < 0 || wininfo.windows[wn] == 0)
	{
		sprintf(msg, "Window #%d does not exist.\n", wn+1);
		print_msg(msg);
		ui_globals.action = MAIN;
		top();
		return;
	}

	sprintf(msg, "destroying window #%d\n", wn+1);
	print_msg(msg);  
  
	can_info = wininfo.canvases[wn];
	kill_plot_window(can_info->plot_window);
	wininfo.windows[wn] = 0;

	// now restore to a sane state (this should all go away- rely on your window manager)
	if (ui_globals.active_window == wn)
	{
		ui_globals.active_window = ui_globals.old_active_window;
		ui_globals.old_active_window = get_old_active_window(ui_globals.active_window);
	}

	if (ui_globals.old_active_window == wn)
	{
		ui_globals.old_active_window = get_old_active_window(ui_globals.old_active_window);
	}

	ui_globals.total_windows--;
	if (ui_globals.total_windows == -1)
	{
		ui_globals.active_window = -1;
		ui_globals.old_active_window = -1;
	}

	display_active_window(ui_globals.active_window+1);
	/* if no window is opened, active_plot = NONE */
	if (ui_globals.active_window != -1)
		display_active_plot(wininfo.canvases[ui_globals.active_window]->active_plot+1);
	else
		display_active_plot(-1);

	sprintf(msg, "Total windows = %d\n", ui_globals.total_windows+1);
	print_msg(msg);  

	ui_globals.action = MAIN;
	top();
}


int get_old_active_window(wn)
     int wn;
{
  int i;
  
  if (wn > 0)
    {
      for (i=wn-1; i>=0; i--)
	{
	  if (wininfo.windows[i] == 1)
	    return i;
	}
    }
  
  for (i=wn+1; i<10; i++)
    {
      if (wininfo.windows[i] == 1)
	return i;
    }
  return -1;
}


/* 19/3/96 cjm: this function should renumber after clearing a plot*/
int get_new_plot_num(alive_plots)
     int alive_plots[10];
{
  int i;
  
  for (i = 0; i < 10; i++)
    {
      if (alive_plots[i] == 0)
	return i;
    }
  /* reached plots limit.. need to delete one or more plots from window */
  return(-1);
}


void plotting_error(pn)
     int pn;
{
  canvasinfo *can_info;
  int i;
  
  can_info = wininfo.canvases[ui_globals.active_window];
  can_info->alive_plots[pn] = 0;
  can_info->total_plots--;
  free(can_info->plots[pn]);
  for(i=0; i<MAX_PLOTS; i++)
    {
      if (can_info->alive_plots[i] == 1)
	{
	  can_info->active_plot = i;
	  return;
	}
    }
  can_info->active_plot = -1;
}


void do_plot(char *cmd)
{
	char desire[256];
	int i, j;
	canvasinfo *can_info;
	plotarray *data;
	int begin, end;
	int xaxis, yaxis;
	int oldplot;
	double xmax, xmin, ymax, ymin;
	int plot_num;
	int initial_active_plot;

	plot_error = 0;

	nocom(cmd);

	if (ui_globals.total_windows == -1) new_win_proc();  

	can_info = wininfo.canvases[ui_globals.active_window];
	initial_active_plot= can_info->active_plot;

	if( (strncmp(plot_cmd, "plotover", 8) == 0 || 
		(strncmp(plot_cmd, "plotsr", 6) == 0)  ||
		(strncmp(plot_cmd, "plotsame", 8) == 0))) 
	{
		if(can_info->total_plots == 0)
		{
			sprintf(msg, "No previous plot. You can't use \"plotover\" etc. since they plot at scale of previous plot.\n");
			print_msg(msg);
			top();
			ui_globals.action = MAIN;
			return;
		}
		else
		{
			oldplot = can_info->active_plot;
		}
	}

	if (can_info->total_plots >= MAX_PLOTS)
	{
		sprintf(msg, "Reached limit of %d plots per window! Command Ignored\n", MAX_PLOTS);
		print_msg(msg);
		top();
		ui_globals.action = MAIN;
		return;
	}

	can_info->total_plots += 1;
	/*  printf("(plot proc)total_plots: %d\n", can_info->total_plots+1); */


	data = (plotarray *) malloc(sizeof(plotarray));
	if (data == NULL)
	{
		can_info->total_plots -= 1;
		sprintf(msg,"Memory allocation error. Plot cannot be drawn.\n");
		print_msg(msg);
		top();
		ui_globals.action = MAIN;
		return;
	}
	
	plot_num = get_new_plot_num(can_info->alive_plots);  	/*get a new number, next in list*/
	can_info->alive_plots[plot_num] = 1;			/*set this one alive*/
	can_info->active_plot = plot_num;			/*make it the active plot*/
	can_info->plots[plot_num] = data;		/*data is a pointer to plotarray struct*/

	/* set the label type */
	if (strcmp(plot_cmd, "pa") == 0 || strcmp(plot_cmd, "plotall") == 0)
		data->label_type = 1;  /* msp-style label */
	else
		data->label_type = 0;  /* regular label */

	/*plotover: Save rows and plot scale. Plot new cols at the same scale as the active plot.*/
	if (strcmp(plot_cmd, "plotover") == 0)
	{
		if (sscanf(cmd, "%s %d %d", desire, &xaxis, &yaxis) != 3)
		{
			nea();
			plotting_error(plot_num);
			top();
			ui_globals.action = MAIN;
			return;
		}
		else
		{
			/* copy values from previous plot */
			xmax = can_info->plots[oldplot]->xmax;
			xmin = can_info->plots[oldplot]->xmin;
			ymax = can_info->plots[oldplot]->ymax;
			ymin = can_info->plots[oldplot]->ymin;

			begin =  can_info->plots[oldplot]->begin;
			end = can_info->plots[oldplot]->end;
			
			if (xaxis >= max_col || yaxis >= max_col)
			{
				sprintf(msg, "Out of range. Col not allocated.\n");
				print_msg(msg);
				plotting_error(plot_num);
				top();
				ui_globals.action = MAIN;
				return;
			}
		}
	}
	else
	{
		if (strcmp(plot_cmd, "plotall") == 0)
		{
			if (sscanf(cmd, "%s %d %d", desire, &xaxis, &yaxis) != 3)
			{
				nea();
				plotting_error(plot_num);
				top();
				ui_globals.action = MAIN;
				return;
			}
			else
			{
				begin = 0;
				/* 28/3/96: note that this could be a problem, should use [col]nelem and  
				should inform user if x and x nelem are not the same...*/ 
				end = head.nrec - 1;
			}
		}
		else if (strcmp(plot_cmd, "plotsr") == 0)
		{	
			/*plotsr: plot the same row numbers but different cols, autoscale*/
			if (sscanf(cmd, "%s %d %d", desire, &xaxis, &yaxis) != 3)
			{
				nea();
				plotting_error(plot_num);
				top();
				ui_globals.action = MAIN;
				return;
			}

			begin = can_info->plots[oldplot]->begin;
			end = can_info->plots[oldplot]->end;
		}
		else if (strcmp(plot_cmd, "plotscale") == 0)
		{	
			if (sscanf(cmd, "%s %d %d %d %d %lf %lf %lf %lf", desire, 
				&xaxis, &yaxis, &begin, &end,
				&xmax, &xmin,
				&ymax, &ymin) != 9)
			{
				nea();
				plotting_error(plot_num);
				top();
				ui_globals.action = MAIN;
				return;
			}
		}
		/* plotsame: plot at the same scale but allow different rows and cols*/
		else    /* plotauto, plotlog, plotsame, pa */ 
		{
			if (sscanf(cmd, "%s %d %d %d %d", desire, &xaxis, &yaxis, &begin, &end) != 5)
			{
				nea();
				plotting_error(plot_num);
				top();
				ui_globals.action = MAIN;
				return;
			}
		}

		if (xaxis >= max_col || yaxis >= max_col)
		{
			sprintf(msg, "Out of range. Col not allocated.\n");
			print_msg(msg);
			plotting_error(plot_num);
			top();
			ui_globals.action = MAIN;
			return;
		}

		if (strcmp(plot_cmd, "plotscale") == 0 || strcmp(plot_cmd, "plotauto") == 0 || strcmp(plot_cmd, "plotlog") == 0 || strcmp(plot_cmd, "plotsame") == 0 || strcmp(plot_cmd, "pa") == 0)
		{
			if (end > head.nrec-1)
			{
				sprintf(msg, "nrec = %d, truncating interval to [%d, %d]\n", head.nrec, begin, head.nrec-1);
				print_msg(msg);
				end = head.nrec - 1;
			}
			if (check_row(&begin, &end, xaxis) != 0)
			{
				plot_error = 1;
				begin = 0;
			}
		}

		/* end reading arguments */

		if (strncmp(desire, "plotauto", 8) == 0 || strncmp(desire, "plotall", 7) == 0 || strncmp(desire, "plotsr", 6) == 0 || strncmp(desire, "pa", 2) == 0)
		{
			stats(&xaxis, &begin, &end);
			xmax = col_stat.max;
			xmin = col_stat.min;

			stats(&yaxis, &begin, &end);
			ymax = col_stat.max;
			ymin = col_stat.min;
		}
		else if (strncmp(desire, "plotlog", 7) == 0)
		{
			stats(&xaxis, &begin, &end);
			if (col_stat.min <= 0)
			{
				sprintf(msg, "Negative numbers within range. Can't do log_plot.\n"); 
				print_msg(msg);
				plot_error = 1;
				plotting_error(plot_num);
				top();
				ui_globals.action = MAIN;
				return;
			}
			else
			{
				xmax = ceil(log10((double)col_stat.max));
				xmin = floor(log10((double)col_stat.min));
			}
			stats(&yaxis, &begin, &end);
			if (col_stat.min <= 0)
			{
				sprintf(msg, "Negative numbers within range. Can't do log_plot.\n");
				print_msg(msg);
				plot_error = 1;
				plotting_error(plot_num);
				top();
				ui_globals.action = MAIN;
				return;	      
			}
			else
			{
				ymax = ceil(log10((double)col_stat.max));
				ymin = floor(log10((double)col_stat.min));
			}
		}
		else if (strncmp(desire, "plotsame", 8) == 0)
		{
			xmax = can_info->plots[oldplot]->xmax;
			xmin = can_info->plots[oldplot]->xmin;
			ymax = can_info->plots[oldplot]->ymax;
			ymin = can_info->plots[oldplot]->ymin;
		}

		if ((end < begin) || (xmax <= xmin) || (ymax <= ymin))
		{
		/*  printf("begin:%d end:%d max.xaxis:%.3f min.xaxis:%.3f max.yaxis:%.3f min.yaxis:%.3f\n", begin, end, xmax, xmin, ymax, ymin); */
			sprintf(msg, "Limits problem: MAX <= MIN ???\n"); 
			print_msg(msg);

			plotting_error(plot_num);
			plot_error = 1;
			top();
			ui_globals.action = MAIN;	  
			return;
		}
	}

	/* sets all the variables in plot_array data structure */
	data->begin = begin;
	data->end = end;  
	data->col_x = xaxis;
	data->col_y = yaxis;
	data->xmax = xmax;
	data->xmin = xmin;
	data->ymax = ymax;
	data->ymin = ymin;  
	data->nrows_x = end - begin +1;
	data->nrows_y = end - begin +1;
	data->mouse = 0;
	data->x1 = 0;
	data->x2 = 0;
	data->y1 = 0;
	data->y2 = 0;
	data->p1 = 0;
	data->p2 = 0;
	data->zp1 = 0;
	data->zp2 = 0;

	data->xarray = (double *)malloc(sizeof(double)*data->nrows_x);
	data->yarray = (double *)malloc(sizeof(double)*data->nrows_x);

	/* copy data from darray to xarray and yarray (plot data arrays) */
	j = 0;
	if (strncmp(desire, "plotlog", 7) == 0)
	{
		for (i = begin; i <= end; ++i)
		{
			data->xarray[j] = log10((double)darray[xaxis][i]);
			data->yarray[j] = log10((double)darray[yaxis][i]);
			j++;
		}
	}
	else
	{
		for (i = begin; i <= end; ++i)
		{
			data->xarray[j] = darray[xaxis][i];
			data->yarray[j] = darray[yaxis][i];
			j++;
		}
	}


/*
	sprintf(msg, "Active window: %d    Canvas num: %d    Plot num: %d\n", ui_globals.active_window+1, can_info->canvas_num+1, can_info->active_plot+1);
	print_msg(msg);
*/

	//  canvas = can_info->canvas;
	//  canvas_xv_window = can_info->xvwin;
	//  win = can_info->win;

	/* draw the plot */
/*
	redraw_proc(canvas, canvas_xv_window, (Display *)xv_get(canvas, XV_DISPLAY),
	(Window)xv_get(canvas_xv_window, XV_XID), NULL);
*/
	/* 28.3.08 cjm, took out. I think this is the line I put in ~ 5/07 to get plots to appear when the plot window first opened
	but, since then I've found that the problem with blank plots was due to the xv_set command and these calls:
	WIN_CONSUME_EVENTS, WIN_NO_EVENTS, WIN_MOUSE_BUTTONS
	so
*/
	// rdm added:
	if(initial_active_plot != can_info->active_plot)
	{
		set_active_plot_in_window(can_info->plot_window, can_info->active_plot);
	} else {
		invalidate_plot_window(can_info->plot_window);
	}

	top();
	ui_globals.action = MAIN;
}




void write_proc(arg)
     char arg[256];
{
  
  strcpy(new_file, arg);

  if ((new = fopen(new_file, "a")) == NULL)
    {
      sprintf(msg, "Can't open file: %s. fopen in write_proc() failed. See cmds1.c\n", new_file);
      print_msg(msg);
      ui_globals.action =MAIN;
      top();
      return;  
    }
  else
    {
      /* file already exist */
      if ((int)ftell(new) > 0)
	{
	  /* ask if want to overwrite old file */
	  if (write_show_warning() != 1)
	    {
	      /* don't overwrite */
	      sprintf(msg, "Write aborted!\n");
	      print_msg(msg);
	      ui_globals.action =MAIN;
	      top();
	      return;
	    }
	  else
	    {
	      if ((new = fopen(new_file, "w")) == NULL)
		{
		  sprintf(msg, "Can't open file: %s.\n", new_file);
		  print_msg(msg);
		  ui_globals.action =MAIN;
		  top();
		  return;
		}
	      else
		{
		  sprintf(msg, "Overwriting %s.\n", new_file);
		  print_msg(msg); 
		}
	    }
	}

      if (act_col() > head.nchan)
	{
	  sprintf(msg, "Active columns = %d\n", act_col());
	  print_msg(msg);
	  sprintf(msg, "Please compact data array to use columns 1 through head.nchan[%d]\n", head.nchan);
	  print_msg(msg);
	  sprintf(msg, "Reallocation is not necessary.\n");
	  print_msg(msg);
	  ui_globals.action =MAIN;
	  top();
	  return;	
	}
      
      rite_lookfile(new);
      fclose(new);
      sprintf(msg, "File %s written. \n", new_file);
      print_msg(msg);
      ui_globals.action =MAIN;
      top(); 
    }
}


void read_proc(cmd)
     char cmd[256];
{
  char dummy[256];
  
  sscanf(cmd, "%s %s", dummy, data_file);

  if ((data = fopen(data_file, "r")) == NULL)
    {
      sprintf(msg, "Can't open data file: %s. fopen in read_proc() failed. Check filename, see cmds1.c\n", data_file);
      print_msg(msg);
      ui_globals.action = MAIN;
      top();
      return;
    }
  else
    {
      sprintf(msg, "Reading %s...\n", data_file);
      print_msg(msg);
      
      /*  reed is in filtersm.c   */
      if (reed(data, ((strncmp(dummy, "append", 6) == 0) ? TRUE : FALSE)) != 1)
	{
          display_active_file(0);
	  fclose(data);
	  ui_globals.action = MAIN;
	  top();
	  return;
	}
      display_active_file(1);
      fclose(data);
      sprintf(msg, "Reading %s done.\n", data_file);
      print_msg(msg);
      ++read_flag; /* file successfully opened */
      ui_globals.action = MAIN;
      top();
    }
}
  

void doit_proc(arg)
     char arg[256];
{
  /* reads a doit file and passes the commands to command_handler() directly */  
  char *fgets();
  char cmd[256];
  int i, j, error;
  
  
  if ((doit_f_open+1) > 9)
    {
      sprintf(msg, "Sorry but you can only nest up to 10 doit files.\n");
      print_msg(msg);
      ui_globals.action=MAIN;
      top();
      return;
    }

  /* need to set action to MAIN here so that the commands in doit file can use action */
  ui_globals.action = MAIN;


  strcpy(pathname[doit_f_open+1], default_path);  	/*set up file path name*/
  strcat(pathname[doit_f_open+1], arg);
  /*strcpy(data_file, arg);
  strcat(pathname[doit_f_open+1], data_file);*/


/* open file, do some initial checking, then add to array of open doit files*/
  if ( (temp_com_file = fopen(pathname[doit_f_open+1], "r")) == NULL)
  {
        sprintf(msg, "Open failed- can't open data file: \"%s\"\n",  pathname[doit_f_open+1]);
        print_msg(msg);
        ui_globals.action = MAIN;
        top();
        return;
  }
  else
  {
	if (fscanf(temp_com_file, "%s", cmd) != 1)
	{
	      sprintf(msg,"Cannot read file.\n");
	      print_msg(msg);
	      fclose(temp_com_file);
	      ui_globals.action = MAIN;
	      top();
	      return;
	}
	if (strncmp(cmd, "begin", 5) != 0)
	{
	      sprintf(msg, "doit file must begin with the string: begin\n");
	      print_msg(msg);
	      fclose(temp_com_file);
	      ui_globals.action = MAIN;
	      top();
	      return;
	}

/* loop thru the file until EOF; read the file line by line, put the string (whole line) in cmd
	    pass cmd to command_handler() */

	doit_f_open++;				/*add to list of open doit files*/
	com_file[doit_f_open] = temp_com_file;  
	
/*clear first line (should be "begin" at this point, report errors*/

	i=1;	/*use to count lines*/
	if( fgets(cmd, 256, com_file[doit_f_open]) == NULL)
	{
	      sprintf(msg, "Error from doit file \"%s\", unexpected EOF on line %d, see cmds1.c \n",  pathname[doit_f_open+1], i);
              print_msg(msg);
	      fclose(com_file[doit_f_open]);
	      if (--doit_f_open < 0)
		doit_f_open = 0;
              ui_globals.action = MAIN;
              top();
              return;
	}

/* the global_error flag is set in messages.c when the error functions are called*/
	while( fgets(cmd, 256, com_file[doit_f_open]) != NULL && global_error == FALSE)
	{
/*read a line from doit file*/
		i++;				/*increment line counter*/
		if( cmd == NULL)
	  	{
	      		sprintf(msg, "Error from doit file \"%s\", unexpected EOF on line %d, see cmds1.c \n",  pathname[doit_f_open+1], i);
              		print_msg(msg);
	      		fclose(com_file[doit_f_open]);
	      		if (--doit_f_open < 0)
				doit_f_open = 0;
              		ui_globals.action = MAIN;
              		top();
              		return;
	  	}

/*make sure line returned by fgets was "whole" --i.e. contained a NEWLINE, otherwise */
/*	lines may be longer than 256 (too long)*/
		j=0; error=TRUE;
	        while(j < 256)			
		{
 		  if(cmd[j] == '\n')		/*find end of lines and replace with nulls*/
		  {
		  	cmd[j] = '\0';
			error=FALSE;
			break;	
		  }
		  j++;
		}
		if(error)
		{
	      		sprintf(msg, "Error from doit file \"%s\", line %d too long. Lines must be < 256 chars. But this should be easy to fix, see cmds1.c \n",  pathname[doit_f_open+1], i);
              		print_msg(msg);
	      		fclose(com_file[doit_f_open]);
	      		if (--doit_f_open < 0)
				doit_f_open = 0;
              		ui_globals.action = MAIN;
              		top();
              		return;
		}

		if (strncmp(cmd, "#", 1) != 0)
		{
			sprintf(msg, "Command: %s\n", cmd);
		  	print_msg(msg);
		}
		  
		if (strncmp(cmd, "end", 3) == 0)
		{
		      sprintf(msg, "Closing doit file %s.\n",pathname[doit_f_open]);
		      print_msg(msg);
		      fclose(com_file[doit_f_open]);
		      if (--doit_f_open < 0)
			doit_f_open = 0;
		      break;
		}
		  
		else if (strncmp(cmd, "#", 1) == 0)
		{
		      continue;
		}

		else 
		{
		      command_handler(cmd);
		}
	}
	cmd[0] = '\0';	/*empty cmd buffer*/
	ui_globals.action = MAIN;
	top();
   }
}

     

void set_path_proc(arg)  
     char arg[256];
{
  /* change: if path is inaccessible, set path to current path instead of default path */
  
  /*  printf("path: %s  def_path: %s\n", arg, default_path); */
  
  if (access(arg, 4) != 0)
    {
      sprintf(msg, "Inaccessible path: %s.\n", arg);
      print_msg(msg);
    }
  
  else
    {
      strcpy(default_path, arg);
      if (default_path[strlen(default_path)-1] != '/')
	strcat(default_path, "/");
    }
  sprintf(msg, "Default path is %s\n", default_path);
  print_msg(msg);
  
  top();
  ui_globals.action = MAIN;
}




void all_final_proc(cmd)
     char cmd[256];
{
  char dummy1[256], dummy2[256];
  int i, j;
  
  if (sscanf(cmd, "%s %s %d %d", dummy1, dummy2, &i, &j) != 4)
    {
      print_msg("Error from all_final_proc: Input not recognized.\n");
      ui_globals.action = MAIN;
      top();
      return;
    }
  
  if ((strcmp(dummy2, "yes") != 0) || (strcmp(dummy2, "y") != 0))
    {
      ui_globals.action = MAIN;
      top();
      return;
    }
    
  if (j==0 || j>=MAX_COL || i==0 || i>max_row) 
    {
      sprintf(msg, "Illegal allocation: 0 < NROW < %d < NCOL < %d\n", max_row, MAX_COL);
      print_msg(msg);
      ui_globals.action = MAIN;
      top();
      return;
    }
  
  allocate(i, j);
  sprintf(msg, "ALLOCATE: DONE\n");
  print_msg(msg);

  ui_globals.action = MAIN;
  top();
  
}

