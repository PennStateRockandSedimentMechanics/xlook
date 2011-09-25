/*
	offscreen_buffer.c
	
	9.23.2011- rdm created (ryan@martellventures.com)
*/

#include <gtk/gtk.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


#include "offscreen_buffer.h"

struct offscreen_buffer *create_buffer_for_widget(GtkWidget *widget)
{
	struct offscreen_buffer *result= (struct offscreen_buffer *) malloc(sizeof(struct offscreen_buffer));
	if(result)
	{
		// clear
		memset(result, 0, sizeof(struct offscreen_buffer));
		
		// create the pixmap..
		result->pixmap= gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);
		if(result->pixmap)
		{
			// store these.
			result->width= widget->allocation.width;
			result->height= widget->allocation.height;
			
			// create the GC...
			result->gc= gdk_gc_new(result->pixmap);
			if(result->gc)
			{
				// success!
				result->pango_layout= gtk_widget_create_pango_layout(widget, NULL);
				if(result->pango_layout)
				{
					// success!!
				} else {
					// clear it out.
					dispose_buffer(result);
					result= NULL;
				}
			} else {
				// clear it out.
				dispose_buffer(result);
				result= NULL;
			}
		} else {
			// Failure
			dispose_buffer(result);
			result= NULL;
		}
	}
	
	return result;
}

void regenerate_pixbuf(struct offscreen_buffer *buffer)
{
	if(buffer->pixbuf)
	{
		g_object_unref(G_OBJECT(buffer->pixbuf));
		buffer->pixbuf= NULL;
	}
	
	buffer->pixbuf= gdk_pixbuf_get_from_drawable(NULL, GDK_DRAWABLE(buffer->pixmap),
		NULL, 0, 0, 0, 0, buffer->width, buffer->height);
	
	assert(buffer->pixbuf);
}


void dispose_buffer(struct offscreen_buffer *buffer)
{
	if(buffer)
	{
		if(buffer->pixbuf)
		{
			g_object_unref(G_OBJECT(buffer->pixbuf));
			buffer->pixbuf= NULL;
		}

		if(buffer->pango_layout)
		{
			g_object_unref(G_OBJECT(buffer->pango_layout));
			buffer->pango_layout= NULL;
		}
		
		if(buffer->gc)
		{
			g_object_unref(G_OBJECT(buffer->gc));
			buffer->gc= NULL;
		}
		
		if(buffer->pixmap)
		{
			g_object_unref(G_OBJECT(buffer->pixmap));
			buffer->pixmap= NULL;
		}
	}

	free(buffer);
}