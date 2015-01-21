// -*- lsst-c++ -*-

%define hscLib_DOCSTRING
"
Interface class for HSC distortion
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.obs.hsc", docstring=hscLib_DOCSTRING) hscLib

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

%include "typemaps.i"
%include "lsst/p_lsstSwig.i"
%include "std_string.i"
%include "std_vector.i"

%template(Vdouble)  std::vector<double>;
%template(VVdouble) std::vector< std::vector<double> >;

%include "lsst/obs/hsc/distest.h"
