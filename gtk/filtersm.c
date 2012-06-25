/*    functions RECALL, EXAMIN, RITE, REED, TASC, and OLDREAD for program LOOK     */
#include <config.h>
#include <assert.h>

#include "global.h"
#include "filtersm.h"
#include "look_funcs.h"
#include "order32.h"

#define SWAP4(q) (((((uint32_t) (q)))>>24) | ((((uint32_t) (q))>>8)&0xff00) | ((((uint32_t) (q))<<8)&0xff0000) | ((((uint32_t) (q))<<24)&0xff000000))

extern void print_msg();
extern char msg[MSG_LENGTH];

/** 
 * Read two-byte words (s) from the data file.
 * 
 * Reads \a count two-byte words  from \a file, and stores them at the given \a target. 
 * Presumes that the file was written in big-endian (network) byte order, and 
 * swaps the bytes appropriately for the current architecture. 
 *
 * This function currently presumes that it is dealing with 32-bit words .
 *
 * @param target The buffer (size = sizeof(int) * count) where this function 
 *               should store the results.
 * @param count The number of two-byte words to read from the file.
 * @param file The open file descriptor to read from.
 * 
 * @return The number of two-byte words that were successfully read from the file.
 */
int read_32(int *target, int count, FILE *file) 
{
	int num_read, i, swap= 0;

	switch(O32_HOST_ORDER)
	{
		case O32_LITTLE_ENDIAN:
			swap= 1;
			break;
		case O32_BIG_ENDIAN:
			break;
		case O32_PDP_ENDIAN:
            exit(-1); /* assert would be preferable */
			break;
	}
	assert(sizeof(uint32_t)==4);

    num_read = fread(target, 4, count, file);
	if(swap)
	{
	    for (i=0; i<num_read; i++)
	    {
        	target[i] = SWAP4(target[i]);
		}
    }
    return num_read;
}

/**   29.4.07 cjm --based on input from Scott Woods
* Write 32-bit values to the data file.
*
* writes 32-bit values to file
* Presumes that the file should be written in big-endian (network) byte order, and
* swaps the bytes appropriately for the current architecture.
*
* This function currently presumes that it is dealing with 32-bit values.
*
* @param target The buffer (size = sizeof(int) * count) from which this function writes
* @param count The number of values from the buffer to write to the file.
* @param file The open file descriptor to write to.
*
* @return The number of values that were successfully written to the file.
*/
int write_32(int *target, int count, FILE *file)
{
	int num_written, written, i, swap= 0;
   int swabbed_int;

	switch(O32_HOST_ORDER)
	{
		case O32_LITTLE_ENDIAN:
			swap= 1;
			break;
		case O32_BIG_ENDIAN:
			break;
		case O32_PDP_ENDIAN:
            exit(-1);
			break;
	}
	assert(sizeof(uint32_t)==4);

   num_written = 0;
   for (i=0; i<count; i++)
   {
     /* convert each value to network byte order to ensure consistency across architectures. */
        /* integers are always written to file in big-endian byte order */
		if(swap)
		{
       		swabbed_int = SWAP4(target[i]);
		} else {
       		swabbed_int = target[i];
		}
       written = fwrite(&swabbed_int, 4, 1, file);

        if (written == 1)
           num_written++;
        else
           break;
   }
   return num_written;
}

/* 13.2.07 cjm
*	Function to determine if the header is a 16 channel (old) version 
*	or a 32 channel (new) version
*	
* 	11.2.2010: cjm: change so that we can read files with 8 byte real numbers 
*
*	return of 64 will indicate and 8-bit file with a 32 channel header
*
*	return is 16, 32, 64 or 0 (0 indicates an error)
*/
/* rdm - added showMessage since I use this for file filtering */
int  header_version(
	FILE *dfile, 
	int showMessage)
{
	int total_bytes, head_bytes, data_bytes, nchan, nrec;
	int result= 0;
	
	fseek(dfile,20L,SEEK_SET);		/* skip over title*/
	read_32(&(nrec),1,dfile) ;
	read_32(&(nchan),1,dfile) ;
	fseek(dfile,0L,SEEK_END);
	total_bytes = ftell(dfile);         /*determine the byte length of the file */
	data_bytes = nchan*nrec*4;   /*assume 4 byte float for each data point; data part of the file is nchan*nrec*4 */ 
	/*if it's a file of 8-byte doubles, we'll figure that out below*/ 
	head_bytes = total_bytes - data_bytes;

	fseek(dfile,0L,SEEK_SET);		/* put file pointer back to start*/

	/*fprintf(stderr,"total_bytes =%d, data bytes=%d, head_bytes=%d, nchan=%d, nrec=%d\n", total_bytes, data_bytes, head_bytes, nchan, nrec);*/

	if(head_bytes == 1380)
	{
		sprintf(msg,"reading 16 channel file\n") ;
		result= 16;
	}
	else if(head_bytes == 2724)
	{
		sprintf(msg,"reading 32 channel file of floats\n") ;
		result= 32;			/*this is an new file, with 32 channels in the header*/
	}
	else if(head_bytes == 2724 + data_bytes)
	{
		sprintf(msg,"reading 32 channel file of doubles\n") ;
		result= 64;			/*this is a file of double-precision values with a 32 channel header*/
	}
	else
	{
		sprintf(msg,"Problem reading file. nchan=%d, nrec=%d, total_bytes=%d, data_bytes=%d, head_bytes=%d\n",nchan, nrec, total_bytes, data_bytes, head_bytes) ;
		result= 0; 	/* hmmm..... there must be some sort of problem*/
	}
	
	if(showMessage) print_msg(msg);

	return result;
}

/***************************** examin *******************************/
/* done */
void examin(dfile)
     FILE *dfile ;
{
  int i ;
  struct header temphead ;
    
  
  fread(&(temphead.title[0]),1,20,dfile) ;
  read_32(&(temphead.nrec),1,dfile) ;
  read_32(&(temphead.nchan),1,dfile) ;
  read_32(&(temphead.swp),1,dfile) ;
  read_32((int *)(&(temphead.dtime)),1,dfile) ;
  /*fread(&(temphead.dtime),4,1,dfile) ;*/
  for( i = 1; i < MAX_COL; ++i )
    {
      fread(&(temphead.ch[i].name[0]),1,13,dfile) ;
      fread(&(temphead.ch[i].units[0]),1,13,dfile) ;
      fread(&(temphead.ch[i].gain),4,1,dfile) ;
      fread(&(temphead.ch[i].comment[0]),1,50,dfile) ;
      read_32(&(temphead.ch[i].nelem),1,dfile) ;
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

#if 0
/*********************************** rite_lookfile *****************************/
void rite_lookfile(FILE *dfile)
{
  int i ;
  
  fwrite(&(head.title[0]),1,20,dfile) ;
  fwrite(&(head.nrec),4,1,dfile) ;
  fwrite(&(head.nchan),4,1,dfile) ;
  fwrite(&(head.swp),4,1,dfile) ;
  fwrite(&(head.dtime),4,1,dfile) ;
  for( i = 1; i < MAX_COL; ++i ) /*for MAX_COL of 33, this will write a 32 channel header, each channel has 84 bytes of data*/
    {
      fwrite(&(head.ch[i].name[0]),1,13,dfile) ;
      fwrite(&(head.ch[i].units[0]),1,13,dfile) ;
      fwrite(&(head.ch[i].gain),4,1,dfile) ;
      fwrite(&(head.ch[i].comment[0]),1,50,dfile) ;
      fwrite(&(head.ch[i].nelem),4,1,dfile) ;
    }
  
  for( i = 1; i < MAX_COL; ++i )
    {
      if( (strncmp(head.ch[i].name,"no_val",6) != 0))
	  fwrite(darray[i],sizeof(double),head.ch[i].nelem,dfile) ;
    }
}

#endif

/* cjm: 12.2.2010: use version above, w/o byte swapping, which is not important (?) for double precision ... ? */
/* byte swapping preserved in version below*/
/* BUT: note that the header in xlook is called 'head' rather than 'lookhead' (in lv2look), so be careful w/ copy/paste!*/

/*********************************** rite_lookfile *****************************/
void rite_lookfile(dfile)
FILE *dfile;
{
  int i;

  fwrite(&(head.title[0]),1,20,dfile) ;
  write_32(&(head.nrec),1,dfile) ;
  write_32(&(head.nchan),1,dfile) ;
  write_32(&(head.swp),1,dfile) ;
  write_32((int *)(&(head.dtime)),1,dfile) ;

  for( i = 1; i < MAX_COL; ++i ) /*for MAX_COL of 33, this will write a 32 channel header, each channel has 84 bytes of data*/
    {
      fwrite(&(head.ch[i].name[0]),1,13,dfile) ;
      fwrite(&(head.ch[i].units[0]),1,13,dfile) ;
      write_32((int *)(&(head.ch[i].gain)),1,dfile) ;
      fwrite(&(head.ch[i].comment[0]),1,50,dfile) ;
      write_32(&(head.ch[i].nelem),1,dfile) ;
    }

/* fprintf(stderr, "before big rite \n");*/

  for( i = 1; i < MAX_COL; ++i )
    {
     if( (strncmp(head.ch[i].name,"no_val",6) != 0))
        fwrite(darray[i],sizeof(double),head.ch[i].nelem,dfile) ;

    }

}
/********************************* reed ***************************/
int reed(
	FILE *dfile,
	int append)
{
	int i,j ;
	int rec , chan, file1_nrec ;
	int file1_nelem[MAX_COL];
	int head_version, head_channels;
	char title[20] ;
	float *temp1, *temp2;

	/* 11.2.2010: cjm: change so that we can read files with 8 byte real numbers and/or with 16 chan headers and files with 32 chan. headers */
	/* 13.2.07: cjm: change so that we can read files with 16 chan headers and files with 32 chan. headers */
	/*MAX_COL is now 32, we can use this to set up arrays etc.*/
	/* we need to find out whether the file has a 16 chan header or a 32 chan header*/
	/* The 16 chan header is x bytes long*/
	head_version = head_channels = header_version(dfile, TRUE);	/*head_version will be 16, 32, 64 or 0*/
	if(head_version == 0)
	{
		sprintf(msg,"Problem reading data file. Are you sure this is a look file? Header is neither 16 channels long nor 32 channels long. See filtersm.c \n");
		print_msg(msg);
		return 0 ;
	}
	else if(head_version == 64)
		head_channels = 32;


	i = 2;
	file1_nrec = 0;
	fread(&(title[0]),1,20,dfile) ;

char dump_filename[128];
sprintf(dump_filename, "%s.txt", title);
FILE *fp= fopen(dump_filename, "w+");
fprintf(fp, "Sizes: int %ld, float: %ld, double: %ld\n", sizeof(int), sizeof(float), sizeof(double));
fprintf(fp, "Head channels: %d\n", head_channels);
fprintf(fp, "Title: %s\n", title);
	read_32(&(rec),1,dfile) ;
fprintf(fp, "Rec: %d\n", rec);
	read_32(&(chan),1,dfile) ;
fprintf(fp, "Chan: %d\n", chan);
  
	if(append == TRUE)
	{
fprintf(fp, "APPENDING!\n");
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
		fseek(dfile, 36, SEEK_SET);		/* changed 13.2.07. This should still point to begin of first channel*/
		for( i = 1; i <= head_channels; ++i )	/*head_version will be 16, 32, or 64  depending on the header version and file type*/
		{
			/*head_channels will be 16 or 32*/
			fseek(dfile, 80, SEEK_CUR);	
			file1_nelem[i] = head.ch[i].nelem;	/* save old */
			read_32(&(head.ch[i].nelem),1,dfile) ;
		}
	}
	else /* use the first file for title, gain etc */
    {
		strcpy(&(head.title[0]),&(title[0])) ;
      
		fseek(dfile, 28, SEEK_SET);	/* changed 13.2.07. This should point to begin of swp, after 20 byte title and two 4-byte ints */
		read_32(&(head.swp),1,dfile) ;
fprintf(fp, "Head.swp: %d\n", head.swp);
		read_32((int *)(&(head.dtime)),1,dfile) ;
fprintf(fp, "Head.dtime: %lf\n", head.dtime);
		/*fread(&(head.dtime),4,1,dfile) ;*/
		for( i = 1; i <= head_channels; ++i )
		{
			fread(&(head.ch[i].name[0]),1,13,dfile) ;
fprintf(fp, "Head.ch[%d].name: %.13s\n", i, head.ch[i].name); // TERMINATED?
			fread(&(head.ch[i].units[0]),1,13,dfile) ;
fprintf(fp, "Head.ch[%d].units: %.13s\n", i, head.ch[i].units); // TERMINATED?
			read_32((int *)&(head.ch[i].gain),1,dfile) ;
fprintf(fp, "Head.ch[%d].gain: %lf\n", i, head.ch[i].gain);
			fread(&(head.ch[i].comment[0]),1,50,dfile) ;
fprintf(fp, "Head.ch[%d].comment: %.50s\n", i, head.ch[i].comment); // TERMINATED?
			read_32(&(head.ch[i].nelem),1,dfile) ;
fprintf(fp, "Head.ch[%d].nelem: %d\n", i, head.ch[i].nelem);
			file1_nelem[i] = 0;
		}
	}
  
	if(head_version != 64)
	{
		/*float array for byte swapping*/
		temp1 = (float *)malloc(head.nrec*sizeof(float));	
	}

	/*This is where we read in the data. They are either 4 byte floats or 8 byte doubles*/
	for( i = 1; i <= head_channels; ++i )
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

			if(head_version == 64)
			{
fprintf(fp, "Reading %d doubles!\n", head.ch[i].nelem);
				fread(&darray[i][file1_nelem[i]],sizeof(double),head.ch[i].nelem,dfile) ;
			}
			else
			{
				temp2= temp1; 
				read_32((int *)(temp2),head.ch[i].nelem,dfile);  
				for(j=0;j<head.ch[i].nelem;j++)
				{
					darray[i][file1_nelem[i]+j]=*temp2;
fprintf(fp, "%d - %f!\n", j, *temp2);
					temp2++;
				}
				/*fprintf(stderr,"nelem =%d, first rec= %g, rec2=%g\n", head.ch[i].nelem, *temp1, *(temp1+1));*/
				/*fread(&darray[i][file1_nelem[i]],sizeof(float),head.ch[i].nelem,dfile) ; not sure why this doesn't work...*/
				/*read_32((int *)(&darray[i][file1_nelem[i]]),head.ch[i].nelem,dfile) ;  */
			}
			head.ch[i].nelem += file1_nelem[i];
fprintf(fp, "head.ch[%d].nelem= %d\n", i, head.ch[i].nelem);
		}
	}
	
	if(head_version != 64)	/*float array for byte swapping*/
		free(temp1);

	if(head_version == 16)
	{
		/* set up extra cols */ 
		for( i = 17; i < MAX_COL; ++i )
		{
			null_col(i);
		}
	}
	
fprintf(fp, "DONE!\n");
fclose(fp);
	
	return 1 ;
}