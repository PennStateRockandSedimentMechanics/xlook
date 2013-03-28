/* cmds1.c */
void new_win_proc(void);
void qi_win_proc(void);
void set_active_window(int win_num, int bring_to_front);
void set_active_plot(int i);
void del_plot_proc(int pn);
void kill_win_proc(int wn);
int get_old_active_window(int wn);
int get_new_plot_num(int alive_plots[10]);
void plotting_error(int pn);
void do_plot(char cmd[256]);
void write_proc(char arg[256]);
void read_proc(const char cmd[256]);
void doit_proc(const char arg[256]);
void set_path_proc(char arg[256]);
void all_final_proc(char cmd[256]);
