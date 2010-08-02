#ifndef DISTEST_UTILS_H
#define DISTEST_UTILS_H

#define ANUM 34
#define fieldMAX (0.8*60*60/0.202)
#define INFMIN pow(10,-6)
#define MNUM2 7

/* distortion coefficients by Okura-kun, to persist over the program */
//double Apdx[10][34][2], Adpx[10][34][2]; 

extern "C" {
int DISTEstdp(float x_real, float y_real, float *x_in, float *y_in, float el, double (*Apdx)[34][2], double (*Adpx)[34][2]);
int DISTEstpd(float x_in, float y_in, float *x_real, float *y_real, float el, double (*Adpx)[34][2], double (*Apdx)[34][2]);
void fBx(double inx[],double Bx[]);
void InvM(int Order,double M[][MNUM2],double Mout[][MNUM2]);
int fpdEst(int A, double EL,double x[],double ex[], double (*Apdx)[34][2], double (*Adpx)[34][2]);
int setDpArray(double (*Adpx)[ANUM][2]);
int setPdArray(double (*Apdx)[ANUM][2]);
//int init_distest(double Axpd[][ANUM][2],double Axdp[][ANUM][2]);
//int init_distest();
}

#endif
