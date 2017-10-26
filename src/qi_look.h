/* qi_look.c */
int exec_qi(double converg_tol, double lambda, double wc, struct rs_parameters *rsp);
double calc_chisq(double a[], double x[], double y[], double wv[], int ma, int ndata, struct rs_parameters *rsp);
double **dm(int row, int col);
void free_dm(double **m, int row, int col);
int msvdfit(double x[], double y[], double wv[], int ndata, double a[], int ma, double **u, double **v, double w[], double *chisq, struct rs_parameters *rsp, double *lambda);
void set_rs_parameters(struct rs_parameters *rsp, double a[], double da[]);
int get_mu_at_x_t(double x[], double mod_mu[], int ndata, struct rs_parameters *rsp);
int do_rk(double *mu, double *psi1, double *psi2, double *H, double *v, struct rs_parameters *rsp, double *constant);
int msvdcmp(double **a, int m, int n, double *w, double **v);
void msvbksb(double **u, double w[], double **v, int m, int n, double b[], double x[]);
void zero_sv(double w[], int ma, double tol, int op_flag);
