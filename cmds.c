#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <array.h>
#include <config.h>
#include <cmds.h>
#include <cmds1.h>
#include <filtersm.h>
#include <fq.h>
#include <func.h>
#include <global.h>
#include <lookio.h>
#include <look_funcs.h>
#include <messages.h>
#include <mem.h>
#include <notices.h>
#include <nrutil.h>
#include <polyfit.h>
#include <qi_look.h>
#include <special.h>
#include <strcmd.h>


extern int action;
extern char msg[MSG_LENGTH];
extern char data_file[20], new_file[20];
extern FILE *data, *fopen(), *new;
extern int doit_des, doit_f_open;
extern char pathname[10][80], default_path[80], metapath[80];
extern int name_action;

int simp_func_action;
 
/* temporary.. until i figure out where to put these vars */
int simp_xch[MAX_COL], simp_ych;
int col1, col2, col_out;
int temp_int;
int first, last;
int intval2;
int h, i, j, k, l;
int adj_init_disp;
int simp_xch[MAX_COL], simp_ych; /* used by scm */
static int order; /* must use static, there's order() function in simplexl.c */


float val2;
float dx_increment;
float trend[10];	/*used to pass results from line fit, see special.c*/
float test1, test2;
float temp_float;
float init_h;
int temp_col;


double a_max, a_min, dc_max, dc_min, err;
double sl, dx, dmu;                  /* used by scm */
double *disp_ptr, *mu_ptr;           /* used by scm */
static double dvar[MAX_COL];  
double	converg_tol, lambda, wc;		/* used by do_qi -inversion */

char desire[256];
char t_string[300], t_string_2[300];
char opr, type;
char mu_fit_mess_1[512];	/* help message for scm, cm, rsm */
char mu_fit_mess_2[512];	/* help message for scm, cm, rsm */
char name_arg[256], unit_arg[256];
char *junkp, *strtok();


/*************************** running average slope calculation ************************/
void do_slope(arg)
     char arg[256];
{ 
  nocom(arg);

  if (sscanf(arg,"%s %d %d %d %d %d %d %s %s", desire, &col1, &col2, &col_out, &first, &last, &temp_int, name_arg, unit_arg) != 9)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(check_row(&first,&last,col1) != 0)
    {
      cre();
      action = MAIN;
      top();
      return;
    }

  if(temp_int > (last - first +1))
    {
      sprintf(msg, "Window can't be bigger than interval for smoothing.\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col ||  col_out >= MAX_COL || col2 >= max_col)
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      top();
      action = MAIN;
      return;
    }
    
  o_slope(col1,col2,col_out,first,last,temp_int); /*in special.c*/ 

  sprintf(msg, "RUNNING AVG. SLOPE: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();  
}

/*************************** trig ************************/
void do_trig(arg)
     char arg[256];
{ 
  nocom(arg);

  if (sscanf(arg, "%s %d %d %s %s %s", 
	     desire, &col1, &col_out, t_string, name_arg, unit_arg) != 6)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }
 
  if ( name(&col_out,col1, name_arg, unit_arg) == 0)
    { 
      ne();
      top();
      action = MAIN;
      return;
    }
  
  switch(t_string[0])
    {
    case 's':
      sprintf(msg,"Computing sin...\n");
      print_msg(msg);
      sinvec(&col1,&col_out) ;
      break;
    case 'c':
      sprintf(msg,"Computing cos...\n");
      print_msg(msg);
      cosvec(&col1,&col_out) ;
      break;
    case 't':
      sprintf(msg,"Computing tan...\n");
      print_msg(msg);
      tanvec(&col1,&col_out) ;
      break;
    case 'a':
      switch(t_string[1])
	{
	case 's':
	  sprintf(msg,"Computing asin...\n");
	  print_msg(msg);
	  asinvec(&col1,&col_out) ;
	  break;
	case 'c':
	  sprintf(msg,"Computing acos...\n");
	  print_msg(msg);
	  acosvec(&col1,&col_out) ;
	  break;
	case 't':
	  sprintf(msg,"Computing atan...\n");
	  print_msg(msg);
	  atanvec(&col1,&col_out) ;
	  break;
	default:
	  sprintf(msg,"Func has to be either: sin, cos, tan, asin, acos, atan   -try again\n");
	  print_msg(msg);
	  break;
	}
    }

  sprintf(msg, "TRIG: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
  
}


/*************************** rcph ************************/
void do_rcph(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], name_arg, unit_arg) != 7)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }
 
 if (name(&col_out,col1, name_arg, unit_arg) == 0)
   {
     ne();
     top();
     action = MAIN;
     return;
   }
  
  for(i=0; i<head.ch[col1].nelem; ++i)
    darray[col_out][i] = rcph((double)darray[col1][i],dvar[0],dvar[1]); 

  sprintf(msg, "RCPH: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** col_power ************************/
void do_col_power(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d %s %s",
	     desire, &col2, &col1, &col_out) != 6)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      top();
      action =MAIN;
      return;
    }
  
  powcvec(&col1,&col_out,&col2) ;
  
  sprintf(msg, "COL_POWER: DONE\n");
  print_msg(msg);
  action = MAIN;
  top();
}


/*************************** power ************************/
void do_power(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %f %d %d %s %s",
	     desire, &val2, &col1, &col_out, name_arg, unit_arg) != 6)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      top();
      action = MAIN;
      return;
    }
  
  powvec(&col1,&col_out,&val2) ;

  sprintf(msg, "POWER: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}

/*************************** recip ************************/
void do_recip(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %s %s", desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      top();
      action = MAIN;
      return;
    }
  
  recipvec(&col1,&col_out) ;

  sprintf(msg, "RECIP: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}

/*************************** log ************************/
void do_log(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %s %s", 
	     desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }


  logvec(&col1,&col_out) ;

  sprintf(msg, "LOG: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}

/*************************** explin ************************/
void do_ExpLin(arg)
     char arg[256];
{ 
  nocom(arg);
    
  if (sscanf(arg, "%s %d %d %lf %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], &dvar[2], name_arg, unit_arg) != 8)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col1].nelem; ++i)
    darray[col_out][i] = ExpLin((double)darray[col1][i],dvar[0],dvar[1],dvar[2]); 
  sprintf(msg, "EXPLIN: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** poly4 ************************/
void do_Poly4(arg)
     char arg[256];
{ 
  nocom(arg);

  if (sscanf(arg, "%s %d %d %lf %lf %lf %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], &dvar[2], &dvar[3], &dvar[4], name_arg, unit_arg) != 10)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }
 
  for(i=0; i<head.ch[col1].nelem; ++i)
    darray[col_out][i] = Poly4((double)darray[col1][i],dvar[0],dvar[1],dvar[2],dvar[3],dvar[4]);
 
  sprintf(msg, "POLY4: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** gensin ************************/
void do_gensin(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %lf %lf %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], &dvar[2], &dvar[3], name_arg, unit_arg) != 9)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col1].nelem; ++i) 
    darray[col_out][i] = gensin((double)darray[col1][i],dvar[0],dvar[1],dvar[2],dvar[3]); 
  
  sprintf(msg, "GENSIN: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** genexp ************************/
void do_genexp(arg)
     char arg[256];
{ 
  nocom(arg);

  if (sscanf(arg, "%s %d %d %lf %lf %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], &dvar[2], &dvar[3], name_arg, unit_arg) != 9)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }
 
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }
 
  for(i=0; i<head.ch[col1].nelem; ++i)
    darray[col_out][i] = genexp((double)darray[col1][i],dvar[0],dvar[1],dvar[2],dvar[3]);

  sprintf(msg, "GENEXP: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** rclow ************************/
void do_rclow(arg)
     char arg[256];
{ 
  nocom(arg);

  if (sscanf(arg, "%s %d %d %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], name_arg, unit_arg) != 7)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }
 
  for(i=0; i<head.ch[col1].nelem; ++i)
    darray[col_out][i] = rclow((double)darray[col1][i],dvar[0],dvar[1]); 

  sprintf(msg, "RCLOW: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** scchisqr ************************/
void do_scchisqr(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %lf %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], &dvar[2], name_arg, unit_arg) != 8)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col1].nelem; ++i)
    darray[col_out][i] = (float)scchisqr((double)darray[col1][i],dvar[0],dvar[1],dvar[2]); 

  sprintf(msg, "SCCHISQR: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** chisqr ************************/
void do_chisqr(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], name_arg, unit_arg) != 6)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
     }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col1].nelem; ++i) 
    darray[col_out][i] = (float)chisqr((double)darray[col1][i],dvar[0]); 
 
  sprintf(msg, "CHISQR: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** normal ************************/
void do_normal(arg)
     char arg[256];
{ 
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], name_arg, unit_arg) != 7)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col1].nelem; ++i) 
    darray[col_out][i] = normal((double)darray[col1][i],dvar[0],dvar[1]); 

  sprintf(msg, "NORMAL: DONE\n");
  print_msg(msg);
  
  action = MAIN;
  top();
}


/*************************** power2 ************************/
void do_Power2(arg)
     char arg[256];
{ 
  nocom(arg);

  if (sscanf(arg, "%s %d %d %lf %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], &dvar[2], name_arg, unit_arg) != 8)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col1].nelem; ++i) 
    darray[col_out][i] = power2((double)darray[col1][i],dvar[0],dvar[1],dvar[2]);

  sprintf(msg, "POWER2: DONE\n");
  print_msg(msg);
 
  action = MAIN;
  top();
}


/*************************** power1 ************************/
void do_Power1(arg)
     char arg[256];
{ 
  nocom(arg);

  if (sscanf(arg, "%s %d %d %lf %lf %s %s", 
	     desire, &col1, &col_out, &dvar[0], &dvar[1], name_arg, unit_arg) != 7)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col1].nelem; ++i) 
    darray[col_out][i] = power1((double)darray[col1][i],dvar[0],dvar[1]); 
 
  sprintf(msg, "POWER1: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** ln ************************/
void do_ln(arg)
     char arg[256];
{ 
  nocom(arg);
  if (sscanf(arg, "%s %d %d %s %s", 
	     desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  lnvec(&col1,&col_out) ;

  sprintf(msg, "LN: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** exp ************************/
void do_exp(arg)
     char arg[256];
{ 
  nocom(arg);
  if (sscanf(arg, "%s %d %d %s %s", 
	     desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  expvec(&col1,&col_out) ;

  sprintf(msg, "EXP: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/***************************  deriv ************************/
void do_deriv(arg)
     char arg[256];
{ 
  /*finite difference derivative using central differences,
    taylor expansion about f(x), and assuming the data is unequally spaced*/
  nocom(arg);
  if (sscanf(arg, "%s %d %d %d %d %d %s %s", 
	     desire, &col1, &col2, &col_out, &i, &j, name_arg, unit_arg) != 8)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col || col2 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if(i>j || i<0 || i>head.nrec-1 || j>head.nrec-1)
    {
      sprintf(msg, "Undefined row interval.\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  deriv(&col1,&col2,&col_out,i,j);

  sprintf(msg, "DERIV: DONE\n");
  print_msg(msg);

  action = MAIN;
  top(); 
}


/*************************** vc ************************/
void do_vc(arg)
     char arg[256];
{ 
  /* corrrect porosity/volume strain (during loading/unloading) for the effect of confining pressure on the material at the edges of the layer.  User provides a compressibility (dv/V/dP) and volume which is appropriate to the "edge
material" undergoing pseudo elastic volume change during loading/unloading -a good guess for the volume of material
affected by Pc during load/unload is an anular -elliptical area- of width = layer thickness * thickness*/
  /* use tau instead of Pc, because we're sure to have that, Pc may have been deleted */

  nocom(arg);
  if (sscanf(arg, "%s %d %d %d %d %d %f %s %s", 
	     desire, &col1, &col2, &col_out, &i, &j, &temp_float, name_arg, unit_arg) != 9)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col || col2 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if(i>j || i<0 || i>head.nrec-1 || j>head.nrec-1)
    {
      sprintf(msg, "Undefined row interval.\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  vol_corr(col1,col2,col_out,i,j,temp_float);

  sprintf(msg, "VC: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** rgt ************************/
void do_rgt(arg)
     char arg[256];
{ 
  /* correct horizontal displacement measurement during direct shear test for "geometric thinning" */
  /* correction is:   del_h = h dx/2L ; where h is thickness, dx is slip increment and L is length of the sliding block parallel to slip */ 
  
  nocom(arg);
  if (sscanf(arg, "%s %f %d %d %d %s %s", desire, &temp_float, &col1, &col2, &col_out, name_arg, unit_arg) != 7)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col || col2 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if(col1 == col_out)
    {
      sprintf(msg, "Sorry, routine is too dumb to allow input col. to equal output col. \nChoose another column for the output\n");
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col2, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  geom_thin(col1,col2,col_out,temp_float); 

  sprintf(msg, "RGT: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** cgt ************************/
void do_cgt(arg)
     char arg[256];
{
  /* calculate geometric thinning */
  /* calculation is:   del_h = h dx/2L ; where h is input (not real) thickness, dx is slip increment and L is length of the sliding block parallel to slip */ 

  nocom(arg);
  if (sscanf(arg, "%s %f %f %d %d %s %s", desire, &temp_float, &init_h, &col1, &col_out, name_arg, unit_arg) != 7)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  calc_geom_thin(col1,col_out,temp_float,init_h);

  sprintf(msg, "CGT: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** ec ************************/
void do_ec(arg)
     char arg[256];
{
  nocom(arg);
  if (sscanf(arg, "%s %d %d %d %d %d %f %s %s", desire, &col1, &col2, &col_out, &i, &j, &temp_float, name_arg, unit_arg) != 9)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col || col2 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

 if(check_row(&i,&j,col1) != 0)
   {
     cre();
     action = MAIN;
     top();
     return;
   }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  elastic_corr(col1,col2,col_out,i,j,temp_float);	       /*in array.c*/
 
  sprintf(msg, "EC: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** cs ************************/
void do_cs(arg)
     char arg[256];
{
  nocom(arg);
  if (sscanf(arg, "%s %d %d %d %d %d %s %s", desire, &col1, &col2, &col_out, &i, &j, name_arg, unit_arg) != 8)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col || col2 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

 if(check_row(&i,&j,col1) != 0)
   {
     cre();
     action = MAIN;
     top();
     return;
   }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  shear_strain(col1,col2,col_out,i,j);          /*in array.c*/

  sprintf(msg, "CS: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}



/*************************** polyfit ************************/
void do_polyfit(arg)
     char arg[256];
{
  double *px, *py, *pw, *pjm1, *pj, *error, ortval();
  
  nocom(arg);
  
	/*nargs will be 11 if fit is extended --the extra arg being a row number in position 9*/
  if(token_count(arg,256) == 10)
       sscanf(arg, "%s %d %d %d %d %d %d %s %s %s", desire, &col1, &col2, &col_out, &order, &i, &j, t_string, name_arg, unit_arg);
  else if(token_count(arg,256) == 11) 
     sscanf(arg, "%s %d %d %d %d %d %d %s %d %s %s", desire, &col1, &col2, &col_out, &order, &i, &j, t_string, &temp_int, name_arg, unit_arg);
  else 
  {
      nea();
      action = MAIN;
      top();
      return;
  }
  
  if(col1 >= max_col || col2 >= max_col || col_out >= MAX_COL)
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if(check_row(&i,&j,col1) != 0)
    {
      cre();
      action = MAIN;
      top();
      return;
    }
 
  if(strncmp(desire,"polyfit_i",9) == 0)
    {
      if(t_string[0] == 'd')
	{
	  stats(&col1,&i,&j) ;
	  dx_increment= (col_stat.max-col_stat.min)/(j-i+1);
	}
      else 
	{
	  dx_increment = atof(t_string);
	  t_string[0] = 'y';	/*so we know to extend below*/
	}
    }
  else
    { 
      if (t_string[0] == 'a' || t_string[0] == 'A')
	{
	  sprintf(msg, "Applying fit to entire col.\n");
	  print_msg(msg);
	}
    }	

  if((t_string[0] == 'y' || t_string[0] == 'Y') && (temp_int>head.nrec-1) )
    {
      sprintf(msg, "Undefined row interval.\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  l = (j-i)+1;
  px= (double *)   malloc(l*sizeof(double));
  py= (double *)   malloc(l*sizeof(double));
  pw= (double *)   malloc(l*sizeof(double));
  pjm1= (double *) malloc(l*sizeof(double));
  pj= (double *)   malloc(l*sizeof(double));
  error= (double *) malloc(l*sizeof(double));

  for(k=0;k<l;++k)
    {
      px[k] = darray[col1][i+k];
      py[k] = darray[col2][i+k];
      pw[k] = 1.0;
    }	

  ortpol(px,py,pw,l,pjm1,pj,error,order);  /*fit*/

  /* write y values for a the x's in a given row or if interpolating (polyfit_i) then write y for some given dx */

  if(strncmp(desire,"polyfit_i",9) == 0)
    {
      if(t_string[0] == 'y' || t_string[0] == 'Y')    /*extend the fit past the "fitted" interval*/
	{
	  for(k=0;k<=(temp_int-i);++k)
	    {
	      darray[col_out][i+k] = ortval(darray[col1][i]+(k*dx_increment));
	    }
	}
      else				/*using the default increment take */
	                                /*the fit only to the end of the   */
 					/*fitted interval		   */
	{
	  for(k=0;(i+k)<=j;++k)
	    {
	      darray[col_out][i+k] = ortval(darray[col1][i]+(k*dx_increment));
	    }
	  printf("the dx increment used was %f\n",dx_increment);
	}
    }
  else
    {
      if(t_string[0] == 'y' || t_string[0] == 'Y')    /*extend the fit past the "fitted" interval*/
	{
	  for(k=0;k<=(temp_int-i);++k)
	    {
	      darray[col_out][i+k] = ortval(darray[col1][i+k]);
	    }
	}
      else if(t_string[0] == 'a' || t_string[0] == 'A') /* apply fit to entire col */
	{
	  for(k=0;k<head.ch[col1].nelem;++k)
	    {
	      darray[col_out][k] = ortval(darray[col1][k]);
	    }
	}
      else
	{
	  for(k=0;(i+k)<=j;++k)
	    {
	      darray[col_out][i+k] = ortval(darray[col1][i+k]);
	    }
	}
    }	
  free(px);
  free(py);
  free(pw);
  free(pjm1);
  free(pj);
  free(error);

  sprintf(msg, "POLYFIT: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}



/*************************** sort ************************/
void do_sort(arg)
     char arg[256];
{
  float ascend;
  float *good_points;
  
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d", desire, &col1,&first,&last) != 4)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
 
  ascend=1.0;
  if(col1 <0) 
    {
      col1 *= -1;
      ascend = -1.0;
    }

  if(col1 >= max_col)
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  if(check_row(&first,&last,col1) != 0)
    {
      cre();
      action = MAIN;
      top();
      return;
    }
  
  good_points = darray[0];	/*use col 0  of look table, store ints as floats*/
  good_points[0] = first;		/* first point is "in order" by def. */
  for(i=first, j=0; i<last; )
    {
      k=1;
      while((ascend*darray[col1][i+k] <= ascend*darray[col1][i]) && (i+k<=last))
	k++;	       /*find next point that's not out-of-order*/
      i += k;	
      if(i <= last)
	good_points[++j] = (float)i;	/*skip points that are out of order*/
    }
  
  temp_int = last-first-j;			/* number of points to ax */

  for(i=last;i<head.ch[col1].nelem;i++)
    good_points[++j] = (float)i;	      /*skip points out of order*/
  
  for(h=1;h<max_col;++h)	/*loop over cols*/
    {
      for(k=first, j=0; k<head.ch[h].nelem-temp_int; k++)
	darray[h][k] = darray[h][(int)(good_points[j++])];	/*put next good row here*/
      head.ch[h].nelem -= temp_int;			/*reduce nelems*/
    }
  
  head.nrec -= temp_int;

  sprintf(msg, "SORT: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
  
}


/*************************** interpolate ************************/
void do_interpolate(arg)
     char arg[256];
{
  char name_arg1[256], unit_arg1[256];
  
  nocom(arg);

  if (sscanf(arg, "%s %d %d %d %d %d %d %s %s %s %s %s",
	     desire, &col1, &col2, &temp_int, &col_out, &first, &last, t_string, name_arg, unit_arg, name_arg1, unit_arg1) != 12) 
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col || col2 >= max_col || col_out >= MAX_COL || temp_int >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    }

   if(check_row(&first,&last,col1) != 0)
     {
       cre();
       action = MAIN;
       top();
       return;
     }       

  if(t_string[0] == 'd')
    {
      stats(&col1, &first, &last) ;
      dx_increment= (col_stat.max-col_stat.min)/(last-first+1);
      sprintf(msg, "Interpolation interval is %f\n",dx_increment);
      print_msg(msg);
    }
  else
    dx_increment = atof(t_string);

  if (name(&temp_int, col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out, col1, name_arg1, unit_arg1) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  i=first;
  k=first+1;		/*leave first point "as is"*/
  darray[col_out][first] = darray[col2][first];		/*"new_y"*/
  darray[temp_int][first] = darray[col1][first];		/*"new_x"*/
		 
  while(k<=last)
    {
      if(darray[temp_int][i]+dx_increment > darray[col1][k])	/*find relevant interval for dx*/
	++k;
      else
	{
	  darray[col_out][i+1] = darray[col2][k-1] + ( (darray[col2][k]-darray[col2][k-1]) * (darray[temp_int][i]+dx_increment-darray[col1][k-1]) / (darray[col1][k]-darray[col1][k-1]) );
	  darray[temp_int][i+1] = darray[temp_int][i]+dx_increment;
	  ++i;
	}
    }
  
  i = i-first+1;
  sprintf(msg, "\t%d points written to cols %d and %d\n",i,temp_int,col_out);
  print_msg(msg);

    
  /*allow interpolate_r or interpolater*/
  if(desire[(strlen((char *)desire)-1)] == 'r')
    {
      sprintf(msg, "Doing auto row remove from %d to %d --end of interpolated values to end of interpolation interval.\n",first+i,last);
      print_msg(msg);

      /* set up tmp doit file */
      strcpy(pathname[doit_f_open+1], default_path);
      strcat(pathname[doit_f_open+1],"temp_junk_doit_file");
      com_file[doit_f_open+1] = fopen(pathname[doit_f_open+1],"w");
      fprintf(com_file[doit_f_open+1],"begin\nr_row %d %d\nend\n", first+i, last);
      fclose(com_file[doit_f_open+1]);
      
      doit_des = open(pathname[doit_f_open+1],O_RDONLY) ; 

      /* call doit_proc(); set action to MAIN first so that it will not go back to interpolater if doit fails */
      action = MAIN;
      doit_proc("temp_junk_doit_file");
    }

  sprintf(msg, "INTERPOLATE: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** math ************************/
void do_math(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %c %f %c %d %s %s", desire, &col1, &opr, &val2, &type, &col_out, name_arg, unit_arg) != 8)
    {
      nea();
      action = MAIN;
      top();
      return;
    };

  if(col1 >= max_col || col_out >= MAX_COL)
    {
      sprintf(msg, "Not enough array space\nReallocate array for more columns\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  if((type == ':') && ((int)val2 >= max_col))
    {
      sprintf(msg, "Not enough array space\nReallocate array for more columns\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }

  i = 0;
  if(type == ':')
    j = (head.ch[col1].nelem > head.ch[(int)val2].nelem) ? head.ch[col1].nelem : head.ch[(int)val2].nelem ;
  else	
    j = head.ch[col1].nelem;
  
  math_final(name_arg, unit_arg);
}

void do_mathint(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %c %f %c %d %d %d %s %s", 
	     desire, &col1, &opr, &val2, &type, &col_out, &i, &j, name_arg, unit_arg) != 10)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(check_row(&i,&j,col1) != 0)
    {
      cre();
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col || col_out >= MAX_COL)
    {
      sprintf(msg, "Not enough array space\nReallocate array for more columns\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  if((type == ':') && ((int)val2 >= max_col))
    {
      sprintf(msg, "Not enough array space\nReallocate array for more columns\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  math_final(name_arg, unit_arg);
}

void math_final(name_arg, unit_arg)
    char *name_arg;
    char *unit_arg;
{  
  if(type == ':' && (head.ch[col1].nelem < head.ch[(int)val2].nelem) )
    {
      if (name(&col_out,(int)val2, name_arg, unit_arg) == 0)
	{
	  ne();
	  action = MAIN;
	  top();
	  return;
	}
    }
  
  else	
    if (name(&col_out,col1, name_arg, unit_arg) == 0)
      {
	ne();
	action = MAIN;
	top();
	return;
      }

  switch(opr)
    {
    case '+':
      intval2 = (int)val2 ;
      add(&col1,&intval2,&val2,&col_out,&type,i,j) ;
      break; 
    case '-':
      intval2 = (int)val2 ;
      sub(&col1,&intval2,&val2,&col_out,&type,i,j) ;
      break;
    case '*':
      intval2 = (int)val2 ;
      prod(&col1,&intval2,&val2,&col_out,&type,i,j) ;
      break;
    case '/':
      intval2 = (int)val2 ;
      mydiv(&col1,&intval2,&val2,&col_out,&type,i,j) ;
      break ;
    default :
      sprintf(msg, "Illegal operation\n") ;
      print_msg(msg);
    }

  sprintf(msg, "MATH: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** r_spike ************************/
void do_r_spike(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d", desire, &col1, &first) != 3)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if( col1 >= max_col)
    {
      coe();
      top();
      action = MAIN;
      return;
    }
  if (first == 0 || first >= head.ch[col1].nelem)
    {
      sprintf(msg, "Spike can't be > n_recs or 0!\n");
      print_msg(msg);
      top();
      action = MAIN;
      return;
    }
  darray[col1][first] = darray[col1][first-1] ;

  sprintf(msg, "R_SPIKE: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}

/*************************** positive ************************/
void do_positive(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %s %s", desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if( col1 >= max_col || col_out >= MAX_COL)
    {
      coe();
      top();
      action = MAIN;
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col_out].nelem; ++i)
    {
      if(darray[col1][i] > 0.0) darray[col_out][i] = 1.0 ;
      else darray[col_out][i] = 0.0 ;
    }

  sprintf(msg, "POSITIVE: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** compress ************************/
void do_compress(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %s %s", desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if( col1 >= max_col || col_out >= MAX_COL)
    {
      coe();
      top();
      action = MAIN;
      return;
    }
  
  if (col1 == col_out) 
    {
      sprintf(msg, "Can't use same column\n");
      print_msg(msg);
      top();
      action = MAIN;
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  j=0;
  for(i=0; i<head.ch[col_out].nelem; ++i)
    {
      if(darray[col1][i] != 0.0) 
	{
	  darray[col_out][j] = darray[col1][i] ; 
	  j+=1;
	}
    }
  head.ch[col_out].nelem = j;

  sprintf(msg, "COMPRESS: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** r_mean ************************/
/* r_mean, z_min, z_max call sub() in array.c */

void do_r_mean(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %s %s", desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
   
  if( col1 >= max_col || col_out >= MAX_COL) 
    {
      coe();
      top();
      action = MAIN;
      return;
    }
  
  i = 0 ;
  j = head.ch[col1].nelem -1;
  stats(&col1,&i,&j) ;
  type = '=' ;
  sub(&col1,&col1,&(col_stat.mean),&col_out,&type,i,j) ;

  if (name(&col_out,col_out, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  sprintf(msg, "R_MEAN: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
  
}


/*************************** z_min ************************/
void do_z_min(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %s %s", desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if( col1 >= max_col || col_out >= MAX_COL)
    {
      coe();
      top();
      action = MAIN;
      return;
    }
  i = 0 ;
  j = head.ch[col1].nelem -1;
  stats(&col1,&i,&j) ;
  type = '=' ;
  sub(&col1,&col1,&(col_stat.min),&col_out,&type,i,j) ;

  if (name(&col_out,col_out, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  sprintf(msg, "Z_MIN: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();

}


/*************************** z_max ************************/
void do_z_max(arg)
     char arg[256];
{
  nocom(arg);
    
  if (sscanf(arg, "%s %d %d %s %s", desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      top();
      action = MAIN;
      return;
    }

  if( col1 >= max_col || col_out >= MAX_COL)
    {
      coe();
      top();
      action = MAIN;
      return;
    }
  i = 0 ;
  j = head.ch[col1].nelem -1;
  stats(&col1,&i,&j) ;
  type = '=' ;
  sub(&col1,&col1,&(col_stat.max),&col_out,&type,i,j) ;
  
  if (name(&col_out,col_out, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  sprintf(msg, "Z_MAX: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();

}


/*************************** trend ************************/
void do_trend(arg)
     char arg[256];
{
  nocom(arg);

  if (sscanf(arg, "%s %d %d %d %d", desire, &col1, &col2, &i, &j) != 5)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if (col1 >= max_col || col2 >= max_col)
    {  
      coe();
      top();
      action = MAIN;
      return;
    }

  if ( i > head.nrec || i > j )
    {
      sprintf(msg, "Row %d not active.\n", i) ; 
      print_msg(msg);
      top();
      action = MAIN;
      return;
    } 

  if ( j > head.nrec )
    {
      j = head.nrec -1 ;
      sprintf(msg, "Total number of records is %d!\n",head.nrec) ;
    }
  do_final_trend();
}

void do_trend_a(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d", desire, &col1, &col2) != 3)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if (col1 >= max_col || col2 >= max_col)
    {  
      sprintf(msg, "ERROR: Column not allocated.\n") ; 
      print_msg(msg);
      top();
      action = MAIN;
      return;
    }
 
  i = 0;	
  j = (head.ch[col1].nelem < head.ch[col2].nelem) ? head.ch[col1].nelem -1 : head.ch[col2].nelem -1; 
  
  do_final_trend();
}


void do_final_trend()
{
  line(&col1,&col2,&i,&j,trend);
  
  sprintf(msg, "\ntrend: X = %s , Y = %s ; records %d to %d\n",head.ch[col1].name,head.ch[col2].name,i,j);
  print_msg(msg);
  
  sprintf(msg, "slope = %g, intercept = %g, correlation coefficient R = %g\n",trend[0],trend[1], trend[4]);
  print_msg(msg);

  sprintf(msg, "std_slope = %g, std_intercept = %g\n",trend[2],trend[3]);
  print_msg(msg);

  sprintf(msg, "x_mean = %g, x_mean_std = %g, y_mean = %g, y_mean_std = %g\n\n",trend[5], trend[7], trend[6], trend[8]);
  print_msg(msg);
  
  sprintf(msg, "TREND: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/*************************** r_trend ************************/
void do_r_trend_input(arg)
     char arg[256];
{
  nocom(arg);

  if (sscanf(arg, "%s %s %s %d %d %d %f %f %s %s", desire, t_string, t_string_2, &col1, &col2, &col_out, &trend[0], &trend[1], name_arg, unit_arg) != 10)
    {
      nea();
      top();
      action = MAIN;
      return;
    }

  if(col1 >= max_col ||  col_out >= MAX_COL || col2 >= max_col)
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  first = 0;
  
  do_r_trend_final();

}


void do_r_trend_comp(arg)
     char arg[256];
{
  nocom(arg);

  if (sscanf(arg, "%s %s %s %d %d %d %d %d %s %s", desire, t_string, t_string_2, &col1, &col2, &col_out, &h, &j, name_arg, unit_arg) != 10)
    {
      nea();
      top();
      action = MAIN;
      return;
    }

  if(col1 >= max_col ||  col_out >= MAX_COL || col2 >= max_col)
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  if(check_row(&h,&j,col1) != 0)
    {
      cre();
      action = MAIN;
      top();
      return;
    }
  
  first = h;
  line(&col1,&col2,&h,&j,trend);  
  sprintf(msg, "slope = %g , intercept = %g\n",trend[0],trend[1]);
  print_msg(msg);
  
  do_r_trend_final();
}
 

void do_r_trend_final()
{
  
  temp_col = 0 ;
  for(i=0; i<head.nrec; ++i) darray[0][i] = 0.0 ;
  /* use h and j as limits for array functions */
  h = 0 ;
  j = head.nrec - 1 ;
  type = '=' ;

  if (t_string[0] == 'X' || t_string[0] == 'x')
    {
      sprintf(msg, "Detrending x values\n");	
      print_msg(msg);
      sub(&col2,&temp_col,&(trend[1]),&temp_col,&type,h,j) ;
      mydiv(&temp_col,&temp_col,&(trend[0]),&temp_col,&type,h,j) ;
      type = ':' ;
      sub(&col1,&temp_col,&val2,&col_out,&type,h,j) ;
    }
  else 
    {
      sprintf(msg, "Detrending y values\n");	
      print_msg(msg);
      /*      sub(&col1,&temp_col,&(darray[col1][h]),&temp_col,&type,h,j) ; */
      prod(&col1,&temp_col,&(trend[0]),&temp_col,&type,h,j) ;
      add(&temp_col,&temp_col,&(trend[1]),&temp_col,&type,h,j) ;
      type = ':' ;
      sub(&col2,&temp_col,&val2,&col_out,&type,h,j) ;
    }

  i = 0 ;
  j = head.nrec - 1 ;
  stats(&col_out,&i,&j) ;
  
 if (name(&col_out,col2, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  sprintf(msg, "stats for new column # %d\n",col_out);
  print_msg(msg);
  sprintf(msg, "mean = %f , max = %f , min = %f , stddev = %f\n",col_stat.mean,col_stat.max,col_stat.min,col_stat.stddev) ; 
  print_msg(msg);

  /* re-zero temporary column */
  for(i=0; i<head.nrec; ++i) darray[0][i] = 0.0 ;
  
  /*allow r_trend_o or r_trendo */
  if(desire[(strlen((char *)desire)-1)] == 'o')
    {
      sprintf(msg, "Doing auto offset of col %d to = col %d at row %d\n",col_out,col2,first);
      print_msg(msg);
      
      strcpy(pathname[doit_f_open+1], default_path);
      strcat(pathname[doit_f_open+1],"temp_junk_doit_file");
      com_file[doit_f_open+1] = fopen(pathname[doit_f_open+1],"w");
      fprintf(com_file[doit_f_open+1],"begin\noffset\n%d,%d,%d,%d\nend\n", col_out, first, col2, first);
      fclose(com_file[doit_f_open+1]);
      
      doit_des = open(pathname[doit_f_open+1],O_RDONLY) ;
      action = MAIN;
      doit_proc("temp_junk_doit_file");
    }

  sprintf(msg, "R_TREND: DONE\n");
  print_msg(msg);
  
  action = MAIN;
  top();
      
}


/*************************** findint ************************/
void do_findint(arg)
     char arg[256];
{
  nocom(arg);

  if (sscanf(arg, "%s %d %d %d", desire, &col1, &i, &j) != 4)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
      
  if ( i > head.nrec || i > j )
    {
      sprintf(msg, "Rows not active.\n") ; 
      print_msg(msg);
      top();
      action = MAIN;
      return;
    }
      
  if ( j > head.nrec )
    {
      j = head.nrec ;
      sprintf(msg, "Total number of records is %d!\n",head.nrec) ;
      print_msg(msg);
    }
  
  test1 = 1 ;
  for( k = i; k < j; ++k)
    {
      test2 = test1 ;
      test1 = darray[col1][k] - darray[col1][k+1] ;
      if((test2*test1) < 0.0)
	{
	  sprintf(msg, "findint: record %d\n",k) ;
	  print_msg(msg);
	}
    }

  sprintf(msg, "FINDINT: DONE\n");
  print_msg(msg);

  top();
  action = MAIN;
}


/*************************** summation ************************/
/* calls summation() in array.c */
void do_summation(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %s %s" , desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if (col1 >= max_col || col_out >= MAX_COL)
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  summation(&col1, &col_out);

  sprintf(msg, "SUMMATION: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/************************** curv **************************/
void do_curv(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %f %s %s", desire, &col1, &col_out, &temp_float, name_arg, unit_arg) != 6)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if (col1 >= max_col || col_out >= MAX_COL)
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=2; i<head.ch[col1].nelem-2; ++i)
    {
      darray[col_out][i] = -30.0*darray[col1][i] + 16.0*(darray[col1][i+1]+darray[col1][i-1]) - (darray[col1][i+2]+darray[col1][i-2]) ;
      darray[col_out][i] /= 12.0*pow(temp_float,2.0);
    }

  sprintf(msg, "CURV: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();  
}


/************************** peak ****************************/
void do_peak(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %s %s", desire, &col1, &col_out, name_arg, unit_arg) != 5)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if (col1 >= max_col || col_out >= MAX_COL)
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=0; i<head.ch[col_out].nelem; ++i) 
    darray[col_out][i] = 0.0 ;
  
  j = 0;
  for(i=2; i<head.ch[col1].nelem-2; ++i)
    {
      if( darray[col1][i] > darray[col1][i+1] && darray[col1][i] > darray[col1][i-1] )
	{
	  if( darray[col1][i] > darray[col1][i+2] && darray[col1][i] > darray[col1][i-2] )
	    {
	      darray[col_out][i] = darray[col1][i] ;
	      j += 1;
	    }
	}
    }
  sprintf(msg, "PEAK: %d peaks found\n",j);
  print_msg(msg);

  sprintf(msg, "PEAK: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/**************************** decimat **************************/
void do_decimat(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d %d %d %s %s", desire, &col1, &col_out, &k, &first, &last, name_arg, unit_arg) != 8)
    {
      nea();
      top();
      action = MAIN;
      return;
    }

  if (col1 >= max_col || col_out >= MAX_COL)
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if(check_row(&first,&last,col1) != 0)
    {
      cre();
      action = MAIN;
      top();
      return;
    }
    
  if (name(&col_out,col1, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  for(i=first, j=first; i <= last; i+=k, j++)
    {
      darray[col_out][j] = darray[col1][i] ;
    }
  
  first=j;		/* first row to remove */
  if(desire[(strlen((char *)desire)-1)] == 'r')
  {
      head.ch[col_out].nelem = first;
  }

  sprintf(msg, "DECIMAT: DONE, last row written to col %d was %d\n",col_out,j-1);
  print_msg(msg);

  action = MAIN;
  top();
}


/**************************** pdf *****************************/
void do_pdf(arg)
     char arg[256];
{
  int max, min;
  
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d %d %d %d", desire, &col1, &col_out, &col2, &k, &max, &min) != 7)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  if (col1 >= max_col || col_out >= MAX_COL || col2 >= max_col)
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  do_pdf1();
  i = 0;
  j = head.nrec-1;
  col_stat.max = max;
  col_stat.min = min;
  do_pdf2();
}


void do_pdfauto(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d %d", desire, &col1, &col_out, &col2, &k) != 5)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if (col1 >= max_col || col_out >= MAX_COL || col2 >= max_col)
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  do_pdf1();
  i = 0;
  j = head.nrec-1;
  stats(&col1, &i, &j);
  do_pdf2();
}

void do_pdf1()
{
  if(check_col(col_out) == 0) head.nchan += 1;
  if(check_col(col2) == 0) head.nchan += 1;
  strcpy(&(head.ch[col_out].name[0]),"prob_dens");
  strcpy(&(head.ch[col2].name[0]),"bin");
  strcpy(&(head.ch[col_out].units[0]),"prob_dens");
  strcpy(&(head.ch[col2].units[0]),&(head.ch[col1].units[0]));
  head.ch[col_out].gain = 1.0 ;
  head.ch[col2].gain = 1.0 ;
  if(head.nrec<k) head.nrec=k ;
  head.ch[col_out].nelem = k ;
  head.ch[col2].nelem = k ;
}

void do_pdf2()
{
  for(j=0; j<k; ++j)	/*initialize and set up dependent axis*/
    {
      darray[col2][j] = col_stat.min + ((float)j+0.5)/(float)(k-1)*(col_stat.max - col_stat.min);
      darray[col_out][j] = 0.0;
    }
  for(i=0; i<head.ch[col1].nelem; ++i)	/*bin data*/
    {
      temp_float = (float)(k-1)*(darray[col1][i]-col_stat.min)/(col_stat.max-col_stat.min) ;
      darray[col_out][(int)temp_float] += 1.0;
    }
  for(j=0; j<k; ++j)	/*scale for probability density*/
    {
      darray[col_out][j] /= head.ch[col1].nelem*((col_stat.max-col_stat.min)/((float)k-1.0));
    }

  sprintf(msg, "PDF: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
  
}


/**************************** r_row **************************/
void do_r_row_col(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d", desire, &col1, &first, &last) != 4)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  do_r_row_final(col1+1, col1, first, last);
  action = MAIN;
  top();
}

void do_r_row(arg)
     char arg[256];
{
  nocom(arg);

  if (sscanf(arg, "%s %d %d", desire, &first, &last) != 3)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  do_r_row_final(max_col, 1, first, last);
  action = MAIN;
  top();
}

void do_r_row_final(l, col1, first, last)
     int l, col1, first, last;
{
  j = first;
  k = last;

  if ( k >= head.nrec )
    {
      k = head.nrec - 1 ;
      sprintf(msg, "Total number of records is %d!\n",head.nrec) ;
      print_msg(msg);
      sprintf(msg, "Rows %d through %d removed.\n",j,head.nrec-1) ;
      print_msg(msg);
    }
    
  if(check_row(&j,&k,1) != 0)  /* use col 1 */
    {
      cre();
      action = MAIN;
      top();
      return;
    }
  
  temp_int = (k - j + 1) ;
  for( h = col1; h < l; ++h)	/*changed so that rows could be removed*/
    {				/*from just one col within the table*/
      j = first ;
      for( i = k; i < head.nrec; ++i)
	{
	  if(head.ch[h].nelem > 0)
	    {
	      darray[h][j] = darray[h][j+temp_int] ;
	    }
	  j += 1 ;
	}
          if(head.ch[h].nelem >= k + 1)
	    {
	      head.ch[h].nelem -= temp_int ;
	    }
	  else
	    {
	      if(head.ch[h].nelem > first + 1) head.ch[h].nelem = first ;
	  }
    }
  
  head.nrec = head.ch[1].nelem;
  for( i = 2; i < max_col; ++i) 
    head.nrec =  (head.nrec <  head.ch[i].nelem)  ? head.ch[i].nelem : head.nrec ;

  sprintf(msg, "R_ROW: DONE\n");
  print_msg(msg);

  action =MAIN;
  top();
  
}


/***************************** comment ***********************/
void do_comment(arg)
     char arg[256];
{
  char *com;
  
  nocom(arg);
  
  if (sscanf(arg, "%s %d %s", desire, &col1, msg) != 3)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
      
  if (col1 > max_col)
    {
      coe();
      action = MAIN;
      top();
      return;
    }
  
  com = (char *)strip(arg, 2);
  
  if (check_col(col1) != 0)
    {
      sprintf(msg, "Old comment reads: %s\n", &(head.ch[col1].comment[0]));  
      print_msg(msg);
      strncpy(&(head.ch[col1].comment[0]), com, 50);
      head.ch[col1].comment[strlen(&(head.ch[col1].comment[0]))] = '\0';
      sprintf(msg, "Current comment reads: %s\n", &(head.ch[col1].comment[0]));
      print_msg(msg);
      sprintf(msg, "COMMENT: DONE\n");
      print_msg(msg);
      
    }
  else
    {
      sprintf(msg, "Sorry, you cannot comment on unnamed columns.\n");
      print_msg(msg);
    }

  action = MAIN;
  top();
}  

/**************************** name ****************************/
/* calls name() in look_funcs.c */

void do_name(arg)
     char arg[256];    
{
  
  nocom(arg);
  
  if (sscanf(arg, "%s %d %s %s", desire, &col_out, name_arg, unit_arg) != 4)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
      
  if (name(&col_out, col_out, name_arg, unit_arg) == 0)
    {
      ne();
      action = MAIN;
      top();
      return;
    }

  sprintf(msg, "NAME: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/***************************** r_col ***************************/
/* calls null_col() in  look_funcs.c */
void do_r_col(col)
     int col;
{
  if (col > max_col)
    {
      coe();
    }
  else
    {
      null_col(col);
      head.nchan -= 1;
      sprintf(msg, "R_COL: DONE\n");
      print_msg(msg);
    }
  action = MAIN;
  top();
}


/****************************** zero **************************/
/* calls zero() in  array.c  */
void do_zero_all(arg)
     char arg[256];
{
  int rec;
  
  if (sscanf(arg, "%s %d", desire, &rec) != 2)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if (rec >= head.nrec)
    {
      sprintf(msg,"No way! nrec = %d\n",head.nrec);
      print_msg(msg);
    }
  else
    for(i=1; i <= max_col-1; ++i)
      {
	zero(&i,&rec) ;
      }

  sprintf(msg, "ZERO_ALL: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}

void do_zero(arg)
     char arg[256];
{
  int rec;
  
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d", desire, &col1, &rec) != 3)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if (col1 >= max_col || rec >= head.nrec) 
    {
      sprintf(msg, "Error. col,row not in array space.\n");
      print_msg(msg);
    }
  else
    zero(&col1,&rec) ;
  
  sprintf(msg, "ZERO: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/******************************** offset_int **********************/
/* calls offset_int() in array.c */

void do_offset_int(arg)
     char arg[256];
{
  int rec1, rec2;
  
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d %s", desire, &col1, &rec1, &rec2, t_string) != 5)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if (rec1 >= head.ch[col1].nelem || rec2 >= head.ch[col1].nelem )
    {
      sprintf(msg, "That column only has %d recs.\n",head.ch[col1].nelem);
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  offset_int(&rec1, &col1, &rec2, t_string); 

  sprintf(msg, "OFFSET_INT: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/******************************** offset **************************/
/* calls offset() in array.c */

void do_offset(arg)
     char arg[256];
{
  int rec1, rec2;
  
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d %d", desire, &col1, &rec1, &col2, &rec2) != 5)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
 
  if (rec1 >= head.ch[col1].nelem || rec2 >= head.ch[col2].nelem )
    {
      sprintf(msg, "ERROR: Check the # of elements in col1 and col2.\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  offset(&rec1,&col1,&rec2,&col2); 

  sprintf(msg, "OFFSET: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/******************************** simplex ***************************/
/* calls simp_func() in simplexl.c */

void simplex_info()
{
  sprintf(msg,"A listing of functions can be found in /lamont/lamont/boitnott/LCO/Simp_functions\n");
    print_msg(msg);
}

void do_simplex(arg)
     char arg[256];
{
  int xcol, ycol;
  
  nocom(arg);
  
  if (sscanf(arg, "%s %s %d %d %d %d", desire, t_string, &first, &last, &xcol, &ycol) != 6)
    {
      nea();
      action = MAIN;
      top();
      return;
    }     
  
  if(strcmp(t_string,"Power1") == 0)
    {
      sprintf(msg,"\n\tfitting   Y = pow(X[0],A) + B\n");
      print_msg(msg);
      l = 2 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"Power2") == 0)
    {
      sprintf(msg,"\n\tfitting   Y = A * pow(X[0],B) + C\n");
      print_msg(msg);
      l = 3 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"normal") == 0)
    {
      sprintf(msg,"\n\tfitting normal distribution,  A=mean, B=stddev\n");
      print_msg(msg);
      l = 2 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"chisqr") == 0)
    {
      sprintf(msg,"\n\tfitting chi-squared distribution,  A= #degree of freedom\n");
      print_msg(msg);
      l = 1 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"scchisqr") == 0)
    {
      sprintf(msg,"\n\tfitting scaled chi-squared distribution, A=stddev, B=ndf, C=offset\n");
      print_msg(msg);
      l = 3 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"genexp") == 0)
    {
      sprintf(msg,"\n\tfitting    y = A * exp(Bx+C) + D  \n");
      print_msg(msg);
      l = 4 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"Poly4") == 0)
    {
      sprintf(msg,"\n\tfitting    y = A + B*x + C*pow(x,2) + D*pow(x,3) + E*pow(x,4)\n");
      print_msg(msg);
      l = 5 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"gensin") == 0)
    {
      sprintf(msg,"\n\tfitting    y = A * sin(Bx+C) + D \n ");
      print_msg(msg);
      l = 4 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"ExpLin") == 0)
    {
      sprintf(msg,"\n\tfitting    y = A * (1.0 - exp(B*x)) + C*x  \n ");
      print_msg(msg);
      l = 3 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"rclow") == 0)
    {
      sprintf(msg,"\n\tfitting y = [1/(x*B)]*sqrt[1+x*x*B*B*A*A] \n");
      print_msg(msg);
      l = 2 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else if(strcmp(t_string,"rcph") == 0)
    {
      sprintf(msg,"\n\tfitting y = -1.0*atan((1.0/(x*A*B))) \n");
      print_msg(msg);
      l = 2 ;    /* number of free parameters to optimize */
      h = 1 ;    /* number of independent variables */
    }
  else 
    {
      sprintf(msg,"Illegal function: %s \n", t_string);
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }

/*  for(k=0; k<h; ++k)
    {
      if(command==stdin && h>1) fprintf(stderr,"enter column for X[%d] -> ",k);
      if(command==stdin && h==1) fprintf(stderr,"enter column for X -> ");
      fscanf(command,"%d",&simp_xch[k]);
  
    }*/
/* h is always 1, therefore, no need the loop.. which is complicated in X */
  simp_xch[0] = xcol;
  
  if(check_col(simp_xch[0]) == 0 || head.ch[simp_xch[k]].nelem < last+1) 
    {
      sprintf(msg,"X-col is not in array space: %d\n", xcol);
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  simp_ych = ycol;
  if(check_col(simp_ych) == 0 || head.ch[simp_ych].nelem < last+1) 
    {
      sprintf(msg,"Y-col is not in array space: %d\n", ycol);
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }


/*

   simp_func(command,l,simp_xch,&simp_ych,t_string,first,last, temp_int);

  sprintf(msg, "SIMPLEX: DONE\n");
  print_msg(msg);

  action = MAIN;
  top(); 
  */


  /* set simp_func_action so that it knows from where it's called from
     set action to simp func
     */

  simp_func_action = SIMPLEX;
  action = SIMP_FUNC;
  return;
  
}


/****************************** stdasc *************************/
/* calls stdasc() in filtersm.c */

void do_stdasc(arg)
     char arg[256];
{
  char fi[20], cr[20];
  int i;
    
  nocom(arg);
  
  if (sscanf(arg, "%s %s %s %s", desire, data_file, cr, fi) != 4)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if ((data = fopen(data_file, "r")) == NULL)
    {
      sprintf(msg, "Can't open data file %s", data_file);
      print_msg(msg);
    }
  else
    {
      if (stdasc(data, cr, fi) != 1)
	{
	  sprintf(msg, "Input error! stdasc command aborted.");
	  print_msg(msg);
	}
      fclose(data);
      for (i = (head.nchan+1); i<MAX_COL; ++i)
	null_col(i);
    }

  sprintf(msg, "STDASC: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/********************************** tasc *******************************/
/* calls tasc() in filtersm.c */

void do_tasc(arg)
     char arg[256];
{
  char fi[20];
  int i, f_flag;
    
  nocom(arg);
  
  if (sscanf(arg, "%s %s %s", desire, data_file, fi) != 3)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if ((data = fopen(data_file, "r")) == NULL)
    {
      sprintf(msg, "Can't open data file %s.\n", data_file);
      print_msg(msg);
    }
  else
    {
      if (fi[0] == 'f') f_flag = 1;
      else f_flag = 0;
      
      if (tasc(data, f_flag) != 1)
	{
	  sprintf(msg, "Input error! tasc command aborted.\n");
	  print_msg(msg);
	}
      fclose(data);
      sprintf(msg, "Warning: assumed active columns are between 1 and head.nchan [%d]\n", head.nchan);
      print_msg(msg);
      
      for (i = (head.nchan+1); i<MAX_COL; ++i)
	null_col(i);
    }

  sprintf(msg, "TASC: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/********************************* head ********************************/
/* calls aschead_scrn() or aschead_file() in lookio.c */

void do_head(arg)
     char arg[256];
{
  char new_file[256];   
  
  strcpy(new_file, arg);
  
  if ((strncmp(new_file, "S", 1) == 0 || strncmp(new_file, "s", 1) == 0)
      && (strlen(new_file) == 1))
    {
      aschead_scrn(&head);
      action =MAIN;
      top();
      return;  
    }
  
  else
    {
      if ((new = fopen(new_file, "a")) == NULL)
	{
	  sprintf(msg, "Can't open file: %s.\n", new_file);
	  print_msg(msg);
	  action =MAIN;
	  top();
	  return;  
	}
      else
	{
	  if ((int)ftell(new) > 0)
	    {
	      if (write_show_warning() != 1)
		{
		  sprintf(msg, "Head command aborted!\n");
		  print_msg(msg);
		  action =MAIN;
		  top();
		  return;
		}
	      else
		{
		  if ((new = fopen(new_file, "w")) == NULL)
		    {
		      sprintf(msg, "Can't open file: %s.\n", new_file);
		      print_msg(msg);
		      action =MAIN;
		      top();
		      return;
		    }
		  else
		    {
		      sprintf(msg, "Overwriting %s.\n", new_file);
		      print_msg(msg); 
		    }
		}
	    }
	  aschead_file(&head, new);
	  fclose(new);

	  sprintf(msg, "HEAD: DONE, written to file %s\n",new_file);
	  print_msg(msg);

	  action = MAIN;
	  top();
	  return;
	}
    }
}

/***************************** getaschead ******************************/
/* calls getaschead() in lookio.c */

void do_getaschead(arg)
     char arg[256];
{
  if ((data = fopen(arg, "r")) == NULL)
      sprintf(msg, "Can't open header file: %s.\n", arg);
  else
  {
      if (getaschead(&head, data) != 1)
	  sprintf(msg, "Input error. getaschead command aborted.\n");
      else
      	  sprintf(msg, "GETASCHEAD: DONE\n");
      fclose(data);
  }
  print_msg(msg);
  action = MAIN;
  top();
}


/*********************************** examin ****************************/
/* calls examin() in filtersm.c */

void do_examin(arg)
     char arg[256];
{
  if ((data = fopen(arg, "r")) == NULL)
    {
      sprintf(msg, "Can't open data file: %s.\n", arg);
      print_msg(msg);
    }
  else
    {
      examin(data);
      fclose(data);
    }

  sprintf(msg, "EXAMIN: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}


/********************************* rsm *********************************/

void print_rsm_help_info()
{
/*   if(desire[(strlen((char *)desire)-1)] == 'h') */

  sprintf(msg, "> This function computes the Dieterich-Ruina rate/state variable friction model at specified displacements.\n");
  print_msg(msg);      
  
  sprintf(msg, "> Friction is calculated at displacement values given by measurements in a displacement column\n");
  print_msg(msg);
  
  sprintf(msg, "> See cm_h or cm2_h for a similar calc -but which generates their own displacement values.\n");
  print_msg(msg);
  
  sprintf(msg, "> Input is: the column that contains displacement, the row # of the velocity step,\n");
  print_msg(msg);
  
  sprintf(msg, "> the row # to which the calculation should be extended, and the various constitutive parameters.\n");
  print_msg(msg);
  
  sprintf(msg, "> k should be given in the same units as normal stress and dc.\n");
  print_msg(msg);
  
  sprintf(msg, "> The pre-step mu value is extended back 100 rows in the model output.\n\n");
  print_msg(msg); 
}


void do_rsm(arg)
     char arg[256];
{
      outdated_cmd();
      sprintf(msg, "Use qi or launch the r/s fric tool and do a forward model.\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
}


/****************************** cm ********************************/
void print_cm_help_info()
{
  sprintf(msg, "> This function computes the Dieterich-Ruina rate/state variable friction model.\n");
  print_msg(msg);
  
  sprintf(msg, "> See rsm_h for a similar calc -but which uses the measured displacement.\n");
  print_msg(msg);

  sprintf(msg, "> It needs: the column that contains displacement, the row # of the velocity step,\n");
  print_msg(msg);
  
  sprintf(msg, "> the row # to which the calculation should be extended, and the various constitutive parameters.\n");
  print_msg(msg);
  
  sprintf(msg, "> k should be given in the same units as normal stress and dc.\n");
  print_msg(msg);

  sprintf(msg, "> The model data occupies 1000 rows more than the original data (if possible) -although 50%% of the data is written within the first 20%% of the total displacement.\n");
  print_msg(msg);
  
  sprintf(msg, "> The pre-step mu value is extended back 100 rows in the model output.\n\n");
  print_msg(msg);
  
  sprintf(msg, mu_fit_mess_1); 
  print_msg(msg);
}


void do_cm(arg)
     char arg[256];
{
  
      outdated_cmd();
      sprintf(msg, "Use qi or launch the r/s fric tool and do a forward model.\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
  
}   


/***************************** mem ****************************/

void do_mem(arg)
     char arg[256];
{ 
  int n_freq, n_poles, type, junk;
  float *mem_dat, *poles, pnp, fdt, facm, facp, ww, tempfloat;
  

  nocom(arg);

  if (sscanf(arg, "%s %d , %d , %d , %d , %d , %d, %d , %d , %c, %c, %c", desire, &col1,&col2,&col_out,&temp_col,&first,&last,&n_freq,&n_poles,&type,&junk) != 11 )
    {
      nea();
      action = MAIN;
      top();
      return;
    }
  
  if(col1 >= max_col ||  col_out >= MAX_COL || col2 >= max_col || temp_col >= MAX_COL)
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  /*nameing BS here so we don't call name() */
  if(strncmp(head.ch[col_out].name,"no_val",6) == 0)
    head.nchan += 1 ;
  if(strncmp(head.ch[temp_col].name,"no_val",6) == 0)
    head.nchan += 1 ;
  head.ch[col_out].nelem = head.ch[temp_col].nelem = head.ch[col1].nelem ; 
  
  if(type == 'l' || type == 'n')  ;
  else
    {
      sprintf(msg, "defaulting to linear frequency distribution\n");
      print_msg(msg);
      type = 'n';
    }

  strcpy(t_string, "1/");
  strcat(t_string, head.ch[col1].units);	
  t_string[13] = '\0';
  
  /*    name(&col_out,col1);   name(&temp_col,col2); */

  strcpy(head.ch[col_out].name, "frequency"); 
  strcpy(head.ch[col_out].units, t_string);
  
  strncpy(t_string, head.ch[col2].units, 10); /*Only 10 in case it's long*/
  t_string[10] = '\0';  /*make sure it's null terminated*/
  strcat(t_string, "**2" );	
  t_string[13] = '\0';
  strcpy(head.ch[temp_col].name, "power"); 
  strcpy(head.ch[temp_col].units, t_string);

  test1 = darray[col1][first+1] - darray[col1][first]; 
  test2 = darray[col1][first+2] - darray[col1][first+1]; 
  if( test1 - test2 > SMALL)
    {            
      sprintf(msg, "Warning: it doesn't look like the data are equally spaced, check col %d\n",col1);
      print_msg(msg);
    }
  
  sprintf(msg, "Nyquist freq. is %f 1/%s\t",1/(2*test1),head.ch[col1].units);
  print_msg(msg);
  /*	sprintf(msg, "increment = %f first=%d last=%d\n",test1,first,last); */
  
  temp_int = last-first+1;			/*n_data = last-first+1 */
  
  mem_dat = (float *)vector(1, temp_int);   	  /* allocate array space */
  poles = (float *)vector(1,n_poles);	
  
  if(junk=='w')			/*apply welsh taper*/
    {
      sprintf(msg, "\t applying Welch taper to data\n");
      print_msg(msg);
      facm = temp_int-0.5;	facp = 1.0/(temp_int + 0.5);  
      for(j=first, k=1, i=0; j <=last; ++j, ++k, ++i)  	/*get data, apply welsh window*/
	{
	  ww = WELCH_WINDOW(i,facm,facp);
	  mem_dat[k] = ww*darray[col2][j]; 
	}
    }
  else
    {
      sprintf(msg, "\t no taper applied to data\n");
      print_msg(msg);
      for(j=first, k=1, i=0; j <=last; ++j, ++k, ++i)  	/*get data, apply welsh window*/
	mem_dat[k] = darray[col2][j]; 
    }
  sprintf(msg, "\t calculating power spectrum\n");
  print_msg(msg);
  memcof(mem_dat,temp_int,n_poles,&pnp,poles);	/*get coefficients*/
  

  first--;	/*so the points are in the right place, with j starting at 1*/
  /*select frequency distribution*/
  if(type == 'n')			/*linear frequency dist*/
    {
      for(j=1;j<=n_freq;++j) 
	{	
	  /*start at lowest freq, go to nyquist=1/2dt*/
	  /* dt=sampling interval */
	  fdt = (float)j / (float)(n_freq*2.000);
	  darray[col_out][first+j] = fdt/test1;	/*test1 = samp. int.*/
	  darray[temp_col][first+j]=evlmem(fdt,poles,n_poles,pnp);
	}
    }
  else 				/*log freq distribution*/
    {
      tempfloat = log10(0.5);		/*evaluate power over 4 decades*/
      for(j=1;j<=n_freq;++j) {
	test2= tempfloat - 4.0 + 4.0*(float)j/(float)n_freq;
	fdt= (float)pow((double)10.0,(double)test2);
	darray[col_out][first+j] = fdt/test1;
	darray[temp_col][first+j]=evlmem(fdt,poles,n_poles,pnp);
      }		
    }
  
  /* free array space */
  free_vector(mem_dat,1,temp_int );
  free_vector(poles,1,n_poles);

  sprintf(msg, "MEM: DONE\n");
  print_msg(msg);
  
  action = MAIN;
  top();
  
}



/***********************************  median smooth *************************/

void do_median_smooth(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg,"%s %d %d %d %d %d %s %s", desire, &col1, &col_out, &first, &last, &temp_int, name_arg, unit_arg) != 8)
    {
      nea();
      action = MAIN;
      top();
      return;
    }
     
  if(check_row(&first,&last,col1) != 0)
    {
      cre();
      action = MAIN;
      top();
      return;
    }
  
  if(temp_int > (last - first +1))
    {
      sprintf(msg, "Window can't be bigger than interval for smoothing.\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }  

  if(col1 >= max_col ||  col_out >= MAX_COL)
    {
      coe();
      action = MAIN;
      top();
      return;
    }

  if (name(&col_out, col1, name_arg, unit_arg) == 0)
    {
      ne();
      top();
      action = MAIN;
      return;
    }

  median_smooth(col1,col_out,first,last,temp_int); /*in special.c*/ 
 
  sprintf(msg, "MEDIAN_SMOOTH: DONE\n");
  print_msg(msg);
  
  top();
  action = MAIN;
}



/****************************** smooth *******************************/

void do_smooth(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg,"%s %d %d %d %d %d %s %s", desire, &col1, &col_out, &first, &last, &temp_int, name_arg, unit_arg) != 8 )
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  if(check_row(&first,&last,col1) != 0)
    {
      cre();
      action = MAIN;
      top();
      return;
    }

  if(temp_int > (last - first +1))
    {
      sprintf(msg, "Window can't be bigger than interval for smoothing\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }

  if(col1 >= max_col ||  col_out >= MAX_COL)
    { 
      coe();
      action = MAIN;
      top();
      return;
    } 

  if (name(&col_out, col1, name_arg, unit_arg) == 0)
    {
      ne();
      top();
      action = MAIN;
      return;
    }

  smooth(col1,col_out,first,last,temp_int); /*in special.c*/ 

  sprintf(msg, "SMOOTH: DONE\n");
  print_msg(msg);
  
  top();
  action = MAIN;
}


/***************************** typeall ***********************************/
/*  calls vision() and p_vision() in look_funcs.c  */
/*      msg_vision() and msg_p_vision() are equivalent funcs of the above which prints to the msg_window instead of to a file.  (also in look_funcs.c)  */


void do_typeall(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %s", desire, new_file) != 2)
    {
      nea();
      top();
      action = MAIN;
      return;
    }

  i = 0 ;
  j = head.nrec - 1 ;
  h = 1 ;
  k = max_col - 1 ;

  do_type_final();
 
  sprintf(msg, "TYPEALL: DONE\n");
  print_msg(msg);
  action = MAIN;
  top();
}


void do_type(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%s %d %d %d %d %s", desire, &i, &j, &h, &k, new_file) != 6)
    {
      nea();
      top();
      action = MAIN;
      return;
    }

  if ( j >= head.nrec )
    {
      j = head.nrec - 1;
      sprintf(msg, "Total number of records is %d!\n", head.nrec);
      print_msg(msg);
    }

  if ( h < 1 || h > k ) 
    {
      sprintf(msg, "First col is < 1 or > last.\n");
      print_msg(msg);
      top();
      action = MAIN;
      return;
    }

  check_row(&i, &j, h);

  if ( k > (max_col-1) )
    {
      sprintf(msg, "Total number of columns is %d!\t Setting col %d to %d\n", max_col-1, k, max_col-1);
      print_msg(msg);
      k = max_col - 1;
    }

  do_type_final();
  
  sprintf(msg, "TYPE: DONE\n");
  print_msg(msg);
  action = MAIN;
  top();
}

void do_type_final()
{
  /*printf("%s\n", desire);*/
  
  if ((strncmp(new_file,"S",1) == 0 || strncmp(new_file,"s",1) == 0) && 
      (strlen(new_file) == 1) )
    {
      if (strncmp(desire,"type_p", 6) == 0)
	msg_p_vision(&i,&j,&h,&k) ;
      else
	msg_vision(&i,&j,&h,&k) ;
    }
  else
    {
      if ((new = fopen(new_file, "a")) == NULL )
        {
	  sprintf(msg, "Can't open file %s.\n", new_file);
	  print_msg(msg);
	  top();
	  action = MAIN;
	  return;
        }
      else
        {
	  if ((int)ftell(new) > 0)
	    {  
	      if (write_show_warning() != 1)
		{
		  sprintf(msg, "Write aborted!\n");
		  print_msg(msg);
		  action =MAIN;
		  top();
		  return;
		}
	      else
                {
		  if ((new = fopen(new_file, "w")) == NULL)
		    {
		      sprintf(msg, "Can't open file: %s.\n", new_file);
		      print_msg(msg);
		      action =MAIN;
		      top();
		      return;
		    }
		  else
		   {
		     sprintf(msg, "Overwriting %s.\n", new_file);
		     print_msg(msg); 
		   }
                }
	    }
	}
      if (strncmp(desire,"type_p",6) == 0)
	p_vision(new,&i,&j,&h,&k) ;
      else
	vision(new,&i,&j,&h,&k) ;
      fclose(new) ; 
    }
}



/****************************** stat *******************************/

void do_stats(arg)
     char arg[256];
{
  double hmean, gmean, rms;
  
  nocom(arg);
 				/*check token count again here, but this has 
	already been done in event.c, by command handler, before do_stats is called*/ 
  i=j=col1=0; /* clear any garb left over from elsewhere */
  if (sscanf(arg,"%s %d %d %d", desire, &col1, &i, &j) == 4 
		|| (strncmp(desire,"stat_a",6) == 0))  /* OK, do stats command*/
  {
    if(strncmp(desire,"stat_a",6) == 0)  /*put in default rows*/
    {
      i=0;
      j=head.ch[col1].nelem -1;
    }
    if(col1 >= max_col)		/*make sure col is OK*/
    {
      coe();
      action = MAIN;
      top();
      return;
    }
    if(check_row(&i,&j,col1) != 0)	/*make sure rows check out*/
    {
      cre();
      action = MAIN;
      top();
      return;
    }
      stats(&col1,&i,&j) ; 
  }
  else	 	/*problem, not enough arguments*/
  {
    nea();	/*not enough arguments, nea() func is in messages.c*/
    action = MAIN;
    top();
    return;
  }
      
  sprintf(msg, "%s: recs = %d min = %e , max = %e , mean = %e , stddev = %e\n",head.ch[col1].name,col_stat.rec,col_stat.min,col_stat.max,col_stat.mean,col_stat.stddev);
  print_msg(msg); 
  
  h = 0;
  hmean = 0.0;
  gmean = 0.0;
  rms = 0.0;
  for(l=i; l<=j; ++l)
    {
      rms += (double)darray[col1][l]*(double)darray[col1][l];
      
      if(darray[col1][l] > 0.0)
	{
	  hmean += (double)1.0 / (double)darray[col1][l] ;
	  gmean += log((double)darray[col1][l]);
	}
      else 
	h++;
    }
	
  rms /= (double)(j-i+1);
  rms = sqrt((double)rms);
  hmean /= (double)(j-i+1) ;
  hmean = 1.0 / hmean ;
  gmean /= (double)(j-i+1) ;
  gmean = exp((double)gmean);

  sprintf(msg, "rms = %E , gmean = %E , hmean = %E\n",rms,gmean,hmean);
  print_msg(msg); 
  sprintf(msg, "%d zeros or negatives ignored for gmean and hmean\n",h);
  print_msg(msg); 

  sprintf(msg, "STAT: DONE\n");
  print_msg(msg);
  action = MAIN;
  top();
} 




/**********************************  qi ******************************/
/* calls exec_qi() in qi_look.c   */

void do_qi(arg)
     char arg[256];
{
  nocom(arg);
  
  sprintf(msg, "Linearized inversion of one or two state variable friction model.\n\t(qi_h gives a detailed description of this function) \n");
   print_msg(msg);
  sprintf(msg, "\nNeed disp. col, mu col, model_mu col, beginning row for fit, row # of vel. step, end row, weight_row, lin_term(c), converg_tol, lambda,  wc, stiffness, v_initial, v_final, mu_init, a, b1, Dc1, b2 and Dc2 for input.\n ");
  print_msg(msg);
  sprintf(msg, "Input:\nd_col, mu_col, mod_col, first_row, vs_row, end_row, weight_row, c, tol, lambda, wc, k, v_o, vf, mu_o, a, b1, Dc1, b2, Dc2 \n") ;
  print_msg(msg);


  /*defaults, this flag is set to 0 at run time*/
  rs_param.op_file_flag &= 0x08;    /* leave eights bit, everything else off*/
  rs_param.op_file_flag  |=  0x04;		/*fours bit on by default -used for interactive control of iterations*/
  rs_param.law = 'r';
  wc = lambda = rs_param.lin_term = 0.0;       
  rs_param.end_weight_row=0;	
  rs_param.n_vsteps = 1;


  
  if (sscanf(arg, "%s %d %d %d %d %d %d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
	     desire,
	     &rs_param.disp_col, &rs_param.mu_col, &rs_param.mu_fit_col, 
	     &rs_param.first_row, &rs_param.vs_row, &rs_param.last_row, 
	     &rs_param.weight_row, &rs_param.lin_term, &converg_tol, 
	     &lambda, &wc, &rs_param.stiff, &rs_param.vo, &rs_param.vf, 
	     &rs_param.muo, &rs_param.a, &rs_param.b1, &rs_param.dc1, 
	     &rs_param.b2, &rs_param.dc2) != 21) 
    {
      nea();
      sprintf(msg, "Error reading rs info and initial guesses. Too many or too few parameters. Correct number is 20\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }


  junkp = strchr(desire, '.');
  if( junkp != NULL)		/*pointer to . -or null char if no .*/
    {
      while(*(++junkp) != '\0')	
	switch(*junkp)
	  {
	  case 'd': 			/*Use Dieterich, slowness law*/
	  case 'D':
	    rs_param.law = 'd';
	    break;
	  case 'p': 			/*Use Perrin-Rice, quadratic law*/
	  case 'P':
	    rs_param.law = 'p';
	    break;
	  case 'm': 			/*my law*/
	  case 'M':
	    rs_param.law = 'm';
	    break;
	  case 'j': 			/*Use Rice law*/
	  case 'J':
	    rs_param.law = 'j';
	    break;
	  case 'v':			/*qi.v or qi_mvs.v indicates verbose mode for op file*/
	  case 'V':
	    rs_param.op_file_flag  |=  0x01;		/*ones bit for verbose more*/
	    break;
	  case 'n':  			/*twos bit means create rather than append to file*/
	  case 'N':
	    rs_param.op_file_flag  |=  0x02;	
	    break;
	  case 'i': 			/*turn off fours bit*/
	  case 'I':
	    rs_param.op_file_flag  &=  ~0x04;		
	    break;
	    /*really want to toggle this*/
	  case 'f':  			/*eights bit means write data table*/
	  case 'F':
	    rs_param.op_file_flag  |=  0x08;
	    break;
	  case 's':  			/*sixteens bit means use separate file for each step*/
	  case 'S':
	    rs_param.op_file_flag  |=  0x10;		/*by appending vs_row to name*/
	    break;
	  case 't':  			/*32's bit means we're modeling mu vs time */
	  case 'T':
	    rs_param.op_file_flag  |=  0x20;		
	    break;
	  case 'k': 			/*64's bit means return velocity from get_mu_at_x_t, rather than mu*/
	  case 'K':
            rs_param.op_file_flag  |=  0x40;            
	    break;
	  case 'l': 			/*128's bit means return state from get_mu_at_x_t, rather than state*/
	  case 'L':
            rs_param.op_file_flag  |=  0x80;            
	    break;

	  }
    }
  

  if(strncmp(desire,"qi_mvs",6) == 0 )
    {
      sprintf(msg, "\nMultiple velocity steps.\n\tWeighting is from vs_row to next vs_row via weight vector -or between two rows if wc <0\n");
      print_msg(msg);
      sprintf(msg, "Enter the number of additional velocity steps, ");
      print_msg(msg);
      sprintf(msg, "a velocity for each step starting with the step after the standard `final' velocity (v1 v2 v3...) and ");	
      print_msg(msg);
      sprintf(msg, "a row number for each velocity change (row_v1_to_v2 row row..) \n");	
      print_msg(msg);

      set_left_footer("Enter: n_vsteps v1 v2 ... vsrow1 vsrow2 ...");
      set_cmd_prompt("qi_mvs input: ");
      action = QI_MVS;
      return;
    }
  
  if(wc < 0)
    {
      sprintf(msg, "Weight data between row %d and end_weight_row by a factor of %g. Enter end_weight_row.\n ", rs_param.weight_row, -wc);
      print_msg(msg);
      set_left_footer("Enter a row number");
      set_cmd_prompt("End_weight_row: ");
      action = QI_WC;
      return;
  }

  do_qi_final();
}

void do_qi_mvs(arg)
     char arg[256];
{
  nocom(arg);
  
  for(i=1;i<MAX_N_VSTEPS;i++)			/*set defaults*/
    {
      rs_param.vel_list[i] = -1;
      rs_param.vs_row_list[i] = -1;
    }
  
  strcpy(t_string,"\t ,");		
  /*load with possible token separators*/
  /* then work our way through "arg" list */
  /* Better than repeated calls to sscanf 
     since sscanf doesn't keep track of position within arg
     sscanf(arg, "%d", &rs_param.n_vsteps);	
     */
  sscanf(strtok(arg,t_string),"%d",&rs_param.n_vsteps); 
  /*read in number of extra steps*/
  rs_param.n_vsteps++;					
  /*add one to account for 1st step*/
  
  for(i=1;i<rs_param.n_vsteps;i++)
    sscanf(strtok(NULL,t_string),"%lf",&rs_param.vel_list[i]);
  
  for(i=1;i<rs_param.n_vsteps;i++)
    sscanf(strtok(NULL,t_string),"%d", &rs_param.vs_row_list[i]);
  
  /*check, make sure enough were entered*/
  
  for(j=0,i=1;i<MAX_N_VSTEPS;i++)			/*count number of v_steps*/
    {
      if(rs_param.vel_list[i] != -1 || rs_param.vs_row_list[i] != -1) 
	j++;
      else
	{
	  rs_param.vel_list[i] = 0;	/*set to zero instead of -1,   */
	  rs_param.vs_row_list[i] = 0;	/*in case they get used, accidentally, later*/
	}
    }
  
  if(DEBUG)
    {
      fprintf(stderr,"vel_list and row_list are:\n");
      for(i=1;i<MAX_N_VSTEPS;i++)			
	fprintf(stderr,"%f\t%d\n",rs_param.vel_list[i],rs_param.vs_row_list[i]);
    }
  if(j+1 != rs_param.n_vsteps)		/*compare*/
    {
      sprintf(msg, "\nProblem with v_steps, number entered does not match n_vsteps as entered (%d) \n", rs_param.n_vsteps);
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }
  
  if(wc < 0)
    { 
      sprintf(msg, "Weight data between row %d and end_weight_row by a factor of %g. Enter end_weight_row.\n ", rs_param.weight_row, -wc);
      print_msg(msg);
      set_left_footer("Enter a row number");
      set_cmd_prompt("End_weight_row: ");
      action = QI_WC;
      return;
    }
  
  do_qi_final();
}


void do_qi_final()  
{

  rs_param.vel_list[0]=rs_param.vf;
  rs_param.vs_row_list[0]=rs_param.vs_row;
  rs_param.vs_row_list[rs_param.n_vsteps]=rs_param.last_row;	/*for error cond.*/
  
  /* check cols and rows */
  if(rs_param.disp_col >= max_col ||  rs_param.mu_col >= max_col || rs_param.mu_fit_col >= MAX_COL) 
    {
      coe();
      action = MAIN;
      top();
      return;
    } 
  
  if( rs_param.last_row < rs_param.weight_row || rs_param.last_row < rs_param.vs_row || rs_param.last_row < rs_param.first_row ||  rs_param.weight_row < rs_param.first_row || rs_param.vs_row < rs_param.first_row || rs_param.last_row > head.ch[rs_param.disp_col].nelem)
    {
      sprintf(msg, "vs_step begin (%d) > end (%d) or end > nelems or weight_row (%d) < vs_row (%d)\n", rs_param.first_row, rs_param.last_row, rs_param.weight_row, rs_param.vs_row);
      print_msg(msg);
      action = MAIN;
      top();
      return;
  }


  /* malloc_debug(1);*/
  /*mallocmap();*/
  /*set up space for disp, mu, mu_fit*/
  /*if(
     (rs_param.disp_data = (double *)calloc((unsigned)(rs_param.last_row-rs_param.first_row+2),
sizeof(double))) == NULL ||
     (rs_param.mu_data   = (double *)calloc((unsigned)(rs_param.last_row-rs_param.first_row+2),
sizeof(double))) == NULL ||
     (rs_param.model_mu  = (double *)calloc((unsigned)(rs_param.last_row-rs_param.first_row+2),
sizeof(double)))  == NULL
    )*/
  if(
     (rs_param.disp_data = (double *)malloc((rs_param.last_row-rs_param.first_row+2)*sizeof(double))) == NULL ||
     (rs_param.mu_data   = (double *)malloc((rs_param.last_row-rs_param.first_row+2)*sizeof(double))) == NULL ||
     (rs_param.model_mu  = (double *)malloc((rs_param.last_row-rs_param.first_row+2)*sizeof(double)))  == NULL
    )
    {
        fprintf(stderr,"memory allocation problem in qi, see line ~4260 of cmds.c\n");
        sprintf(msg,"memory allocation problem in qi, see line ~4260 of cmds.c\n");
        print_msg(msg);
        action = MAIN;
        top();
        return;
    }
 
  /* copy data from look table into arrays*/
  for(j=1, i=rs_param.first_row; i <= rs_param.last_row; ++i, ++j)
    {
      /*start at 1 for "fortran" c of NR*/
      rs_param.disp_data[j]=(double)darray[rs_param.disp_col][i];  
      rs_param.mu_data[j]=(double)darray[rs_param.mu_col][i];
    }


  if(rs_param.op_file_flag & 0x20)
  {
      sprintf(msg, "\nModeling time vs. mu, not disp. vs. mu\n");
      print_msg(msg);
  }

/*returning velocity rather than mu*/
 if(rs_param.op_file_flag & 0x40)    
  {
      sprintf(msg, "\n\"k\" option used. Velocity --and not mu-- will be returned. This only works for forward model, so lambda is being set < 0. \n");
      print_msg(msg);
      lambda = -.1;
  }
  else if(rs_param.op_file_flag & 0x80)  
  {
      sprintf(msg, "\n\"l\" option used. State, the 1st state variable --and not mu-- will be returned. This only works for forward model, so lambda is being set < 0. \n");
      print_msg(msg);
      lambda = -.1;
  } 

  if(lambda < 0)
    {
      sprintf(msg, "\nCalculating rs model for given parameters --no inversion being done.\n\n");
      print_msg(msg);
    }
  else
    {
      sprintf(msg, "Calling qi...%s recs: %d to %d\n\n", head.title, rs_param.first_row, rs_param.last_row);
      print_msg(msg);
    }
  
  set_left_footer("qi running     ");


  if(exec_qi(converg_tol, lambda, wc, &rs_param) == -1)
    {
      free(rs_param.model_mu);
      free(rs_param.mu_data);
      free(rs_param.disp_data);
      sprintf(msg, "problem in qi inversion\n");
      print_msg(msg);
      action = MAIN;
      top();
      return;
    }


  /*nameing BS here so we don't call name()*/
  if(strncmp(head.ch[rs_param.mu_fit_col].name,"no_val",6) == 0)
    head.nchan += 1 ;
  head.ch[rs_param.mu_fit_col].nelem = head.ch[rs_param.disp_col].nelem ;

  if(rs_param.op_file_flag & 0x40)    /*returning velocity rather than mu*/
    strcpy(head.ch[rs_param.mu_fit_col].name, "mod_vel_"); 
  else if(rs_param.op_file_flag & 0x80)  /*returning state rather than mu*/
    strcpy(head.ch[rs_param.mu_fit_col].name, "state_"); 
  else		/*default, mu is returned*/
  {
    /* name for model fric. col is "mod_mu ". Units are "." */
    if(rs_param.one_sv_flag)
      strcpy(head.ch[rs_param.mu_fit_col].name, "mod_mu1_"); 
    else
      strcpy(head.ch[rs_param.mu_fit_col].name, "mod_mu2_"); 

  }

  strcat(head.ch[rs_param.mu_fit_col].name,&rs_param.law);
  strcpy(head.ch[rs_param.mu_fit_col].units, "." );
  
  /* install model data */
  for(j=1, i=rs_param.first_row; i <= rs_param.last_row; ++i, ++j)
    darray[rs_param.mu_fit_col][i]=(float)rs_param.model_mu[j];
  
  /*mallocmap();*/
  free(rs_param.model_mu);
  free(rs_param.mu_data);
  free(rs_param.disp_data);


  sprintf(msg, "QI: DONE\n");
  print_msg(msg);
  action = MAIN;
  top();
}



void do_qi_help()
{
  
  sprintf(msg, "> An iterative, non-linear inverse routine using the\n\tLevenberg-Marquardt method (variable damping) and svd to solve the inverse problem.\n");
  print_msg(msg);
  
  sprintf(msg, "> Default is to invert for the Ruina (slip law) rate and state friction law,\n");
  print_msg(msg);

  sprintf(msg, "> \tother laws can also be used (see options below) \n");
  print_msg(msg);

  sprintf(msg, "> tol is the convergence tolerance (1-chisq/prev_chisq), default is 1e-4\n");
  print_msg(msg);

  sprintf(msg, "> lambda is the Levenberg-Marquardt factor (0.1 is a good start)\n");
  print_msg(msg);

sprintf(msg, "> wc is a weighting factor for the weight vector w[i] = (ndata/i)^wc\n");
print_msg(msg);

  sprintf(msg, "> for weighting: i is relative to weight_row (this will normally be vs_row)\n");
print_msg(msg);

  sprintf(msg, "%f \n", mu_fit_mess_2);
  print_msg(msg);

  sprintf(msg, "> A linear term can be added to the friction law to account for hardening/weakening\n\t\t: mu = mu_* + a ln(V/V*) + b phi + (c)*dx, where c has units of 1/disp\n>\tSet c=0 for no lin_term\n");
  print_msg(msg);

  sprintf(msg, "> This routine needs: the columns that contain disp. and friction, the row # of the velocity step,\n");
  print_msg(msg);

  sprintf(msg, "> the row # to which the fit should be extended, and initial guesses for the various constitutive parameters.\n");
  print_msg(msg);

  sprintf(msg, "> Give k in the units of 1/dc (e.g., friction/dc).\n");
  print_msg(msg);

  sprintf(msg, "> The model output has the same number of rows as the input data.\n");
  print_msg(msg);

  sprintf(msg, ">If lambda is < 0, a rate state calc. is done for the initial guesses and put in mod_mu_col\n");
  print_msg(msg);

  sprintf(msg, ">If wc is < 0 weighting is not done via the standard function (above).\n>Instead data between weight_row and end_weight_row are weighted by a factor of wc more than other data.\n");
  print_msg(msg);

  sprintf(msg, ">If Dc2 is < 0 a one state variable is computed.\n");
  print_msg(msg);

  sprintf(msg, ">Multiple velocity steps may be computed (set lambda<0) or inverted using the command `qi_mvs'\n");
  print_msg(msg);

  sprintf(msg, ">The following options are available by appending `#' to the command, where # is:\n");
  print_msg(msg);

  sprintf(msg, "\t.i turn off interactive mode (on by default) which queries about whether to continue iterating in some situations.\n");
  print_msg(msg);

  sprintf(msg, "\t.f final mode: create data table. Once turned on this stays on until .f is used again\n");
  print_msg(msg);

  sprintf(msg, "\t.n create new log file rather than append to existing file (which is destroyed).\n");
  print_msg(msg);

  sprintf(msg, "\t.s write a separate log file for each step (by appending vs_row to the name).\n");
  print_msg(msg);

  sprintf(msg, "\t.v write a slightly more verbose log file.\n");
  print_msg(msg);

  sprintf(msg, "\t.t Model time vs. mu, not disp. vs. mu.\n");
  print_msg(msg);

  sprintf(msg, "\t.d invert for the Dieterich-Ruina, slowness law\n");
  print_msg(msg);
  
  sprintf(msg, "\t.p invert for the Perrin-Rice, quadratic law\n");
  print_msg(msg);

  sprintf(msg, "\t.j invert for the Rice law\n");
  print_msg(msg);

  sprintf(msg, "\t.k return velocity rather than mu. Velocity is put in mod_mu_col. Only works for forward model. \n");
  print_msg(msg);

  sprintf(msg, "\t.l return state rather than mu. 1st state var. is put in mod_mu_col. Only works for forward model.\n");
  print_msg(msg);

  sprintf(msg, ">multiple options are allowed, use (e.g.) .nvi\n");
  print_msg(msg);

  sprintf(msg, ">NOTE that qi type modeling is available in a tool that can be launched from the main window or with the command \"qi_tool\" which also takes the regular dot extensions \n");
  print_msg(msg);

   sprintf(msg, "\nqi input: disp._col, fric_col, model_output_col, first_row_for_fit, row_#_of_vel._step, last_row, weight_row, lin_term(c), converg_tol, lambda,  weighting_coef, stiffness, v_initial, v_final, mu_init, a, b1, Dc1, b2 and Dc2 for input.\n ");
   print_msg(msg);
}




/************************************* scm ****************************/
/* calls simp_func() in simplexl.c, simp_weight() function is moved from look_funcs.c into this section  */


void do_scm(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg,"%s %d %d %d %d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", 
	     desire,
	     &rs_param.disp_col, &rs_param.mu_col, &rs_param.vs_row, 
	     &rs_param.last_row, &rs_param.mu_fit_col,
	     &rs_param.stiff, &rs_param.sig_n, &rs_param.vo, &rs_param.vf, 
	     &rs_param.muo,
	     &rs_param.muf, &rs_param.a, &rs_param.b1, &rs_param.dc1, 
	     &rs_param.dc2) != 16)
    {
      nea();
      action = MAIN;
      top();
      return;
    }

  adj_init_disp = mu_check(&rs_param.muo,1); /* returns false if passed -33 */
  mu_check(&rs_param.muf,0);	     /* this is in look_funcs.c */
  rs_param.amb = (rs_param.muf-rs_param.muo)/log(rs_param.vf/rs_param.vo);
  if(rs_param.dc2 < 0.0)
    {
      rs_param.b2 = 0.0;
      rs_param.dc2 = 1e10;		/* I think leaving it neg. will lead to matherr() being called */
      rs_param.b1 = rs_param.a - rs_param.amb;
      l = 2;
    }
  else
    {
      rs_param.b2 = -(rs_param.amb-rs_param.a+rs_param.b1);
      l = 4;
    }
  
  if(rs_param.disp_col >= max_col ||  rs_param.mu_col >= max_col || rs_param.mu_fit_col >= MAX_COL)
    {
      coe();
      top();
      action = MAIN;
      return;
    } 
  
  if(rs_param.last_row < rs_param.vs_row || rs_param.last_row > head.ch[rs_param.disp_col].nelem)
    {
      sprintf(msg,"vs_step begin (%d) > end (%d) or end > nelems.\n", rs_param.vs_row, rs_param.last_row);
      print_msg(msg);
      top();
      action = MAIN;
      return;
    }
  
  /*nameing BS here so we don't call name() */
  if(strncmp(head.ch[rs_param.mu_fit_col].name,"no_val",6) == 0)
    head.nchan += 1 ;
  head.ch[rs_param.mu_fit_col].nelem = head.ch[rs_param.disp_col].nelem ;

  /* name for model fric. col is "simp_mu". Units are "." */
  if(l==2)
    strcpy(head.ch[rs_param.mu_fit_col].name, "simp_1_mu"); 
  else
    strcpy(head.ch[rs_param.mu_fit_col].name, "simp_2_mu"); 
  
  strcpy(head.ch[rs_param.mu_fit_col].units, "." );

  /* set weighting. rs_param.weight_control is neg. if */
  /*pre-peak overestimate is favored over underestimate and postpeak*/
  /* underestimate is favored over underestimate */

  /*simp_weight(command,l,&temp_int); */


  if(l==2)
    {
      sprintf(msg, "\nEnter the simplex parameters: max_iterations, peak row #, row # to weight to, parameter error (a, dc, and total error) , and first step size  (a and dc) \n");
      print_msg(msg);
      
      sprintf(msg, ">> Enter -1 for weight row to get less than std weighting or 0 to get no weighting\n");
      print_msg(msg);

      sprintf(msg, ">> Enter -1*(peak row) to favor overestimate of mu between the vs_row and the peak row\n");
      print_msg(msg);

      sprintf(msg, ">> If you want (very tight) defaults for the error and step size, set a_err to < 0\n");
      print_msg(msg);

      sprintf(msg, "\tInput: max_iter, peak_row, wt_row, a_err, dc_err, total_err, a_step, dc_step\n");
      print_msg(msg);

      set_left_footer("Type the arguments for simplex weight");
      set_cmd_prompt("Input: ");
      action = SCM_SIMP_WEIGHT_L2;
      return;
    }
  else
    {
      sprintf(msg, "Enter the simplex parameters: max_interations, peak row #, row # to weight to, parameter error (a, b1, dc1, dc2, and total error) , and first step size  (a, b1, dc1, and dc2)\n");
      print_msg(msg);

      sprintf(msg, ">> Enter -1 for weight row to get less than std weighting or 0 to get no weighting\n");
      print_msg(msg);

      sprintf(msg, ">> Enter -1*(peak row) to favor overestimate of mu between the vs_row and the peak row\n");
      print_msg(msg);

      sprintf(msg, ">> If you want (very tight) defaults for the error and step size, set a_err to < 0\n");
      print_msg(msg);
      
      sprintf(msg, "\tInput: max_iter, peak_row, wt_row, a_err, b1_err, dc1_err, dc2_err, total_err, a_step, b1_step, dc1_step, dc2_step\n");
      print_msg(msg);

      set_left_footer("Type the arguments for simplex weight");
      set_cmd_prompt("Input: ");
      action = SCM_SIMP_WEIGHT;  
      return;
    }
}


void do_simp_weight_l2(arg)
     char arg[256];
{
  /*     int l, *temp_int; */

  nocom(arg);
  
  if (sscanf(arg, "%d %d %d %lf %lf %lf %lf %lf", temp_int, &rs_param.peak_row, &rs_param.weight_pts, &rs_param.a_er, &rs_param.dc1_er, &rs_param.total_er,&rs_param.a_step,&rs_param.dc1_step) != 8)
    {
      nea();
      top();
      action = MAIN;
      return;
    }

  if( rs_param.a_er < 0)
    {
      if(rs_param.a_er == -22)
	{
	  rs_param.a_er =         1.00000e-4;  /* less stringent values */
	  rs_param.dc1_er =       1.00000e-3;
	  rs_param.total_er =     1.00000e-4;
	}
	  
      else
	{
	  rs_param.a_er =         1.00000e-8;  /* install default values */
	  rs_param.dc1_er =       1.00000e-8;
	  rs_param.total_er =     1.00000e-9;
	}
	  
      rs_param.a_step = 0.010;
      (rs_param.dc1 > 1.2) ? rs_param.dc1_step = -1.0 : rs_param.dc1_step -0.5;
    }
   
  do_simp_weight_final();
  
}


void do_simp_weight(arg)
     char arg[256];
{
  nocom(arg);
  
  if (sscanf(arg, "%d %d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf", 
	     temp_int, &rs_param.peak_row, &rs_param.weight_pts, 
	     &rs_param.a_er, &rs_param.b1_er,
	     &rs_param.dc1_er, &rs_param.dc2_er, &rs_param.total_er, 
	     &rs_param.a_step, &rs_param.b1_step,
	     &rs_param.dc1_step, &rs_param.dc2_step) != 12) 
    {
      nea();
      top();
      action = MAIN;
      return;
    }

  if( rs_param.a_er < 0)
    {
      if(rs_param.a_er == -22)
	{
	  rs_param.a_er =         1.00000e-4;   /* less stringent values */
	  rs_param.b1_er =        1.00000e-4;
	  rs_param.dc1_er =       1.00000e-3;
	  rs_param.dc2_er =       1.00000e-3;
	  rs_param.total_er =     1.00000e-4;
	}
      else
	{
	  rs_param.a_er =         1.00000e-8;  /* install default values */
	  rs_param.b1_er =        1.00000e-8;
	  rs_param.dc1_er =       1.00000e-8;
	  rs_param.dc2_er =       1.00000e-8;
	  rs_param.total_er =     1.00000e-9;
	}
      
      rs_param.a_step =       0.01;
      rs_param.b1_step =      0.01;
      rs_param.dc1_step =     -0.5;
      rs_param.dc2_step =     -5.0;
    }
  
  do_simp_weight_final();
}        
  

void do_simp_weight_final()
{
  
  /* gives weight of 5x for 100 pts*/
  /* peak is weighted 6x weight and interpolated pts are weighted 0.75x weight */
  rs_param.weight = (rs_param.last_row-rs_param.vs_row)/20.00;
  
  /* reduce weighting optionally or for big data sets */
  if( (rs_param.last_row-rs_param.vs_row > 300) || (rs_param.weight_pts < 0) )
    {
      rs_param.weight /= (rs_param.last_row-rs_param.vs_row)/100.0;
      sprintf(msg, "weight limited optionally or because data set > 300..%c\n",BELL);
      print_msg(msg);
      
      if(rs_param.weight_pts < 0)
	rs_param.weight_pts *= -1;
    }

  if(rs_param.weight < 2)
    rs_param.weight = 2.00; 
  
  if(rs_param.weight_pts == 0)		/* no weighting */
    rs_param.weight=1.00;
  
  if(rs_param.peak_row < 0)
    {
      rs_param.peak_row *= -1;
      rs_param.weight_control = -1;
    }
  else
    rs_param.weight_control = 1;
  
  do_scm_next();
  
}



void do_scm_next()
{
  
  /*  allocate space for arrays used by simplex and fq */
  rs_param.disp_data = (double *)calloc((unsigned)(rs_param.last_row-rs_param.vs_row+13), sizeof(double)) ;
  rs_param.mu_data   = (double *)calloc((unsigned)(rs_param.last_row-rs_param.vs_row+13), sizeof(double)) ;
  rs_param.model_mu  = (double *)calloc((unsigned)(rs_param.last_row-rs_param.vs_row+13), sizeof(double)) ;
  
  disp_ptr = rs_param.disp_data;
  mu_ptr = rs_param.mu_data;
  
  if(adj_init_disp)
    {
      /* adjust disp. of first pt to account for mu0 (possibly) not = mu[vs_row]  */
      
      sl = (darray[rs_param.mu_col][rs_param.vs_row+1]-darray[rs_param.mu_col][rs_param.vs_row]) /
	(darray[rs_param.disp_col][rs_param.vs_row+1]-darray[rs_param.disp_col][rs_param.vs_row]) ;

      dx = (rs_param.muo-darray[rs_param.mu_col][rs_param.vs_row])/sl + darray[rs_param.disp_col][rs_param.vs_row];

      if (dx > darray[rs_param.disp_col][rs_param.vs_row+1])
	{
	  sprintf(msg,"Problem with getting init. disp. correct for input init. mu.\n");
	  print_msg(msg);
	  sprintf(msg,"Sorry, but you'll have to fix something or change init. mu to be closer to mu of vs_row.\n");
	  print_msg(msg);
	  top();
	  action = MAIN;
	  return;
	}
      else
	*disp_ptr++ = dx;	/* install as vs_row */
    }
  else
    *disp_ptr++=darray[rs_param.disp_col][rs_param.vs_row];
  
  
  *mu_ptr++ = rs_param.muo;	/* install as vs_row */
  
  /* disp, mu pt is now on a line <> vs pt and next pt */
  
  rs_param.added_pts = 0;		/* default value */	
  
  if(rs_param.peak_row-rs_param.vs_row < 4 )
    {
      /* take avg. slope -weighted a bit more toward peak pt */
      sl = 0.0;
      for(j = rs_param.vs_row+1; j <= rs_param.peak_row; ++j)
	sl += (darray[rs_param.mu_col][j]- *rs_param.mu_data)/(darray[rs_param.disp_col][j]- *rs_param.disp_data);
      
      /* weight slope to peak */
      sl += (darray[rs_param.mu_col][rs_param.peak_row]- *rs_param.mu_data)/(darray[rs_param.disp_col][rs_param.peak_row]- *rs_param.disp_data);
      sl /= (double) (rs_param.peak_row-rs_param.vs_row +1);
      
      dx = (darray[rs_param.disp_col][rs_param.peak_row]- *rs_param.disp_data) / 5.00;
      
      /* 16/6/93 changed this from 10 points between peak and vs_row to 5 points */
      for(i=1; i <= 5; ++i)          /* interpolated points */
	{
	  *disp_ptr++ = *rs_param.disp_data + dx*(double)i;
	  *mu_ptr++ =  *rs_param.mu_data + sl*dx*(double)i;
	}	
      rs_param.added_pts = (int)(floor(5.0/(rs_param.peak_row - rs_param.vs_row))) -1;
      
    }
  else
    {
      for(j=rs_param.vs_row+1; j <=rs_param.peak_row; ++j)
	{
	  *disp_ptr++ = darray[rs_param.disp_col][j] ;
	  *mu_ptr++ = darray[rs_param.mu_col][j] ;
	}
    }
  
  for(j=rs_param.peak_row+1; j <=rs_param.last_row; ++j)
    {
      *disp_ptr++ = darray[rs_param.disp_col][j] ;
      *mu_ptr++ = darray[rs_param.mu_col][j] ;
    }
  
  /* so that calc stops properly*/
  *disp_ptr = darray[rs_param.disp_col][rs_param.last_row]+1e9 ;
  
  /* to check interp pts.. */
  /*	disp_ptr = rs_param.disp_data;
	mu_ptr = rs_param.mu_data;
	for(i=rs_param.vs_row; i < rs_param.last_row+12; ++i)
	{
	darray[rs_param.disp_col][i] = *disp_ptr++;
	darray[rs_param.mu_col][i] = *mu_ptr++;
	} 
	break;		*/
  
  strcpy(t_string, "rs_fit");			/* for simplexl */

  
  if(rs_param.a < 0)		/*calculate error matrix*/
    {
      sprintf(msg,"Calculate the chi-squarred error between a model and data (1sv only).\nOutput is to a file named cse_exp_name\n\tEnter the range for a and dc: ta_min, a_max, a_inc, dc_min, dc_max, dc_inc");
      print_msg(msg);
      
      set_left_footer("Type the arguments");
      set_cmd_prompt("Input: ");

      /* get args for scm_1
	 call do_scm_1()
	 DONE
	 */
            
      action = SCM_1;
      return;
    }

  
  else	/* just do the simp calc like I used to */
    {
      /* temp_int = max_iterations */
      sprintf(msg, "calling simp_func... %s recs: %d to %d\n", head.title, rs_param.vs_row, rs_param.last_row);
      print_msg(msg);
      
      /* 
	 set simp_func_action so it knows where to go to after that
	  (because simp_func can also be called from simplex cmd)
	 call simp_func()
	 then go to do_scm_2()
	 DONE
	 */

/*
      set_left_footer("Type the arguments");
      set_cmd_prompt("Input: ");
*/

      simp_func_action = SCM;
      action = SIMP_FUNC;
      return; 

      /* simp_func(command,l,simp_xch,&simp_ych,t_string,first,last, temp_int);
       */
      
    }
} 



void do_scm_1(arg)
     char arg[256];
{
  if (sscanf(arg,"%lf , %lf , %lf , %lf , %lf , %lf",&a_min, &a_max, &rs_param.a_step, &dc_min, &dc_max, &rs_param.dc1_step) != 6)
    {
      nea();
      top();
      action = MAIN;
      return;
    }
  
  /*open file for data: x y z vectors*/
  strcpy(t_string,"cse_");
  strcat(t_string,head.title);
  new = fopen(t_string, "w");      
  
  rs_param.b2 = 0.0;		/*1sv model*/
  rs_param.dc2 = 1e10;            
  l = 2;

  /*put x and y vectors as first two rows*/
  for(rs_param.a=a_min; rs_param.a<=a_max; rs_param.a +=rs_param.a_step)
    fprintf(new,"%f\t",rs_param.a);
  fprintf(new,"\n");
  for(rs_param.dc1=dc_min; rs_param.dc1<=dc_max; rs_param.dc1 +=rs_param.dc1_step)
    fprintf(new,"%f\t",rs_param.dc1);
  fprintf(new,"\n");

  for(rs_param.a=a_min; rs_param.a<=a_max; rs_param.a +=rs_param.a_step)
    {
      rs_param.b1 = rs_param.a - rs_param.amb;
      for(rs_param.dc1=dc_min; rs_param.dc1<=dc_max; rs_param.dc1 +=rs_param.dc1_step)
	{
	  err=simp_rate_state_mod();	
	  fprintf(new,"%g\t",err);	/*put in matrix form for contour plot*/
	  /*fprintf(new,"%f\t%f\t%g\n",rs_param.a,rs_param.dc1,err);*/
	  sprintf(msg, "loops over `a' remaining %d, loops over `dc' remaining %d      \r",(int)((a_max-rs_param.a)/rs_param.a_step)+1, (int)((dc_max-rs_param.dc1)/rs_param.dc1_step)+1);
	}
      fprintf(new,"\n");
    }
  fclose(new);
  sprintf(msg, "\n");

  free(rs_param.model_mu);
  free(rs_param.mu_data);
  free(rs_param.disp_data);
  
  sprintf(msg, "SCM: DONE\n");
  print_msg(msg);

  action = MAIN;
  top();
}



void do_scm_2()
{
  if(l == 2 )
    {
      sprintf(msg, "a-b = %g\t Best simplex fit (vs_row=%d): a=%g\tb=%g\tdc=%g\n", rs_param.amb, rs_param.vs_row, rs_param.a, rs_param.b1, rs_param.dc1);
      print_msg(msg);
    }
  else
    {
      sprintf(msg, "a-b = %g\t Best simplex fit (vs_row=%d): a=%g\tb1=%g\tb2=%g\tdc1=%g\tdc2=%g\n", rs_param.amb, rs_param.vs_row, rs_param.a, rs_param.b1, rs_param.b2, rs_param.dc1, rs_param.dc2);
      print_msg(msg);
    }
      
  for ( i= (rs_param.vs_row > 15) ? (rs_param.vs_row-15) : 0; i <= rs_param.vs_row; ++i)
    darray[rs_param.mu_fit_col][i] = rs_param.muo;;
  
  /* install model data */
  mu_ptr = rs_param.model_mu; 
  
  if(rs_param.added_pts != 0)
    {
      disp_ptr = rs_param.disp_data+1;	/* recall vs_row is in array*/
      
      dx = 0.0;
      for(i=0; i < 10; ++i);
      {
	dx += fabs( *(disp_ptr+1) - *disp_ptr++)/50.00 ;
      }
      dx /= 10.00;
      if(dx  < 0.0200) dx = 0.0200;
      
      disp_ptr = rs_param.disp_data+1;
      
      for(i=rs_param.vs_row+1, j = 0; j < 5; ++j)
	{
	  /* do it this way, since if pts <> vs_row and peak are now evenly spaced we can't simply jump ahead x pts to find the mu value corresponding to x at a given i */
	  if( (darray[rs_param.disp_col][i] - *(disp_ptr++)) < dx)
	    darray[rs_param.mu_fit_col][i++] = *mu_ptr++ ;
	  else
	    mu_ptr++;
	}
      
      if( i != rs_param.peak_row+1)
	{
	  sprintf(msg, "Problem installing data from simp. array %c%c\n",BELL,BELL);
	  print_msg(msg);
	}
      
      for(i=rs_param.peak_row+1; i <=rs_param.last_row; ++i)
	darray[rs_param.mu_fit_col][i] = *mu_ptr++ ;
    }
  
  else
    for(i=rs_param.vs_row+1; i <=rs_param.last_row; ++i)
      darray[rs_param.mu_fit_col][i] = *mu_ptr++ ;
  
  free(rs_param.model_mu);
  free(rs_param.mu_data);
  free(rs_param.disp_data);

  sprintf(msg, "SCM: DONE\n");
  print_msg(msg);
  
  action = MAIN;
  top();
}

void do_scm_help()
{
  sprintf(msg,"> This function does a simplex fit of the Dieterich-Ruina rate/state variable friction model to data.\n");
  print_msg(msg);
  sprintf(msg,"> It needs: the columns that contain disp. and friction, the row # of the velocity step,\n");
  print_msg(msg);
  sprintf(msg,"> the row # to which the fit should be extended, and initial guesses for the various constitutive parameters.\n");
  print_msg(msg);
  sprintf(msg,"> Give k in the same units as normal stress and dc.\n");
  print_msg(msg);
  sprintf(msg,"> The pre-step mu value is extended back 15 rows in the model output.\n");
  print_msg(msg);
  sprintf(msg, mu_fit_mess_1);
  print_msg(msg);
  /*sprintf(msg,"> To get a one state variable fit set Dc2 to < 0. ");
    print_msg(msg);
    sprintf(msg,"> 	In this case b1 will be set automatically ");
    print_msg(msg);
    sprintf(msg,"> On picking the velocity step row: Pick v_step row as first row after v_step\n>\tThe displacement of that point is adjusted to account for mu_o (possibly) not = mu[v_step row]");
    print_msg(msg);
    sprintf(msg,"> On mu: enter -22 to get the average mu value within a 10point window around vs_row or end_row\n\tenter -33 to get the mu value 1pt before either row or\n\tUse any other negative number to get the mu value at vs_row or end_row"); 
    print_msg(msg); */
}

void do_scm_info()
{
  sprintf(msg,"Simplex fit of one or two state variable friction model. (scm_h or simp_rsm_h gives a detailed description of this function) \n");
  print_msg(msg);
  
  sprintf(msg,"> To get a one state variable fit set Dc2 to < 0. b1 will be set automatically \n");
  print_msg(msg);
  
  sprintf(msg,"\nInput: disp. col., mu col, row # of vel. step and end of data to model, col for model_mu,  stiffness (k), Sigma_n, v_initial (v_o), v_final, mu_o, mu_f, a, b1, Dc1, and Dc2, tdisp_col,  mu_col, vs_row,  end_row,  m_mu_col,  k,  Sigma_n,  v_o,  vf,  mu_o,  mu_f,  a,  b1, Dc1, Dc2: ") ;
  print_msg(msg);
}
 

