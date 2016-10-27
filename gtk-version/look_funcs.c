#include "config.h"
#include "global.h"
#include "look_funcs.h"
#include "messages.h"

extern int action;
extern char msg[MSG_LENGTH];
int name_action;


/* all the funcs that used to be at the end of lookv3.c */
/*  ---and more...*/

/*-------------------------------------------------------------------------*/
#define OUT 0
#define IN 1

int token_count(buf,max_length)
     char buf[];
     int max_length;
{

  int state, n, i;

  state=OUT;                      /* count the number of tokens in buf */
  for(n=0,i=0; buf[i] != '\0'; ++i)
    {
      if(i==max_length-2 && buf[i+1] != '\0')		/*stop at max_length*/
      {
	buf[i+1] = '\0';
	printf("Token_count: string did not contain a null. One was added\n");
      }
      if(buf[i] == ' ' || buf[i] == '\t' || buf[i] == ',')
	{
	  state=OUT;
	}
      else if(state==OUT)
	{
	  state=IN;
	  ++n;
	}
    }
  
  return(n);
}
#undef OUT
#undef IN


/* -------------------------------------------------------------------------*/
int allocate(row,col)
     int row , col ;
{
  int i , j ;
  unsigned unbytes ;
  unbytes = (unsigned)(sizeof(double)*row) ;
  if ( col >= MAX_COL || col <= 0 || row <= 0) 
    {
       	sprintf(msg,"Error. col >= MAX_COL || col <= 0 || row <= 0, You entered %d for ncol and %d for nrow.\n",col,row);
        print_msg(msg);
       	sprintf(msg,"MAX number of cols is %d. This can be changed by changing global.h and recompiling.\n",MAX_COL-1);
        print_msg(msg);
	return 0 ;
    }
  if ( col >= max_col || row >= max_row)
    {
      for( i=0; i<max_col; ++i)
	{
	  darray[i] = (double *)realloc((char *)darray[i],unbytes) ;
	  if( row>max_row-1 )
	    {
	      for (j=max_row; j<row; ++j)
		{
		  darray[i][j] = 0.0 ;
		}
	    }
	}
      for( i=max_col; i<col; ++i )
	{
	  darray[i] = (double *)calloc((unsigned)row,sizeof(double)) ;
	}
    }
  if ( row<head.nrec )
    {
      head.nrec = row ;
      for( j=1; j < max_col; ++j )
	{
	  if( check_col(j) > 0 ) head.ch[j].nelem = row ;
	}
    }
  
  max_col = (col > max_col) ? col : max_col ;
  max_row = (row > max_row) ? row : max_row ;
  
  /* for plotting */
/* These are defined and the allocation is done in main.c*/
  arrayx = (double *)realloc((char *)arrayx,(unsigned)(max_row * sizeof(double))) ;
  arrayy = (double *)realloc((char *)arrayy,(unsigned)(max_row * sizeof(double))) ;
  return 1 ;
}

/*------------------------------------------------------------------------*/

void null_col(col)
     int col ;
{
   strcpy(&(head.ch[col].name[0]),"no_val") ;
   strcpy(&(head.ch[col].units[0]),"no_val") ;
   strcpy(&(head.ch[col].comment[0]),"none") ;
   head.ch[col].nelem = 0 ;
   head.ch[col].gain = 0 ;
}

/*------------------------------------------------------------------------*/
int mu_check(mu,vstep)		
     /*vstep is true if mu is mu_initial and false if mu is mu_final*/
     double	*mu;
     int	vstep;
{

  int i;
  int mu_window_start;
  int mu_row;
  int test;

  test = TRUE;

  if(*mu < 0)
    {
      /*take 10 points before the vstep or 5 points on either side of the last point*/
      mu_window_start = (vstep == TRUE) ? (rs_param.vs_row-9) : (rs_param.last_row-5);
      mu_row	= (vstep == TRUE) ? rs_param.vs_row : rs_param.last_row;

      if(*mu == -22)         /* avg. of 10 pts */
	{
	  *mu = 0.0;
	  for(i=mu_window_start; i <= mu_window_start+10; ++i)
	    *mu += darray[rs_param.mu_col][i];
	  *mu /= 10.0;
	}
      if(*mu == -33)         /* val at row < vs_row -> do this to allow for emergent direct effect*/
	{
	  *mu =  darray[rs_param.mu_col][mu_row-1];
	  test=FALSE;
	}
      else                            /* just take vs_row val */
	*mu =  darray[rs_param.mu_col][mu_row];
     
      sprintf(msg,"\tauto mu set: mu%c = %f\n", (vstep == TRUE) ? 'o' : 'f', *mu);
      print_msg(msg);
    }
  
  return(test);
}


/*------------------------------------------------------------------------*/
int check_row(first_row, last_row, this_col)
     int *first_row, *last_row, this_col;
{
  int test;
  
  test = 0;

  if(this_col >= max_col)
  {
      sprintf(msg, "col (%d) not defined\n",this_col);
      print_msg(msg);
      test++;
      return test;
  }

  if( strncmp(head.ch[this_col].name,"no_val",6) ==0 )
    {
      sprintf(msg, "col (%d) not defined\n",this_col);
      print_msg(msg);
      test++;
      return test;
    }

  /* use -1 as variable for nrec */
  if(*last_row == -1 || *last_row == head.ch[this_col].nelem)
    *last_row = head.ch[this_col].nelem-1;
  else if(*last_row >= head.ch[this_col].nelem)
    {
      sprintf(msg, "last row (%d) > nelems for col (%d)\n", *last_row, this_col);
      print_msg(msg);
      test++; 	
      return test;
    }

  if(*first_row < 0)
    {
      sprintf(msg, "first row (%d) can't be less than 0\n", *first_row);
      print_msg(msg);
      test++; 	
      return test;
    }

  if(*last_row < *first_row)
    {
      sprintf(msg, "last row (%d) can't be less than first row (%d)\n", *last_row, *first_row);
      print_msg(msg);
      test++; 	
      return test;
    }
  return test;
  
}


/*------------------------------------------------------------------------*/
int check_col(col)
     int col ;
{
  int test ;

  test = 0 ;
  test += strncmp(head.ch[col].name,"no_val",6) ;
  test += strncmp(head.ch[col].units,"no_val",6) ;
  test += strncmp(head.ch[col].comment,"none",6) ;
  if( head.ch[col].gain != 0 ) test += 1 ;
  if( head.ch[col].gain != 0 ) test += 1 ;
  return test ;
}


/*------------------------------------------------------------------------*/
int act_col()
{
  int numb=0 ;
  int i ;
  for( i=1; i<MAX_COL; ++i)
    {
      if(check_col(i) > 0) numb = i ;
    }
  return numb ;
}


/*------------------------------------------------------------------------*/
  /*changes to formats, cjm 20150517 & 20150628*/
void review(file, firstcol, lastcol)
FILE *file ;
int *firstcol, *lastcol;
{
  int i ;
  if (file == stderr) 
    fprintf(file,"ALLOCATION: max_col = %d , max_row = %d\t",max_col,max_row);
  /*`printf(file,"number of records = %d\n     ",head.nrec) ;*/
/*
  for( i = *firstcol; i <= *lastcol; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  fprintf(file,"     col %1d",i) ;
	}
    }
  fprintf(file,"\n     ") ;
*/

  fprintf(file,"%7s","RecNum") ;
  for( i = *firstcol; i <= *lastcol; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  fprintf(file,"%15s",head.ch[i].name) ;
	}
    }
  fprintf(file,"\n     ") ;

  fprintf(file,".") ;
  for( i = *firstcol; i <= *lastcol; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  fprintf(file,"%15s",head.ch[i].units) ;
	}
    }
  fprintf(file,"\n");

  if (file == stderr) 
  {
  for( i = *firstcol; i <= *lastcol; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  fprintf(file,"%7d recs",head.ch[i].nelem) ;
	}
    }
   fprintf(file,"\n") ;
  }
}

/*------------------------------------------------------------------------*/
  /*added this so that we can have csv format, cjm 20150628*/
void p_review(file, firstcol, lastcol)
FILE *file ;
int *firstcol, *lastcol;
{
  int i ;
  if (file == stderr) 
    fprintf(file,"ALLOCATION: max_col = %d , max_row = %d\t",max_col,max_row);

  fprintf(file,"%s","RecNum") ;
  for( i = *firstcol; i <= *lastcol; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  fprintf(file,", %s",head.ch[i].name) ;
	}
    }
  fprintf(file,"\n") ;

  fprintf(file,".") ;
  for( i = *firstcol; i <= *lastcol; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  fprintf(file,", %s",head.ch[i].units) ;
	}
    }
  fprintf(file,"\n");

  if (file == stderr) 
  {
  for( i = *firstcol; i <= *lastcol; ++i )
    {
      if ( strncmp(&(head.ch[i].name[0]),"no_val",6) != 0)
	{
	  fprintf(file,"%d recs",head.ch[i].nelem) ;
	}
    }
   fprintf(file,"\n") ;
  }
}


/* -------------------------------------------------------------------------*/
void vision(file,firstrow,lastrow,firstcol,lastcol)
     int *firstrow , *lastrow , *firstcol , *lastcol ;
     FILE *file ;
{
  int i , j ;
  review(file, firstcol, lastcol) ;
  for ( i = *firstrow; i <= *lastrow ; ++i)
    {
      fprintf(file,"%5d",i) ;
      for ( j = *firstcol; j <= *lastcol; ++j)
	{
	  if(strncmp(&(head.ch[j].name[0]),"no_val",5) != 0)
	    {
	      fprintf(file,"  %.10f",darray[j][i]) ;
	    }
	}
      fprintf(file,"\n") ;
    }
}

/* ------------------------------------------------------------------------*/
void msg_vision(firstrow,lastrow,firstcol,lastcol)
     int *firstrow , *lastrow , *firstcol , *lastcol ;
{
  int i , j ;
  char tmp_msg[MSG_LENGTH];
  
  for ( i = *firstrow; i <= *lastrow ; ++i)
    {
      sprintf(msg, "%5d", i);
      for ( j = *firstcol; j <= *lastcol; ++j)
	{
	  if(strncmp(&(head.ch[j].name[0]),"no_val",5) != 0)
	    {
	      sprintf(tmp_msg, "  %.10f",darray[j][i]) ;
	      strcat(msg, tmp_msg);
	    }
	}
      strcat(msg, "\n");
      print_msg(msg);
    }
}


/* -------------------------------------------------------------------------*/
void p_vision(file,firstrow,lastrow,firstcol,lastcol)
     int *firstrow , *lastrow , *firstcol , *lastcol ;
     FILE *file ;
{
  int i , j ;
  p_review(file, firstcol, lastcol) ;
  for ( i = *firstrow; i <= *lastrow ; ++i)
    {
      fprintf(file,"%d",i) ;
      for ( j = *firstcol; j <= *lastcol; ++j)
	{
	  if(strncmp(&(head.ch[j].name[0]),"no_val",5) != 0)
	    {
	      fprintf(file,", %.9g",darray[j][i]) ;
	    }
	}
      fprintf(file,"\n") ;
    }
}

/* ------------------------------------------------------------------------*/
void msg_p_vision(firstrow,lastrow,firstcol,lastcol)
     int *firstrow , *lastrow , *firstcol , *lastcol ;
{
  int i , j ;
  char tmp_msg[MSG_LENGTH];

  for ( i = *firstrow; i <= *lastrow ; ++i)
    {
      sprintf(msg, "%d", i);
      for ( j = *firstcol; j <= *lastcol; ++j)
	{
	  if(strncmp(&(head.ch[j].name[0]),"no_val",5) != 0)
	    {
	      sprintf(tmp_msg, ", %.9g",darray[j][i]);
	      strcat(msg, tmp_msg);
	    }
	}
      strcat(msg, "\n") ;
      print_msg(msg);
    }
}


/*---------------------------------------------------------------------*/


int name(unknown, orig_col, arg_name, arg_unit)		/*returns 0 on error, 1 if successful*/
     int *unknown;
     int orig_col;
     char *arg_name, *arg_unit;     
{
  if( (*unknown) >= 1 && (*unknown) <= max_col-1)
    {
      /*if it's a new col increment nchan and give it nrec elements*/
      if(strncmp(head.ch[*unknown].name,"no_val",6) == 0)
	{
	  head.nchan += 1 ;
	  if (head.nrec == 0) head.nrec = 1000;
	  /*note this may get changed below */
	  head.ch[*unknown].nelem = head.nrec; 
	}
      
      strcpy(&head.ch[*unknown].name[0], arg_name);
      
      /*if user changed their mind (?) re-set nchan etc*/
      if(strncmp(head.ch[*unknown].name,"no_val",6) == 0)
	{
	  head.nchan -= 1 ;
	  strcpy(&(head.ch[*unknown].units[0]),"no_val");
	  strcpy(&(head.ch[*unknown].comment[0]),"none");
	  head.ch[*unknown].gain = 0;
	  head.ch[*unknown].nelem = 0;
/*
   set these in cmd_handler since it may be called by other procedures..
	  action = MAIN;
	  top();
*/
	  return 0;
	}
      else
	{
	  strcpy(&head.ch[*unknown].units[0], arg_unit);
	  head.ch[*unknown].nelem = head.ch[orig_col].nelem ;
	}

      return 1;
    }
  else
  {
	coe();
  	return 0;
  }
}
