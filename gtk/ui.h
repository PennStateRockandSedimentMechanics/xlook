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

void ui_textfield_clear(UITextFieldIdentifier id); // textsw_reset(fileinfo_window, 0, 0);
void ui_textfield_normalize(UITextFieldIdentifier id); // textsw_normalize_view(fileinfo_window, 1);
void ui_textfield_insert(UITextFieldIdentifier id, char *txt, int len); // check and handle 0 length
void ui_label_set(UILabelFieldIdentifier id, char *txt);

#ifdef __GTK_H__
GtkWidget *lookup_widget_by_name(GtkWidget *parent, char *name);
#endif