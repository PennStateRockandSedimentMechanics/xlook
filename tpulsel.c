/* tpulse - Solves the analytical solution of Carslaw and Jaeger for the  */
/*		pulse decay problem. The solution is given in section 3:13:v  */
/*		The solution for pulse decay with no sample storage is also   */
/*		given for comparison.					      */
/*									      */
/*   version adapted for use with lookv3				      */
/*									      */
/*	bugs  - does not seem to work with 200 roots			      */
/*									      */

#include "global.h"

double tpulse(perm,sstor,length,vol,time)
double perm,sstor,length,vol,time;
{
	double root();
	double ezhead, truehead, diffuse, cond, area;
	double rhog, visc, rt[200];
	double inc,accur,rootsq,num,denom,node,beta,por,limit;
	int i,nroots;
	double ehch;

	nroots = 50;
	accur = 1.0e-09;
	area = 9.58e-04;
	beta = 4.58e-10;
	visc = 0.001;
	rhog = 9800.0;
	por = sstor/rhog/beta;
	ehch = por*area/vol;
	inc = PI/100.0/length;

	for (i=0; i<nroots; ++i)
	{
		node = (double)i*PI/length;
		limit = ((double)i+(0.5))*PI/length;
		rt[i] = root(&node,ehch,length,&inc,limit,accur);
	}
	cond = rhog*perm/visc;
	truehead = 0.0;
	ezhead = 0.0;
	diffuse = cond/sstor;
	for (i=0; i<nroots; ++i)
	{
		rootsq = pow(rt[i],2.0);
		num = 2.0*(rootsq+ehch*ehch)*exp((-diffuse*rootsq*time))*sin(rt[i]*length);
		denom = rt[i]*(length*(rootsq+ehch*ehch)+ehch);
		truehead -= num/denom;
	}
/*	ezhead = exp((-cond*area*time/beta/vol/length/rhog)); */
return(-truehead);
}
/*-----------------------------------------------------*/
double atpulse(perm,sstor,length,vol,time,beta,visc)
double perm,sstor,length,vol,time,beta,visc;
{
	double root();
	double ezhead, truehead, diffuse, cond, area;
	double rhog, rt[200];
	double inc,accur,rootsq,num,denom,node,por,limit;
	int i,nroots;
	double ehch;

	nroots = 50;
	accur = 1.0e-09;
	area = 9.58e-04;
	rhog = 9800.0;
	por = sstor/rhog/beta;
	ehch = por*area/vol;
	inc = PI/100.0/length;

	for (i=0; i<nroots; ++i)
	{
		node = (double)i*PI/length;
		limit = ((double)i+(0.5))*PI/length;
		rt[i] = root(&node,ehch,length,&inc,limit,accur);
	}
	cond = rhog*perm/visc;
	truehead = 0.0;
	ezhead = 0.0;
	diffuse = cond/sstor;
	for (i=0; i<nroots; ++i)
	{
		rootsq = pow(rt[i],2.0);
		num = 2.0*(rootsq+ehch*ehch)*exp((-diffuse*rootsq*time))*sin(rt[i]*length);
		denom = rt[i]*(length*(rootsq+ehch*ehch)+ehch);
		truehead -= num/denom;
	}
/*	ezhead = exp((-cond*area*time/beta/vol/length/rhog)); */
return(-truehead);
}
/*    function =root                             */
/*                                  */
/*                                */
/*                                              */
#define TRUE 1
#define FALSE 0
#define EQUATION  (var*tan(var*ll)-hh)

double root(a,hh,ll,b,limit,accuracy)
double *a,hh,ll,*b,limit,accuracy;

{

     double var,loop;
     double result[2];
     int root_found = FALSE;
     int first_loop = TRUE;

     var= *a;
     loop= *b;


while ( var < limit && root_found == FALSE )
{
     result[0] = EQUATION;

     if ( first_loop == TRUE )
     {
          result[1] = result [0];
          first_loop = FALSE;
     }
     else
     {
        var += loop;
	if(var>limit) var = limit;
/*	fprintf(stderr,"%le ",var); */
     }

     if ( (result[0] < 0.0 && (result[0]*-1.0) < accuracy) || (result[0] > 0.0 && result[0] < accuracy ) )
     {
/*        fprintf(stderr,"root at %le\n",var-loop);*/
          root_found = TRUE;
     }
     else if ( result[0]/result[1] < 0.0 )
     {
     		if ( (result[0] < 0.0 && (result[0]*-1.0) < accuracy) || (result[0] > 0.0 && result[0] < accuracy ) )
               {
/*                    fprintf(stderr,"root between %le and %le\n",var-2.0*loop,var-loop);*/
                    root_found = TRUE;
               }
               else
               {
                    var -= 2.0*loop;
                    loop = loop/10.0;
                    first_loop = TRUE;
/*                    fprintf(stderr,"flag indicating change of increment %e\n",var);*/
               }
     }
     result[1] = result[0];

}
*a = var-loop;
return (var-loop);
}
