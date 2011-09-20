#include <stdio.h>
#include <string.h>

#include "config.h"
#include "global.h"
#include "messages.h"
#include "strcmd.h"
#include "ui.h"

/* "private" function prototypes */


void nocom(
	char *str)
{
	int n=0;

	/* 6 Nov. 1995, removed. Looks useless, this func just replaces commas with blanks*/
	/*  while (str[n]==' ' || str[n]==',' || str[n]=='\t' || str[n]=='\n')
	{
		n++;
	}*/

	while (str[n] != '\0')
	{
		if (str[n] == ',') str[n] = ' ';
		n++;
	}  
}


/* return the number of the col to be named, this will be in token # "num" */
int getcol(
     char *str,
     int num)
{
	int i=0;
	int colnum;
	char temp_string[256];
	char buf[8];
	char *strtok();
	int token_count();

	if(num < 3)	
	{
		print_msg("ERROR: getcol called with <3 args, see strcmd.c\n Sorry, try again.\n");
		return(-1);
	}
	else		/*ok, go ahead*/
	{
		temp_string[0] = '\0';

		strcpy(buf," ,");	/*separators*/
		strcpy(temp_string,str);	/*use tmp copy*/

		if(token_count(str) <= 2)	/*try to avoid the problem of user changing their mind and just typing one letter*/
			return(-1);

		/*fprintf(stderr,"colnum=%d in getcol %s, len=%d\n", colnum, str, (int)strlen(str));
		fprintf(stderr,"tokens=%d, in getcol %s, len=%d\n", token_count(str),str, (int)strlen(str));*/

		strtok(temp_string,buf);	/*make a first call*/
		for(i=0;i<num-2;++i)
			strtok(NULL,buf);	
		sscanf(strtok(NULL,buf),"%d",&colnum);

		return(colnum);
	}
}

char *strip(
	char *str,
	int num)
{
	char tmp[256];
	int i=0, n=0;

	strcpy(tmp, str);

	while (tmp[n] != '\0')
	{
		if (i == num) break;
		if (tmp[n] == ' ' || tmp[n] == ',' || tmp[n] == '\t' || tmp[n] == '\n')
		{
			i++;
			/*  printf("found a separator\n"); */

			while (tmp[n]==' ' || tmp[n]==',' || tmp[n]=='\t' || tmp[n]=='\n')
			{
				n++;
			}
			/*  printf("found a new arg\n"); */
			continue;
		}
		n++;
	}  

	/*  printf("[%s],[%s] : [%s]\n", str, &str[0], &str[n]);  */
	if (i != num) 
		return (NULL);

	return(&str[n]);
}

/* splits a string into two at a location specified by num. after num words, puts a '\0' into the input string and sends the rest of the string to command line. this procedure modifies the input string! */ 

void stripper(
	char *str, 
	int num)
{
	int i=0, n=0;
	int stop = 0;

	while (str[n]==' ' || str[n]==',' || str[n]=='\t' || str[n]=='\n')
	{
		n++;
	}

	while (str[n] != '\0')
	{
		if (i == num) 
		{
			set_command_text(&str[n]);
			break;
		}

		if (str[n] == ' ' || str[n] == ',' || str[n] == '\t' || str[n] == '\n')
		{
			i++;
			/*  printf("found a separator\n"); */
			stop = n;

			while (str[n]==' ' || str[n]==',' || str[n]=='\t' || str[n]=='\n')
			{
				n++;
			}
			/*  printf("found a new arg\n"); */
			continue;
		}
		n++;
	}  
	str[stop] = '\0';
	/*  printf("[%s],[%s] : [%s]\n", str, &str[0], &str[n]);   */
}

