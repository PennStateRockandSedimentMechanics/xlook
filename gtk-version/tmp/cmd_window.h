/* cmd_window.h */

void record_command(char *cmd);
void show_command_window();

#ifdef __GTK_H__
void setup_command_window(GtkWindow *window);
#endif