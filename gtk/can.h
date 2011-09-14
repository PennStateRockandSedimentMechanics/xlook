// Mabye create a struct canvas * opaque type?

struct plot_window;

/* can.c */
struct plot_window *create_plot_window(void);
void kill_plot_window(struct plot_window *pw);

/* Used by cmds1.c (and others) */
void set_active_plot_in_window(struct plot_window *pw, int i);
void remove_plot_in_window(struct plot_window *pw, int pn);
void invalidate_plot_window(struct plot_window *pw);

