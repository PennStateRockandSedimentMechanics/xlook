/*    functions RECALL, EXAMIN, RITE, REED, TASC, and OLDREAD for program LOOK     */
#include "global.h"

extern void print_msg();
extern char msg[MSG_LENGTH];
extern int action; 


/***************************** examin *******************************/
/* done */
void examin(dfile)
     FILE *dfile ;
{
  int i ;
  struct header temphead ;
    
  
  fread(&(temphead.title[0]),1,20,dfile) ;
  fread(&(temphead.nrec),4,1,dfile) ;
  fread(&(temphead.nchan),4,1,dfile) ;
  fread(&(temphead.swp),4,1,dfile) ;
  fread(&(temphead.dtime),4,1,dfile) ;
  for( i = 1; i < MAX_COL; ++i )
    {
      fread(&(temphead.ch[i].name[0]),1,13,dfile) ;
      fread(&(temphead.ch[i].units[0]),1,13,dfile) ;
      fread(&(temphead.ch[i].gain),4,1,dfile) ;
      fread(&(temphead.ch[i].comment[0]),1,50,dfile) ;
      fread(&(temphead.ch[i].nelem),4,1,dfile) ;
    }
  
  sprintf(msg,"%s\n",temphead.title) ;
  print_msg(msg);
  
  sprintf(msg,"nrec = %5d, nchan = %5d, swp = %5d, dtime = %14.7e\n",temphead.nrec,temphead.nchan,temphead.swp,temphead.dtime);
  print_msg(msg);
  
  for( i = 1; i < MAX_COL; ++i ) 
    {
      sprintf(msg,"%2d %13s %13s %14.7e %5d\n",i,&(temphead.ch[i].name[0]),&(temphead.ch[i].units[0]),temphead.ch[i].gain,temphead.ch[i].nelem) ;
      print_msg(msg);
      sprintf(msg,"%50s\n",&(temphead.ch[i].comment[0])) ;
      print_msg(msg);
    }
}

/*********************************** rite *****************************/
/* done */
void rite(dfile)
     FILE *dfile;
{
  int i ;
  
  fwrite(&(head.title[0]),1,20,dfile) ;
  fwrite(&(head.nrec),4,1,dfile) ;
  fwrite(&(head.nchan),4,1,dfile) ;
  fwrite(&(head.swp),4,1,dfile) ;
  fwrite(&(head.dtime),4,1,dfile) ;
  for( i = 1; i < MAX_COL; ++i )
    {
      fwrite(&(head.ch[i].name[0]),1,13,dfile) ;
      fwrite(&(head.ch[i].units[0]),1,13,dfile) ;
      fwrite(&(head.ch[i].gain),4,1,dfile) ;
      fwrite(&(head.ch[i].comment[0]),1,50,dfile) ;
      fwrite(&(head.ch[i].nelem),4,1,dfile) ;
    }
  
  for( i = 1; i < MAX_COL; ++i )
    {
      if( (strncmp(&(head.ch[i].name[0]),"no_val",6) != 0))
	{
	  fwrite(darray[i],4,head.ch[i].nelem,dfile) ;
	}
    }
}


/********************************* reed ***************************/
int reed(dfile,append)
     FILE	*dfile ;
     int	append;
{
  int i ;
  int rec , chan, file1_nrec ;
  int file1_nelem[MAX_COL];
  char title[20] ;
    
  
  i = 2;
  file1_nrec = 0;
  fread(&(title[0]),1,20,dfile) ;
  fread(&(rec),4,1,dfile) ;
  fread(&(chan),4,1,dfile) ;
  
  if(append == TRUE)
    {
      file1_nrec = head.nrec;
      chan = (chan > head.nchan) ? chan : head.nchan;
    }
  
  if( (file1_nrec+rec+1>max_row || chan+3>max_col ) && (allocate((file1_nrec+rec+1), (chan+3)) != 1))
    {
      sprintf(msg,"Problem with allocation.\nALLOCATION\tnrec = %d\tnchan = %d\n",rec,chan);
      print_msg(msg);
      return 0 ;
    } 
  
  head.nrec = file1_nrec+rec ;
  head.nchan = chan ;
  
  if(append == TRUE)	
    {
      fseek(dfile,8, 1);	
      for( i = 1; i < MAX_COL; ++i )
        {
	  fseek(dfile, 80, 1);	
	  file1_nelem[i] = head.ch[i].nelem;	/* save old */
	  fread(&(head.ch[i].nelem),4,1,dfile) ;
        }
    }
  else			 /* use the first file for title, gain etc */
    {
      strcpy(&(head.title[0]),&(title[0])) ;
      
      fread(&(head.swp),4,1,dfile) ;
      fread(&(head.dtime),4,1,dfile) ;
      for( i = 1; i < MAX_COL; ++i )
   	{
	  fread(&(head.ch[i].name[0]),1,13,dfile) ;
	  fread(&(head.ch[i].units[0]),1,13,dfile) ;
	  fread(&(head.ch[i].gain),4,1,dfile) ;
	  fread(&(head.ch[i].comment[0]),1,50,dfile) ;
	  fread(&(head.ch[i].nelem),4,1,dfile) ;
	  file1_nelem[i] = 0;
   	}
    }
  
  for( i = 1; i < MAX_COL; ++i )
    {
      if( (strncmp(&(head.ch[i].name[0]),"no_val",6) != 0))
	{
	  if(head.ch[i].nelem <= 0)
	    {
	      sprintf(msg,"WARNING: problem with header.\n Active column has no elements.\n");
	      print_msg(msg);
	      sprintf(msg,"Assumed nelem = nrec\nCONTINUE READ\n");
	      print_msg(msg);
	      head.ch[i].nelem = rec ;
	    }
	  fread(&darray[i][file1_nelem[i]],4,head.ch[i].nelem,dfile) ;
	  head.ch[i].nelem += file1_nelem[i];
	}
    }
  return 1 ;
}

/* ----------------------------- STDASC -------------------------- */
/* done */
int stdasc(pfile, cr, fi)
     FILE *pfile ;
     char cr[20], fi[20];
{
  int i , j , temp_int;
  char headline[20];
  float samp;
  float tmax, tmin;
  /* char *strcat() , *strcmp() ;*/
  int chan , rec ;
    
  
  while(strncmp(headline,"ivar",4) != 0)
    {
      fscanf(pfile,"%s\n",headline) ;
    }
  fscanf(pfile,"%6d\n",&(chan)) ;
  while(strncmp(headline,"ndata",5) != 0)
    {
      fscanf(pfile,"%s,\n",headline) ;
    }
  fscanf(pfile,"%6d\n",&(rec)) ;
  if( rec>=max_row || chan>=max_col )
    {
      sprintf(msg,"INSUFFICIENT ALLOCATION\nnrec = %d\nnchan = %d\n",rec,chan);
      print_msg(msg);
      return 0 ;
    }
  else
    {
      head.nrec = rec ;
      head.nchan = chan ;
    }
  while(strncmp(headline,"samp",4) != 0)
    {
      fscanf(pfile,"%s,\n",headline) ;
    }
  fscanf(pfile,"%f\n",&samp) ;
  while(strncmp(headline,"tmin",4) != 0)
    {
      fscanf(pfile,"%s,\n",headline) ;
    }
  fscanf(pfile,"%f\n",&tmin) ;
  while(strncmp(headline,"tmax",4) != 0)
    {
      fscanf(pfile,"%s,\n",headline) ;
    }
  fscanf(pfile,"%f\n",&tmax) ;
  while(strncmp(headline,"title",5) != 0)
    {
      fscanf(pfile,"%s,\n",headline) ;
    }
  fscanf(pfile,"%s,\n",&(head.title[0])) ;
  while(strncmp(headline,"24,",3) != 0)
    {
      fscanf(pfile,"%s,\n",headline) ;
    }
  fscanf(pfile,"%f\n",&(head.ch[2].gain)) ;
  while(strncmp(headline,"data",4) != 0)
    {
      fscanf(pfile,"%s\n",headline) ;
    }
  strcpy(headline, cr);
  if(strncmp(headline,"comp",4) == 0)
    {
      strcpy(&(head.ch[1].name[0]),"ind_var") ;
      strcpy(&(head.ch[1].units[0]),"no_info") ;
      strcpy(&(head.ch[2].name[0]),"real") ;
      strcpy(&(head.ch[2].units[0]),"no_info") ;
      strcpy(&(head.ch[3].name[0]),"complex") ;
      strcpy(&(head.ch[3].units[0]),"no_info") ;
      head.nchan = 3 ;
      head.nrec /= 2 ;
    }
  else
    {
      fprintf(stderr,"reading data as real numbers\n");
      strcpy(&(head.ch[1].name[0]),"independ") ;
      strcpy(&(head.ch[1].units[0]),"no_info") ;
      strcpy(&(head.ch[2].name[0]),"depend") ;
      strcpy(&(head.ch[2].units[0]),"no_info") ;
      head.nchan = 2 ;
/*      printf("float or scaled interger format??\n");
      fscanf(stdin,"%s",headline); */
      strcpy(headline, fi);
    }
  
  /*	READ DATA    */
  for ( i = 0; i < head.nrec; ++i)
    {
      for ( j = 1; j < 3; ++j)
	{
	  if ( j == 1 )
	    {
	      darray[j][i] = samp ;
	    }
	  else
	    {
	      if ( head.nchan == 2 )
		{
		  if(strncmp(headline, "float", 5) == 0)
		    {
		      fscanf(pfile," %f,\n",&(darray[j][i]));
		    }
		  else
		    {
		      fscanf(pfile," %f\n",&(darray[j][i]));
		      darray[j][i] *= head.ch[2].gain;
		    }
		}
	      if ( head.nchan == 3 )
		{
		  fscanf(pfile," %f,\n",&(darray[j][i]));
		  darray[j][i] *= head.ch[2].gain;
		  fscanf(pfile," %f,\n",&(darray[j+1][i]));
		  darray[j+1][i] *= head.ch[2].gain;
		}
	    }
	}
    }
  return 1 ;
}


/*------------------------------  TASC ----------------------------------*/ 
/* done */
int tasc(pfile, float_flag)
     FILE *pfile ;
     int  float_flag;
{
    
  int i , j , temp_int , rec , chan;
  char junk ;
  float xtra[5] ;
  char  end_head[5] , title[20];
  /* char *strcat() , *strcmp() ;*/
  float temp_float;

  fscanf(pfile,"%s\n",&(title[0])) ;
  fscanf(pfile,"%5d%5d\n",&(chan),&(rec)) ;
  if (chan >= max_col || rec >= max_row)
    {
      sprintf(msg, "INSUFFICIENT ALLOCATION\nnrec = %d\nnchan = %d\n",rec,chan+1);
      print_msg(msg);
      return 0 ;
    }
  else
    {
      head.nrec = rec ;
      head.nchan = chan ;
      strcpy(&(head.title[0]),&title[0]) ;
    }
  fscanf(pfile,"%5d %e\n",&(head.swp),&(head.dtime)) ;
  for ( i = 2; i <= head.nchan+1; ++i)
    {
      fscanf(pfile,"%e",&(head.ch[i].gain)) ;
      fscanf(pfile,"%s %s\n",&(head.ch[i].name[0]),&(head.ch[i].units[0])) ;
      strcpy(&(head.ch[i].comment[0]),"none") ;
      head.ch[i].nelem = head.nrec ;
    } 
  i = 1 ;
  strcpy(&(head.ch[i].name[0]),"deltatime") ;
  strcpy(&(head.ch[i].units[0]),"seconds") ;
  strcpy(&(head.ch[i].comment[0]),"none") ;
  head.ch[i].gain = 1.0 ;
  head.ch[i].nelem = head.nrec ;
  head.nchan += 1 ;
  
  for ( i = 0; i < 5; ++i)
    {
      fscanf(pfile,"%e\n",&(head.extra[i])) ;
    }
  fscanf(pfile,"%s\n",&(end_head[0])) ;
  
  if (strncmp(&(end_head[0]) , "*EOH", 4 ) == 0)
    {
      sprintf(msg, "Header read successfully.\n");
      print_msg(msg);
    }
  else
    {
      sprintf(msg, "Problem reading header info - sorry!\n");
      print_msg(msg);
      return 0 ;
    }
  
  /*	READ DATA    */
  
  if(float_flag == 0)
    sprintf(msg, "Reading data as int.\n");
  else	
    sprintf(msg, "Reading data as float.\n");
  print_msg(msg);
    
  for ( i = 0; i < head.nrec; ++i)
    {
      for ( j = 1; j <= head.nchan; ++j)
	{
	  /*0 = int	1 = float	*/
	  if(float_flag == 0) 
	    {
	      fscanf(pfile,"%10d",&temp_int) ; 
	      
	      if ( j == 1 )
		{
		  darray[j][i] = temp_int * head.dtime;
		}
	      else
		{
		  darray[j][i] = temp_int * head.ch[j].gain ;
		}
	    }

	  else	
	    {
	      fscanf(pfile,"%e",&temp_float) ;
	      
	      if ( j == 1 )
		{
		  darray[j][i] = temp_float * head.dtime;
		}
	      else
		{
		  darray[j][i] = temp_float * head.ch[j].gain ;
		}
	    }
	  
	}
      fscanf(pfile,"%c",&junk);
      if ( junk == '\n' || junk == ' ' || junk == '\t' ) ;
      else
	{
	  sprintf(msg, "Error reading data on line %d!\n",i) ;
	  print_msg(msg);
	  i = head.nrec ;
	}
      
    }
  fclose(pfile);
  return 1 ;
}
/********************************************/

