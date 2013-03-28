#include	<math.h>
#include	<stdio.h>

#include    <config.h>
#include	<global.h>
#include    <fq.h>

#define		SR_2   1.41421356
#define		BIGNUM 1e20

extern char msg[MSG_LENGTH];


/* This function is currently not being used. -- WAC 3 May 2006 */
int rate_state_mod(disp_col, vstep_row, vstep_end, mod_disp_col, mod_mu_col, k, sig_n, muo, muf, vo, vf, A, B1, B2, dc1, dc2)
     int	disp_col, vstep_row, vstep_end, mod_disp_col, mod_mu_col;
     double	k, sig_n, muo, muf, vo, vf, A, B1, B2, dc1, dc2;
{
  int	i,j, init_pts, init_write_pt;
  /* int	isnan(); */
  double	psi1, psi2, h, H, K1, K2, K3, K4;
  double	x, x_inc, fin_disp_inc, mu, v;
  double  M1, M2, M3, M4, Vo_dc1, Vo_dc2, w_psi1, w_mu, time, alpha;
  double	J1, J2, J3, J4, w_psi2;
  double  wmu_muf_on_A, B1_on_A, B2_on_A, omB1_on_A, omB2_on_A, K_s_vf;
  double  exp();


  /*	if(k > 0.05) 
	init_write_pt = 1;*/	/* write every other pt */
  /*	else	*/
  init_write_pt =2;	/* write every 3rd pt */	
  
  /*set up so that Vo is velocity *after* step. MUo is mu *after* step */
  mu = muo;
  /* get disp increment so we know how many points to write*/
  fin_disp_inc = (darray[disp_col][vstep_end] - darray[disp_col][vstep_row]) / (0.8*(vstep_end - vstep_row) );
  /* write initial 20 % of the data closely spaced */
  init_pts = vstep_row + (int) (0.2*(vstep_end - vstep_row));
  
  /*init_disp_inc = fin_disp_inc / 25.00;
    init_disp = darray[disp_col][vstep_row] + (darray[disp_col][vstep_end] - darray[disp_col][vstep_row]) * 0.02000; 
    fin_disp_inc /= 0.5102; */
  
  /* the model mu output contains 100 rows (if possible) of the initial mu value -to establish a "baseline" */
  
  for ( i= (vstep_row > 100) ? (vstep_row-100) : 0; i <= vstep_row; ++i)
    {
      darray[mod_mu_col][i] = muo;
      darray[mod_disp_col][i] = darray[disp_col][i];;
    }
  /* increase so that model reaches sufficient disp for comparsion*/
  vstep_end = (vstep_end + 1000 < max_row) ? (vstep_end+1000) : max_row-1;
  /*									*/
  /*									*/
  /*dpsi/dt = -vf/dc1 exp[(muo - muf - Bpsi)/A] [psi(1-B/A) + (muo - muf)/A] 	*/
  /*									*/
  /*dmu/dt = k/sigma_n [vf (1 - exp[(muo - muf - Bpsi)/A]			*/ 
  /*									*/
  /*									*/
  /*									*/
  
  time = 0.0;		/*x is slip distance */
  K_s_vf = (k / sig_n) * vf;
  h = 0.010000;	/* h = time step*/
  /* normalize by velocity so that we have more points*/
  /* in the fast areas and less when the slip vel is low*/
  H = h / vo;
  psi1 = psi2 = -log(vo/vf);
  
  Vo_dc1 = -vf/dc1;
  Vo_dc2 = -vf/dc2;
  
  x = darray[disp_col][vstep_row];
  x_inc = 0;
  
  B1_on_A = B1/A;
  B2_on_A = B2/A;

  omB1_on_A = 1.000 - B1_on_A;
  omB2_on_A = 1.000 - B2_on_A;

  j = 0;					/* loop counter */
  while( i <= vstep_end)
    {
      alpha = exp((mu - muf - B2*psi2 - B1*psi1)/A);
      wmu_muf_on_A = (mu - muf)/A;
      J1 = H * (Vo_dc1 * alpha *  (psi1 * omB1_on_A - B2_on_A*psi2 + wmu_muf_on_A) );
      K1 = H * (Vo_dc2 * alpha *  (psi2 * omB2_on_A - B1_on_A*psi1 + wmu_muf_on_A) );
      M1 = H *  K_s_vf * (1.00000 - alpha)  ; 

      w_psi1 = psi1 + J1*0.500000;  
      w_psi2 = psi2 + K1*0.500000;
      w_mu = mu + M1*0.500000; 
      alpha = exp((w_mu - muf - B2*w_psi2 - B1*w_psi1)/A);
      wmu_muf_on_A = (w_mu - muf)/A;
      J2 = H*( Vo_dc1 * alpha * (w_psi1 * omB1_on_A - B2_on_A*w_psi2 + wmu_muf_on_A) );
      K2 = H*( Vo_dc2 * alpha * (w_psi2 * omB2_on_A - B1_on_A*w_psi1 + wmu_muf_on_A) );
      M2 = H*  K_s_vf * (1.00000 - alpha) ;


      w_psi1 = psi1 + J2*0.500000000;
      w_psi2 = psi2 + K2*0.500000000;
      w_mu = mu + M2*0.500000000; 
      alpha = exp((w_mu - muf - B2*w_psi2 - B1*w_psi1)/A);
      wmu_muf_on_A = (w_mu - muf)/A;
      J3 = H*( Vo_dc1 * alpha * (w_psi1 * omB1_on_A - B2_on_A*w_psi2 + wmu_muf_on_A) );
      K3 = H*( Vo_dc2 * alpha * (w_psi2 * omB2_on_A - B1_on_A*w_psi1 + wmu_muf_on_A) );
      M3 = H*  K_s_vf * (1.00000 - alpha) ;

	
      w_psi1 = psi1 + J3;  
      w_psi2 = psi2 + K3;
      w_mu = mu + M3;   
      alpha = exp((w_mu - muf - B2*w_psi2 - B1*w_psi1)/A);
      wmu_muf_on_A = (w_mu - muf)/A;
      J4 = H*( Vo_dc1 * alpha * (w_psi1 * omB1_on_A - B2_on_A*w_psi2 + wmu_muf_on_A) );
      K4 = H*( Vo_dc2 * alpha * (w_psi2 * omB2_on_A - B1_on_A*w_psi1 + wmu_muf_on_A) );
      M4 = H*  K_s_vf * (1.00000 - alpha) ; 



      psi1 +=  (J1 + 2.0000*J2 + 2.0000*J3 + J4) * 0.166666666666667; /*psi1[n+1] = psi1[n] + .*/
      psi2 +=  (K1 + 2.0000*K2 + 2.0000*K3 + K4) * 0.166666666666667; /*psi2[n+1] = psi2[n] + .*/
      mu   +=  (M1 + 2.0000*M2 + 2.0000*M3 + M4) * 0.166666666666667;



      v = vf * exp( (mu - muf - B1*psi1 -B2*psi2)/A );
      x_inc += vf * H;

      H = h / v;
      time += H;
	
      /* write out a reasonable # of points -instead of the huge # we'd get if we wrote every point */
      if( (j++ == init_write_pt && i < init_pts) ||  x_inc > fin_disp_inc )
	{
	  j = 0;
	  x += x_inc;
	  x_inc = 0;
	  darray[mod_disp_col][i] = x;
	  darray[mod_mu_col][i] = mu;
	  ++i;
	  if( isnan(mu) || isnan(x) )
	    {
	      sprintf(msg, "Something bombed in calculation. Check parameter values and try reducing a or k \n");
	      print_msg(msg);
	      return(-1);
	    } 
	}
    }
  
  sprintf(msg, "%d points written before the vel. step and %d points written after.\n", (vstep_row > 100) ? 100 : vstep_row, i-vstep_row-1); 	
  print_msg(msg);
  return 0;
}
/*********************************************************************/


double simp_rate_state_mod()

{

  /* declaring the doubles as register doesn't make any diff. to calc time */
  /*	at least when compiling with optimization...  */


  char	inc_pt = FALSE, adjust_H = TRUE;
  int	i, j, k, total_added_pts, row_num, calc_bombed = 0;
  /* int	isnan(); */
  double	*disp_ptr, *mu_ptr, *model_mu_ptr;
  double	psi1, psi2, h, H, K1, K2, K3, K4;
  double	close, not_close, mu, v, x;
  double  M1, M2, M3, M4, Vo_dc1, Vo_dc2, w_psi1, w_mu, alpha;
  double	J1, J2, J3, J4, w_psi2;
  double  B1_on_A, B2_on_A, wmu_muf_on_A, omB1_on_A, omB2_on_A, K_s_vf;
  double	current_disp;	/* used to handle out of order pts. -tdxr noise */
  double	final_disp;
  double	error, exp();
  double  old_v, old_x, old_mu, old_psi1, old_psi2, old_H;

  total_added_pts = rs_param.added_pts*(rs_param.peak_row-rs_param.vs_row);

  /* first pt in array is vs_row */
  disp_ptr	= rs_param.disp_data;
  mu_ptr		= rs_param.mu_data;
  model_mu_ptr	= rs_param.model_mu;

  error = 0.00;

  /* look at x increment near v. step -assume this is the smallest inc */
  /*	don't let close be smaller than 0.020    */

  close = 0.000;

  for(i=0; i < 10; ++i)
    {
      close += fabs( *(disp_ptr+1) - *disp_ptr++)/50.00 ;
    }
  close /= 10.00;
  
  if(close  < 0.0200) close = 0.0200;
  not_close = close*3.000;

  /* for a vo of 10 mic/s and H_init = 0.001 x_inc will be 0.01 micron ; Say we have meas. every micron, then */
  /*	this should be small enough so that it's no problem getting model mu values at the right disp. */
  
  /*									*/
  /*									*/
  /*dpsi/dt = -vf/dc1 exp[(muo - muf - Bpsi)/A] [psi(1-B/A) + (muo - muf)/A] 	*/
  /*									*/
  /*dmu/dt = k/sigma_n [vf (1 - exp[(muo - muf - Bpsi)/A]			*/ 
  /*									*/
  /*									*/
  /*									*/

  /*set up so that Vo is velocity *after* step. MUo is mu *after* step */
  mu = rs_param.muo;
  /*A = rs_param.a; */
  /*B1 = rs_param.b1; */
  /*B2 = rs_param.b2; */
  
  K_s_vf = (rs_param.stiff / rs_param.sig_n) * rs_param.vf;
  
  if(close > 0.1)
    h = 0.01;	/* use bigger time step if disp. spacing is large */
  else
    h = 0.0050000;	/* h = time step*/
  /* normalize by velocity so that we have more points*/
  /* in the fast areas and less when the slip vel is low*/
  H = h / rs_param.vo;
  psi1 = psi2 = -log(rs_param.vo/rs_param.vf);
  
  Vo_dc1 = -rs_param.vf/rs_param.dc1;
  Vo_dc2 = -rs_param.vf/rs_param.dc2;

  B1_on_A = rs_param.b1/rs_param.a;
  B2_on_A = rs_param.b2/rs_param.a;

  omB1_on_A = 1.000 - B1_on_A;
  omB2_on_A = 1.000 - B2_on_A;
						/*x is slip distance */
  disp_ptr = rs_param.disp_data;		/* re-align */
  
  x = current_disp = *disp_ptr++;
  mu_ptr++;				/* first pt for compar. is vs_row +1*/

  (rs_param.added_pts == 0) ? (row_num = rs_param.vs_row+1) : (row_num = rs_param.vs_row);		
  j = 1;
  k = 0;					/* counter for sections of added points */
  
  final_disp = *(disp_ptr + (rs_param.last_row-rs_param.vs_row-1+total_added_pts));	
  while( row_num <= rs_param.last_row )
    {
      alpha = exp((mu - rs_param.muf - rs_param.b2*psi2 - rs_param.b1*psi1)/rs_param.a);
      wmu_muf_on_A = (mu - rs_param.muf)/rs_param.a;
      J1 = H*(Vo_dc1 * alpha * (psi1 * omB1_on_A - B2_on_A*psi2 + wmu_muf_on_A) );
      K1 = H*(Vo_dc2 * alpha * (psi2 * omB2_on_A - B1_on_A*psi1 + wmu_muf_on_A) );
      M1 = H*  K_s_vf * (1.00 - alpha)  ; 
      
      w_psi1 = psi1 + J1*0.50000000000;  
      w_psi2 = psi2 + K1*0.50000000000;
      w_mu = mu + M1*0.50000000000; 
      alpha = exp((w_mu - rs_param.muf - rs_param.b2*w_psi2 - rs_param.b1*w_psi1)/rs_param.a);
      wmu_muf_on_A = (w_mu - rs_param.muf)/rs_param.a;
      J2 = H*( Vo_dc1 * alpha * (w_psi1 * omB1_on_A - B2_on_A*w_psi2 + wmu_muf_on_A) );
      K2 = H*( Vo_dc2 * alpha * (w_psi2 * omB2_on_A - B1_on_A*w_psi1 + wmu_muf_on_A) );
      M2 = H*  K_s_vf * (1.00000 - alpha)  ;
      

      w_psi1 = psi1 + J2*0.500000000000;
      w_psi2 = psi2 + K2*0.500000000000;
      w_mu = mu + M2*0.500000000000; 
      alpha = exp((w_mu - rs_param.muf - rs_param.b2*w_psi2 - rs_param.b1*w_psi1)/rs_param.a);
      wmu_muf_on_A = (w_mu - rs_param.muf)/rs_param.a;
      J3 = H*( Vo_dc1 * alpha * (w_psi1 * omB1_on_A - B2_on_A*w_psi2 + wmu_muf_on_A) );
      K3 = H*( Vo_dc2 * alpha * (w_psi2 * omB2_on_A - B1_on_A*w_psi1 + wmu_muf_on_A) );
      M3 = H*  K_s_vf * (1.00000 - alpha)  ;
      
      
      w_psi1 = psi1 + J3;  
      w_psi2 = psi2 + K3;
      w_mu = mu + M3;   
      alpha = exp((w_mu - rs_param.muf - rs_param.b2*w_psi2 - rs_param.b1*w_psi1)/rs_param.a);
      wmu_muf_on_A = (w_mu - rs_param.muf)/rs_param.a;
      J4 = H*( Vo_dc1 * alpha * (w_psi1 * omB1_on_A - B2_on_A*w_psi2 + wmu_muf_on_A) );
      K4 = H*( Vo_dc2 * alpha * (w_psi2 * omB2_on_A - B1_on_A*w_psi1 + wmu_muf_on_A) );
      M4 = H*  K_s_vf * (1.00000 - alpha)  ; 
      

      old_psi1 = psi1;		/* save last vals */
      old_psi2 = psi2;
      old_mu = mu;
      old_H  = H;
      old_v = v;
      old_x = x;

      psi1 +=  (J1 + 2.00000*J2 + 2.00000*J3 + J4) * 0.166666666666666666666667;  /*psi1[n+1] = psi1[n] + .*/
      psi2 +=  (K1 + 2.00000*K2 + 2.00000*K3 + K4) * 0.166666666666666666666667;  /*psi2[n+1] = psi2[n] + .*/
      mu   +=  (M1 + 2.00000*M2 + 2.00000*M3 + M4) * 0.166666666666666666666667;



      v = rs_param.vf * exp( (mu - rs_param.muf - rs_param.b1*psi1 -rs_param.b2*psi2)/rs_param.a );
      x += rs_param.vf * H;
      
      H = h / v;
	
      if( isnan(mu) || isnan(v) || (v > BIGNUM) )
	{
	  if(++calc_bombed == 5)		/* give up if problem persists */
	    {
	      sprintf(msg, "calc bombed *out* on: a=%g, b1=%g, b2=%g, dc1=%g, dc2=%g, -Simplex continuing\n",rs_param.a,rs_param.b1,rs_param.b2,rs_param.dc1,rs_param.dc2);
	      print_msg(msg);
	      
	      return(BIGNUM);		/* inform simplex of problem*/
	    }
	  else
	    {
	      psi1 = old_psi1;	/* install old vals */
	      psi2 = old_psi2;
	      mu   = old_mu;
	      H    = old_H/2.00;	/* reduce H -stablize calc? */
	      v    = old_v;
	      x    = old_x;
	    }
	} 
      else
	{
	  calc_bombed = 0;		/* reset */
	  
	  
	  if( x >= *disp_ptr )	 /* gone past x point */
	    { 
	      if(adjust_H)			/* adjust_H starts = TRUE */	
		{		/* adjust H to get mu at right disp*/
		  psi1 = old_psi1; /* start calc from old vals */ 
		  psi2 = old_psi2;
		  mu   = old_mu;
		  v    = old_v;
		  x    = old_x;
		  H = fabs(x-*disp_ptr)/rs_param.vf;
		  adjust_H = FALSE;
		}
	      else		/* at correct disp; calc error etc.*/
		{
		  adjust_H = TRUE;	/* ready for next pt*/
		  
		  if( fabs(*disp_ptr - x) > not_close)
		    {
		      sprintf(msg, "\nProblem in rate/state calc: row# %d, not comparing mu at appropriate x\n", row_num);
		      print_msg(msg);
		      
		      return(-2); /* not comparing mu at appropriate x */			
		    }
		  
		  
		  /* weighting criteria set up in look_funcs.c */
		  if(row_num <= rs_param.weight_pts )		/* put extra weight on some portion of initial data */
		    {
		      if( k < (rs_param.peak_row-rs_param.vs_row) && rs_param.added_pts != 0)
			{
			  /*heavily favor overestimate for points between vs_row 
			    and peak row */
			  if(rs_param.weight_control < 0)	
			    {
			      /* overestimate better than underestimate */
			      if( fabs(mu - rs_param.muo) > fabs(*mu_ptr - rs_param.muo) )
				error += 0.1*rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
			      else
				error += 10.0*rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
			    }
			  else
			    error += rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
			}	
		      else if( row_num == rs_param.peak_row )
			{
			  if(rs_param.weight_control < 0)	
			    {
			      /* overestimate better than underestimate */
			      if( fabs(mu - rs_param.muo) > fabs(*mu_ptr - rs_param.muo) )
				error += 0.200*rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
			      else
				error += 50.000*rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
			    }
			  else
			    error += 6.0*rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
			}
		      else				/* past peak row or no added points, but still weighting */
			{
			  if(rs_param.weight_control < 0)
			    {
			      if( row_num < rs_param.peak_row )
				{
				  /* overestimate better than underestimate */
				  if( fabs(mu - rs_param.muo) > fabs(*mu_ptr - rs_param.muo) )                                                         
				    error += rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
				  else
				    error += 5.000*rs_param.weight*(*mu_ptr - mu)*(*mu_ptr -
										   mu) ;
				}
			      else
				{
				  /* underestimate better than overestimate */
				  if( fabs(mu - rs_param.muo) < fabs(*mu_ptr - rs_param.muo) )
				    error += rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
				  else
				    error += 5.000*rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
				}
			    }
			  else
			    error += rs_param.weight*(*mu_ptr - mu)*(*mu_ptr - mu) ;
			}
		    }
		  else
		    error += (*mu_ptr - mu)*(*mu_ptr - mu) ;
		  
		  inc_pt = TRUE;
		  
		  while(inc_pt)	/* increm. until next_x > curr. x */
		    {
		      disp_ptr++;				/* increment pointers */
		      mu_ptr++;
		      *model_mu_ptr++ = mu;			/* save mu value */

/* for debuging... 
printf("x=%g\tdisp_ptr=%g\tmu=%g\tmu_ptr=%g\trow_num=%d\n",x, *(disp_ptr-1), mu, *(mu_ptr-1),row_num);  */
		
		      if( k >= rs_param.peak_row-rs_param.vs_row || rs_param.added_pts == 0 )
			{
			  k++;
			  row_num++;
			}
		      else if( j++ == rs_param.added_pts )
			{
			  row_num++;
			  j = 0; /* start at zero, to allow for real pts <> interp. pts */
			  k++;
			}
		      
		      if(row_num == rs_param.weight_pts )
			{
			  close *= 2.00;
			  not_close *= 2.00;
			  H *= 2.00; /* increase time step after weighted points */
			  /* psuedo = to starting with H = 0.05 instead of 0.01 */
			}
		      if(*disp_ptr <= current_disp)	/* check for noise in disp. tdxr... */
			inc_pt = TRUE;
		      else
			{
			  inc_pt = FALSE; /* don't increment again */
			  current_disp =  *disp_ptr; /* update disp. */
			}
		    } /* end of increment pt loop */		
		} /* end of else for adjust_H test */
	    }	/* end of x > *disp_ptr test */
	}	/* end of else for nan test */
    }		/* end of loop for calc */

  if( row_num-1 != rs_param.last_row )
    {
      disp_ptr = rs_param.disp_data + (rs_param.last_row-rs_param.vs_row-1+total_added_pts-10);

      /* check for noise in disp. tdxr */
      for(k = 0, j = 0; j < 10; ++j)
	{
	  if(*disp_ptr++ > final_disp)
	    k = -1;		/* signify pts out of order */
	}
      
      if( isnan(mu) || isnan(v) )
	return(BIGNUM);			/* signify calc. out of bounds -num. error */
      
      /* got a fair way through calc. if i-1 > j */
      /* guess that problem is noise in disp. tdxr */
      else if(k < 0 && row_num-1 > rs_param.last_row - 10)	
	return(error);			/* no error message, just ignore last pts */
      
      else					/* some other problem */
	{
	  sprintf(msg, "\nProblem in rate/state calc: missed points in error calc or compared the wrong ones \n");
	  print_msg(msg);
	  sprintf(msg, "last row for mu comparison = %d, disp. = %g\n", row_num-1, x);
	  print_msg(msg);
	  return(-3);		/* missed points in error calc -or compared the wrong ones */
	}
    }
  else
    return(error);		/* cumulative error -for simplex */
  
}


