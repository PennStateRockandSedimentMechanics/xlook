/* cmd_window.h */

#include <gtk/gtk.h>
#include <math.h>
#include <assert.h>

#include "config.h"
#include "global.h"
#include "messages.h"
#include "ui.h"
#include "cmd_window.h"

struct command_window {
	GtkWindow *window;
};

// FIXME
struct command_window *open_command_window()
{
	return NULL;
}

void record_command(char *cmd)
{
	fprintf(stderr, "Record: %s\n", cmd);
}