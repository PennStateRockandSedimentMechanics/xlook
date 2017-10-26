#include <stdio.h>
#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <xview/xv_xrect.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/font.h>

/* #include <xview/textsw.h> 
#include <usercore.h> */
 
#define  ROW_SIZE 500000
#define  MAX_FILES 20
#define  MAX_COLS 2000
#define OUT 0
#define IN 1
#define BUF_SZ 1000
#define GC_TICK 1
#define GC_TITLE 2 

 
int     n_cols[MAX_FILES], n_rows[MAX_FILES];

float   *data_ptr[MAX_FILES][MAX_COLS];
char    heading[BUF_SZ];
char    n_head[BUF_SZ];

char    string[100],skip[15],new_file[20]; 
void    *calloc();
FILE    *fopen();
int     ii,segname,file_num,i,j; 
int     n_files;
int     first_file;
char	set_x_range = 'n';
char	set_y_range = 'n';
float   scale_x, scale_y;
int     start_x, start_y, end_x, end_y;
float   maxx,minx,maxy,miny;
int     tickfontheight, tickfontwidth, titlefontheight, titlefontwidth;

Frame mspframe;
Panel buttonpanel, infopanel;
Panel_item X, Y, ROW, plot_button;
Canvas plotwin;


void redraw(), event_proc();

main(argc,argv)
     int         argc;
     char       *argv[];
{

  Display *dpy;
  XGCValues gcvalues;
  GC gctitle, gctick;
  Xv_Font titlefont, tickfont;

  void clr_plot(), new_plot(), plot(), add_plot(), print_plot(), exit();
 
  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);
  Xv_window xvwindow;
  Window win;
  
  if (argc > 1)
    {
      process_input(argc, argv);
      process_files(argv);
    }
  else 
    {
	fprintf(stderr,"usage:  xmsp (-naxy) filename filename ...\n");
	fprintf(stderr,"default is for all files to have a first line header\n");
	fprintf(stderr,"-n only first file has first line heading\n");
	fprintf(stderr,"-a neither file has first line heading\n");
	fprintf(stderr,"-x#t# set x plot scale, first val. is min, second is max\n");
	fprintf(stderr,"-y#t# set y plot scale, first val. is min, second is max\n");
	fprintf(stderr,"  NOTE: for x and y scales, the letter t separates the min & max\n");
      exit(0);
    }
  

  mspframe=(Frame)xv_create(XV_NULL, FRAME, FRAME_LABEL, "MSP", NULL);
  
  buttonpanel=(Panel)xv_create(mspframe, PANEL, XV_HEIGHT, 35, NULL);
  (void) xv_create(buttonpanel, PANEL_BUTTON,
                   PANEL_LABEL_STRING, "CLEAR PLOT",
                   PANEL_NOTIFY_PROC, clr_plot,
                   NULL);
  
  (void) xv_create(buttonpanel, PANEL_BUTTON,
                   PANEL_LABEL_STRING, "NEW PLOT",
                   PANEL_NOTIFY_PROC, new_plot,
                   NULL);

  (void) xv_create(buttonpanel, PANEL_BUTTON,
                   PANEL_LABEL_STRING, "PLOT",
		   PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
                   PANEL_NOTIFY_PROC, plot,
                   NULL);
  
  (void) xv_create(buttonpanel, PANEL_BUTTON,
                   PANEL_LABEL_STRING, "ADD PLOT",
                   PANEL_NOTIFY_PROC, add_plot,
                   NULL);

  (void) xv_create(buttonpanel, PANEL_BUTTON,
                   PANEL_LABEL_STRING, "PRINT",
                   PANEL_NOTIFY_PROC, print_plot,
                   NULL); 

  (void) xv_create(buttonpanel, PANEL_BUTTON,
                   PANEL_LABEL_STRING, "QUIT",
                   PANEL_NOTIFY_PROC, exit,
                   NULL);

  infopanel=(Panel)xv_create(mspframe, PANEL, XV_HEIGHT, 30, NULL);
    
  X = (Panel_item)xv_create(infopanel, PANEL_TEXT,
                            PANEL_LABEL_STRING, "X: ",
			    PANEL_VALUE_DISPLAY_LENGTH, 8,
			    NULL);
  
  Y = (Panel_item)xv_create(infopanel, PANEL_TEXT,
                            PANEL_LABEL_STRING, "Y: ",
			    PANEL_VALUE_DISPLAY_LENGTH, 8,
			    NULL);
  
  ROW = (Panel_item)xv_create(infopanel, PANEL_NUMERIC_TEXT,
			      PANEL_LABEL_STRING, "ROW#: ",
			      PANEL_MAX_VALUE, 100000,
			      NULL);
 

  plotwin=(Canvas)xv_create(mspframe, CANVAS, 
			    CANVAS_REPAINT_PROC, redraw,
			    CANVAS_AUTO_SHRINK, TRUE,
			    CANVAS_AUTO_EXPAND, TRUE,
			    WIN_BELOW, infopanel,
			    /* XV_HEIGHT, canvas_height,
			    XV_WIDTH, canvas_width,  */
			    CANVAS_X_PAINT_WINDOW, TRUE,
			    NULL);


  xv_set(canvas_paint_window(plotwin),
	 WIN_EVENT_PROC, event_proc,
	 /*WIN_CONSUME_EVENTS, WIN_NO_EVENTS, WIN_MOUSE_BUTTONS, NULL, removed 28.3.08 --this was causing a blank plot on start */
	 NULL);
  
  titlefont=(Xv_Font)xv_find(plotwin, FONT,
/*			       FONT_NAME, "-adobe-courier-medium-r-normal--14-140-75-75-m-90-iso8859-1",
	*/
			       FONT_FAMILY, FONT_FAMILY_LUCIDA,
			       FONT_STYLE, FONT_STYLE_NORMAL,
			       FONT_SIZE, 14,
			       NULL);

  tickfont=(Xv_Font)xv_find(plotwin, FONT,
			    /*FONT_RESCALE_OF, titlefont, WIN_SCALE_SMALL,*/
			    FONT_FAMILY, FONT_FAMILY_LUCIDA,
			    FONT_STYLE, FONT_STYLE_NORMAL,
			    FONT_SIZE, 12,
			    NULL);
	
  dpy = (Display *)xv_get(mspframe, XV_DISPLAY);

  gcvalues.font = (Font)xv_get(titlefont, XV_XID);
  gcvalues.foreground = BlackPixel(dpy, DefaultScreen(dpy));
  gcvalues.background = WhitePixel(dpy, DefaultScreen(dpy));
  gcvalues.graphics_exposures = FALSE;
  gctitle = XCreateGC(dpy, RootWindow(dpy, DefaultScreen(dpy)), GCForeground | GCBackground | GCFont | GCGraphicsExposures, &gcvalues);
  
  gcvalues.font = (Font)xv_get(tickfont, XV_XID);
  gcvalues.foreground = BlackPixel(dpy, DefaultScreen(dpy));
  gcvalues.background = WhitePixel(dpy, DefaultScreen(dpy));
  gcvalues.graphics_exposures = FALSE;
  gctick = XCreateGC(dpy, RootWindow(dpy, DefaultScreen(dpy)), GCForeground | GCBackground | GCFont | GCGraphicsExposures, &gcvalues);

  xv_set(plotwin, XV_KEY_DATA, GC_TICK, gctick, NULL);
  xv_set(plotwin, XV_KEY_DATA, GC_TITLE, gctitle, NULL);
  
  titlefontheight=(int)xv_get(titlefont, FONT_DEFAULT_CHAR_HEIGHT);
  titlefontwidth=(int)xv_get(titlefont, FONT_DEFAULT_CHAR_WIDTH);

  tickfontheight=(int)xv_get(tickfont, FONT_DEFAULT_CHAR_HEIGHT);
  tickfontwidth=(int)xv_get(tickfont, FONT_DEFAULT_CHAR_WIDTH);
  
  window_fit(mspframe);

  xv_main_loop(mspframe);
}




/* ------------------------ event handler -------------------------*/
  
void clr_plot(item, event)
     Panel_item item;
     Event *event;
{
  Display *dpy;
  Window win;
  GC gc;
  
  dpy = (Display *)xv_get(plotwin, XV_DISPLAY);
  win=(Window)xv_get(canvas_paint_window(plotwin), XV_XID);

  printf("clearing window \n");
  
  XClearWindow(dpy, win);
}


void new_plot(item, event)
     Panel_item item;
     Event *event;
{
  printf("new plot\n");
  
}

void plot(item, event)
     Panel_item item;
     Event *event;
{
  Xv_window xvwindow;
  void redraw();
 
  xvwindow=canvas_paint_window(plotwin);
  
  redraw(plotwin, xvwindow,
	 xv_get(plotwin, XV_DISPLAY), 
	 xv_get(xvwindow, XV_XID), 
	 (Xv_xrectlist *) NULL); 

}


void add_plot(item, event)
     Panel_item item;
     Event *event;
{
  printf("add plot\n");
  
}


void print_plot(item, event)
Panel_item item;
Event *event;
{
  printf("printing plot\n");

}


 



      



/* --------------------------- other procedures ------------------------ */


process_input(argc, argv)
     int argc;
     char *argv[];
{
  int i;
  int done = 0;
  char buf[100];
  char *strtok();

  segname=first_file=1;
  sprintf(new_file, "hardcopy");
  
  first_file = 1;
  i=1;

  while(!done)
  {
    if (*argv[i] == '-')
    {
      if(*(argv[i]+1) == 'a' || *(argv[i]+1) == 'n')
      	skip[1] = *(argv[1]+1);
      else
      {
      	if(*(argv[i]+1) == 'x')
	{
		strcpy(buf, (argv[i]+2) );		
		sscanf(strtok(buf,"t"),"%f",&minx);
		sscanf(strtok(NULL,"t"),"%f",&maxx);
		set_x_range = 'y';
	}
      	if(*(argv[i]+1) == 'y')
	{
		strcpy(buf, (argv[i]+2) );		
		sscanf(strtok(buf,"t"),"%f",&miny);
		sscanf(strtok(NULL,"t"),"%f",&maxy);
		set_y_range = 'y';
	}
      }
      i++;
      first_file++;		/*this points to the first "file_name" in the cmd line*/
    }
    else 
    {
	done = 1;
	--i;
    }
  }
  n_files = argc - first_file;	/*if no options in cmd line, number of files is (ac - 1)*/

}




process_files(argv)
     char *argv[];
{
  int col, row;
  float tmp_xmin, tmp_xmax, tmp_ymin, tmp_ymax;

  file_num = 0;
  j = first_file;
  for (i=0; i<n_files;++i)
    read_data(argv[j++], skip, file_num++);

  tmp_xmin = minx; tmp_xmax = maxx; tmp_ymin = miny; tmp_ymax = maxy;

  maxx = maxy = -1e20;
  minx = miny = 1e20;
  for (i=0; i<n_files; ++i)
    {
      for (row=0; row<n_rows[i]; ++row)
	{
	  if (data_ptr[i][0][row] > maxx) 
	    maxx = data_ptr[i][0][row];
	  if (data_ptr[i][0][row] < minx) 
	    minx = data_ptr[i][0][row];
	}
      for (col=1; col<n_cols[i]; ++col)
	{
	  for (row=0; row<n_rows[i]; ++row)
	    {
	      if (data_ptr[i][col][row] > maxy)
		maxy = data_ptr[i][col][row];
	      if (data_ptr[i][col][row] < miny)
		miny =  data_ptr[i][col][row];
	    }
	}
    }
   if(set_x_range == 'y')
   {
	minx = tmp_xmin; 
	maxx = tmp_xmax ;
   }
   if(set_y_range == 'y')
   {
	miny = tmp_ymin; 
	maxy = tmp_ymax ;
   }

fprintf(stderr,"min_x= %g, max_x= %g\tmin_y=%g, max_y%g\n",minx,maxx,miny,maxy);
}



/* ------------------------ get_row_number ------------------------ */

int get_row_number(nrow, x1, y1)
     int nrow;
     float x1, y1;
{
  int   ii,stop;
  
  /* find row number of picked value */
  /* check x first -once the picked x is between two rows check y*/
  /* for y allow value to be either < the first and > the second or vise versa*/  

  
  ii=stop=0;
  
  --nrow;                       /*this takes care of checking one past the last point in the x+1 check*/
  
  while(stop ==0 && ii < nrow){
    if(((x1 >= data_ptr[0][0][ii] && x1 <= data_ptr[0][0][ii+1]) || 
	(x1 <= data_ptr[0][0][ii] && x1 >= data_ptr[0][0][ii+11])) && 
       (fabs(y1 - data_ptr[0][1][ii]) < (data_ptr[0][1][ii]/100))) 
      stop=1;
        
    else 
      ++ii;
    
  }

  return(ii);
    
}



/* ------------------------ read_data -------------------------- */

read_data( file, skip, file_num)
     int  file_num;
     char *file,skip[];
{
  FILE   *data, *fopen();
  static int n = 0;
  int    i, col, row, flag, state;
  float  garb, garb1;
  char   c, buf[BUF_SZ];
  
  if ((data = fopen( file, "r")) == NULL ) 
    {
      fprintf( stderr, "can't open data file\n");
      exit( 10);
    }
  
  /* find out how many cols there are */
  /* do this first so that I can then just rewind back to beginning of file*/
  for(i=0;i<4;++i)
    fgets(buf,BUF_SZ,data);     /* look at the 4th line, assume this is past any header stuff */
  
  rewind(data);
  
  if(skip[1] =='a' || (skip[1]=='n' && file_num>0))
    i=0;        /*do nothing, easier to read (think about...) */
  else
    {
      flag=1;
      for(i=0;(c=fgetc(data))!=EOF && c!='\n'; i++)
        {
          if(flag)
            {
              heading[i]=c;
              if(i>35 && (c==' ' || c=='\t'))
                {
                  heading[++i]='\0';
                  flag=0;
                  fscanf(data,"%s",n_head);
                  i=strlen(n_head)-1;
                }
	    }
          else
            n_head[i]=c;
	}
      n_head[++i]='\0';
    }
  
  /*printf("nhead: %s\n", n_head);*/
  
  state=OUT;                    /* count the number of cols in each row */
  for(n_cols[file_num]=i=0; buf[i] != '\0'; ++i)
    {
      if(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n')
        {
	  state=OUT;
	}
      else if(state==OUT)
        {
          state=IN;
          ++n_cols[file_num];
        }
    }
  /*allocate space*/
  for(col=0;col<n_cols[file_num];++col)
    data_ptr[file_num][col] = (float*) calloc(ROW_SIZE, sizeof(float));
  
  
  /*read data*/
  col=row=0;
  while (fscanf(data, "%f", &(data_ptr[file_num][0][row])) != EOF)
    {
      for(col=1;col<n_cols[file_num];++col)
	{
	  fscanf( data, "%f", &(data_ptr[file_num][col][row]));
	}
      ++row;
    }      
  n_rows[file_num] = row;          
      
  fclose(data);
  printf("file %s has %d rows and %d cols \n", file, n_rows[file_num], n_cols[file_num]);
}



void event_proc(window, event)
     Xv_window window;
     Event *event;
 {
  int xloc, yloc;
  int row_num;
  float xval, yval;
  char string[20];
  

  switch(event_action(event))
    {
    case ACTION_SELECT:
      if (event_is_up(event))
	{      
	  xloc = event_x(event);
	  yloc = event_y(event);
	  if (xloc > start_x && xloc < end_x && yloc < start_y && yloc > end_y)
	    {
	      xval = ((float)(xloc - start_x))/scale_x + minx;
	      yval = ((float)(start_y - yloc))/scale_y + miny;

	      sprintf(string, "%.5g", xval);

	      xv_set(X, PANEL_VALUE, string, NULL);

	      sprintf(string, "%.5g", yval);

	      xv_set(Y, PANEL_VALUE, string, NULL);

	      row_num = get_row_number(n_rows[0], xval, yval);
	      if (row_num < n_rows[0])
		xv_set(ROW, PANEL_VALUE, row_num, NULL);
	      else printf("Pick again. The point you picked was not on the curve\n"); 
	    }
	}
      break;
    case WIN_REPAINT:
      return;
    case WIN_RESIZE:
      redraw(plotwin, window,
	     xv_get(plotwin, XV_DISPLAY), xv_get(window, XV_XID), NULL);
      break;  /* if set to return, need to click on the canvas to display */ 
    default: 
      return;
    }

}

/* --------------------------- axes_and_ticks ----------------------- */

void redraw(canvas, paint_window, dpy, win, area)
     Canvas canvas;
     Xv_Window paint_window;
     Display *dpy;
     Window win;
     Xv_xrectlist *area;

{
  GC gctick, gctitle;
  int a;
  float big_tickx, big_ticky;
  int 	i, j;
  char	string[128];
  float x1, x2, y1, y2;
  int col, row;
  int labelx, labely;
  int tickx, ticky;
  int start_xaxis, start_yaxis, end_xaxis, end_yaxis;
  int tmp;
  double  pow(), log10(), floor(), ceil();
  double diffx, diffy;
  double decx, decy;
  double ten = 10.000;
  float stop_xmin, stop_xmax, stop_ymin, stop_ymax;
  int stringlen, where_x, where_y;
  int canvas_width, canvas_height;
  
  

  XClearWindow(dpy, win);
  gctick = (GC)xv_get(canvas, XV_KEY_DATA, GC_TICK);
  gctitle = (GC)xv_get(canvas, XV_KEY_DATA, GC_TITLE);
  
  diffx=maxx-minx;
  diffy=maxy-miny;
 
  canvas_width = (int) xv_get(plotwin, XV_WIDTH);
  canvas_height = (int) xv_get(plotwin, XV_HEIGHT);

  start_xaxis = (int)canvas_width/10*1.5;
  end_xaxis = canvas_width - (int)canvas_width/10*1.5;

  start_yaxis = canvas_height - (int)canvas_height*0.1;
  end_yaxis = (int)canvas_height*0.2;

  start_x = (int)canvas_width/5;  /* starts at 0.2 width  */
  end_x = canvas_width - (int)canvas_width/5;  /* ends at 0.2 width */
  
  start_y = canvas_height - (int)canvas_height*0.15;  /* starts at 0.15 */
  end_y = (int)(canvas_height*0.25); /* ends at 0.75 */ 
  
  scale_x = (float)((float)end_x - (float)start_x)/diffx;
  scale_y = (float)((float)start_y - (float)end_y)/diffy;
  
  
  /* x-axis  */
  XDrawLine(dpy, win, gctick, 
	    start_xaxis, start_yaxis, 
	    end_xaxis, start_yaxis);

  /* left y-axis  */
  XDrawLine(dpy, win, gctick,
	    start_xaxis, start_yaxis,
	    start_xaxis, end_yaxis); 

  /* right y-axis  */
  XDrawLine(dpy, win, gctick, 
	    end_xaxis, start_yaxis,
	    end_xaxis, end_yaxis);
  
  

  ticky=(int)canvas_width*0.02;
  tickx=(int)canvas_height*0.02;

  stop_xmax = maxx + diffx/20;
  stop_ymax = maxy + diffy/20;

  stop_xmin = minx - diffx/20;
  stop_ymin = miny - diffy/20;

 
/* figure out where to round scales so ticks come out as a factor of 10 or 5*/
 
/* take min=101 max = 154 diff=53,      round 101 to 110 by dividing 101 by*/
/*                                      10 raised to (floor(log10(53))=1) */
/*                                      i.e. 101/10 = 10.1 round to 11 using*/
/*                                      ceil.  then get back to 110 with 11 */
/*                                      times 10 to the one                */
 
  /*      decx = (double)floor(log10(diffx));     i.e. return -2 for -1.5, 1 for 1.5*/
 
 
  decx=(double)floor(log10(diffx));
  decy=(double)floor(log10(diffy));

  
  big_tickx= floor(minx*pow(ten,-decx)) * pow(ten, decx);
  big_ticky= floor(miny*pow(ten,-decy)) * pow(ten, decy);
/*fprintf(stderr, "diffx = %f, maxx=%f, minx=%f, diffy=%f, maxy=%f, miny=%f\n",diffx, maxx, minx, diffy, maxy, miny); 
fprintf(stderr,"decx= %f\t big_tickx=%f decy=%f big_ticky=%f\n",decx,big_tickx,decy,big_ticky);
fprintf(stderr,"start_y=%f, start_x=%f, scale_y=%f, scale_x=%f\n",start_y, start_x, scale_y, scale_x);*/
     
  /* y axis big tick marks  */
  for (a=0;big_ticky<=stop_ymax;++a)
    {
      where_y = start_y-(int)((big_ticky-miny)*scale_y);
  
				/*because of the way we set "big_tick"s and "start_y" it's possible
for the first tick to fall outside the plot*/
      if(where_y < start_yaxis)    	
      {
      XDrawLine(dpy, win, gctick, 
		start_xaxis, where_y,
  		start_xaxis+ticky, where_y);
      
      XDrawLine(dpy, win, gctick, 
		end_xaxis, where_y,
  		end_xaxis-ticky, where_y);
            
      sprintf(string, "%.5g", big_ticky);
      strcat(string, "\0");
            
      XDrawString(dpy, win, gctick,
		  start_xaxis - tickfontwidth*(strlen(string)+1),
		  start_y-(int)((big_ticky-miny)*scale_y)+tickfontheight/2,
		  string, strlen(string));
      } 		  
      big_ticky += pow(ten, decy);
      
    }
  
  /* y axis small tick mrks */
  if (a<2)
    {
      big_ticky -= 1.5 * pow(ten, decy);
      
      if (big_ticky < stop_ymin) big_ticky += pow(ten, decy);
      while (big_ticky < stop_ymax)
	{
	  where_y = start_y-(int)((big_ticky-miny)*scale_y);
      
	  XDrawLine(dpy, win, gctick, 
		    start_xaxis, where_y,
		    start_xaxis+ticky, where_y);
	  
	  XDrawLine(dpy, win, gctick, 
		    end_xaxis, where_y,
		    end_xaxis-ticky, where_y);
	  
	  sprintf(string, "%.5g", big_ticky);
	  strcat(string, "\0");
	  
	  XDrawString(dpy, win, gctick,
		      start_xaxis - tickfontwidth*(strlen(string)+1),
		      start_y-(int)((big_ticky-miny)*scale_y)+tickfontheight/2,
		      string, strlen(string));
	  
	  big_ticky += pow(ten, decy);
	}
    }



  /*  x axis big tick marks  */
  for (a=0;big_tickx<=stop_xmax;++a)
    {
      where_x=start_x + (int)((big_tickx-minx)*scale_x);

      if(where_x >= start_xaxis)    	
      {
      
      XDrawLine(dpy, win, gctick, 
		where_x, start_yaxis,
		where_x, start_yaxis - tickx);
            
      sprintf(string, "%.5g", big_tickx);
      strcat(string, "\0");
      stringlen = strlen(string);
          
      XDrawString(dpy, win, gctick,
		  where_x - tickfontwidth*stringlen/2,
		  start_yaxis + tickfontheight + 5,
		  string, stringlen);
      }

      big_tickx += pow(ten, decx);
    }
  
  /* x axis small tick marks */
  if (a<2)
    {
      big_tickx -= 1.5 * pow(ten, decx);
      
      if (big_tickx < stop_xmin) big_tickx += pow(ten, decx);
      while (big_tickx < stop_xmax)
	{
	  where_x=start_x + (int)((big_tickx-minx)*scale_x);
      
	  XDrawLine(dpy, win, gctick, 
		    where_x, start_yaxis,
		    where_x, start_yaxis - tickx);
	  
	  sprintf(string, "%.5g", big_tickx);
	  strcat(string, "\0");
	  stringlen = strlen(string);
          
	  XDrawString(dpy, win, gctick,
		      where_x - tickfontwidth*stringlen/2,
		      start_yaxis + tickfontheight + 5,
		      string, stringlen);
      		  
	  big_tickx += pow(ten, decx);
	}
    }


 
  if (strcmp(heading, "")==1) strcpy(heading, "Test Plot\n");  
  
  XDrawString(dpy, win, gctitle, 
	      start_xaxis, end_yaxis-4*titlefontheight, 
	      heading, strlen(heading));

  				/*put up to 50 chars on each line*/
  for(i=tmp=0;i<strlen(n_head)/90;i++)
  {
	for(j=0;j<90;j++)
  		string[j] = n_head[tmp++];
   	string[j] = '\0';
   	XDrawString(dpy, win, gctitle, start_xaxis, (end_yaxis+(-3+i)*titlefontheight), 
	  string, strlen(string));
  }
  j=0;
  while(tmp<=strlen(n_head))
	string[j++] = n_head[tmp++];
  XDrawString(dpy, win, gctitle, start_xaxis, (end_yaxis+(-3+i)*titlefontheight), 
    string, strlen(string));

  /* plot each point  */

  
  
  for (i=0; i<n_files; ++i)
      for (col=1; col<n_cols[i]; col++)
	for (row=1; row<n_rows[i]; row++)
	  {
	    x1 = (data_ptr[i][0][row-1] - minx);
	    x2 = (data_ptr[i][0][row] - minx);
	    y1 = (data_ptr[i][col][row-1] - miny);
	    y2 = (data_ptr[i][col][row] - miny);
	  
	    XDrawLine(dpy, win, gctick, 
		      start_x + (int)(x1*scale_x),
		      start_y - (int)(y1*scale_y),
		      start_x + (int)(x2*scale_x),
		      start_y - (int)(y2*scale_y));
	  }
}






