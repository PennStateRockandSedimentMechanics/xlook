/* look_funcs.c */
int token_count(char buf[], int max_length);
int allocate(int row, int col);
void null_col(int col);
int mu_check(double *mu, int vstep);
int check_row(int *first_row, int *last_row, int this_col);
int check_col(int col);
int act_col(void);
void vision(FILE *file, int *firstrow, int *lastrow, int *firstcol, int *lastcol);
void p_vision(FILE *file, int *firstrow, int *lastrow, int *firstcol, int *lastcol);
void msg_vision(int *firstrow, int *lastrow, int *firstcol, int *lastcol);
void msg_p_vision(int *firstrow, int *lastrow, int *firstcol, int *lastcol);
int name(int *unknown, int orig_col, char *arg_name, char *arg_unit);
