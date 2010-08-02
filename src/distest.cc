#include<cstdio>
#include<cstdlib>
#include<cmath>
#include "hsc/meas/match/distest.h"
#include "hsc/meas/match/distest_utils.h"


/*------------------------------------------------------------------------------*/
void hsc::meas::match::getDistortedPosition(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation)
/*------------------------------------------------------------------------------*/
{

    /* global variables for distortion coefficients by Okura-kun */
    double (*Apdx)[34][2] = new double[10][34][2];
    double (*Adpx)[34][2] = new double[10][34][2];


    if( setPdArray(Apdx) != 0) {  
        printf("Distortion coefficients can not be initialized.\n");
        exit(-1);
    }
    if( setDpArray(Adpx) != 0) {  
        printf("Distortion coefficients can not be initialized.\n");
        exit(-1);
    }
//    else {
//        printf("Distortion coefficients are initialized.\n");
//    }


    DISTEstpd(x_undist, y_undist, x_dist, y_dist, elevation, Apdx, Adpx);

    delete [] Apdx;
    delete [] Adpx;


//    return 0;

}

/*------------------------------------------------------------------------------*/
void hsc::meas::match::getUndistortedPosition(float x_dist, float y_dist, float* x_undist, float* y_undist, float elevation)
/*------------------------------------------------------------------------------*/
{

    /* global variables for distortion coefficients by Okura-kun */

    double (*Apdx)[34][2] = new double[10][34][2];
    double (*Adpx)[34][2] = new double[10][34][2];

    if( setPdArray(Apdx) != 0) {  
        printf("Distortion coefficients can not be initialized.\n");
        exit(-1);
    }
    if( setDpArray(Adpx) != 0) {  
        printf("Distortion coefficients can not be initialized.\n");
        exit(-1);
    }
//    else {
//        printf("Distortion coefficients are initialized.\n");
//    }


    DISTEstdp(x_dist, y_dist, x_undist, y_undist, elevation, Apdx, Adpx);


    delete [] Apdx;
    delete [] Adpx;

//    return 0;

}
