#include <config.h>
#include <global.h>
#include <look_funcs.h>
#include <nr.h>
#include <special.h>

/* -------------------------------------------------------------------------*/
void line(colx,coly,first,last,result)
int *colx , *coly , *first , *last;
double result[];		/*has 10 elements*/
{
double y_mean = 0.0, x_mean = 0.0,  sumx=0.0 , sumy=0.0 , sumxy=0.0 , sumx2=0.0 , sumy2=0.0;
double slope, intercept, sum_resid_sq, mean_error, std_slope, std_intercept, corr_coef;
double sqrt();
int i , ndat;

    ndat = *last - *first +1;

    for( i = (*first); i <= (*last); ++i )
    {
        sumx += darray[*colx][i] ;
        sumy += darray[*coly][i] ;
    /*    sumxy += darray[*colx][i] * darray[*coly][i] ;
        sumx2 += darray[*colx][i] * darray[*colx][i] ;*/
    }

    x_mean = sumx/(double)ndat; 
    y_mean = sumy/(double)ndat; 

    for( i = (*first); i <= (*last); ++i )
    {
	sumx2 += (darray[*colx][i]-x_mean)*(darray[*colx][i]-x_mean);
	sumy2 += (darray[*coly][i]-y_mean)*(darray[*coly][i]-y_mean);
	sumxy += (darray[*coly][i]-y_mean)*(darray[*colx][i]-x_mean);
    }

    slope = result[0] = sumxy / sumx2;		
    intercept = result[1] = y_mean-slope*x_mean;    

    for( i = (*first); i <= (*last); ++i )
        sum_resid_sq += ( darray[*coly][i] -slope* darray[*colx][i]-intercept) *
				 (darray[*coly][i]-slope* darray[*colx][i]-intercept);

    mean_error = sqrt(sum_resid_sq/(double)(ndat-2));
    std_slope = result[2] = mean_error/sqrt(sumx2);
 
    std_intercept = result[3] = sqrt( (mean_error*mean_error / (double)ndat) +
			 std_slope*std_slope*x_mean*x_mean) ;

/*fprintf(stderr,"sum_resid_sq=%g, mean_error=%g, std_slope = %g, std_intercept = %g\n", sum_resid_sq, mean_error, std_slope, std_intercept);*/

    corr_coef = result[4] = sumxy/sqrt(sumx2*sumy2);

    result[5] = x_mean;
    result[6] = y_mean;
    result[7] = sqrt(sumx2/(double)ndat);
    result[8] = sqrt(sumy2/(double)ndat);

    /*i = *last - *first + 1;
    result[0] = (sumx * sumy - (double)i * sumxy) /
                               (sumx * sumx - (double)i * sumx2) ;
    result[1] = (sumy - result[0] * sumx) / (double)i ;*/
}
/*------------------------------------------------------------------------*/
void stats(col,first,last)
     int *col , *first , *last ;
{
  int i;   
  double rec;
  double max, min;
  double mean, sum_sq, sum_dev;
  double sqrt();

  max = (double)darray[*col][*first] ;
  min = (double)darray[*col][*first] ;
  mean = sum_sq = 0.0;
  sum_dev = 0.0;
  for( i = (*first); i <= (*last); ++i)
    {
      max = (darray[*col][i]<max) ? max : darray[*col][i] ;
      min = (darray[*col][i]<min) ? darray[*col][i] : min ;
      mean += darray[*col][i] ;
      sum_sq += darray[*col][i] * darray[*col][i] ;
    }
  rec = (double)(*last) - (double)(*first) + 1.0;
  i = *last - *first + 1;
  col_stat.rec = i ;
  col_stat.mean = (mean / (double)rec) ;
  col_stat.max = max ;
  col_stat.min = min ;
  for( i = (*first); i <= (*last); ++i)
    {
      sum_dev += pow((darray[*col][i]-col_stat.mean),2.0);
    }
  col_stat.stddev = sqrt( (sum_dev / (double)(rec-1.0) ) );
}

/* -------------------------------------------------------------------------*/
void median_smooth(col,new_col,start,end,window)
int col, new_col, start, end, window;
{
static double *data, median;
int	i,j,k;
int	half_window;
void	mdian1();

fprintf(stderr,"in special.c\n");

	window=2*(int)(window/2) + 1;                /* force wind_size to be odd */	
	half_window = (int)(window/2);

	data = (double *)malloc((window+1)*sizeof(double));
	
	for(j=start, i=0; i< half_window; ++i)
		darray[new_col][j] = darray[col][j++];	

	for(j=start+half_window, i=start ; i<=end-window+1; ++i)
	{
		for(k=0; k < window; ++k)
			data[k+1] = darray[col][i+k];

		mdian1(data, window, &median);
		darray[new_col][j++] = median;	
	}

	for(i=end-(int)(window/2); i<=end; ++i)
		darray[new_col][i] = darray[col][i];	

	free(data);
}
/* -------------------------------------------------------------------------*/
void smooth(col,new_col,start,end,window)
int col, new_col, start, end, window;
{

	double sum;
	int i,j,k,half_wind;
	
	for(i=start; i<=  end; ++i)
		darray[0][i] = darray[col][i];


	k = half_wind = 0;
	sum = 0;

	window=2*(int)(window/2) + 1;                /* force wind_size to be odd */	
	half_wind = (int)window/2;


	for(i=0; i < half_wind; ++i)		
		sum += darray[0][start];


	for(i=0; i < window; ++i){
	  if(i < half_wind)	
		sum += darray[0][start + i];

	  else {	
		sum += darray[0][start + i];
		darray[new_col][start + k] = sum / (double)window;
		sum -= darray[0][start];
		++k;
	  }
	}
	
	j = window -1;

	for(i=start + window; i <= end; ++i){
		sum += darray[0][i];
		darray[new_col][i-half_wind] = sum / (double)window;
		sum -= darray[0][i-j];
	}
	

	for(i=1; i <= half_wind; ++i){
		sum += darray[0][end];
		darray[new_col][end - half_wind +i] = sum / (double)window;
		sum -= darray[0][end-window];
	}	
}
/* -------------------------------------------------------------------------*/
void o_slope(col1,col2,new_col,start,end,window)
int col1, col2, new_col, start, end, window;
{
        double xsum, sumxy, ysum, sumx2, x_mean, y_mean;
        int i,j,k,l,half_wind;
 
        xsum = ysum = sumxy = sumx2 = 0;

        window=(int)(2*(int)(window/2) + 1);                /* force wind_size to be odd */
        half_wind = (int)(window/2);

        for(i=start; i < start+window-1; ++i)
	{
                xsum += darray[col1][i];
                ysum += darray[col2][i];
	}

	k=start+half_wind;			
	j=start;

						/*start with next val beyond window*/
/*window is from j to i, k is the middle point, where we'll write best fit slope */

        for(i=start+window-1; i <=end; i++, k++, j++)
        {
                xsum += darray[col1][i];		/*add next value*/
                ysum += darray[col2][i];
		x_mean = xsum/(double)window;
		y_mean = ysum/(double)window;
		sumx2 = sumxy = 0;

		for(l=j; l <=i ; l++)
		{
			sumx2 += (darray[col1][l]-x_mean)*(darray[col1][l]-x_mean);
			sumxy += (darray[col1][l]-x_mean)*(darray[col2][l]-y_mean);
		}

                /*darray[new_col][k] = (xsum*ysum - ((double)(window) * sumxy)) /
						(xsum*xsum - ((double)(window) * sumx2)) ;*/

		darray[new_col][k] = sumxy / sumx2;

					/*move window*/
                xsum -= darray[col1][j];		/*remove old val*/
                ysum -= darray[col2][j];
        }
        
			/*deal with ends, by just padding with first rational value*/ 
        for(i=0; i < half_wind; ++i)
		darray[new_col][i+start] = darray[new_col][start+half_wind];

        for(i= 0 ; i < half_wind; ++i)
		darray[new_col][end-i] = darray[new_col][end-half_wind-1];
}
/* -------------------------------------------------------------------------*/
