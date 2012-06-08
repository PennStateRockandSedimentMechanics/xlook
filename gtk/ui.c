#include <gtk/gtk.h>
#include <stdio.h>
#include <assert.h>


#include "global.h"
#include "ui.h"


static GtkTextView *textViewFromID(UITextFieldIdentifier id);
//static GtkWidget *lookup_widget_by_name(GtkWidget *parent, char *name);
static GtkLabel *labelFromID(UILabelFieldIdentifier id);

void ui_textfield_clear(UITextFieldIdentifier id)
{
	GtkTextView *textView= textViewFromID(id);
	if(textView)
	{
		char *clear= "";
		GtkTextBuffer *buffer= gtk_text_view_get_buffer(textView);
		assert(buffer);
		gtk_text_buffer_set_text(buffer, clear, strlen(clear));
	}
} 

void ui_textfield_normalize(UITextFieldIdentifier id)
{
return;
	// textsw_normalize_view(fileinfo_window, 1);	
	GtkTextView *textView= textViewFromID(id);
	if(textView)
	{
		GtkTextBuffer *buffer= gtk_text_view_get_buffer(textView);
        GtkTextIter iter;

		assert(buffer);

        /* get end iter */
        gtk_text_buffer_get_end_iter (buffer, &iter);

        /* scroll to end iter */
        gtk_text_view_scroll_to_iter(textView, &iter, 0.0, FALSE, 0, 0);
	}
} 

void set_command_text(char *txt)
{
	GtkEntry *entry= GTK_ENTRY(lookup_widget_by_name(GTK_WIDGET(ui_globals.main_window), "textEntry_Command"));
	assert(entry);

	gtk_entry_set_text(entry, txt);
	gtk_editable_set_position(GTK_EDITABLE(entry), -1); // cursor at end...
}


void ui_textfield_insert(
	UITextFieldIdentifier id, 
	char *txt, 
	int len)
{
	if(len)
	{
		GtkTextView *textView= textViewFromID(id);
		if(textView)
		{
			GtkTextBuffer *buffer= gtk_text_view_get_buffer(textView);
			assert(buffer);

			if(FALSE)
			{
				/* insert the text */
				gtk_text_buffer_insert_at_cursor(buffer, txt, len);
			} else {
				GtkTextIter end_start_iter;
				GtkTextMark *insert_mark;

				/* get end iter */
				gtk_text_buffer_get_end_iter (buffer, &end_start_iter);

				/* set the text in the buffer  */
				gtk_text_buffer_insert(buffer, &end_start_iter, txt, len);

				/* get end iter again */
				gtk_text_buffer_get_end_iter (buffer, &end_start_iter); 

				/* get the current ( cursor )mark name */
				insert_mark = gtk_text_buffer_get_insert(buffer);

				/* move mark and selection bound to the end */
				gtk_text_buffer_place_cursor(buffer, &end_start_iter);

				/* scroll to the end view */
				gtk_text_view_scroll_to_mark(textView, insert_mark, 0.0, TRUE, 0.0, 1.0);
			}
		}
	}
}

void ui_label_set(
	UILabelFieldIdentifier id, 
	char *txt)
{
	// set the label with the value
//	fprintf(stderr, "Set Label> %d - %s\n", id, txt);
	GtkLabel *label= labelFromID(id);
	assert(label);
	gtk_label_set_text(label, txt);
}

static GtkLabel *labelFromID(UILabelFieldIdentifier id)
{
	char *textfield_names[]= {"label_CommandPrompt", "label_LeftFooter", "label_ActiveWindowCount", "label_ActivePlot", "label_ActiveFilename" };
	
	assert(id>=0 && id<ARRAY_SIZE(textfield_names));
	return (GtkLabel *) lookup_widget_by_name(GTK_WIDGET(ui_globals.main_window), textfield_names[id]);
}


static GtkTextView *textViewFromID(UITextFieldIdentifier id)
{
	char *textfield_names[]= {"textview_Message", "textview_FileInfo" };
	
	assert(id>=0 && id<ARRAY_SIZE(textfield_names));
	return (GtkTextView *) lookup_widget_by_name(GTK_WIDGET(ui_globals.main_window), textfield_names[id]);
}


GtkWidget *lookup_widget_by_name(GtkWidget *parent, const char *name)
{
	GtkWidget *result= NULL;

fprintf(stderr, "Lookup widget by name parent: %p Name: %s\n", parent, name);
if(gtk_widget_get_name(parent)!=NULL) fprintf(stderr, "Current name: %s\n", gtk_widget_get_name(parent));

	if(gtk_widget_get_name(parent)!=NULL && strcmp(gtk_widget_get_name(parent), name)==0)
	{
		result= parent;
	} else {
		if(GTK_IS_BIN(parent)) {
		    GtkWidget *child = gtk_bin_get_child(GTK_BIN(parent));
fprintf(stderr, "Recursing into bin..\n");
			result= lookup_widget_by_name(child, name);
		} else if(GTK_IS_CONTAINER(parent)) {
			GList *initial_list = gtk_container_get_children(GTK_CONTAINER(parent));

			// iterate the linked list
fprintf(stderr, "Iterating list...\n");
			GList *entry= initial_list;
			while(entry)
			{
				result= lookup_widget_by_name(GTK_WIDGET(entry->data), name);
				if(result) break;
				entry= entry->next;
			}
			g_list_free(initial_list);
		}
	}
       
fprintf(stderr, "Returning: %p\n", result);
	return result;
}

GtkWindow *parent_gtk_window(GtkWidget *widget)
{
	GtkWindow *result= NULL;
	
	
	if(GTK_IS_WINDOW(widget))
	{
		result= GTK_WINDOW(widget);
	} else {
		GtkWidget *parent= widget;
		
		// walk up the parents...
		do {
			// this looks wrong, but should be correct unless there is really weird multithreading going on.
			g_object_get (parent, "parent", &parent, NULL);
//			fprintf(stderr, "Parent: %p\n", parent);
			if(GTK_IS_WINDOW(parent))
			{
				result= GTK_WINDOW(parent);
			}
			g_object_unref(parent);
			
			
		} while(result==NULL && parent!=NULL);
	}
	
	return result;
}


void get_color_for_type(UIColorIdentifier c, GdkColor *color)
{
	memset(color, 0, sizeof(GdkColor));
	switch(c)
	{
		case COLOR_WHITE:
			color->red= color->green= color->blue= 0xffff;
			break;
		case COLOR_BLACK:
			// already memset to 0
			break;
		case ACTIVE_PLOT_COLOR:
			color->blue= 0xffff;
			break;
		case INACTIVE_PLOT_COLOR:
		case PLOT_LABEL_COLOR:
		case PLOT_VERTICAL_LINE_COLOR:
			// already memset to black
			break;
		case CROSSHAIRS_COLOR:
			color->red= 0xffff;
			break;
		default:
			fprintf(stderr, "Invlaid color identifier: %d\n", c);
			break;
	}
}
