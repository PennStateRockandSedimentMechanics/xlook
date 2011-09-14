/* last update 29/9/97 cjm: fix minor bug with the way error checking
was done before sending string to command handler. I had been relying
on the text in "parameter_str[]" for the check on row/cols, but these
were not being set for the case where the windows was called up clean
and parameters were set by hand (but without tabs or returns after each
entry).  I changed so that an "xv_get" is issued for these parameters
before the check. Now, as long as these few rows and cols are entered
it will try to construct a command line for the handler. see lines 
around 850
*/
/*
   The source code for R/S FRIC TOOL window.
*/

#include <math.h>
#include <string.h>
#include <sys/file.h>

#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/xv_xrect.h>
#include <xview/font.h>
#include <xview/cursor.h>

#include <config.h>
#include <global.h>
#include <messages.h>
#include <rs_fric_tool.h>
#include <strcmd.h>

extern int check_row();
extern char msg[MSG_LENGTH];
extern Frame main_frame; /* main window frame */
extern int qi_flag; /* flag indicating the tool is launched or not */
extern int total_windows; /* number of plot windows */
extern int active_window;
extern char qiparams[1024]; /* string of input parameters */ 
extern void command_handler(); /* command handling procedure event.c */
extern Panel_item cmd_hist_panel_list; /* panel storing the command history */
extern int cmd_num; /* index used in cmd_hist_panel_list */

typedef char parameter_str[20]; /* parameter string */
parameter_str parameter_strs[21]; /* array of parameter strings */
char alt_wt_str[9];

char mvs_params[60]; /* addtional input string when mvs option is selected */
char plotcmd[60];
Panel qi_parameter_panel, button_panel; /* for the parameters input */

Panel_text_item pt_lambda, b2, dc2;
Panel_text_item alt_wt, mvs_par;
Panel_text_item wc_panel;
Panel_text_item options, disp_col, mu_col, mu_fit_col, first_row;
Panel_text_item vs_row, last_row, weight_row, lin_term, c_tol;
Panel_text_item stiff, v_initial, v_final, mu_initial, a, b1, dc1, b2, dc2;


Panel_choice_item lambda_toggle, dc2_toggle, wc_toggle;


/* to manage dynamically created text inputs */
int alt_wt_flag, mvs_flag, second_set_flag;

int tok_cnt; /* indexing integer , also used as a flag */ /*don't use "x" as int*/
int temp_int1, temp_int2, temp_int3;		/*for use in check_row, and general temp use*/ 

Frame qi_frame; /* main frame of R/S FRIC TOOL */

double lambda_flag, wc_flag; /* converted from inputs, define global for use in run_proc */
int dc2_flag;		/*define global, for use in create() and run()*/

void create_qi_canvas()
{
  int lambda_choice, wc_choice; /* to set the toggle buttons */

  /* to parse the input */
  char *ptr, qi_head[12];
  char qi_cmd_str[160], t_string[160];

  /* notifying procedures */
  void qp_panel_proc();
  void run_proc(), kill_qi_proc();
  void lambda_proc(), lambda_value_proc(), dc2_proc();
  void wc_proc(), mvs_proc();
  void get_options(), get_alt_wt(), get_b2_dc2(), get_mvs_par(), get_wc_proc();
  
  canvasinfo *can_info;		/*use to check active plot*/

  can_info = wininfo.canvases[active_window]; /*set info*/
  
  /*************    PANEL SETUP ***********************************/

  nocom(qiparams); /* removes commas and substitute with spaces  */
  
/********** do command line parsing ********/

  mvs_flag = 0;
/*printf("before string stuff\n");*/
  if(  (ptr = strchr(qiparams, '.')) != NULL)	/*allow for absence of options or calls to */
  { 						/*   create_qi_canvas() without a command line*/
	++ptr;					/*points to dot, increment one char beyond*/
  	strcpy(qi_cmd_str,ptr);			/*save command line section after dot*/
  	sscanf(qiparams,"%s",qi_head); 		/*save command line section preceeding dot*/
	sscanf(ptr, "%s", parameter_strs[0]);	/*get options from tok following dot*/
  }

  strcpy(t_string,"\t ,");	/*make init call to read past first tok-options*/

  tok_cnt = 0;		/*this is used below as a flag for plotting*/
  if( (strtok(qiparams,t_string)) != NULL)
  {
/*printf("after string stuff, before tok_cnt, qiparams string is %s\n",qiparams);*/
  	for(tok_cnt=1; tok_cnt<21; tok_cnt++)
	{
		ptr = strtok(NULL,t_string);
		if(ptr == NULL)
			break;
		else
			sscanf(ptr, "%s", parameter_strs[tok_cnt]);
 	}
/*printf("after tok cnt ,tok_cnt %d=\n",tok_cnt);*/
  	if (strncmp(qi_head, "qi_tool_mvs", 11) == 0 ) 
    		mvs_flag = 1;
  }
    
  
  qi_frame = (Frame)xv_create(main_frame, FRAME,
			      FRAME_LABEL, "R/S FRIC TOOL",
			      XV_HEIGHT, 355, 
			      XV_WIDTH, 600,
			      FRAME_SHOW_FOOTER, TRUE,  
			      FRAME_LEFT_FOOTER, "Type parameters.",
			      NULL);
  

  /* text panels for each parameter of qi command */

  qi_parameter_panel =  (Panel)xv_create(qi_frame, PANEL, 
					 XV_HEIGHT, 320, NULL);

  options = (Panel_text_item) xv_create(qi_parameter_panel, PANEL_TEXT,
		   XV_X, 20,
		   XV_Y, 15,
		   PANEL_LABEL_STRING, "law/options",
		   PANEL_VALUE_DISPLAY_LENGTH, 8,
		   PANEL_VALUE, parameter_strs[0],
		   PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		   PANEL_NOTIFY_PROC, get_options,
		   NULL);
  
  disp_col = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 220,
		    XV_Y, 15, 
		    PANEL_LABEL_STRING, "disp_col",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[1],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 1,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  mu_col = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 420,
		    XV_Y, 15,
		    PANEL_LABEL_STRING, "mu_col",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[2],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 2,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  mu_fit_col = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 20,
		    XV_Y, 40,
		    PANEL_LABEL_STRING, "mu_fit_col",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[3],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 3,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  first_row = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 220,
		    XV_Y, 40,
		    PANEL_LABEL_STRING, "first_row",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[4],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 4,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  vs_row = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 420,
		    XV_Y, 40,
		    PANEL_LABEL_STRING, "vs_row",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[5],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 5,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  last_row = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 20,
		    XV_Y, 65,
		    PANEL_LABEL_STRING, "last_row",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[6],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 6,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  weight_row = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 220,
		    XV_Y, 65,
		    PANEL_LABEL_STRING, "weight_row",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[7],
		    PANEL_NOTIFY_LEVEL,
		    PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 7,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  lin_term = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 420,
		    XV_Y, 65,
		    PANEL_LABEL_STRING, "lin_term",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[8],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 8,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  c_tol = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 20,
		    XV_Y, 90,
		    PANEL_LABEL_STRING, "c_tol",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[9],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 9,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);  
  
  pt_lambda = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
					XV_X, 220,
					XV_Y, 90,
					PANEL_LABEL_STRING, "lambda",
					PANEL_VALUE_DISPLAY_LENGTH, 8,
					PANEL_VALUE, parameter_strs[10],
					PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
					PANEL_NOTIFY_PROC, lambda_value_proc,
					NULL);
  
  
  wc_panel = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
				    XV_X, 420,
				    XV_Y, 90,
				    PANEL_LABEL_STRING, "wc",
				    PANEL_VALUE_DISPLAY_LENGTH, 8,
				    PANEL_VALUE, parameter_strs[11],
				    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
				    PANEL_NOTIFY_PROC, get_wc_proc,
				    NULL);
  
  stiff = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 20,
		    XV_Y, 115,
		    PANEL_LABEL_STRING, "stiff",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[12],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 12,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  v_initial = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 220,
		    XV_Y, 115,
		    PANEL_LABEL_STRING, "v_initial",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[13],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 13,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  v_final = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 420,
		    XV_Y, 115,
		    PANEL_LABEL_STRING, "v_final",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[14],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 14,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  mu_initial = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 20,
		    XV_Y, 140,
		    PANEL_LABEL_STRING, "mu_initial",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[15],
		    PANEL_NOTIFY_LEVEL, 
		    PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 15,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  a = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 220,
		    XV_Y, 140,
		    PANEL_LABEL_STRING, "a",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[16],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 16,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  b1 = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 420,
		    XV_Y, 140,
		    PANEL_LABEL_STRING, "b1",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[17],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 17,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  dc1 = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
		    XV_X, 20,
		    XV_Y, 165,
		    PANEL_LABEL_STRING, "dc1",
		    PANEL_VALUE_DISPLAY_LENGTH, 8,
		    PANEL_VALUE, parameter_strs[18],
		    PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		    PANEL_CLIENT_DATA, 18,
		    PANEL_NOTIFY_PROC, qp_panel_proc,
		    NULL);
  
  b2 = (Panel_text_item) xv_create(qi_parameter_panel, PANEL_TEXT,
				   XV_X, 220,
				   XV_Y, 165,
				   XV_SHOW, FALSE, /* default */
				   PANEL_LABEL_STRING, "b2",
				   PANEL_VALUE_DISPLAY_LENGTH, 8,
				   PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
				   PANEL_CLIENT_DATA, 1,
				   PANEL_NOTIFY_PROC, get_b2_dc2,
				   NULL);
  
  dc2 = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
				     XV_X, 420,
				     XV_Y, 165,
				     XV_SHOW, FALSE, /* default */
				     PANEL_LABEL_STRING, "dc2",
				     PANEL_VALUE_DISPLAY_LENGTH, 8,
				     PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED, 
				     PANEL_CLIENT_DATA, 2,
				     PANEL_NOTIFY_PROC, get_b2_dc2,
				     NULL);
  


  /****************** lambda toggle swtich ******************************/  
  
  lambda_flag = atof(parameter_strs[10]);  /* returns 0 if no given lambda */
  
  if (lambda_flag <= 0) /* forward model */
    lambda_choice = 0;
  else                  /* inversion model */
    lambda_choice = 1;
  
  lambda_toggle = (Panel_choice_item)  xv_create(qi_parameter_panel, 
						 PANEL_CHOICE,
						 XV_X, 20,
						 XV_Y, 190,
						 PANEL_LABEL_STRING, "Model",
						 PANEL_CHOICE_STRINGS, 
						 "Forward","Inversion", NULL,
						 PANEL_NOTIFY_PROC, 
						 lambda_proc,
						 PANEL_VALUE, lambda_choice,
						 NULL);
  
   

  /******************* dc2 toggle switch *******************************/


  if (atof(parameter_strs[20]) < 0 ) /* one state variable model */
    dc2_flag = 0; 
  else               /* two state variable model */
    dc2_flag = 1;
  
    
  dc2_toggle = (Panel_choice_item)  xv_create(qi_parameter_panel, PANEL_CHOICE,
					      XV_X, 260,
					      XV_Y, 190,
					      PANEL_CHOICE_STRINGS, 
					      "1 State Variable", 
					      "2 State Variables", NULL,
					      PANEL_NOTIFY_PROC, dc2_proc,
					      PANEL_VALUE, dc2_flag,
					      NULL);
  
  second_set_flag = 0;
  
  if (dc2_flag == 1)
    { xv_set(b2, XV_SHOW, TRUE, PANEL_VALUE, parameter_strs[19], NULL);
      xv_set(dc2, XV_SHOW, TRUE, PANEL_VALUE, parameter_strs[20], NULL);
      second_set_flag = 1;}


  /********************* wc toggle switch *******************************/
  
  wc_flag = atof(parameter_strs[11]);
  
  alt_wt_flag = 0;
  
  if (wc_flag < 0) /* alternative weighting */
    wc_choice = 1;
  else             /* normal weighting */
    wc_choice = 0;
  
  
  wc_toggle = (Panel_choice_item)  xv_create(qi_parameter_panel, PANEL_CHOICE,
					     XV_X, 20,
					     XV_Y, 225,
					     PANEL_CHOICE_STRINGS, 
					     "Normal Weighting",
					     "Alternative Weighting", NULL,
					     PANEL_NOTIFY_PROC, wc_proc,
					     PANEL_VALUE, 0,
					     NULL);

  alt_wt = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
					XV_X, 300,
					XV_Y, 225,
					XV_SHOW, FALSE, /* default */
					PANEL_LABEL_STRING, "row number",
					PANEL_VALUE_DISPLAY_LENGTH, 8,
					PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED, 
					PANEL_NOTIFY_PROC, get_alt_wt,
					NULL);
  
  if (wc_choice == 1) { 
    xv_set(alt_wt, XV_SHOW, TRUE, NULL);
    alt_wt_flag = 1;
  } 
  
/**********************  mvs toggle swtich ****************************/

  
  (void) xv_create(qi_parameter_panel, PANEL_CHOICE,
		   XV_X, 20,
		   XV_Y, 260,
		   PANEL_CHOICE_STRINGS, "Single Velocity Step", 
		   "Multiple Velocity Steps", NULL,
		   PANEL_NOTIFY_PROC, mvs_proc,
		   PANEL_VALUE, mvs_flag,
		   NULL);

  mvs_par = (Panel_text_item)  xv_create(qi_parameter_panel, PANEL_TEXT,
					      XV_X, 20,
					      XV_Y, 295,
					      XV_SHOW, mvs_flag, /* default */
					      PANEL_LABEL_STRING, 
					      "qi_mvs input",
					      PANEL_VALUE_DISPLAY_LENGTH, 60,
					      PANEL_NOTIFY_LEVEL, 
					      PANEL_SPECIFIED, 
					      PANEL_NOTIFY_PROC, get_mvs_par,
					      NULL);
  


/******************** For Buttons ***********************************/
  
  button_panel =  (Panel)xv_create(qi_frame, PANEL, XV_HEIGHT, 35, NULL);
  
  /* run button */
  (void) xv_create(button_panel, PANEL_BUTTON,
		   XV_X, 300,
		   PANEL_LABEL_STRING, "Do QI",
		   PANEL_NOTIFY_PROC, run_proc, 
		   NULL);
  
  /* kill window button */
  (void) xv_create(button_panel, PANEL_BUTTON,
		   XV_X, 360,
		   PANEL_LABEL_STRING, "Kill Window",
		   PANEL_NOTIFY_PROC, kill_qi_proc, 
		   PANEL_CLIENT_DATA, qi_frame,
		   NULL);
  
  xv_set(qi_frame, XV_SHOW, TRUE, NULL);

/*printf("before other scanfs\n");*/
    temp_int1 = atoi(parameter_strs[4]);	/*first row*/
    temp_int2 = atoi(parameter_strs[6]);	/*last row*/
    temp_int3 = atoi(parameter_strs[1]);	/*col*/
						/*make sure rows & cols are valid*/
/*printf("after other scanfs, ps1 string is %s, temp_ints are: %d %d %d\n",parameter_strs[1],temp_int1, temp_int2, temp_int3);*/
    if( (tok_cnt > 1) && (check_row(&temp_int1,&temp_int2,temp_int3) == 0) )
    {
  						/* plot the original data */
    	strcpy(plotcmd, "plotauto");
    	strcat(plotcmd, " "); 
    	strcat(plotcmd, parameter_strs[1]);
    	strcat(plotcmd, " "); 
    	strcat(plotcmd, parameter_strs[2]);
    	strcat(plotcmd, " "); 
    	strcat(plotcmd, parameter_strs[4]);
    	strcat(plotcmd, " "); 
    	strcat(plotcmd, parameter_strs[6]);
  	command_handler(plotcmd);

  	if(mvs_flag == 1)			/*don't run a model, wait for mvs input*/
	{
    		strcpy(qiparams, "qi_mvs.");
    		left_footer("MVS input, enter: n_vsteps v1 v2 ... vsrow1 vsrow2 ...");
		sprintf(msg, "MVS input needed. Enter: n_vsteps v1 v2 ... vsrow1 vsrow2 ... and then hit \"do_qi\" button\n");
		print_msg(msg);
	}
	else					 /*rebuild qi command, substitute "qi" for qi_tool and run*/
	{
    		strcpy(qiparams,"qi.");	
    		strcat(qiparams, qi_cmd_str);
    		command_handler(qiparams);	/*execute qi*/

    		strcpy(plotcmd, "plotover ");		/*make sure cols have been updated*/
		strcpy(parameter_strs[1], (char *)xv_get(disp_col, PANEL_VALUE));
    		strcat(plotcmd, parameter_strs[1]);
    		strcat(plotcmd, " ");
		strcpy(parameter_strs[3], (char *)xv_get(mu_fit_col, PANEL_VALUE));
    		strcat(plotcmd, parameter_strs[3]);
    		command_handler(plotcmd);
	}
  }
}





/******************* Callback Functions ******************************/


void qp_panel_proc(item, event)
     Panel_item item;
     Event *event;
{
  /* panel_item callback function; 
     stores the string input for further process */
  int i;
  
  i = (int) xv_get(item, PANEL_CLIENT_DATA);
  strcpy(parameter_strs[i], (char *)xv_get(item, PANEL_VALUE));

  if (strlen(parameter_strs[i]) == 0) 
    error_msg(i);
  else {
    left_footer("");
    panel_advance_caret(qi_parameter_panel);
  }
}

void get_options(item, event)
     Panel_item item;
     Event *event;
{
  strcpy(parameter_strs[0], (char *)xv_get(item, PANEL_VALUE));
  if (strlen(parameter_strs[0]) == 0)
    left_footer("No options selected");
  panel_advance_caret(qi_parameter_panel);
}

/* for lambda toggle button
   shows/hides the input panel depending on the selection 
   changes panel value accordingly */

void lambda_proc(item, value, event)
     Panel_item item;
     int value;
     Event *event;
{
  double temp, atof(), fabs();

  if (value == 0) {
    temp = -1*fabs(atof(parameter_strs[10]));	/*doing forward modeling, change lambda to negative value*/
    if(temp == 0)
	temp = -0.1;
    sprintf(parameter_strs[10],"%g", temp);
    xv_set(pt_lambda, PANEL_VALUE, parameter_strs[10], NULL);
    left_footer("lambda < 0, Forward Modeling");
  }
  else { 
    temp = fabs(atof(parameter_strs[10]));	/*doing inversion, change lambda to positive value*/
    if(temp == 0)
	temp = 0.1;
    sprintf(parameter_strs[10],"%g", temp);
    xv_set(pt_lambda, PANEL_VALUE, parameter_strs[10], NULL);
    left_footer("lambda > 0, Inversion");
  }
}

void lambda_value_proc(item, event)
     Panel_item item;
     Event *event;
{ 
  int i;
  
  strcpy(parameter_strs[10], (char *)xv_get(item, PANEL_VALUE));
  
  if (strlen(parameter_strs[10]) == 0) {
    left_footer("Missing lambda");
    return;
  }
  
  if (atof(parameter_strs[10]) <= 0.) {
    i = 0;
    left_footer("lambda < 0, Forward Modeling"); 
  }
  else {
    i = 1;
    left_footer("lambda > 0, Inversion");
  }
  
  xv_set(lambda_toggle, PANEL_VALUE, i, NULL);
  panel_advance_caret(qi_parameter_panel);
}



/* for dc2 toggle button
   shows and hides the input panels depending on the selection
   on the toggle button */

void dc2_proc(item, value, event)
     Panel_item item;
     int value;
     Event *event;
{
  if ((value == 1)&&(second_set_flag == 0)) { 
    xv_set(b2, XV_SHOW, TRUE, PANEL_VALUE, "", NULL);
    xv_set(dc2, XV_SHOW, TRUE, PANEL_VALUE, "", NULL);
    strcpy(parameter_strs[19], "");
    strcpy(parameter_strs[20], "");
    second_set_flag = 1;
    dc2_flag = 1;
    left_footer("Enter values for b2 and dc2.");
  }
  
  if ((value == 0)&&(second_set_flag == 1)) { 
    xv_set(b2, XV_SHOW, FALSE, PANEL_VALUE, "1", NULL);
    xv_set(dc2, XV_SHOW, FALSE, PANEL_VALUE, "-1", NULL);
    strcpy(parameter_strs[19], "1");
    strcpy(parameter_strs[20], "-1");
    second_set_flag = 0;
    dc2_flag = 0;
    left_footer("dc2 < 0"); 
  }
}

void get_b2_dc2(item, event)
     Panel_item item;
     Event *event;
{ 

  int i, j;

  j = (int) xv_get(item, PANEL_CLIENT_DATA);
  
  if ( j==1 ) {
    strcpy(parameter_strs[19], (char *)xv_get(item, PANEL_VALUE));
    if (strlen(parameter_strs[19]) == 0) {
      left_footer("Missing b2");
      return;
    }
    left_footer("");      
    panel_advance_caret(qi_parameter_panel);
    return;
  }

  
  strcpy(parameter_strs[20], (char *)xv_get(item, PANEL_VALUE));
  if (strlen(parameter_strs[20]) == 0) {
    left_footer("Missing dc2");
    return;
  }

  if (atof(parameter_strs[20]) <= 0.) {
    i = 0;
    xv_set(b2, XV_SHOW, FALSE, NULL);
    xv_set(dc2, XV_SHOW, FALSE, NULL);
    second_set_flag = 0;
    left_footer("dc2 < 0"); 
  }
  else {
    i = 1;
    left_footer("Enter values for b2 and dc2");
  }
  
  panel_advance_caret(qi_parameter_panel);
  xv_set(dc2_toggle, PANEL_VALUE, i, NULL);
}


/* for wc toggle button
   dynamically creates and destroys the input panel for 
   the new row number for alternative weighting */

void wc_proc(item, value, event)
     Panel_item item;
     int value;
     Event *event;
{
  if ((value == 1)&&(alt_wt_flag == 0)) {
    xv_set(wc_panel, PANEL_VALUE, "-0.1", NULL);   
    xv_set(alt_wt, XV_SHOW, TRUE, NULL); 
    strcpy(parameter_strs[11],"-0.1");
    alt_wt_flag = 1; 
    left_footer("wc < 0. Enter a row number.");
  }
  
  if ((value == 0)&&(alt_wt_flag == 1)) {
    xv_set(wc_panel, PANEL_VALUE, "0.1", NULL);   
    xv_set(alt_wt, XV_SHOW, FALSE, NULL); 
    strcpy(parameter_strs[11],"0.1");
    alt_wt_flag = 0;
    left_footer("wc > 0");
  } 

}


void get_wc_proc(item, event)
     Panel_item item;
     Event *event;
{
  int i;
  
  strcpy(parameter_strs[11], (char *)xv_get(item, PANEL_VALUE));
  
  if (strlen(parameter_strs[11]) == 0) {
    left_footer("Missing wc");
    return;
  }
  
  left_footer("");

  if (atof(parameter_strs[11]) >= 0.) {
    i = 0;
    xv_set(alt_wt, XV_SHOW, FALSE, NULL);
    alt_wt_flag = 0;
    left_footer("wc > 0"); 
  }
  else {
    i = 1;
    xv_set(alt_wt, XV_SHOW, TRUE, NULL);
    alt_wt_flag = 1;
    left_footer("wc < 0. Enter a row number.");
  }

  panel_advance_caret(qi_parameter_panel);
  xv_set(wc_toggle, PANEL_VALUE, i, NULL);
}


void get_alt_wt(item, event)
     Panel_item item;
     Event *event;
{

  strcpy(alt_wt_str, (char *)xv_get(item, PANEL_VALUE));
  if(strlen(alt_wt_str) == 0) 
    left_footer("Missing row number");
  else {
    panel_advance_caret(qi_parameter_panel);
    left_footer("");
  }
}


/* for mvs toggle button
   dynamically creates and destroys the input panel for
   the mvs option */

void mvs_proc(item, value, event)
     Panel_item item;
     int value;
     Event *event;
{
  if ((value == 0)&&(mvs_flag == 1)) { 
    xv_set(mvs_par, XV_SHOW, FALSE, NULL);
    mvs_flag = 0;
    left_footer("Single velocity step.");
  }
  
  if ((value == 1)&&(mvs_flag == 0)) {
    xv_set(mvs_par, XV_SHOW, TRUE, NULL);
    mvs_flag = 1;
    left_footer("Enter: n_vsteps v1 v2 ... vsrow1 vsrow2 ...");
  }
}


void get_mvs_par(item, event)
     Panel_item item;
     Event *event;
{
  strcpy(mvs_params, (char *)xv_get(item, PANEL_VALUE));
  if (strlen(mvs_params) < 3)
    left_footer("Missing mvs parameters, at least three are needed");
  else {
    panel_advance_caret(qi_parameter_panel);
    left_footer("");
  }
}

/* for Do QI buttion
   concatenates the parameters into a qi command line and sends it
   to the command_handler */

void run_proc(item, event)
     Panel_item item;
     Event *event;
{
  char concat_params[160];
  char mvs_str[60], alt_str[9];
  canvasinfo *can_info;		/*use to check active plot*/

  can_info = wininfo.canvases[active_window]; /*set info*/

/**********Check that things are set properly, use rows and disp col as simple test, 
		make sure that at least this is set with reasonable values  ****************/
/*changed 29/9/97*/
  temp_int1 = atoi((char *)xv_get(first_row, PANEL_VALUE));	/*first row*/
  temp_int2 = atoi((char *)xv_get(last_row, PANEL_VALUE));	/*last row*/
  temp_int3 = atoi((char *)xv_get(disp_col, PANEL_VALUE));	/*col*/

  /*temp_int1 = atoi(parameter_strs[4]);
  temp_int2 = atoi(parameter_strs[6]);	
  temp_int3 = atoi(parameter_strs[1]);	*/

  if(check_row(&temp_int1,&temp_int2,temp_int3) != 0)
  {
	left_footer("Parameters have to be set before running");
	sprintf(msg, "R/S_fric_tool does not have parameters set properly\n");
	print_msg(msg);
	return;
  } 
/*************ok, build command line to send qi**************/
  strcpy(concat_params, "qi");
  
  if (mvs_flag == 1) 
    strcat(concat_params, "_mvs");

  if (strlen((char *)xv_get(options, PANEL_VALUE)) != 0) {
    strcat(concat_params, ".");
    strcat(concat_params, (char *)xv_get(options, PANEL_VALUE)); 
  }

  strcat(concat_params, " ");
  if (strlen((char *)xv_get(disp_col, PANEL_VALUE)) == 0)
  	{ error_msg(1); return; }
  strcat(concat_params, (char *)xv_get(disp_col, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(mu_col, PANEL_VALUE)) == 0)
  	{ error_msg(2); return; }
  strcat(concat_params, (char *)xv_get(mu_col, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(mu_fit_col, PANEL_VALUE)) == 0)
  	{ error_msg(3); return; }
  strcat(concat_params, (char *)xv_get(mu_fit_col, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(first_row, PANEL_VALUE)) == 0)
  	{ error_msg(4); return; }
  strcat(concat_params, (char *)xv_get(first_row, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(vs_row, PANEL_VALUE)) == 0)
  	{ error_msg(5); return; }
  strcat(concat_params, (char *)xv_get(vs_row, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(last_row, PANEL_VALUE)) == 0)
  	{ error_msg(6); return; }
  strcat(concat_params, (char *)xv_get(last_row, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(weight_row, PANEL_VALUE)) == 0)
  	{ error_msg(7); return; }
  strcat(concat_params, (char *)xv_get(weight_row, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(lin_term, PANEL_VALUE)) == 0)
  	{ error_msg(8); return; }
  strcat(concat_params, (char *)xv_get(lin_term, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(c_tol, PANEL_VALUE)) == 0)
  	{ error_msg(9); return; }
  strcat(concat_params, (char *)xv_get(c_tol, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(pt_lambda, PANEL_VALUE)) == 0)
  	{ error_msg(10); return; }
  strcat(concat_params, (char *)xv_get(pt_lambda, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(wc_panel, PANEL_VALUE)) == 0)
  	{ error_msg(11); return; }
  strcat(concat_params, (char *)xv_get(wc_panel, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(stiff, PANEL_VALUE)) == 0)
  	{ error_msg(12); return; }
  strcat(concat_params, (char *)xv_get(stiff, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(v_initial, PANEL_VALUE)) == 0)
  	{ error_msg(13); return; }
  strcat(concat_params, (char *)xv_get(v_initial, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(v_final, PANEL_VALUE)) == 0)
  	{ error_msg(14); return; }
  strcat(concat_params, (char *)xv_get(v_final, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(mu_initial, PANEL_VALUE)) == 0)
  	{ error_msg(15); return; }
  strcat(concat_params, (char *)xv_get(mu_initial, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(a, PANEL_VALUE)) == 0)
  	{ error_msg(16); return; }
  strcat(concat_params, (char *)xv_get(a, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(b1, PANEL_VALUE)) == 0)
  	{ error_msg(17); return; }
  strcat(concat_params, (char *)xv_get(b1, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if (strlen((char *)xv_get(dc1, PANEL_VALUE)) == 0)
  	{ error_msg(18); return; }
  strcat(concat_params, (char *)xv_get(dc1, PANEL_VALUE)); 
  strcat(concat_params, " ");
  if(dc2_flag != 0)	/*indicates 2sv model, panels will contain values and xv_get will work*/
  {
/*printf("in run proc, dc2_flag > 0, b2 is %s dc2 is %s\n",(char *)xv_get(b2, PANEL_VALUE),xv_get(dc2, PANEL_VALUE));*/
  if (strlen((char *)xv_get(b2, PANEL_VALUE)) == 0)
  	{ error_msg(19); return; }
  	strcat(concat_params, (char *)xv_get(b2, PANEL_VALUE)); 
  	strcat(concat_params, " ");
  if (strlen((char *)xv_get(dc2, PANEL_VALUE)) == 0)
  	{ error_msg(20); return; }
  	strcat(concat_params, (char *)xv_get(dc2, PANEL_VALUE)); 
  }
  else			/*otherwise, 1sv model. fill param strings and send defaults to qi*/
  {
	strcpy(parameter_strs[19], "1");
	strcpy(parameter_strs[20], "-1");
	strcat(concat_params, "1 -1 ");
  }
  xv_set(cmd_hist_panel_list, PANEL_LIST_INSERT, 
	 cmd_num, PANEL_LIST_STRING, cmd_num, concat_params, NULL);
  cmd_num++;
  
  if(mvs_flag == 1)			/*update mvs params*/
  {
    strcpy(mvs_params, (char *)xv_get(mvs_par, PANEL_VALUE));
    if( strlen(mvs_params) < 3)
    { 
	left_footer("Missing qi_mvs input for multiple velocity steps.");
	return;
    }
  }

  if ((alt_wt_flag == 1)&&( strlen(alt_wt_str) == 0))  
  {
    left_footer("Missing row number for alternative weighting.");
    return;
  }

  /*printf("\nconcat_params: %s", concat_params);*/
  command_handler(concat_params);
  
  strcpy(mvs_str, mvs_params);
  if (mvs_flag == 1) { 
    xv_set(cmd_hist_panel_list, 
	   PANEL_LIST_INSERT, cmd_num, 
	   PANEL_LIST_STRING, cmd_num, 
	   mvs_str, 
	   NULL); 
    cmd_num++;
    command_handler(mvs_str);
  }

  strcpy(alt_str, alt_wt_str);
  if (alt_wt_flag == 1) {
    xv_set(cmd_hist_panel_list, 
	   PANEL_LIST_INSERT, cmd_num, 
	   PANEL_LIST_STRING, cmd_num, 
	   alt_str, 
	   NULL); 
    cmd_num++;
    command_handler(alt_str);
  }
  

/*printf("in run proc: active_window = %d, active plot = %d\n",active_window, can_info->active_plot+1);*/
  if (active_window == -1 || can_info->active_plot == -1) {
    strcpy(plotcmd, "plotauto ");
    strcat(plotcmd, parameter_strs[1]);
    strcat(plotcmd, " ");
    strcat(plotcmd, parameter_strs[2]);
    strcat(plotcmd, " ");
    strcat(plotcmd, parameter_strs[4]);
    strcat(plotcmd, " ");
    strcat(plotcmd, parameter_strs[6]);
    command_handler(plotcmd);
  }
  
  strcpy(plotcmd, "plotover ");
  strcpy(parameter_strs[1], (char *)xv_get(disp_col, PANEL_VALUE));
  strcat(plotcmd, parameter_strs[1]);
  strcat(plotcmd, " ");
  strcpy(parameter_strs[3], (char *)xv_get(mu_fit_col, PANEL_VALUE));
  strcat(plotcmd, parameter_strs[3]);
  
  command_handler(plotcmd);
 
}


/* for kill button
   kills the R/S FRIC TOOL and frees memory */

void kill_qi_proc(item, event)
     Panel_item item;
     Event *event;
{
  Frame fr;
  int i;
      
  fr = (Frame) xv_get(item, PANEL_CLIENT_DATA);
  xv_destroy_safe(fr);
  qi_flag = 0;
    
  for(i=0;i<=20;++i)
    strcpy(parameter_strs[i],"");
}
  

void error_msg(indx)
     int indx;
{
  if (indx == 1)
    left_footer("Missing disp_col");
  if (indx == 2)
    left_footer("Missing mu_col");
  if (indx == 3)
    left_footer("Missing mu_fit_col");
  if (indx == 4)
    left_footer("Missing first_row");
  if (indx == 5)
    left_footer("Missing vs_row");
  if (indx == 6)
    left_footer("Missing last_row");
  if (indx == 7)
    left_footer("Missing weight_row");
  if (indx == 8)
    left_footer("Missing lin_term");
  if (indx == 9)
    left_footer("Missing c_tol");
  if (indx == 10)
    left_footer("Missing lambda");
  if (indx == 11)
    left_footer("Missing wc");
  if (indx == 12)
    left_footer("Missing stiff");
  if (indx == 13)
    left_footer("Missing v_initial");
  if (indx == 14)
    left_footer("Missing v_final");
  if (indx == 15)
    left_footer("Missing mu_initial");
  if (indx == 16)
    left_footer("Missing a");
  if (indx == 17)
    left_footer("Missing b1");
  if (indx == 18)
    left_footer("Missing dc1");
  if (indx == 19)
    left_footer("Missing b2");
  if (indx == 20)
    left_footer("Missing dc2");

  sprintf(msg, "Input error.  R/S_fric_tool does not have parameters set properly.\nSee Left Footer of R/S Fric Tool window\n\n");
 	print_msg(msg);
}

/* convenience procedure to handle foot notes */

void left_footer(txt)
     char txt[256];
{
  xv_set(qi_frame, FRAME_LEFT_FOOTER, txt, NULL);
}

void update_params()
{
  int i;
  xv_destroy_safe(qi_frame);
    
  for(i=0;i<=20;++i)
    strcpy(parameter_strs[i],"");

  create_qi_canvas();
  qi_flag = 1;
}







