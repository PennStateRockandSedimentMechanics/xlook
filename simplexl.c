/*  simplex  is a method for optimising coefficients in an equation.
 *
 *  if the form of a function to fit some data is known, then values
 *  for the coefficients in the equation can be found, according to
 *  some error criterion, with total error better than some specified value
 *  
 *  this version needs to be compiled with functions 
 *
 *  void    get_initial_values( temp, errormax, first_step, &n_param, &iter_max)
 *	double temp[] enters the initial guesses for the parameters
 *	double errormax[] gives the criteria for stopping the iteration
 *	float first_step[] enters the initial offsets to the guesses
 *	int   n_param  gives the # parameters being optimised
 *	int   iter_max  enters the max # iterations, if the stopping criteria are not met
 *
 *  void    table_write( temp)
 *	double temp[] passes the solution, so that a table of results may be prepared
 *
 *  double   error( funcname, temp, x, y, ndata)
	char funcname[] contains name of function to fit to data
 *  	double temp[] contains a guess at up to MAX_PARAM coefficients
 *  	float x[ndata][10], y[ndata]  is the data read from a file specified as a command-line argument 
 *      --it returns a value for the total error between the predicted values and the data
 *
 *  Simon Cox,  June 1985.
 *		revised to accept multiple independent variables, May 1986.
 *  Greg Boitnott
 *		adapted for use in lookv3 with some corrections, Sept 1988.
 *
    Chris Marone
                adapted for use with rate and state friction calculation, June 1991.
	     
    Lokman Alwi  
                adapted for use with xlook, Jan 1995.
    Deepak Kumar
                modifications implemented, Apr 1996.
*/

#include "global.h"
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <strings.h>


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

extern do_scm_2();


extern int action;
extern int simp_func_action;
extern char msg[MSG_LENGTH];
		
extern int l, first, last, temp_int, simp_xch[MAX_COL], simp_ych;
extern char t_string[300];

FILE 	*op_file;
time_t	tp;
int		done;
static int	n, i, j;
int		ndata, iter_max, scrn_write, sw_num;
int         high[MAX_PARAM+1], low[MAX_PARAM+1];
float       first_step[MAX_PARAM];
double	centre[MAX_PARAM];
double	errormax[MAX_PARAM+1], error(), scm_err[MAX_PARAM+1];
double	simp_rate_state_mod();
char	*strcat();
char	buf[80];
int max_iter;


do_simp_func()
     /*(infile,free,xch,ych,funcname,first,last,max_iter)
       FILE *infile;
       int free, xch[], *ych;
       char funcname[];
       int first, last, max_iter; */
{
  
  max_iter = temp_int;
  
  n_param = l;
  ndata = last - first + 1;
  
  /*
   *  get the starting guesses for the parameters .. in the array simp[0][]
   *  the stopping criteria .. in the array errormax[]
   *  the initial offsets .. in first_step[]
   *  the # of parameters being optimised
   *  and the max # of iterations allowed
   *
   *  and then send a nicely formatted copy to outf
   */
  
  if( strncmp(t_string, "rs_fit", 6) == 0 )
    {
      if(n_param == 4)		/* two state variable fit */
	{
	  simp[0][0] = temp[0] = rs_param.a;	/* these are set in look */
	  simp[0][1] = temp[1] = rs_param.b1;
	  simp[0][2] = temp[2] = rs_param.dc1;
	  simp[0][3] = temp[3] = rs_param.dc2;
	  
	  errormax[0] = rs_param.a_er;  
	  errormax[1] = rs_param.b1_er;
	  errormax[2] = rs_param.dc1_er;
	  errormax[3] = rs_param.dc2_er;
	  errormax[4] = rs_param.total_er; 
	  
	  first_step[0] = rs_param.a_step;
	  first_step[1] = rs_param.b1_step;
	  first_step[2] = rs_param.dc1_step;
	  first_step[3] = rs_param.dc2_step;
	}
      
      else if(n_param == 2)		/* only doing 1 state variable fit... */
	{
	  simp[0][0] = temp[0] = rs_param.a;	/*set in look */
	  simp[0][1] = temp[1] = rs_param.dc1;
	  
	  errormax[0] = rs_param.a_er;  
	  errormax[1] = rs_param.dc1_er; 
	  errormax[2] = rs_param.total_er; 
	  
	  first_step[0] = rs_param.a_step;        
	  first_step[1] = rs_param.dc1_step;
	}
      else
	{
	  sprintf(msg,"simplex got confused about whether it was a 1 or 2 state variable fit\n"); 
	  print_msg(msg);
	}
      
      iter_max = max_iter;
      scrn_write = (int)(max_iter/10 -1);	/* keep user informed of progress... */
      sw_num = 1;
      simp_func_final();
      
    }
  else			/* -one of the other simplex look functions... */
    {

      /* get args for get_initial_values
	 get_initial_values(errormax, first_step, &iter_max);
	 init_values_write( errormax,stdout);
	
	 then  calculate the initial error
	 for ( j=0; j<n_param; ++j)  simp[0][j] = temp[j]; 
	
	 then call simp_func_final()
	 */
          
      set_left_footer("Type the maximum number of iterations needed");
      set_cmd_prompt("Max Iter: ");
      
      action = SIMP_FUNC_GET_MAX_ITER;
      return;
    }
  
}


do_get_initial_values(arg)
     char arg[256];
{
  static int i;

  nocom(arg);
  
  sscanf(arg, "%d", &iter_max);

  for(i=0; i<n_param; ++i)
    {
      sscanf(arg, " %lf",&temp[i]); 
    }

  for(i=0; i<n_param; ++i)
    {
      errormax[i] = 0.0;
    }

  for(i=0; i<n_param; ++i)
    {
      sscanf(arg, " %f",&first_step[i]); 
    }

  init_values_write( errormax,stdout);
	
  for ( i=0; i<n_param; ++i)  simp[0][i] = temp[i]; 
}

  

simp_func_final()
{
  
  simp[0][n_param] = error( t_string, simp_xch, simp_ych, ndata, first);
  
  get_starting_simplex(t_string,first_step,simp_xch,simp_ych,ndata,first);
  sprintf(msg, "starting simplex:\n");
  print_msg(msg);
  printSimplex(stdout, t_string);
  
  if( strncmp(t_string, "rs_fit", 6) == 0 )
    {
      strcpy(buf,"simplex_log_");
      strcat(buf,head.title);
      op_file = fopen(buf, "a");	/* append to default file */
	fprintf(op_file,"\n**************************************************\n\n");
      time(&tp);
      fprintf(op_file,"Fit for Exp: %s \t\t Date: %s\n", head.title, ctime(&tp));
      fprintf(op_file,"recs fit: %d to %d, v_init. = %5.3f, v_f = %5.3f, mu_init. = %5.4f, mu_f = %5.4f\nRows %d to %d weighted by %5.3f\tPeak row (%d) weighted by %5.3f %s\n", rs_param.vs_row, rs_param.last_row, rs_param.vo, rs_param.vf, rs_param.muo, rs_param.muf, rs_param.vs_row+1, rs_param.weight_pts, rs_param.weight, rs_param.peak_row, rs_param.weight*6.000, ((rs_param.weight_control < 0) ? ("\n***Weighting adjusted: Pre-peak overestimate is favored over underestimate.") : (" ")) ); 
      if(n_param == 4)
	fprintf(op_file,"Simplex parameters: max_interations %d,\n  Tolerances: a= %5.3g,\tb1= %5.3g,\tdc1= %5.3g,\tdc2= %5.3g,\ttotal error= %5.3g\n  Init. step size:\t a= %5.3g,\tb1= %5.3g,\tdc1= %5.3g,\tdc2= %5.3g\n", max_iter, rs_param.a_er, rs_param.b1_er, rs_param.dc1_er, rs_param.dc2_er, rs_param.total_er, rs_param.a_step, rs_param.b1_step, rs_param.dc1_step, rs_param.dc2_step); 
      else	
	fprintf(op_file,"Simplex parameters: max_interations %d,  Tolerances: a= %5.3g,\tdc= %5.3g\ttotal error= %5.3g\n  Initial step size:\t a= %5.3g,\tdc= %5.3g\n", max_iter, rs_param.a_er,  rs_param.dc1_er,  rs_param.total_er, rs_param.a_step,  rs_param.dc1_step); 
      
      fprintf(op_file, "starting simplex:\n");
      printSimplex(op_file, t_string);
    }
/*
 *  find the high and low values for each parameter 
 */
  
  for ( i=0; i<=n_param; ++i) {		/* initialise */
    low[i]=0; 
    high[i]=0;
  }
  order( high, low);
  
  /**********************************
   *  START ITERATION
   */
  
  for ( n=0, done=0; n<iter_max && done==0; ++n)
    {
      
      find_centroid(centre,high[n_param]);
      
      reflect_worst(centre,high[n_param]);
      
      /*
       * TEST IT:	BETTER THAN BEST?
       */
      
      if( (temp[n_param] = error(t_string,simp_xch,simp_ych,ndata,first)) <= simp[ low[n_param] ][n_param] )
	{
	  saveAs_new_vertex( high[n_param]);
	  expand_reflection(centre,high[n_param]);
	  
	  /*
	   *  BETTER STILL ?
	   */
	  
	  if( (temp[n_param] = error(t_string,simp_xch,simp_ych,ndata,first)) <= simp[ low[n_param] ][n_param] )
	    saveAs_new_vertex( high[n_param]);
	}
      
      /*
       *  OR ONLY BETTER THAN WORST..?
       */
      
      else if( temp[n_param] <= simp[ high[n_param] ][n_param] )
	saveAs_new_vertex( high[n_param]);
      
      /*
       *  IF WORSE THAN WORST,..
       */
      
      else 
	{
	  contract_worst(centre,high[n_param]);
	  
	  /*
	   *  NOW BETTER THAN WORST?
	   */
	  
	  if( (temp[n_param] = error(t_string,simp_xch,simp_ych,ndata,first)) <= simp[ high[n_param] ][n_param] )
	    saveAs_new_vertex( high[n_param]);
	  else 
	    {
	      contract_all(t_string,centre,low[n_param],simp_xch,simp_ych,ndata,first);
	    }
	} 
      
      /*
       *  CHECK FOR CONVERGENCE
       */
      
      order( high, low);
      for ( i=0, done=1; i<=n_param; ++i) {
	scm_err[i] = fabs( (simp[ high[i] ][i] - simp[ low[i] ][i])/simp[ high[i] ][i] );
	/* flag to stop if parameter is essentially zero */
	if ( scm_err[i] > errormax[i] && simp[high[i]][i] > 1e-7 )		done = FALSE;
      }
      if( strncmp(t_string, "rs_fit", 6) == 0 )
	{
	  if(n==0)
	    {
	      sprintf(msg,"Iteration: 0  ");
	      print_msg(msg);
	    }
	  else if(n > scrn_write)
	    {
	      sw_num++;
	      scrn_write = (max_iter/10) * sw_num -1;
	      sprintf(msg,"%d  ", n);
	      print_msg(msg);
	    }
	  if( temp[n_param] < 0 ) 
	    {
	      sprintf(msg,"\nSimplex main loop: error in fit. error() returned %g -Exiting loop early....\n", temp[n_param]);
	      print_msg(msg);
	      fprintf(op_file,"\nSimplex main loop: error in fit. error() returned %g -Exiting loop early....\n", temp[n_param]);
	      done = 1;
	    }
	}
    }
  
  /****  END ITERATION
   ************************************************
   *
   *   average final results 	
   *  and send the results to outf
   */
  
  for ( i=0; i<=n_param; ++i) {
    temp[i] = 0;
    for ( j=0; j<=n_param; ++j)
      temp[i] += simp[j][i];
    temp[i] /= ( n_param+1 );
  }
  
  solution_write(n,scm_err,stdout,t_string);
  
  if( strncmp(t_string, "rs_fit", 6) == 0 )
    {
      solution_write(n,scm_err,op_file,t_string);
      if(n_param == 2)
 	{	
	  rs_param.a = temp[0];
	  rs_param.dc1 = temp[1];
	  rs_param.b1 = rs_param.a - rs_param.amb;
	  fprintf(op_file,"\nBest simplex fit (vs_row=%d): a= %g\tb= %g\tdc= %g\ta-b = %g\n", rs_param.vs_row, rs_param.a, rs_param.b1, rs_param.dc1,rs_param.amb);
	}
      else if(n_param == 4)
        {       
	  rs_param.a = temp[0];
	  rs_param.b1 = temp[1];
	  rs_param.dc1 = temp[2];
	  rs_param.dc2 = temp[3];
	  rs_param.b2 = -(rs_param.amb-rs_param.a+rs_param.b1);
	  fprintf(op_file,"\nBest simplex fit (vs_row=%d): a= %g\tb1= %g\tb2= %g\tdc1= %g\tdc2= %g\ta-b = %g\n", rs_param.vs_row, rs_param.a, rs_param.b1, rs_param.b2, rs_param.dc1, rs_param.dc2,rs_param.amb); 
        }
      else
	{
	  sprintf(msg,"simplex got confused about 1 or 2 state variable model. Numbers are probably GARBAGE!\n");
	  print_msg(msg);
	}
      
      simp_rate_state_mod();		/* to set final (averaged) values in look array */
      fclose(op_file);
    }

  switch(simp_func_action)
    {
    case SCM:
      do_scm_2();
      break;
      
    case SIMPLEX:
      sprintf(msg, "SIMPLEX: DONE\n");
      print_msg(msg);
      action = MAIN;
      top();
      break;
      
    }
    
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

init_values_write( errormax,outf)
     FILE	*outf;
     double	errormax[];
{
  int     i;
  
  if (outf == stdout)
    {
      sprintf(msg, "starting values:\n");
      print_msg(msg);
      
      for ( i=0; i<n_param; ++i) 
	{
	  sprintf(msg, "   %c    %.3e\n", (65+i), temp[i]);
	  print_msg(msg);
	}
    }
  
  else
    {
      fprintf(outf, "starting values:\n");
      for ( i=0; i<n_param; ++i) 
	{
	  fprintf(outf, "   %c    %.3e\n", (65+i), temp[i]);
	  /*
	     fprintf(outf, "   allowed error:  %.2e\n", errormax[i]);
	   */
	}
      /*
	 fprintf(outf, "              total allowed error:  %.2e\n", errormax[n_param]);
       */
    }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

get_starting_simplex(funcname, first_step,p_xch,p_ych,ndata,first)
     char funcname[];
     int	ndata, p_xch[], *p_ych;
     float	first_step[];
     int first;
{
  int	i,j;
  float	p[MAX_PARAM],q[MAX_PARAM];
  double	error();
  /*
   *  find initial vertices offset  
   */
  
  for ( j=0; j<n_param; ++j) {
    p[j] = first_step[j]*( sqrt((float)n_param+1) + n_param - 1)/( n_param*sqrt(2.) );
    q[j] = first_step[j]*( sqrt((float)n_param+1) - 1)/( n_param*sqrt(2.) );
  }
  
  /*
   *  find starting simplex	
   */
  
  for ( i=1; i<=n_param; ++i) {
    for ( j=0; j<n_param; ++j)   simp[i][j] = simp[0][j] + q[j];
    simp[i][i-1] = simp[0][i-1] + p[i-1];
    for ( j=0; j<n_param; ++j)   temp[j] = simp[i][j];
    simp[i][n_param] = error(funcname,p_xch,p_ych,ndata,first);
  }
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

solution_write(n, scm_err, outf, f_name)
     
     FILE	*outf;
     int	n;
     double	scm_err[];
     char	f_name[];
{
  static int	i,j;
  
  if (outf == stdout)
    {
      sprintf(msg, ".......stopped after %d iterations...\n", n);
      print_msg(msg);

      sprintf(msg, "The final simplex is:\n");
      print_msg(msg);

      printSimplex(outf, f_name);
      
      sprintf(msg, " the mean values are:\n");
      print_msg(msg);
      for ( i=0; i<n_param; ++i)
	{
	  sprintf(msg, "%14.5e", temp[i]);
	  print_msg(msg);
	}

      sprintf(msg, "\n");
      print_msg(msg);

      sprintf(msg, " and the estimated fractional error is:\n");
      print_msg(msg);

      for ( i=0; i<=n_param; ++i)
	{
	  sprintf(msg, "%14.5e", scm_err[i]);
	  print_msg(msg);
	}
      
      sprintf(msg, "\n");
      print_msg(msg);
    }

  else
    {
      fprintf(outf, ".......stopped after %d iterations...\n", n);
      fprintf(outf, "The final simplex is:\n");
      
      printSimplex(outf, f_name);
      
      fprintf(outf, " the mean values are:\n");
      for ( i=0; i<n_param; ++i){
	fprintf(outf, "%14.5e", temp[i]);
      }
      fprintf(outf, "\n");
      fprintf(outf, " and the estimated fractional error is:\n");
      for ( i=0; i<=n_param; ++i)
	fprintf(outf, "%14.5e", scm_err[i]);
      fprintf(outf, "\n");
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

printSimplex(outf, function_name)
     FILE	*outf;
     char 	function_name[];
{
  int	i,j;
  
  if( strncmp(function_name, "rs_fit", 6) == 0 )
    {
      if( n_param == 4)	
	fprintf(outf,"   a             b1             dc1            dc2               misfit\n");
      else
	fprintf(outf,"   a             dc             misfit\n");
      
    }
  else
    {
      for ( i=0; i<n_param; ++i)
	fprintf(outf, "  %c           ",(65+i));
      fprintf (outf, "     misfit\n");
    }
  for ( i=0; i<=n_param; ++i) {
    for ( j=0; j<=n_param; ++j)
      fprintf(outf, "%14.5e", simp[i][j]);
    fprintf(outf, "\n");
  }
  fprintf(outf, "\n");
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

order ( high, low)
     int	high[], low[];
{
  int     i, j;
  
  for ( i=0; i<=n_param; ++i) 
    {
      for ( j=0; j<=n_param; ++j) 
	{
	  if ( simp[i][j] < simp[ low[j]][j] )	low[j] = i;
	  if ( simp[i][j] > simp[ high[j]][j] )	high[j] = i;
	}
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

find_centroid(centre,h)
     int	h;
     double	centre[];
{
  int	i,j;
  
  for ( i=0; i<n_param; ++i)	
    centre[i] = 0;		/* initialise */
  for ( j=0; j<=n_param; ++j) 
    {
      if ( j != h )
	{		/* exclude worst */
	  for ( i=0; i<n_param; ++i)
	    centre[i] += simp[j][i];
	}
    } 
  for ( i=0; i<n_param; ++i) centre[i] /= n_param;
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

reflect_worst(centre,h)
     int	h;
     double	centre[];
{
  int	i;
  
  for ( i=0; i<n_param; ++i) 
    {
      temp[i] = ( ALPHA+1 )*centre[i] - ALPHA*simp[h][i];
    }
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

expand_reflection(centre,h)
     int	h;
     double	centre[];
{
  int	i;
  
  for ( i=0; i<n_param; ++i) 
    {
      temp[i] = GAMMA*simp[h][i] + ( 1-GAMMA )*centre[i];
    }
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

contract_worst(centre,h)
     int	h;
     double	centre[];
{
  int	i;
  
  for ( i=0; i<n_param; ++i)
    {
      temp[i] = BETA*simp[h][i] + ( 1-BETA )*centre[i];
    }
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

contract_all(funcname,centre,l,p_xch,p_ych,ndata,first)
     char	funcname[];
     int	l,ndata;
     int	p_xch[], *p_ych;
     double	centre[];
     int first;
{
  int	i,j;
  
  for ( i=0; i<=n_param; ++i) 
    {
      for ( j=0; j<n_param; ++j)
	{
	  simp[i][j] = (simp[i][j] + simp[l][j])*BETA;
	  temp[j] = simp[i][j];
	}
      simp[i][n_param] = error( funcname, p_xch, p_ych, ndata, first);
    }
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

saveAs_new_vertex (h)
     int	h;
{
  int    i;
  
  for ( i=0; i<=n_param; ++i)	simp[h][i] = temp[i];
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*
get_initial_values(errormax, first_step, p_iter_max)
     double errormax[];
     float first_step[];
     int *p_iter_max;
{
  static int i;
  sprintf(msg,"maximum number of iterations -> ");

  fscanf(infile,"%d",p_iter_max);
  sprintf(msg,"\ninitial guesses for free parameters\n");
  for(i=0; i<n_param; ++i)
    {
      if(infile==stdin) sprintf(msg,"%c -> ",(char)(65+i));
      fscanf(infile,"%lf",&temp[i]); 
    }
  *
   *...............................................................................
   *	BECAUSE of lack of use, error criterion for end of iteration is disabled.
   *		To enable, input of maximum total error must be added
   *		and a few print statements which have been commented out
   *		must be re-instated.
   *...............................................................................
   *
   *	if(infile==stdin) sprintf(msg,"desired precision of free parameters\n");
   *	for(i=0; i<n_param; ++i)
   *	{
   *		if(infile==stdin) sprintf(msg,"%c -> ",(char)(65+i));
   *		fscanf(infile,"%lf",&errormax[i]); 
   *	}
   *
  for(i=0; i<n_param; ++i)
    {
      errormax[i] = 0.0;
    }
  if(infile==stdin) sprintf(msg,"initial step-size for each parameter\n");
  for(i=0; i<n_param; ++i)
    {
      if(infile==stdin) sprintf(msg,"%c -> ",(char)(65+i));
      fscanf(infile,"%f",&first_step[i]); 
    }
  if(infile==stdin) sprintf(msg,"\n");
}
*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

double error(name,p_xch,p_ych,nd,first)
     char name[];
     int p_xch[], *p_ych, nd, first;
{
  static int i;
  int ob_flag;	/* FALSE means simplex parameters are *not* out of bounds */
  static double u, s;
  double this_err;
  double simp_rate_state_mod(), power1(), power2(), normal(), chisqr(), scchisqr(), genexp(), ExpLin(), gensin(), rclow(), rcph(), Poly4();
  
  
  if(strncmp(name,"rs_fit",6)==0)
    {
      ob_flag = FALSE;
      if(n_param == 2)
	{       
	  /* constrain a, dc */	
	  if(temp[0] < 0 || temp[1] < 0.01)		
	    ob_flag = TRUE;
	  rs_param.a = temp[0];
	  rs_param.dc1 = temp[1];
	  rs_param.b1 = rs_param.a - rs_param.amb;
	}
      else if(n_param == 4)
	{       
	  /* constrain a, b1, dc1, or dc2 */	
	  if(temp[0] < 0 || temp[1] < 0 || temp[2] < 0.01 || temp[3] < 0.1)	
	    ob_flag = TRUE;
	  rs_param.a = temp[0];
	  rs_param.b1 = temp[1];
	  rs_param.dc1 = temp[2]; 
	  rs_param.dc2 = temp[3];
	  rs_param.b2 = -(rs_param.amb-rs_param.a+rs_param.b1);
	}	
      else
	sprintf(msg,"simplex got confused about 1 or 2 state variable model. Numbers are probably GARBAGE!\n");
      
      if(!ob_flag)
	/* parameters defined in the global structure rs_param */
	this_err = simp_rate_state_mod();	
      else
	this_err = 1000;
      
    }
  else if(strncmp(name,"Power1",6)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = power1((double)darray[p_xch[0]][i],temp[0],temp[1]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"Power2",6)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = power2((double)darray[p_xch[0]][i],temp[0],temp[1],temp[2]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"normal",6)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = normal((double)darray[p_xch[0]][i],temp[0],temp[1]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"chisqr",6)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = chisqr((double)darray[p_xch[0]][i],temp[0]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"scchisqr",8)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = scchisqr((double)darray[p_xch[0]][i],temp[0],temp[1],temp[2]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"genexp",6)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = genexp((double)darray[p_xch[0]][i],temp[0],temp[1],temp[2],temp[3]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"Poly4",5)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = Poly4((double)darray[p_xch[0]][i],temp[0],temp[1],temp[2],temp[3],temp[4]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"gensin",6)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = gensin((double)darray[p_xch[0]][i],temp[0],temp[1],temp[2],temp[3]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"ExpLin",6)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = ExpLin((double)darray[p_xch[0]][i],temp[0],temp[1],temp[2],temp[3]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"rclow",5)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = rclow((double)darray[p_xch[0]][i],temp[0],temp[1]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else if(strncmp(name,"rcph",4)==0)
    {
      this_err = 0.0;
      for(i=first; i<first+nd; ++i)
	{
	  u = rcph((double)darray[p_xch[0]][i],temp[0],temp[1]);
	  s = (double)darray[*p_ych][i];
	  this_err += fabs(u*u-s*s);
	}
    }
  else sprintf(msg,"can't find function\n");
  return(this_err);
}
