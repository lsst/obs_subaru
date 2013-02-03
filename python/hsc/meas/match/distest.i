%module  distest
%include "typemaps.i"
%include "lsst/p_lsstSwig.i"

%lsst_exceptions();

%{
// This part is passed through into modified C++ code
// usually header files (and necessary declarations)
// to be included in the modified C++ code are listed.
#include<cstdio>
#include<cstdlib>
#include<cmath>
#include "hsc/meas/match/distest.h"
#include "hsc/meas/match/distest_utils2.h"
#include "hsc/meas/match/LeastSquares.h"
%}
%{
// This part should include definitions of newly defined
// functions in the modified C++ code.
// Typically, used to define a larger function combining
// smaller functions in the original C++ code.
%}

namespace hsc {
namespace meas { 
namespace match {
        void getDistortedPosition(float x_undist, float y_undist, float *OUTPUT, float *OUTPUT, float elevation);
        void getDistortedPositionIterative(float x_undist, float y_undist, float *OUTPUT, float *OUTPUT, float elevation);
        void getUndistortedPosition(float x_dist, float y_dist, float *OUTPUT, float *OUTPUT, float elevation);
} 
}
}
