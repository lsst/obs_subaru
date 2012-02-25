//--------------------------------------------------
//Calculating Distortion for HSC Simulation
//
//Last modification : 2012/02/25
//--------------------------------------------------
#include<iostream>
#include<cmath>

class CL_DISTforSIM{
private:
public:

    int EL;
    int ORDER;
    int DNUM;
    int ERROR;
    double **REAL_X_CCD;//[DNUM[X,Y,COUNT]
    double **REAL_X_SKY;
    double **PREDICT_X_CCD;
    double **PREDICT_X_SKY;
    double Coef_CCDtoSKY[2][55];
    double Coef_SKYtoCCD[2][55];

    void F_DSIM_CCDtoSKY();
    void F_DSIM_SKYtoCCD();
    void F_DSIM_NEWCCDtoSKY();
    void F_DSIM_NEWSKYtoCCD();
    void F_DSIM_DELCCDtoSKY();
    void F_DSIM_DELSKYtoCCD();

    void F_DSIM_GETCOEFCCDtoSKY();
    void F_DSIM_GETCOEFSKYtoCCD();
};
