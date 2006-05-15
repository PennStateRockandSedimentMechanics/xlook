#include <math.h>

#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/xv_xrect.h>
#include <xview/panel.h>

#include <cmds1.h>
#include <config.h>
#include <drawwin.h>
#include <global.h>
#include <messages.h>

extern int active_window;
extern int old_active_window;
extern int total_windows;
extern int action;
extern char msg[MSG_LENGTH];

extern char plot_cmd[256];

extern int tickfontheight, tickfontwidth;
extern int titlefontheight, titlefontwidth;
extern GC gctitle, gctick;
extern GC gcbg;

extern Frame main_frame;



/* cjm, 3/19/96: I think "label_type" refers to the way ticks and tick spacing is done*/
void label_type0()
{
  canvasinfo *can_info;
  int plot;
  plotarray *data;
  int tickx, ticky;
  double ten = 10.000;
  char string[256];
  int stringlen;  
  float xmax, xmin, ymax, ymin;
  int start_xaxis, start_yaxis, end_xaxis, end_yaxis;
  int start_x, start_y, end_x, end_y;
  Display *dpy;
  Window win;
  int width, height;
  

  can_info = wininfo.canvases[active_window];
  plot = can_info->active_plot;
  data = can_info->plots[plot];
  win = can_info->win;
  dpy = (Display *)xv_get(main_frame, XV_DISPLAY);

  width = (int)xv_get(can_info->canvas, XV_WIDTH);
  height = (int)xv_get(can_info->canvas, XV_HEIGHT);

  start_xaxis = can_info->start_xaxis;
  start_yaxis = can_info->start_yaxis;
  end_xaxis = can_info->end_xaxis;
  end_yaxis = can_info->end_yaxis;

  xmin = data->xmin;
  ymin = data->ymin;
  xmax = data->xmax;
  ymax = data->ymax;

  start_x = can_info->start_x;
  end_x = can_info->end_x;
  start_y = can_info->start_y;
  end_y = can_info->end_y;

  if (can_info->total_plots != 0)
    {
      XFillRectangle(dpy, win, gcbg, 0, 0, width, end_yaxis-1);
      XFillRectangle(dpy, win, gcbg, 
		     0, end_y-1, 
		     start_x-1, start_y-end_y+2);
      XFillRectangle(dpy, win, gcbg, 
		     end_x+1, end_y-1, 
		     width-end_x, start_y-end_y+2);
      XFillRectangle(dpy, win, gcbg, 0, start_y+1, width, height-start_y);
    }
  
   /* x-axis  (bottom) */
  XDrawLine(dpy, win, gctick, 
	    start_xaxis, start_yaxis, 
	    end_xaxis, start_yaxis);
  /* left y-axis  */
  XDrawLine(dpy, win, gctick,
	    start_xaxis, start_yaxis,
	    start_xaxis, end_yaxis); 
  /* right y-axis  */
  XDrawLine(dpy, win, gctick, 
	    end_xaxis, start_yaxis,
	    end_xaxis, end_yaxis);

  
  /* sizes for the tick marks */
  ticky=(int)width*0.02/2;
  tickx=(int)height*0.02/2;

  /* tick and label for ymin */
  XDrawLine(dpy, win, gctick, 
	    start_xaxis, start_y,
	    start_xaxis+ticky, start_y);
  XDrawLine(dpy, win, gctick, 
	    end_xaxis, start_y,
	    end_xaxis-ticky, start_y);
  sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
	  pow(ten, (double)ymin) : ymin);
  stringlen = strlen(string); 
  XDrawString(dpy, win, gctick,
	      start_xaxis - tickfontwidth*(strlen(string)+1),
	      start_y + tickfontheight/2,
	      string, stringlen);

  /* tick and label for ymax */ 
  XDrawLine(dpy, win, gctick, 
	    start_xaxis, end_y,
	    start_xaxis+ticky, end_y);
  XDrawLine(dpy, win, gctick, 
	    end_xaxis, end_y,
	    end_xaxis-ticky, end_y);
  sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
	  pow(ten, (double)ymax) : ymax);
  stringlen = strlen(string); 
  XDrawString(dpy, win, gctick,
	      start_xaxis - tickfontwidth*(strlen(string)+1),
	      end_y + tickfontheight/2,
	      string, stringlen);

  /* tick and label for y mid */ 
  XDrawLine(dpy, win, gctick, 
	    start_xaxis, end_y + (start_y - end_y)/2,
	    start_xaxis+ticky, end_y + (start_y - end_y)/2);
  XDrawLine(dpy, win, gctick, 
	    end_xaxis, end_y + (start_y - end_y)/2,
	    end_xaxis-ticky, end_y + (start_y - end_y)/2);
  sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
	  pow(ten, (double)(ymax-ymin)/2) : (ymax+ymin)/2);
  stringlen = strlen(string); 
  XDrawString(dpy, win, gctick,
	      start_xaxis - tickfontwidth*(strlen(string)+1),
	      end_y + (start_y - end_y)/2 + tickfontheight/2,
	      string, stringlen);

 /* min x axis */
 XDrawLine(dpy, win, gctick, 
	    start_x, start_yaxis,
	    start_x, start_yaxis - ticky);
  sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
	  pow(ten, (double)xmin) : xmin);
  stringlen = strlen(string);
  XDrawString(dpy, win, gctick,
	      start_x - tickfontwidth*stringlen/2,
	      start_yaxis + tickfontheight + 2,
	      string, stringlen);

  /* max x axis */
 XDrawLine(dpy, win, gctick, 
	    end_x, start_yaxis,
	    end_x, start_yaxis - ticky);
  sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
	  pow(ten, (double)xmax) : xmax);
  stringlen = strlen(string);
  XDrawString(dpy, win, gctick,
	      end_x - tickfontwidth*stringlen/2,
	      start_yaxis + tickfontheight + 2,
	      string, stringlen);

  /* mid x axis */
  XDrawLine(dpy, win, gctick, 
	    start_x + (end_x-start_x)/2, start_yaxis,
	    start_x + (end_x-start_x)/2, start_yaxis - ticky);
  sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
	  pow(ten, (double)(xmax-xmin)/2) : (xmax+xmin)/2);
  stringlen = strlen(string);
  XDrawString(dpy, win, gctick,
	      start_x + (end_x-start_x)/2 - tickfontwidth*stringlen/2,
	      start_yaxis + tickfontheight + 2,
	      string, stringlen);

  sprintf(string, "%d.   %s  vs  %s", 
	  plot+1,
	  head.ch[data->col_x].name, 
	  head.ch[data->col_y].name);

  XDrawString(dpy, win, gctitle, 
	      start_xaxis, titlefontheight+2, 
	      string, strlen(string));

  sprintf(string,"%s",head.title);	/*add title, right justified*/

  XDrawString(dpy, win, gctitle,
              end_xaxis-titlefontwidth*strlen(string), titlefontheight+2,
              string, strlen(string));
}


void label_type1()
{
  Display *dpy;
  Window win;
  canvasinfo *can_info;
  int plot;
  plotarray *data;
  double ten = 10.000;
  char string[256];
  float xmax, xmin, ymax, ymin;
  float scale_x, scale_y;
  int start_xaxis, start_yaxis, end_xaxis, end_yaxis;
  int start_x, start_y, end_x, end_y;
  int width, height;
  int a;
  float big_tickx, big_ticky;
  int tickx, ticky;
  double diffx, diffy;
  double decx, decy;
  float stop_xmin, stop_xmax, stop_ymin, stop_ymax;
  int where_x, where_y;
  int stringlen;
  
  
  can_info = wininfo.canvases[active_window];
  plot = can_info->active_plot;
  data = can_info->plots[plot];
  win = can_info->win;
  dpy = (Display *)xv_get(main_frame, XV_DISPLAY);

  width = (int)xv_get(can_info->canvas, XV_WIDTH);
  height = (int)xv_get(can_info->canvas, XV_HEIGHT);

  start_xaxis = can_info->start_xaxis;
  start_yaxis = can_info->start_yaxis;
  end_xaxis = can_info->end_xaxis;
  end_yaxis = can_info->end_yaxis;

  xmin = data->xmin;
  ymin = data->ymin;
  xmax = data->xmax;
  ymax = data->ymax;

  start_x = can_info->start_x;
  end_x = can_info->end_x;
  start_y = can_info->start_y;
  end_y = can_info->end_y;

  scale_x = data->scale_x;
  scale_y = data->scale_y;
  
  if (can_info->total_plots != 0)
    {
      /* clear title */
      XFillRectangle(dpy, win, gcbg, 0, 0, width, end_yaxis-1);
      /* clear left y axis */
      XFillRectangle(dpy, win, gcbg, 
		     0, end_y-1, 
		     start_x-1, start_y-end_y+2);
      /* clear right y axis */
      XFillRectangle(dpy, win, gcbg, 
		     end_x+1, end_y-1, 
		     width-end_x, start_y-end_y+2);
      /* clear x axis */
      XFillRectangle(dpy, win, gcbg, 0, start_y+1, width, height-start_y);
    }
  
   /* x-axis  (bottom) */
  XDrawLine(dpy, win, gctick, 
	    start_xaxis, start_yaxis, 
	    end_xaxis, start_yaxis);
  /* left y-axis  */
  XDrawLine(dpy, win, gctick,
	    start_xaxis, start_yaxis,
	    start_xaxis, end_yaxis); 
  /* right y-axis  */
  XDrawLine(dpy, win, gctick, 
	    end_xaxis, start_yaxis,
	    end_xaxis, end_yaxis);

  
  /* sizes for the tick marks */
  ticky=(int)width*0.02;
  tickx=(int)height*0.02;

  diffx = xmax - xmin;
  diffy = ymax - ymin;
  
  stop_xmax = xmax + diffx/20;
  stop_ymax = ymax + diffy/20;

  stop_xmin = xmin - diffx/20;
  stop_ymin = ymin - diffy/20;

  decx=(double)floor(log10(diffx));
  decy=(double)floor(log10(diffy));
  
  big_tickx= ceil(xmin*pow(ten,-decx)) * pow(ten, decx);
  big_ticky= ceil(ymin*pow(ten,-decy)) * pow(ten, decy);
     
  /* y axis big tick marks */
  for (a=0;big_ticky<=stop_ymax;a++)
    {
      where_y = start_y-(int)((big_ticky-ymin)*scale_y);

      XDrawLine(dpy, win, gctick, 
		start_xaxis, where_y,
  		start_xaxis+ticky, where_y);
      
      XDrawLine(dpy, win, gctick, 
		end_xaxis, where_y,
  		end_xaxis-ticky, where_y);
            
      sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
	      pow(ten, (double)big_ticky) : big_ticky);
      stringlen = strlen(string);
      
      XDrawString(dpy, win, gctick,
		  start_xaxis - tickfontwidth*(strlen(string)+1),
		  start_y-(int)((big_ticky-ymin)*scale_y)+tickfontheight/2,
		  string, stringlen);
      		  
      big_ticky += pow(ten, decy);
      
    }
  
  /* y axis small tick marks */
  if (a<2)
    {
      big_ticky -= 1.5 * pow(ten, decy);
      if (big_ticky < stop_ymin) big_ticky += pow(ten, decy);

      while (big_ticky < stop_ymax)
	{
	  where_y = start_y-(int)((big_ticky-ymin)*scale_y);
      
	  XDrawLine(dpy, win, gctick, 
		    start_xaxis, where_y,
		    start_xaxis+ticky, where_y);
	  
	  XDrawLine(dpy, win, gctick, 
		    end_xaxis, where_y,
		    end_xaxis-ticky, where_y);
	  
	  sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		  pow(ten, (double)big_ticky) : big_ticky);
	  stringlen = strlen(string);

	  XDrawString(dpy, win, gctick,
		      start_xaxis - tickfontwidth*(stringlen+1),
		      start_y-(int)((big_ticky-ymin)*scale_y)+tickfontheight/2,
		      string, stringlen);
	  
	  big_ticky += pow(ten, decy);
	}
    }



  /*  x axis big tick marks */
  for (a=0;big_tickx<=stop_xmax;a++)
    {
      where_x= start_x + (int)((big_tickx-xmin)*scale_x);
      
      XDrawLine(dpy, win, gctick, 
		where_x, start_yaxis,
		where_x, start_yaxis - tickx);
            
      sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
	      pow(ten, (double)big_tickx) : big_tickx);
      stringlen = strlen(string);

      XDrawString(dpy, win, gctick,
		  where_x - tickfontwidth*stringlen/2,
		  start_yaxis + tickfontheight + 2,
		  string, stringlen);
      		    
      big_tickx += pow(ten, decx);
    }
  
  /* x axis small tick marks */
  if (a<2)
    {
      big_tickx -= 1.5 * pow(ten, decx);
      
      if (big_tickx < stop_xmin) big_tickx += pow(ten, decx);

      while (big_tickx < stop_xmax)
	{
	  where_x=start_x + (int)((big_tickx-xmin)*scale_x);
      
	  XDrawLine(dpy, win, gctick, 
		    where_x, start_yaxis,
		    where_x, start_yaxis - tickx);
	 
	  sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		  pow(ten, (double)big_tickx) : big_tickx);
	  stringlen = strlen(string);
         
	  XDrawString(dpy, win, gctick,
		      where_x - tickfontwidth*stringlen/2,
		      start_yaxis + tickfontheight + 2,
		      string, stringlen);
      		  
	  big_tickx += pow(ten, decx);
	}
    }

  sprintf(string, "%d.   %s  vs  %s", 
	  plot+1,
	  head.ch[data->col_x].name, 
	  head.ch[data->col_y].name);

  XDrawString(dpy, win, gctitle, 
	      start_xaxis, titlefontheight+2, 
	      string, strlen(string));

  sprintf(string,"%s",head.title);	/*add title, right justified*/

  XDrawString(dpy, win, gctitle,
              end_xaxis-titlefontwidth*strlen(string), titlefontheight+2,
              string, strlen(string));
}

 

void redraw_proc(canvas, paint_window, dpy, win, area)
     Canvas canvas;
     Xv_Window paint_window;
     Display *dpy;
     Window win;
     Xv_xrectlist *area;
{
  int i;
  float x1, x2, y1, y2;
  canvasinfo *can_info;
  int plot, plots;
  plotarray *data;
  float xmax, xmin, ymax, ymin;
  int start_x, start_y, end_x, end_y;
  float scale_x, scale_y;
  char rows_string[64];    

  if(active_window == -1 || total_windows == -1)
	return;

  can_info = wininfo.canvases[active_window];
  plot = can_info->active_plot;
  plots = can_info->total_plots;

  if(plots == 0) 			/*window's been cleared, no plots*/
  {
  	sprintf(rows_string, "PLOT ROWS:  ");
  	xv_set(xv_get(canvas, XV_KEY_DATA, CAN_PLOT_ROWS), PANEL_LABEL_STRING, rows_string, NULL);
  	return;
  }
  display_active_plot(plot+1);
    
  data = can_info->plots[plot];

                          /*display row numbers for active plot*/
  sprintf(rows_string, "PLOT ROWS: %d to %d", data->begin, data->end);
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_PLOT_ROWS), PANEL_LABEL_STRING, rows_string, NULL);
  
  xmin = data->xmin;
  ymin = data->ymin;
  xmax = data->xmax;
  ymax = data->ymax;

  start_x = can_info->start_x;
  end_x = can_info->end_x;
  start_y = can_info->start_y;
  end_y = can_info->end_y;
  
  scale_x = (float)(end_x - start_x)/(xmax - xmin);
  scale_y = (float)(start_y - end_y)/(ymax - ymin);
  
  data->scale_x = scale_x;
  data->scale_y = scale_y;

  /* print the labels */
  if (data->label_type)
    label_type1();
  else
    label_type0();
  

  /* plot each point  */
  if (can_info->point_plot == 1)
    {
      /* plot the individual points */
    
      for (i=0; i < data->nrows_x -1; i++)
	{
	  x1 = data->xarray[i] - xmin;
	  y1 = data->yarray[i] - ymin;
	  	  
	  XDrawPoint(dpy, win, gctitle, 
		     start_x + (int)(x1*scale_x),
		     start_y - (int)(y1*scale_y));
	}
    }
  else
    {
      /* connect the points */
      
      for (i=0; i < data->nrows_x -1; i++)
	{
	  x1 = data->xarray[i] - xmin;
	  x2 = data->xarray[i+1] - xmin;
	  y1 = data->yarray[i] - ymin;
	  y2 = data->yarray[i+1] - ymin;
	  
	  XDrawLine(dpy, win, gctitle, 
		    start_x + (int)(x1*scale_x),
		    start_y - (int)(y1*scale_y),
		    start_x + (int)(x2*scale_x),
		    start_y - (int)(y2*scale_y));
	}
    }
				/*set footer info. This should be redundant since it only gets set or changed from the panel buttons  --do anyway just to be safe...*/
   switch(can_info->plots[plot]->mouse)
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
}



void clear_canvas_proc(canvas)
     Canvas canvas;
{
  Display *dpy;
  Window win;
    
  dpy = (Display *)xv_get(canvas, XV_DISPLAY);
  win = (Window)xv_get(canvas_paint_window(canvas), XV_XID);
  XClearWindow(dpy, win);
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_PLOT_ROWS), PANEL_LABEL_STRING, "PLOT ROWS:  ", NULL);
}




void redraw_all_proc(canvas, xvwindow, dpy, win, area)
     Canvas canvas;
     Xv_Window xvwindow;
     Display *dpy;
     Window win;
     Xv_xrectlist *area;
{
  int can_num, i;
  canvasinfo *can_info;
  int active_plot;
  
  can_num =  xv_get(canvas, XV_KEY_DATA, CAN_NUM);
  set_active_window(can_num);

  can_info = wininfo.canvases[active_window];    
  active_plot = can_info->active_plot;
 /* display_active_plot(active_plot+1); */
/*cjm 26/3/96: seems unnec. given that it's done in redraw_proc*/

  if (can_info->total_plots == 0) return;

  XClearWindow(dpy, win);

  for (i=0; i<MAX_PLOTS; i++)
    {
      if (can_info->alive_plots[i] == 1)
	{
	  can_info->active_plot = i;
	  redraw_proc(canvas, xvwindow, dpy, win, NULL); 
	}
    }
  can_info->active_plot = active_plot ;	/*reinstall active plot*/
}

  
  
