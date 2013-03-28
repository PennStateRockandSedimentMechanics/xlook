#include <stdio.h>
#include <math.h>

#include <config.h>
#include <global.h>
#include <perrfl.h>

#define PI 3.141592654

void perrf(Kamp,Kph,lgth,Vol,Per,pGperm,pGdif,psstor)
double Kamp,Kph,lgth,Vol,Per,*pGperm,*pGdif,*psstor;
{

	static double cond, area, omega, perm, diffuse;
	static double beta;
	static double kay, rhog, visc;
	static double alpha, gamma, lambda;
	static double Ag, Pg, Dalp, Dgam, Num, Dom;
	static double sdg, efpor;
	static double Gperm, Gdif, sstor;

	Gperm = *pGperm;
	Gdif = *pGdif;
	sstor = *psstor;

	beta = 4.58e-10;
	rhog = 9800.0;
	visc = 0.001;
	area = 9.58e-04;

	cond = rhog*Gperm/visc;
	Gdif = cond/sstor;

	omega = 2.0*PI/Per;
	kay = sqrt(omega/(2.0*Gdif)) ;
	lambda = Gperm*area/visc/beta/Vol;
	alpha = lambda/pow(2.0*omega*Gdif,0.5);
	gamma = omega*lgth/pow(2.0*omega*Gdif,0.5);

	Ag = 4.0*alpha*alpha/((2.0*alpha*alpha+1.0)*cosh(2.0*gamma)+(2.0*alpha*alpha-1.0)*cos(2.0*gamma)+2.0*alpha*(sinh(2.0*gamma)-sin(2.0*gamma)));
	Ag = sqrt(Ag);
   	Num = -(sin(gamma)+tanh(gamma)*(2*alpha*sin(gamma)+cos(gamma)));
	Dom = (tanh(gamma)+2*alpha)*cos(gamma)-sin(gamma);
	Pg = atan(Num/Dom);
	if(Pg < 0) Pg=-Pg;
	if(Pg > 2*PI) Pg=Pg-2*PI;
	if(Pg > PI) Pg=Pg-PI;
	if (Dom > 0)
	  {if (Num > 0)
	    {Pg = Pg-2*PI;}
	     else {Pg = -Pg;}}
	else
	  {if (Num > 0)
	    {Pg = -Pg-PI;}
	     else {Pg = Pg-PI;}};	            
	alpha = 0.1;
	Dgam = 0.1;
	gamma = 0.1;
	while(Dgam>1.0e-09)
	{
	Dalp = 1.0;
	Ag = 4.0*alpha*alpha/((2.0*alpha*alpha+1.0)*cosh(2.0*gamma)+(2.0*alpha*alpha-1.0)*cos(2.0*gamma)+2.0*alpha*(sinh(2.0*gamma)-sin(2.0*gamma)));
	Ag = sqrt(Ag);

	while(fabs(Ag-Kamp) > 1.0e-10)
	{
		if(Ag>Kamp)
		{
			sdg = -1.0;
			while(Ag>Kamp)
			{
				alpha += (Dalp*sdg);
				if(alpha <= Dalp/10.0) { alpha=Dalp; Dalp/=10.0; if(Dalp<1.0e-08) break;}
				Ag = 4.0*alpha*alpha/((2.0*alpha*alpha+1.0)*cosh(2.0*gamma)+(2.0*alpha*alpha-1.0)*cos(2.0*gamma)+2.0*alpha*(sinh(2.0*gamma)-sin(2.0*gamma)));
				Ag = sqrt(Ag);
			}
			Dalp /= 10.0;
		}
		if(Ag<Kamp)
		{
			sdg = 1.0;
			while(Ag<Kamp)
			{
				alpha += (Dalp*sdg);
				Ag = 4.0*alpha*alpha/((2.0*alpha*alpha+1.0)*cosh(2.0*gamma)+(2.0*alpha*alpha-1.0)*cos(2.0*gamma)+2.0*alpha*(sinh(2.0*gamma)-sin(2.0*gamma)));
				Ag = sqrt(Ag);
			}
			Dalp /= 10.0;
		}
	}
        Num = -(sin(gamma)+tanh(gamma)*(2*alpha*sin(gamma)+cos(gamma)));
	Dom = (tanh(gamma)+2*alpha)*cos(gamma)-sin(gamma);
	Pg = atan(Num/Dom);
	if(Pg < 0) Pg=-Pg;
	if(Pg > 2*PI) Pg=Pg-2*PI;
	if(Pg > PI) Pg=Pg-PI;
	if (Dom > 0)
	  {if (Num > 0)
	    {Pg = Pg-2*PI;}
	     else {Pg = -Pg;}}
	else
	  {if (Num > 0)
	    {Pg = -Pg-PI;}
	     else {Pg = Pg-PI;}}	
        perm = visc*beta*Vol*alpha*omega*lgth/area/gamma;
	diffuse = omega*lgth*lgth/2.0/gamma/gamma;
	if(Kph > Pg)
	{
		gamma -= Dgam;
		Dgam /= 10.0;
	}
	else { gamma += Dgam; }
	
	}
        Num = -(sin(gamma)+tanh(gamma)*(2*alpha*sin(gamma)+cos(gamma)));
	Dom = (tanh(gamma)+2*alpha)*cos(gamma)-sin(gamma);
	Pg = atan(Num/Dom);
	if(Pg < 0) Pg=-Pg;
	if(Pg > 2*PI) Pg=Pg-2*PI;
	if(Pg > PI) Pg=Pg-PI;
	if (Dom > 0)
	  {if (Num > 0)
	    {Pg = Pg-2*PI;}
	     else {Pg = -Pg;}
	}
	else
	  {if (Num > 0)
	    {Pg = -Pg-PI;}
	     else {Pg = Pg-PI;}
	}	
	perm = visc*beta*Vol*alpha*omega*lgth/area/gamma;
	diffuse = omega*lgth*lgth/2.0/gamma/gamma;
	cond = rhog*perm/visc;
	sstor = cond/diffuse;
	efpor = sstor/rhog/beta;

*pGperm = perm;
*psstor = sstor;
fprintf(stderr,"%le\t%le\n",perm,sstor);
return;
}
