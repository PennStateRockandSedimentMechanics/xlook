/* cjm 21.1.08:  added a new line to plot window info, for xlook compile version and to
deal with wrapping problem caused by OS 10.5*/

#include <gtk/gtk.h>
#include <math.h>
#include <assert.h>

#include "config.h"
#include "global.h"
#include "messages.h"
#include "plot_window.h"
#include "ui.h"
#include "cmds1.h" // for kill_win_proc

extern char plot_cmd[256]; // FIXME: move to globals.h

#define MAKE_CLEAR_PROC_DATA(ci, pl) (((ci)<<4) | (pl))
#define GET_CANVAS_INDEX(d) (d>>4)
#define GET_PLOT_NUMBER(d) (d&0xf)
#define CLEAR_ALL_PLOTS_CONSTANT (0xf) // NOTE: If we have more than 10 plots, this will need to change.

typedef enum {
	PLOT_LABEL_LEFT_FOOTER= 0,
	LABEL_X,
	LABEL_Y,
	LABEL_ROW_NUMBER,
	LABEL_PLOT_ROWS,
	NUMBER_OF_LABELS
} PlotLabelID;

enum
{
  PLOT_TREE_COLUMN_ACTIVE= 0,
  PLOT_TREE_COLUMN_NAME,
  NUMBER_OF_PLOT_TREE_COLUMNS
} ;


static int get_new_window_num();
static void adjust_canvas_size(int index);
static int window_index_from_window(GtkWindow *window);
static void set_left_footer_message(GtkWindow *parent, char *txt);
static void set_plot_label_message(GtkWindow *parent, PlotLabelID id, char *txt);
static void erase_rectangle(GtkWidget *widget, int x, int y, int width, int height);
static void draw_line(GtkWidget *widget, int x0, int y0, int x1, int y1);
static void label_type1(GtkWidget *drawingArea, canvasinfo *can_info);
static void label_type0(GtkWidget *drawingArea, canvasinfo *can_info);
static int get_row_number(canvasinfo *can_info, int nrow, float x1, float y1);
static void invalidate_widget_drawing(GtkWidget *widget);
static canvasinfo *canvas_info_for_widget(GtkWidget *widget);
static void adjust_canvas_size(int index);
static void rebuild_active_plot_combo_list(GtkComboBox *widget);
static void invalidate_plot(GtkWindow *window);
static gint clear_Plots_PopupHandler (GtkWidget *widget, GdkEvent *event, gpointer user_data);
static gint mouse_and_zoom_PopupHandler (GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void adjust_clear_plots_menu(GtkWidget *widget, GtkMenu *menu);
static void on_clear_plot_menu_item(GtkObject *object, gpointer user_data);
static void on_zoom_menu_item(GtkObject *object, gpointer user_data);
static void on_mouse_menu_item(GtkObject *object, gpointer user_data);

enum {
	_zoom_item_zoom_and_clear= 0,
	_zoom_item_zoom,
	_zoom_item_zoom_new_window,
	NUMBER_OF_ZOOM_ITEMS
};

static const char *zoom_menu_labels[]= { "Zoom and Clear", "Zoom", "Zoom New Window" };

enum {
	_mouse_item_line_plot = 0,
	_mouse_item_vertical_line,
	_mouse_item_distance,
	NUMBER_OF_MOUSE_ITEMS
};

static const char *mouse_menu_labels[]= { "Line Plot", "Vertical Line", "Distance" };

//extern Frame main_frame;
//Xv_Cursor xhair_cursor;

short canvas0_image[] = {
#include "canvas.ico"
};
//Server_image c0image;
//Icon cstate;

struct plot_window
{
	GtkWindow *window;
};


struct plot_window *create_plot_window()
{ 
	struct plot_window *result= NULL;
	int win_num = get_new_window_num();

	if (win_num == -1)
	{
		sprintf(msg, "Reached limit of 10 windows. Ignored!\n");
		print_msg(msg);
		ui_globals.total_windows--;
		return NULL;
	} else {
		int ii;
		GtkBuilder *builder = gtk_builder_new ();
		gtk_builder_add_from_file (builder, "plot_window.glade", NULL);

		// load the rest of them...
		GtkWidget *window = GTK_WIDGET (gtk_builder_get_object (builder, "plotWindow"));
		gtk_builder_connect_signals (builder, NULL);
		g_object_unref (G_OBJECT (builder));

		// set the window title...
		sprintf(msg, "Plot Window %d", win_num+1);
		gtk_window_set_title(GTK_WINDOW(window), msg);

		// create the canvasinfo (this is sorta silly)
		canvasinfo *can_info = (canvasinfo *) malloc(sizeof(canvasinfo));
		if (can_info == NULL)
		{
			print_msg("Memory allocation error. Window cannot be created.");
			return NULL;
		}

		ui_globals.old_active_window = ui_globals.active_window;
		ui_globals.active_window = win_num;
		fprintf(stderr, "Window pointer is %p\n", window);
		can_info->canvas_num = ui_globals.active_window;
		can_info->total_plots = 0;
		can_info->active_plot = -1;

		for (ii=0; ii<10; ii++)
		{
			can_info->alive_plots[ii] = 0;
		}

		// must do this before it is realized.
		result= malloc(sizeof(struct plot_window));
		if(result)
		{
			result->window= GTK_WINDOW(window);
		}
		
		// store it.
		can_info->plot_window= result;

		display_active_window(ui_globals.active_window+1);
		display_active_plot(0);

		wininfo.windows[ui_globals.active_window] = 1;
		wininfo.canvases[ui_globals.active_window] = can_info;

		
		/* connect our handler which will popup the menu */
{
	GtkWidget *menu = gtk_menu_new();

	GtkButton *button= GTK_BUTTON(lookup_widget_by_name(GTK_WIDGET(window), "btn_ClearPlots"));
    gtk_signal_connect (GTK_OBJECT (button), "event", GTK_SIGNAL_FUNC (clear_Plots_PopupHandler), GTK_OBJECT (menu));
}
{
	GtkWidget *menu = gtk_menu_new();
	int ii;
	
	// wanted to build this in glade, but no joy.
	// add all the items back...
	assert(ARRAY_SIZE(zoom_menu_labels)==NUMBER_OF_ZOOM_ITEMS);
	for(ii=0; ii<NUMBER_OF_ZOOM_ITEMS; ii++)
	{
		GtkWidget *menu_item;

		/* Create a new menu-item with a name... */
		menu_item = gtk_menu_item_new_with_label (zoom_menu_labels[ii]);
		gtk_menu_append (GTK_MENU (menu), menu_item);
		gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (on_zoom_menu_item), GINT_TO_POINTER (MAKE_CLEAR_PROC_DATA(can_info->canvas_num, ii)));
		gtk_widget_show (menu_item);
	}

	GtkButton *button= GTK_BUTTON(lookup_widget_by_name(GTK_WIDGET(window), "btn_Zoom"));
    gtk_signal_connect (GTK_OBJECT (button), "event", GTK_SIGNAL_FUNC (mouse_and_zoom_PopupHandler), GTK_OBJECT (menu));
}

{
	GtkWidget *menu = gtk_menu_new();
	int ii;
	
	// wanted to build this in glade, but no joy.
	// add all the items back...
	assert(ARRAY_SIZE(mouse_menu_labels)==NUMBER_OF_MOUSE_ITEMS);
	for(ii=0; ii<NUMBER_OF_MOUSE_ITEMS; ii++)
	{
		GtkWidget *menu_item;

		/* Create a new menu-item with a name... */
		menu_item = gtk_menu_item_new_with_label (mouse_menu_labels[ii]);
		gtk_menu_append (GTK_MENU (menu), menu_item);
		gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (on_mouse_menu_item), GINT_TO_POINTER (MAKE_CLEAR_PROC_DATA(can_info->canvas_num, ii)));
		gtk_widget_show (menu_item);
	}

	GtkButton *button= GTK_BUTTON(lookup_widget_by_name(GTK_WIDGET(window), "btn_Mouse"));
    gtk_signal_connect (GTK_OBJECT (button), "event", GTK_SIGNAL_FUNC (mouse_and_zoom_PopupHandler), GTK_OBJECT (menu));
}


		// okay; the "proper" way to do this would be to store wininfo off the GtkWindow, and iterate the window manager to figure out what we have up.
		// sorta like this:
		//g_object_set_data_full (G_OBJECT (event_box), "wininfo", g_strdup(user_name), (GDestroyNotify) g_free); (replacing allocater and free'r)
		// however, at this point, I'm going to do it dirty.
		adjust_canvas_size(ui_globals.active_window);

		gtk_widget_show (window);
	}

	return result;
}

void bring_plot_window_to_front(struct plot_window *pw)
{
	assert(pw);
	assert(pw->window);
	
	// present it (bring to front)
	gtk_window_present(GTK_WINDOW(pw->window));
}

void set_active_plot_in_window(struct plot_window *pw, int i)
{
fprintf(stderr, "set active plot in window to %d\n", i);
	assert(pw);
	assert(pw->window);
	canvasinfo *can_info= canvas_info_for_widget(GTK_WIDGET(pw->window));

	/* set the active plot */
	can_info->active_plot = i;

	/* invalidate the drawing area */
	invalidate_plot(pw->window);

	if(i < 0)			/*no active plot, no plots*/
	{
		display_active_plot(can_info->active_plot);
	}
	else if (can_info->alive_plots[i] == 0)
	{
		sprintf(msg, "Plot does not exist, it can't be set active, error!\n");
		print_msg(msg);
		top();
		ui_globals.action = MAIN;
	}
	else
	{
		display_active_plot(can_info->active_plot+1);
	}
	
	// rebuild the combo list...
	GtkComboBox *activeCombo= GTK_COMBO_BOX(lookup_widget_by_name(GTK_WIDGET(pw->window), "comboboxActivePlot"));
	rebuild_active_plot_combo_list(activeCombo);
}

void remove_plot_in_window(struct plot_window *pw, int pn)
{
	assert(pw);
	assert(pw->window);
	canvasinfo *can_info= canvas_info_for_widget(GTK_WIDGET(pw->window));
	int i, j, hole_index[MAX_PLOTS];
  
	pn--;					/*internal number starts at zero, instead of 1*/
  
	if (can_info->alive_plots[pn] == 0 || can_info->active_plot == -1 )
	{
		sprintf(msg, "Plot does not exist or there's no active plot (error!).\n");
		print_msg(msg);
		top();
		ui_globals.action = MAIN;
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

	// not sure why it's done more than once.
	invalidate_plot(pw->window);

	if(can_info->total_plots ==0)
	{
		can_info->active_plot = -1;
	}
	else
	{
		j=can_info->active_plot;		/*save active plot*/
		for (i=0; i<MAX_PLOTS; i++)		/*re-draw */
		{
			if (can_info->alive_plots[i] == 1)
			{
				can_info->active_plot = i;
				invalidate_plot(pw->window);
			}
		}
		
		if(can_info->alive_plots[j] == 0)     /*active plot got axed, use default*/
		{
			can_info->active_plot = can_info->total_plots-1;
		}
		else
		{
			can_info->active_plot = j;
		}
	}

	set_active_plot_in_window(can_info->plot_window, can_info->active_plot);

	ui_globals.action = MAIN;
	top();
}

void kill_plot_window(struct plot_window *pw)
{
	assert(pw);
	assert(pw->window);

// fprintf(stderr, "in kill plot window\n");
	/* kill the xwindow */
	gtk_widget_destroy(GTK_WIDGET(pw->window));
}  

void invalidate_plot_window(struct plot_window *pw)
{
	assert(pw);
	assert(pw->window);
	invalidate_plot(pw->window);
}


void clear_multiple_plots(struct plot_window *pw, int *intar2)
{
	canvasinfo *can_info= canvas_info_for_widget(GTK_WIDGET(pw->window));
	char cbuf[20];
	int i,j, hole_index[MAX_PLOTS], n_holes; 
   
	/* kill selected plots */
	sprintf(msg, "Clear plots: ");
	for(i=0; i<MAX_PLOTS;i++)
	{
		if((intar2[i]==1 && can_info->alive_plots[i] == 0) || can_info->active_plot == -1 )
		{
			sprintf(msg, "Error, plot #%d does not exist, it can't be cleared.\n",i+1);
			print_msg(msg);
			top();
			ui_globals.action = MAIN;
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
		{
			/*reset active plot if axing currently active plot. If it's 0, just leave it as 0 */
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
		hole_index[i]= 0;                /*default*/
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
 
	int new_active_plot= -1;
	if(can_info->total_plots ==0)
	{
		new_active_plot= -1;
	}
	else  
	{  
		j= can_info->active_plot;               /*save active plot*/
		for (i=0; i<MAX_PLOTS; i++)            /*re-draw */
		{
			if (can_info->alive_plots[i] == 1)
			{
				new_active_plot= i;
//				can_info->active_plot = i;
//				redraw_proc(can_info->canvas, can_info->xvwin, xv_get(can_info->canvas, XV_DISPLAY), can_info->win, NULL);
			}
		}
		if(can_info->alive_plots[j] == 0) /*active plot got axed, use default*/
			new_active_plot= can_info->total_plots-1;
		else
			new_active_plot = j;
	}
	
	// causes it to redraw as well
	set_active_plot_in_window(can_info->plot_window, new_active_plot);
}

/* --------------------- Everything below here is local (static) code.  If it doesn't have static on the func def, it's because it's a late binding signal from Glade */
// this gets called by gtk_widget_destroy()
void on_plotWindow_destroy(
	GtkObject *object,
	gpointer   user_data)
{
	int i;
	canvasinfo *can_info= canvas_info_for_widget(GTK_WIDGET(object));
	assert(can_info);

//	fprintf(stderr, "In plotWindowDestroy event!: %p %s\n", object, gtk_widget_get_name(object));

	// free all the plots in this window 
	for (i=0; i<MAX_PLOTS; i++)
	{
		if(can_info->alive_plots[i])
			free(can_info->plots[i]);
	}

	// free the canvasinfo for this window 
	free(can_info->plot_window);
	free(can_info);
}

// for this, we use the kill window to make sure global data structures are up to date.
gboolean on_plotWindow_delete_event(
	GtkWidget *widget,
	GdkEvent  *event,
	gpointer   user_data)
{
	int win_index= window_index_from_window(GTK_WINDOW(widget));
//	fprintf(stderr, "In plotWindowDelete event!: %p %s (%d)\n", widget, gtk_widget_get_name(widget), win_index);

	if(win_index != NONE)
	{
		// use the "kilL_win_proc" to make sure everything is cleaned up the way we expect.
		kill_win_proc(win_index+1); // +1 since they are 1 based.
	}	

	// stop processing.
	return TRUE;
}

static void invalidate_plot(GtkWindow *window)
{
	// now redraw the graphic..
	GtkWidget *drawingArea= lookup_widget_by_name(GTK_WIDGET(window), "chartArea");
	invalidate_widget_drawing(drawingArea);
}

static void adjust_canvas_size(int index)
{
	canvasinfo *can_info;
	//  Canvas canvas;
	int start_xaxis, start_yaxis, end_xaxis, end_yaxis;
	int canvas_width, canvas_height;
	int start_x, start_y, end_x, end_y;

	can_info = wininfo.canvases[index];
	GtkWidget *drawingArea= lookup_widget_by_name(GTK_WIDGET(can_info->plot_window->window), "chartArea");
	canvas_width= drawingArea->allocation.width;
	canvas_height= drawingArea->allocation.height;
//	fprintf(stderr, "Width: %d Height: %d\n", canvas_width, canvas_height);

	/* xaxis is from 0.15 to 0.85 canvas width */
	start_xaxis = (int)canvas_width*0.15;
	end_xaxis = canvas_width - (int)canvas_width*0.15;

	/* yaxis is from 0.2 to 0.9 canvas height; 
	  make room for title at the top (4 lines),
	  make room for xaxis labels at the bottom (4 lines)
	  and also 2 pixels of space between lines */
	start_yaxis = canvas_height - 5*ui_globals.tickfontheight - 10;
	end_yaxis = ui_globals.titlefontheight*4 + 10; 

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

static int get_new_window_num()
{
	int i;

	for (i = 0; i <MAXIMUM_NUMBER_OF_WINDOWS; i++)
	{
		if (wininfo.windows[i] == 0)
			return i;
	}
	
	/* reached plots limit.. need to delete one or more plots from window */
	return(-1);
}

/*
Mouse and button press signals to respond to input from the user. (Use gtk_widget_add_events() to enable events you wish to receive.)

The "realize" signal to take any necessary actions when the widget is instantiated on a particular display. (Create GDK resources in response to this signal.)

The "configure_event" signal to take any necessary actions when the widget changes size.

The "expose_event" signal to handle redrawing the contents of the widget.

Expose events are normally delivered when a drawing area first comes onscreen, or when it's covered by another window and then uncovered (exposed). 
You can also force an expose event by adding to the "damage region" of the drawing area's window; gtk_widget_queue_draw_area() and gdk_window_invalidate_rect() 
are equally good ways to do this. You'll then get an expose event for the invalid region.
*/

void on_chartArea_realize(GtkWidget *widget, gpointer user_data)
{
	GdkCursor *cursor= gdk_cursor_new_for_display(gtk_widget_get_display(widget), GDK_CROSSHAIR);
	gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
}

/* Handle the mouse motion in the box */
gboolean on_chartArea_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	GtkWindow *window= parent_gtk_window(widget);

	int win_index= window_index_from_window(window);
	if(win_index != NONE)
	{
		canvasinfo *can_info;
		plotarray *data;
		char xstring[MSG_LENGTH], ystring[MSG_LENGTH], rowstring[MSG_LENGTH];
		float xval, yval;
		float xmatch, ymatch;
		int match_set= FALSE;
		int nrows, row_num;

		int xloc= (int) event->x;
		int yloc= (int) event->y;
	
		can_info = wininfo.canvases[win_index];

		if (can_info->active_plot == -1)
		{
//			sprintf(msg, "There is no plot in this window.\n");
//			print_msg(msg);
			return FALSE;
		}

		data = can_info->plots[can_info->active_plot];
//		draw_crosshair(xloc, yloc);

		xval = (xloc - can_info->start_x)/data->scale_x + data->xmin;
		yval = (can_info->start_y - yloc)/data->scale_y + data->ymin;

		nrows = data->nrows_x;
//fprintf(stderr, "Mouse: %d,%d Strt: %d,%d Scale: %f,%f\n", xloc, yloc, can_info->start_x, can_info->start_y, data->scale_x, data->scale_y);
//fprintf(stderr, "Active Plot: %d XVal: %f YVal: %f NRows: %d\n", can_info->active_plot, xval, yval, nrows);
		row_num = get_row_number(can_info, nrows, xval, yval);
		if (row_num <= nrows && row_num != -1)
		{
			/* get x and y values from data (not screen) */
			xmatch = data->xarray[row_num]; 
			ymatch = data->yarray[row_num]; 
			match_set= TRUE;
		
			row_num = row_num + data->begin; 
			sprintf(rowstring, "Row Number: %d", row_num);
			/* draw crosshair on point */
//			draw_xhair(xval, yval);
		}
		else 
		{
			sprintf(rowstring, "Row Number: None");
			sprintf(msg, "The point picked was not on the curve.\n");
			print_msg(msg);
		}

		set_plot_label_message(window, LABEL_ROW_NUMBER, rowstring);

		/*  print the x and y coord on the panels */
		if(match_set)
		{
			sprintf(xstring, "X: %.5g (Data: %.5g)", xval, xmatch); 
			set_plot_label_message(window, LABEL_X, xstring);
			sprintf(ystring, "Y: %.5g (Data: %.5g)", yval, ymatch);
			set_plot_label_message(window, LABEL_Y, ystring);
		} else {
			sprintf(xstring, "X: %.5g (No Match)", xval); 
			set_plot_label_message(window, LABEL_X, xstring);
			sprintf(ystring, "Y: %.5g  (No Match)", yval);
			set_plot_label_message(window, LABEL_Y, ystring);
		}

		/*  print the x and y coord on the msg window */
		if(match_set)
		{
			sprintf(xstring, "X: %.5g  Y: %.5g", xmatch, ymatch);
		} else {
			sprintf(xstring, "X: %.5g  Y: %.5g", xval, yval);
		}
//		draw_string(widget, xloc+10, yloc, xstring);
		strcat(xstring, "\n");
		print_msg(xstring);
	}

	return FALSE;
}

gboolean on_chartArea_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	// erase to white.
	GtkWindow *window= parent_gtk_window(widget);

	int win_index= window_index_from_window(window);
	if(win_index != NONE)
	{
		int i;
		float x1, x2, y1, y2;
		canvasinfo *can_info;
		int plot_index, plots;
		plotarray *data;
		float xmax, xmin, ymax, ymin;
		int start_x, start_y, end_x, end_y;
		char rows_string[64];    

		can_info = wininfo.canvases[win_index];
		plots = can_info->total_plots;

		// clear it out...
		GdkColor black;
		
		black.red= black.green= black.blue= 0;

		erase_rectangle(widget, 0, 0, widget->allocation.width, widget->allocation.height);
		gdk_gc_set_rgb_fg_color(widget->style->fg_gc[gtk_widget_get_state (widget)],	&black);

		if(plots == 0) 			/*window's been cleared, no plots*/
		{
			sprintf(rows_string, "PLOT ROWS:  ");
			set_plot_label_message(window, LABEL_PLOT_ROWS, rows_string);
			set_left_footer_message(window, "Normal Mode: left & middle buttons pick row numbers, right button gives x-y position");
			return FALSE;
		}
		
		// draw all the active plots.
		for (plot_index=0; plot_index<MAX_PLOTS; plot_index++)
		{
			if(can_info->alive_plots[plot_index] == 1)
			{
				data = can_info->plots[plot_index];

				xmin = data->xmin;
				ymin = data->ymin;
				xmax = data->xmax;
				ymax = data->ymax;

				start_x = can_info->start_x;
				end_x = can_info->end_x;
				start_y = can_info->start_y;
				end_y = can_info->end_y;

				// these two need to be set before the labelling
				data->scale_x = (float)(end_x - start_x)/(xmax - xmin);
				data->scale_y = (float)(start_y - end_y)/(ymax - ymin);

				if(plot_index==can_info->active_plot)
				{
					display_active_plot(plot_index+1);

					/*display row numbers for active plot*/
					sprintf(rows_string, "PLOT ROWS: %d to %d", data->begin, data->end);
					set_plot_label_message(window, LABEL_PLOT_ROWS, rows_string);

					/* print the labels */
					if (data->label_type)
					{
						label_type1(widget, can_info);
					}
					else
					{
						label_type0(widget, can_info);
					}

					/*set footer info. This should be redundant since it only gets set or changed from the panel buttons  --do anyway just to be safe...*/
					switch(can_info->plots[plot_index]->mouse)
					{
						case 0:
							set_left_footer_message(window, "Normal Mode: left & middle buttons pick row numbers, right button gives x-y position");
							break;
						case 1:
							set_left_footer_message(window, "Draw Line Mode: left button picks 1st point, middle picks 2nd, right button quits mode");
							break;
						case 2:
							set_left_footer_message(window, "Vertical Line Mode: left & middle buttons draw vertical line, right button quits mode");
							break;
						case 3:
							set_left_footer_message(window, "Distance Mode: left button picks 1st point, middle picks 2nd, right button quits mode");
							break;
						case 4:
							set_left_footer_message(window, "Zoom Mode: left button picks 1st point, middle picks 2nd, right button commits zoom");
							break;
					}
				}

				/* plot each point  */
				if (can_info->point_plot == 1)
				{
					/* plot the individual points */
					for (i=0; i < data->nrows_x -1; i++)
					{
						x1 = data->xarray[i] - xmin;
						y1 = data->yarray[i] - ymin;

					 	gdk_draw_point(
							widget->window, 
							widget->style->fg_gc[gtk_widget_get_state (widget)],
							start_x + (int)(x1*data->scale_x),
							start_y - (int)(y1*data->scale_y));
					}
				}
				else
				{
					// plot 0 is the measured values.
					// plot 1 is the curve
					/* connect the points */
					for (i=0; i < data->nrows_x -1; i++)
					{
						x1 = data->xarray[i] - xmin;
						x2 = data->xarray[i+1] - xmin;
						y1 = data->yarray[i] - ymin;
						y2 = data->yarray[i+1] - ymin;

					 	draw_line(widget,
							start_x + (int)(x1*data->scale_x),
							start_y - (int)(y1*data->scale_y),
							start_x + (int)(x2*data->scale_x),
							start_y - (int)(y2*data->scale_y));
					}
				}
			}
		}
	} else {
		fprintf(stderr, "something has gone bellyup.\n");
	}

	return TRUE; // should be false?
}

gboolean on_chartArea_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
	// which one are we?
	int win_index= window_index_from_window(parent_gtk_window(widget));
	if(win_index != NONE)
	{
		adjust_canvas_size(win_index);
	}
		
	return FALSE;
}

/* ------------- popup menu buttons */
// because swapped, the menu is the gpointer.
static gint clear_Plots_PopupHandler (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	GtkMenu *menu;
	GdkEventButton *event_button;
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (user_data != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_MENU (user_data), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	/* The "widget" is the menu that was supplied when
	* g_signal_connect_swapped() was called.
	*/
	menu = GTK_MENU (user_data);
	if (event->type == GDK_BUTTON_PRESS)
	{
		event_button = (GdkEventButton *) event;
		if (event_button->button == 1)
		{
			adjust_clear_plots_menu(widget, menu);
			
			gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
			return TRUE;
		}
	}
	
	return FALSE;
}

static void adjust_clear_plots_menu(GtkWidget *widget, GtkMenu *menu)
{
	canvasinfo *can_info= canvas_info_for_widget(GTK_WIDGET(widget));
	gint canvas_index= can_info->canvas_num;
	int ii;

	// remove all the items.
	GList *initial_list = gtk_container_get_children(GTK_CONTAINER(menu));
	GList *entry= initial_list;
	while(entry)
	{
		gtk_widget_destroy(GTK_WIDGET(entry->data));
		entry= entry->next;
	}
	g_list_free(initial_list);

	// add all the items back...
	GtkWidget *menu_item;
	char buf[200];

	/* Copy the names to the buf. */
	sprintf (buf, "Clear All Plots");

	/* Create a new menu-item with a name... */
	menu_item = gtk_menu_item_new_with_label (buf);
	gtk_menu_append (GTK_MENU (menu), menu_item);
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (on_clear_plot_menu_item), GINT_TO_POINTER (MAKE_CLEAR_PROC_DATA(canvas_index, CLEAR_ALL_PLOTS_CONSTANT)));
	gtk_widget_show (menu_item);

	/* Append a separator */
	menu_item =  gtk_separator_menu_item_new();
	gtk_menu_append (GTK_MENU (menu), menu_item);
	gtk_widget_show (menu_item);

	// now all the other ones ...
	for(ii=0; ii<MAX_PLOTS; ii++)
	{
		if (can_info->alive_plots[ii])
		{
			sprintf(buf, "Clear %s  vs  %s",
				head.ch[can_info->plots[ii]->col_x].name, 
				head.ch[can_info->plots[ii]->col_y].name);
				
			/* Create a new menu-item with a name... */
			menu_item = gtk_menu_item_new_with_label (buf);
			gtk_menu_append (GTK_MENU (menu), menu_item);
			gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (on_clear_plot_menu_item), GINT_TO_POINTER (MAKE_CLEAR_PROC_DATA(canvas_index, ii)));
			gtk_widget_show (menu_item);
		}
    }

	// store it..
}

static void on_clear_plot_menu_item(
	GtkObject *object,
	gpointer user_data)
{
	gint clear_proc_data= GPOINTER_TO_INT(user_data);
	
	// load it in.
	int win_index= GET_CANVAS_INDEX(clear_proc_data);
	int plot_index= GET_PLOT_NUMBER(clear_proc_data);
	
//	fprintf(stderr, "Clear plot index %d Canvas Index: %d\n", plot_index, win_index);

	assert(win_index>=0 && win_index<MAXIMUM_NUMBER_OF_WINDOWS);
	canvasinfo *can_info= wininfo.canvases[win_index];

	if(plot_index==CLEAR_ALL_PLOTS_CONSTANT)
	{
		int i;

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

		display_active_window(ui_globals.active_window+1); // WHY?
		display_active_plot(-1);    
	} else {
		int n_holes, i, j;
		char tmp_buf[200];
		int hole_index[MAX_PLOTS];
		
		// clear the one plot
		/* kill selected plots */
		sprintf(msg, "Clear plots: ");

		n_holes=0; /*number of plots being axed*/
		for (i=0; i<MAX_PLOTS; i++)
		{
			if(i==plot_index && can_info->alive_plots[i])
			{					
				/*reset active plot if axing currently active plot. If it's 0, just leave it as 0 */
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

		if(can_info->total_plots ==0)
		{
			can_info->active_plot = -1;
		}
		else  
		{  
			j=can_info->active_plot;                /*save active plot*/
			for (i=0; i<MAX_PLOTS; i++)             /*re-draw */
			{
				if (can_info->alive_plots[i] == 1)
				{
					can_info->active_plot = i;
				}
			}
			if(can_info->alive_plots[j] == 0)     /*active plot got axed, use default*/
				can_info->active_plot = can_info->total_plots-1;
			else
				can_info->active_plot = j;
		}
		set_active_plot(can_info->active_plot);
	}

	// either way, we need to invalidate.
	invalidate_plot(can_info->plot_window->window);
}

/* ---------------------- Zoom Popup Menu */
static gint mouse_and_zoom_PopupHandler (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	GtkMenu *menu;
	GdkEventButton *event_button;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (user_data != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_MENU (user_data), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	/* The "widget" is the menu that was supplied when
	* g_signal_connect_swapped() was called.
	*/
	menu = GTK_MENU (user_data);
	if (event->type == GDK_BUTTON_PRESS)
	{
		event_button = (GdkEventButton *) event;
		if (event_button->button == 1)
		{
			gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event_button->button, event_button->time);
			return TRUE;
		}
	}
	
	return FALSE;
}

static void on_zoom_menu_item(GtkObject *object, gpointer user_data)
{
	gint clear_proc_data= GPOINTER_TO_INT(user_data);
	
	// load it in.
	int win_index= GET_CANVAS_INDEX(clear_proc_data);
	int cmd_index= GET_PLOT_NUMBER(clear_proc_data); // reuse.

	assert(win_index>=0 && win_index<MAXIMUM_NUMBER_OF_WINDOWS);
	canvasinfo *can_info= wininfo.canvases[win_index];
	
	switch(cmd_index)
	{
		case _zoom_item_zoom_and_clear:
			fprintf(stderr, "Zoom and Clear\n");
			break;
		case _zoom_item_zoom:
			fprintf(stderr, "Zoom\n");
			break;
		case _zoom_item_zoom_new_window:
			fprintf(stderr, "Zoom and New\n");
			break;
		default:
			fprintf(stderr, "Unknown command index for zoom menu: %d\n", cmd_index);
			break;
	}
	
	return;
}

/* ------------------- Mouse Menu Item hnalde */
static void on_mouse_menu_item(GtkObject *object, gpointer user_data)
{
	gint clear_proc_data= GPOINTER_TO_INT(user_data);
	
	// load it in.
	int win_index= GET_CANVAS_INDEX(clear_proc_data);
	int cmd_index= GET_PLOT_NUMBER(clear_proc_data); // reuse.

	assert(win_index>=0 && win_index<MAXIMUM_NUMBER_OF_WINDOWS);
	canvasinfo *can_info= wininfo.canvases[win_index];
	
	switch(cmd_index)
	{
		case _mouse_item_line_plot:
			fprintf(stderr, "Line Plot\n");
			break;
		case _mouse_item_vertical_line:
			fprintf(stderr, "Vertical Line\n");
			break;
		case _mouse_item_distance:
			fprintf(stderr, "Distance\n");
			break;
		default:
			fprintf(stderr, "Unknown command index for mouse menu: %d\n", cmd_index);
			break;
	}
	
	return;
}



/* ----------------- other local code */
static void invalidate_widget_drawing(GtkWidget *widget)
{
	gtk_widget_queue_draw_area(widget, 0, 0, widget->allocation.width, widget->allocation.height);
}

static canvasinfo *canvas_info_for_widget(GtkWidget *widget)
{
	GtkWindow *window;
	canvasinfo *result= NULL;
	
	if(GTK_IS_WINDOW(widget))
	{
		window= GTK_WINDOW(widget);
	} else {
		window= parent_gtk_window(widget);
	}

	int win_index= window_index_from_window(window);
	if(win_index != NONE)
	{
		result= wininfo.canvases[win_index];
	}
	
	return result;
}

static void set_left_footer_message(GtkWindow *parent, char *txt)
{
	set_plot_label_message(parent, PLOT_LABEL_LEFT_FOOTER, txt);
}

static void set_plot_label_message(GtkWindow *parent, PlotLabelID id, char *txt)
{
	char *names[]= { "label_LeftFooter", "label_X", "label_Y", "label_RowNumber", "label_PlotRows" };
	
	assert(id>=0 && id<ARRAY_SIZE(names));
	GtkLabel *label= GTK_LABEL(lookup_widget_by_name(GTK_WIDGET(parent), names[id]));
	assert(label);
	gtk_label_set_text(label, txt);
}

static int window_index_from_window(GtkWindow *window)
{
	int i;
	
	for (i = 0; i <MAXIMUM_NUMBER_OF_WINDOWS; i++)
	{
//		fprintf(stderr, "Checking %d for window %p...", i, window);
		if (wininfo.windows[i] == 1 && GTK_WINDOW(wininfo.canvases[i]->plot_window->window)==window)
		{
//			fprintf(stderr, "Found!\n");
			return i;
		} else {
//			fprintf(stderr, "Nope!\n");
		}
	}
	return NONE;
}


static void draw_string(GtkWidget *widget, int x, int y, char *string)
{
	gtk_draw_string(
		gtk_widget_get_style(widget), 
		widget->window, 
		gtk_widget_get_state(widget), 
		x, y, string);
/*

	gdk_draw_string(widget->window,
    	NULL, // GdkFont *font,
		widget->style->fg_gc[gtk_widget_get_state (widget)],
		x, y, string);
*/
}

static void draw_line(GtkWidget *widget, int x0, int y0, int x1, int y1)
{
	gdk_draw_line(widget->window, 
		widget->style->fg_gc[gtk_widget_get_state (widget)],
		x0, y0, 
		x1, y1);
}


static void erase_rectangle(GtkWidget *widget, int x, int y, int width, int height)
{
	GdkColor white;
	
	white.red= white.green= white.blue= 0xffff;
	gdk_gc_set_rgb_fg_color(widget->style->fg_gc[gtk_widget_get_state (widget)],&widget->style->white);
	gdk_draw_rectangle(
		widget->window, 
		widget->style->fg_gc[gtk_widget_get_state (widget)],
	    TRUE,
		x, y, 
		width, height);
	gdk_gc_set_rgb_fg_color(widget->style->fg_gc[gtk_widget_get_state (widget)], &widget->style->black);
}


/* cjm, 3/19/96: I think "label_type" refers to the way ticks and tick spacing is done*/
static void label_type0(GtkWidget *drawingArea, canvasinfo *can_info)
{
	int plot;
	plotarray *data;
	int tickx, ticky;
	double ten = 10.000;
	char string[256];
	int stringlen;  
	float xmax, xmin, ymax, ymin;
	int start_xaxis, start_yaxis, end_xaxis, end_yaxis;
	int start_x, start_y, end_x, end_y;
	int width, height;

	plot = can_info->active_plot;
	data = can_info->plots[plot];

	width = drawingArea->allocation.width;
	height = drawingArea->allocation.height;

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
		erase_rectangle(drawingArea, 0, 0, width, end_yaxis-1);

		erase_rectangle(drawingArea,
			0, end_y-1, 
			start_x-1, start_y-end_y+2);
			
		erase_rectangle(drawingArea,
			end_x+1, end_y-1, 
			width-end_x, start_y-end_y+2);
			
		erase_rectangle(drawingArea,
		 	0, start_y+1, width, height-start_y);
	}
  
   /* x-axis  (bottom) */
	draw_line(drawingArea,
		start_xaxis, start_yaxis, 
		end_xaxis, start_yaxis);

	/* left y-axis  */
	draw_line(drawingArea,
		start_xaxis, start_yaxis,
		start_xaxis, end_yaxis); 

	/* right y-axis  */
	draw_line(drawingArea,
		end_xaxis, start_yaxis,
		end_xaxis, end_yaxis);


	/* sizes for the tick marks */
	ticky=(int)width*0.02/2;
	tickx=(int)height*0.02/2;

	/* tick and label for ymin */
	draw_line(drawingArea,
		start_xaxis, start_y,
		start_xaxis+ticky, start_y);
	draw_line(drawingArea,
		end_xaxis, start_y,
		end_xaxis-ticky, start_y);

	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)ymin) : ymin);
	draw_string(drawingArea,
	      start_xaxis - ui_globals.tickfontwidth*(strlen(string)+1),
	      start_y + ui_globals.tickfontheight/2,
	      string);

	/* tick and label for ymax */ 
	draw_line(drawingArea,
		start_xaxis, end_y,
		start_xaxis+ticky, end_y);
	draw_line(drawingArea,
		end_xaxis, end_y,
		end_xaxis-ticky, end_y);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)ymax) : ymax);

	draw_string(drawingArea,
		start_xaxis - ui_globals.tickfontwidth*(strlen(string)+1),
		end_y + ui_globals.tickfontheight/2,
		string);

	/* tick and label for y mid */ 
	draw_line(drawingArea,
		start_xaxis, end_y + (start_y - end_y)/2,
		start_xaxis+ticky, end_y + (start_y - end_y)/2);
	draw_line(drawingArea,
		end_xaxis, end_y + (start_y - end_y)/2,
		end_xaxis-ticky, end_y + (start_y - end_y)/2);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)(ymax-ymin)/2) : (ymax+ymin)/2);
	draw_string(drawingArea,
		start_xaxis - ui_globals.tickfontwidth*(strlen(string)+1),
		end_y + (start_y - end_y)/2 + ui_globals.tickfontheight/2,
		string);

	/* min x axis */
	draw_line(drawingArea,
		start_x, start_yaxis,
		start_x, start_yaxis - ticky);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)xmin) : xmin);
	draw_string(drawingArea,
		start_x - ui_globals.tickfontwidth*stringlen/2,
		start_yaxis + ui_globals.tickfontheight + 2,
		string);

	/* max x axis */
	draw_line(drawingArea,
		end_x, start_yaxis,
		end_x, start_yaxis - ticky);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)xmax) : xmax);
	draw_string(drawingArea,
		end_x - ui_globals.tickfontwidth*stringlen/2,
		start_yaxis + ui_globals.tickfontheight + 2,
		string);

	/* mid x axis */
	draw_line(drawingArea,
		start_x + (end_x-start_x)/2, start_yaxis,
		start_x + (end_x-start_x)/2, start_yaxis - ticky);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)(xmax-xmin)/2) : (xmax+xmin)/2);
	draw_string(drawingArea,
		start_x + (end_x-start_x)/2 - ui_globals.tickfontwidth*stringlen/2,
		start_yaxis + ui_globals.tickfontheight + 2,
		string);

	sprintf(string, "%d.   %s  vs  %s", 
		plot+1,
		head.ch[data->col_x].name, 
		head.ch[data->col_y].name);

	draw_string(drawingArea,
		start_xaxis, ui_globals.titlefontheight+2, 
		string);

	sprintf(string,"%s",head.title);	/*add title, right justified*/

	draw_string(drawingArea,
		end_xaxis-ui_globals.titlefontwidth*strlen(string), ui_globals.titlefontheight+2,
		string);
}


static void label_type1(GtkWidget *drawingArea, canvasinfo *can_info)
{
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


	plot = can_info->active_plot;
	data = can_info->plots[plot];

	width = drawingArea->allocation.width;
	height = drawingArea->allocation.height;

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
		erase_rectangle(drawingArea, 0, 0, width, end_yaxis-1);

		/* clear left y axis */
		erase_rectangle(drawingArea,
			0, end_y-1, 
			start_x-1, start_y-end_y+2);

		/* clear right y axis */
		erase_rectangle(drawingArea,
			end_x+1, end_y-1, 
			width-end_x, start_y-end_y+2);

		/* clear x axis */
		erase_rectangle(drawingArea,
			0, start_y+1, width, height-start_y);
	}

	/* x-axis  (bottom) */
	draw_line(drawingArea,
		start_xaxis, start_yaxis, 
		end_xaxis, start_yaxis);
		
	/* left y-axis  */
	draw_line(drawingArea,
		start_xaxis, start_yaxis,
		start_xaxis, end_yaxis); 
	
	/* right y-axis  */
	draw_line(drawingArea,
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

		draw_line(drawingArea,
			start_xaxis, where_y,
			start_xaxis+ticky, where_y);

		draw_line(drawingArea,
			end_xaxis, where_y,
			end_xaxis-ticky, where_y);

		sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
			pow(ten, (double)big_ticky) : big_ticky);

		draw_string(drawingArea,
			start_xaxis - ui_globals.tickfontwidth*(strlen(string)+1),
			start_y-(int)((big_ticky-ymin)*scale_y)+ui_globals.tickfontheight/2,
			string);

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

			draw_line(drawingArea,
				start_xaxis, where_y,
				start_xaxis+ticky, where_y);

			draw_line(drawingArea,
				end_xaxis, where_y,
				end_xaxis-ticky, where_y);

			sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
				pow(ten, (double)big_ticky) : big_ticky);

			draw_string(drawingArea,
				start_xaxis - ui_globals.tickfontwidth*(stringlen+1),
				start_y-(int)((big_ticky-ymin)*scale_y)+ui_globals.tickfontheight/2,
				string);

			big_ticky += pow(ten, decy);
		}
	}

	/*  x axis big tick marks */
	for (a=0;big_tickx<=stop_xmax;a++)
	{
		where_x= start_x + (int)((big_tickx-xmin)*scale_x);

		draw_line(drawingArea,
			where_x, start_yaxis,
			where_x, start_yaxis - tickx);

		sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
			pow(ten, (double)big_tickx) : big_tickx);

		draw_string(drawingArea,
			where_x - ui_globals.tickfontwidth*stringlen/2,
			start_yaxis + ui_globals.tickfontheight + 2,
			string);

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

			draw_line(drawingArea,
				where_x, start_yaxis,
				where_x, start_yaxis - tickx);

			sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
				pow(ten, (double)big_tickx) : big_tickx);
			
			draw_string(drawingArea,
				where_x - ui_globals.tickfontwidth*stringlen/2,
				start_yaxis + ui_globals.tickfontheight + 2,
				string);

			big_tickx += pow(ten, decx);
		}
	}

	sprintf(string, "%d.   %s  vs  %s", 
		plot+1,
		head.ch[data->col_x].name, 
		head.ch[data->col_y].name);

	draw_string(drawingArea,
		start_xaxis, ui_globals.titlefontheight+2, 
		string);

	sprintf(string,"%s",head.title);	/*add title, right justified*/

	draw_string(drawingArea,
		end_xaxis-ui_globals.titlefontwidth*strlen(string), ui_globals.titlefontheight+2,
		string);
}


static int get_row_number(canvasinfo *can_info, int nrow, float x1, float y1)
{
	int   ii, j	;
	int	*list;
	float	xmax,xmin,ymax,ymin;
	double min_dist, dist;
	plotarray *data;
	int rownum;
	double *x, *y;

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

/* ------------------------ Plot Lines/Points Combobox */
enum {
	PLOT_TYPE_NAME= 0,
	NUMBER_OF_PLOT_TYPE_COLUMNS
};

void on_comboboxPlotType_realize(GtkWidget *widget, gpointer user_data)
{
	// create the model for this combobox..
	GtkListStore *store = gtk_list_store_new (NUMBER_OF_PLOT_TYPE_COLUMNS, G_TYPE_STRING);
	
	/* set the model */
	gtk_combo_box_set_model(GTK_COMBO_BOX(widget), GTK_TREE_MODEL(store));

	/* set the renderer */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", PLOT_TYPE_NAME);

	// setup the menu (initial conditions)
	GtkTreeIter iter;

	/* Add a new row to the model */
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, PLOT_TYPE_NAME, "Lines", -1);

	/* Add a new row to the model */
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, PLOT_TYPE_NAME, "Points", -1);

	// set the initial state...
	canvasinfo *info= canvas_info_for_widget(GTK_WIDGET(widget));
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), info->point_plot);
}


void on_comboboxPlotType_changed(GtkComboBox *widget, gpointer user_data)
{
	canvasinfo *info= canvas_info_for_widget(GTK_WIDGET(widget));
	gchar *active_plot_text= gtk_combo_box_get_active_text(widget);
	int new_value= info->point_plot;
	
	if(active_plot_text)
	{
		if(strcmp(active_plot_text, "Lines")==0)
		{
			new_value= 0;
		} else {
			new_value= 1;
		}
	}

	if(new_value != info->point_plot)
	{
		// update it.
		info->point_plot= new_value;

		// now redraw the graphic..
		invalidate_plot(parent_gtk_window(GTK_WIDGET(widget)));
	}
}

/* ------------------------- Activating Plots & The Plot Combobox */
enum {
	ACTIVE_PLOT_NAME= 0,
	ACTIVE_PLOT_INDEX,
	NUMBER_OF_ACTIVE_PLOT_COLUMNS
};

void on_comboboxActivePlot_realize(GtkWidget *widget, gpointer user_data)
{
	// create the model for this combobox..
	GtkListStore *store = gtk_list_store_new (NUMBER_OF_ACTIVE_PLOT_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
	
	/* set the model */
	gtk_combo_box_set_model(GTK_COMBO_BOX(widget), GTK_TREE_MODEL(store));

	/* set the renderer */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", ACTIVE_PLOT_NAME);

	// setup the menu (initial conditions)
	rebuild_active_plot_combo_list(GTK_COMBO_BOX(widget));
}

// on every popdown, we should reset it (maybe?)

// when it is changed..
void on_comboboxActivePlot_changed(GtkComboBox *widget, gpointer user_data)
{
	canvasinfo *can_info= canvas_info_for_widget(GTK_WIDGET(widget));

fprintf(stderr, "Active changed!\n");
	gchar *active_plot_text= gtk_combo_box_get_active_text(widget);
	if(active_plot_text)
	{
		int i;
		
		for(i=0; i<MAX_PLOTS; i++)
		{
			if (can_info->alive_plots[i])
			{
				char temp[512];
				sprintf(temp, "%d. %s  vs  %s", i+1,
					head.ch[can_info->plots[i]->col_x].name, 
					head.ch[can_info->plots[i]->col_y].name);
					
				if(strcmp(temp, active_plot_text)==0)
				{
					if(can_info->active_plot != i)
					{
						set_active_plot_in_window(can_info->plot_window, i);
						invalidate_plot(parent_gtk_window(GTK_WIDGET(widget)));
					}
					break;
				}
			}
	    }
		
		g_free(active_plot_text);
	}
}

static void rebuild_active_plot_combo_list(GtkComboBox *widget)
{
	int i;
	gint activeIndex= NONE;
	canvasinfo *can_info= canvas_info_for_widget(GTK_WIDGET(widget));
	char temporary[512];

	GtkListStore *store= GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(widget)));

	// clear everything in there currently...
	gtk_list_store_clear(store);
   
	for(i=0; i<MAX_PLOTS; i++)
	{
		if (can_info->alive_plots[i])
		{
			GtkTreeIter iter;
			sprintf(temporary, "%d. %s  vs  %s", i+1,
				head.ch[can_info->plots[i]->col_x].name, 
				head.ch[can_info->plots[i]->col_y].name);
				
			if(can_info->active_plot==i)
			{
				activeIndex= i;
			}

			/* Add a new row to the model */
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
				ACTIVE_PLOT_NAME, temporary,
				ACTIVE_PLOT_INDEX, i,
				-1);
		}
    }

	// set the initial selection...
	if(activeIndex != NONE)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), activeIndex);
	}
}
