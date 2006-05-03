#include <config.h>
#include "global.h"
#include <string.h>
#include <sys/file.h>
#include <xview/frame.h>
#include <xview/canvas.h>
#include <X11/Xlib.h>
#include <xview/xv_xrect.h>
#include <xview/panel.h>
#include <xview/xview.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

char pathname[10][80];
char default_path[80];
char metapath[80];
char data_file[32];
char new_file[32];
char headline[32];

FILE *data, *fopen(), *new;
int reed();

int doit_des, doit_f_open, meta_fd, errno;
int plot_error;
int read_flag = 0; 
int qi_flag = 0;

extern char plot_cmd[256], trig_cmd[256];

extern int active_window;
extern int old_active_window;
extern int total_windows;
extern int action;
extern char msg[MSG_LENGTH];
extern update_params();

extern void redraw_proc();
extern Frame main_frame;
extern Panel_item cmd_panel_item;

new_win_proc()
{
  total_windows++;
  create_canvas();
  sprintf(msg, "Total windows = %d\n", total_windows+1);
  print_msg(msg);
}

qi_win_proc()
{

 if (read_flag != 0) {
    if (qi_flag == 1) 
      update_params();
    else {
      create_qi_canvas();
      qi_flag = 1;
    }
  } 
 else {
   sprintf(msg, "File has not been read yet. File must be read first.\n");
   print_msg(msg);
   set_left_footer("Type the data file to read");	  
   action = READ;
   set_cmd_prompt("Filename: ");
 }

}


set_active_window(win_num)
     int win_num;
{
  if (wininfo.windows[win_num] != 1)
    {
      sprintf(msg, "Window #%d does not exist.\n", win_num+1);
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  if (active_window != win_num)
    {
      if (win_num == old_active_window)
	{
	  old_active_window = active_window;
	  active_window = win_num;
	}
      else
	{
	  old_active_window = active_window;
	  active_window = win_num;
	}
    }

  display_active_window(active_window+1);
  display_active_plot(wininfo.canvases[active_window]->active_plot+1);  
  display_active_file(1);
}


set_active_plot(i)
     int i;
{
  canvasinfo *can_info;
  Canvas canvas;
  Xv_Window canvas_xv_window;

  can_info = wininfo.canvases[active_window];
  canvas = can_info->canvas;
  canvas_xv_window = canvas_paint_window(canvas);

  can_info->active_plot = i;

  redraw_proc(can_info->canvas, can_info->xvwin, xv_get(can_info->canvas, XV_DISPLAY),
                          can_info->win, NULL);

  if(i < 0)			/*no active plot, no plots*/
  {
				/*set a default footer*/
       xv_set(xv_get(canvas, XV_KEY_DATA, GRAF_FRAME), FRAME_LEFT_FOOTER, "Normal Mode: left & middle buttons pick row numbers, right button gives x-y position", NULL);
   	display_active_plot(can_info->active_plot);
  }
  else if (can_info->alive_plots[i] == 0)
  {
      sprintf(msg, "Plot does not exist, it can't be set active, error!\n");
      print_msg(msg);
      top();
      action = MAIN;
  }
  else
  {
   switch(can_info->plots[i]->mouse)
   {
    case 0:
      xv_set(xv_get(canvas, XV_KEY_DATA, GRAF_FRAME),
	FRAME_LEFT_FOOTER, "Normal Mode: left & middle buttons pick row numbers, right button gives x-y position", NULL);
      break;
    case 1:
      xv_set(xv_get(canvas, XV_KEY_DATA, GRAF_FRAME),
	FRAME_LEFT_FOOTER, "Draw Line Mode: left button picks 1st point, middle picks 2nd, right button quits mode", NULL);
      break;
    case 2:
      xv_set(xv_get(canvas, XV_KEY_DATA, GRAF_FRAME),
	FRAME_LEFT_FOOTER, "Vertical Line Mode: left & middle buttons draw vertical line, right button quits mode", NULL);
      break;
    case 3:
      xv_set(xv_get(canvas, XV_KEY_DATA, GRAF_FRAME),
	FRAME_LEFT_FOOTER, "Distance Mode: left button picks 1st point, middle picks 2nd, right button quits mode", NULL);
      break;
    case 4:
      xv_set(xv_get(canvas, XV_KEY_DATA, GRAF_FRAME),
	FRAME_LEFT_FOOTER, "Zoom Mode: left button picks 1st point, middle picks 2nd, right button commits zoom", NULL);
      break;
   }
   display_active_plot(can_info->active_plot+1);
  }
}



del_plot_proc(pn)
     int pn;
{
  canvasinfo *can_info;
  int i, j, hole_index[MAX_PLOTS];
  
  pn--;					/*internal number starts at zero, instead of 1*/
  
  can_info = wininfo.canvases[active_window];

  if (can_info->alive_plots[pn] == 0 || can_info->active_plot == -1 )
    {
      sprintf(msg, "Plot does not exist or there's no active plot (error!).\n");
      print_msg(msg);
      top();
      action = MAIN;
      return;
    }
  
  can_info->alive_plots[pn] = 0;	/*make it dead*/
  free(can_info->plots[pn]);		/*free space */

				/*if killed plot was active, or if active plot was the last one
				in the list, reset active plot*/
  if( (pn == can_info->active_plot || can_info->active_plot == can_info->total_plots -1)
	&& can_info->active_plot)
    can_info->active_plot--;
/*reshuffle the list, so that, say, removing plot #2 from a list of 4
results in #3 going to #2 and #4 going to #3. This will require repointing the 
plot data pointers etc.*/

  for(j=i=0;i<MAX_PLOTS;i++)	/*first set up an index for holes (actually non-holes)*/
  {
	hole_index[i]=0;		/*default*/
 	if (can_info->alive_plots[i])	/*if alive, numbers will be the same */
		hole_index[j++] = i; 
  }

  can_info->total_plots--;		/*now decrement this counter*/

  for(j=i=0;i<can_info->total_plots;i++)		/*shuffle list*/	
  {
		can_info->alive_plots[i] = 1; /*set it live*/
  		can_info->plots[i] = can_info->plots[hole_index[i]];  /*repoint pointer to data*/
  }

  for(i=can_info->total_plots;i< MAX_PLOTS;i++) /* all the rest are dead*/
	can_info->alive_plots[i] = 0;
  
  clear_canvas_proc(can_info->canvas);

  if(can_info->total_plots ==0)
        can_info->active_plot = -1;
  else
  {
  	j=can_info->active_plot;		/*save active plot*/
  	for (i=0; i<MAX_PLOTS; i++)		/*re-draw */
	{
		if (can_info->alive_plots[i] == 1)
		{
	      		can_info->active_plot = i;
	      		redraw_proc(can_info->canvas, can_info->xvwin, 
			  xv_get(can_info->canvas, XV_DISPLAY), can_info->win, NULL); 
	    	}
	}
	if(can_info->alive_plots[j] == 0)     /*active plot got axed, use default*/
        	can_info->active_plot = can_info->total_plots-1;
  	else
        	can_info->active_plot = j;
  }

  set_active_plot(can_info->active_plot);

  action = MAIN;
  top();
}

kill_win_proc(wn)
     int wn;
{
  int i;
  Frame graf_frame;
  canvasinfo *can_info;
  
  wn--;

  if (wn < 0 || wininfo.windows[wn] == 0)
    {
      sprintf(msg, "Window #%d does not exist.\n", wn+1);
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }

  sprintf(msg, "destroying window #%d\n", wn+1);
  print_msg(msg);  
  
  can_info = wininfo.canvases[wn];

  /* free all the plots in this window */
  for (i=0; i<MAX_PLOTS; i++)
    {
      if(can_info->alive_plots[i])
	free(can_info->plots[i]);
    }

  /* kill the xwindow */
  graf_frame = xv_get(can_info->canvas, XV_KEY_DATA, GRAF_FRAME);
  xv_destroy_safe(graf_frame);
    
  
  if (active_window == wn)
    {
      active_window = old_active_window;
      old_active_window = get_old_active_window(active_window);
    }
  
  if (old_active_window == wn)
    {
      old_active_window = get_old_active_window(old_active_window);
    }
  
  total_windows--;
  if (total_windows == -1)
    {
      active_window = -1;
      old_active_window = -1;
    }
  
  display_active_window(active_window+1);
  /* if no window is opened, active_plot = NONE */
  if (active_window != -1)
    display_active_plot(wininfo.canvases[active_window]->active_plot+1);
  else
    display_active_plot(-1);
  
  wininfo.windows[wn] = 0;
  
  sprintf(msg, "Total windows = %d\n", total_windows+1);
  print_msg(msg);  
    
  /* free the canvasinfo for this window */
  free(can_info);

  action = MAIN;
  top();
  
}


get_old_active_window(wn)
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
get_new_plot_num(alive_plots)
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


plotting_error(pn)
     int pn;
{
  canvasinfo *can_info;
  int i;
  
  can_info = wininfo.canvases[active_window];
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


do_plot(cmd)
     char cmd[256];
{
  char desire[256];
  int i, j;
  canvasinfo *can_info;
  plotarray *data;
  int start_x, start_y, end_x, end_y;
  int begin, end;
  int xaxis, yaxis;
  int oldplot;
  float xmax, xmin, ymax, ymin;
  int plot_num;
  float scale_x, scale_y;
  Canvas canvas;
  Xv_Window canvas_xv_window, xvwin;
  Display *dpy;
  Window win;
    
  plot_error = 0;
  
  nocom(cmd);
  
  if (total_windows == -1) new_win_proc();  

  can_info = wininfo.canvases[active_window];

  if( (strncmp(plot_cmd, "plotover", 8) == 0 || 
      (strncmp(plot_cmd, "plotsr", 6) == 0)  ||
      (strncmp(plot_cmd, "plotsame", 8) == 0))) 
  {
	if(can_info->total_plots == 0)
	{
		sprintf(msg, "No previous plot. You can't use \"plotover\" etc. since they plot at scale of previous plot.\n");
		print_msg(msg);
		top();
		action = MAIN;
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
      action = MAIN;
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
      action = MAIN;
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
	  action = MAIN;
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
	      action = MAIN;
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
	    action = MAIN;
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
	      action = MAIN;
	      return;
	    }

	  begin = can_info->plots[oldplot]->begin;
	  end = can_info->plots[oldplot]->end;
      }
      else if (strcmp(plot_cmd, "plotscale") == 0)
      {	
	  if (sscanf(cmd, "%s %d %d %d %d %f %f %f %f", desire, 
		     &xaxis, &yaxis, &begin, &end,
		     &xmax, &xmin,
		     &ymax, &ymin) != 9)
	    {
	      nea();
	      plotting_error(plot_num);
	      top();
	      action = MAIN;
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
	      action = MAIN;
	      return;
	    }
      }


      if (xaxis >= max_col || yaxis >= max_col)
      {
	  sprintf(msg, "Out of range. Col not allocated.\n");
	  print_msg(msg);
	  plotting_error(plot_num);
	  top();
	  action = MAIN;
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
	      action = MAIN;
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
	      action = MAIN;
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
	  /*	  printf("begin:%d end:%d max.xaxis:%.3f min.xaxis:%.3f max.yaxis:%.3f min.yaxis:%.3f\n", begin, end, xmax, xmin, ymax, ymin); */
	  sprintf(msg, "Limits problem: MAX <= MIN ???\n"); 
	  print_msg(msg);

	  plotting_error(plot_num);
	  plot_error = 1;
	  top();
	  action = MAIN;	  
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
  
  data->xarray = (float *)malloc(sizeof(float)*data->nrows_x);
  data->yarray = (float *)malloc(sizeof(float)*data->nrows_x);
    

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
     sprintf(msg, "Active window: %d    Canvas num: %d    Plot num: %d\n", active_window+1, can_info->canvas_num+1, can_info->active_plot+1);
     print_msg(msg);
     */

  canvas = can_info->canvas;
  canvas_xv_window = can_info->xvwin;
  win = can_info->win;

  /* draw the plot */
  redraw_proc(canvas, canvas_xv_window, (Display *)xv_get(canvas, XV_DISPLAY),
	       (Window)xv_get(canvas_xv_window, XV_XID), NULL);
    
  top();
  action = MAIN;
}




write_proc(arg)
     char arg[256];
{
  
  strcpy(new_file, arg);

  if ((new = fopen(new_file, "a")) == NULL)
    {
      sprintf(msg, "Can't open file: %s.\n", new_file);
      print_msg(msg);
      action =MAIN;
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
	      action =MAIN;
	      top();
	      return;
	    }
	  else
	    {
	      if ((new = fopen(new_file, "w")) == NULL)
		{
		  sprintf(msg, "Can't open file: %s.\n", new_file);
		  print_msg(msg);
		  action =MAIN;
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
	  sprintf(msg, "Active columns = %d\n", act_col);
	  print_msg(msg);
	  sprintf(msg, "Please compact data array to use columns 1 through head.nchan[%d]\n", head.nchan);
	  print_msg(msg);
	  sprintf(msg, "Reallocation is not necessary.\n");
	  print_msg(msg);
	  action =MAIN;
	  top();
	  return;	
	}
      
      rite(new);
      fclose(new);
      action =MAIN;
      top(); 
    }
}


read_proc(cmd)
     char cmd[256];
{
  char dummy[256];
  
  sscanf(cmd, "%s %s", dummy, data_file);

  if ((data = fopen(data_file, "r")) == NULL)
    {
      sprintf(msg, "Can't open data file: %s.\n", data_file);
      print_msg(msg);
      action = MAIN;
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
	  action = MAIN;
	  top();
	  return;
	}
      display_active_file(1);
      fclose(data);
      sprintf(msg, "Reading %s done.\n", data_file);
      print_msg(msg);
      ++read_flag; /* file successfully opened */
      action = MAIN;
      top();
    }
}
  

doit_proc(arg)
     char arg[256];
{
  /* reads a doit file and passes the commands to command_handler() directly */  
  char *fgets();
  char cmd[256], ch;
  int i, j, error;
  
  
  if ((doit_f_open+1) > 9)
    {
      sprintf(msg, "Sorry but you can only nest up to 10 doit files.\n");
      print_msg(msg);
      action=MAIN;
      top();
      return;
    }

  /* need to set action to MAIN here so that the commands in doit file can use action */
  action = MAIN;


  strcpy(pathname[doit_f_open+1], default_path);  	/*set up file path name*/
  strcat(pathname[doit_f_open+1], arg);
  /*strcpy(data_file, arg);
  strcat(pathname[doit_f_open+1], data_file);*/


/* open file, do some initial checking, then add to array of open doit files*/
  if ( (temp_com_file = fopen(pathname[doit_f_open+1], "r")) == NULL)
  {
        sprintf(msg, "Open failed- can't open data file: \"%s\"\n",  pathname[doit_f_open+1]);
        print_msg(msg);
        action = MAIN;
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
	      action = MAIN;
	      top();
	      return;
	}
	if (strncmp(cmd, "begin", 5) != 0)
	{
	      sprintf(msg, "doit file must begin with the string: begin\n");
	      print_msg(msg);
	      fclose(temp_com_file);
	      action = MAIN;
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
	      sprintf(msg, "Error from doit file \"%s\", unexpected EOF on line %d, see cmds1.c \n",  i, pathname[doit_f_open+1]);
              print_msg(msg);
	      fclose(com_file[doit_f_open]);
	      if (--doit_f_open < 0)
		doit_f_open = 0;
              action = MAIN;
              top();
              return;
	}

/* the global_error flag is set in messages.c when the error functions are called*/
	while( fgets(cmd, 256, com_file[doit_f_open]) != EOF && global_error == FALSE)
	{
/*read a line from doit file*/
		i++;				/*increment line counter*/
		if( cmd == NULL)
	  	{
	      		sprintf(msg, "Error from doit file \"%s\", unexpected EOF on line %d, see cmds1.c \n",  i, pathname[doit_f_open+1]);
              		print_msg(msg);
	      		fclose(com_file[doit_f_open]);
	      		if (--doit_f_open < 0)
				doit_f_open = 0;
              		action = MAIN;
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
	      		sprintf(msg, "Error from doit file \"%s\", line %d too long. Lines must be < 256 chars. But this should be easy to fix, see cmds1.c \n",  i, pathname[doit_f_open+1]);
              		print_msg(msg);
	      		fclose(com_file[doit_f_open]);
	      		if (--doit_f_open < 0)
				doit_f_open = 0;
              		action = MAIN;
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
	action = MAIN;
	top();
   }
}

     

set_path_proc(arg)  
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
  action = MAIN;
}




all_final_proc(cmd)
     char cmd[256];
{
  char dummy1[256], dummy2[256];
  int i, j;
  
  if (sscanf(cmd, "%s %s %d %d", dummy1, dummy2, &i, &j) != 4)
    {
      print_msg("Error from all_final_proc: Input not recognized.\n");
      action = MAIN;
      top();
      return;
    }
  
  if ((strcmp(dummy2, "yes") != 0) || (strcmp(dummy2, "y") != 0))
    {
      action = MAIN;
      top();
      return;
    }
    
  if (j==0 || j>=MAX_COL || i==0 || i>max_row) 
    {
      sprintf(msg, "Illegal allocation: 0 < NROW < %d < NCOL < %d\n", max_row, MAX_COL);
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  allocate(i, j);
  sprintf(msg, "ALLOCATE: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
  
}

