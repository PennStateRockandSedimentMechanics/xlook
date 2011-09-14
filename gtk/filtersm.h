#include <stdio.h>

/* filtersm.c */
int read_32(int *target, int count, FILE *file);
int  header_version(FILE *dfile, int show_message);
void examin(FILE *dfile);
void rite_lookfile(FILE *dfile);
int reed(FILE *dfile, int append);

