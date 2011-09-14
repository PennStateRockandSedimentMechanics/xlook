/* simplexl.c */
void do_simp_func(void);
void do_get_initial_values(char arg[256]);
void simp_func_final(void);
void init_values_write(double errormax[], FILE *outf);
void get_starting_simplex(char funcname[], float first_step[], int p_xch[], int *p_ych, int ndata, int first);
void solution_write(int n, double scm_err[], FILE *outf, char f_name[]);
void printSimplex(FILE *outf, char function_name[]);
void order(int high[], int low[]);
void find_centroid(double centre[], int h);
void reflect_worst(double centre[], int h);
void expand_reflection(double centre[], int h);
void contract_worst(double centre[], int h);
void contract_all(char funcname[], double centre[], int l, int p_xch[], int *p_ych, int ndata, int first);
void saveAs_new_vertex(int h);
double error(char name[], int p_xch[], int *p_ych, int nd, int first);
