#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <xview/panel.h>
#include "global.h"

extern Panel_item cmd_panel_item;
extern char msg[MSG_LENGTH];



void nocom(str)
     char *str;
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
int getcol(str, num)
     char *str;
     int num;
{
  int i=0;
  int colnum;
  char temp_string[256];
  char buf[8];
  char *strtok();

  if(num < 3)	
  {
  	sprintf(msg, "ERROR: getcol called with <3 args, see strcmd.c\n Sorry, try again.\n");
  	print_msg(msg);
	return(-1);
  }
  else		/*ok, go ahead*/
  {
	strcpy(buf," ,");	/*separators*/
	strcpy(temp_string,str);	/*use tmp copy*/

	strtok(temp_string,buf);	/*make a first call*/
  	for(i=0;i<num-2;++i)
		strtok(NULL,buf);	
	sscanf(strtok(NULL,buf),"%d",&colnum);

  return(colnum);
  }
}

char * strip(str, num)
     char *str;
     int num;
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
	  /*	  printf("found a separator\n"); */
	  
	  while (tmp[n]==' ' || tmp[n]==',' || tmp[n]=='\t' || tmp[n]=='\n')
	    {
	      n++;
	    }
	  /*	  printf("found a new arg\n"); */
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

void stripper(str, num)
     char *str;
     int num;
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
	  xv_set(cmd_panel_item, PANEL_VALUE, &str[n], NULL);
	  break;
	}
      
      if (str[n] == ' ' || str[n] == ',' || str[n] == '\t' || str[n] == '\n')
	{
	  i++;
	  /*	  printf("found a separator\n"); */
	  stop = n;
	  
	  while (str[n]==' ' || str[n]==',' || str[n]=='\t' || str[n]=='\n')
	    {
	      n++;
	    }
	  /*	  printf("found a new arg\n"); */
	  continue;
	}
      n++;
    }  
  str[stop] = '\0';
  
  /*  printf("[%s],[%s] : [%s]\n", str, &str[0], &str[n]);   */
}

