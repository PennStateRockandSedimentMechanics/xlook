#include <stdio.h>

/* filtersm.c */
/* Note, this is bad form; it should be responsible for opening and closing the FILE * in this code - rdm */
/* Ref the binary bug (that worked on mac but not on PC) because fopen was just r, not rb */
int  header_version(FILE *dfile, int show_message);
void examin(FILE *dfile);
void rite_lookfile(FILE *dfile);
int reed(FILE *dfile, int append);

