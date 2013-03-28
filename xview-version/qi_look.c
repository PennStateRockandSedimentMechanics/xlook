/* 	Non-linear inverse routine using the levenberg-marquardt method and 
		svd

This version is set up to invert for the parameters of a 2 state variable 
rate and state model

last modified
	26/7/94 changed the way the L-M lambda parameter is modified. It now
	gets reduced only after 3 steps in the right direction.
	12/8/94 changed format of op_file to include param info at bottom
	14/8/94 changed to allow data table to be written automatically
 14.2.2010, cjm: change so that we don't model mu_o, but instead treat it as a known value

*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>

#include <config.h>
#include <global.h>
#include <qi_look.h>
#include <messages.h>

#define CLOSE(a,b) (fabs((a-(int)a) - b) < 0.01)

extern char msg[MSG_LENGTH];


FILE	*op_file, *data_table;


int exec_qi(converg_tol, lambda, wc, rsp)
     double converg_tol, lambda, wc;
     struct rs_parameters *rsp;	   /*rate state stuff, defined in global.h*/
{
  time_t  tp;
  char  buf[80], mess1[200], final_mess[1000];
  char	*cp, tmp_msg[MSG_LENGTH];
  char	weight_vec_only=FALSE;
  int     i, j, k;
  int	iter, iter_max, ndata, ma, temp; 
  int	cont = TRUE;
  int	neglig;
  int	get_mu_at_x_t();
  int	msvdfit();
  double	*wv, temp_d;
  double	**u,**v;
  double	a[7],w[7],a_unscaled[7], std_a[7], std_a_unscaled[7];
  double	chisq, prev_chisq, lambda_max;
  double	sqrt(), pow();
  double	calc_chisq();
  double	**dm();
  void	free_dm();
  void	set_rs_parameters();
  
  ma=5;				/*set defaults*/
  rsp->one_sv_flag=FALSE;
  
  if(rsp->dc2 < 0)
    {
      ma=3;
      rsp->one_sv_flag=TRUE;
    }
  ndata=rsp->last_row - rsp->first_row+1;
  
  rsp->vs_row++;		/* to account for fortran c, here vs_row is relative to first row, which = 1 */
  rsp->weight_row++;
  rsp->end_weight_row++;	
  for(i=1;i<=rsp->n_vsteps;i++)	 /*add one to account for "fortran" c, don't do 0*/
    rsp->vs_row_list[i]++;
  
  /*allocate space*/
  /*a = (double *)malloc((ma+1)*sizeof(double));
  w = (double *)malloc((ma+1)*sizeof(double));
  std_a = (double *)malloc((ma+1)*sizeof(double));
  std_a_unscaled = (double *)malloc((ma+1)*sizeof(double));
  a_unscaled = (double *)malloc((ma+1)*sizeof(double));*/

  wv = (double *)malloc((ndata+1)*sizeof(double)); /*weighting vector*/

  u = dm(ndata,ma);
  v = dm(ma,ma);
  
  if(rsp->one_sv_flag)			/*to handle 1sv case*/
    {
      rsp->b2 = a[5] = 0.0;
      rsp->dc2 = a[6] = 1e30;
    }

  if(lambda >= 0 || DEBUG)		/*if less than 0, don't write log files, unless debugging*/
    {
      /* write info to log  */
      strcpy(buf,"qi_log_");
      strcat(buf,head.title);
      if(rsp->op_file_flag & 0x10)	/*sixteens bit means use separate file for each step*/
	{
	  sprintf(mess1,"%d",rsp->vs_row-1);
	  strcat(buf,"_");
	  strcat(buf,mess1);
	}
      /*twos bit on means create rather than append*/
      if(rsp->op_file_flag & 0x2)	
 	op_file = fopen(buf, "w");      /* write to default file */
      else
 	op_file = fopen(buf, "a");      /* append to default file */
      
      if(rsp->op_file_flag & 0x08)     /*eights bit on means write data table*/
	{
	  data_table = fopen("data_table", "a");   /* append to default file */
	  if( (int)ftell(data_table) == 0 )	/*write header if first line*/
	    fprintf(data_table,"%9s,%9s,%9s,%9s,%8s,%8s,%9s,%9s,%8s,%8s,%9s,%s,%3s,%s,%6s,%10s\n","a","std a","b1","std b1","Dc1","std Dc1","b2","std b2","Dc2","std Dc2","a-b","lin_term","wc","law","step","run date");
	}
      fprintf(op_file,"\n**************************************************\n");
      time(&tp);
      fprintf(op_file,"%d state variable fit for Exp: %s \t\t Date: %s", (rsp->one_sv_flag) ? 1: 2, head.title, ctime(&tp));
      fprintf(op_file,"result to column %d of look file\n",rsp->mu_fit_col);
      fprintf(op_file,"recs fit: %d to %d, vs_row=%d, fric_lin_term=%g, v_init. = %g, v_f = %g, k=%g,\n\tconvergence_tol=%g,  L-M_lambda=%g\n", rsp->first_row, rsp->last_row, rsp->vs_row-1, rsp->lin_term, rsp->vo, rsp->vf, rsp->stiff, converg_tol, lambda);
      
      if(rsp->n_vsteps >1)
	{
	  fprintf(op_file,"Fitting multiple velocity steps,");
	  for(i=1;i<rsp->n_vsteps;i++)		 /*1st new vel in i=1, 3rd v overall*/
	    fprintf(op_file,"\tvs_row=%d v=%g%c",rsp->vs_row_list[i]-1,rsp->vel_list[i],(i==rsp->n_vsteps-1) ? ',': '\n');	
	}
      
      if(wc > 0)
 	sprintf(mess1,"Weighting with wv[i] = (ndata/i)^%g, first weighted row (i=1) is %d\n",wc, rsp->weight_row-1);
      
      else if(wc < 0)
 	sprintf(mess1,"Weighting data from row %d to row %d by a factor of %g\n",rsp->weight_row-1, rsp->end_weight_row-1,-wc);
      
      else
 	sprintf(mess1,"no weighting\n");
      
      fprintf(op_file,mess1);
      sprintf(msg, "%s\n", mess1);
      print_msg(msg);
      
      strncpy(final_mess,mess1,strlen(mess1));		/*save */
      switch(rsp->law)
	{
        case 'o':
	  sprintf(buf,"Ruina law -using original (old) code");
	  break;
        case 'r':
	  sprintf(buf,"Ruina-Dieterich, slip law");
	  break;
        case 'd':
	  sprintf(buf,"Dieterich-Ruina, slowness law");
	  break;
        case 'p':
	  sprintf(buf,"Perrin-Rice law");
	  break;
        case 'm':
	  sprintf(buf,"m law, my-law");
	  break;
        case 'j':                                    
	  /*check this later, just a guess now 15/7/94 	 */
	  sprintf(buf,"Rice law");
	  break;
	}
      strcat(final_mess,buf);		/*save */

      if(rsp->op_file_flag & 0x20)	/*32's bit means were modeling time vs mu*/
      	strcpy(mess1,"Modeling time vs mu\nInverting for the ");
      else
      	strcpy(mess1,"Inverting for the ");

      strcat(mess1,buf);
      strcat(mess1,"\n");
      fprintf(op_file,mess1);
      sprintf(msg, "%s\n", mess1);
      print_msg(msg);
      
      strcpy(msg, "");
      if(rsp->one_sv_flag)
 	sprintf(msg, "Doing one sv fit,\t");
      if(rsp->lin_term)
      {
 	sprintf(buf, "\tadding linear term to friction law: %g*dx\n",rsp->lin_term);
 	strcat(msg, buf);
      }
      else
 	strcat(msg, "\n");
      print_msg(msg);
      
      fprintf(op_file,"Initial guesses for parameters, with mu_o=%g:\na= %g,\tb1= %g,\tdc1= %g,\tb2= %g,\tdc2= %g\n", rsp->muo, rsp->a, rsp->b1, rsp->dc1, rsp->b2, rsp->dc2);
    } 

  else
    op_file=stderr; /*redirect op to stderr if no log file used*/
  
/* 14.2.2010, cjm: change so that we don't model mu_o, but instead treat it as a known value*/
  /* pre 14.2 change: a[1] = mu_o, a[2]=a, a[3]=b1, a[4]=log(dc1), a[5]=b2, a[6]=log(dc2) */
  /* post 14.2 change: a[1]=a, a[2]=b1, a[3]=log(dc1), a[4]=b2, a[5]=log(dc2) */
  a[1] = rsp->a;
  a[2] = rsp->b1;
  a[3] = rsp->dc1;
  a[4] = rsp->b2;
  a[5] = rsp->dc2;

  /*solve for log of Dc, following RWIM*/
  a[3] = log(a[3]);
  if(!rsp->one_sv_flag)
    a[5] = log(a[5]);
  
  /* gen func. for setting a-b, mu_f etc */
  set_rs_parameters(rsp,a,NULL);		
  
  if(lambda < 0)
    {
      if( get_mu_at_x_t(rsp->disp_data,rsp->model_mu,ndata,rsp) == -1)
	{
	  sprintf(msg, "error from get_mu_at_x_t \n");
	  print_msg(msg);
	}
      
      for(i=1;i<=ndata;i++)	
	wv[i]= 1.0;
      chisq = prev_chisq = calc_chisq(a, rsp->disp_data, rsp->mu_data, wv, ma, ndata, rsp);
      sprintf(msg, "chisq=%g\n",chisq);
      print_msg(msg);

      free(wv);
      free_dm(u,ndata,ma);
      free_dm(v,ma,ma);

      return(1);
    }

  /*weighting*/
  if( CLOSE(wc,0.1234) )	/*so I can see weighting vector*/
    {
      wc -=0.1234;
      weight_vec_only=TRUE;
    }
  
  if(wc < 0)					/*weight between two points*/
    {
      for(i=1; i<(rsp->weight_row - rsp->first_row); ++i)
	wv[i] = 1.0;
      for(i=rsp->weight_row - rsp->first_row; i<=rsp->end_weight_row - rsp->first_row; ++i)	
	wv[i] = -wc;
      for(i=rsp->end_weight_row - rsp->first_row +1; i<=ndata; ++i)
	wv[i] = 1.0;
      wc *= -1.0;
    }
  
  else				     /*weight with decreasing function*/
    {	 /*for mul. v steps, default is to weight with std wv between steps*/
      /*first part*/
      for(i=1;i< rsp->vs_row - rsp->first_row; ++i)
	wv[i] = 1.0;
      
      for(k=1;k<=rsp->n_vsteps;k++)
	{				/*temp will = ndata if n_vsteps = 1*/
	  temp = rsp->vs_row_list[k] - rsp->vs_row_list[k-1] +1;
	  for(j=1, i=rsp->vs_row_list[k-1] - rsp->first_row; 
	      i <= rsp->vs_row_list[k] - rsp->first_row; ++i)
	    wv[i] = pow((double)((double)temp/(double)j++),wc);
	}
    }

  if( weight_vec_only )	/*so I can see weighting vector*/
    {
      for(i=1;i<=ndata;i++)	
	rsp->model_mu[i] = wv[i];
      sprintf(msg, "\n\nweighting vector in mod_mu col, wc=%g\n\n",wc);
      print_msg(msg);
      return(2);
    }

  /*initialize*/
  chisq = prev_chisq = calc_chisq(a, rsp->disp_data, rsp->mu_data, wv, ma, ndata, rsp);
  if(chisq == -1)
    {
      fclose(op_file);
      return(-1);
    }

  fprintf(op_file,"chisq for initial model is: %f\n",chisq);
  
  lambda *= -1.0;				/*for initialization*/
  if(msvdfit(rsp->disp_data, rsp->mu_data, wv, ndata, a, ma, u, v, w, &chisq, rsp, &lambda)==-1)
    {
      sprintf(msg, "problem in msvdfit: initialization%c\n",BELL);
      print_msg(msg);
      fclose(op_file);
      return(-1); 
    }
  
  
  lambda_max = 5000; iter_max=1000;
  iter=neglig=0;
  while( cont )				/*main loop for iterations*/
    {
      if(msvdfit(rsp->disp_data, rsp->mu_data, wv, ndata, a, ma, u, v, w, &chisq, rsp, &lambda)==-1)
	{
	  sprintf(msg, "problem in msvdfit: iteration = %d chisq=%g%c\n",iter,chisq,BELL);
	  print_msg(msg);
	  fclose(op_file);
	  return(-1); 
	}

      /* check for convergence */
      /*don't count if it went up*/
      if((chisq < prev_chisq) && (1.0-chisq/prev_chisq < converg_tol)) 
	neglig++;

      if(neglig > 1)			/* stop on the 2nd time */
	cont = FALSE;		
      iter++;

      /*uniform variance is chisq/(ndata-ms) */
      /*ones bit of op flag is verbose option*/
      if( (iter % 2)==0 || (rsp->op_file_flag & 0x1))
	{
	  fprintf(op_file,"new params for a\n");
	  for(i=1;i<=ma; ++i)
	    fprintf(op_file," %f%c",((i==3 || i==5) ? exp(a[i]) : a[i]), (i % ma)  ? '\t' : '\n');
	  fprintf(op_file,"Iteration: %d \tchisq=%f\tdel_chisq=%g\t1-chisq/prev_chisq=%g\tL-M lambda=%g\n", iter,chisq,(chisq-prev_chisq),1-chisq/prev_chisq,lambda);
	}

      sprintf(msg, "new params for a\n");
      print_msg(msg);
      msg[0] = '\0';
      for(i=1;i<=ma; ++i)
	{
	  sprintf(tmp_msg, " %f%c",((i==3 || i==5) ? exp(a[i]) : a[i]), (i % ma)  ? '\t' : '\n');
	  strcat(msg, tmp_msg);
	}
      print_msg(msg);

      sprintf(msg, "Iteration: %d \tchisq=%f\tdel_chisq=%g\t1-chisq/prev_chisq=%g\tL-M lambda=%g\n", iter,chisq,(chisq-prev_chisq),1-chisq/prev_chisq,lambda);
      print_msg(msg);
      
      prev_chisq = chisq;		/* keep previous error */

      if( (lambda > lambda_max || iter > iter_max) && (rsp->op_file_flag & 0x4) )	/*fours bit on means interactive mode */
	{
	  sprintf(msg, "\n\tlambda too large or too many iterations. Try again with new maximum values for both.\n");
	  /* scanf("%lf %d",&lambda_max, &iter_max); */
	  /* changed: just exit and ask the user to retry. */
	  print_msg(msg);
	  return(-1);
	}
    }					/*end of main iteration loop*/

  /* do undamped, non-weighted L-M and free work space on final call */
  lambda = 0.0; 	
  if(wc)
    for(i=1;i<=ndata;++i)	/*remove weighting if nec. */
      wv[i]=1.0;
  /*covar is returned in v*/
  if(msvdfit(rsp->disp_data, rsp->mu_data, wv, ndata, a, ma, u, v, w, &chisq, rsp, &lambda)==-1)
    {
      sprintf(msg, "problem in msvdfit: final call to set covar%c\n",BELL);
      print_msg(msg);
      fclose(op_file);
      return(-1); 
    }

  /* mu for best fit params */
  if( get_mu_at_x_t(rsp->disp_data,rsp->model_mu,ndata,rsp) == -1)
    {
      fclose(op_file);
      return(-1);	/*signal problem to caller*/
    }

  /*variance = chisq/(ndata-ma)*/
  chisq = calc_chisq(a, rsp->disp_data, rsp->mu_data, wv, ma, ndata, rsp);
  if(chisq == -1)
    {
      sprintf(msg, "problem from chisq calc: final call after getting covar%c\n",BELL);
      print_msg(msg);
      fclose(op_file);
      return(-1);
    }

  fprintf(op_file,"std covar:\n");
  for(i=1;i<=ma;++i)
    for(j=1; j<=i; ++j)
      fprintf(op_file,"%11.6g%c",v[i][j],((i==j) ? '\n' : '\t'));

  for(i=1;i<=ma;++i)	     /*calculate scaled covariance : covar*variance */
    {
      for(j=1; j<=i; ++j)
	v[i][j] = v[i][j]*(chisq/(ndata-ma));

      /*std_a is std dev of log Dc's */
      std_a_unscaled[i] = std_a[i] = sqrt(v[i][i]); 

      a_unscaled[i] = a[i];	/*unscaled model vector*/	
    }

  fprintf(op_file,"covar*variance (for logDc's) is:\n");
  for(i=1;i<=ma;++i)
    for(j=1; j<=i; ++j)
      fprintf(op_file,"%11.6g%c",v[i][j],((i==j) ? '\n' : '\t'));

  for(i=1;i<=ma;++i)			/*calculate correlation coef*/
    for(j=1; j<=i; ++j)
      u[i][j] = v[i][j]/(sqrt(v[i][i])*sqrt(v[j][j]));

  fprintf(op_file,"correlation coefficient matrix:\n");
  for(i=1;i<=ma;++i)			/*print correlation coef*/
    for(j=1; j<=i; ++j)
      fprintf(op_file,"%11.6g%c",u[i][j],((i==j) ? '\n' : '\t'));

  a_unscaled[3] = exp(a[3]); 			   /*unscaled model params */ 
  std_a_unscaled[3] = exp(a[3]+std_a[3])-exp(a[3]);	/*unscaled std. dev. of model params */
  if(!rsp->one_sv_flag) 
    {
      a_unscaled[5] = exp(a[5]);
      std_a_unscaled[5] = exp(a[5]+std_a[5])-exp(a[5]);  	/*treat +,- separately*/
    }
  
  fprintf(op_file,"\nunweighted Chi square error is %g\t\tvariance=chisq/dof = %g\n\n",chisq,chisq/(ndata-ma));
  
  /*best fit values*/
  sprintf(msg, "\nparameters (best fit) which minimize chi_sq = |A a - b|^2\nNote that mu_o is assumed to be known; it is not modeled\n");
  print_msg(msg);
  fprintf(op_file,"\nparameters (best fit) which minimize chi_sq = |A a - b|^2\nNote that mu_o is assumed to be known; it is not modeled\n");
  
  if(rsp->one_sv_flag)
    {
      sprintf(msg, "mu_o      a         b1        dc1       a-b\n");
      print_msg(msg);
      
      fprintf(op_file,"mu_o      a         b1        dc1       a-b\n");
      sprintf(msg, "%8.6f %9.6f %9.6f %8.3f %9.6f\n",rsp->muo,a_unscaled[1],a_unscaled[2],a_unscaled[3],rsp->amb);
      print_msg(msg);

      fprintf(op_file,"%8.6f %9.6f %9.6f %8.3f %9.6f\n",rsp->muo,a_unscaled[1],a_unscaled[2],a_unscaled[3],rsp->amb);
    }
  else
    {
      sprintf(msg, "mu_o      a         b1        dc1      b2        dc2      a-b\n");
      print_msg(msg);
      
      fprintf(op_file,"mu_o      a         b1        dc1      b2        dc2      a-b\n");
      sprintf(msg, "%8.6f %9.6f %9.6f %8.3f %9.6f %8.3f %9.6f\n",rsp->muo,a_unscaled[1],a_unscaled[2],a_unscaled[3],a_unscaled[4],a_unscaled[5],rsp->amb);
      print_msg(msg);
      
      fprintf(op_file,"%8.6f %9.6f %9.6f %8.3f %9.6f %8.3f %9.6f\n",rsp->muo,a_unscaled[1],a_unscaled[2],a_unscaled[3],a_unscaled[4],a_unscaled[5],rsp->amb);
    }

  /*std deviations*/
  sprintf(msg, "std_dev's  (real Dc's)\n");
  print_msg(msg);
  fprintf(op_file,"std_dev's  (real Dc's)\n");
  if(rsp->one_sv_flag) 
    {
      sprintf(msg, "         %9.6f %9.6f %8.3f\n",std_a_unscaled[1],std_a_unscaled[2],std_a_unscaled[3]);
      print_msg(msg);
      sprintf(msg, "%37.3f\n",exp(a[3]-std_a[3])-exp(a[3]));
      print_msg(msg);
      fprintf(op_file,"         %9.6f %9.6f %8.3f\n",std_a_unscaled[1],std_a_unscaled[2],std_a_unscaled[3]);
      fprintf(op_file,"%37.3f\n",exp(a[3]-std_a[3])-exp(a[3]));
    }
  else
    {
      sprintf(msg, "         %9.6f %9.6f %8.3f %9.6f %8.3f\n",std_a_unscaled[1],std_a_unscaled[2],std_a_unscaled[3],std_a_unscaled[4],std_a_unscaled[5]);
      print_msg(msg);
      sprintf(msg, "%37.3f %18.3f\n",exp(a[3]-std_a[3])-exp(a[3]),exp(a[5]-std_a[5])-exp(a[5]));
      print_msg(msg);
      fprintf(op_file,"         %9.6f %9.6f %8.3f %9.6f %8.3f\n",std_a_unscaled[1],std_a_unscaled[2],std_a_unscaled[3],std_a_unscaled[4],std_a_unscaled[5]);
      fprintf(op_file,"%37.3f %18.3f\n",exp(a[3]-std_a[3])-exp(a[3]),exp(a[5]-std_a[5])-exp(a[5]));
    }

  sprintf(buf,";   fric_lin_term=%g \n", rsp->lin_term);
  strcat(final_mess,buf);
  if(rsp->op_file_flag & 0x20)
  {
  	sprintf(buf,"  Inversion is for time vs. mu\n");
  	strcat(final_mess,buf);
  }
  fprintf(op_file,"%d state variable fit for %s; recs fit: %d to %d, vs_row=%d\n", (rsp->one_sv_flag) ? 1: 2, head.title,rsp->first_row, rsp->last_row, rsp->vs_row-1);
  fprintf(op_file,final_mess);

  if(rsp->op_file_flag & 0x08) /*write to data table*/
    { 
      if( !rsp->one_sv_flag && (a_unscaled[5] < a_unscaled[3]) ) 	/*force dc1 < dc2*/
	{
	  temp_d=a_unscaled[3];				/*dc's*/
	  a_unscaled[3] = a_unscaled[5];
	  a_unscaled[5] = temp_d;
	  temp_d=a_unscaled[2];				/*b's*/
	  a_unscaled[2] = a_unscaled[4];
	  a_unscaled[4] = temp_d;
	  temp_d=std_a_unscaled[3];			/*dc's std*/
	  std_a_unscaled[3] = std_a_unscaled[5];
	  std_a_unscaled[5] = temp_d;
	  temp_d=std_a_unscaled[2];			/*b's std*/
	  std_a_unscaled[2] = std_a_unscaled[4];
	  std_a_unscaled[4] = temp_d;
	}
      strcpy(buf,head.title);
      sprintf(mess1,"%d",rsp->vs_row-1);
      strcat(buf,"_");
      strcat(buf,mess1);
      cp = ctime(&tp);
      strcpy(mess1,cp);
      for(i=0;i<strlen(mess1);i++)	/*remove spaces from date*/
	if(mess1[i] == ' ')
	  mess1[i] = '_';


      if(rsp->op_file_flag & 0x08)    /*eights bit on means write data table*/
	{
	  if(rsp->one_sv_flag) 
	    fprintf(data_table,"%8.6f,%8.6f,%9.6f,%9.6f,%9.6f,%9.6f,%9s,%9s,%8s,%8s,%9.6f,%11g,%3g,%c,%s,%s",a_unscaled[1],std_a_unscaled[1],a_unscaled[2],std_a_unscaled[2],a_unscaled[3],std_a_unscaled[3]," "," "," "," ",rsp->amb,rsp->lin_term,wc,rsp->law,buf,mess1);
	  else 
	    fprintf(data_table,"%8.6f,%8.6f,%9.6f,%9.6f,%9.6f,%9.6f,%8.4g,%8.4g,%9.6f,%9.6f,%9.6f,%11g,%3g,%c,%s,%s",a_unscaled[1],std_a_unscaled[1],a_unscaled[2],std_a_unscaled[2],a_unscaled[3],std_a_unscaled[3],a_unscaled[4],std_a_unscaled[4],a_unscaled[5],std_a_unscaled[5],rsp->amb,rsp->lin_term,wc,rsp->law,buf,mess1);
	  fclose(data_table);
	}
    }
  
  /*free(a);*/
  /*free(w);*/
  /*free(std_a);*/
  /*free(std_a_unscaled);*/
  /*free(a_unscaled);*/
  free(wv);
  free_dm(u,ndata,ma);
  free_dm(v,ma,ma);
  fclose(op_file);
  return(0);
}

#undef CLOSE


/****************************************************************************/
/*  calculation of the chisq err for a given vector of model parameters a[i] 

   pre 14.2 change: a[1] = mu_o, a[2]=a, a[3]=b1, a[4]=log(dc1), a[5]=b2, a[6]=log(dc2) 
   post 14.2 change: a[1]=a, a[2]=b1, a[3]=log(dc1), a[4]=b2, a[5]=log(dc2) 

	 wv is a weighting vector, set in main() 
*/
double calc_chisq(a, x, y, wv, ma, ndata, rsp)
     double 	a[], x[], y[], wv[];
     int	ma, ndata;
     struct rs_parameters *rsp;
{
  
  int i;
  double dy, chisq;
  
  /*mod_mu = (double *) malloc((ndata+1)*sizeof(double));*/
  /*mod_mu = (double *) calloc((unsigned)ndata+1,sizeof(double));*/

  chisq=0.0;
  
  /*this function supplies mu at the displacements in x for the parameters in a*/

  /*if(get_mu_at_x_t(x,mod_mu,ndata,rsp)==-1)*/
  if(get_mu_at_x_t(x,rsp->model_mu,ndata,rsp)==-1)
    return(-1);	/*signal problem to caller*/
  
  for(i=1;i<=ndata;i++)
    {
      /*dy = y[i]-mod_mu[i];*/
      dy = y[i] - rsp->model_mu[i];
       /*chisq  += fabs(dy)*wv[i];, tried this, but it looks to give stddev's that are much too large*/
      chisq  += dy*dy*wv[i];
    }

  /*free(mod_mu);*/
  
  return(chisq);
}


/**********************************************************************/

double **dm(row,col)
     int row,col;
{ 
  
  int i; 
  double **m;
  
  m = (double**)malloc((row+1)*sizeof(double*));
  /*m = (double**)calloc((unsigned)row+1,sizeof(double*));*/
  for(i=0;i<=row;++i)
    {
      m[i] = (double*)malloc((col+1)*sizeof(double));
      /*m[i] = (double*)calloc((unsigned)col+1,sizeof(double));*/
      if(!m[i])
	{
	  sprintf(msg, "dm(): allocation error, memory full? \n");
	  print_msg(msg);
	  return NULL;
	}
    }
  return m;
}


/**********************************************************************/

void free_dm(m,row,col)
     double **m;
     int row,col;
{ 
  
  int i; 
  
  for(i=0;i<=row;++i)
    free(m[i]);
  free(m);
}


/**********************************************************************/
/*Below are functions for the svd and lm solution to 
  the non-linear inverse problem. Several aspects of the approach
  are based on the method of Reinen and Weeks 1993, JGR 98, 15937-15950.
  */

/*   pre 14.2 change: a[1] = mu_o, a[2]=a, a[3]=b1, a[4]=log(dc1), a[5]=b2, a[6]=log(dc2) 
   post 14.2 change: a[1]=a, a[2]=b1, a[3]=log(dc1), a[4]=b2, a[5]=log(dc2) */

/*
   da is the change in model parameters used to calculated the derivative 
   a_corr is the correction to the model parameters a based on the linearized solution 
   to the inverse problem.
   */
#define DA_TOL 1.0e-10	   /*probably no point in making it smaller than this*/
#define DELTA 0.01

int msvdfit(x,y,wv,ndata,a,ma,u,v,w,chisq,rsp,lambda)
     double x[],y[],wv[],a[],**u,**v,w[],*chisq,*lambda;
     int ndata,ma;
     struct rs_parameters *rsp;
{
  char  covar_err=0,tmp_str[4096],err_str[4096];
  int i, j, k;
  int do_lm_reduce;
  int get_mu_at_x_t();
  static double *b, **alpha, *beta, **u_norm, **alpha_norm, **v_tmp, **alpha_inv;
  static double *jac_norm, *a_corr, *a2, *atry, *da, ochisq, *mod_mu, *mod_mu2;
  double calc_chisq(), **dm(), sqrt(), exp();
  int msvdcmp();
  void msvbksb();
  void set_rs_parameters();
  void free_dm(), zero_sv();

  if(*lambda<0)			/*for initialization*/
    {
      do_lm_reduce=0;
      *lambda *= -1;		/* change sign */
      alpha = dm(ma,ma);
      alpha_norm = dm(ma,ma);
      alpha_inv = dm(ma,ma);
      u_norm = dm(ndata,ma);
      v_tmp = dm(ma,ma); 
      beta = (double *) malloc((ma+1)*sizeof(double));
      b = (double *) malloc((ndata+1)*sizeof(double));
      mod_mu = (double *) malloc((ndata+1)*sizeof(double));
      mod_mu2 = (double *) malloc((ndata+1)*sizeof(double));
      jac_norm = (double *) malloc((ma+1)*sizeof(double));
      a2 = (double *) malloc((ma+1)*sizeof(double));
      da = (double *) malloc((ma+1)*sizeof(double)); /* for derivative step size */
      a_corr = (double *) malloc((ma+1)*sizeof(double));  /*corrections to current a*/
      atry = (double *) malloc((ma+1)*sizeof(double));

      /*beta = (double *) calloc((unsigned)ma+1,sizeof(double));
      b = (double *) calloc((unsigned)ndata+1,sizeof(double));
      mod_mu = (double *) calloc((unsigned)ndata+1,sizeof(double));
      mod_mu2 = (double *) calloc((unsigned)ndata+1,sizeof(double));
      jac_norm = (double *) calloc((unsigned)ma+1,sizeof(double));
      a2 = (double *) calloc((unsigned)ma+1,sizeof(double));
      da = (double *) calloc((unsigned)ma+1,sizeof(double));
      a_corr = (double *) calloc((unsigned)ma+1,sizeof(double));
      atry = (double *) calloc((unsigned)ma+1,sizeof(double));*/

      for(k=1;k<=ma;++k)
	a_corr[k]= (k==3 || k==5) ? 10*exp(a[k]) : 10*a[k];	/*make 10x a so that da starts with 10% of a*/
      return(0);								/* use real dc's, which can't be zero */
    }

  for(k=1;k<=ma;k++)  /* derivative step size is 0.01 of last correction to model vector*/
    da[k] = (fabs(a_corr[k]*DELTA) < DA_TOL) ? DA_TOL : a_corr[k]*DELTA;
  
  ochisq=*chisq;			/*chisq for current a is sent here*/
  
  /*calculate the Jacobian matrix of partial */
  /*derivatives: dy[i]/da[j]; i:1,ndata,j:1,ma*/
  if(get_mu_at_x_t(x,mod_mu,ndata,rsp) == -1)
    return(-1);

  for(j=1;j<=ma;j++)
    {
      for(k=1;k<=ma;k++)		/*set up for derivative*/
	a2[k] = (k==j) ? a[k]+da[k] : a[k];
      set_rs_parameters(rsp,a2,da);	/* set rs params */
      /*get mu for new a vector*/
      if(get_mu_at_x_t(x,mod_mu2,ndata,rsp) == -1)
	return(-1);
      for(i=1;i<=ndata;i++)			/* calc. jacobian matrix */
	u[i][j] = (mod_mu2[i]-mod_mu[i])/da[j];	/*do weighting with alpha*/	
    }

  /*this is vector of residuals for a*/
  for (i=1;i<=ndata;i++) 
    b[i]=(y[i]-mod_mu[i]); 		/*unweighted at this point*/

  for(j=1;j<=ma;++j)			/*normalize jacobian -follow RWIM*/
    {
      jac_norm[j]=0.0;
      for(i=1;i<=ndata;i++)
	jac_norm[j] += u[i][j]*u[i][j];
      jac_norm[j] = sqrt(jac_norm[j]);
      for(i=1;i<=ndata;i++)
	u_norm[i][j] = u[i][j]/jac_norm[j];  /* u_norm is normalized jacobian */
    }

  for(i=1;i<=ma;++i)		 	/*alpha_norm = (u_normT wvT u_norm) */
    {
      for(j=1;j<=ma;++j)
	{
	  alpha[i][j]=alpha_norm[i][j]=0;
	  for(k=1;k<=ndata;k++)
	    {
	      alpha_norm[i][j] += u_norm[k][i]*wv[k]*u_norm[k][j];	
	      alpha[i][j] += u[k][i]*wv[k]*u[k][j];	
	    }
	}
      alpha_norm[i][i] += *lambda;		/*Levenberg-Marquardt*/
      alpha[i][i] += *lambda;		/*Levenberg-Marquardt*/
    }
  
  for(j=1;j<=ma;++j) 			/*calculate beta = (uT wvT b) */
    {
      beta[j]=0;
      for(k=1;k<=ndata;k++)
	beta[j] += u_norm[k][j]*wv[k]*b[k];	/*beta uses normalized u*/
    }

/*
   sprintf(msg, "alpha_norm u:\n");
   print_msg(msg);

   for(i=1;i<=ma; ++i)
   { 
   for(j=1;j<=ma; ++j)
   sprintf(msg, "%f, ",alpha_norm[i][j]);
   sprintf(msg, "\n");
   }
   sprintf(msg, "\n");

   sprintf(msg, "da for deriv\n");
   for(i=1;i<=ma; ++i)
   sprintf(msg, " %f%c",da[i], ( (i % ma)  ? '\t' : '\n'));
*/	


	
  /* svd of alpha_norm into U=alpha_norm, w, v */
  if (msvdcmp(alpha_norm,ma,ma,w,v) == -1)
	return(-1);

  zero_sv(w,ma,1e-5,(rsp->op_file_flag & 0x1));	    /* zero singular values */
  
  if(*lambda==0)			/* signal convergence, finish up, clean up */
    {
      for(i=1;i<=ma;++i)	/*eigenvalues, account for svd zeros */
	w[i] = (w[i]) ? (1.0/w[i]) : 0;		/* use 1/w instead of 1/w^2 to account for normalization */

      /*calculated covariance, put it in alpha_norm */
      for(j=1;j<=ma;j++)	/* use eq'n 14.3.20 from Num. Rec.*/
	{
	  for(k=1;k<=ma;k++)
	    {
	      alpha_norm[j][k]=0;		
	      for(i=1;i<=ma;i++)
		alpha_norm[j][k] += v[j][i]*v[k][i]*w[i];
	      /*remove normalization*/
	      alpha_norm[j][k] /= (jac_norm[j]*jac_norm[k]);
	    }
	}

      for(i=1;i<=ma;++i)
	for(j=1;j<=ma;j++)
	  v[i][j] = alpha_norm[i][j];	/*return covar in v */

      /* get inverse of alpha (un norm) to check covar */
      if( msvdcmp(alpha,ma,ma,w,v_tmp) == -1) 
	return(-1);
      
      zero_sv(w,ma,1e-8,(rsp->op_file_flag & 0x1));			/* zero singular values */
      
      for(i=1;i<=ma;i++)
	w[i] = (w[i]) ? (1/w[i]) : 0;	/* use 1/w */
      
      for(i=1;i<=ma;i++)
	{
	  for(j=1;j<=ma;j++)
	    {
	      alpha_inv[i][j]=0;
	      for(k=1;k<=ma;k++)
		alpha_inv[i][j] += v_tmp[i][k]*w[k]*alpha[j][k];
	    }
	}

      strcpy(err_str,"row,col\t(1-alpha_inv/svd_covar)\n");
      k=0;
      for(i=1;i<=ma;i++)		/* compare covar from alpha_inv and covar from V*V/w */
	for(j=1;j<=ma;j++)
	  if( fabs(1-(alpha_inv[i][j]/v[i][j])) > 1e-3)
	    {
	      covar_err = 1;
	      k++;
	      sprintf(tmp_str,"%d,%d:%16.6g%c",i,j,1-alpha_inv[i][j]/v[i][j], (k % 3) ?'\t' : '\n');
	      strcat(err_str,tmp_str);
	    }
      
      if(covar_err && (rsp->op_file_flag & 0x1))	/*verbose option*/
	{
	  sprintf(msg, "%cCovar from inverse of unnormalized alpha and eigenvector/eigenvalues disagree\nCheck op_file for a list\n",BELL);
	  print_msg(msg);
	  
	  fprintf(op_file,"Covar from inverse of unnormalized alpha and eigenvector/eigenvalues disagree\nCheck the following entries\n");
	  fprintf(op_file,"%s\n\n",err_str);
	}

      free_dm(alpha,ma,ma);
      free_dm(alpha_norm,ma,ma);
      free_dm(alpha_inv,ma,ma);
      free_dm(u_norm,ndata,ma);
      free_dm(v_tmp,ma,ma);
      free(beta);
      free(b);
      free(mod_mu);
      free(mod_mu2);
      free(jac_norm);
      free(a2);
      free(da);
      free(a_corr);
      free(atry);
      return(0);			/*stop here*/
    }

  for(i=1;i<=ma;++i)		   /*save old values in case new ones are ng*/
    a2[i]=a_corr[i];

  msvbksb(alpha_norm,w,v,ma,ma,beta,a_corr);       /*a_corr will contain the corrections to a*/
  
  for(i=1;i<=ma;++i)			/*set up to see if these are better*/
    {
      a_corr[i] /=jac_norm[i];	 /* remove normalization*/
      atry[i] = a[i]+a_corr[i];
    }
	

  set_rs_parameters(rsp,atry,a_corr);		/* re-set params */
  /*check if better*/
  *chisq =  calc_chisq(atry, x, y, wv, ma, ndata, rsp);
  if(*chisq ==  -1)
    return(-1);

  if(*chisq < ochisq)
    {
      do_lm_reduce++;
      if(do_lm_reduce > 2)
	{
	  *lambda *= 0.1;			/*reduce L-M factor*/
	  do_lm_reduce=0;
	}
      for(i=1;i<=ma;++i)
	a[i] = atry[i];		/*save new parameters */
    }
  else
    {
      do_lm_reduce=0;
      fprintf(op_file,"in svdfit: chisq worse for new guesses, prev_chisq=%f, new chisq=%f\n",ochisq,*chisq); 
      *lambda *= 10.0;		/*increase L-M factor*/
      *chisq = ochisq;		/*old values were better */
      for(i=1;i<=ma;++i)		
	a_corr[i]=a2[i];
    }

  set_rs_parameters(rsp,a,a_corr);	/* re-set params */

     return 0;
}
#undef DA_TOL 
#undef DELTA 



/*************************************************************************/

/* general function to set a-b and mu_f. 
   This should be called whenever a, b1, b2 is changed, such as by inversion routine.
   Callers should send null if they don't want to update from a[] or da[] 
   
   note that a[3]=dc1 and a[5]=dc2 are assumed to be log(dc1) and log(dc2)
*/

/* 14.2.2010, cjm: change so that we don't model mu_o, but instead treat it as a known value*/
  /* pre 14.2 change: a[1] = mu_o, a[2]=a, a[3]=b1, a[4]=log(dc1), a[5]=b2, a[6]=log(dc2) */
  /* post 14.2 change: a[1]=a, a[2]=b1, a[3]=log(dc1), a[4]=b2, a[5]=log(dc2) */


#define BIG_NUM 1e30

void set_rs_parameters(rsp,a,da)
     struct rs_parameters *rsp;
     double a[], da[];
{
  
  double log(),exp(),fabs();

  if(a != NULL)
    { 					/* update*/
      rsp->a   = a[1];
      rsp->b1  = a[2];
      rsp->dc1 = exp(a[3]);		/*a[3] is really log of dc1*/	
      rsp->b2  = (rsp->one_sv_flag) ? 0.0 : a[4];
      rsp->dc2 = (rsp->one_sv_flag) ? BIG_NUM : exp(a[5]); /*a[5]=log(dc1)*/
    }

  if(da != NULL)				
    { 			/* these are size of last step (update) from svd solution*/
      rsp->a_step   = da[1];		
      rsp->b1_step  = da[2];
      rsp->dc1_step = exp(a[3]+da[3])-exp(a[3]);	
      rsp->b2_step  = (rsp->one_sv_flag) ? 0.0: da[4];
      rsp->dc2_step = (rsp->one_sv_flag) ? 0.0: exp(a[5]+da[5])-exp(a[5]);
    }

  rsp->amb = rsp->a - (rsp->b1 + rsp->b2); 

                                /*set V_REF, MU_REF*/
  rsp->v_ref = 1e-1;			/*use initial mu value (muo) as ref. for mu_ref*/
  rsp->mu_ref = rsp->muo - rsp->amb*log(rsp->vo/rsp->v_ref);

				/*this is old approach, pre 30/5/96*/
/*   rsp->muf = rsp->muo + rsp->amb * log(rsp->vf/rsp->vo);*/
	
}
#undef BIG_NUM 


/*************************************************************************/

#define DISP_TOL 1e-3   /* I assume this is in microns */
#define ONE	 1.0
#define TWO	 2.0
#define SMALL_NUM 1e-30
#define EPSILON  1e-11 
#define	PGROW	-0.200
#define PSHRINK	-0.250
#define FCOR	1.0/15.0
#define SAFETY	0.9
#define ERRCON	6.0e-4

/* This function calculates friction at the displacements in x[] for a few
Rate/State friction law . The result is put in mod_mu[].

   Ruina law (old method)
   mu = mu_init + a ln(v/v_init) + b psi + c dx  , where c (lin_term) 
   is a term to describe linear hardening/weakening
   d_psi/dt = -v/Dc[psi+ ln (v/v_init)]
   d_mu = k[v - v_init]			, v is slider velocity after step, v_init is load point v 

  constit. law
                (1)     mu = mu_ref + a ln(v/v_ref) + b ln(v_ref*theta/Dc)
                (2)     d_theta/dt = -(v*theta/Dc) *ln(v*theta/Dc)
  elastic coupling
                (3)     d_mu/dt = k (v_lp - v)

 calculation involves solving (1) for v and subing this into (2), so that:

                (4)     d_theta/dt = (alpha*v_ref*theta/Dc) * ln(alpha*v_ref*theta/Dc)
 where we define:
        alpha = exp[(mu - mu_ref - b*ln(v_ref*theta/Dc)/a]
 or for two sv's alpha = exp[(mu - mu_ref - b1*ln(v_ref*theta1/Dc1)-b2*ln(v_ref*theta2/Dc1))/a]

 Then the solution involves solving (4) with a rewritten version of (3):

                (5)     d_mu/dt = K/sigma_n * (v_lp - alpha/v_ref)


a linear term is also an option
        mu = mu_init + ....+ c dx  , where c (lin_term)
                                                        is a term to describe linear hardening/weakening

   See office notes from about 15/7/94 for other laws and info..
   
   a is the ma vector of rs parameters: 
   pre 14.2 change: a[1] = mu_o, a[2]=a, a[3]=b1, a[4]=log(dc1), a[5]=b2, a[6]=log(dc2) 
  post 14.2 change: a[1]=a, a[2]=b1, a[3]=log(dc1), a[4]=b2, a[5]=log(dc2) 

   x is the ndata vector of displacements at which mod_mu[i] is needed

   rsp contains things like stiffness etc. It's defined in global.h
   rsp->vs_row is the point at which the vel. step occurred  (in the look file)

   Modifications:
   modified to handle multiple velocity steps
   15/7/94 modified to handle other rs laws
   25/7/94 modified approach used for multiple velocity steps, 
   use initial vo like reference velocity
   29/5/96 modified to handle slide-hold-slide tests, 
	in this case, modeling will be done in time (not disp)
	and we need to be able to set lp_v = 0, which requires modifying 
	the way vf and mu_f are treated --using real reference v and mu values.
   1/7/98, added new law --called "my law" for now...
   25/3/99, Added option to return state or velocity rather than friction. But I haven't
	check if this has any ramifications for inversion. I'm assuming it will be used 
	only for forward modeling.
 14.2.2010, cjm: change so that we don't model mu_o, but instead treat it as a known value
   */

int get_mu_at_x_t(x, mod_mu, ndata, rsp)
     double x[], mod_mu[];
     int ndata;
     struct rs_parameters *rsp;
{
  double h, hh, next_h, hdid, tnow, ttnd, v;
  double mu, p_mu, old_mu, psi1, p_psi1, old_psi1, psi2, p_psi2, old_psi2;
  double disp, time;
  double mu_err_scale, psi_err_scale, err, max_err;
  double mu_err, psi1_err, psi2_err; 
  double fabs(), log(), exp();
  double constant[5];			/*for rk calc*/
  int i, j, h_err;
  int do_rk();
  
  constant[0]=constant[1]=0.0;
  constant[2]=constant[3]=0.5;
  constant[4]=1.0;
					/* recall that vel_list[0] contains the velocity after
				the step change, it is not v_initial*/
  rsp->vf = rsp->vel_list[0];		/*reset vf, which gets changed for multiple v steps */

/*  rsp->v_ref = rsp->vf; 		*/
/*use velocity and mu after the step as reference values, */
/* rsp->mu_ref = rsp->muf;		*/
/* unless doing multiple velocity steps*/

  mu_err_scale = rsp->muo;		/* use constant fractional errors*/
  psi_err_scale = rsp->dc1/rsp->vo;
  
  			/* initial time step */ 
  			/* This is the first point after (at) the velocity step */
  next_h  = (x[(rsp->vs_row-rsp->first_row)+1]-x[(rsp->vs_row-rsp->first_row)])/(10.0*rsp->vo);
  
  rsp->vr_dc1 = rsp->v_ref / rsp->dc1;		/*std settings*/
  rsp->vr_dc2 = rsp->v_ref / rsp->dc2;
  
  switch(rsp->law)		    /* set up initial value for psi, note I use "psi" for theta */
    {						
      /* 	in case of multiple steps, use 
		initial vo like true ref velocity, e.g.
		change only vf for each additional v step */
       
    case 'o':
      psi1 = psi2 = -log(rsp->vo/rsp->v_ref);
      break;
    case 'm':		/*my law --same as d and r*/
    case 'r':
    case 'd':
      psi1 = rsp->dc1/rsp->vo;        /*this will be the same for theta version of r and d*/
      psi2 = rsp->dc2/rsp->vo;
      break;
    case 'p':
      rsp->vr_dc1 = rsp->v_ref/(TWO*rsp->dc1);		/*modify for Perrin-Rice law*/
      rsp->vr_dc2 = rsp->v_ref/(TWO*rsp->dc2);
      psi1 = (TWO*rsp->dc1)/rsp->vo;
      psi2 = (TWO*rsp->dc2)/rsp->vo;
      break;
    case 'j':                                       /*check this later, just a guess now 15/7/94 */
      fprintf(stderr,"rice law not implemented\n");
      return(-1);
      break;
    }

  if(rsp->one_sv_flag)
    rsp->vr_dc2 = SMALL_NUM;  /*b2 is zero for 1sv model, set dc2 to very large number*/

  rsp->v_lp = rsp->vf;		/*load point velocity is velocity after step */

  mu = rsp->muo; 		/* initial mu, muf varies with a-b*/
  v  = rsp->vo;

  for(i=1;i<=(rsp->vs_row-rsp->first_row);i++)	/*fill with prejump mu*/
  {
  	if(rsp->op_file_flag & 0x40)	/*return velocity rather than mu*/
	  mod_mu[i] = v;
  	else if(rsp->op_file_flag & 0x80) /*return state rather than mu*/
	  mod_mu[i] = psi1;
  	else
	  mod_mu[i] = mu+rsp->lin_term*(x[i]-x[(rsp->vs_row-rsp->first_row)]);
  }
  
  i=(rsp->vs_row-rsp->first_row+1);		/*first point after vs_row*/
  disp = x[(rsp->vs_row-rsp->first_row)];

  if(rsp->op_file_flag & 0x20)		/*note time will be in "x" vol if we're modeling mu vs time*/
	time=disp;

  j=1;					/*j=0 is the initial step*/

  while( i <= ndata )
    {
      if(i==(rsp->vs_row_list[j]-rsp->first_row+1) )	/*add one to get v step in right place*/
	{ 					
					/*Multiple velocity steps*/
					/*deal with multiple velocity step (mvs) option*/
					/*don't update psi etc. to steady state, just change lp_v*/
	  rsp->v_lp = rsp->vf = rsp->vel_list[j++];            /*new load_point velocity*/
	  next_h /= 10;			/*make smaller, for safety*/

	  /* use muo for first step as reference velocity */	
/* removed 30/5/96*/
/*	  rsp->v_ref = rsp->vo;
	  rsp->muf = rsp->muo + rsp->amb * log(rsp->vf/rsp->vo);
	  rsp->vr_dc1 = rsp->vf / rsp->dc1;		
	  rsp->vr_dc2 = rsp->vf / rsp->dc2;
	  switch(rsp->law)
	    {
	    case 'p':
	      rsp->vr_dc1 = rsp->vf/(TWO*rsp->dc1);          
	      rsp->vr_dc2 = rsp->vf/(TWO*rsp->dc2);
	      break;
	    case 'j':                                       
	      break;
	    }
	  if(rsp->one_sv_flag)
	    rsp->vr_dc2 = SMALL_NUM;  
	  j++;
*/
	}
      if(rsp->op_file_flag & 0x20)	/*32's bit means we're modeling mu vs. time*/
	ttnd = fabs(x[i]-x[i-1]);
      else
	ttnd = fabs(x[i]-x[i-1])/rsp->v_lp;		/* time to next data*/
      tnow=0; 					/* time since last data*/ 

      while(tnow < ttnd)
	{
	  h_err=TRUE;
	  
	  if(tnow+next_h >= ttnd)			/*set time*/
	    h = ttnd-tnow;
	  else
	    h = next_h;
	  
	  while(h_err)
	    {
	      /* save old values */
	      old_mu = p_mu = mu;
	      old_psi1 = p_psi1 = psi1;
	      old_psi2 = p_psi2 = psi2;
	      
	      hh = h/TWO;				/* half-steps*/

	      
	      if((do_rk(&mu, &psi1, &psi2, &hh, &v, rsp, constant) == -1) ||  
		 /*1st half step*/
		 (do_rk(&mu, &psi1, &psi2, &hh, &v, rsp, constant) == -1) ||  
		 /*2nd half step, using updated vals*/
		 (do_rk(&p_mu, &p_psi1, &p_psi2, &h, &v, rsp, constant) == -1)    /* full step, updated vals in p_ */)
		{
		  sprintf(msg, "\terror from do_rk, row=%d, exiting early\t",i);
		  print_msg(msg);
		  
		  fprintf(op_file,"\terror from do_rk, row=%d, exiting early\t",i);
		  rsp->v_lp = rsp->vf = rsp->vel_list[0];	/*reset*/
		  return(-1);
		}
	      
	      if(!h)				   /*time step too small? */
		{
		  sprintf(msg, "\terror from do_rk, time_step = 0, row=%d, exiting early\t",i); 
		  print_msg(msg);
		  fprintf(op_file,"\terror from do_rk, time_step = 0, row=%d, exiting early\t",i);
		  rsp->v_lp = rsp->vf = rsp->vel_list[0];
		  return(-1);
		}

	      max_err=0;			      /* evaluate error*/
	      mu_err = mu-p_mu; 
	      err = fabs(mu_err/mu_err_scale);
	      max_err = (err > max_err) ? err : max_err;
	      
	      psi1_err = psi1-p_psi1;
	      err = fabs(psi1_err/psi_err_scale);
	      max_err = (err > max_err) ? err : max_err;
	      
	      psi2_err = psi2-p_psi2;
	      err = fabs(psi2_err/psi_err_scale);
	      max_err = (err > max_err) ? err : max_err;
	      
	      max_err /= EPSILON;	     /* scale relative to tolerance*/
	      
	      if(max_err <=1.0)				
		/*step succeeded, compute size of next step*/
		{
		  hdid = h;
		  next_h	= (max_err > ERRCON) ? SAFETY*h*exp(PGROW*log(max_err)) : 4.0*h;
		  h_err=FALSE;
		}
	      else
		{
		  h = SAFETY*h*exp(PSHRINK*log(max_err));	/*truncation error too large, reduce step size*/
		  mu   = old_mu;
		  psi1 = old_psi1;
		  psi2 = old_psi2;
		}
	    }			/* loop for adaptive step size control*/
	  tnow += hdid;
	  time += hdid;		/*cumulative time*/
	  mu += mu_err*FCOR;			  /* fifth order bit */
	  psi1 += psi1_err*FCOR;
	  psi2 += psi2_err*FCOR;
	  disp += rsp->v_lp*hdid;
						/*update slider vel*/
	  v = (rsp->v_ref * exp((mu - rsp->mu_ref - (rsp->b1*log(rsp->vr_dc1 * psi1)) - (rsp->b2*log(rsp->vr_dc2 * psi2)))/rsp->a));
	}			/* loop for time to next data  */
      
      if(rsp->op_file_flag & 0x20)		/*check vs time if modeling time, otherwise displ*/
      {
        if( fabs(time-x[i]) > DISP_TOL )
	{
	  fprintf(op_file,"\tget_mu_at_x_t: error, time not correct, row=%d, measured_disp=%f, numer_disp =%f, exiting early\t",i+rsp->first_row,x[i],disp);
	  return(-1);
	}
      }
      else
      {
      	if( fabs(disp-x[i]) > DISP_TOL )
	{
	  fprintf(op_file,"\tget_mu_at_x_t: error, displacement not correct, row=%d, measured_disp=%f, numer_disp =%f, exiting early\t",i+rsp->first_row,x[i],disp);
	  return(-1);
	}
      }
  
      if(rsp->op_file_flag & 0x40)    /*return velocity rather than mu*/
          mod_mu[i] = v;
      else if(rsp->op_file_flag & 0x80) /*return state rather than mu*/
          mod_mu[i] = psi1;
      else
          mod_mu[i] = mu+rsp->lin_term*(x[i]-x[(rsp->vs_row-rsp->first_row)]);

      ++i;                    /* increment data counter */

    }		/* end of loop for data points */
 
  rsp->v_lp = rsp->vf = rsp->vel_list[0];			/*reset v_lp and vf*/
  return(0);	/* success! */
}
#undef SMALL_NUM 
#undef ONE
#undef TWO
#undef PGROW	
#undef PSHRINK
#undef FCOR
#undef SAFETY
#undef ERRCON
#undef EPSILON  
#undef DISP_TOL 


/*************************************************************************/

/*	This func. does most of the work of solving the coupled
	equations for rate/state friction and elastic interaction. It's called 
	from get_mu_at_x_t(). 
   pre 14.2 change: a[1] = mu_o, a[2]=a, a[3]=b1, a[4]=log(dc1), a[5]=b2, a[6]=log(dc2) 
   post 14.2 change: a[1]=a, a[2]=b1, a[3]=log(dc1), a[4]=b2, a[5]=log(dc2) 
	*/

#define	BIGNUM 1e20
#define ONE	1.0
#define TWO	2.0
#define FIVE	5.0
#define TEN	10.0
#define P16	ONE/6.0

/*      these are defined by caller
	constant[0]=0;
	constant[1]=constant[2]=0.5;
	constant[3]=1;
	*/

int do_rk(mu, psi1, psi2, H, v, rsp, constant)
     double *mu, *psi1, *psi2, *H, *v, *constant;
     struct rs_parameters *rsp;
{
  
  int	calc_bombed=1;
  int 	i;
  /* int	isnan(); */
  double	alpha,arg;
  double	J[5], K[5], M[5];
  double	w_psi1, w_psi2, w_mu;
  double	old_psi1, old_psi2, old_mu, old_H, old_v;
  double	exp(), log();

 while(calc_bombed)
 {
   old_psi1 = *psi1;             /* save last vals in case calc bombs ...*/
   old_psi2 = *psi2;
   old_mu = *mu;
   old_H = *H; 
   old_v = *v; 
   J[0]=K[0]=M[0]=0;	
   /* Runga Kutta calc */
   switch(rsp->law)
     {
     case 'o':				   /*old, original Runia slip law*/
       for(i=1;i<5;++i)	
	 {
	   w_psi1 =	*psi1 + J[i-1]*constant[i];
	   w_psi2 =	*psi2 + K[i-1]*constant[i];
	   w_mu = 	*mu   + M[i-1]*constant[i];
	   alpha = exp((w_mu - rsp->mu_ref - rsp->b1*w_psi1 - rsp->b2*w_psi2)/rsp->a);
	   J[i] = (*H) * -rsp->vr_dc1 * alpha * (w_psi1 + log(alpha));
	   K[i] = (*H) * -rsp->vr_dc2 * alpha * (w_psi2 + log(alpha));
	   M[i] = (*H) * rsp->stiff * (rsp->v_lp - rsp->v_ref*alpha)  ;
	 } 
       *psi1 +=  (J[1] + TWO*J[2] + TWO*J[3] + J[4])*P16; 
       *psi2 +=  (K[1] + TWO*K[2] + TWO*K[3] + K[4])*P16;
       *mu   +=  (M[1] + TWO*M[2] + TWO*M[3] + M[4])*P16;

       *v = rsp->v_ref * exp((*mu - rsp->mu_ref - (rsp->b1*(*psi1)) - (rsp->b2*(*psi2)))/rsp->a);
       break;
	
     case 'r':			/*Ruina, slip law, using theta as sv*/
       for(i=1;i<5;++i)	
	 {
	   w_psi1 =	*psi1 + J[i-1]*constant[i];
	   w_psi2 =	*psi2 + K[i-1]*constant[i];
	   w_mu = 	*mu   + M[i-1]*constant[i];
	   alpha = exp((w_mu - rsp->mu_ref - rsp->b1*log(rsp->vr_dc1*w_psi1) - rsp->b2*log(rsp->vr_dc2*w_psi2) )/rsp->a);
	   arg = alpha * rsp->vr_dc1 * w_psi1;
	   J[i] = (*H) * -arg * log(arg);
	   arg = alpha * rsp->vr_dc2 * w_psi2;
	   K[i] = (*H) * -arg * log(arg);
	   M[i] = (*H) * rsp->stiff * (rsp->v_lp - rsp->v_ref*alpha);
	 } 
       *psi1 +=  (J[1] + TWO*J[2] + TWO*J[3] + J[4])*P16; 
       *psi2 +=  (K[1] + TWO*K[2] + TWO*K[3] + K[4])*P16;
       *mu   +=  (M[1] + TWO*M[2] + TWO*M[3] + M[4])*P16;

       *v = rsp->v_ref * exp((*mu - rsp->mu_ref - (rsp->b1*log(rsp->vr_dc1 * *psi1)) - (rsp->b2*log(rsp->vr_dc2 * *psi2)))/rsp->a);
       break;
       
     case 'd':			/*Dieterich, slowness law*/
       for(i=1;i<5;++i)	
	 {
	   w_psi1 =	*psi1 + J[i-1]*constant[i];
	   w_psi2 =	*psi2 + K[i-1]*constant[i];
	   w_mu = 	*mu   + M[i-1]*constant[i];
	   alpha = exp((w_mu - rsp->mu_ref - rsp->b1*log(rsp->vr_dc1*w_psi1) - rsp->b2*log(rsp->vr_dc2*w_psi2) )/rsp->a);
	   arg = alpha * rsp->vr_dc1 * w_psi1;
	   J[i] = (*H) * (ONE - arg);
	   arg = alpha * rsp->vr_dc2 * w_psi2;
	   K[i] = (*H) * (ONE - arg);
	   M[i] = (*H) * rsp->stiff * (rsp->v_lp - rsp->v_ref*alpha);
	 } 
       *psi1 +=  (J[1] + TWO*J[2] + TWO*J[3] + J[4])*P16; 
       *psi2 +=  (K[1] + TWO*K[2] + TWO*K[3] + K[4])*P16;
       *mu   +=  (M[1] + TWO*M[2] + TWO*M[3] + M[4])*P16;

       *v = rsp->v_ref * exp((*mu - rsp->mu_ref - (rsp->b1*log(rsp->vr_dc1 * *psi1)) - (rsp->b2*log(rsp->vr_dc2 * *psi2)))/rsp->a);
     break;
       
     case 'm':                     /*my law , requires 2 svs*/
	for(i=1;i<5;++i)
	{
		w_psi1 =        *psi1 + J[i-1]*constant[i];
		w_psi2 =        *psi2 + K[i-1]*constant[i];
		w_mu =          *mu   + M[i-1]*constant[i];
		alpha = exp((w_mu - rsp->mu_ref - rsp->b1*log(rsp->vr_dc1*w_psi1) - rsp->b2*log(rsp->vr_dc2*w_psi2) )/rsp->a);
		arg = alpha * rsp->vr_dc1 * w_psi1;
		J[i] = (*H) * (ONE - arg);
		arg = alpha * rsp->vr_dc2 * w_psi2;
		K[i] = (*H) * -arg * log(arg);
		M[i] = (*H) * rsp->stiff * (rsp->v_lp - rsp->v_ref*alpha);
	}
	*psi1 +=  (J[1] + TWO*J[2] + TWO*J[3] + J[4])*P16;
	*psi2 +=  (K[1] + TWO*K[2] + TWO*K[3] + K[4])*P16;
	*mu   +=  (M[1] + TWO*M[2] + TWO*M[3] + M[4])*P16;

	*v = ( rsp->v_ref * exp((*mu - rsp->mu_ref - (rsp->b1*log(rsp->vr_dc1 * *psi1)) - (rsp->b2*log(rsp->vr_dc2 * *psi2)))/rsp->a));
     break;

     case 'j':			/*Rice Law*/
       sprintf(msg, "no rice law yet \n");
       print_msg(msg);
       return(-1);         /* inform caller of problem*/
       break;

     case 'p':		/*Perrin-Rice-Zheng Quadratic law*/
       for(i=1;i<5;++i)	
	 {
	   w_psi1 =	*psi1 + J[i-1]*constant[i];
	   w_psi2 =	*psi2 + K[i-1]*constant[i];
	   w_mu = 	*mu   + M[i-1]*constant[i];
	   alpha = exp((w_mu - rsp->mu_ref - rsp->b1*log(rsp->vr_dc1*w_psi1) - rsp->b2*log(rsp->vr_dc2*w_psi2) )/rsp->a);
	   arg = alpha * rsp->vr_dc1 * w_psi1;
	   J[i] = (*H) * (ONE - arg*arg);
	   arg = alpha * rsp->vr_dc2 * w_psi2;
	   K[i] = (*H) * (ONE - arg*arg);
	   M[i] = (*H) * rsp->stiff * (rsp->v_lp - rsp->v_ref*alpha);
	 } 
       *psi1 +=  (J[1] + TWO*J[2] + TWO*J[3] + J[4])*P16; 
       *psi2 +=  (K[1] + TWO*K[2] + TWO*K[3] + K[4])*P16;
       *mu   +=  (M[1] + TWO*M[2] + TWO*M[3] + M[4])*P16;

       *v = rsp->v_ref * exp((*mu - rsp->mu_ref - (rsp->b1*log(rsp->vr_dc1 * *psi1)) - (rsp->b2*log(rsp->vr_dc2 * *psi2)))/rsp->a);
       break;

     default : 
       sprintf(msg, "no rs law chosen. --how could this have happened?\n");
       print_msg(msg);
       return(-1);         /* inform caller of problem*/
     }
   
   if( isnan(*v) || isnan(*mu) || (fabs(*v) > BIGNUM) )
     {
       fprintf(op_file,"do_rk: calculation bombed, retry #%d,\tcurrent parameters are:\na=%g, b1=%g, dc1=%g, b2=%g, dc2=%g,\nmu_o=%g, mu_f=%g, time step=%g, v=%g old_H=%g, old_v=%g\n",calc_bombed,rsp->a,rsp->b1,rsp->dc1,rsp->b2,rsp->dc2,rsp->muo,rsp->mu_ref,*H,*v,old_H, old_v);
       if(++calc_bombed == 3 )          /* give up if problem persists */
	 {
	   sprintf(msg, "do_rk: calculation bombed out\tcurrent parameters are:\na=%g, b1=%g, dc1=%g, b2=%g, dc2=%g,\nmu_o=%g, mu_f=%g, time step=%g, v=%g old_H=%g, old_v=%g\n",rsp->a,rsp->b1,rsp->dc1,rsp->b2,rsp->dc2,rsp->muo,rsp->mu_ref,*H,*v,old_H, old_v);
	   print_msg(msg);
	   fprintf(op_file,"do_rk: calculation bombed out\tcurrent parameters are:\na=%g, b1=%g, dc1=%g, b2=%g, dc2=%g,\nmu_o=%g, mu_f=%g, time step=%g, v=%g old_H=%g, old_v=%g\n",rsp->a,rsp->b1,rsp->dc1,rsp->b2,rsp->dc2,rsp->muo,rsp->mu_ref,*H,*v,old_H, old_v);
	   return(-1);         /* inform caller of problem*/
	 }
       else
	 {
	   *psi1 = old_psi1;        /* install old vals */
	   *psi2 = old_psi2;
	   *mu   = old_mu;
	   *H    = old_H/TEN;      /* reduce H -stablize calc? */
	   *v    = old_v;
	 }
     }
   else
     calc_bombed = FALSE;               	/* all OK, stop and return */ 
   
 }	/* end of calc_bombed loop */
  return(0);
}
#undef ONE
#undef TEN
#undef FIVE
#undef TWO
#undef P16
#undef BIGNUM

/*************************************************************************/

#include <math.h>

static double at,bt,ct;
#define PYTHAG(a,b) ((at=fabs(a)) > (bt=fabs(b)) ? \
		     (ct=bt/at,at*sqrt(1.0+ct*ct)) : (bt ? (ct=at/bt,bt*sqrt(1.0+ct*ct)): 0.0))

static double maxarg1,maxarg2;
#define MAXIM(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
		  (maxarg1) : (maxarg2))
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

int msvdcmp(a,m,n,w,v)
     double **a,*w,**v;
     int m,n;
{
  int flag,i,its,j,jj,k,l,nm;
  double c,f,h,s,x,y,z;
  double anorm=0.0,g=0.0,scale=0.0;
  double *rv1;
  
  if (m < n) 	
    {
      sprintf(msg, "nrerror: SVDCMP: You must augment A with extra zero rows\n");
      print_msg(msg);
      return(-1);	
    }

  rv1=(double *)malloc((n+1)*sizeof(double));
  /*rv1=(double *)calloc((unsigned)n+1,sizeof(double));*/
  for (i=1;i<=n;i++)
    {
      l=i+1;
      rv1[i]=scale*g;
      g=s=scale=0.0;
      if (i <= m)
	{
	  for (k=i;k<=m;k++) scale += fabs(a[k][i]);
	  if (scale)
	    {
	      for (k=i;k<=m;k++)
		{
		  a[k][i] /= scale;
		  s += a[k][i]*a[k][i];
		}
	      f=a[i][i];
	      g = -SIGN(sqrt(s),f);
	      h=f*g-s;
	      a[i][i]=f-g;
	      if (i != n) 
		{
		  for (j=l;j<=n;j++) 
		    {
		      for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
		      f=s/h;
		      for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
		    }
		}
	      for (k=i;k<=m;k++) a[k][i] *= scale;
	    }
	}
      
      w[i]=scale*g;
      g=s=scale=0.0;
      if (i <= m && i != n)
	{
	  for (k=l;k<=n;k++) scale += fabs(a[i][k]);
	  if (scale) 
	    {
	      for (k=l;k<=n;k++)
		{
		  a[i][k] /= scale;
		  s += a[i][k]*a[i][k];
		}
	      f=a[i][l];
	      g = -SIGN(sqrt(s),f);
	      h=f*g-s;
	      a[i][l]=f-g;
	      for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
	      if (i != m) 
		{
		  for (j=l;j<=m;j++) 
		    {
		      for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
		      for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
		    }
		}
	      for (k=l;k<=n;k++) a[i][k] *= scale;
	    }
	}
      anorm=MAXIM(anorm,(fabs(w[i])+fabs(rv1[i])));
    }
  for (i=n;i>=1;i--)
    {
      if (i < n)
	{
	  if (g)
	    {
	      for (j=l;j<=n;j++)
		v[j][i]=(a[i][j]/a[i][l])/g;
	      for (j=l;j<=n;j++) 
		{
		  for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
		  for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
		}
	    }
	  for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
	}
      v[i][i]=1.0;
      g=rv1[i];
      l=i;
    }
  for (i=n;i>=1;i--) 
    {
      l=i+1;
      g=w[i];
      if (i < n)
	for (j=l;j<=n;j++) a[i][j]=0.0;
      if (g)
	{
	  g=1.0/g;
	  if (i != n)
	    {
	      for (j=l;j<=n;j++) 
		{
		  for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
		  f=(s/a[i][i])*g;
		  for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
		}
	    }
	  for (j=i;j<=m;j++) a[j][i] *= g;
      } 
      else 
	{
	  for (j=i;j<=m;j++) a[j][i]=0.0;
	}
      ++a[i][i];
    }
  for (k=n;k>=1;k--) 
    {
      for (its=1;its<=30;its++)
	{
	  flag=1;
	  for (l=k;l>=1;l--) 
	    {
	      nm=l-1;
	      if (fabs(rv1[l])+anorm == anorm)
		{
		  flag=0;
		  break;
		}
	      if (fabs(w[nm])+anorm == anorm) break;
	    }
	  if (flag) 
	    {
	      c=0.0;
	      s=1.0;
	      for (i=l;i<=k;i++) 
		{
		  f=s*rv1[i];
		  if (fabs(f)+anorm != anorm)
		    {
		      g=w[i];
		      h=PYTHAG(f,g);
		      w[i]=h;
		      h=1.0/h;
		      c=g*h;
		      s=(-f*h);
		      for (j=1;j<=m;j++) 
			{
			  y=a[j][nm];
			  z=a[j][i];
			  a[j][nm]=y*c+z*s;
			  a[j][i]=z*c-y*s;
			}
		    }
		}
	    }
	  z=w[k];
	  if (l == k) 
	    {
	      if (z < 0.0) 
		{
		  w[k] = -z;
		  for (j=1;j<=n;j++) v[j][k]=(-v[j][k]);
		}
	      break;
	    }
	  if (its == 30) 
	    {
	      sprintf(msg, "nrerror: No convergence in 30 SVDCMP iterations\n");
	      print_msg(msg);
	      return(-1);
	    }
	  x=w[l];
	  nm=k-1;
	  y=w[nm];
	  g=rv1[nm];
	  h=rv1[k];
	  f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
	  g=PYTHAG(f,1.0);
	  f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
	  c=s=1.0;
	  for (j=l;j<=nm;j++)
	    {
	      i=j+ 1;
	      g=rv1[i];
	      y=w[i];
	      h=s*g;
	      g=c*g;
	      z=PYTHAG(f,h);
	      rv1[j]=z;
	      c=f/z;
	      s=h/z;
	      f=x*c+g*s;
	      g=g*c-x*s;
	      h=y*s;
	      y=y*c;
	      for (jj=1;jj<=n;jj++) 
		{
		  x=v[jj][j];
		  z=v[jj][i];
		  v[jj][j]=x*c+z*s;
		  v[jj][i]=z*c-x*s;
		}
	      z=PYTHAG(f,h);
	      w[j]=z;
	      if (z) 
		{
		  z=1.0/z;
		  c=f*z;
		  s=h*z;
		}
	      f=(c*g)+(s*y);
	      x=(c*y)-(s*g);
	      for (jj=1;jj<=m;jj++) 
		{
		  y=a[jj][j];
		  z=a[jj][i];
		  a[jj][j]=y*c+z*s;
		  a[jj][i]=z*c-y*s;
		}
	    }
	  rv1[l]=0.0;
	  rv1[k]=f;
	  w[k]=x;
	}
    }
  free(rv1);
  return(0);
}

#undef SIGN
#undef MAXIM
#undef PYTHAG


/********************************************************************/

void msvbksb(u,w,v,m,n,b,x)
     double **u,w[],**v,b[],x[];
     int m,n;
{
  int jj,j,i;
  double s,*tmp;

  tmp=(double *)malloc((n+1)*sizeof(double));
  /*tmp=(double *)calloc((unsigned)n+1,sizeof(double));*/
  for (j=1;j<=n;j++) 
    {
      s=0.0;
      if (w[j]) 
	{
	  for (i=1;i<=m;i++) s += u[i][j]*b[i];
	  s /= w[j];
	}
      tmp[j]=s;
    }
  for (j=1;j<=n;j++) 
    {
      s=0.0;
      for (jj=1;jj<=n;jj++) s += v[j][jj]*tmp[jj];
      x[j]=s;
    }
  free(tmp);
}


/********************************************************************/

void zero_sv(w,ma,tol,op_flag)
     double w[],tol;
     int ma,op_flag;
{
  int j;
  double wmax, thresh;
  
  wmax=0.0;                       /* zero singular values */
  for (j=1;j<=ma;j++)
    if (w[j] > wmax)
      wmax=w[j];
  thresh=tol*wmax;
  for (j=1;j<=ma;j++)
    {
      if (w[j] < thresh)
	{
	  if(op_flag)
	    fprintf(op_file,"singular value zeroed, w[%d]=%g, wmax=%g, tol.=%g\n",j,w[j],wmax,tol);
	  w[j]=0.0;
	}
    }
}



