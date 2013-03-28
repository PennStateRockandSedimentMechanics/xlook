/*
	plot_window.h
	
	9.23.2011- rdm created (ryan@martellventures.com)
*/

struct plot_window;

/* can.c */
struct plot_window *create_plot_window(void);
void kill_plot_window(struct plot_window *pw);

void bring_plot_window_to_front(struct plot_window *pw);

/* Used by cmds1.c (and others) */
void set_active_plot_in_window(struct plot_window *pw, int i);
void remove_plot_in_window(struct plot_window *pw, int pn);
void invalidate_plot_window(struct plot_window *pw);

void clear_multiple_plots(struct plot_window *pw, int *intar2); // intar2?  what's that supposed to be?