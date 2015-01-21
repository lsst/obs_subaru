#ifndef DISTEST_UTILS2_H
#define DISTEST_UTILS2_H
#include<iostream>
#include<cstdlib>
#include<fstream>
#include<cmath>

const int XYOrder = 9;

/* distortion coefficients by Okura-kun, to be accessible in any functions */
//extern double ****Coef;

//int initDistEst(double ****Coef);
//int deleteDistEst(double ****Coef);
void convUndist2DistPos(float x_undist, float y_undist, 
                        float *x_dist, float *y_dist, float elevation, double ***Coef);
void convUndist2DistPosIterative(float x_undist, float y_undist, 
                                 float *x_dist, float *y_dist, float elevation, double ***Coef);
void convDist2UndistPos(float x_dist, float y_dist, 
                        float *x_undist, float *y_undist, float elevation, double ***Coef);
void   F_SETCOEF(double ***Coef);
void   F_SETCOEF_HSCSIM(double ***Coef);
//double F_CCDtoSKY_X(int, double, double ****, double, double);
//double F_CCDtoSKY_Y(int, double, double ****, double, double);
//double F_SKYtoCCD_X(int, double, double ****, double, double);
//double F_SKYtoCCD_Y(int, double, double ****, double, double);
//double F_CS(int CS,int ELOrder, double EL,double ****Coef, double X, double Y);
void F_CS_CCD2SKY_XY(int ELOrder, double EL, double ***Coef, double X, double Y, double *X_out, double *Y_out);
void F_CS_SKY2CCD_XY(int ELOrder, double EL, double ***Coef, double X, double Y, double *X_out, double *Y_out);
///void   F_LS1(int ,int ,double **,double *);

#endif
