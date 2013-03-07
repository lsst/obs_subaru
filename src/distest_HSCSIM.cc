#include<cstdio>
#include<cstdlib>
#include<cmath>
#include "hsc/meas/match/distest.h"
#include "hsc/meas/match/distest_utils2.h"


/*------------------------------------------------------------------------------*/
void hsc::meas::match::getDistortedPosition_HSCSIM(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation)
/*------------------------------------------------------------------------------*/
{
    /* global variables for distortion coefficients by Okura-kun */
    double ***Coef;
    try {
        int ni = 4;
        int nj = NSAMPLE_EL+1;
        int nk = (XYOrder+1)*(XYOrder+1);
        Coef = new double**[ni];
        for(int i=0; i<ni; i++) {
            Coef[i] = new double*[nj];
            for(int j=0; j<nj; j++) {
                Coef[i][j]= new double[nk];
                for(int k=0; k<nk; k++) {
                    Coef[i][j][k] = 0.0;
                }
            }
        }
    }
    catch (std::bad_alloc &) {
        std::cout << "memory allocation error - Coeff"  << std::endl;
        return;
    }
    F_SETCOEF_HSCSIM(Coef);

    // Getting transformed positions
    convUndist2DistPos(x_undist, y_undist, x_dist, y_dist, elevation, Coef);


    // Deleting Coef
    try {
        int ni = 4;
        int nj = NSAMPLE_EL+1;
        for(int i=0; i<ni; i++) {
            for(int j=0; j<nj; j++) {
                delete [] Coef[i][j];
            }
            delete [] Coef[i];
        }
        delete [] Coef;
    }
    catch( ... ) {
        std::cout << "something weired happend in delete Coeff"  << std::endl;
        return;
    }
    
    return;

}


/*------------------------------------------------------------------------------*/
void hsc::meas::match::getDistortedPositionIterative_HSCSIM(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation)
/*------------------------------------------------------------------------------*/
{
    /* global variables for distortion coefficients by Okura-kun */
    double ***Coef;
    try {
        int ni = 4;
        int nj = NSAMPLE_EL+1;
        int nk = (XYOrder+1)*(XYOrder+1);
        Coef = new double**[ni];
        for(int i=0; i<ni; i++) {
            Coef[i] = new double*[nj];
            for(int j=0; j<nj; j++) {
                Coef[i][j]= new double[nk];
                for(int k=0; k<nk; k++) {
                    Coef[i][j][k] = 0.0;
                }
            }
        }
    }
    catch (std::bad_alloc &) {
        std::cout << "memory allocation error - Coeff"  << std::endl;
        return;
    }
    F_SETCOEF_HSCSIM(Coef);

    // Getting transformed positions
    convUndist2DistPosIterative(x_undist, y_undist, x_dist, y_dist, elevation, Coef);

    // Deleting Coef
    try {
        int ni = 4;
        int nj = NSAMPLE_EL+1;
        for(int i=0; i<ni; i++) {
            for(int j=0; j<nj; j++) {
                delete [] Coef[i][j];
            }
            delete [] Coef[i];
        }
        delete [] Coef;
    }
    catch( ... ) {
        std::cout << "something weired happend in delete Coeff"  << std::endl;
        return;
    }
    
    return;

}


/*------------------------------------------------------------------------------*/
void hsc::meas::match::getUndistortedPosition_HSCSIM(float x_dist, float y_dist, float* x_undist, float* y_undist, float elevation)
/*------------------------------------------------------------------------------*/
{

    /* global variables for distortion coefficients by Okura-kun */
    double ***Coef;
    try {
        int ni = 4;
        int nj = NSAMPLE_EL+1;
        int nk = (XYOrder+1)*(XYOrder+1);
        Coef = new double**[ni];
        for(int i=0; i<ni; i++) {
            Coef[i] = new double*[nj];
            for(int j=0; j<nj; j++) {
                Coef[i][j]= new double[nk];
                for(int k=0; k<nk; k++) {
                    Coef[i][j][k] = 0.0;
                }
            }
        }
    }
    catch (std::bad_alloc &) {
        std::cout << "memory allocation error - Coeff"  << std::endl;
        return;
    }
    F_SETCOEF_HSCSIM(Coef);

    // Getting transformed positions
    convDist2UndistPos(x_dist, y_dist, x_undist, y_undist, elevation, Coef);


    // Deleting Coef
    try {
        int ni = 4;
        int nj = NSAMPLE_EL+1;
        for(int i=0; i<ni; i++) {
            for(int j=0; j<nj; j++) {
                delete [] Coef[i][j];
            }
            delete [] Coef[i];
        }
        delete [] Coef;
    }
    catch( ... ) {
        std::cout << "something weired happend in delete Coeff"  << std::endl;
        return;
    }
    
    return;

}
