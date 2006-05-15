#include <math.h>
#include <string.h>

#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>

#include <cmds1.h>
#include <config.h>
#include <drawwin.h>
#include <global.h>
#include <menus.h>
#include <messages.h>

extern int active_window;
extern int old_active_window;
extern int total_windows;
extern int action;

/* extern void print_msg(); */ 
extern char msg[MSG_LENGTH];

extern Frame main_frame;
Frame clr_plots_frame;
static int chosen[MAX_PLOTS];
static Panel_item p0, p1, p2, p3, p4, p5, p6, p7, p8, p9;
static Panel_item message;
static char plabel[MAX_PLOTS][256];



void clr_plots_notify_proc(item, event)
     Panel_item item;
     Event *event;
{
  set_active_window(xv_get(item, PANEL_CLIENT_DATA));
}



/*******************  clear all **************************************/

void clr_all_plots_proc(menu, menu_item)
     Menu menu;     
     Menu_item menu_item;
{
  canvasinfo *can_info;
  int i;
  
  can_info = wininfo.canvases[active_window];

  for (i=0; i<MAX_PLOTS; i++)
    {
      if (can_info->alive_plots[i] == 1)
	{
	  free (can_info->plots[i]);
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




/************************* clear selected plots ****************************/

/*---------------------- setup the menu ----------------------------*/

void clr_plots_proc(menu, menu_item)
     Menu menu;
     Menu_item menu_item;
{

  Panel panel;
  canvasinfo *can_info;
  int i;
  char labels[MAX_PLOTS][256];
  void clr_plots_done_proc(), clr_plots_cancel_proc();
  char msg_label[256];
   
  /*  can_num = xv_get(menu, MENU_CLIENT_DATA);
  set_active_window(can_num); */

  can_info = wininfo.canvases[active_window];

  sprintf(msg_label, "Plots in window #%d to clear:", active_window+1);

  /* create the labels */    
  for (i=0; i<MAX_PLOTS; i++)
    {
      if (can_info->alive_plots[i] == 1)
	{
	  sprintf(labels[i], "%d. %s  vs  %s", 
		  i+1,
		  head.ch[can_info->plots[i]->col_x].name, 
		  head.ch[can_info->plots[i]->col_y].name);
	}
      else
	{
	  sprintf(labels[i], "No Plot");
	}
    }
 

  if (!clr_plots_frame)
    {
      /* the frame does not exist yet; create it now */
      /* can be used by all the windows */

      clr_plots_frame = (Frame)xv_create(main_frame,
					 FRAME, 
					 FRAME_LABEL, "Clear Plots",
					 NULL);

      panel = (Panel) xv_create(clr_plots_frame, PANEL,   
			       PANEL_LAYOUT, PANEL_VERTICAL,
			       NULL);

      message = (Panel_item) xv_create(panel, PANEL_MESSAGE, 
				       PANEL_LABEL_BOLD, TRUE,
				       PANEL_LABEL_STRING, msg_label,
				       XV_X, xv_col(panel, 3),
				       NULL);      

      /* set the label for each item */
      p0 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[0], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 1),
				  NULL);
      p1 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[1], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 2),
				  NULL);
      p2 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[2], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 3),
				  NULL);
      p3 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[3], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 4),
				  NULL);
      p4 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[4], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 5),
				  NULL);
      p5 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[5], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 6),
				  NULL);
      p6 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[6], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 7),
				  NULL);
      p7 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[7], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 8),
				  NULL);
      p8 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[8], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 9),
				  NULL);
      p9 = (Panel_item) xv_create(panel, PANEL_CHECK_BOX,
				  PANEL_CHOICE_STRINGS, labels[9], NULL,
				  PANEL_VALUE_X, xv_col(panel, 3),
				  XV_Y, xv_row(panel, 10),
				  NULL);

      (void) xv_create(panel, PANEL_BUTTON,
		       PANEL_LABEL_STRING, "Done",
		       PANEL_NOTIFY_PROC, clr_plots_done_proc,
		       XV_X, xv_col(panel, 4),
		       XV_Y, xv_row(panel, 11),
		  NULL);
      
      (void) xv_create(panel, PANEL_BUTTON,
		       PANEL_LABEL_STRING, "Cancel",
		       PANEL_NOTIFY_PROC, clr_plots_cancel_proc,
		       XV_X, xv_col(panel, 20),
		       XV_Y, xv_row(panel, 11),
		  NULL);
      
      window_fit(panel);
      window_fit(clr_plots_frame);
    }
  
  else
    {
      /* set all the labels for the items */
      xv_set(message, PANEL_LABEL_STRING, msg_label, NULL);
      
      xv_set(p0, PANEL_CHOICE_STRINGS, labels[0], NULL, NULL);
      xv_set(p1, PANEL_CHOICE_STRINGS, labels[1], NULL, NULL);
      xv_set(p2, PANEL_CHOICE_STRINGS, labels[2], NULL, NULL);
      xv_set(p3, PANEL_CHOICE_STRINGS, labels[3], NULL, NULL);
      xv_set(p4, PANEL_CHOICE_STRINGS, labels[4], NULL, NULL);
      xv_set(p5, PANEL_CHOICE_STRINGS, labels[5], NULL, NULL);
      xv_set(p6, PANEL_CHOICE_STRINGS, labels[6], NULL, NULL);
      xv_set(p7, PANEL_CHOICE_STRINGS, labels[7], NULL, NULL);
      xv_set(p8, PANEL_CHOICE_STRINGS, labels[8], NULL, NULL);
      xv_set(p9, PANEL_CHOICE_STRINGS, labels[9], NULL, NULL);
    
    }   

  /* set items inactive if there's no corresponding plot */
  if (can_info->alive_plots[0] == 0) xv_set(p0, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p0, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[1] == 0) xv_set(p1, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p1, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[2] == 0) xv_set(p2, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p2, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[3] == 0) xv_set(p3, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p3, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[4] == 0) xv_set(p4, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p4, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[5] == 0) xv_set(p5, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p5, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[6] == 0) xv_set(p6, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p6, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[7] == 0) xv_set(p7, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p7, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[8] == 0) xv_set(p8, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p8, PANEL_INACTIVE, FALSE, NULL);
  if (can_info->alive_plots[9] == 0) xv_set(p9, PANEL_INACTIVE, TRUE, NULL);
  else  xv_set(p9, PANEL_INACTIVE, FALSE, NULL);

  xv_set(clr_plots_frame, XV_SHOW, TRUE, NULL);
}  


void clr_plots_done_proc(item, event)
     Panel_item item;
     Event *event;
{
  /* get the checked items, and clear the corresponding plot */

  static char tmp_buf[20];
  int i,j, hole_index[MAX_PLOTS], n_holes; 
  canvasinfo *can_info;
   
  can_info =  wininfo.canvases[active_window];  

  /* get all the checked items */
  chosen[0] = (int)xv_get(p0, PANEL_VALUE);
  chosen[1] = (int)xv_get(p1, PANEL_VALUE);
  chosen[2] = (int)xv_get(p2, PANEL_VALUE);
  chosen[3] = (int)xv_get(p3, PANEL_VALUE);
  chosen[4] = (int)xv_get(p4, PANEL_VALUE);
  chosen[5] = (int)xv_get(p5, PANEL_VALUE);
  chosen[6] = (int)xv_get(p6, PANEL_VALUE);
  chosen[7] = (int)xv_get(p7, PANEL_VALUE);
  chosen[8] = (int)xv_get(p8, PANEL_VALUE);
  chosen[9] = (int)xv_get(p9, PANEL_VALUE);
  

  /* reset all the items to unchecked */
  xv_set(p0, PANEL_VALUE, 0, NULL);
  xv_set(p1, PANEL_VALUE, 0, NULL);
  xv_set(p2, PANEL_VALUE, 0, NULL);
  xv_set(p3, PANEL_VALUE, 0, NULL);
  xv_set(p4, PANEL_VALUE, 0, NULL);
  xv_set(p5, PANEL_VALUE, 0, NULL);
  xv_set(p6, PANEL_VALUE, 0, NULL);
  xv_set(p7, PANEL_VALUE, 0, NULL);
  xv_set(p8, PANEL_VALUE, 0, NULL);
  xv_set(p9, PANEL_VALUE, 0, NULL);

  /* kill selected plots */
  sprintf(msg, "Clear plots: ");

  n_holes=0;				/*number of plots being axed*/
  for (i=0; i<MAX_PLOTS; i++)
  {
      if(chosen[i] && can_info->alive_plots[i])
	    {					/*reset active plot if axing currently active plot. If it's 0, just leave it as 0 */
	      if ((can_info->active_plot == i) && (can_info->active_plot != 0))	
    			can_info->active_plot--;
	      can_info->alive_plots[i] = 0;
  	      /*free(can_info->plots[i]);*/            /*free space */
	      sprintf(tmp_buf, " %d ", i+1);
	      strcat(msg, tmp_buf);
	      n_holes++;
	    }
  }
  strcat(msg, "\n");
  print_msg(msg);

  /*reshuffle the list, so that, say, removing plot #2 from a list of 4
results in #3 going to #2 and #4 going to #3. This will require repointing the
plot data pointers etc.*/

  for(j=i=0;i<MAX_PLOTS;i++)    /*first set up an index for holes (actually non-holes)*/
  {
        hole_index[i]=0;                /*default*/
        if (can_info->alive_plots[i])   /*if alive, numbers will be the same */
                hole_index[j++] = i;
  }

  can_info->total_plots -= n_holes;

  for(j=i=0;i<can_info->total_plots;i++)                /*shuffle list*/
  {
                can_info->alive_plots[i] = 1; /*set it live*/
                can_info->plots[i] = can_info->plots[hole_index[i]];  /*repoint pointer to data*/
  }

  for(i=can_info->total_plots;i< MAX_PLOTS;i++) /* all the rest are dead*/
	can_info->alive_plots[i] = 0;

  xv_set(clr_plots_frame, XV_SHOW, FALSE, NULL);

  clear_canvas_proc(can_info->canvas);
 
  if(can_info->total_plots ==0)
        can_info->active_plot = -1;
  else  
  {  
        j=can_info->active_plot;                /*save active plot*/
        for (i=0; i<MAX_PLOTS; i++)             /*re-draw */
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
}


void clr_plots_cancel_proc(item, event)
     Panel_item item;
     Event *event;     
{
  /* resets all the items to unchecked state */
  xv_set(p0, PANEL_VALUE, 0, NULL);
  xv_set(p1, PANEL_VALUE, 0, NULL);
  xv_set(p2, PANEL_VALUE, 0, NULL);
  xv_set(p3, PANEL_VALUE, 0, NULL);
  xv_set(p4, PANEL_VALUE, 0, NULL);
  xv_set(p5, PANEL_VALUE, 0, NULL);
  xv_set(p6, PANEL_VALUE, 0, NULL);
  xv_set(p7, PANEL_VALUE, 0, NULL);
  xv_set(p8, PANEL_VALUE, 0, NULL);
  xv_set(p9, PANEL_VALUE, 0, NULL);

  /* unmaps the window */
  xv_set(clr_plots_frame, XV_SHOW, FALSE, NULL);
}



/**************************** activate plot ***************************/

void create_act_plot_menu_proc(item, event)
     Panel_item item;
     Event *event;     
{
  /* create the items */

  int i;
  canvasinfo *can_info;
  Menu_item mi;
  Menu menu=(Menu) xv_get(item, PANEL_ITEM_MENU);
  int can_num;
  void get_act_item_proc();

  can_num = (int) xv_get(item, PANEL_CLIENT_DATA);
  set_active_window(can_num);
  can_info = wininfo.canvases[active_window];

  for (i=MAX_PLOTS; i>0; i--)
    {
      if (xv_get(menu, MENU_NTH_ITEM, i))
	{
	  xv_set(menu, MENU_REMOVE, i, NULL);
	  xv_destroy(xv_get(menu, MENU_NTH_ITEM, i));
	}
    }
   
  for(i=0; i<MAX_PLOTS; i++)
    {
      if (can_info->alive_plots[i])
	{
	  sprintf(plabel[i], "%d. %s  vs  %s", i+1,
		  head.ch[can_info->plots[i]->col_x].name, 
		  head.ch[can_info->plots[i]->col_y].name);

	  mi = (Menu_item)xv_create(XV_NULL, MENUITEM,
				    MENU_STRING, plabel[i], 
				    MENU_NOTIFY_PROC, get_act_item_proc,
				    MENU_RELEASE,
				    NULL);			      
	  
	  xv_set(menu, MENU_APPEND_ITEM, mi, NULL);
	}
    }
}


void get_act_item_proc(menu, item)
     Menu menu;
     Menu_item item;
{
  /* get the selected item and set it active */

  int i;
  canvasinfo *can_info;
  Canvas canvas;
  Xv_Window canvas_xv_window;

  /* no need to set active window, since it was already activated when pressing the menu button */
  
  can_info = wininfo.canvases[active_window];
  canvas = can_info->canvas;
  canvas_xv_window = canvas_paint_window(canvas);

  for (i=0; i<MAX_PLOTS; i++)
    if (xv_get(menu, MENU_NTH_ITEM, i+1) == item)
      if (can_info->alive_plots[i])
	set_active_plot(i);
}

void clr_mult_plots_proc(intar2)
int *intar2;
{
  static char cbuf[20];
  int i,j, hole_index[MAX_PLOTS], n_holes; 
  canvasinfo *can_info;
   
  can_info =  wininfo.canvases[active_window];  

  /* kill selected plots */
  sprintf(msg, "Clear plots: ");

  for(i=0; i<MAX_PLOTS;i++)
  {
   if((intar2[i]==1 && can_info->alive_plots[i] == 0) || can_info->active_plot == -1 )
   {
      	sprintf(msg, "Error, plot #%d does not exist, it can't be cleared.\n",i+1);
      	print_msg(msg);
      	top();
      	action = MAIN;
      	return;
   }
   else if(intar2[i]==1 && can_info->alive_plots[i] == 1)
   {
	free(can_info->plots[i]); /*free space */
   }
  }

  n_holes=0;/*number of plots being axed*/
  for (i=0; i<MAX_PLOTS; i++)
  {
      if((intar2[i] == 1) && (can_info->alive_plots[i]==1))
	    {					/*reset active plot if axing currently active plot. If it's 0, just leave it as 0 */
	      if ((can_info->active_plot == i) && (can_info->active_plot != 0))	
    	      can_info->active_plot--;
	      can_info->alive_plots[i] = 0;
	      sprintf(cbuf, " %d ", i+1);
	      strcat(msg, cbuf);
	      n_holes++;
	    }
  }
  strcat(msg, "\n");
  print_msg(msg);

  /*reshuffle the list, so that, say, removing plot #2 from a list of 4 results in #3 going to #2 and #4 going to #3. This will require repointing the plot data pointers etc.*/

  for(j=i=0;i<MAX_PLOTS;i++)    /*first set up an index for holes (actually non-holes)*/
  {
        hole_index[i]=0;                /*default*/
        if (can_info->alive_plots[i])   /*if alive, numbers will be the same */
                hole_index[j++] = i;
  }

  can_info->total_plots -= n_holes;

  for(j=i=0;i<can_info->total_plots;i++) /*shuffle list*/
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
        j=can_info->active_plot;               /*save active plot*/
        for (i=0; i<MAX_PLOTS; i++)            /*re-draw */
        {
                if (can_info->alive_plots[i] == 1)
                {
                        can_info->active_plot = i;
                        redraw_proc(can_info->canvas, can_info->xvwin, xv_get(can_info->canvas, XV_DISPLAY), can_info->win, NULL);
                }
        }
        if(can_info->alive_plots[j] == 0) /*active plot got axed, use default*/
                can_info->active_plot = can_info->total_plots-1;
        else
                can_info->active_plot = j;
  }
  set_active_plot(can_info->active_plot);
}
