typedef enum {
	TEXT_FIELD_MESSAGE_WINDOW= 0,
	TEXT_FIELD_FILE_INFO_WINDOW,
	NUMBER_OF_TEXT_FIELDS
} UITextFieldIdentifier;

typedef enum {
	LABEL_COMMAND_PROMPT,
	LABEL_LEFT_FOOTER,
	LABEL_ACTIVE_WINDOW_COUNT,
	LABEL_ACTIVE_PLOT,
	LABEL_ACTIVE_FILENAME
} UILabelFieldIdentifier;


typedef enum {
	COLOR_WHITE= 0,
	COLOR_BLACK,
	ACTIVE_PLOT_COLOR,
	INACTIVE_PLOT_COLOR,
	CROSSHAIRS_COLOR,
	PLOT_LABEL_COLOR,
	PLOT_VERTICAL_LINE_COLOR,
	NUMBER_OF_COLORS
} UIColorIdentifier;


void ui_textfield_clear(UITextFieldIdentifier id); // textsw_reset(fileinfo_window, 0, 0);
void ui_textfield_normalize(UITextFieldIdentifier id); // textsw_normalize_view(fileinfo_window, 1);
void ui_textfield_insert(UITextFieldIdentifier id, char *txt, int len); // check and handle 0 length
void ui_label_set(UILabelFieldIdentifier id, char *txt);

void set_command_text(char *txt);

#ifdef __GTK_H__
GtkWidget *lookup_widget_by_name(GtkWidget *parent, const char *name);
GtkWindow *parent_gtk_window(GtkWidget *widget);
void get_color_for_type(UIColorIdentifier c, GdkColor *color);
char *safe_get_widget_name(GtkWidget *widget);
#endif