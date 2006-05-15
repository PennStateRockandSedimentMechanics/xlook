#include <math.h>

#include <config.h>
#include <global.h>
#include <array.h>

extern int action;
extern char msg[256];


/* -------------------------------------------------------------------------*/
void zero(col,rec)
int *col , *rec ;
{
   static int i ;
   static float temp_fl ;
   temp_fl = darray[*col][*rec] ;
   for( i = 0; i < head.ch[*col].nelem; ++i)
   {
      darray[*col][i] -= temp_fl ;
   }
}
/* -------------------------------------------------------------------------*/
void offset_int(rec,col,rec2,ts)
int *rec , *col , *rec2;
char *ts;
{
   static int i ;
   static float temp_fl ;
   temp_fl = darray[*col][*rec2] - darray[*col][*rec] ;
   if(*ts == 'y') 		
   			/*set values between rec1 and rec2 equal to rec 1*/
   {
     for( i = *rec; i < *rec2; ++i)
	darray[*col][i] = darray[*col][*rec];
   }

   for( i = *rec2; i < head.ch[*col].nelem; ++i)
   {
      darray[*col][i] -= temp_fl ;
   }
}
/* -------------------------------------------------------------------------*/
void offset(rec,col,rec2,col2)
int *rec , *col , *rec2 , *col2;
{
   static int i ;
   static float temp_fl ;
   temp_fl = darray[*col2][*rec2] - darray[*col][*rec] ;
   for( i = 0; i < head.ch[*col].nelem; ++i)
   {
      darray[*col][i] += temp_fl ;
   }
}
/* -------------------------------------------------------------------------*/
void summation(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 1; i < head.ch[*old].nelem; ++i)
   {
	darray[*new][i] = darray[*old][i] + darray[*new][i-1] ;
   }
}
/* -------------------------------------------------------------------------*/
void expvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = exp(darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void lnvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = log(darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void sinvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = sin((double)darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void cosvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = cos((double)darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void tanvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = tan((double)darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void asinvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = asin((double)darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void acosvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = acos((double)darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void atanvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = atan((double)darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void logvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = log10(darray[*old][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void recipvec(old,new)
int *old , *new ;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = 1.0/darray[*old][i] ;
   }
}
/* -------------------------------------------------------------------------*/
void powcvec(old,new,power_col)
int *old , *new ;
int *power_col;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = (float)pow((double)darray[*old][i],(double)darray[*power_col][i]) ;
   }
}
/* -------------------------------------------------------------------------*/
void powvec(old,new,power)
int *old , *new ;
float *power;
{
   static int i ;
   for( i = 0; i < head.ch[*old].nelem; ++i)
   {
      darray[*new][i] = (float)pow((double)darray[*old][i],(double)*power) ;
   }
}
/* -------------------------------------------------------------------------*/
void elastic_corr(ad_c, tau_c, new_col, first, last, E_on_L)

int ad_c, tau_c, new_col;		/*ad_c= column that contains axial displacement*/
int first, last;
float E_on_L;
{

  int i,n_data;
  int row, nrb;
  float *disp;

	n_data = last-first;
	row = first;
	nrb = row-1;
						/*first point of the corrected disp. column is the same as that from the uncorrected */

	disp = (float *) malloc(n_data*sizeof(float));	 /* in case they chose same col for op */
	/*disp = (float *) calloc(n_data,sizeof(float));	 */
	for(i=first; i<=last;++i)
		disp[i-first] = darray[ad_c][i];
	
	darray[new_col][row] = darray[ad_c][row];

	for(i=0;i<n_data;++i)
	{
		nrb++;
		row++;
		darray[new_col][row] = ( darray[new_col][nrb] + (disp[i+1]-disp[i]) -
					 (darray[tau_c][row]-darray[tau_c][nrb])/E_on_L ) ;
	
	}
	free( (void *) disp );
}
/* -------------------------------------------------------------------------*/
void
shear_strain(disp_col, thick_col, strain_col, first, last)

int disp_col, thick_col, strain_col;  
int first, last;
{

  int i,n_data;
  int row, orb;
  float *disp;

        n_data = last-first;
        orb = row = first;

        disp = (float *) malloc(n_data*sizeof(float));   /* in case they chose s */
        /*disp = (float *) calloc(n_data,sizeof(float));  */ 

        for(i=first; i<=last;++i)
                disp[i-first] = darray[disp_col][i];

        darray[strain_col][row] = darray[disp_col][row]/darray[thick_col][row];

        for(i=0;i<n_data;++i)
        {
                row++;
                darray[strain_col][row] = ( darray[strain_col][orb] + 
				(disp[i+1]-disp[i]) /
				((darray[thick_col][row]+darray[thick_col][orb])/2.00) );
                orb++;
        
        }
	free( (void *) disp);
}
/* -------------------------------------------------------------------------*/
void
calc_geom_thin(disp_col, new_col, L, h_h )
 
int disp_col, new_col;
float L, h_h; 
{
 
  int i; 
 
 
        /* calculate geometric thinning during direct shear test for */   
        /* calculation is:   del_h = h dx/2L ; where h is initial thickness, dx is slip increment and L is length of the sliding block parallel to slip */
        /* see lookv3.c "cgt" for more info */

        L *=  2.00000;

        darray[new_col][0] = h_h;

        for(i=1; i<head.ch[disp_col].nelem; ++i)
        {
          h_h -= (darray[disp_col][i]-darray[disp_col][i-1]) * h_h/L;
          darray[new_col][i] =  h_h; 
        }



}

/* -------------------------------------------------------------------------*/
void
geom_thin(disp_col, gouge_thick_col, new_col, L)
 
int disp_col, gouge_thick_col, new_col; 
float L;
{

  int i;
  double del_h = 0;


        /* correct horizontal displacement measurement during direct shear test for "geometric thinning" */
        /* correction is:   del_h = h dx/2L ; where h is thickness, dx is slip increment and L is length of the sliding block parallel to slip */
	/* see lookv3.c "rgt" for more info */

	L *=  2.00000;

        darray[new_col][0] = darray[gouge_thick_col][0];

        for(i=1; i<head.ch[disp_col].nelem; ++i)
	{
	  del_h += (darray[disp_col][i]-darray[disp_col][i-1]) * darray[gouge_thick_col][i-1]/L; 
          darray[new_col][i] =  darray[gouge_thick_col][i-1]  + del_h;
	}
			
        
        
}

/* -------------------------------------------------------------------------*/
void
vol_corr(vs_c, tau_c, new_col, first, last, SV_on_V)

int vs_c, tau_c, new_col;                       /*vs_c= column that contains axial displacement*/
					/* SV_on_V = S*Va/V */
int first, last;
float SV_on_V;
{

  int i,n_data;
  int row, nrb;

        n_data = last-first;
        row = first;
        nrb = row-1;
                                                /*first point of the corrected vs column is
the same as that from the uncorrected */

/* corrected vs = vs - dtau*S*Va/V  NOTE that dtau=dPc for the constant normal stress loading conditions and a gouge layer at 45 degrees to sigma_1 */

        darray[new_col][row] = darray[vs_c][row];

        for(i=0;i<n_data;++i)
        {
                nrb++;
                row++;
                darray[new_col][row] = ( darray[new_col][nrb] +
                                         darray[vs_c][row]-darray[vs_c][nrb] -
                                         (darray[tau_c][row]-darray[tau_c][nrb])*SV_on_V ) ;
        
        }
}
/* -------------------------------------------------------------------------*/
void deriv(xcol,ycol,new_col,first,last)
int *xcol, *ycol, *new_col;
int first, last;

{
 double denom;
 float *next[2];
 float  aa[2], bb[2], cc[2], dydx;
 int 	i, j, n_data;

	n_data = (last - first ) +1;
	next[0] = &(darray[*xcol][first]);
	next[1] = &(darray[*ycol][first]);

	for(j=0; j<2; ++j){
		aa[j] = *next[j];
		next[j] += 1;
		bb[j] = *next[j];	
		next[j] += 1;
		cc[j] = *next[j];	
	}	


/*form is:	[ dydx = ep2*ep2*f(x+ep1) - ep1 * ep1* (f(x-ep2) - (ep2*ep2-ep1*ep1)* f(x) ] / (ep2*ep2*ep1 + ep1*ep1*ep2)*/

/* ep1 		= cc[0] - bb[0]         */
/* ep2 		= bb[0] - aa[0]         */
/* f(x+ep1) 	= cc[1]             	*/
/* f(x)         = bb[1]         	*/
/* f(x-ep2) 	= aa[1]             	*/

	denom=(fabs(cc[0]-bb[0])*(bb[0]-aa[0])*(bb[0]-aa[0]))+(fabs(bb[0]-aa[0])*(cc[0]-bb[0])*(cc[0]-bb[0]));

	dydx = ((bb[0]-aa[0])*(bb[0]-aa[0])*cc[1] - ((cc[0] -bb[0])*(cc[0]-bb[0])*aa[1]) - ((bb[0]-aa[0])*(bb[0]-aa[0])-(cc[0]-bb[0])*(cc[0]-bb[0]))*bb[1] ) / denom;

	darray[*new_col][first] = dydx;
	darray[*new_col][first+1] = dydx;

	n_data -= 3;	/*go until 1 before the last point*/

	for(i=0; i<n_data; ++i)
	{
       	   for(j=0; j<2; ++j)
       	   {
       	           next[j] -= 1;
       	           aa[j] = *next[j];
        
       	           next[j] += 1;
       	           bb[j] = *next[j];

       	           next[j] += 1;
       	           cc[j] = *next[j];
       	   }
       	   denom=(fabs(cc[0]-bb[0])*(bb[0]-aa[0])*(bb[0]-aa[0]))+(fabs(bb[0]-aa[0])*(cc[0]-bb[0])*(cc[0]-bb[0])); 

       	   dydx = ((bb[0]-aa[0])*(bb[0]-aa[0])*cc[1] - ((cc[0] -bb[0])*(cc[0]-bb[0])*aa[1]) - ((bb[0]-aa[0])*(bb[0]-aa[0])-(cc[0]-bb[0])*(cc[0]-bb[0]))*bb[1] ) / denom;
       	 
 /* fprintf(stderr,"x= %f %f\n",bb[0], dydx); */

	   darray[*new_col][first+i+2] = dydx;

	}

	darray[*new_col][last] = dydx;

}
/* -------------------------------------------------------------------------*/
void add(a,b,f,c,to,first,last)
int *a , *b, *c ;
char *to ;
float *f ;
int first , last ;
{
int i ;

switch(*to)
{
   case '=':for ( i = first; i <= last; ++i)
            {
            darray[*c][i] = darray[*a][i] + (*f) ;
            }
            break ;
   case ':':for ( i = first; i <= last; ++i)
            {
            darray[*c][i] = darray[*a][i] + darray[*b][i] ;
            }
            break ;
}
}
/* -------------------------------------------------------------------------*/
void sub(a,b,f,c,to,first,last)
     int *a , *b, *c ;
     char *to ;
     float *f ;
     int first , last ;
{
  int i ;

  
  switch(*to)
    {
    case '=':
      for ( i = first; i <= last; ++i)
	{
	  darray[*c][i] = darray[*a][i] - (*f) ;
	}
      break ;
    case ':':
      for ( i = first; i <= last; ++i)
	{
	  darray[*c][i] = darray[*a][i] - darray[*b][i] ;
	}
      break ;
    }
}
/* -------------------------------------------------------------------------*/
void prod(a,b,f,c,to,first,last)
int *a, *b, *c ;
char *to ;
float *f ;
int first , last ;
{
int i;


switch(*to)
{
   case '=':for (i = first; i <= last; ++i)
           {
           darray[*c][i] = darray[*a][i] * (*f) ;
           }
            break ;
   case ':':for ( i = first; i <= last; ++i)
            {
            darray[*c][i] = darray[*a][i] * darray[*b][i] ;
            }
            break ;
}
}
/* -------------------------------------------------------------------------*/
void mydiv(a,b,f,c,to,first,last)
int *a, *b, *c ;
char *to ;
float *f ;
int first , last ;
{
int i;


switch(*to)
{
   case '=':for (i = first; i <= last; ++i)
           {
           darray[*c][i] = darray[*a][i] / (*f) ;
           }
            break ;
   case ':':for ( i = first; i <= last; ++i)
            {
            darray[*c][i] = darray[*a][i] / darray[*b][i] ;
            }
            break ;
}
}
