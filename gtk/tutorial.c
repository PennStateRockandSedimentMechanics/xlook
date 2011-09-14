#include <gtk/gtk.h>
#include <unistd.h>

#include "global.h"
#include "cmds1.h"
#include "messages.h"
#include "filtersm.h"
#include "look_funcs.h"
#include "event.h"


short state0_image[] = {
#include "xlook.ico"
};


typedef enum {
	_file_type_script=0,
	_file_type_data,
	_file_type_unrecognized,
	NUMBER_OF_FILE_TYPES
} FileType;


extern int plot_error;
extern char default_path[80];

ui_globals_struct ui_globals;
char msg[MSG_LENGTH];



static gboolean open_filter_function(const GtkFileFilterInfo *filter_info, gpointer data);
static FileType file_type_with_path(const char *path);
static void initialize(int argc, char *argv[]);


void quit_xlook()
{
	gtk_main_quit();
}

void on_window_destroy (
	GtkObject *object, 
	gpointer user_data)
{
	quit_xlook();
}

static void initialize_globals()
{
	memset(&ui_globals, 0, sizeof(ui_globals));
	ui_globals.active_window = -1;
	ui_globals.old_active_window = -1;
	ui_globals.total_windows = -1;
	ui_globals.action = 0;
	ui_globals.cmd_num = 0;


	ui_globals.tickfontheight= 10;
	ui_globals.tickfontwidth= 10;
	ui_globals.titlefontheight= 10;
	ui_globals.titlefontwidth= 10;
#if FIXME
  titlefontheight=(int)xv_get(titlefont, FONT_DEFAULT_CHAR_HEIGHT);
  titlefontwidth=(int)xv_get(titlefont, FONT_DEFAULT_CHAR_WIDTH);

  tickfontheight=(int)xv_get(tickfont, FONT_DEFAULT_CHAR_HEIGHT);
  tickfontwidth=(int)xv_get(tickfont, FONT_DEFAULT_CHAR_WIDTH);
#endif
}

int main(
	int argc, 
	char *argv[])
{
	GtkBuilder *builder;
	
	initialize_globals();
	initialize(argc, argv);


	gtk_init (&argc, &argv);

	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, "xlook.glade", NULL);

	// load the rest of them...
	ui_globals.main_window = GTK_WIDGET (gtk_builder_get_object (builder, "mainWindow"));

	gtk_builder_connect_signals (builder, NULL);          
	g_object_unref (G_OBJECT (builder));

	// set the messages appropriately
	display_active_window(0);
	display_active_plot(0);
	display_active_file(0);

	gtk_widget_show (ui_globals.main_window);       
	gtk_main ();

	return 0;
}

/* ------------- entering return in command prompt */
void on_textEntry_Command_activate(
	GtkObject *object,
	gpointer user_data)
{
	char cmd_buffer[256];

	// copy it in
	strncpy(cmd_buffer, gtk_entry_get_text(GTK_ENTRY(object)), ARRAY_SIZE(cmd_buffer)-1);
	cmd_buffer[ARRAY_SIZE(cmd_buffer)-1]= 0;

	// clear it out..
	gtk_entry_set_text(GTK_ENTRY(object), "");

	/*only do something if a command was entered. Don't do anything if user just hit return*/
	if(strcspn(cmd_buffer," ,\n\t"))
	{
#ifdef FIXME
		xv_set(cmd_hist_panel_list, PANEL_LIST_INSERT, cmd_num, PANEL_LIST_STRING, cmd_num, cmd_text, NULL);
		cmd_num++;
#endif

		/* call cmd multiplexor  */
		command_handler(cmd_buffer);
	}
}

/* ------------- Menu Handlers */
void on_quit_menu_item_activate(
	GtkObject *object,
	gpointer user_data)
{
	quit_xlook();
}

/* ------------- Opening Files */
void on_open_menu_item_activate( // as expected, the signals can't be static or they won't latebind.
	GtkObject *object,
	gpointer user_data)
{
	GtkWidget *dialog;
	
	dialog= gtk_file_chooser_dialog_new("Open XView File", 
		GTK_WINDOW(ui_globals.main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	
	// create the filter.
	GtkFileFilter *filter= gtk_file_filter_new(); // DIspose where?
	gtk_file_filter_set_name(filter, "XLook Files");
	gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, open_filter_function, NULL, NULL);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename= gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		switch(file_type_with_path(filename))
		{
			case _file_type_script:
			    doit_proc(filename);
				break;
			case _file_type_data:
				read_proc(filename);
				break;
			default:
				break;
		}		
	    g_free (filename);
	}
	// dialog takes ownership of the filter, so we don't have to do anything to free it.
	gtk_widget_destroy(dialog);
}

static gboolean open_filter_function(
	const GtkFileFilterInfo *filter_info, 
	gpointer data)
{
	return (file_type_with_path(filter_info->filename)!=_file_type_unrecognized);
}


static FileType file_type_with_path(const char *path)
{
	FileType type= _file_type_unrecognized;
	FILE *fp= fopen(path, "r");
	if(fp)
	{
		char *key= "begin";
		int key_length= strlen(key);
		char buffer[128];
		int length_read;

		length_read= fread(buffer, 1, key_length, fp);
		if(length_read==key_length && strncmp(key, buffer, key_length)==0)
		{
			type= _file_type_script;
		} else {
			/* in filtersm.h */
			/*	return is 16, 32, 64 or 0 (0 indicates an error) */
			int version= header_version(fp, FALSE);
			if(version != 0)
			{
				type= _file_type_data;
			}
		}
	
		fclose(fp);
	}	
	
	return type;
}


// FIXME: Move these into globals- if at all possible
// max_col, max_row, plot_error, arrayx, arrayy, darray
static void initialize(int argc, char *argv[])
{
	int i;

	plot_error = -1;

	max_col = 33;			/*start with 33, which is MAX limit*/
	max_row = 10000;
	for (i = 0; i < max_col; ++i)
		darray[i] = (double *)malloc((unsigned)(max_row*sizeof(double)));
	/*darray[i] = (float *)malloc((unsigned)(max_row*sizeof(float)));*/
	/*darray[i] = (float *)calloc((unsigned)max_row, (unsigned)4);*/

	arrayx = (double *)malloc((unsigned)(max_row*sizeof(double)));
	arrayy = (double *)malloc((unsigned)(max_row*sizeof(double)));
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
			fprintf(stderr, "EXPECTED: %s default_path\n", argv[0]);
			exit(-1);
		}
	}
}
