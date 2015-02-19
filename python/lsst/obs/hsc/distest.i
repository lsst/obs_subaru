%module  distest
%include "typemaps.i"
%include "lsst/p_lsstSwig.i"
%include "std_string.i"
%include "std_vector.i"

%lsst_exceptions();

%{
// This part is passed through into modified C++ code
// usually header files (and necessary declarations)
// to be included in the modified C++ code are listed.
#include<cstdio>
#include<cstdlib>
#include<cmath>
#include "lsst/obs/hsc/distest.h"
#include "lsst/obs/hsc/distest_utils2.h"
#include "lsst/obs/hsc/LeastSquares.h"
#include "lsst/obs/hsc/distest2.h"
%}
%{
// This part should include definitions of newly defined
// functions in the modified C++ code.
// Typically, used to define a larger function combining
// smaller functions in the original C++ code.
%}
%include "lsst/obs/hsc/distest2.h"

namespace lsst {
namespace obs { 
namespace hsc {
        void getDistortedPosition(float x_undist, float y_undist, float *OUTPUT, float *OUTPUT, float elevation);
        void getDistortedPositionIterative(float x_undist, float y_undist, float *OUTPUT, float *OUTPUT, float elevation);
        void getUndistortedPosition(float x_dist, float y_dist, float *OUTPUT, float *OUTPUT, float elevation);
} 
}
}

%template(Vdouble)  std::vector<double>;
%template(VVdouble) std::vector< std::vector<double> >;
