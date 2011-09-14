#include <gtk/gtk.h>
#include <stdio.h>
#include <assert.h> // FIXME


#include "global.h"
#include "ui.h"


static GtkTextView *textViewFromID(UITextFieldIdentifier id);
//static GtkWidget *lookup_widget_by_name(GtkWidget *parent, char *name);
static GtkLabel *labelFromID(UILabelFieldIdentifier id);

void ui_textfield_clear(UITextFieldIdentifier id)
{
// textsw_reset(fileinfo_window, 0, 0);	
} 

void ui_textfield_normalize(UITextFieldIdentifier id)
{
	// textsw_normalize_view(fileinfo_window, 1);	
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
			gtk_text_buffer_insert_at_cursor(buffer, txt, len);
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


GtkWidget *lookup_widget_by_name(GtkWidget *parent, char *name)
{
	GtkWidget *result= NULL;
	
	if(gtk_widget_get_name(parent)!=NULL && strcmp(gtk_widget_get_name(parent), name)==0)
	{
		result= parent;
	} else {
		if(GTK_IS_BIN(parent)) {
		    GtkWidget *child = gtk_bin_get_child(GTK_BIN(parent));
			result= lookup_widget_by_name(child, name);
		} else if(GTK_IS_CONTAINER(parent)) {
			GList *initial_list = gtk_container_get_children(GTK_CONTAINER(parent));

			// iterate the linked list
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
	
	return result;
}