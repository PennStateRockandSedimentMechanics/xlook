#include	<stdio.h>
#include        <math.h>
#include	<stdlib.h>
#include	<string.h>
// #include 	<xview/canvas.h>

/*13.2.07  NOTE: header no longer has 'extras' and no longer uses EOH at the end*/

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#define DEBUG FALSE		/*use to turn debugging statements on and off in source*/
#define MAX_COL   33		/*set this at 33, which is 32 channels of data and a time channel */ 
				/*this is a default for memory allocation. It isn't a hard*/
				/*  limit. See allocate in "look_funcs.c" and the functions */
				/*  in filtersm.c  Also, main.c does an initialization*/
				/* Assume that data files, as recorded, are limited to 32 chans. */
#define MAX_ROW 200000
#define MAX_PLOTS 10		/*max number of plots for each window*/
#define MAX_N_VSTEPS 20		/*max number of v_steps for multiple velocity step (mvs) option*/
#define    BELL     '\007'
#define ALPHA   1.0             /*  reflection factor   */
#define BETA    0.5             /*  contraction factor  */
#define GAMMA   2.0             /*  expansion factor    */
#define MAX_PARAM 10            /*  max # of parameters to be optimised */
#define TWOPI 6.283185308
#define PI 3.141592654
#define SMALL 1e-10
#define NONE (-1)
#define SQR(a) (a*a)
#define WELCH_WINDOW(j,a,b) (1.0-SQR((((j)-1)-(a))*(b)))   /*see p 445 of numerical recipes in c*/
#define         HALF  0.5000
#define         ROUND(A)  ( ( (A-floor(A) ) > HALF ) ? ceil(A)  : floor(A) )

/*  %%%%%%%%%%%%%  GLOBAL DECLARATIONS  %%%%%%%%%%%%%  */

/*---------------------------------------------------------------*/
char global_error;

struct	channel 
{
  char	name[13];
  char	units[13];
  float	gain;
  char    comment[50];
  int     nelem;
};

/*13.2.07  NOTE: header no longer has 'extras' and no longer uses EOH at the end*/

struct	header 
{
  char	title[20];
  int	nchan;
  int     nrec;
  int	swp;
  float	dtime;
  struct	channel	ch[MAX_COL];
};
struct  header head ;

/*--------------------------------------------------------------*/
struct plot_sum
{
  int   nrow[MAX_COL]  ;
  float max[MAX_COL]   ;
  float min[MAX_COL]   ;
};
struct  plot_sum  plot_info ;

struct statistics
{
  int rec ;
  double mean ;
  double max ;
  double min ;
  double stddev ;
};
struct statistics col_stat ;

struct rs_parameters
{
  char	law;			/*d=Dieterich, r=Runia, j=Rice, p=Perrin*/
  int	one_sv_flag;		/*true if doing a 1sv case*/
  int	op_file_flag;	/* use bits, see cmds.c for key*/
  int	vs_row;
  int	first_row;
  int	last_row;
  int	mu_col;
  int	disp_col;
  int	mu_fit_col;
  int	weight_row;
  int	end_weight_row;
  int	weight_pts;
  int	weight_control;
  int	peak_row;
  int     added_pts;
  double  *disp_data;
  double  *mu_data;
  double  *model_mu;
  double	stiff;
  double	sig_n;
  double	v_s;
  double	v_lp;
  double	v_ref;
  double	mu_ref;
  double	vo;
  double	vf;
  double	muo;
  double	muf;
  double	amb;
  double	a;
  double	a_er;
  double	a_step;
  double	b1;
  double	b1_er;
  double	b1_step;
  double	b2;
  double	b2_er;
  double	b2_step;
  double	dc1;
  double	dc1_er;
  double	dc1_step;
  double	dc2;
  double	dc2_er;
  double	dc2_step;
  double	total_er;
  double	weight;
  double  vr_dc1;
  double  vr_dc2;
  double  vo_dc1;		/*changed to vr for real reference velocity*/
  double  vo_dc2;		/* but leave for backward compat.*/
  double  k_vf;
  double  lin_term;
  double  vel_list[MAX_N_VSTEPS];
  int	n_vsteps;
  int	vs_row_list[MAX_N_VSTEPS];
};
struct rs_parameters rs_param ;


/*--------------------------------------------------------------*/

double     *darray[MAX_COL] ;
int        max_col ;
int        max_row ;
int     n_param;
double  simp[MAX_PARAM+1][MAX_PARAM+1],temp[MAX_PARAM+1];

double	*arrayx, *arrayy;

FILE *command , *com_file[10] , *temp_com_file;


typedef struct plot_array 
{
	double *xarray;
	double *yarray;
	int nrows_x; /* probably need just one nrows */
	int nrows_y;
	int col_x;
	int col_y;
	int begin;
	int end;
	float xmin;
	float xmax;
	float ymin;
	float ymax;
	float scale_x;
	float scale_y;
	int label_type;


	// this stuff should probably be on the canvas info; it doesn't really make sense to have a "per plot array"
	// mouse tracking code; that's really per window.
/*
	int mouse;
	int x1;
	int y1;
	int x2;
	int y2;
	int p1;
	int p2;
	int zp1;
	int zp2;
*/
}plotarray; 

// forward declaration, so the core code doesn't have to know about GTK.
//extern struct GtkWidget;

typedef struct point2d {
	int x;
	int y;
} point2d;

struct mouse_tracking_data {
	int mode;
	int tracking;
	point2d start;
	point2d end;
	int zp1, zp2; // these are the zoom point (row numbers)
};

typedef struct canvas_info 
{
	struct plot_window *plot_window;
	plotarray *plots[MAX_PLOTS];
	int canvas_num;
	int active_plot;
	int total_plots;
	int alive_plots[MAX_PLOTS];
	int start_x;		/*these are in pixel coords and refer to the whole window*/
	int start_y;
	int end_x;
	int end_y;
	int start_xaxis;
	int start_yaxis;
	int end_xaxis;
	int end_yaxis;
	char point_plot;
	struct mouse_tracking_data mouse;
	struct offscreen_buffer *offscreen_buffer;
	struct offscreen_buffer *tracking_buffer; // to avoid evil flicker.
} canvasinfo;

#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(x[0])))

#define MAXIMUM_NUMBER_OF_WINDOWS (10)

struct windows_info 
{
  int windows[MAXIMUM_NUMBER_OF_WINDOWS];
  canvasinfo *canvases[MAXIMUM_NUMBER_OF_WINDOWS]; // why?  this is tiny, no reason for allocations here.
 /* int active_window = 0;  easier to declare as global var... */
};

struct windows_info wininfo;

/* UI Globals - Trying to unify some of these */
typedef struct
{
	struct GtkWidget *main_window;
	struct GtkWidget *command_history;
	struct rs_fric_window *rs_window; // was qi_flag.
	int active_window;
	int old_active_window;
	int total_windows;
	int action;
	int cmd_num;
	
	// for calculations of drawing
	int tickfontheight;
	int tickfontwidth;
	int titlefontheight; 
	int titlefontwidth;
} ui_globals_struct;

extern ui_globals_struct ui_globals;


/* There is one "wininfo"  It contains 10 "canvases" --each of which is a window that may contain
up to 10 plots*/
#define GC_TICK (1)
#define GC_TITLE (2)
#define CAN_X (1)
#define CAN_Y (2)
#define CAN_ROW (3)
#define CAN_NUM (4)
#define GRAF_FRAME (5)
#define CAN_PLOT_ROWS (6)
#define MSG_LENGTH 1024

extern char msg[MSG_LENGTH];


/*************************  action DEFINE variables ********************/
#define MAIN              (0)
#define SET_PATH          (5)
#define DOIT              (6)
#define STDIN             (7)
#define COM_FILE          (8)
#define ALL               (9)
#define READ              (10)
#define APPEND            (11)
#define WRITE             (12)
#define HEAD              (15)
#define GETASCHEAD         (16)
#define EXAMIN            (17)
#define OVERWRITE         (18)
#define ALL_NEED_ROW_COL  (20)
#define PLOT_GET_BE       (21)
#define PLOT_GET_XY       (22)
#define PLOT_OTHERS       (23)
#define PLOT_SCALE        (24)
#define KILL_PLOT          (25)
#define KILL_WIN          (26)
#define CREATE_WIN        (27)
#define SET_ACTIVE_WINDOW (28)
#define INTERPOLATE       (29)
#define INTERPOLATE_GET_F_L (30)
#define INTERPOLATE_GET_INC (31)
#define MATHINT           (32)
#define MATHINT_F_L       (33)
#define MATH (34)
#define R_MEAN (35) 
#define R_SPIKE (36)
#define POSITIVE (37)
#define COMPRESS (38)
#define Z_MAX (39)
#define Z_MIN (40)
#define TREND_XY (41)
#define TREND_F_L (42)
#define TREND_A (43)
#define FINDINT (44)
#define SUMMATION (45)
#define CURV (46)
#define PEAK (47)
#define DECIMAT (48)
#define DECIMAT_R (49)
#define DECIMAT_F_L (50)
#define PDF (51)
#define PDFAUTO (52)
#define R_ROW_COL (53)
#define R_ROW_COL_F_L (54)
#define R_ROW_F_L (55)
#define COMMENT_COL (56)
#define COMMENT_STR (57)
#define NAME (58)
#define NAME_GET_NAME (59)
#define NAME_GET_UNIT (60)
#define R_COL (61)
#define ZERO_ALL (62)
#define ZERO (63)
#define OFFSET (64)
#define OFFSET_INT (65)
#define OFFSET_INT_GET_YN (66)
#define SIMPLEX_GET_FN (67)
#define SIMPLEX_GET_F_L (68)
#define SIMPLEX_GET_COLS (69)
#define SCM_GET_ARGS (70)
#define EXAMIN_GET_FILENAME (71)
#define HEAD_GET_FILENAME (72)
#define GETASCHEAD_GET_FILENAME (73)
/*  These are not current/no-longer used. Removed 12.2.2010, cjm
no longer used #define TASC_GET_FILENAME (74)
no longer used #define TASC_GET_FI (75)
no longer used #define STDASC_GET_FILENAME (76)
no longer used #define STDASC_GET_FI (77)
no longer used #define STDASC_GET_CR (78)
*/
#define INTERPOLATE_1 (79)
#define INTERPOLATE_2 (80)
#define MATH_FINAL_1 (81)
#define INTERPOLATE_R (82)
#define COL_POWER (83)
#define POWER (84)
#define RECIP (85)
#define LOG (86)
#define EXPLIN (87)
#define POLY4 (88)
#define GENSIN (89)
#define GENEXP (90)
#define RCLOW (91)
#define RCPH (92)
#define SCCHISQR (93)
#define CHISQR (94)
#define NORMAL (95)
#define POWER2 (96)
#define POWER1 (97)
#define LN (98)
#define EXP (99)
#define DERIV (100)
#define DERIV_GET_INT (101)
#define VC (102)
#define VC_GET_INT (103)
#define VC_GET_F (104)
#define CGT (105)
#define RGT (106)
#define EC (107)
#define EC_GET_INT (108)
#define EC_GET_F (109)
#define CS (110)
#define CS_GET_INT (111)
#define POLYFIT (112)
#define POLYFIT_GET_INT (113)
#define POLYFIT_GET_INC (114)
#define POLYFIT_I (115)
#define POLYFIT_I_GET_INT (116)
#define POLYFIT_I_GET_INC (117)
#define POLYFIT_I_GET_ROW (118)
#define SORT (119)
#define SLOPE (120)
/*#define O_SLOPE (121)*/
#define RSM (122)
#define TRIG (123)
#define CM (124)
#define MEDIAN_SMOOTH (125)
#define SMOOTH (126)
#define MEM (127)
#define STAT (128)
#define TYPE (129)
#define TYPE_FL (130)
#define TYPE_S (131)
#define TYPEALL (132)
#define R_TREND (133)
#define R_TREND_TYPE (134)
#define R_TREND_INPUT (135)
#define R_TREND_COMP (136)
#define R_TREND_COMP_FL (137)
#define SIMP_FUNC_GET_MAX_ITER (138)
#define SIMP_FUNC_GET_INIT_GUESSES (139)
#define SIMP_FUNC_GET_INIT_STEP_SIZE (140)
#define SIMP_FUNC (141)
#define SCM_SIMP_WEIGHT_L2 (142)
#define SCM_SIMP_WEIGHT (143)
#define SCM_1 (144)
#define QI_MVS (145)
#define QI_WC (146)
#define QI (147)
#define SIMPLEX (148)
#define SCM (149)
#define SET_ACTIVE_PLOT (150)
#define STAT_A (151)



/*  name DEFINE variables  */
#define  SLOPE_NAME (1001)
#define  TRIG_NAME (1002)
#define  RCPH_NAME (1003)
#define  POWER_NAME (1004)
#define  RECIP_NAME (1005)
#define  LOG_NAME (1006)
#define  EXPLIN_NAME (1007)
#define  POLY4_NAME (1008)
#define  GENSIN_NAME (1009)
#define  GENEXP_NAME (1010)
#define  RCLOW_NAME (1011)
#define  SCCHISQR_NAME (1012)
#define  CHISQR_NAME (1013)
#define  LN_NAME (1014)
#define  NORMAL_NAME (1015)
#define  POWER1_NAME (1016)
#define  POWER2_NAME (1017)
#define  EXP_NAME (1018)
#define  DERIV_NAME (1019)
#define  VC_NAME (1020)
#define  RGT_NAME (1021)
#define  CGT_NAME (1022)
#define  EC_NAME (1023)
#define  CS_NAME (1024)
#define  POLYFIT_NAME (1025)
#define  INTERPOLATE_NAME (1026)
#define  MATH_NAME (1027)
#define  MATHINT_NAME (1028)
#define  POSITIVE_NAME (1029)
#define  COMPRESS_NAME (1030)
#define  Z_MIN_NAME (1031)
#define  Z_MAX_NAME (1032)
#define  SUMMATION_NAME (1033)
#define  DECIMAT_NAME (1034)
#define  CURV_NAME (1035)
#define  PEAK_NAME (1036)
#define  R_MEAN_NAME (1037)
#define  SMOOTH_NAME (1038)
#define  MEDIAN_SMOOTH_NAME (1039)
#define  R_TREND_COMP_NAME (1040)
#define  R_TREND_INPUT_NAME (1041)
#define  INTERPOLATE_NAME1 (1042)
#define  INTERPOLATE_NAME2 (1043)

