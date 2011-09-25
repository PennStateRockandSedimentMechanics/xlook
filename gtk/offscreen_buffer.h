/*
	offscreen_buffer.h
	
	9.23.2011- rdm created (ryan@martellventures.com)
*/


struct offscreen_buffer
{
	GdkGC *gc;
	GdkPixmap *pixmap;
	GdkPixbuf *pixbuf;
	PangoLayout *pango_layout;
	int width;
	int height;
};


struct offscreen_buffer *create_buffer_for_widget(GtkWidget *widget);

void regenerate_pixbuf(struct offscreen_buffer *buffer);

void dispose_buffer(struct offscreen_buffer *buffer);