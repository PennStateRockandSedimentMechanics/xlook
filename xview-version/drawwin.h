#include <xview/xview.h>
#include <xview/xv_xrect.h>

/* drawwin.c */
void label_type0(void);
void label_type1(void);
void redraw_proc(Canvas canvas, Xv_Window paint_window, Display *dpy, Window win, Xv_xrectlist *area);
void clear_canvas_proc(Canvas canvas);
void redraw_all_proc(Canvas canvas, Xv_Window paint_window, Display *dpy, Window win, Xv_xrectlist *area);

