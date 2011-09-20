#include <gtk/gtk.h>
#include <math.h>
#include <assert.h>

#include "config.h"
#include "global.h"
#include "messages.h"
#include "ui.h"
#include "rs_fric_window.h"
#include "strcmd.h" // for nocom
#include "look_funcs.h" // for check_row
#include "event.h" // for command_handler
#include "cmd_window.h"

extern char qiparams[1024]; /* string of input parameters */ 

typedef char parameter_str[20]; /* parameter string */

enum {
	_parameter_law_options= 0,
	_parameter_disp_col,
	_parameter_mu_col,
	_parameter_mu_fit_col,
	_parameter_first_row, 
	_parameter_vs_row,
	_parameter_last_row,
	_parameter_weight_row,
	_parameter_lin_term,
	_parameter_c_tol,
	_parameter_lambda, 
	_parameter_wc,
	_parameter_stiff, 
	_parameter_v_initial,
	_parameter_v_final,
	_parameter_mu_initial,
	_parameter_a,
	_parameter_b1,
	_parameter_dc1,

	// optional parameters based on the other settings.
	_parameter_b2, // 19
	_parameter_dc2, // 20
	
	// newly added parameters (to keep them all in one place)
	_parameter_alt_wt, // 21 (this replaces alt_wt_str)
	_parameter_mvs_params, // 22 (this replaces mvs_params)
	NUMBER_OF_PARAMETERS
};

/* ------------ structures */
struct rs_fric_window
{
	GtkWindow *window;
	parameter_str parameter_strs[NUMBER_OF_PARAMETERS]; /* array of parameter strings */
	/* to manage dynamically created text inputs */
	int alt_wt_flag, mvs_flag, second_set_flag;
	int dc2_flag;
	struct rs_fric_window *next;
};

struct command_line_parsing_data {
	int token_count;
	char qi_cmd_str[160];
};


/* ------------ strings */
static const char *parameter_names[]= { "options", "disp_col", "mu_col", "mu_fit_col", "first_row", "vs_row", "last_row", "weight_row", "lin_term", "c_tol", 
	"lambda", "wc", "stiff", "v_initial", "v_final", "mu_initial", "a", "b1", "dc1", "b2", "dc2", "row_number", "qi_mvs" };
static const char *parameter_error_messages[]= { "No Options Selected", "Missing disp_col", "Missing mu_col", "Missing mu_fit_col", "Missing first_row", "Missing vs_row", 
	"Missing last_row", "Missing weight_row", "Missing lin_term", "Missing c_tol", "Missing lambda", "Missing wc", "Missing stiff", "Missing v_initial",
	"Missing v_final", "Missing mu_initial", "Missing a", "Missing b1", "Missing dc1", "Missing b2","Missing dc2", "Missing row number", "Missing mvs parameters." };	

static const char *stateVariableButtons[]= { "btn_OneStateVariable", "btn_TwoStateVariables" };
static const char *twoVariableComponentNames[]= { "lbl_b2", "lbl_dc2", "b2", "dc2" };
static const char *modelDirectionButtons[]= { "btn_ModelForward", "btn_ModelInversion" };
static const char *weightingButtons[]= { "btn_WeightingNormal", "btn_WeightingAlternative" };
static const char *velocityButtons[]= { "btn_VelocitySingle", "btn_VelocityMultiple" };
static const char *multipleVelocityComponentNames[]= { "lbl_QiMVS", "qi_mvs" };
static const char *alternativeWeightingComponentNames[]= { "lbl_RowNumber", "row_number" };

/* ------------ local globals */
static struct rs_fric_window *first_window= NULL;

/* ------------------- local prototypes */
static void parse_command_line(struct rs_fric_window *rs_fric_window, char *command_line, struct command_line_parsing_data *data);
static void toggle_buttons(GtkWidget *widget, const char *names[]);
static void set_initial_toggle_button_state_by_index(GtkWindow* window, int index, const char *names[]);
static struct rs_fric_window *rs_fric_window_from_gtk_window(GtkWindow *window);
static void update_parameters_display(struct rs_fric_window *rs_fric_window, int start, int last);
static void left_footer(struct rs_fric_window *rs_fric_window, const char *msg);
static void change_widget_visibility(GtkWindow *window, const char *names[], int length, gboolean show);
static const gchar *get_parameter_from_display(struct rs_fric_window *rs_fric_window, int index);
static void error_message(struct rs_fric_window *rs_fric_window, int index);

/* ---------------------- code */
struct rs_fric_window *create_rs_fric_window(void)
{
	struct rs_fric_window *result= NULL;
	GtkBuilder *builder = gtk_builder_new ();
	
	// load the builder
	gtk_builder_add_from_file (builder, "rs_fric_window.glade", NULL);

	// load the rest of them...
	GtkWidget *window = GTK_WIDGET (gtk_builder_get_object (builder, "rsfricWindow"));
	gtk_builder_connect_signals (builder, NULL);
	g_object_unref (G_OBJECT (builder));

	// create the canvasinfo (this is sorta silly)
	fprintf(stderr, "RS Fric Window is %p\n", window);

	// must do this before it is realized.
	result= malloc(sizeof(struct rs_fric_window));
	if(result)
	{
		// clear.
		memset(result, 0, sizeof(struct rs_fric_window));
		
		result->window= GTK_WINDOW(window);

		// add to our linked list of windows.
		result->next= first_window;
		first_window= result;

		// update the parameters...
		update_parameters(result);
		
		// and show the window.
		gtk_widget_show (window);
	} else {
		// close out, failure.
		gtk_widget_destroy(GTK_WIDGET(window));
	}

	return result;
}

void update_parameters(struct rs_fric_window *rs_fric_window) // was update_params
{
	GtkWindow *window= rs_fric_window->window;
	struct command_line_parsing_data cmd_params;
	
	// parse the command line.
	parse_command_line(rs_fric_window, qiparams, &cmd_params);
	
	// now sync to the parameters.
	assert(ARRAY_SIZE(parameter_names)==NUMBER_OF_PARAMETERS);
	update_parameters_display(rs_fric_window, 0, NUMBER_OF_PARAMETERS);
	
	// rs_fric_window->dc2_flag sets the one or two variables buttons...
	if(rs_fric_window->dc2_flag) rs_fric_window->second_set_flag= 1;
	set_initial_toggle_button_state_by_index(window, rs_fric_window->dc2_flag, stateVariableButtons);

	float lambda_flag = atof(rs_fric_window->parameter_strs[_parameter_lambda]);  /* returns 0 if no given lambda */
	int lambda_choice;
	if (lambda_flag <= 0) /* forward model */
		lambda_choice = 0;
	else                  /* inversion model */
		lambda_choice = 1;
	set_initial_toggle_button_state_by_index(window, lambda_choice, modelDirectionButtons);

	float wc_flag = atof(rs_fric_window->parameter_strs[11]);
	int wc_choice;
	if (wc_flag < 0) /* alternative weighting */
		wc_choice = 1;
	else             /* normal weighting */
		wc_choice = 0;
	set_initial_toggle_button_state_by_index(window, wc_choice, weightingButtons);
	set_initial_toggle_button_state_by_index(window, rs_fric_window->mvs_flag, velocityButtons);
	
	// FIXME: sync the buttons.

	// now we need to do the plotting if applicable
	
	/*printf("before other scanfs\n");*/
	int temp_int1 = atoi(rs_fric_window->parameter_strs[_parameter_first_row]);	/*first row*/
	int temp_int2 = atoi(rs_fric_window->parameter_strs[_parameter_last_row]);	/*last row*/
	int temp_int3 = atoi(rs_fric_window->parameter_strs[_parameter_disp_col]);	/*col*/
	/*make sure rows & cols are valid*/
	/*printf("after other scanfs, ps1 string is %s, temp_ints are: %d %d %d\n",parameter_strs[1],temp_int1, temp_int2, temp_int3);*/
	if( (cmd_params.token_count > 1) && (check_row(&temp_int1,&temp_int2,temp_int3) == 0) )
	{
		char plotcmd[200];
		
		/* plot the original data */
		strcpy(plotcmd, "plotauto");
		strcat(plotcmd, " "); 
		strcat(plotcmd, rs_fric_window->parameter_strs[_parameter_disp_col]);
		strcat(plotcmd, " "); 
		strcat(plotcmd, rs_fric_window->parameter_strs[_parameter_mu_col]);
		strcat(plotcmd, " "); 
		strcat(plotcmd, rs_fric_window->parameter_strs[_parameter_first_row]);
		strcat(plotcmd, " "); 
		strcat(plotcmd, rs_fric_window->parameter_strs[_parameter_last_row]);
		command_handler(plotcmd);

		if(rs_fric_window->mvs_flag == 1)			/*don't run a model, wait for mvs input*/
		{
			strcpy(qiparams, "qi_mvs.");
			left_footer(rs_fric_window, "MVS input, enter: n_vsteps v1 v2 ... vsrow1 vsrow2 ...");
			sprintf(msg, "MVS input needed. Enter: n_vsteps v1 v2 ... vsrow1 vsrow2 ... and then hit \"do_qi\" button\n");
			print_msg(msg);
		}
		else					 /*rebuild qi command, substitute "qi" for qi_tool and run*/
		{
			strcpy(qiparams,"qi.");	
			strcat(qiparams, cmd_params.qi_cmd_str);
			command_handler(qiparams);	/*execute qi*/

			strcpy(plotcmd, "plotover ");		/*make sure cols have been updated*/
			strcpy(rs_fric_window->parameter_strs[_parameter_disp_col], get_parameter_from_display(rs_fric_window, _parameter_disp_col));
			strcat(plotcmd, rs_fric_window->parameter_strs[_parameter_disp_col]);
			strcat(plotcmd, " ");
			strcpy(rs_fric_window->parameter_strs[_parameter_mu_fit_col], get_parameter_from_display(rs_fric_window, _parameter_mu_fit_col));
			strcat(plotcmd, rs_fric_window->parameter_strs[_parameter_mu_fit_col]);
			command_handler(plotcmd);
		}
	}
}

/* ----------------- signals */

/* -------------- StateVariables */
void on_btn_TwoStateVariables_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	if(gtk_toggle_button_get_active(togglebutton))
	{
		GtkWindow *window= parent_gtk_window(GTK_WIDGET(togglebutton));
		struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);

		fprintf(stderr, "Two State Variables Set!\n");
	
		// set the flag. (FIXME)
		fric_window->dc2_flag= 1;
		fric_window->second_set_flag= 1;

		// clear the parameters.
		strcpy(fric_window->parameter_strs[_parameter_b2], "");
		strcpy(fric_window->parameter_strs[_parameter_dc2], "");
		update_parameters_display(fric_window, _parameter_b2, _parameter_dc2);
	
		// set the footer.
		left_footer(fric_window, "Enter values for b2 and dc2");
	
		// show the two other panels.
		change_widget_visibility(window, twoVariableComponentNames, ARRAY_SIZE(twoVariableComponentNames), TRUE);
	}
/*
if ((value == 1)&&(second_set_flag == 0)) { 
xv_set(b2, XV_SHOW, TRUE, PANEL_VALUE, "", NULL);
xv_set(dc2, XV_SHOW, TRUE, PANEL_VALUE, "", NULL);
strcpy(parameter_strs[19], "");
strcpy(parameter_strs[20], "");
second_set_flag = 1;
dc2_flag = 1;
left_footer("Enter values for b2 and dc2.");
*/
}

void on_btn_OneStateVariable_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	if(gtk_toggle_button_get_active(togglebutton))
	{
		GtkWindow *window= parent_gtk_window(GTK_WIDGET(togglebutton));
		struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);

		fprintf(stderr, "One State Variables Set!\n");

		// clear the flag. (FIXME)
		fric_window->dc2_flag= 0;
		fric_window->second_set_flag= 0;

		// clear the parameters.
		strcpy(fric_window->parameter_strs[_parameter_b2], "1");
		strcpy(fric_window->parameter_strs[_parameter_dc2], "-1");
		update_parameters_display(fric_window, _parameter_b2, _parameter_dc2);

		// set the footer.
		left_footer(fric_window, "dc2 < 0");

		// hide the two other panels.
		change_widget_visibility(window, twoVariableComponentNames, ARRAY_SIZE(twoVariableComponentNames), FALSE);
	}
/*
if ((value == 0)&&(second_set_flag == 1)) { 
  xv_set(b2, XV_SHOW, FALSE, PANEL_VALUE, "1", NULL);
  xv_set(dc2, XV_SHOW, FALSE, PANEL_VALUE, "-1", NULL);
  strcpy(parameter_strs[19], "1");
  strcpy(parameter_strs[20], "-1");
  second_set_flag = 0;
  dc2_flag = 0;
  left_footer("dc2 < 0"); 
}
*/
}


gboolean on_btn_StateVariables_button_release_event(
	GtkWidget *widget, 
	GdkEventButton *event, 
	gpointer user_data)
{
	toggle_buttons(widget, stateVariableButtons);
	return TRUE;
}

/* --------- Model Direction */
void on_btn_ModelForward_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	if(gtk_toggle_button_get_active(togglebutton))
	{
		GtkWindow *window= parent_gtk_window(GTK_WIDGET(togglebutton));
		struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);

		fprintf(stderr, "Forward set!\n");

		float temp = -1*fabs(atof(fric_window->parameter_strs[_parameter_lambda]));	/*doing forward modeling, change lambda to negative value*/
		if(temp == 0) temp = -0.1;

		// this should be a function, as we use this for different things (including updating the text on focus change)
		sprintf(fric_window->parameter_strs[_parameter_lambda], "%g", temp);
		update_parameters_display(fric_window, _parameter_lambda, _parameter_lambda);
		left_footer(fric_window, "lambda < 0, Forward Modeling");
	}
}

void on_btn_ModelInversion_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	if(gtk_toggle_button_get_active(togglebutton))
	{
		GtkWindow *window= parent_gtk_window(GTK_WIDGET(togglebutton));
		struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);

		fprintf(stderr, "Inverse set!\n");
		float temp = fabs(atof(fric_window->parameter_strs[_parameter_lambda]));	/*doing inversion, change lambda to positive value*/
		if(temp == 0) temp = 0.1;

		// this should be a function, as we use this for different things (including updating the text on focus change)
		sprintf(fric_window->parameter_strs[_parameter_lambda], "%g", temp);
		update_parameters_display(fric_window, _parameter_lambda, _parameter_lambda);
		left_footer(fric_window, "lambda > 0, Inversion");
	}
}

gboolean on_btn_Model_button_release_event(
	GtkWidget *widget, 
	GdkEventButton *event, 
	gpointer user_data)
{
	toggle_buttons(widget, modelDirectionButtons);
	return TRUE;
}

/* -------------- Weighting */
void on_btn_WeightingNormal_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	if(gtk_toggle_button_get_active(togglebutton))
	{
		GtkWindow *window= parent_gtk_window(GTK_WIDGET(togglebutton));
		struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);

		fprintf(stderr, "Normal Weighting set!\n");

		// show the two other panels.
		change_widget_visibility(window, alternativeWeightingComponentNames, ARRAY_SIZE(alternativeWeightingComponentNames), FALSE);

		// setup the parameters
		strcpy(fric_window->parameter_strs[_parameter_wc],"0.1");
		update_parameters_display(fric_window, _parameter_wc, _parameter_wc);
		fric_window->alt_wt_flag= 0;
		left_footer(fric_window, "wc > 0");
	}
}

void on_btn_WeightingAlternative_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	if(gtk_toggle_button_get_active(togglebutton))
	{
		GtkWindow *window= parent_gtk_window(GTK_WIDGET(togglebutton));
		struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);

		fprintf(stderr, "Alternative Weighting set!\n");

		// show the two other panels.
		change_widget_visibility(window, alternativeWeightingComponentNames, ARRAY_SIZE(alternativeWeightingComponentNames), TRUE);

		strcpy(fric_window->parameter_strs[_parameter_wc], "-0.1");
		update_parameters_display(fric_window, _parameter_wc, _parameter_wc);
	    fric_window->alt_wt_flag = 1; 
	    left_footer(fric_window, "wc < 0. Enter a row number.");
	}
}

gboolean on_btn_Weighting_button_release_event(
	GtkWidget *widget, 
	GdkEventButton *event, 
	gpointer user_data)
{
	toggle_buttons(widget, weightingButtons);
	return TRUE;
}

/* -------------- Weighting */
void on_btn_VelocitySingle_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	if(gtk_toggle_button_get_active(togglebutton))
	{
		GtkWindow *window= parent_gtk_window(GTK_WIDGET(togglebutton));
		struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);

		fprintf(stderr, "Single Velocity set!\n");

		// hide the two other panels.
		change_widget_visibility(window, multipleVelocityComponentNames, ARRAY_SIZE(multipleVelocityComponentNames), FALSE);
		fric_window->mvs_flag = 0;
		left_footer(fric_window, "Single velocity step.");
	}
}

void on_btn_VelocityMultiple_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	if(gtk_toggle_button_get_active(togglebutton))
	{
		GtkWindow *window= parent_gtk_window(GTK_WIDGET(togglebutton));
		struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);

		fprintf(stderr, "Multiple Velocity set!\n");

		// show the two other panels.
		change_widget_visibility(window, multipleVelocityComponentNames, ARRAY_SIZE(multipleVelocityComponentNames), TRUE);
		fric_window->mvs_flag = 1;
		left_footer(fric_window, "Enter: n_vsteps v1 v2 ... vsrow1 vsrow2 ...");
	}
}

gboolean on_btn_Velocity_button_release_event(
	GtkWidget *widget, 
	GdkEventButton *event, 
	gpointer user_data)
{
	toggle_buttons(widget, velocityButtons);
	return TRUE;
}

/* -------------- focus */
gboolean on_focus_out_event(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
	GtkWindow *window= parent_gtk_window(widget);
	struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);
	int ii;
	int parameter_index= NONE;
	
//	fprintf(stderr, "Focus out of %s\n", gtk_widget_get_name(widget));
	
	for(ii= 0; ii<ARRAY_SIZE(parameter_names); ii++)
	{
		if(strcmp(parameter_names[ii], gtk_widget_get_name(widget))==0)
		{
			parameter_index= ii;
			break;
		}
	}
	
	if(parameter_index != NONE)
	{
		strcpy(fric_window->parameter_strs[parameter_index], gtk_entry_get_text(GTK_ENTRY(widget)));
		if(strlen(fric_window->parameter_strs[parameter_index])==0)
		{
			error_message(fric_window, parameter_index);
		} else {
		    left_footer(fric_window, "");
		}
	}
	
	// handle the "special ones" here.
	switch(parameter_index)
	{
		case _parameter_lambda:
			if (atof(fric_window->parameter_strs[parameter_index]) <= 0.) {
				set_initial_toggle_button_state_by_index(window, 0, modelDirectionButtons);
			}
			else {
				set_initial_toggle_button_state_by_index(window, 1, modelDirectionButtons);
			}
			break;
			
		case _parameter_dc2:
			if (atof(fric_window->parameter_strs[_parameter_dc2]) <= 0.) {
			    fric_window->second_set_flag = 0;
				set_initial_toggle_button_state_by_index(window, 0, stateVariableButtons);
			}
			else {
				set_initial_toggle_button_state_by_index(window, 1, stateVariableButtons);
			}
			break;
			
		case _parameter_mvs_params:
			if(strlen(fric_window->parameter_strs[_parameter_mvs_params]) < 3)
			{
			    left_footer(fric_window, "Missing mvs parameters, at least three are needed");
			}
			break;
	}
	
	return FALSE;
}

/* -------------------------------- run QI */

/* for Do QI buttion
   concatenates the parameters into a qi command line and sends it
   to the command_handler */

void on_btn_PerformQI_clicked(
	GtkButton *button,
	gpointer user_data)
{
	GtkWindow *window= parent_gtk_window(GTK_WIDGET(button));
	struct rs_fric_window *fric_window= rs_fric_window_from_gtk_window(window);
	char concat_params[256];
	canvasinfo *can_info;		/*use to check active plot*/

	can_info = wininfo.canvases[ui_globals.active_window]; /*set info*/ // FIXME!

	/**********Check that things are set properly, use rows and disp col as simple test, 
			make sure that at least this is set with reasonable values  ****************/
	/*changed 29/9/97*/
	int temp_int1 = atoi(fric_window->parameter_strs[_parameter_first_row]);	/*first row*/
	int temp_int2 = atoi(fric_window->parameter_strs[_parameter_last_row]);	/*last row*/
	int temp_int3 = atoi(fric_window->parameter_strs[_parameter_disp_col]);	/*col*/

	if(check_row(&temp_int1,&temp_int2,temp_int3) != 0)
	{
		left_footer(fric_window, "Parameters have to be set before running");
		print_msg("R/S_fric_tool does not have parameters set properly\n");
		return;
	} 

	/*************ok, build command line to send qi**************/
	strcpy(concat_params, "qi");  
	if (fric_window->mvs_flag == 1) strcat(concat_params, "_mvs");

	if (strlen(get_parameter_from_display(fric_window, _parameter_law_options)) != 0) {
		strcat(concat_params, ".");
		strcat(concat_params, get_parameter_from_display(fric_window, _parameter_law_options)); 
	}

	strcat(concat_params, " ");
	int parameter_index;
	
	if(fric_window->dc2_flag==0)
	{
		// make sure the values are correct for non-dual. (this is probably redunadant)
		strcpy(fric_window->parameter_strs[_parameter_b2], "1");
		strcpy(fric_window->parameter_strs[_parameter_dc2], "-1");
		update_parameters_display(fric_window, _parameter_b2, _parameter_dc2);
	}
	
	// build the command line.
	for(parameter_index= _parameter_disp_col; parameter_index<=_parameter_dc2; parameter_index++)
	{
		const char *parameter_value= get_parameter_from_display(fric_window, parameter_index);
		if(strlen(parameter_value)==0)
		{
			error_message(fric_window, parameter_index);
			return;
		} else {
			strcat(concat_params, parameter_value); 
			strcat(concat_params, " ");
		}
	}

	if(fric_window->mvs_flag==1 && (strlen(get_parameter_from_display(fric_window, _parameter_mvs_params))<3)) /*update mvs params*/
	{
		left_footer(fric_window, "Missing qi_mvs input for multiple velocity steps.");
		return;
	}

	if(fric_window->alt_wt_flag==1 && (strlen(get_parameter_from_display(fric_window, _parameter_alt_wt))==0)) /*update mvs params*/
	{
		left_footer(fric_window, "Missing row number for alternative weighting.");
		return;
	}

//	fprintf(stderr, "\nconcat_params: %s", concat_params);
	record_command(concat_params);
	command_handler(concat_params);

	if (fric_window->mvs_flag == 1) { 
		char mvs_str[128];
		
		assert(strlen(get_parameter_from_display(fric_window, _parameter_mvs_params))<sizeof(mvs_str)-1);
		strcpy(mvs_str, get_parameter_from_display(fric_window, _parameter_mvs_params));

		record_command(mvs_str);

		command_handler(mvs_str);
	}
	
	if(fric_window->alt_wt_flag == 1) {
		char alt_str[128];
		
		assert(strlen(get_parameter_from_display(fric_window, _parameter_alt_wt))<sizeof(alt_str)-1);
		strcpy(alt_str, get_parameter_from_display(fric_window, _parameter_alt_wt));

		record_command(alt_str);

	    command_handler(alt_str);
	}

	char plotcmd[200];

	/*printf("in run proc: active_window = %d, active plot = %d\n",active_window, can_info->active_plot+1);*/
	if (ui_globals.active_window == -1 || can_info->active_plot == -1) {
		strcpy(plotcmd, "plotauto ");
		strcat(plotcmd, fric_window->parameter_strs[_parameter_disp_col]);
		strcat(plotcmd, " ");
		strcat(plotcmd, fric_window->parameter_strs[_parameter_mu_col]);
		strcat(plotcmd, " ");
		strcat(plotcmd, fric_window->parameter_strs[_parameter_first_row]);
		strcat(plotcmd, " ");
		strcat(plotcmd, fric_window->parameter_strs[_parameter_last_row]);
		command_handler(plotcmd);
	}

	strcpy(plotcmd, "plotover ");
	strcpy(fric_window->parameter_strs[_parameter_disp_col], get_parameter_from_display(fric_window, _parameter_disp_col));
	strcat(plotcmd, fric_window->parameter_strs[_parameter_disp_col]);
	strcat(plotcmd, " ");
	strcpy(fric_window->parameter_strs[_parameter_mu_fit_col], get_parameter_from_display(fric_window, _parameter_mu_fit_col));
	strcat(plotcmd, fric_window->parameter_strs[_parameter_mu_fit_col]);

	command_handler(plotcmd);
}

/* ------------ close the window */
void on_btn_CloseWindow_clicked(
	GtkButton *button,
	gpointer user_data)
{
	fprintf(stderr, "Close Window\n");
	gtk_widget_destroy(GTK_WIDGET(parent_gtk_window(button)));
}


void on_rsfricWindow_destroy(
	GtkObject *object,
	gpointer   user_data)
{
	// clear out our global variable so we do the right thng next time (recreate me)
	ui_globals.rs_window= NULL;
}
/* ---------------------------------------------- internal code */
static void left_footer(struct rs_fric_window *rs_fric_window, const char *msg)
{
	GtkLabel *label= GTK_LABEL(lookup_widget_by_name(GTK_WIDGET(rs_fric_window->window), "lbl_LeftFooterRS"));
	assert(label);
	gtk_label_set_text(label, msg);
}

static void parse_command_line(struct rs_fric_window *rs_fric_window, char *command_line, struct command_line_parsing_data *data)
{
	char *ptr, qi_head[12];
	char *t_string= "\t ,"; /*make init call to read past first tok-options*/ 
	char *qi_tool_mvs= "qi_tool_mvs";
	int ii;
	
	// cleara
	memset(data, 0, sizeof(struct command_line_parsing_data));
	
	// clear out the parameters..
	for(ii= 0; ii<NUMBER_OF_PARAMETERS; ii++)
	{
		rs_fric_window->parameter_strs[ii][0]= '\0'; // clear
	}
	
	nocom(command_line); /* removes commas and substitute with spaces  */

	/********** do command line parsing ********/
	rs_fric_window->mvs_flag = 0;
	/*printf("before string stuff\n");*/
	if(  (ptr = strchr(command_line, '.')) != NULL)	/* allow for absence of options or calls to create_qi_canvas() without a command line */
	{
		/*points to dot, increment one char beyond*/
		++ptr;
		
		/*save command line section after dot*/
		strcpy(data->qi_cmd_str, ptr);
		
		/*save command line section preceeding dot*/
		sscanf(command_line, "%s", qi_head); // buffer overrun issue here FIXME
		
		/*get options from tok following dot*/
		sscanf(ptr, "%s", rs_fric_window->parameter_strs[0]);
	}

	/*this is used below as a flag for plotting*/
	data->token_count = 0;
	if( (strtok(command_line, t_string)) != NULL)
	{
		/* printf("after string stuff, before tok_cnt, qiparams string is %s\n",qiparams); */
		for(data->token_count=1; data->token_count<NUMBER_OF_PARAMETERS; data->token_count++)
		{
			ptr = strtok(NULL, t_string);
			if(ptr == NULL)
			{
				break;
			}
			else
			{
				sscanf(ptr, "%s", rs_fric_window->parameter_strs[data->token_count]);
			}
		}

		/* printf("after tok cnt ,tok_cnt %d=\n",tok_cnt); */
		if (strncmp(qi_head, qi_tool_mvs, strlen(qi_tool_mvs)) == 0 ) 
		{
			rs_fric_window->mvs_flag = 1;
		}
	}
	
	if (atof(rs_fric_window->parameter_strs[_parameter_dc2]) < 0 )
	{
		/* one state variable model */	
		rs_fric_window->dc2_flag= 0;
	} else {
		rs_fric_window->dc2_flag= 1;
	}
	
	return;
}

static void set_initial_toggle_button_state_by_index(GtkWindow* window, int index, const char *names[])
{
	int ii;
	// we do this twice, so that we KNOW the toggled callback is called...
	for(ii= 0; ii<2; ii++)
	{
		GtkToggleButton *button= GTK_TOGGLE_BUTTON(lookup_widget_by_name(GTK_WIDGET(window), names[ii]));
		gtk_toggle_button_set_active(button, FALSE);
		if(ii==index)
		{
			gtk_toggle_button_set_active(button, TRUE);
		}
	}
}

static void toggle_buttons(GtkWidget *widget, const char *names[])
{
	const char *name= gtk_widget_get_name(GTK_WIDGET(widget));
	int index_hit= NONE;
	int ii;

	// get the name of this one, and untoggle the other
	for(ii= 0; ii<2; ii++)
	{
		if(strcmp(names[ii], name)==0)
		{
			index_hit= ii;
			break;
		}
	}
	
//	fprintf(stderr, "OnButtonReleaseEvent: %s (%d)\n", name, index_hit);
	
	// return false, everything happens as normal.  Return true, however, and nothing else happens unless we make it happen.
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
		// if we're active, we don't do anything (don't change, yadda yadda yadda.)
	} else {
		// we togle, and untoggle our friend.
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);

		if(index_hit != NONE)
		{
			int other_index= !index_hit;

			GtkToggleButton *other_button= GTK_TOGGLE_BUTTON(lookup_widget_by_name(GTK_WIDGET(parent_gtk_window(widget)), names[other_index]));
			gtk_toggle_button_set_active(other_button, FALSE);
		}
	}

	return;
}

static struct rs_fric_window *rs_fric_window_from_gtk_window(GtkWindow *window)
{
	struct rs_fric_window *test= first_window;
	
	while(test != NULL && test->window != window)
	{
		test= test->next;
	}
	
	return test;
}

static const gchar *get_parameter_from_display(struct rs_fric_window *rs_fric_window, int index)
{
	assert(rs_fric_window);
	assert(index>=0 && index<ARRAY_SIZE(parameter_names));

	GtkEntry *entry= GTK_ENTRY(lookup_widget_by_name(GTK_WIDGET(rs_fric_window->window), parameter_names[index]));
	assert(entry);
	
	return gtk_entry_get_text(entry);
}

static void update_parameters_display(struct rs_fric_window *rs_fric_window, int start, int last)
{
	GtkWindow *window= rs_fric_window->window;
	int ii;

	assert(start>=0 && start<ARRAY_SIZE(parameter_names));
	assert(last>=0 && last<=ARRAY_SIZE(parameter_names));
	assert(last>=start);

	if(last==NUMBER_OF_PARAMETERS) last -= 1; // allow passing in NUMBER_OF_PARAMETERS
	for(ii= start; ii<=last; ii++)
	{
		GtkEntry *entry= GTK_ENTRY(lookup_widget_by_name(GTK_WIDGET(window), parameter_names[ii]));
		assert(entry);
		gtk_entry_set_text(entry, rs_fric_window->parameter_strs[ii]);
	}	
}

static void change_widget_visibility(GtkWindow *window, const char *names[], int length, gboolean show)
{
	int ii;
	for(ii= 0; ii<length; ii++)
	{
		GtkWidget *w= lookup_widget_by_name(GTK_WIDGET(window), names[ii]);
		if(show)
		{
			gtk_widget_show(w);
		} else {
			gtk_widget_hide(w);
		}
	}
}

static void error_message(struct rs_fric_window *rs_fric_window, int index)
{
	char temporary[512];

	assert(rs_fric_window);
	assert(index>=0 && index<ARRAY_SIZE(parameter_error_messages));
	
	left_footer(rs_fric_window, parameter_error_messages[index]);
	
  	sprintf(temporary, "Input error.  R/S_fric_tool does not have parameters set properly.\nSee Left Footer of R/S Fric Tool window\n\n");
 	print_msg(temporary);
}
