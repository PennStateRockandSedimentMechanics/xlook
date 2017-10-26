#include <gtk/gtk.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

#include "global.h"
#include "cmds1.h"
#include "messages.h"
#include "filtersm.h"
#include "look_funcs.h"
#include "event.h"
#include "cmd_window.h"
#include "rs_fric_window.h"
#include "ui.h"
#include "ui_static.h"

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
extern char qiparams[1024];
extern char plot_cmd[256];

ui_globals_struct ui_globals;
char msg[MSG_LENGTH];
char delayed_file_to_open[1024];


static gboolean open_filter_function(const GtkFileFilterInfo *filter_info, gpointer data);
static FileType file_type_with_path(const char *path);
static int initialize(int argc, char *argv[]);
static int set_initial_path(const char *path);
static int load_command_line_file(const char *filename);
static void print_usage(int argc, char *argv[]);
static void handle_open_filepath(const char *filename);
static gboolean startswith(const char *haystack, const char *needle);
static void on_activate_plot_window(GtkObject *object, gpointer user_data);

void quit_xlook()
{
	gtk_main_quit();
}


G_MODULE_EXPORT gboolean on_mainWindow_delete_event(
	GtkWidget *widget,
	GdkEvent  *event,
	gpointer   user_data)
{
	quit_xlook();
	
	return TRUE;
}

G_MODULE_EXPORT gboolean on_mainWindow_destroy_event(
	GtkWidget *widget,
	GdkEvent  *event,
	gpointer   user_data)
{
	quit_xlook();

	return FALSE;
}

static void initialize_globals()
{
	*delayed_file_to_open= '\0';
	
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
	int exit_code= 0;
	
	initialize_globals();
	if(initialize(argc, argv))
	{
		GtkBuilder *builder;
		GError *error= NULL;
		
		gtk_init (&argc, &argv);

		builder = gtk_builder_new ();
#ifdef STATIC_UI
		if(!gtk_builder_add_from_string(builder,  xlook_glade_data, -1, &error))
		{
			if (error != NULL)
			{
				/* Report error to user, and free error */
				fprintf (stderr, "Unable to add from string: %s\n", error->message);
				g_error_free (error);
			}
		}
#else
		gtk_builder_add_from_file (builder, "xlook.glade", NULL);
#endif
		// load the rest of them...
		ui_globals.main_window = (struct GtkWidget *)(gtk_builder_get_object (builder, "mainWindow"));
		ui_globals.command_history = (struct GtkWidget *)(gtk_builder_get_object (builder, "commandWindow"));


		// set the font.
		PangoFontDescription *desc= pango_font_description_from_string("Monospace 9");
		char *names[]= { "textview_FileInfo", "textview_Message"};
		int ii;
		for(ii= 0; ii<ARRAY_SIZE(names); ii++)
		{
			GtkWidget *w = (struct GtkWidget *)(gtk_builder_get_object (builder, names[ii]));
			gtk_widget_modify_font(w, desc);
		}

		setup_command_window(GTK_WINDOW(ui_globals.command_history));

		gtk_builder_connect_signals (builder, NULL);          
		g_object_unref (G_OBJECT (builder));

		// set the messages appropriately
		display_active_window(0);
		display_active_plot(0);
		display_active_file(0);

		gtk_widget_show(GTK_WIDGET(ui_globals.main_window));       
		
		// set the font..
//		GtkWidget *fileInfo= lookup_widget_by_name(ui_globals.main_window, "textview_FileInfo");
//		gtk_widget_modify_font(fileinfo, PangoFontDescription *font_desc);
		
		
		
		// open if we should from command line.
		if(strlen(delayed_file_to_open))
		{
			handle_open_filepath(delayed_file_to_open);
		}

		gtk_main ();
		exit_code= 0;
	} else {
		exit_code= -1;
	}
	
	return exit_code;
}

G_MODULE_EXPORT void on_fontbutton1_font_set(
	GtkFontButton *widget, 
	gpointer user_data)
{
	fprintf(stderr, "Font set was: %s", gtk_font_button_get_font_name(widget));
	
	GtkWidget *fileInfo= lookup_widget_by_name(ui_globals.main_window, "textview_FileInfo");
	PangoFontDescription *desc= pango_font_description_from_string(gtk_font_button_get_font_name(widget));
fprintf(stderr, "Desc: %p FileInfo: %p\n", desc, fileInfo);
	gtk_widget_modify_font(fileInfo, desc);
//	pango_font_description_free(desc);

	/*
	Retrieves the name of the currently selected font. This name includes style and size information as well. 
	If you want to render something with the font, use this string with pango_font_description_from_string() . 
	*/
}

/* ------------- entering return in command prompt */
G_MODULE_EXPORT void on_textEntry_Command_activate(
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
		/* remember it */
		record_command(cmd_buffer);

		/* call cmd multiplexor  */
		command_handler(cmd_buffer);
	}
}

/* ------------- Menu Handlers */
G_MODULE_EXPORT void on_quit_menu_item_activate(
	GtkObject *object,
	gpointer user_data)
{
	quit_xlook();
}

G_MODULE_EXPORT void on_menu_ShowRSFric_activate(
	GtkObject *object,
	gpointer user_data)
{
	if(ui_globals.rs_window)
	{
		// bring it to front.
		bring_rs_fric_window_to_front(ui_globals.rs_window);
	} else {
		strcpy(qiparams, "");
		qi_win_proc();
	}
}

G_MODULE_EXPORT void on_menu_ShowCommandWindow_activate(
	GtkObject *object,
	gpointer user_data)
{
	show_command_window();
}

// wow; can't believe i have to write this; i've had my libs so long I didn't realize this wasn't core.
static char *strtolower(char *str)
{
	char *src= str;
	while(*src) *src= tolower(*src), src++;

	return str;
}

G_MODULE_EXPORT void on_plot_item_activate(
	GtkObject *object,
	gpointer user_data)
{
	const gchar *label= gtk_menu_item_get_label(GTK_MENU_ITEM(object));
	char *plot1_commands[]= { "plotall", "plotover", "plotsr" };
	char *plot2_commands[]= { "plotauto", "plotlog", "plotsame", "plotscale", "pa" };
	int found= FALSE;
	char temporary[512];
	int ii;

	strcpy(temporary, label);
	strtolower(temporary);

	// expects to have ellipsis at the end..
	if(strlen(temporary)>3)
	{
		temporary[strlen(temporary)-3]= '\0';
	}

	for(ii= 0; ii<ARRAY_SIZE(plot1_commands); ii++)
	{
		if(strcmp(plot1_commands[ii], temporary)==0)
		{
			strcpy(plot_cmd, temporary);
			set_left_footer("Type the x-axis and y-axis");
			set_cmd_prompt("X-AXIS Y-AXIS: ");
			ui_globals.action = PLOT_GET_XY; 
			found= TRUE;
		}
	}
	
	if(!found)
	{
		for(ii= 0; ii<ARRAY_SIZE(plot2_commands); ii++)
		{
			if(strcmp(plot2_commands[ii], temporary)==0)
			{
				strcpy(plot_cmd, temporary);
				set_left_footer("Type the x-axis and y-axis");
				set_cmd_prompt("X-AXIS Y-AXIS: ");
				ui_globals.action = PLOT_GET_BE; 
				found= TRUE;
			}
		}
	}

	if(!found)
	{
		fprintf(stderr, "Didn't find plot command: %s\n", temporary);
	}
}

G_MODULE_EXPORT void on_save_menu_item_activate(
	GtkObject *object,
	gpointer user_data)
{
	GtkWidget *dialog;
	char previous_filename[100];
	
	*previous_filename= '\0';

	dialog = gtk_file_chooser_dialog_new ("Save Xlook File",
		GTK_WINDOW(ui_globals.main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

	if (strlen(previous_filename)==0)
	{
//		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), default_folder_for_saving);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "Untitled document");
	}
	else
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), previous_filename);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		// perform the save.
		write_proc(filename);

		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

/* ------------ Window Menu handling */

static gboolean startswith(const char *haystack, const char *needle)
{
	gboolean matches= FALSE;
	
	if(strlen(haystack)>=strlen(needle))
	{
		const char *src= needle;
		const char *dst= haystack;
		while(*src)
		{
			if(*src != *dst) break;
			src++, dst++;
		}
		
		if(!(*src)) matches= TRUE;
	}
	
	return matches;
}


G_MODULE_EXPORT void on_viewMenu_activate(
	GtkObject *object,
	gpointer user_data)
{
	GtkMenuItem *item= GTK_MENU_ITEM(object);
	
	if(gtk_menu_item_get_submenu(item) != NULL)
	{
		char buffer[100];
		char *prefix= "Plot Window";
		
		GtkMenu *menu= GTK_MENU(gtk_menu_item_get_submenu(item));
		GList *initial_list = gtk_container_get_children(GTK_CONTAINER(menu));

		// iterate the linked list and remove all the Plot Window entries...
		GList *entry= initial_list;
		while(entry)
		{
			if(GTK_IS_MENU_ITEM(entry->data) && !GTK_IS_SEPARATOR_MENU_ITEM(entry->data))
			{
				const char *existing_label= gtk_menu_item_get_label(GTK_MENU_ITEM(entry->data));
//				fprintf(stderr, "Label %s\n", existing_label);
				if(startswith(existing_label, prefix))
				{
//					fprintf(stderr, "Removing %s\n", existing_label);
					// do we have to remove it explicitly, or can we just nuke and pave?
					gtk_widget_destroy(GTK_WIDGET(entry->data));
				}
			}
			entry= entry->next;
		}
		g_list_free(initial_list);

		int ii;
		for(ii= 0; ii<MAXIMUM_NUMBER_OF_WINDOWS; ii++)
		{
			if (wininfo.windows[ii]==1)
			{
				sprintf(buffer, "%s %d", prefix, ii+1);
				GtkWidget *child= gtk_menu_item_new_with_label(buffer);
				gtk_signal_connect (GTK_OBJECT (child), "activate", GTK_SIGNAL_FUNC (on_activate_plot_window), GINT_TO_POINTER (ii));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), child);
				gtk_widget_show(child);
			}
		}
	}	
}

// internally used only (in above function)
static void on_activate_plot_window(
	GtkObject *object,
	gpointer user_data)
{
	gint window_index= GPOINTER_TO_INT(user_data);
	
	fprintf(stderr, "Activate plot window %d\n", window_index);
	set_active_window(window_index, TRUE);
}

G_MODULE_EXPORT void on_new_plot_window_item_activate(
	GtkObject *object,
	gpointer user_data)
{
	new_win_proc();
}

/* ------------- Opening Files */
G_MODULE_EXPORT void on_open_menu_item_activate( // as expected, the signals can't be static or they won't latebind.
	GtkObject *object,
	gpointer user_data)
{
	GtkWidget *dialog;
	
	dialog= gtk_file_chooser_dialog_new("Open Xlook File", 
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
		handle_open_filepath(filename);
	    g_free (filename);
	}
	// dialog takes ownership of the filter, so we don't have to do anything to free it.
	gtk_widget_destroy(dialog);
}

static void handle_open_filepath(const char *filename)
{
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
	FILE *fp= fopen(path, "rb");
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
static int initialize(int argc, char *argv[])
{
	int i;
	int abort_launch= FALSE;

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


//	if (argc > 1)
	{
		if (argc == 2 && strcmp(argv[1], "-?")!=0 && strcmp(argv[1], "-h")!=0 && strcmp(argv[1], "--help")!=0)
		{
			set_initial_path(argv[1]);
		}
		else
		{
			int c;
			static struct option long_options[] = {
				{"path", 1, 0, 0}, // name, has_arg, flag, val
				{"file", 1, 0, 0},
				{"help", 0, 0, 0},
				{NULL, 0, NULL, 0}
			};
			int option_index = 0;
			while ((c = getopt_long_only(argc, argv, "p:f:?h", long_options, &option_index)) != -1) 
			{
				switch (c) 
				{
					case 0:
						switch(option_index)
						{
							case 0: // path
								abort_launch= set_initial_path(optarg);
								break;
							case 1: // file
								abort_launch= load_command_line_file(optarg);
								break;
							case 2: // file
								abort_launch= TRUE;
								break;
						}
						break;
					case 'p':
						abort_launch= set_initial_path(optarg);
						break;
					case 'f':
						abort_launch= load_command_line_file(optarg);
						break;
					case '?':
					case 'h':
					default:
						abort_launch= TRUE;
						break;
				}
			}

			if (optind < argc) 
			{
//				printf ("non-option ARGV-elements: ");
				while (optind < argc)
				{
//					printf ("%s ", argv[optind++]);
				}
//				printf ("\n");
			}
			
			if(abort_launch)
			{
				print_usage(argc, argv);
			}
		}
	}
	
	return !abort_launch;
}

static int set_initial_path(const char *path)
{
	int abort_launch= FALSE;
	
	if(path != NULL)
	{
		if (access(path, 4) == 0)
		{
			strcpy(default_path, path);
			fprintf(stderr, "Pathname for DOIT files is %s\n", default_path);
			if (default_path[strlen(default_path)-1] != '/')
				strcat(default_path, "/");
		}
		else
		{
			fprintf(stderr, "Inaccessible pathname \n%s IGNORED\n", path);
		}
	} else {
		fprintf(stderr, "Pathname missing from parameter!\n");
		abort_launch= TRUE;
	}
	
	return abort_launch;
}

static int load_command_line_file(const char *filename)
{
	int abort_launch= FALSE;
	
	// should set the path if the filename is longer than just a name.
	if(filename != NULL)
	{
		strcpy(delayed_file_to_open, filename);
	} else {
		fprintf(stderr, "Filename missing from parameter!\n");
		abort_launch= TRUE;
	}
	
	return abort_launch;
}

static void print_usage(int argc, char *argv[])
{
	fprintf(stdout, "Usage: %s [args] where args are:\n", argv[0]);
	fprintf(stdout, "  --path PATHNAME - Sets the default path to PATHNAME\n");
	fprintf(stdout, "  --file FILENAME - Launches the application then runs doit(FILENAME)\n");
	fprintf(stdout, "  --help - Prints this usage information.\n");
}