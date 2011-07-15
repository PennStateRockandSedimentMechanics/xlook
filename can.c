/* cjm 21.1.08:  added a new line to plot window info, for xlook compile version and to
deal with wrapping problem caused by OS 10.5*/

#include <math.h>

#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/xv_xrect.h>
#include <xview/font.h>
#include <xview/cursor.h>

#include <config.h>
#include <global.h>
#include <messages.h>
#include <can.h>

extern int active_window;
extern int total_windows;
extern int old_active_window;
extern int tickfontheight, titlefontheight;
extern char msg[MSG_LENGTH];

extern Frame main_frame;
Xv_Cursor xhair_cursor;

short canvas0_image[] = {
#include "canvas.ico"
};
Server_image c0image;
Icon cstate;


void setup_canvas()
{
  canvasinfo *can_info;
  Canvas canvas;
  int start_xaxis, start_yaxis, end_xaxis, end_yaxis;
  int canvas_width, canvas_height;
  int start_x, start_y, end_x, end_y;
  
  can_info = wininfo.canvases[active_window];
  canvas = can_info->canvas;
  
  canvas_width = (int) xv_get(canvas, XV_WIDTH);
  canvas_height = (int) xv_get(canvas, XV_HEIGHT);
  
  /* xaxis is from 0.15 to 0.85 canvas width */
  start_xaxis = (int)canvas_width*0.15;
  end_xaxis = canvas_width - (int)canvas_width*0.15;

/* yaxis is from 0.2 to 0.9 canvas height; 
     make room for title at the top (4 lines),
     make room for xaxis labels at the bottom (4 lines)
     and also 2 pixels of space between lines */
  start_yaxis = canvas_height - 5*tickfontheight - 10;
  end_yaxis = titlefontheight*4 + 10; 

  /* plotting area */
  start_x = start_xaxis + canvas_width/20;
  end_x = end_xaxis - canvas_width/20; 
  
  /* plotting area */
  start_y = start_yaxis - canvas_height/20; 
  end_y = end_yaxis;
    
  can_info->start_x = start_x;
  can_info->end_x = end_x;
  can_info->start_y = start_y;
  can_info->end_y = end_y;
  can_info->start_xaxis = start_xaxis;
  can_info->end_xaxis = end_xaxis;
  can_info->start_yaxis = start_yaxis;
  can_info->end_yaxis = end_yaxis;
  can_info->point_plot = 0;
  
}


void create_canvas()
{ Frame graf_frame;
  Canvas canvas;
  Xv_Window xvwin;
  Window win;
  
  Panel canvas_menu_panel;
  /*Panel canvas_info_panel;*/
  Panel_item X, Y, ROW, PLOT_ROWS;
  Panel_item XLOOK_VERSION;
  canvasinfo *can_info;
  int win_num;
  int i;
  Menu clr_plot_menu, act_plot_menu, mouse_menu, zoom_menu;

  extern void canvas_event_proc(), redraw_proc(), resize_proc(), refresh_win_proc();
  extern void clr_plots_proc(), clr_all_plots_proc(), clr_plots_notify_proc();
  extern void clear_win_proc(), kill_graf_proc(), zoom_plot_proc(), zoom_new_plot_proc(); 
  extern void zoom_clr_all_plots_proc(), zoom_clr_plots_notify_proc();
  extern void create_act_plot_menu_proc(), get_act_item_proc();
  extern void line_plot_proc(), mouse_mu_proc(), dist_proc();
  extern void point_plot_proc();
 
/* 
sprintf(msg, "cjm. in can.c window menu, main frame!\n");
print_msg(msg);
*/
  win_num = get_new_window_num();
  
  if (win_num == -1)
  {
      sprintf(msg, "Reached limit of 10 windows. Ignored!\n");
      print_msg(msg);
      total_windows--;
      return;
  }
/*----------------ICON SETUP---------------------------------*/
  c0image = (Server_image) xv_create (XV_NULL, SERVER_IMAGE,
      XV_WIDTH,        64,
      XV_HEIGHT,       64,
      SERVER_IMAGE_BITS,  canvas0_image,
      NULL );

  cstate = (Icon) xv_create (XV_NULL, ICON,
                          ICON_IMAGE, c0image,
                          ICON_LABEL, "Xlook",
                          NULL );

  sprintf(msg, "Plot Window %d", win_num+1);
  
  graf_frame = (Frame)xv_create(main_frame, FRAME,
				FRAME_ICON, cstate,
				FRAME_LABEL, msg,
				XV_HEIGHT, 500, 
				XV_WIDTH, 800,
				FRAME_SHOW_FOOTER, TRUE,
				NULL);

  canvas_menu_panel = (Panel)xv_create(graf_frame, PANEL, XV_HEIGHT, 85, NULL);
  
  XLOOK_VERSION = (Panel_item)xv_create(canvas_menu_panel, PANEL_MESSAGE,
			    PANEL_LABEL_STRING, "xlook version: working.2011",
			    PANEL_VALUE_DISPLAY_LENGTH, 20,
			    NULL);

 /*move Kill button to top row;  This works fine for 10.4.11*/ 

  (void) xv_create(canvas_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Kill Window",
		   PANEL_NOTIFY_PROC, kill_graf_proc,
		   PANEL_CLIENT_DATA, win_num,
		   NULL);

  act_plot_menu = (Menu) xv_create(XV_NULL, MENU,
				   NULL);
  
  (void) xv_create(canvas_menu_panel, PANEL_BUTTON,
			    PANEL_NEXT_ROW, -1,
		   PANEL_LABEL_STRING, "Activate Plot",
		   PANEL_ITEM_MENU, act_plot_menu,
		   PANEL_NOTIFY_PROC, create_act_plot_menu_proc,
		   PANEL_CLIENT_DATA, win_num,
		   NULL);

  clr_plot_menu = (Menu) xv_create(XV_NULL, MENU,
				   MENU_GEN_PIN_WINDOW, 
				   graf_frame, "Clear Plots", 
				   MENU_CLIENT_DATA, win_num,
				   MENU_ACTION_ITEM, 
				   "Clear All Plots", clr_all_plots_proc,
				   MENU_ACTION_ITEM,
				  "Select Plots...", clr_plots_proc,
				   NULL);
  
  (void) xv_create(canvas_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Clear Plot(s)",
		   PANEL_ITEM_MENU, clr_plot_menu,
		   PANEL_NOTIFY_PROC, clr_plots_notify_proc,
		   PANEL_CLIENT_DATA, win_num,
		   NULL);

  (void) xv_create(canvas_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Plot Points",
		   PANEL_NOTIFY_PROC, point_plot_proc,
		   PANEL_CLIENT_DATA, win_num,
		   NULL);

  zoom_menu     = (Menu) xv_create(XV_NULL, MENU, 
				   MENU_GEN_PIN_WINDOW, 
/* cjm 22.1.08*, change to graf frame, main_frame is the other --main-- window, seems like this was a mistake
				   main_frame, "Zoom", */
				   graf_frame, "Zoom", 
				   MENU_CLIENT_DATA, win_num,
				   MENU_ACTION_ITEM, 
				   "Zoom and Clear", zoom_clr_all_plots_proc,
				   MENU_ACTION_ITEM,
				  "Zoom", zoom_plot_proc,
				   MENU_ACTION_ITEM,
				  "Zoom New Window", zoom_new_plot_proc,
				   NULL);

  (void) xv_create(canvas_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Zoom",
		   PANEL_ITEM_MENU, zoom_menu,
		   PANEL_CLIENT_DATA, win_num,
		   NULL);			   
		   /*PANEL_NOTIFY_PROC, zoom_clr_plots_notify_proc,*/
  
  mouse_menu = (Menu) xv_create(XV_NULL, MENU,
				MENU_GEN_PIN_WINDOW, 
/* cjm 22.1.08*, change to graf frame, main_frame is the other --main-- window, seems like this was a mistake
				main_frame, "Mouse", */
				graf_frame, "Mouse", 
				MENU_CLIENT_DATA, win_num,
				MENU_ACTION_ITEM, 
				"Line Plot", line_plot_proc,
				MENU_ACTION_ITEM, 
				"Vertical Line", mouse_mu_proc,
				MENU_ACTION_ITEM, 
				"Distance", dist_proc,
				NULL);
  
  (void) xv_create(canvas_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Mouse",
		   PANEL_ITEM_MENU, mouse_menu,
		   PANEL_CLIENT_DATA, win_num,
		   NULL);

 (void) xv_create(canvas_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Refresh",
		   PANEL_NOTIFY_PROC, refresh_win_proc,
		   PANEL_CLIENT_DATA, win_num,
		   NULL);


  /*canvas_info_panel=(Panel)xv_create(graf_frame, PANEL, XV_HEIGHT, 30, NULL);*/

  X = (Panel_item)xv_create(canvas_menu_panel, PANEL_MESSAGE,
			    PANEL_NEXT_ROW, -1,
			    PANEL_LABEL_STRING, "X: ",
			    PANEL_VALUE_DISPLAY_LENGTH, 8,
			    NULL);
  
  Y = (Panel_item)xv_create(canvas_menu_panel, PANEL_MESSAGE,
                            PANEL_LABEL_STRING, "Y: ",
			    XV_X, xv_col(canvas_menu_panel, 15),
			    PANEL_VALUE_DISPLAY_LENGTH, 8,
			    NULL);
  
  ROW = (Panel_item)xv_create(canvas_menu_panel, PANEL_MESSAGE,
  			      PANEL_LABEL_STRING, "ROW#: ",
			      XV_X, xv_col(canvas_menu_panel, 25),
			      PANEL_VALUE_DISPLAY_LENGTH, 8,
			      NULL);
  
  PLOT_ROWS = (Panel_item)xv_create(canvas_menu_panel, PANEL_MESSAGE,
  			      PANEL_LABEL_STRING, "PLOT ROWS: ",
			      XV_X, xv_col(canvas_menu_panel, 50),
			      PANEL_VALUE_DISPLAY_LENGTH, 10,
			      NULL);
  
/*cjm: 10.4.07 this is the fix for buttons that don't work. Must be something in PANEL_TEXT that activates*/
  (void) xv_create(canvas_menu_panel, PANEL_TEXT,
                   PANEL_LABEL_STRING, "",
                   PANEL_VALUE_DISPLAY_LENGTH, 1,
		   XV_X, xv_col(canvas_menu_panel, 80),
/*                   PANEL_VALUE, parameter_strs[0],
                   PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
                   PANEL_NOTIFY_PROC, get_options, */
                   NULL);

  canvas = (Canvas)xv_create(graf_frame, CANVAS, 
			     CANVAS_RESIZE_PROC, resize_proc, 
			     CANVAS_RETAINED, TRUE,
			     CANVAS_REPAINT_PROC, redraw_proc,
			     WIN_BELOW, canvas_menu_panel,
			     CANVAS_AUTO_SHRINK, TRUE,
			     CANVAS_AUTO_EXPAND, TRUE,
			     CANVAS_X_PAINT_WINDOW, TRUE, 
			     NULL); 
  
  xv_set(canvas_paint_window(canvas),
	 WIN_CURSOR, xhair_cursor, NULL);
 
  xv_set(canvas_paint_window(canvas), 
	 WIN_EVENT_PROC, canvas_event_proc,
	 /*WIN_CONSUME_EVENTS, WIN_NO_EVENTS, WIN_MOUSE_BUTTONS, LOC_DRAG, LOC_MOVE, NULL, cjm 28.3.08
this makes plot windows open with no data (plot) in them... */
	 NULL);

  xv_set(canvas, XV_KEY_DATA, CAN_X, X,  NULL);
  xv_set(canvas, XV_KEY_DATA, CAN_Y, Y,  NULL);
  xv_set(canvas, XV_KEY_DATA, CAN_ROW, ROW, NULL);
  xv_set(canvas, XV_KEY_DATA, CAN_NUM, win_num, NULL);
  xv_set(canvas, XV_KEY_DATA, GRAF_FRAME, graf_frame, NULL);
  xv_set(canvas, XV_KEY_DATA, CAN_PLOT_ROWS, PLOT_ROWS, NULL);
  
  window_fit(graf_frame);

  can_info = (canvasinfo *) malloc(sizeof(canvasinfo));
  if (can_info == NULL)
    {
      print_msg("Memory allocation error. Window cannot be created.");
      return;
    }
  
  old_active_window = active_window;
  active_window = win_num;
  can_info->canvas_num = active_window;
  can_info->total_plots = 0;
  can_info->active_plot = -1;
  
  for (i=0; i<10; i++)
    can_info->alive_plots[i] = 0 ;
  
  can_info->canvas = canvas;
  xvwin = (Xv_Window)canvas_paint_window(canvas);
  can_info->xvwin = xvwin;
  win = (Window) xv_get(xvwin, XV_XID);
  can_info->win = win;
    
  display_active_window(active_window+1);
  display_active_plot(0);
  
  wininfo.windows[active_window] = 1;
  wininfo.canvases[active_window] = can_info;

  setup_canvas();
  xv_set(graf_frame, XV_SHOW, TRUE, NULL);

/*  sprintf(msg, "(create_canvas) Window #%d is active.\n", active_window+1);
  print_msg(msg); 
  sprintf(msg, "(create_canvas)total windows = %d\n", total_windows+1);
  print_msg(msg);

  sprintf(msg, "(create_canvas) Window #%d is old active window.\n", old_active_window+1);
  print_msg(msg); */

}


int get_new_window_num()
{
  int i;
  
  for (i = 0; i <10; i++)
    {
      if (wininfo.windows[i] == 0)
	return i;
    }
  /* reached plots limit.. need to delete one or more plots from window */
  return(-1);
  
}

