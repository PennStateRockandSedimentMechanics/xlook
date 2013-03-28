/* mouse.c */
void set_line_plot(void);
void do_line_plot(void);
void set_mouse_mu_proc(void);
void do_mouse_mu(void);
void set_dist_proc(void);
void do_dist(void);
void set_zoom(void);
void zoom_get_pt(int xloc, int yloc, int p);
void zoom(void);
void print_xy(int xloc, int yloc);
void print_xyrow(int xloc, int yloc, int draw_string);
int get_row_number(int nrow, double x1, double y1);
void draw_xhair(double xval, double yval);
void draw_crosshair(int xloc, int yloc);
void clr_ap(void);
