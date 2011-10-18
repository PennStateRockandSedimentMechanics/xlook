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
#include "offscreen_buffer.h"

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

#define DATA_TO_SCREEN_X(x, ddd, ccc) ((ccc)->start_x + ((x) - (ddd)->xmin)*(ddd)->scale_x)
#define DATA_TO_SCREEN_Y(y, ddd, ccc) ((ccc)->start_y + ((y) - (ddd)->ymin)*(ddd)->scale_y)

#define SCREEN_TO_DATA_X(x, ddd, ccc) ((float)((x) - (ccc)->start_x)/(ddd)->scale_x + (ddd)->xmin)
#define SCREEN_TO_DATA_Y(y, ddd, ccc) ((float)((y) - (ccc)->start_y)/(ddd)->scale_y + (ddd)->ymin)

#define SCALE_X(ddd, ccc)	 (float)(((ccc)->end_x - (ccc)->start_x)/((ddd)->xmax - (ddd)->xmin))
#define SCALE_Y(ddd, ccc)	 (float)(((ccc)->end_y - (ccc)->start_y)/((ddd)->ymax - (ddd)->ymin))

static int get_new_window_num();
static void adjust_canvas_size(int index);
static int window_index_from_window(GtkWindow *window);
static void set_left_footer_message(GtkWindow *parent, const char *txt);
static void set_plot_label_message(GtkWindow *parent, PlotLabelID id, const char *txt);


static void label_type1(canvasinfo *can_info);
static void label_type0(canvasinfo *can_info);
static int get_row_number(canvasinfo *can_info, int nrow, float x1, float y1);
static canvasinfo *canvas_info_for_widget(GtkWidget *widget);
static void adjust_canvas_size(int index);
static void rebuild_active_plot_combo_list(GtkComboBox *widget);
static void invalidate_plot(GtkWindow *window, gboolean clear_annotations);
static gint clear_Plots_PopupHandler (GtkWidget *widget, GdkEvent *event, gpointer user_data);
static gint mouse_and_zoom_PopupHandler (GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void adjust_clear_plots_menu(GtkWidget *widget, GtkMenu *menu);
static void on_clear_plot_menu_item(GtkObject *object, gpointer user_data);
static void on_zoom_menu_item(GtkObject *object, gpointer user_data);
static void on_mouse_menu_item(GtkObject *object, gpointer user_data);
static void set_mouse_message_for_mode(GtkWindow *window, int mode);
static void set_mouse_mode(canvasinfo *can_info, int mouse_mode);
static void draw_into_offscreen_buffer(GtkWidget *widget);
static void draw_chart_immediately(GtkWidget *widget, canvasinfo *info);
static void draw_data_crosshair(struct offscreen_buffer *buffer, const char *string,int xloc, int yloc);
static void clear_active_plot(canvasinfo *can_info);
static void draw_crosshair_with_coordinates(GtkWidget *widget, float x, float y);

char *csprintf(char *buffer, char *format, ...);

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

enum {
	_mouse_mode_normal= 0,
	_mouse_mode_draw_line,
	_mouse_mode_vertical_line,
	_mouse_mode_distance,
	_mouse_mode_zoom_and_clear,
	_mouse_mode_zoom,
	_mouse_mode_zoom_new_window,
	NUMBER_OF_MOUSE_MODES
};

static const char *mouse_mode_names[]= {
	"Normal",
	"Line",
	"Vertical Line",
	"Distance",
	"Zoom and Clear",
	"Zoom",
	"Zoom New Window"
};

static const char *mouse_mode_labels[]= {
	"Normal Mode: Left & middle buttons pick row numbers. Right button gives x-y position",
	"Draw Line Mode: Left button click and drag to draw a line.",
	"Vertical Line Mode: Left button draws a vertical line.", // FIXME
	"Distance Mode: Left button click and drag to draw a line and show distance.",
	"Zoom Mode: Left button click and drag selects zoom area.",
	"Zoom Mode: Left button click and drag selects zoom area.",
	"Zoom Mode: Left button click and drag selects zoom area."
};


short canvas0_image[] = {
#include "canvas.ico"
};
//Server_image c0image;
//Icon cstate;

struct plot_window
{
	GtkWindow *window;
};

/* ------------------- primary code */
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

		// null it out.
		memset(can_info, 0, sizeof(canvasinfo));
		
		ui_globals.old_active_window = ui_globals.active_window;
		ui_globals.active_window = win_num;
//		fprintf(stderr, "Window pointer is %p\n", window);
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


		/*set footer info. This should be redundant since it only gets set or changed from the panel buttons...*/
		set_mouse_message_for_mode(GTK_WINDOW(window), can_info->mouse.mode);
		set_plot_label_message(GTK_WINDOW(window), LABEL_PLOT_ROWS, "PLOT ROWS:  ");

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
//fprintf(stderr, "set active plot in window to %d\n", i);
	assert(pw);
	assert(pw->window);
	canvasinfo *can_info= canvas_info_for_widget(GTK_WIDGET(pw->window));

	/* set the active plot */
	can_info->active_plot = i;

	/* invalidate the drawing area */
	invalidate_plot(pw->window, TRUE);

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

	invalidate_plot(pw->window, TRUE);
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
	invalidate_plot(pw->window, TRUE);
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

static void invalidate_plot(GtkWindow *window, gboolean clear_annotations)
{
	// now redraw the graphic..
	GtkWidget *drawingArea= lookup_widget_by_name(GTK_WIDGET(window), "chartArea");

	canvasinfo *info= canvas_info_for_widget(GTK_WIDGET(window));

	// invalidate is a "real" dirty; so we need to nuke our backbuffer.
	if(clear_annotations)
	{
		if(info->offscreen_buffer != NULL)
		{
			dispose_buffer(info->offscreen_buffer);
			info->offscreen_buffer= NULL;
		}
	} else {
		// they called this with false, which means they probably drew into the offscreen buffer (annotations)
		if(info->offscreen_buffer)
		{
			regenerate_pixbuf(info->offscreen_buffer);
		}
	}
	
	// now setup the redraw flags...
	gtk_widget_queue_draw_area(drawingArea, 0, 0, drawingArea->allocation.width, drawingArea->allocation.height);
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
//	can_info->point_plot = 0;
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

static void draw_crosshair_with_coordinates(GtkWidget *widget, float x, float y)
{
	canvasinfo *info= canvas_info_for_widget(widget);
	if(info->active_plot>0)
	{
		GdkDrawable *drawable= info->offscreen_buffer->pixmap;
		GdkGC *gc= info->offscreen_buffer->gc;
		int text_width, text_height;

		plotarray *data = info->plots[info->active_plot];
		GdkColor color;

		get_color_for_type(PLOT_LABEL_COLOR, &color);
		gdk_gc_set_rgb_fg_color(gc, &color);

		/* get the x and y (data) values */ 
		float xval= SCREEN_TO_DATA_X(info->mouse.start.x, data, info);
		float yval= SCREEN_TO_DATA_Y(info->mouse.start.y, data, info);

		sprintf(msg, "X: %.5g Y: %.5g", xval, yval);
		PangoLayout *pango_layout= info->offscreen_buffer->pango_layout;
		pango_layout_set_text(pango_layout, msg, strlen(msg));
		pango_layout_get_size(pango_layout, &text_width, &text_height);
		gdk_draw_layout(info->offscreen_buffer->pixmap, info->offscreen_buffer->gc, 
			x+3, y-(PANGO_PIXELS(text_height)),
			pango_layout);

		gdk_draw_line(drawable, gc, x-5, y, x+5, y);
		gdk_draw_line(drawable, gc, x, y-5, x, y+5);
	}	
}

gboolean on_chartArea_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	canvasinfo *info= canvas_info_for_widget(widget);
	GtkWindow *window= GTK_WINDOW(info->plot_window->window);

	info->mouse.tracking= FALSE;

	// so we can use it below.
	info->mouse.start.x= event->x;
	info->mouse.start.y= event->y;

	if (info->active_plot == -1)
	{
		sprintf(msg, "There is no plot in this window.\n");
		print_msg(msg);
	} 
	else if(info->mouse.mode==_mouse_mode_vertical_line)
	{
		gdk_draw_line(info->offscreen_buffer->pixmap, info->offscreen_buffer->gc,
			info->mouse.start.x, info->start_y, 
			info->mouse.start.x, info->end_y);
		
		invalidate_plot(window, FALSE);
	} 
	else if(info->mouse.mode==_mouse_mode_normal)
	{
		/* print row num on info panel only */
		plotarray *data = info->plots[info->active_plot];

		/* get the x and y (data) values */ 
		float xval= SCREEN_TO_DATA_X(info->mouse.start.x, data, info);
		float yval= SCREEN_TO_DATA_Y(info->mouse.start.y, data, info);

		// show the information
		if(event->button==1) // left
		{
			int row_num = get_row_number(info, data->nrows_x, xval, yval);
			if (row_num <= data->nrows_x && row_num != -1)
			{
				/* get x and y values from data (not screen) */
				xval = data->xarray[row_num]; 
				yval = data->yarray[row_num]; 
				row_num = row_num + data->begin; 

				sprintf(msg, "Row Number: %d", row_num);
				set_plot_label_message(window, LABEL_ROW_NUMBER, msg);
				
				/* draw crosshair on point */
				point2d converted_pt;
				converted_pt.x= DATA_TO_SCREEN_X(xval, data, info);
				converted_pt.y= DATA_TO_SCREEN_Y(yval, data, info);

//				(info->offscreen_buffer->pixmap, info->offscreen_buffer->gc, converted_pt.x, converted_pt.y);
				draw_crosshair_with_coordinates(widget, converted_pt.x, converted_pt.y);
			}
			else 
			{
				set_plot_label_message(window, LABEL_ROW_NUMBER, "Row Number: None");
				print_msg("The point picked was not on the curve.\n");
			}
			
			sprintf(msg, "X: %.5g", xval); 
			set_plot_label_message(window, LABEL_X, msg);
			sprintf(msg, "Y: %.5g", yval); 
			set_plot_label_message(window, LABEL_Y, msg);
		} else { // right or middle
//		  	print_xy(xloc, yloc);
			set_plot_label_message(window, LABEL_ROW_NUMBER, "");

			/*  print the x and y coord on the panels */
			sprintf(msg, "X: %.5g", xval); 
			set_plot_label_message(window, LABEL_X, msg);
			sprintf(msg, "Y: %.5g", yval);
			set_plot_label_message(window, LABEL_Y, msg);

		  	/*  print the x and y coord on the msg window */
			sprintf(msg, "X: %.5g Y: %.5g", xval, yval);
			draw_crosshair_with_coordinates(widget, info->mouse.start.x, info->mouse.start.y);
			strcat(msg, "\n");
			print_msg(msg);
		}
		
		// and update.
		invalidate_plot(window, FALSE);
	} else {
		/* print row num on info panel only */
		plotarray *data = info->plots[info->active_plot];

		// we are tracking
		info->mouse.tracking= TRUE;

		/* get the x and y (data) values */ 
		float xval= SCREEN_TO_DATA_X(info->mouse.start.x, data, info);
		float yval= SCREEN_TO_DATA_Y(info->mouse.start.y, data, info);

		int row_num = get_row_number(info, data->nrows_x, xval, yval);
		if (row_num <= data->nrows_x && row_num != -1)
		{
			info->mouse.zp1 = row_num;
		} else {
			info->mouse.zp1 = NONE;
		}
	}
		
	return FALSE;
}


gboolean on_chartArea_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
//	fprintf(stderr, "Mouse button released\n");
	canvasinfo *info= canvas_info_for_widget(widget);
	GtkWindow *window= GTK_WINDOW(info->plot_window->window);
	
	if(info->mouse.tracking)
	{
		// assert(info->active_plot>=0 && info->active_plot<MAX_PLOTS);
		plotarray *data = info->plots[info->active_plot];

		// released- no longer tracking...
		info->mouse.tracking= FALSE;
//		fprintf(stderr, "Was tracking; should commit.");

		// store the final points
		info->mouse.end.x= event->x;
		info->mouse.end.y= event->y;

		/* get the x and y (data) values */ 
		float xval= SCREEN_TO_DATA_X(info->mouse.end.x, data, info);
		float yval= SCREEN_TO_DATA_Y(info->mouse.end.y, data, info);

		int row_num = get_row_number(info, data->nrows_x, xval, yval);
		if (row_num <= data->nrows_x && row_num != -1)
		{
			info->mouse.zp2 = row_num;
		} else {
			info->mouse.zp2 = NONE;
		}
		
		switch(info->mouse.mode)
		{
			case _mouse_mode_normal:
			case _mouse_mode_vertical_line:
			{
				// handled on the press
			}
				break;
				
			case _mouse_mode_draw_line:
			case _mouse_mode_distance:
				{
					float slope, intercept, x1, x2, y1, y2;

					// Draw the line into the backbuffer (the main one), and update...
					gdk_draw_line(info->offscreen_buffer->pixmap, info->offscreen_buffer->gc,
						info->mouse.start.x, info->mouse.start.y, info->mouse.end.x, info->mouse.end.y);
						
					// draw the points on both ends..
					draw_crosshair_with_coordinates(widget, info->mouse.start.x, info->mouse.start.y);
					draw_crosshair_with_coordinates(widget, info->mouse.end.x, info->mouse.end.y);
						
					// now update the pixmap
					invalidate_plot(window, FALSE);

//					fprintf(stderr, "Start: %d,%d End: %d, %d\n", info->mouse.start.x, info->mouse.start.y, info->mouse.end.x, info->mouse.end.y);

					x1= SCREEN_TO_DATA_X(info->mouse.start.x, data, info);
					y1= SCREEN_TO_DATA_Y(info->mouse.start.y, data, info);

					x2= SCREEN_TO_DATA_X(info->mouse.end.x, data, info);
					y2= SCREEN_TO_DATA_Y(info->mouse.end.y, data, info);

//					fprintf(stderr, "Coords Start: %f,%f End: %f, %f\n", x1, y1, x2, y2);

					set_plot_label_message(window, LABEL_ROW_NUMBER, "");
					
					if(info->mouse.mode==_mouse_mode_draw_line)
					{
						if((x2-x1)!=0)
						{
							slope = (y2 - y1)/(x2 - x1); // FIXME: DIVDIE BY ZERO
							intercept = y1 - slope*x1;
						
							set_plot_label_message(window, LABEL_X, csprintf(msg, "Slope: %.5g", slope));
							set_plot_label_message(window, LABEL_Y, csprintf(msg, "Y intercept: %.5g", intercept));

							sprintf(msg, "line plot: x1,y1:(%f, %f), x2,y2:(%f, %f)\nslope: %g, int.: %g\n", x1, y1, x2, y2, slope, intercept);
						} else {
							sprintf(msg, "line plot: x1,y1:(%f, %f), x2,y2:(%f, %f)\nslope: Inf, int.: None\n", x1, y1, x2, y2);

							set_plot_label_message(window, LABEL_X, "Slope: Infinite");
							set_plot_label_message(window, LABEL_Y, "Y intercept: None");
						}
						print_msg(msg);
					} else {
						// measurement
						float dx = fabs(x1 - x2);
						float dy = fabs(y1 - y2);
						float dv = sqrt((double)((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)));

						set_plot_label_message(window, LABEL_X, csprintf(msg, "dX: %.5g", dx));
						set_plot_label_message(window, LABEL_Y, csprintf(msg, "dY: %.5g", dy));
						set_plot_label_message(window, LABEL_ROW_NUMBER, csprintf(msg, "dV: %.5g", dv));
					}
				}
				break;
			case _mouse_mode_zoom_and_clear:
			case _mouse_mode_zoom:
			case _mouse_mode_zoom_new_window:
				// if we want a two stage, we'll have to rethink it.
				if(event->button==1)
				{
					// commit it.
					int begin, end;

					/*set some default values, force row order*/
					if(info->mouse.zp1 < 0)
					{
						info->mouse.zp1 = 0;
					}
					
					if(info->mouse.zp2 <= 0)
					{
						info->mouse.zp2 = data->end - data->begin;
					}
						
					if(info->mouse.zp2 < info->mouse.zp1)
					{
						begin = info->mouse.zp2 + data->begin;
						end = info->mouse.zp1 + data->begin;
					}
					else
					{
						begin = info->mouse.zp1 + data->begin;
						end = info->mouse.zp2 + data->begin;
					}

					print_msg(csprintf(msg, "data-begin is %d, data-end is %d, data-zp1 is %d, data-zp2 is %d\n", data->begin, data->end, info->mouse.zp1, info->mouse.zp2));

					if (info->mouse.mode==_mouse_mode_zoom_and_clear)
					{
						clear_active_plot(info);
					}
					else if(info->mouse.mode==_mouse_mode_zoom_new_window)
					{
						/*this is where we have to make a new window, and plot to it*/
						/*zoom to new window*/
						new_win_proc();
					}
					strcpy(plot_cmd, "plotauto"); // stupid global
					do_plot(csprintf(msg, "plotauto %d %d %d %d", data->col_x, data->col_y, begin, end));

					// now update the pixmap (remove our meddling)
					invalidate_plot(window, FALSE);
				}
				break;
		}
	}
	
	return FALSE;	
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
		int row_num;

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
//		draw_crosshair(widget, xloc, yloc);

		xval = (xloc - can_info->start_x)/data->scale_x + data->xmin;
		yval = (can_info->start_y - yloc)/data->scale_y + data->ymin;

		// we got a notify before the buffer was setup.
		if(can_info->tracking_buffer==NULL) return FALSE;

		// draw the base pixmap in...
		gdk_draw_pixbuf(
			can_info->tracking_buffer->pixmap, 
			can_info->tracking_buffer->gc,
			can_info->offscreen_buffer->pixbuf, 0, 0, 0, 0, 
			can_info->offscreen_buffer->width,
			can_info->offscreen_buffer->height,
			GDK_RGB_DITHER_NONE,
			0, 0);

		if((event->state & GDK_BUTTON1_MASK) && can_info->mouse.tracking)
		{
			// update the points.
			can_info->mouse.end.x= xloc;
			can_info->mouse.end.y= yloc;

			if(can_info->mouse.mode==_mouse_mode_zoom || can_info->mouse.mode==_mouse_mode_zoom_and_clear || can_info->mouse.mode==_mouse_mode_zoom_new_window)
			{
				// drawing a rectangle
				gdk_draw_rectangle(can_info->tracking_buffer->pixmap, can_info->tracking_buffer->gc, 
					FALSE,
					MIN(can_info->mouse.start.x, can_info->mouse.end.x),
					MIN(can_info->mouse.start.y, can_info->mouse.end.y),
					abs(can_info->mouse.end.x - can_info->mouse.start.x), 
					abs(can_info->mouse.end.y - can_info->mouse.start.y));

			} else {
				// draw_line or distance- drawing a line.
				// erase the old one...

				// draw the new one...
				gdk_draw_line(can_info->tracking_buffer->pixmap, can_info->tracking_buffer->gc,
					can_info->mouse.start.x, can_info->mouse.start.y, can_info->mouse.end.x, can_info->mouse.end.y);
			}

//			fprintf(stderr, "Moving mouse with the button down...\n");
		} 

//fprintf(stderr, "Mouse: %d,%d Strt: %d,%d Scale: %f,%f\n", xloc, yloc, can_info->start_x, can_info->start_y, data->scale_x, data->scale_y);
//fprintf(stderr, "Active Plot: %d XVal: %f YVal: %f NRows: %d\n", can_info->active_plot, xval, yval, nrows);
		row_num = get_row_number(can_info, data->nrows_x, xval, yval);
		if (row_num <= data->nrows_x && row_num != -1)
		{
			/* get x and y values from data (not screen) */
			xmatch = data->xarray[row_num]; 
			ymatch = data->yarray[row_num]; 
			match_set= TRUE;
		
			row_num = row_num + data->begin; 
			sprintf(rowstring, "Row Number: %d", row_num);
			set_plot_label_message(window, LABEL_ROW_NUMBER, rowstring);

			/*  print the x and y coord on the panels */
			sprintf(xstring, "X: %.5g (Data: %.5g)", xval, xmatch); 
			set_plot_label_message(window, LABEL_X, xstring);
			sprintf(ystring, "Y: %.5g (Data: %.5g)", yval, ymatch);
			set_plot_label_message(window, LABEL_Y, ystring);
			
			/* draw crosshair on point */
			if(!can_info->mouse.tracking) // we don't show this if we are tracking a mouse down.
			{
				point2d pt;
			
				pt.x = DATA_TO_SCREEN_X(xmatch, data, can_info); // can_info->start_x + (xmatch - data->xmin)*data->scale_x;
				pt.y = DATA_TO_SCREEN_Y(ymatch, data, can_info); // can_info->start_y - (ymatch - data->ymin)*data->scale_y;

				sprintf(xstring, "X: %.5g  Y: %.5g\nRow: %d", xmatch, ymatch, row_num);
				draw_data_crosshair(can_info->tracking_buffer, xstring, pt.x, pt.y);
			}
		}
		else 
		{
			sprintf(rowstring, "Row Number: None");
			set_plot_label_message(window, LABEL_ROW_NUMBER, rowstring);

			/*  print the x and y coord on the panels */
			sprintf(xstring, "X: %.5g (No Match)", xval); 
			set_plot_label_message(window, LABEL_X, xstring);
			sprintf(ystring, "Y: %.5g  (No Match)", yval);
			set_plot_label_message(window, LABEL_Y, ystring);
		}

		// regenerate it..
		regenerate_pixbuf(can_info->tracking_buffer);
	
		// now draw the entire thing into the window...
		gdk_draw_pixbuf(widget->window, widget->style->fg_gc[gtk_widget_get_state (widget)],
			can_info->tracking_buffer->pixbuf, 0, 0, 0, 0, 
			can_info->offscreen_buffer->width,
			can_info->offscreen_buffer->height,
			GDK_RGB_DITHER_NONE,
			0, 0);

		if(event->is_hint)
		{
			gdk_event_request_motions(event);
		}
	}

	return FALSE;
}

gboolean on_chartArea_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	// erase to white.
	GtkWindow *window= parent_gtk_window(widget);

//	fprintf(stderr, "Drawing\n");
	int win_index= window_index_from_window(window);
	if(win_index != NONE)
	{
		canvasinfo *can_info = wininfo.canvases[win_index];

		draw_chart_immediately(widget, can_info);
	} else {
		fprintf(stderr, "something has gone bellyup.\n");
	}

	return TRUE; // should be false?
}

static void draw_chart_immediately(GtkWidget *widget, canvasinfo *can_info) 
{
	assert(widget);
	assert(can_info);

	// draw it offscreen...
	draw_into_offscreen_buffer(widget);

	// now draw the offscreen buffer to the screen.
	if(can_info->offscreen_buffer)
	{
		if(!can_info->offscreen_buffer->pixbuf)
		{
			fprintf(stderr, "odd.  pixbuf not allocated");
			regenerate_pixbuf(can_info->offscreen_buffer);
			assert(can_info->offscreen_buffer->pixbuf);
		}

		// draw it into
		gdk_draw_pixbuf(widget->window, widget->style->fg_gc[gtk_widget_get_state (widget)],
			can_info->offscreen_buffer->pixbuf, 0, 0, 0, 0, 
			can_info->offscreen_buffer->width,
			can_info->offscreen_buffer->height,
			GDK_RGB_DITHER_NONE,
			0, 0);
	}
}

static void draw_into_offscreen_buffer(GtkWidget *widget)
{
	// erase to white.
	GtkWindow *window= parent_gtk_window(widget);

	int win_index= window_index_from_window(window);
	if(win_index != NONE)
	{
		canvasinfo *can_info;
		int plot_index, i;
		plotarray *data;

		can_info = wininfo.canvases[win_index];
		
		if(can_info->offscreen_buffer==NULL || can_info->offscreen_buffer->width != widget->allocation.width || can_info->offscreen_buffer->height != widget->allocation.height)
		{
			// dispose...
			if(can_info->offscreen_buffer)
			{
				dispose_buffer(can_info->offscreen_buffer);
				can_info->offscreen_buffer= NULL;
			}
			
			// create...
			can_info->offscreen_buffer= create_buffer_for_widget(widget);

			assert(can_info->offscreen_buffer);
//			fprintf(stderr, "Drawing offscreen\n");
		
			GdkGC *gc= can_info->offscreen_buffer->gc;
			GdkDrawable *drawable= GDK_DRAWABLE(can_info->offscreen_buffer->pixmap);

			// erase the rectangle.
			gdk_gc_set_rgb_fg_color(gc, &widget->style->white);
			gdk_draw_rectangle(drawable, gc, TRUE, 
				0, 0, widget->allocation.width, widget->allocation.height);

			if(can_info->total_plots != 0)
			{
				// draw all the active plots.
				for (plot_index=0; plot_index<MAX_PLOTS; plot_index++)
				{
					if(can_info->alive_plots[plot_index] == 1)
					{
						GdkColor color;
						
						data = can_info->plots[plot_index];

						// these two need to be set before the labelling
						data->scale_x = SCALE_X(data, can_info); // (float)(can_info->end_x - can_info->start_x)/(data->xmax - data->xmin);
						data->scale_y = SCALE_Y(data, can_info); // (float)(can_info->end_y - can_info->start_y)/(data->ymax - data->ymin);

						if(plot_index==can_info->active_plot)
						{
							/* print the labels */
							if (data->label_type)
							{
								label_type1(can_info);
							}
							else
							{
								label_type0(can_info);
							}
							get_color_for_type(ACTIVE_PLOT_COLOR, &color);
						} else {
							get_color_for_type(INACTIVE_PLOT_COLOR, &color);
						}
						
						gdk_gc_set_rgb_fg_color(gc, &color);

						/* plot each point  */
						if (can_info->point_plot == 1)
						{
							/* plot the individual points */
							for (i=0; i < data->nrows_x -1; i++)
							{
							 	gdk_draw_point(drawable, gc,
									DATA_TO_SCREEN_X(data->xarray[i], data, can_info),
									DATA_TO_SCREEN_Y(data->yarray[i], data, can_info));
							}
						}
						else
						{
							// plot 0 is the measured values.
							// plot 1 is the curve
							/* connect the points */
							for (i=0; i < data->nrows_x -1; i++)
							{
								gdk_draw_line(drawable, gc,
									DATA_TO_SCREEN_X(data->xarray[i], data, can_info),
									DATA_TO_SCREEN_Y(data->yarray[i], data, can_info),
									DATA_TO_SCREEN_X(data->xarray[i+1], data, can_info),
									DATA_TO_SCREEN_Y(data->yarray[i+1], data, can_info));
							}
						}
					}
				}

				// recache.
				regenerate_pixbuf(can_info->offscreen_buffer);
				
				// now regenerate the other one.
				if(can_info->tracking_buffer)
				{
					dispose_buffer(can_info->tracking_buffer);
					can_info->tracking_buffer= NULL;
				}
				can_info->tracking_buffer= create_buffer_for_widget(widget);
				assert(can_info->tracking_buffer);
			}
		}
	} else {
		fprintf(stderr, "something has gone bellyup.\n");
	}
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

static void set_mouse_message_for_mode(GtkWindow *window, int mode)
{
	assert(mode>=0 && mode<NUMBER_OF_MOUSE_MODES);
	assert(NUMBER_OF_MOUSE_MODES==ARRAY_SIZE(mouse_mode_labels));
	
	set_left_footer_message(window, mouse_mode_labels[mode]);
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
	invalidate_plot(can_info->plot_window->window, TRUE);
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
			set_mouse_mode(can_info, _mouse_mode_zoom_and_clear);
			break;
		case _zoom_item_zoom:
			set_mouse_mode(can_info, _mouse_mode_zoom);
			break;
		case _zoom_item_zoom_new_window:
			set_mouse_mode(can_info, _mouse_mode_zoom_new_window);
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
			set_mouse_mode(can_info, _mouse_mode_draw_line);
			break;
		case _mouse_item_vertical_line:
			set_mouse_mode(can_info, _mouse_mode_vertical_line);
			break;
		case _mouse_item_distance:
			set_mouse_mode(can_info, _mouse_mode_distance);
			break;
		default:
			fprintf(stderr, "Unknown command index for mouse menu: %d\n", cmd_index);
			break;
	}
	
	return;
}


static void set_mouse_mode(canvasinfo *can_info, int mouse_mode)
{
	assert(can_info);
	assert(mouse_mode>=0 && mouse_mode<NUMBER_OF_MOUSE_MODES);
	
	// not sure if this line is required...
	set_active_window(can_info->canvas_num);

	if (can_info->active_plot == -1)
	{
		sprintf(msg, "There is no plot in this window.\n");
		print_msg(msg);
		return;
	}

	// set the mouse mode...
	memset(&can_info->mouse, 0, sizeof(struct mouse_tracking_data));
	can_info->mouse.mode= mouse_mode;

	// update the prompt.
	set_mouse_message_for_mode(GTK_WINDOW(can_info->plot_window->window), mouse_mode);
}



/* ----------------- other local code */
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

static void set_left_footer_message(GtkWindow *parent, const char *txt)
{
	set_plot_label_message(parent, PLOT_LABEL_LEFT_FOOTER, txt);
}

static void set_plot_label_message(GtkWindow *parent, PlotLabelID id, const char *txt)
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

static void draw_data_crosshair(
	struct offscreen_buffer *buffer, 
	const char *string,
	int xloc, 
	int yloc)
{
	GdkGC *gc= buffer->gc;
	GdkDrawable *drawable= GDK_DRAWABLE(buffer->pixmap);
	PangoLayout *pango_layout= buffer->pango_layout;
	GdkGCValues original_values;
	GdkColor crosshairs_color, white, black;
	int text_width, text_height;

	get_color_for_type(CROSSHAIRS_COLOR, &crosshairs_color);
	get_color_for_type(COLOR_WHITE, &white);
	get_color_for_type(COLOR_BLACK, &black);

	
	gdk_gc_get_values(gc, &original_values);
	gdk_gc_set_rgb_fg_color(gc, &crosshairs_color);

	gdk_draw_rectangle(drawable, gc, 
		FALSE, // TRUE?
		xloc-2, yloc-2, 
		4, 4);

	gdk_draw_line(drawable, gc,
	    xloc-5, yloc, 
	    xloc-2, yloc);

	gdk_draw_line(drawable, gc,
	    xloc+2, yloc, 
	    xloc+5, yloc);

	gdk_draw_line(drawable, gc,
	    xloc, yloc-5,
	    xloc, yloc-2);

	gdk_draw_line(drawable, gc,
	    xloc, yloc+2,
	    xloc, yloc+5);

	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);

	int y_pos= yloc- (PANGO_PIXELS(text_height)+5);
	int x_pos= xloc + 5;

	gdk_gc_set_rgb_fg_color(gc, &white);
	gdk_draw_rectangle(drawable, gc, 
		TRUE, 
		x_pos, 
		y_pos, 
		PANGO_PIXELS(text_width)+2, 
		PANGO_PIXELS(text_height)+2);

	gdk_gc_set_rgb_fg_color(gc, &black);
	gdk_draw_rectangle(drawable, gc, 
		FALSE, 
		x_pos, 
		y_pos, 
		PANGO_PIXELS(text_width)+2, 
		PANGO_PIXELS(text_height)+2);

	gdk_draw_layout(drawable, gc, 
		x_pos+1,
		y_pos+2,
		pango_layout);

	// crosshair, with a box in t
	gdk_gc_set_foreground(gc, &original_values.foreground);
}

/* cjm, 3/19/96: I think "label_type" refers to the way ticks and tick spacing is done*/
static void label_type0(canvasinfo *can_info)
{
	int plot;
	plotarray *data;
	int tickx, ticky;
	double ten = 10.000;
	char string[256];
	float xmax, xmin, ymax, ymin;
	int start_xaxis, start_yaxis, end_xaxis, end_yaxis;
	int start_x, start_y, end_x, end_y;
	int width, height, text_width, text_height;
	GdkColor color;

	GdkGC *gc= can_info->offscreen_buffer->gc;
	GdkDrawable *drawable= GDK_DRAWABLE(can_info->offscreen_buffer->pixmap);
	PangoLayout *pango_layout= can_info->offscreen_buffer->pango_layout;

	get_color_for_type(PLOT_LABEL_COLOR, &color);
	gdk_gc_set_rgb_fg_color(gc, &color);

	plot = can_info->active_plot;
	data = can_info->plots[plot];

	width = can_info->offscreen_buffer->width;
	height = can_info->offscreen_buffer->height;

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

#if false
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
#endif
  
   /* x-axis  (bottom) */
	gdk_draw_line(drawable, gc,
		start_xaxis, start_yaxis, 
		end_xaxis, start_yaxis);

	/* left y-axis  */
	gdk_draw_line(drawable, gc,
		start_xaxis, start_yaxis,
		start_xaxis, end_yaxis); 

	/* right y-axis  */
	gdk_draw_line(drawable, gc,
		end_xaxis, start_yaxis,
		end_xaxis, end_yaxis);


	/* sizes for the tick marks */
	ticky=(int)width*0.02/2;
	tickx=(int)height*0.02/2;

	/* tick and label for ymin */
	gdk_draw_line(drawable, gc,
		start_xaxis, start_y,
		start_xaxis+ticky, start_y);
	gdk_draw_line(drawable, gc,
		end_xaxis, start_y,
		end_xaxis-ticky, start_y);

	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)ymin) : ymin);

	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		start_xaxis - (PANGO_PIXELS(text_width)+10),
		start_y - PANGO_PIXELS(text_height)/2,
		pango_layout);

	/* tick and label for ymax */ 
	gdk_draw_line(drawable, gc,
		start_xaxis, end_y,
		start_xaxis+ticky, end_y);
	gdk_draw_line(drawable, gc,
		end_xaxis, end_y,
		end_xaxis-ticky, end_y);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)ymax) : ymax);

	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		start_xaxis - (PANGO_PIXELS(text_width)+10),
		end_y - PANGO_PIXELS(text_height)/2,
		pango_layout);

	/* tick and label for y mid */ 
	gdk_draw_line(drawable, gc,
		start_xaxis, end_y + (start_y - end_y)/2,
		start_xaxis+ticky, end_y + (start_y - end_y)/2);
	gdk_draw_line(drawable, gc,
		end_xaxis, end_y + (start_y - end_y)/2,
		end_xaxis-ticky, end_y + (start_y - end_y)/2);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)(ymax-ymin)/2) : (ymax+ymin)/2);
	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		start_xaxis - (PANGO_PIXELS(text_width)+10),
		end_y + (start_y - end_y)/2 - PANGO_PIXELS(text_height)/2,
		pango_layout);

	/* min x axis */
	gdk_draw_line(drawable, gc,
		start_x, start_yaxis,
		start_x, start_yaxis - ticky);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)xmin) : xmin);
	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		start_x - PANGO_PIXELS(text_width)/2,
		start_yaxis + PANGO_PIXELS(text_height) + 2,
		pango_layout);

	/* max x axis */
	gdk_draw_line(drawable, gc,
		end_x, start_yaxis,
		end_x, start_yaxis - ticky);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)xmax) : xmax);
	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		end_x - PANGO_PIXELS(text_width)/2,
		start_yaxis + PANGO_PIXELS(text_height) + 2,
		pango_layout);

	/* mid x axis */
	gdk_draw_line(drawable, gc,
		start_x + (end_x-start_x)/2, start_yaxis,
		start_x + (end_x-start_x)/2, start_yaxis - ticky);
	sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
		pow(ten, (double)(xmax-xmin)/2) : (xmax+xmin)/2);
	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		start_x + (end_x-start_x)/2 - PANGO_PIXELS(text_width)/2,
		start_yaxis + PANGO_PIXELS(text_height) + 2,
		pango_layout);

	sprintf(string, "%d.   %s  vs  %s", 
		plot+1,
		head.ch[data->col_x].name, 
		head.ch[data->col_y].name);

	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		start_xaxis, PANGO_PIXELS(text_height)+2, 
		pango_layout);

	sprintf(string,"%s",head.title);	/*add title, right justified*/

	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		end_xaxis- PANGO_PIXELS(text_width), PANGO_PIXELS(text_height)+2,
		pango_layout);
}


static void label_type1(canvasinfo *can_info)
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
	int text_width, text_height;
	GdkColor color;

	GdkGC *gc= can_info->offscreen_buffer->gc;
	GdkDrawable *drawable= GDK_DRAWABLE(can_info->offscreen_buffer->pixmap);
	PangoLayout *pango_layout= can_info->offscreen_buffer->pango_layout;

	get_color_for_type(PLOT_LABEL_COLOR, &color);
	gdk_gc_set_rgb_fg_color(gc, &color);

	plot = can_info->active_plot;
	data = can_info->plots[plot];

	width = can_info->offscreen_buffer->width;
	height = can_info->offscreen_buffer->height;

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


#if false
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
#endif

	/* x-axis  (bottom) */
	gdk_draw_line(drawable, gc,
		start_xaxis, start_yaxis, 
		end_xaxis, start_yaxis);
		
	/* left y-axis  */
	gdk_draw_line(drawable, gc,
		start_xaxis, start_yaxis,
		start_xaxis, end_yaxis); 
	
	/* right y-axis  */
	gdk_draw_line(drawable, gc,
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

		gdk_draw_line(drawable, gc,
			start_xaxis, where_y,
			start_xaxis+ticky, where_y);

		gdk_draw_line(drawable, gc,
			end_xaxis, where_y,
			end_xaxis-ticky, where_y);

		sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
			pow(ten, (double)big_ticky) : big_ticky);

		pango_layout_set_text(pango_layout, string, strlen(string));
		pango_layout_get_size(pango_layout, &text_width, &text_height);
		gdk_draw_layout(drawable, gc, 
			start_xaxis - (PANGO_PIXELS(text_width)+10),
			start_y-(int)((big_ticky-ymin)*scale_y)-PANGO_PIXELS(text_height)/2, 
			pango_layout);

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

			gdk_draw_line(drawable, gc,
				start_xaxis, where_y,
				start_xaxis+ticky, where_y);

			gdk_draw_line(drawable, gc,
				end_xaxis, where_y,
				end_xaxis-ticky, where_y);

			sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
				pow(ten, (double)big_ticky) : big_ticky);

			pango_layout_set_text(pango_layout, string, strlen(string));
			pango_layout_get_size(pango_layout, &text_width, &text_height);
			gdk_draw_layout(drawable, gc, 
				start_xaxis - (PANGO_PIXELS(text_width)+10),
				start_y-(int)((big_ticky-ymin)*scale_y)-PANGO_PIXELS(text_height)/2,
				pango_layout);

			big_ticky += pow(ten, decy);
		}
	}

	/*  x axis big tick marks */
	for (a=0;big_tickx<=stop_xmax;a++)
	{
		where_x= start_x + (int)((big_tickx-xmin)*scale_x);

		gdk_draw_line(drawable, gc,
			where_x, start_yaxis,
			where_x, start_yaxis - tickx);

		sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
			pow(ten, (double)big_tickx) : big_tickx);

		pango_layout_set_text(pango_layout, string, strlen(string));
		pango_layout_get_size(pango_layout, &text_width, &text_height);
		gdk_draw_layout(drawable, gc, 
			where_x - PANGO_PIXELS(text_width)/2,
			start_yaxis + PANGO_PIXELS(text_height) + 2,
			pango_layout);
			
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

			gdk_draw_line(drawable, gc,
				where_x, start_yaxis,
				where_x, start_yaxis - tickx);

			sprintf(string, "%.5g", (strncmp(plot_cmd, "plotlog", 7)==0) ? 
				pow(ten, (double)big_tickx) : big_tickx);
			
			pango_layout_set_text(pango_layout, string, strlen(string));
			pango_layout_get_size(pango_layout, &text_width, &text_height);
			gdk_draw_layout(drawable, gc, 
				where_x - PANGO_PIXELS(text_width)/2,
				start_yaxis + PANGO_PIXELS(text_height) + 2,
				pango_layout);

			big_tickx += pow(ten, decx);
		}
	}

	sprintf(string, "%d.   %s  vs  %s", 
		plot+1,
		head.ch[data->col_x].name, 
		head.ch[data->col_y].name);

	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		start_xaxis, PANGO_PIXELS(text_height)+2, 
		pango_layout);

	sprintf(string,"%s",head.title);	/*add title, right justified*/

	pango_layout_set_text(pango_layout, string, strlen(string));
	pango_layout_get_size(pango_layout, &text_width, &text_height);
	gdk_draw_layout(drawable, gc, 
		end_xaxis- PANGO_PIXELS(text_width), PANGO_PIXELS(text_height)+2,
		pango_layout);
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

static void clear_active_plot(canvasinfo *can_info)
{
	int i;

	for (i=0; i<MAX_PLOTS; i++)
	{
		if (can_info->alive_plots[i] == 1)
		{
			can_info->alive_plots[i] = 0;
		}
	}
	can_info->total_plots = 0;
	can_info->active_plot = -1;
  
	sprintf(msg, "Clear all plots.\n");
	print_msg(msg);
  
	display_active_window(ui_globals.active_window+1);
	display_active_plot(-1);
	
	// redraw.
	invalidate_plot(GTK_WINDOW(can_info->plot_window->window), TRUE);
}

/* ------------------------ Mouse Mode Combobox */
enum {
	MOUSE_MODE_NAME= 0,
	MOUSE_MODE_VALUE,
	NUMBER_OF_MOUSE_MODE_COLUMNS
};

void on_comboboxMouseMode_realize(GtkWidget *widget, gpointer user_data)
{
	// create the model for this combobox..
	GtkListStore *store = gtk_list_store_new (NUMBER_OF_MOUSE_MODE_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
	int ii;
	
	assert(NUMBER_OF_MOUSE_MODES==ARRAY_SIZE(mouse_mode_names));

	/* set the model */
	gtk_combo_box_set_model(GTK_COMBO_BOX(widget), GTK_TREE_MODEL(store));

	/* set the renderer */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", MOUSE_MODE_NAME);

	// setup the menu (initial conditions)
	GtkTreeIter iter;

	for(ii= 0; ii<NUMBER_OF_MOUSE_MODES; ii++)
	{
		/* Add a new row to the model */
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, MOUSE_MODE_NAME, mouse_mode_names[ii], MOUSE_MODE_VALUE, ii, -1);
	}
	
	// set the initial state...
	canvasinfo *info= canvas_info_for_widget(GTK_WIDGET(widget));
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), info->mouse.mode);
}

void on_comboboxMouseMode_changed(GtkComboBox *widget, gpointer user_data)
{
	canvasinfo *info= canvas_info_for_widget(GTK_WIDGET(widget));
	gchar *active_plot_text= gtk_combo_box_get_active_text(widget);
	int new_value= info->mouse.mode;
	
	if(active_plot_text)
	{
		int ii;
		for(ii= 0; ii<NUMBER_OF_MOUSE_MODES; ii++)
		{
			if(strcmp(active_plot_text, mouse_mode_names[ii])==0)
			{
				new_value= ii;
				break;
			}
		}
	}

	if(new_value != info->mouse.mode)
	{
		// update it.
		set_mouse_mode(info, new_value);
	}
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
		invalidate_plot(parent_gtk_window(GTK_WIDGET(widget)), TRUE);
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

//fprintf(stderr, "Active changed!\n");
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
						invalidate_plot(parent_gtk_window(GTK_WIDGET(widget)), TRUE);
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

// this was the old refresh.
void on_btn_ClearAnnotations_clicked(
	GtkButton *button,
	gpointer   user_data)
{
	canvasinfo *info= canvas_info_for_widget(GTK_WIDGET(button));
	GtkWindow *window= GTK_WINDOW(info->plot_window->window);

	invalidate_plot(window, TRUE);
}

char *csprintf(
               char *buffer,
               char *format,
               ...)
{
	va_list arglist;
	va_start(arglist, format);
//        checked_vsprintf(buffer, CSPRINTF_BUFFER_SIZE, format, arglist);
	vsprintf(buffer, format, arglist);
	va_end(arglist);

	return buffer;
}
