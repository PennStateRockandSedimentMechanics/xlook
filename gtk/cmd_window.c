/* cmd_window.h */

#include <gtk/gtk.h>
#include <math.h>
#include <assert.h>

#include "config.h"
#include "global.h"
#include "messages.h"
#include "ui.h"
#include "cmd_window.h"

enum
{
  COMMAND_NAME_COLUMN= 0,
  NUMBER_OF_COLUMNS
};

/* ------------ prototypes */
static GtkWidget *create_view_and_model(GtkWindow *window);
static GtkWidget *create_view_and_model (GtkWindow *window);
static GtkWidget *treeviewForCommandHistory();
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);


void setup_command_window(GtkWindow *window)
{
	// setup the tree model...
	create_view_and_model(window);
	
	// don't delete the window- hide it on delete.
	g_signal_connect (G_OBJECT(window), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
}

void show_command_window()
{
	gtk_widget_show(GTK_WIDGET(ui_globals.command_history));
}

void record_command(char *cmd)
{
	GtkTreeView *treeview= GTK_TREE_VIEW(treeviewForCommandHistory());

	assert(treeview);
	
	// now append to the model for the treeview....
	GtkTreeModel *model= gtk_tree_view_get_model(treeview);
	if(model)
	{
		GtkTreeIter iter;

		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, COMMAND_NAME_COLUMN, cmd, -1);
	}
}

/* ----------- private code */
void on_btn_CloseCommandWindow_clicked(GtkButton *button, gpointer user_data)
{
	// close the window
	gtk_widget_hide(GTK_WIDGET(parent_gtk_window(GTK_WIDGET(button))));
}

static GtkTreeModel *create_and_fill_model(void)
{
  return GTK_TREE_MODEL(gtk_list_store_new (NUMBER_OF_COLUMNS, G_TYPE_STRING));
}

static GtkWidget *treeviewForCommandHistory()
{
	return lookup_widget_by_name(GTK_WIDGET(ui_globals.command_history), "treeview_Commands");
}

static GtkWidget *create_view_and_model (GtkWindow *window)
{
	GtkCellRenderer     *renderer;
	GtkTreeModel        *model;
	GtkWidget           *view;
	GtkTreeSelection *select;

	view = lookup_widget_by_name(GTK_WIDGET(window), "treeview_Commands");
	

	/* Create the model */
	model = create_and_fill_model ();

	/* --- Column #1 --- */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), -1, "Command", renderer, "text", COMMAND_NAME_COLUMN, NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);

	/* The tree view has acquired its own reference to the
	*  model, so we can drop ours. That way the model will
	*  be freed automatically when the tree view is destroyed */

	g_object_unref (model);

	/* Setup the selection handler */
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed", G_CALLBACK (tree_selection_changed_cb), NULL);

	return view;
}

static void tree_selection_changed_cb(
	GtkTreeSelection *selection, 
	gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *command;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, COMMAND_NAME_COLUMN, &command, -1);

		// set the UI to match the history.
		set_command_text(command);

		g_free (command);
	}
}
