#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <math.h>

#include "global.h"

extern char msg[MSG_LENGTH];
extern int active_window;
extern GC gctick, gctitle;
extern char plot_cmd[256];
extern void zoom_plot_proc();
int CLEAR_FLAG = 0;
  

void clr_ap(void);
void draw_xhair(float, float);
void draw_crosshair(int, int);


/******************************** point plot ******************************/
void point_plot_proc(item, event)
     Panel_item item;
     Event *event;
{
  int can_num = xv_get(item, PANEL_CLIENT_DATA);
  canvasinfo *can_info;
  char pp;
  int ap;
  
  set_active_window(can_num);
  can_info = wininfo.canvases[active_window];
  ap = can_info->active_plot;

  pp = 1 - can_info->point_plot;

  can_info->point_plot = pp;
  
  if (pp == 1) xv_set(item, PANEL_LABEL_STRING, "Plot Line", NULL);
  else xv_set(item, PANEL_LABEL_STRING, "Plot Points", NULL);

  /* redraw all the plots */
/*  redraw_all_proc(can_info->canvas, can_info->xvwin,
	      (Display *)xv_get(can_info->canvas, XV_DISPLAY),
	      (Window)xv_get(can_info->xvwin, XV_XID), NULL);
*/

}


/******************************** line plot ******************************/

void set_line_plot()
{
  canvasinfo *can_info;

  can_info = wininfo.canvases[active_window];

  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }
  can_info->plots[can_info->active_plot]->mouse = 1;
  can_info->plots[can_info->active_plot]->p1 = 0;
  can_info->plots[can_info->active_plot]->p2 = 0;
  can_info->plots[can_info->active_plot]->x1 = 0;
  can_info->plots[can_info->active_plot]->x2 = 0;
  can_info->plots[can_info->active_plot]->y1 = 0;
  can_info->plots[can_info->active_plot]->y2 = 0;
  can_info->plots[can_info->active_plot]->zp1 = 0;
  can_info->plots[can_info->active_plot]->zp2 = 0;

  xv_set(xv_get(can_info->canvas, XV_KEY_DATA, GRAF_FRAME), 
	 FRAME_LEFT_FOOTER, "Draw Line Mode: left button picks first point, middle picks 2nd, right button quits", NULL);
}

void line_plot_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  int can_num = xv_get(menu, MENU_CLIENT_DATA);
  canvasinfo *can_info;
  int ap;
  
  set_active_window(can_num);
  can_info = wininfo.canvases[active_window];
  ap = can_info->active_plot;

  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  set_line_plot(); 
}

void do_line_plot()
{
  canvasinfo *can_info;
  int ap;
  plotarray *data;
  float slope, intercept, x1, x2, y1, y2;
  char xstring[20], ystring[20], rowstring[20];
  Canvas canvas;
  
  can_info = wininfo.canvases[active_window];

  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  ap = can_info->active_plot;
  data = can_info->plots[ap];
  
  canvas = can_info->canvas;
  
  XDrawLine((Display *)xv_get(can_info->canvas, XV_DISPLAY), can_info->win, gctick, 
	    data->x1, data->y1,
	    data->x2, data->y2);

  x1 = (data->x1 - can_info->start_x)/data->scale_x + data->xmin;
  x2 = (data->x2 - can_info->start_x)/data->scale_x + data->xmin;
  y1 = (can_info->start_y - data->y1)/data->scale_y + data->ymin;
  y2 = (can_info->start_y - data->y2)/data->scale_y + data->ymin;

  slope = (y2 - y1)/(x2 - x1);
  intercept = y1 - slope*x1;

  sprintf(msg, "line plot: x1,y1:(%f, %f), x2,y2:(%f, %f)\nslope: %g, int.: %g\n", x1, y1, x2, y2, slope, intercept);
  print_msg(msg);
  
  rowstring[0] = '\0'; 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_ROW), PANEL_LABEL_STRING, rowstring, NULL);	      
  sprintf(xstring, "Slope: %.5g", slope); 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_X), PANEL_LABEL_STRING, xstring, NULL);	      
  sprintf(ystring, "Y intercept: %.5g", intercept); 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_Y), PANEL_LABEL_STRING, ystring, NULL);	       
}



/************************** mouse mu **********************************/
void set_mouse_mu_proc()
{
   canvasinfo *can_info;

  can_info = wininfo.canvases[active_window];

  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  can_info->plots[can_info->active_plot]->mouse = 2;
  can_info->plots[can_info->active_plot]->p1 = 0;
  can_info->plots[can_info->active_plot]->p2 = 0;
  can_info->plots[can_info->active_plot]->x1 = 0;
  can_info->plots[can_info->active_plot]->x2 = 0;
  can_info->plots[can_info->active_plot]->y1 = 0;
  can_info->plots[can_info->active_plot]->y2 = 0;
  can_info->plots[can_info->active_plot]->zp1 = 0;
  can_info->plots[can_info->active_plot]->zp2 = 0;

  xv_set(xv_get(can_info->canvas, XV_KEY_DATA, GRAF_FRAME), 
	 FRAME_LEFT_FOOTER, "Vertical Line Mode: left & middle buttons draw vertical line, right button quits mode", NULL);
}

void mouse_mu_proc(menu, item)		/*draw vertical line on plot at chosen position*/
     Menu menu;
     Menu_item item;
{
  int can_num = xv_get(menu, MENU_CLIENT_DATA);
  canvasinfo *can_info;
  int ap;
  
  set_active_window(can_num);
  can_info = wininfo.canvases[active_window];
  ap = can_info->active_plot;

  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  set_mouse_mu_proc(); 
}

void do_mouse_mu()
{ 
  canvasinfo *can_info;
  int ap;
  plotarray *data;
  
  can_info = wininfo.canvases[active_window];
  ap = can_info->active_plot;

  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  data = can_info->plots[ap];
    
  XDrawLine((Display *)xv_get(can_info->canvas, XV_DISPLAY), can_info->win, gctick, 
	    data->x1, can_info->start_y,
	    data->x1, can_info->end_y);
}



/*******************************  dist *********************************/
void set_dist_proc()
{
  canvasinfo *can_info;

  can_info = wininfo.canvases[active_window];
 
  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }
  can_info->plots[can_info->active_plot]->mouse = 3; 
  can_info->plots[can_info->active_plot]->p1 = 0;
  can_info->plots[can_info->active_plot]->p2 = 0;
  can_info->plots[can_info->active_plot]->x1 = 0;
  can_info->plots[can_info->active_plot]->x2 = 0;
  can_info->plots[can_info->active_plot]->y1 = 0;
  can_info->plots[can_info->active_plot]->y2 = 0;
  can_info->plots[can_info->active_plot]->zp1 = 0;
  can_info->plots[can_info->active_plot]->zp2 = 0;

  xv_set(xv_get(can_info->canvas, XV_KEY_DATA, GRAF_FRAME), 
	 FRAME_LEFT_FOOTER, "Distance Mode: left button picks 1st point, middle picks 2nd, right button quits mode", NULL);
}

void dist_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  int can_num, ap;
  canvasinfo *can_info;
  
  can_num = xv_get(menu, MENU_CLIENT_DATA);
  set_active_window(can_num);
  can_info = wininfo.canvases[active_window];
  ap = can_info->active_plot;

  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  set_dist_proc(); 
}

void do_dist()
{
  canvasinfo *can_info;
  int ap;
  plotarray *data;
  float dx, dy, da;
  char xstring[20], ystring[20], vstring[20];
  Canvas canvas;
  float x1, x2, y1, y2;
  
  can_info = wininfo.canvases[active_window];
  ap = can_info->active_plot;

  if(can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  data = can_info->plots[ap];

  x1 = (data->x1 - can_info->start_x)/data->scale_x + data->xmin;
  x2 = (data->x2 - can_info->start_x)/data->scale_x + data->xmin;
  y1 = (can_info->start_y - data->y1)/data->scale_y + data->ymin;
  y2 = (can_info->start_y - data->y2)/data->scale_y + data->ymin;

  dx = fabs(x1 - x2);
  dy = fabs(y1 - y2);
  da = sqrt((double)((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)));
  
  canvas = can_info->canvas;
  
  sprintf(xstring, "dX: %.5g", dx); 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_X), PANEL_LABEL_STRING, xstring, NULL);	      
  sprintf(ystring, "dY: %.5g", dy); 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_Y), PANEL_LABEL_STRING, ystring, NULL);	      
  sprintf(vstring, "dV: %.5g", da); 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_ROW), PANEL_LABEL_STRING, vstring, NULL);	      
}



/******************************** zoom ********************************/

void set_zoom()
{
  canvasinfo *can_info;

  can_info = wininfo.canvases[active_window];
  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }
  can_info->plots[can_info->active_plot]->mouse = 4;
  can_info->plots[can_info->active_plot]->p1 = 0;
  can_info->plots[can_info->active_plot]->p2 = 0;
  can_info->plots[can_info->active_plot]->x1 = 0;
  can_info->plots[can_info->active_plot]->x2 = 0;
  can_info->plots[can_info->active_plot]->y1 = 0;
  can_info->plots[can_info->active_plot]->y2 = 0;
  can_info->plots[can_info->active_plot]->zp1 = 0;
  can_info->plots[can_info->active_plot]->zp2 = 0;

  xv_set(xv_get(can_info->canvas, XV_KEY_DATA, GRAF_FRAME), 
	 FRAME_LEFT_FOOTER, "Zoom Mode: left button picks 1st point, middle picks 2nd, right button commits zoom", NULL);
}


void zoom_plot_proc(menu,item)
     Menu menu; 
     Menu_item item;
{
  int ap;
  canvasinfo *can_info;
  
  set_active_window(xv_get(menu, MENU_CLIENT_DATA));
  can_info = wininfo.canvases[active_window];
  ap = can_info->active_plot;

  if(can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  set_zoom(); 
}


void zoom_clr_all_plots_proc(menu, item)
     Menu menu;
     Menu_item item;
{ 
  CLEAR_FLAG = 1;
  zoom_plot_proc(menu,item);
}


void zoom_get_pt(xloc, yloc, p)
     int xloc, yloc, p;
{
  float xval, yval;
  int nrows, row_num;
  canvasinfo *can_info;
  plotarray *data;

  can_info = wininfo.canvases[active_window];
  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }
  data = can_info->plots[can_info->active_plot];
  
  xval = (xloc - can_info->start_x)/data->scale_x + data->xmin;
  yval = (can_info->start_y - yloc)/data->scale_y + data->ymin;
  
  nrows = data->nrows_x;
  row_num = get_row_number(nrows, xval, yval);
  if (row_num <= nrows && row_num != -1)
    {
      xval = data->xarray[row_num]; 
      yval = data->yarray[row_num]; 
      draw_xhair(xval, yval);

      /* row_num in zp1 and zp2 is relative to this plot's begin index */      
      if (p == 1)
	data->zp1 = row_num;
      else
	data->zp2 = row_num;
    }
    /* else, do nothing */
}


void zoom()
{
  canvasinfo *can_info;
  plotarray *data;
  int begin, end;

  can_info = wininfo.canvases[active_window];
  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }
  data = can_info->plots[can_info->active_plot];
  
				/*set some default values, force row order*/
  if(data->zp1 < 0)
	data->zp1 = 0;
  if(data->zp2 <= 0)
	data->zp2 = data->end - data->begin;
  if(data->zp2 < data->zp1)
  {
      begin = data->zp2 + data->begin;
      end = data->zp1 + data->begin;
  }
  else
  {
      begin = data->zp1 + data->begin;
      end = data->zp2 + data->begin;
  }
sprintf(msg, "data-begin is %d, data-end is %d, data-zp1 is %d, data-zp2 is %d\n", data->begin, data->end, data->zp1, data->zp2);
print_msg(msg);

  if (CLEAR_FLAG == 1)
    {
     clr_ap();
     CLEAR_FLAG =0;
    }
  strcpy(plot_cmd, "plotauto");
  sprintf(msg, "plotauto %d %d %d %d", data->col_x, data->col_y, begin, end);
  do_plot(msg);

}

   
 
void print_xy(xloc, yloc)
     int xloc, yloc;
{
  canvasinfo *can_info;
  plotarray *data;
  char xstring[20], ystring[20], rowstring[20];
  Canvas canvas;
  float xval, yval;
  
  can_info = wininfo.canvases[active_window];

  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }

  data = can_info->plots[can_info->active_plot];
  canvas = can_info->canvas;
  
  draw_crosshair(xloc, yloc);

  xval = (xloc - can_info->start_x)/data->scale_x + data->xmin;
  yval = (can_info->start_y - yloc)/data->scale_y + data->ymin;
  
  rowstring[0] = '\0';
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_ROW), 
	 PANEL_LABEL_STRING, rowstring, NULL);
  
  /*  print the x and y coord on the panels */
  sprintf(xstring, "X: %.5g", xval); 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_X), PANEL_LABEL_STRING, xstring, NULL);	      
  sprintf(ystring, "Y: %.5g", yval);
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_Y), PANEL_LABEL_STRING, ystring, NULL);
  /*  print the x and y coord on the msg window */	      
  strcat(xstring, "  ");
  strcat(xstring, ystring);
  XDrawString((Display *)xv_get(canvas, XV_DISPLAY) , can_info->win, gctick, xloc+10, yloc, xstring, strlen(xstring));
  strcpy(msg, xstring);
  strcat(msg, "\n");
  print_msg(msg);
}


void print_xyrow(xloc, yloc, draw_string)
     int xloc, yloc, draw_string;
{
  canvasinfo *can_info;
  plotarray *data;
  char xstring[20], ystring[20], rowstring[20];
  char xyrowstring[60];
  Canvas canvas;
  float xval, yval;
  int nrows, row_num;
  
  can_info = wininfo.canvases[active_window];
  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }
  data = can_info->plots[can_info->active_plot];
  canvas = can_info->canvas;

  /* get the x and y (data) values */ 
  xval = (xloc - can_info->start_x)/data->scale_x + data->xmin;
  yval = (can_info->start_y - yloc)/data->scale_y + data->ymin;
  
  nrows = data->nrows_x;
  row_num = get_row_number(nrows, xval, yval);
  if (row_num <= nrows && row_num != -1)
    {
      /* get x and y values from data (not screen) */
      xval = data->xarray[row_num]; 
      yval = data->yarray[row_num]; 
      row_num = row_num + data->begin; 
      sprintf(rowstring, "Row Number: %d", row_num);
      /* draw crosshair on point */
      draw_xhair(xval, yval);
    }
  else 
    {
      sprintf(rowstring, "Row Number: None");
      sprintf(msg, "The point picked was not on the curve.\n");
      print_msg(msg);
    }
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_ROW), PANEL_LABEL_STRING, rowstring, NULL);	      
  /*  print the x and y coord on the panels */
  sprintf(xstring, "X: %.5g", xval); 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_X), PANEL_LABEL_STRING, xstring, NULL);	      
  sprintf(ystring, "Y: %.5g", yval);
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_Y), PANEL_LABEL_STRING, ystring, NULL);

  if (draw_string == 1)
    {
      sprintf(xyrowstring, "(%.5g, %.5g) row %d", xval, yval, row_num);
      xloc = (xval - data->xmin)*data->scale_x + can_info->start_x;
      yloc = can_info->start_y - (yval - data->ymin)*data->scale_y; 
      XDrawString((Display *)xv_get(canvas, XV_DISPLAY) , can_info->win, gctick, xloc+10, yloc+10, xyrowstring, strlen(xyrowstring));
    }
  
}

/*
print the x and y coord on the info panel and msg window

void draw_xy(xloc, yloc)
     int xloc, yloc;
{
  canvasinfo *can_info;
  plotarray *data;
  char xstring[20], ystring[20], rowstring[20];
  Canvas canvas;
  float xval, yval;
  
  can_info = wininfo.canvases[active_window];
  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }
  data = can_info->plots[can_info->active_plot];
  canvas = can_info->canvas;

  xval = (xloc - can_info->start_x)/data->scale_x + data->xmin;
  yval = (can_info->start_y - yloc)/data->scale_y + data->ymin;
  
  sprintf(rowstring, "");
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_ROW), 
	 PANEL_LABEL_STRING, rowstring, NULL);
  

  sprintf(xstring, "X: %.5g", xval); 
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_X), PANEL_LABEL_STRING, xstring, NULL);	      
  sprintf(ystring, "Y: %.5g", yval);
  xv_set(xv_get(canvas, XV_KEY_DATA, CAN_Y), PANEL_LABEL_STRING, ystring, NULL);

  strcat(xstring, "  ");
  strcat(xstring, ystring);
  strcpy(msg, xstring);
  strcat(msg, "\n");
  print_msg(msg);
  
}

*/




int get_row_number(nrow, x1, y1)
     int nrow;
     float x1, y1;
{
  int   ii, j;
  int	*list;
  float	xmax,xmin,ymax,ymin;
  double min_dist, dist;
  canvasinfo *can_info;
  plotarray *data;
  int rownum;
  float *x, *y;
  
  can_info = wininfo.canvases[active_window];
  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return -1;
  }
  data = can_info->plots[can_info->active_plot];
  
  list = (int *)malloc((unsigned)(nrow*sizeof(int)));
  /*list = (int *)calloc((unsigned)nrow,sizeof(int));*/

  xmax = data->xmax;
  xmin = data->xmin;
  ymax = data->ymax;
  ymin = data->ymin;
  x = data->xarray;
  y = data->yarray;
  
  j = 0;
  rownum = -1;		/* default value for "nothing fits" */

  for(ii = 0; ii < nrow; ++ii)
    {
      /* this doesn't do too well if x[ii] = x[ii+1] */
      if( (x1 >= x[ii]) && (x1 <= x[ii+1]) )
	list[j++] = ii;		
      /* in case function is multiply defined, search for */
      /*   all cases where "picked x" is between two points */
   }		
  
  /* then calc. dist. to each point and take min. */
  min_dist = 1e100;

  for(ii = 0; ii < j; ++ii)
    {
      /* check both point in list and next point (the two that bracket x1) */
      /* to see which fits best */
      if( (dist = sqrt( (x1-x[list[ii]])*(x1-x[list[ii]]) + (y1-y[list[ii]])*(y1-y[list[ii]]) ) ) < min_dist)
	{
	  min_dist = dist;	
	  rownum = list[ii];
	}
      
      if( (dist = sqrt( (x1-x[list[ii]+1])*(x1-x[list[ii]+1]) + (y1-y[list[ii]+1])*(y1-y[list[ii]+1]) ) ) < min_dist)
	{
	  min_dist = dist;	
	  rownum = list[ii]+1;
	}
    }

  free(list);
  return rownum;
  }



void draw_xhair(xval, yval)
     float xval, yval;
{
  canvasinfo *can_info;
  int xloc, yloc;
  int start_x, start_y;
  float scale_x, scale_y;
  float xmin, ymin;
  int active_plot;
  
  can_info = wininfo.canvases[active_window];
  active_plot = can_info->active_plot;
  if (can_info->active_plot == -1)
  {
      sprintf(msg, "There is no plot in this window.\n");
      print_msg(msg);
      return;
  }
  start_x = can_info->start_x;
  start_y = can_info->start_y;
  scale_x = can_info->plots[active_plot]->scale_x;
  scale_y = can_info->plots[active_plot]->scale_y;
  xmin = can_info->plots[active_plot]->xmin;
  ymin= can_info->plots[active_plot]->ymin;

  xloc = (xval-xmin)*scale_x;
  yloc = (yval-ymin)*scale_y;
  
  XDrawLine((Display *)xv_get(can_info->canvas, XV_DISPLAY), can_info->win, gctitle, 
	    (start_x + xloc - 5),
	    (start_y - yloc),
	    (start_x + xloc + 5),
	    (start_y - yloc));
  XDrawLine((Display *)xv_get(can_info->canvas, XV_DISPLAY), can_info->win, gctitle, 
	    start_x + xloc,
	    start_y - yloc - 5,
	    start_x + xloc,
	    start_y - yloc + 5);

}

void draw_crosshair(xloc, yloc)
     int xloc, yloc;
{
  canvasinfo *can_info;

  can_info = wininfo.canvases[active_window];
  
  XDrawLine((Display *)xv_get(can_info->canvas, XV_DISPLAY), can_info->win, gctitle, 
	    xloc-5, yloc, 
	    xloc+5, yloc);
  
  XDrawLine((Display *)xv_get(can_info->canvas, XV_DISPLAY), can_info->win, gctitle, 
	    xloc, yloc-5,
	    xloc, yloc+5);
  
}

void clr_ap()
{
  
canvasinfo *can_info;
  int i;
  
  can_info = wininfo.canvases[active_window];

  for (i=0; i<MAX_PLOTS; i++)
    {
      if (can_info->alive_plots[i] == 1)
	{
/*	  free (can_info->plots[i]);*/
	  can_info->alive_plots[i] = 0;
	}
    }
  can_info->total_plots = 0;
  can_info->active_plot = -1;
  
  sprintf(msg, "Clear all plots.\n");
  print_msg(msg);
  
  clear_canvas_proc(can_info->canvas);
  display_active_window(active_window+1);
  display_active_plot(-1);
}

/*void zoom_clr_plots_notify_proc(item, event)
     Panel_item item;
     Event *event;
{
  set_active_window(xv_get(item, PANEL_CLIENT_DATA));
}*/




