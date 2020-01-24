#ifndef __PL_FFT__
#define __PL_FFT__

extern int *charge_ip_2d;
extern int *charge_ip_3d;
extern int charge_dft_n_2d;
extern int charge_dft_n_3d;
extern int charge_dft_nbin_2d;
extern int charge_dft_nbin_3d;
extern int charge_dft_nbit_2d;
extern int charge_dft_nbit_3d;
extern int charge_dft_nw_2d;
extern int charge_dft_nw_3d;
extern float **den_2d_st2;
extern float ***den_3d_st3;
extern float **phi_2d_st2;
extern float ***phi_3d_st3;
extern float **e_2d_st2;
extern float ***e_3d_st3;
extern float **ex_2d_st2;
extern float ***ex_3d_st3;
extern float **ey_2d_st2;
extern float ***ey_3d_st3;
extern float **ez_2d_st2;
extern float ***ez_3d_st3;

extern float *w_2d;
extern float *w_3d;
extern float *wx_2d_st;
extern float *wx2_2d_st;
extern float *wy_2d_st;
extern float *wy2_2d_st;
extern float *wz_2d_st;
extern float *wz2_2d_st;
extern float *wx_3d_st;
extern float *wx2_3d_st;
extern float *wy_3d_st;
extern float *wy2_3d_st;
extern float *wz_3d_st;
extern float *wz2_3d_st;
extern float *wx_2d_iL;
extern float *wy_2d_iL;
extern float *wx2_2d_iL;
extern float *wy2_2d_iL;

extern float DFT_SCALE_2D;
extern float DFT_SCALE_3D;
extern struct POS dft_bin_2d;
extern struct POS dft_bin_3d;

/// Electrostatic-Based Functions /////////////////////////////////////////
void charge_fft_init(struct POS nbin, struct FPOS stp, int flg);
void charge_fft_init_2d(struct POS nbin, struct FPOS stp);

void charge_fft_delete(int flg);
void charge_fft_delete_2d(void);

void charge_fft_call(int flg);
void charge_fft_call_2d(void);

inline void copy_e_from_fft_2D(struct FPOS *e, struct POS p) {
  e->x = ex_2d_st2[p.x][p.y];
  e->y = ey_2d_st2[p.x][p.y];
}
inline void copy_phi_from_fft_2D(float *phi, struct POS p) {
  *phi = phi_2d_st2[p.x][p.y];
}
inline void copy_den_to_fft_2D(float den, struct POS p) {
  den_2d_st2[p.x][p.y] = den;
}
/// Thermal-Based Functions ///////////////////////////////////////////////
// void thermal_fft_init    (struct POS nbin, struct FPOS stp, int flg);
// void thermal_fft_init_2d (struct POS nbin, struct FPOS stp);
// void thermal_fft_init_3d (struct POS nbin, struct FPOS stp);
//
// void thermal_fft_delete    (int flg);
// void thermal_fft_delete_2d (void);
// void thermal_fft_delete_3d (void);
//
// void thermal_fft_call (int flg);
// void thermal_fft_call_2d (void);
// void thermal_fft_call_3d (void);
//
// void copy_heatflux_from_fft   (struct FPOS *heatflux, struct POS p, int flg);
// void copy_theta_from_fft      (float   *phi, struct POS p, int flg);
// void copy_powerDensity_to_fft (float   powerDensity, struct POS p, int flg);

/// 1D FFT ////////////////////////////////////////////////////////////////
void cdft(int n, int isgn, float *a, int *ip, float *w);
void ddct(int n, int isgn, float *a, int *ip, float *w);
void ddst(int n, int isgn, float *a, int *ip, float *w);

/// 2D FFT ////////////////////////////////////////////////////////////////
void cdft2d(int, int, int, float **, float *, int *, float *);
void rdft2d(int, int, int, float **, float *, int *, float *);
void ddct2d(int, int, int, float **, float *, int *, float *);
void ddst2d(int, int, int, float **, float *, int *, float *);
void ddsct2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);
void ddcst2d(int n1, int n2, int isgn, float **a, float *t, int *ip, float *w);

/// 3D FFT ////////////////////////////////////////////////////////////////
void cdft3d(int, int, int, int, float ***, float *, int *, float *);
void rdft3d(int, int, int, int, float ***, float *, int *, float *);
void ddct3d(int, int, int, int, float ***, float *, int *, float *);
void ddst3d(int, int, int, int, float ***, float *, int *, float *);
void ddscct3d(int, int, int, int isgn, float ***, float *, int *, float *);
void ddcsct3d(int, int, int, int isgn, float ***, float *, int *, float *);
void ddccst3d(int, int, int, int isgn, float ***, float *, int *, float *);

#endif
