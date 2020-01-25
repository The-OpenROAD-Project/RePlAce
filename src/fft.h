#ifndef __REPLACE_FFT__
#define __REPLACE_FFT__


namespace replace {

// trying to implement...
class FFT {
  public:
    FFT();
    ~FFT();

  private:
    std::vector<std::vector<float>> binDensity;
    std::vector<std::vector<float>> ex;
    std::vector<std::vector<float>> ey;
    int binCntX;
    int binCntY;
};



// following is the previous code that encrypted =(
//
extern float **den_2d_st2;
extern float **phi_2d_st2;
extern float **ex_2d_st2;
extern float **ey_2d_st2;

extern float *w_2d;
extern float *wx_2d_st;
extern float *wx2_2d_st;
extern float *wy_2d_st;
extern float *wy2_2d_st;

extern float DFT_SCALE_2D;
extern struct POS dft_bin_2d;

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
inline void copy_den_to_fft_2D(float density, struct POS p) {
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

// From FFT Library
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

}

#endif
