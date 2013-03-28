/*  asc2look.c  */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <arpa/inet.h>
#include "global.h"
#define SEEK_END 2
	
struct  header   head;                      /*header for look , defined in global.h*/


int main(ac,av)
int ac;
char *av[];
{
	void exit(), *calloc();
	void	null_col(), rite_lookfile(); 
	int i , j;
	char *strcat(), *strcpy(), outfile[50];
	FILE    *infile, *dfile, *fopen();

	if(ac != 4) 
	{
		fprintf(stderr,"This program converts an ascii table (numbers separated by white space) to a look file. Output is a look format binary file with the letter ell appended to the filename\n"); 
		fprintf(stderr,"The first two lines of the file should have column names and units\n"); 
		fprintf(stderr,"Usage:  asc2look #records #columns filename \n"); 
		exit(1);
	}


/*open a file to write. Append the letter ell */
 	strcpy(outfile,av[3]); 
	if ((infile  = fopen(outfile, "r")) == NULL) 
	{
		fprintf(stderr,"Error. Couldn't open the input file to write. \n"); 
		exit(1);
	}
 	strcat(outfile,"l"); 
	if ((dfile  = fopen(outfile, "w")) == NULL) 
	{
		fprintf(stderr,"Error. Couldn't open the file to write. Do you have write permission in this directory?\n"); 
		exit(1);
	}

	sscanf(av[1],"%d",&(head.nrec));
	sscanf(av[2],"%d",&(head.nchan));
	
	strcpy(head.title,av[3]);

	for( i=0; i < head.nchan+1; ++i )
          darray[i] = (double *)calloc((unsigned)head.nrec,(unsigned)sizeof(double)) ;

				/* write null columns */
        for(j=head.nchan+1; j < MAX_COL; ++j) 
                null_col(j);

	for(j=1; j <= head.nchan; ++j)
		fscanf(infile,"%s",(char *)&(head.ch[j].name));
	for(j=1; j <= head.nchan; ++j)
		fscanf(infile,"%s",(char *)&(head.ch[j].units));

	for(j=1; j <= head.nchan; ++j)
	{
		head.ch[j].nelem = head.nrec ;
		head.ch[j].gain = 1.0 ;
	}

	for(i=0; i<head.nrec; ++i)
	{
		for(j=1; j <= head.nchan; ++j)
		{
			fscanf(infile,"%lf",&(darray[j][i]));
		}
	}

        	/*write data in look format*/
  rite_lookfile(dfile) ;


  fprintf(stderr,"%s\n",head.title) ;
  fprintf(stderr,"ncol=%d  nrec=%d\n",head.nchan,head.nrec) ;

  for ( i = 1; i <= head.nchan; ++i)
    {
      fprintf(stderr,"%s %s nelem=%d\n", head.ch[i].name, head.ch[i].units, head.ch[i].nelem) ;
    }

fprintf(stderr,"done\n");
exit(0);
}

/*------------------------------------------------------------------------*/

void null_col(col)
int col ;
{
   strcpy(&(head.ch[col].name[0]),"no_val") ;
   strcpy(&(head.ch[col].units[0]),"no_val") ;
   strcpy(&(head.ch[col].comment[0]),"none") ;
   head.ch[col].nelem = 0 ;
   head.ch[col].gain = 0 ;
}

/*------------------------------------------------------------------------*/
/********* rite_lookfile *****************************/

/*Note: Pay Attention to what's commented out below, see #if 0 and #endif*/

/* note that this function also exists in lv2look.c and in filtersm.c 
  It is included here for simplicity
 BUT: note that the header here and in xlook is called 'head' rather than 'lookhead' (as in lv2look), so be careful w/ copy/paste!
*/


/*this is the version with network byte swapping. not sure we need that....*/

void rite_lookfile(FILE *dfile)
{
  int i;
  int write_32();

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
      if( (strncmp(&(head.ch[i].name[0]),"no_val",6) != 0))
      {
	fwrite(darray[i],sizeof(double),head.ch[i].nelem,dfile) ;

      }
    }

}

/*********************************** read_16 *****************************/
/**
 * Read two-byte words from the data file.
 * * Reads \a count two-byte words  from \a file, and stores them at the given \a target.
 * Presumes that the file was written in big-endian (network) byte order, and
 * swaps the bytes appropriately for the current architecture.
 *
 * This function currently presumes that it is dealing with 16-bit words .
 *
 * @param target The buffer (size = sizeof(short) * count) where this function
 *               should store the results.
 * @param count The number of two-byte words to read from the file.
 * @param file The open file descriptor to read from.
 *
 * @return The number of two-byte words that were successfully read from the file.
 */
int read_16(short *target, int count, FILE *file)
{
    int num_read, i;

    num_read = fread(target, 2, count, file);
    for (i=0; i<num_read; i++)
    {
        /* integers are always written to file in big-endian byte order */
/*fprintf(stderr,"target[%d] = %X %d\n",i, target[i], target[i]);*/
        target[i] = ntohs(target[i]);
/*fprintf(stderr,"target[%d] = %X %d\n",i, target[i], target[i]);*/
    }
    return num_read;
}

/*********************************** read_32 *****************************/
/* Read four-byte words (s) from the data file.
 *
 * Reads \a count four-byte words  from \a file, and stores them at the given \a target.
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
 * @return The number of four-byte words that were successfully read from the file.
 */
int read_32(int *target, int count, FILE *file)
{
    int num_read, i;

    num_read = fread((uint32_t *)target, 4, count, file);
    for (i=0; i<num_read; i++)
    {
        /* integers are always written to file in big-endian byte order */
/*fprintf(stderr,"target[%d] = %X\n",i, target[i]);*/
        target[i] = ntohl(target[i]); 
/*fprintf(stderr,"target[%d] = %X\n",i, target[i]);*/
    }
    return num_read;
}
/*********************************** write_32 *****************************/
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
   int num_written, written, i;
   int swabbed_int;

   num_written = 0;
   for (i=0; i<count; i++)
   {
     /* convert each value to network byte order to ensure consistency across architectures. */
        /* integers are always written to file in big-endian byte order */
       swabbed_int = htonl(target[i]); 
       written = fwrite(&swabbed_int, 4, 1, file);

        if (written == 1)
           num_written++;
        else
           break;
   }
   return num_written;
}



