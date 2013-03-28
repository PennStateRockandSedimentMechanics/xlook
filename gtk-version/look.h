#include        <math.h>
#include	<stdio.h>

#define    MAX_COL   33
/*---------------------------------------------------------------*/
struct	channel {
	char	name[13];
	char	units[13];
	float	gain;
        char    comment[50];
        int     nelem;
};

struct	lheader {
	char	title[20];
        int     nrec;
	int	nchan;
	int	swp;
	float	dtime;
	struct	channel	ch[MAX_COL];
};

double     *darray[MAX_COL] ;
/*---------------------------------------------------------------*/
