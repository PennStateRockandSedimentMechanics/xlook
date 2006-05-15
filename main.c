#include <math.h>

#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/sel_attrs.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/notice.h>
#include <xview/font.h>
#include <xview/scrollbar.h>
#include <xview/cursor.h>

#include <cmds1.h>
#include <config.h>
#include <drawwin.h>
#include <global.h>
#include <look_funcs.h>
#include <main.h>
#include <messages.h>
#include <mouse.h>

int active_window = -1;
int old_active_window = -1;
int total_windows = -1;
int action = 0;
int cmd_num = 0;

char msg[MSG_LENGTH];

short state0_image[] = {
#include "xlook.ico"
};
Server_image s0image;
Icon state;

extern char plot_cmd[256];
extern int plot_error;
extern int xaxis, yaxis;
extern char default_path[80];
extern int read_flag;
extern char qiparams[256];

/* main_frame objects */
Frame main_frame;
Panel main_frame_menu_panel;
Panel main_frame_cmd_panel;
Panel_item cmd_panel_item;
Panel main_frame_info_panel;
Panel_item active_win_info, active_plot_info, active_file_info;
Textsw msgwindow;
Textsw fileinfo_window;
int textsw_total_used;
int textsw_memory_limit;
int textsw_saves=0;



/* cmd_hist_frame objects */
Frame cmd_hist_frame;
Panel cmd_hist_panel;
Panel_item cmd_hist_panel_list;

int tickfontheight, tickfontwidth;
int titlefontheight, titlefontwidth;
GC gctitle, gctick;
GC gcbg;
GC gcrubber;
Xv_Cursor xhair_cursor;
XColor fgc, bgc;


int main(argc, argv)
     int argc;
     char *argv[];    
{
  void plot1_proc(), plot2_proc(), plot3_proc(), plot4_proc(), plot5_proc(), plot6_proc(), plot7_proc(), plot8_proc();
  void new_window_proc(), act_window_proc(), kill_window_proc();
  void close_graf_proc(), show_hist_proc();
  void exit_proc(), get_cmd_proc(), cmd_hist_proc(), close_hist_proc();
  void qi_window_proc();

  Display *dpy;
  XGCValues gcvalues;
  Xv_Font titlefont, tickfont; 
  Menu file_menu, window_menu, plot_cmd_menu;
  Colormap cmap;
  int i;
  char *bgcolor="white", *fgcolor="black";
  void extern write_file_proc(), read_file_proc(), exit_file_proc();
  
   for (i=1; i<argc; i++)
    {
      char *arg = argv[i];
      if (arg[0] == '-')
	{
	  switch (arg[1])
	    {
	    case 'b':
	      if (++i < argc)
		bgcolor = argv[i];
	      continue;
	    case 'f':
	      if (++i < argc)
		fgcolor = argv[i];
	      continue;
	    default:
	      break;
	    }
	}
    }
	     
  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

  initialize(argc, argv);

/*----------------ICON SETUP---------------------------------*/
s0image = (Server_image) xv_create (XV_NULL, SERVER_IMAGE,
      XV_WIDTH,        64,
      XV_HEIGHT,       64,
      SERVER_IMAGE_BITS,  state0_image,
      NULL );

state = (Icon) xv_create (XV_NULL, ICON,
                          ICON_IMAGE, s0image,
                          ICON_LABEL, "Xlook",
                          NULL );

/* ------------------------  FRAMES INITIALIZATION ------------------------ */

  main_frame = (Frame)xv_create(XV_NULL, FRAME,
                                FRAME_ICON, state,
				FRAME_SHOW_FOOTER, TRUE,
				FRAME_LEFT_FOOTER, "Type your command in the command panel",
				FRAME_LABEL, "XLook", NULL);


  cmd_hist_frame = (Frame)xv_create(main_frame, FRAME, FRAME_LABEL, "Command History",
			    NULL);
  
  
/* ------------------------ MAIN FRAME SETUP ----------------------------- */


/* --------------------------  MENUS CREATION ----------------------- */

  main_frame_menu_panel = (Panel)xv_create(main_frame, PANEL, XV_HEIGHT, 55, NULL);

  file_menu = (Menu) xv_create(XV_NULL, MENU,
			       MENU_GEN_PIN_WINDOW, main_frame, "File",
			       MENU_ACTION_ITEM, 
			       "Read File", read_file_proc,
			       MENU_ACTION_ITEM,
			       "Write File", write_file_proc,
			       NULL);

  (void) xv_create(main_frame_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "FILE",
		   PANEL_ITEM_MENU, file_menu,
		   NULL);
  
  window_menu = (Menu) xv_create(XV_NULL, MENU,
				 MENU_GEN_PIN_WINDOW, main_frame, "Window",
				 MENU_ACTION_ITEM, 
				 "New Window", new_window_proc,
				 MENU_ACTION_ITEM, 
				 "Activate Window...", act_window_proc,
				 MENU_ACTION_ITEM,
				 "Kill Window...", kill_window_proc,
				 NULL);
  
  (void) xv_create(main_frame_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "WINDOW",
		   PANEL_ITEM_MENU, window_menu,
		   NULL);
  
  plot_cmd_menu = (Menu) xv_create(XV_NULL, MENU,
				   MENU_GEN_PIN_WINDOW, 
				   main_frame, "Plot Commands", 
				   MENU_ACTION_ITEM, 
				   "Plotall...", plot1_proc,
				   MENU_ACTION_ITEM, 
				   "Plotover...", plot2_proc,
				   MENU_ACTION_ITEM, 
				   "Plotsr...", plot3_proc,
				   MENU_ACTION_ITEM, 
				   "Plotauto...", plot4_proc,
				   MENU_ACTION_ITEM, 
				   "Plotlog...", plot5_proc,
				   MENU_ACTION_ITEM, 
				   "Plotsame...", plot6_proc,
				   MENU_ACTION_ITEM, 
				   "Plotscale...", plot7_proc,
				   MENU_ACTION_ITEM, 
				   "Pa...", plot8_proc,
				   NULL);

  (void) xv_create(main_frame_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "PLOT",
		   PANEL_ITEM_MENU, plot_cmd_menu,
		   NULL);

  (void) xv_create(main_frame_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "SHOW HISTORY",
		   PANEL_NOTIFY_PROC, show_hist_proc, 		 
		   PANEL_CLIENT_DATA, cmd_hist_frame,
		   NULL);

  /* Thu Mar 13 16:25:44 EST 1997
     by park
     for adding qi button
     nofify proc is borrowed from new_window

     Mon Mar 17 17:55:08 EST 1997
     by park
     new_window_proc -> qi_window_proc
     
     */

  (void) xv_create(main_frame_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "R/S FRIC TOOL",
		   PANEL_NOTIFY_PROC, qi_window_proc,
		   NULL);

  (void) xv_create(main_frame_menu_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "QUIT",
		   PANEL_NOTIFY_PROC, exit_proc,
		   NULL);


/* ----------------------------- COMMAND PANEL --------------------------*/

  main_frame_cmd_panel = (Panel)xv_create(main_frame, PANEL, 
			       PANEL_LAYOUT, PANEL_VERTICAL,
			       XV_HEIGHT, 35,
			       WIN_BELOW, main_frame_menu_panel,
			       NULL);

  cmd_panel_item = (Panel_item) xv_create(main_frame_cmd_panel, PANEL_TEXT,
				 PANEL_LABEL_STRING, "Command: ",
				 PANEL_VALUE_STORED_LENGTH, 200,
				 PANEL_VALUE_DISPLAY_LENGTH, 80,
				 PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
				 PANEL_NOTIFY_STRING, "\r",
				 PANEL_NOTIFY_PROC, get_cmd_proc,
				 NULL);

  
/* -------------------------- FILE INFO WINDOW ---------------------------- */

  fileinfo_window = (Textsw)xv_create(main_frame, TEXTSW,
			        TEXTSW_IGNORE_LIMIT, TEXTSW_INFINITY,
				WIN_ROWS, 7,
				TEXTSW_BROWSING, TRUE,
        			TEXTSW_CONFIRM_OVERWRITE, TRUE,
				TEXTSW_LINE_BREAK_ACTION, TEXTSW_WRAP_AT_WORD,
				NULL);

 /* (void) xv_create(fileinfo_window, SCROLLBAR,
		   SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
		   NULL); */
  

/* ------------------------------ INFO PANEL -------------------------- */

  main_frame_info_panel = (Panel)xv_create(main_frame, PANEL, XV_HEIGHT, 35, NULL);

  active_win_info = (Panel_item)xv_create(main_frame_info_panel, PANEL_MESSAGE,
					  PANEL_NEXT_ROW, -1,
					  PANEL_LABEL_STRING, "Active Window: ",
					  NULL);
  
  active_plot_info = (Panel_item)xv_create(main_frame_info_panel, PANEL_MESSAGE,
					   XV_X, xv_col(main_frame_info_panel, 20),
					   PANEL_LABEL_STRING, "Active Plot: ",
					   NULL);


  active_file_info = (Panel_item)xv_create(main_frame_info_panel, PANEL_MESSAGE,
					   XV_X, xv_col(main_frame_info_panel, 50),
					   PANEL_LABEL_STRING, "File: ",
					   NULL);


/* ----------------------------- MESSAGE WINDOW --------------------------*/

  msgwindow = (Textsw)xv_create(main_frame, TEXTSW,
				TEXTSW_IGNORE_LIMIT, TEXTSW_INFINITY,
				WIN_ROWS, 20,
				TEXTSW_BROWSING, TRUE,
				TEXTSW_LINE_BREAK_ACTION, TEXTSW_WRAP_AT_WORD,
				TEXTSW_MEMORY_MAXIMUM, 200000,
				NULL);
  
 /* (void) xv_create(msgwindow, SCROLLBAR,
		   SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
		   NULL); */

  (void)textsw_possibly_normalize(msgwindow, (Textsw_index)xv_get(msgwindow, TEXTSW_INSERTION_POINT));

  window_fit(msgwindow);
  

  

/* -----------------  CMD_HISTORY FRAME SETUP  ------------------- */

  cmd_hist_panel = (Panel)xv_create(cmd_hist_frame, PANEL,
				    PANEL_LAYOUT, PANEL_VERTICAL,
				    NULL);

  (void) xv_create(cmd_hist_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "CLOSE",
		   PANEL_NOTIFY_PROC, close_hist_proc,
		   NULL);
  
  cmd_hist_panel_list = (Panel_item) xv_create(cmd_hist_panel, PANEL_LIST,
					  PANEL_LIST_DISPLAY_ROWS, 10,
					  PANEL_LIST_MODE, PANEL_LIST_READ, 
					  PANEL_LIST_WIDTH, -1,
					  PANEL_NOTIFY_PROC, cmd_hist_proc,
					  NULL);
  window_fit(cmd_hist_panel);
  window_fit(cmd_hist_frame);
  
  xhair_cursor = (Xv_Cursor)xv_create(XV_NULL, CURSOR,
				      CURSOR_SRC_CHAR, 22,
				      NULL);

/*----------------  PLOTTING WINDOW SETUP  ---------------------   */
 /* create_canvas(); */

 /* create font for title */
  titlefont=(Xv_Font)xv_find(main_frame, FONT, 
			     /* FONT_NAME, "-adobe-courier-medium-r-normal--14-140-75-75-m-90-iso8859-1", 	*/
			     FONT_FAMILY, FONT_FAMILY_LUCIDA, 		 
			     FONT_STYLE, FONT_STYLE_NORMAL, 	
			     FONT_SIZE, 14,
			     NULL);

  /* create font for label */
  tickfont=(Xv_Font)xv_find(main_frame, FONT,
			    /*FONT_RESCALE_OF, titlefont, WIN_SCALE_SMALL,*/
			    FONT_FAMILY, FONT_FAMILY_LUCIDA,
			    FONT_STYLE, FONT_STYLE_NORMAL,
			    FONT_SIZE, 12,
			    NULL);
	
  dpy = (Display *)xv_get(main_frame, XV_DISPLAY);
 
  /* allocate color map for background and foreground colors */
  cmap = DefaultColormap(dpy, DefaultScreen(dpy));
  if (XAllocNamedColor (dpy, cmap, bgcolor, &bgc, &bgc) == 0);
  if (XAllocNamedColor (dpy, cmap, fgcolor, &fgc, &fgc) == 0);

  /* create gc for title font */
  gcvalues.foreground = fgc.pixel;
  gcvalues.background = bgc.pixel;
  gcvalues.font = (Font)xv_get(titlefont, XV_XID);
  gcvalues.graphics_exposures = False;
  gctitle = XCreateGC(dpy, RootWindow(dpy, DefaultScreen(dpy)), GCFont | GCGraphicsExposures | GCForeground | GCBackground, &gcvalues);

 /* create gc for tick font */
  gcvalues.foreground = fgc.pixel;
  gcvalues.background = bgc.pixel;
  gcvalues.font = (Font)xv_get(tickfont, XV_XID);
  gcvalues.graphics_exposures = False;
  gctick = XCreateGC(dpy, RootWindow(dpy, DefaultScreen(dpy)), GCFont | GCGraphicsExposures | GCForeground | GCBackground, &gcvalues);
  
  /* create gc for rubber-band-like lines */
  gcvalues.foreground = fgc.pixel;
  gcvalues.background = bgc.pixel;
  gcvalues.graphics_exposures = False;
  gcrubber = XCreateGC(dpy, RootWindow(dpy, DefaultScreen(dpy)), GCGraphicsExposures | GCForeground | GCBackground, &gcvalues);
  XSetFunction(dpy, gcrubber, GXxor);
  
  gcvalues.foreground = bgc.pixel;
  gcbg = XCreateGC(dpy, RootWindow(dpy, DefaultScreen(dpy)), GCForeground, &gcvalues);
 

  titlefontheight=(int)xv_get(titlefont, FONT_DEFAULT_CHAR_HEIGHT);
  titlefontwidth=(int)xv_get(titlefont, FONT_DEFAULT_CHAR_WIDTH);

  tickfontheight=(int)xv_get(tickfont, FONT_DEFAULT_CHAR_HEIGHT);
  tickfontwidth=(int)xv_get(tickfont, FONT_DEFAULT_CHAR_WIDTH);
  
/* ------------------------------------------------------------------------ */

  window_fit(main_frame);

  display_active_window(0);
  display_active_plot(0);
  display_active_file(0);

  textsw_memory_limit = (int)xv_get(msgwindow, TEXTSW_MEMORY_MAXIMUM);
  

  xv_main_loop(main_frame);
  return 0;  
}  



void quit_xlook()
{
  extern Frame clr_plots_frame;

  if (clr_plots_frame) xv_destroy_safe(clr_plots_frame);
  xv_destroy_safe(main_frame);
}





/* ------------------------ FILE MENU PROCS ------------------------------ */
/*
 not used anymore. use the quit button instead.

void exit_file_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  quit_xlook();
}
*/


void read_file_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  set_left_footer("Type the data file to read");	  
  action = READ;
  set_cmd_prompt("Filename: ");
}

void write_file_proc(menu, item)
     Menu menu;
     Menu_item item;
{ 
  set_left_footer("Type the file to write");
  action = WRITE; 
  set_cmd_prompt("Filename: ");
}

void append_file_proc(menu, item)
     Menu menu;
     Menu_item item;
{  
  set_left_footer("Type the file to append");
  action = APPEND; 
  set_cmd_prompt("Filename: ");
}


/* ------------------------ WINDOW MENU PROCS ---------------------------- */
void act_window_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  set_left_footer("Which window to be set active?");
  set_cmd_prompt("Window Number: ");
  action = SET_ACTIVE_WINDOW;
}

void kill_window_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  set_left_footer("Type the window number to destroy");
  set_cmd_prompt("Window Number: ");
  action = KILL_WIN;
}

void new_window_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  new_win_proc();  
}

void qi_window_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  strcpy(qiparams, "");
  qi_win_proc();
}

  

/* ----------------------- PLOT COMMANDS MENU --------------------------- */
void plot1_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  strcpy(plot_cmd, "plotall");
  goto_plot1_proc();
}

void plot2_proc(menu, item)
     Menu menu;
     Menu_item item;
{
 strcpy(plot_cmd, "plotover");
 goto_plot1_proc();
}

void plot3_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  strcpy(plot_cmd, "plotsr");
  goto_plot1_proc();
}

void plot4_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  strcpy(plot_cmd, "plotauto");
  goto_plot2_proc();
}

void plot5_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  strcpy(plot_cmd, "plotlog");
  goto_plot2_proc();
}

void plot6_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  strcpy(plot_cmd, "plotsame");
  goto_plot2_proc();
}

void plot7_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  strcpy(plot_cmd, "plotscale");
  goto_plot2_proc();
}

void plot8_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  strcpy(plot_cmd, "pa");
  goto_plot2_proc();
}


void goto_plot1_proc()
{
   set_left_footer("Type the x-axis and y-axis");
   set_cmd_prompt("X-AXIS Y-AXIS: ");
   action = PLOT_GET_XY; 
 }

void goto_plot2_proc()
{
   set_left_footer("Type the x-axis and y-axis");
   set_cmd_prompt("X-AXIS Y-AXIS: ");
   action = PLOT_GET_BE; 
 }


/*  ----------------------- BUTTONS PROCEDURES ---------------------------  */ 

void exit_proc(item, event)
     Panel_item item;
     Event *event;
{
  quit_xlook();
}


void get_cmd_proc(item, event)
     Panel_item item;
     Event *event;
{
  /* command panel callback function; 
     passes a copy of the string to command_handler() */

  extern void command_handler();
  char cmd_text[256];
  char buf[8];
  
  strcpy(buf," ,\n\t");	/*set of null chars for cmd*/

  strcpy(cmd_text, (char *)xv_get(item, PANEL_VALUE));

/*only do something if a command was entered. Don't do anything if user just hit return*/

  if(strcspn(cmd_text,buf))
  {
  	xv_set(cmd_hist_panel_list, PANEL_LIST_INSERT, cmd_num, PANEL_LIST_STRING, cmd_num, cmd_text, NULL);
  	cmd_num++;

  	xv_set(item, PANEL_VALUE, "", NULL); 

  	/* call cmd multiplexor  */
  	command_handler(cmd_text);
  }
  else	/*just clear line*/
  	xv_set(item, PANEL_VALUE, "", NULL); 

}


void cmd_hist_proc(item, cmd_text, client_data, op, event, row)
     Panel_item item;
     char *cmd_text;
     Xv_opaque client_data;
     Panel_list_op op;
     Event *event;
     int row;
{
  /* copies every input to the history command window;
   don't exactly know how big this buffer can be */

  xv_set(cmd_panel_item, PANEL_VALUE, cmd_text, NULL);
  
}

  
void show_hist_proc(item, event)
     Frame item;
     Event *event;
{
  /* maps the history window to screen */

  Frame cmd_hist_frame = (Frame)xv_get(item, PANEL_CLIENT_DATA);
  if ((int)xv_get(cmd_hist_frame, XV_SHOW)== FALSE)  
    xv_set(cmd_hist_frame, XV_SHOW, TRUE, NULL);
}


void close_hist_proc(item, event)
     Frame item;
     Event *event;
{
  /* unmaps the history window from screen */

  if ((int)xv_get(cmd_hist_frame, XV_SHOW) == TRUE)
    xv_set(cmd_hist_frame, XV_SHOW, FALSE, NULL);
}




/* --------------------------- CANVAS HANDLER --------------------------- */

/*
void close_graf_proc(item, event)
     Frame item;
     Event *event;
{
  Frame graf_frame;

  graf_frame = xv_get(wininfo.canvases[active_window]->canvas, XV_KEY_DATA, GRAF_FRAME);
  active_window = old_active_window;
  if (old_active_window != 0)
    old_active_window--;
  
  if ((int)xv_get(graf_frame, XV_SHOW) == TRUE)
    xv_set(graf_frame, XV_SHOW, FALSE, NULL);
}
*/

void clear_win_proc(item, event)
     Panel_item item;
     Event *event;
{
  canvasinfo *can_info;
  int can_num;
   
  can_num = xv_get(item, PANEL_CLIENT_DATA);
  set_active_window(can_num);
  can_info = wininfo.canvases[active_window];
  display_active_plot(-1);

  clr_all();
}

void clr_all()
{
  canvasinfo *can_info;
  int i;
  
  can_info = wininfo.canvases[active_window];

  for (i=0; i<10; i++)
    {
      if (can_info->alive_plots[i] == 1)
	{
	  free(can_info->plots[i]);
	  can_info->alive_plots[i] = 0;
	}
    }
  can_info->active_plot = -1;
  can_info->total_plots = 0;

  clear_canvas_proc(can_info->canvas);
}


void refresh_win_proc(item, event)
     Panel_item item;
     Event *event;
{
  canvasinfo *can_info;
  int can_num, active_plot;
  Canvas canvas;
  
  can_num = xv_get(item, PANEL_CLIENT_DATA);
  set_active_window(can_num);
  can_info = wininfo.canvases[active_window];

  if(can_info->total_plots ==0)		/*nothing to refresh if no plots*/
	return;
		/*save active plot*/
/*fprintf(stderr, "just before ref to can_info->active_plot.\n");*/
  active_plot = can_info->active_plot;  
  display_active_plot(active_plot+1);
  canvas = can_info->canvas;
/*fprintf(stderr, "just before redraw proc.\n");*/
  
  redraw_all_proc(canvas, can_info->xvwin, xv_get(canvas, XV_DISPLAY), can_info->win, NULL); 
		/*re-set active plot*/

/*fprintf(stderr, "after redraw.\n");*/
/* 26/3/96: obs: crash when refresh a window with nothing in it. OK to here, crashes in set_active plot*/
  set_active_plot(active_plot);
}


void kill_graf_proc(item, event)
     Panel_item item;
     Event *event;
{
  int win;
  
  win = xv_get(item, PANEL_CLIENT_DATA);
  
  kill_win_proc(win+1); 
}



void canvas_event_proc(xvwindow, event)
     Xv_Window xvwindow;
     Event *event;
{
  /* canvas event handler:
     always get the window number first and set active window to this num.
     if mouse event, check the current mode, then execute the proc.
     */

  int xloc, yloc;
  Canvas canvas;
  int can_num;
  canvasinfo *can_info;
  plotarray *data;
  int active_plot;
  extern void do_line_plot(), do_mouse_mu(), do_dist();
   
  
  if(active_window == -1)		/*null event, don't go through here*/
  {
	return;
  }
  canvas = (Canvas)xv_get(xvwindow, CANVAS_PAINT_CANVAS_WINDOW);
  can_num = xv_get(canvas, XV_KEY_DATA, CAN_NUM);
  set_active_window(can_num);

  can_info = wininfo.canvases[active_window];    
  active_plot = can_info->active_plot;
  display_active_plot(active_plot+1);
  
  switch(event_action(event))
    {
      
    case LOC_MOVE:
      
      if (can_info->total_plots == 0) break;
      data = can_info->plots[active_plot];

      xloc = event_x(event);
      yloc = event_y(event);

      /* draw rubber band line for line plot */
      if (data->mouse == 1 && data->p1 == 1)
	{
	  if (data->p2 == 1)
	    {
	      /* remove previous line */
	      XDrawLine((Display *)xv_get(canvas, XV_DISPLAY) , can_info->win, gcrubber, data->x1, data->y1, data->x2, data->y2);
	    }
	  /* draw new line */
	  XDrawLine((Display *)xv_get(canvas, XV_DISPLAY) , can_info->win, gcrubber, data->x1, data->y1, xloc, yloc);
	  data->x2 = xloc;
	  data->y2 = yloc;
	  data->p2 = 1;
	}
      
      break;
	 

    case LOC_DRAG:      
      break;


    case ACTION_MENU:  /* ACTION_MENU is the right button*/
		/* we use to pick points or to indicate done selecting parameters*/ 
      if (!event_is_up(event)) break;
      if (can_info->total_plots == 0) break;

      data = can_info->plots[active_plot];
      xloc = event_x(event);
      yloc = event_y(event);

      /* default case (mouse == 0) */
      if (data->mouse == 0 )
	  print_xy(xloc, yloc);

      if (data->mouse == 4 )
       {
	 /* commit zoom */
	 zoom();
	 data->zp1 = data->zp2 = 0;
       }
      
      if (data->mouse != 0)
	{
	  sprintf(msg, "Done.\n");
	  print_msg(msg);
	  xv_set(xv_get(can_info->canvas, XV_KEY_DATA, GRAF_FRAME),
	  FRAME_LEFT_FOOTER, "Normal Mode: left & middle buttons pick row numbers, right button gives x-y position", NULL);
	  data->x1 = 0;
	  data->y1 = 0;	  
	  data->x2 = 0;
	  data->y2 = 0;	
	  data->p1 = 0;
	  data->p2 = 0;  
	  data->zp1 = 0;
	  data->zp2 = 0;
	  data->mouse = 0;
	}
      break;
      
      
    case ACTION_ADJUST:   /* middle button */
      if (!event_is_up(event)) break;
      if (can_info->total_plots == 0) break;
      
      data = can_info->plots[active_plot];

      xloc = event_x(event);
      yloc = event_y(event);
	 
      if (data->mouse == 0 )
	{
	  print_xyrow(xloc, yloc, 1);
	  break;
	}
            
      if ((data->mouse == 1 || data->mouse == 3 || data->mouse == 4) && data->p1==0)
	{
	  /* not enough points picked */
	  sprintf(msg, "Pick 1st point with left button first.\n");
	  print_msg(msg);
	  break;
	}
	
      if (data->mouse == 1 && data->p1 == 1)
	{
	  /* get the 2nd point */
	  sprintf(msg, "2nd point picked\n");
	  print_msg(msg);
	  data->x2 = xloc;
	  data->y2 = yloc;
	  data->p2 = 1;

	  /* commit line plot */
	  draw_crosshair(xloc, yloc);
	  XDrawLine((Display *)xv_get(canvas, XV_DISPLAY) , can_info->win, gcrubber, data->x1, data->y1, data->x2, data->y2);
	  do_line_plot();
	  
	  /* reset for next line plot */
	  data->p1 = 0;
	  data->p2 = 0;
	  data->x1 = data->x2 = data->y1 = data->y2 = 0;
	  break;
	}

      if (data->mouse == 2)
	{
	  data->x1 = xloc;
	  data->y1 = yloc;
	  data->p1 = 1;
	  do_mouse_mu();
	  break;
	}

      if (data->mouse == 3 )
	{
	  /* get the 2nd point */
	  sprintf(msg, "2nd point picked\n");
	  print_msg(msg);
	  data->x2 = xloc;
	  data->y2 = yloc;
	  data->p2 = 1;
	  
	  /* commit distance */
	  draw_crosshair(xloc, yloc);
	  do_dist();
	  data->p1 = 0;
	  data->p2 = 0;
	  data->x1 = data->x2 = data->y1 = data->y2 = 0;
	  break;
	}

      if (data->mouse == 4 )
	{
	  /* get the 2nd point */
	  sprintf(msg, "2nd point picked\n");
	  print_msg(msg);
	  data->x2 = xloc;
	  data->y2 = yloc;
	  data->p2 = 1;
	  
	  /* 2nd corner for zoom */
	  zoom_get_pt(xloc, yloc, 2);
	  break;
	}
      break;
  
  

    case ACTION_SELECT:   /* left button */
      if (!event_is_up(event)) break;
      if (can_info->total_plots == 0) break;
	
      data = can_info->plots[active_plot];
      
      xloc = event_x(event);
      yloc = event_y(event);

      if (data->mouse == 0 )
      {
	  /* print row num on info panel only */
	  print_xyrow(xloc, yloc, 0);
	  break;
      }
      else
      {

	  if (data->mouse == 1 && data->p2 == 1 && data->p1 == 1)
	    {
	      /* do nothing */
	      break;
	    }
	  
	  data->x1 = xloc;
	  data->y1 = yloc;
	  data->p1 = 1;
	  
	  if (data->mouse == 2 )
	    {
	      do_mouse_mu();
	      break;
	    }
	  
	  if (data->mouse == 4 )
	    {
	      zoom_get_pt(xloc, yloc, 1);
	      sprintf(msg, "1st point picked\n");
	      print_msg(msg);
	      break;
	    }
	  
	  draw_crosshair(xloc, yloc);
	  /* get the start point */
	  sprintf(msg, "1st point picked\n");
	  print_msg(msg);
      }
      break;
      
  
    /*case WIN_RESIZE:
	setup_canvas();
fprintf(stderr,"past setup canvas, before redraw in canvas_event_proc\n");
  	redraw_all_proc(canvas, can_info->xvwin, xv_get(canvas, XV_DISPLAY), can_info->win, NULL); 
fprintf(stderr,"after redraw in WIN_RESIZE\n");
      break;*/  /* if set to return, need to click on the canvas to display */ 
      
    default:
      sprintf(msg, "NULL event\n");
      break;
     /* return;*/
    }
}     

void resize_proc(canvas, width, height)
 Canvas canvas;
 int width;
 int height;
{
  Display *dpy;
  Window win;
  canvasinfo *can_info;
/*fprintf(stderr,"in resize proc\n");*/
  setup_canvas();
    
  dpy = (Display *)xv_get(canvas, XV_DISPLAY);
  win = (Window)xv_get(canvas_paint_window(canvas), XV_XID);

  XClearWindow(dpy, win);

  can_info = wininfo.canvases[active_window];
  redraw_all_proc(canvas, can_info->xvwin, xv_get(canvas, XV_DISPLAY), win, NULL); 

}


void initialize(argc, argv)
     int argc;
     char *argv[];
{
  int i;
  
  plot_error = -1;
  
  max_col = 17;			/*start with 17, which is MAX limit*/
  max_row = 10000;
  for (i = 0; i < max_col; ++i)
    darray[i] = (float *)malloc((unsigned)(max_row*sizeof(float)));
    /*darray[i] = (float *)calloc((unsigned)max_row, (unsigned)4);*/
  
  arrayx = (float *)malloc((unsigned)(max_row*sizeof(float)));
  arrayy = (float *)malloc((unsigned)(max_row*sizeof(float)));
  /*arrayx = (float *)calloc((unsigned)max_row, (unsigned)4);
  arrayy = (float *)calloc((unsigned)max_row, (unsigned)4);*/
  
  for (i=0; i<MAX_COL; ++i) null_col(i);

   if (argc > 1)
    {
      if (argc == 2)
	{
	  if (access(argv[1], 4) == 0)
	    {
	      strcpy(default_path, argv[1]);
	      fprintf(stderr, "Pathname for DOIT files is %s\n", default_path);
	      if (default_path[strlen(default_path)-1] != '/')
		strcat(default_path, "/");
	    }
	  else
	    fprintf(stderr, "Inaccessible pathname \n%s IGNORED\n", argv[1]);
	}
      else
	{
	  fprintf(stderr, "Don't understand argument list\n");
	  fprintf(stderr, "EXPECTED: look default_path\n");
	  exit(-1);
	}
    }
}
