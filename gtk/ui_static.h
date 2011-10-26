/*
	This file exists to have all the UI stuff from Glade compiled into the application.
	
	I don't like this, as it prevents modification of the UI, but Chris wants to have a single file that you can move around (which makes sense as well)
*/

// If this is defined, the glade files are compiled into the application...
#define STATIC_UI


#ifdef STATIC_UI
extern const char *xlook_glade_data;
extern const char *rs_fric_window_glade_data;
extern const char *plot_window_glade_data;
#endif