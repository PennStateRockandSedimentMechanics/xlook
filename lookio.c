#include "global.h"

extern int action;
extern int doit;
extern char msg[256];


/********************************* aschead ***********************************/
void aschead_file(ph,file)
     struct header *ph ;
     FILE *file ;
{
  struct header h ;
  int i ;
  char eoh[5] ;
  
  h = *ph;
  fprintf(file,"%s\n",h.title) ;
  fprintf(file,"%d  %d\n",h.nchan,h.nrec) ;
  fprintf(file,"%d  %14.7e\n",h.swp,h.dtime) ;
  for ( i = 1; i < MAX_COL; ++i)
    {
      if(strlen(h.ch[i].comment) == 0)	
	strcpy(h.ch[i].comment,"none");
      fprintf(file,"%14.7e %s %s %d %s \n",h.ch[i].gain,h.ch[i].name,h.ch[i].units,h.ch[i].nelem,h.ch[i].comment) ;
    } 
  for ( i = 0; i < 5; ++i)
    {
      fprintf(file,"%14.7e\n",h.extra[i]) ;
    }
  strncpy(eoh,"*EOH\0",5);
  fprintf(file,"%s\n",eoh) ;
}

void aschead_scrn(ph)
     struct header *ph ;
{
  struct header h ;
  int i ;
  char eoh[5] ;
  
  h = *ph;
  sprintf(msg,"%s\n",h.title) ;
  print_msg(msg);
  sprintf(msg,"%d  %d\n",h.nchan,h.nrec) ;
  print_msg(msg);
  sprintf(msg,"%d  %14.7e\n",h.swp,h.dtime) ;
  print_msg(msg);
  
  for ( i = 1; i < MAX_COL; ++i)
    {
      if(strlen(h.ch[i].comment) == 0)	
	strcpy(h.ch[i].comment,"none");
      sprintf(msg,"%14.7e %s %s %d %s \n",h.ch[i].gain,h.ch[i].name,h.ch[i].units,h.ch[i].nelem,h.ch[i].comment) ;
      print_msg(msg);
    }
  
  for ( i = 0; i < 5; ++i)
    {
      sprintf(msg,"%14.7e\n",h.extra[i]) ;
      print_msg(msg);
    }
  strncpy(eoh,"*EOH\0",5);
  sprintf(msg,"%s\n",eoh) ;
  print_msg(msg);
}


/********************************* getaschead ****************************/
int getaschead(ph,file)
     struct header *ph ;
     FILE *file ;
{
  struct header h ;
  int i ;
  char title[20] , eoh[5];
  int rec , chan ;
   
  fscanf(file,"%s",title) ;
  fscanf(file,"%d %d",&chan,&rec) ;
  if ( chan>=max_col || rec>=max_row )
    {
      sprintf(msg, "INSUFFICIENT ALLOCATION.\n") ; 
      print_msg(msg);
      return 0 ; 
    }

  strcpy(h.title,title) ;
  h.nchan = chan ;
  h.nrec = rec ;
  fscanf(file,"%d %e",&(h.swp),&(h.dtime)) ;
  for ( i = 1; i < MAX_COL; ++i)
    {
      fscanf(file,"%e %s %s %d %s\n",&(h.ch[i].gain),h.ch[i].name,
			h.ch[i].units,&(h.ch[i].nelem),h.ch[i].comment);
    } 
  for ( i = 0; i < 5; ++i)
    {
      fscanf(file,"%e",&(h.extra[i])) ;
    }
  fscanf(file,"%s",&(eoh[0])) ;
  if (strncmp(eoh,"*EOH",4) == 0)
    {
      sprintf(msg, "Header accepted.\n") ;
      print_msg(msg);
      *ph = h ;
      display_active_file(1);
      return 1 ;
    }
  
  fprintf(stderr,"Header not accepted.\n");
  print_msg(msg);
  return 0 ;
}
