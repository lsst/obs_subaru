%define meas_deblender_cmath_DOCSTRING
"
Python interface to miscellaneous math functions
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.meas.deblender", docstring=meas_deblender_cmath_DOCSTRING) cmath

%{
#include <math.h>

// k: DOF
double chisq_pdf(double k, double x) {
    return pow(x, k/2.-1.) * exp(-x/2.) / (exp2(k/2.) * tgamma(k/2.));
}
%}

double tgamma(double x);

double chisq_pdf(double k, double x);

