// -*- lsst-c++ -*-
%define meas_deblenderLib_DOCSTRING
"
Python interface to lsst::meas::deblender classes
"
%enddef

%feature("autodoc", "1");

%module(package="lsst.meas.deblender", docstring=meas_deblenderLib_DOCSTRING) deblenderLib

%{
#include <exception>
#include <list>
#include <boost/shared_ptr.hpp>
#include "lsst/meas/deblender/deblender.h"
%}

%include "lsst/p_lsstSwig.i"
%lsst_exceptions();

# raiseLsstException() is in
# /usr/local/lsst-sandbox/DarwinX86//utils/3.4.3/python/lsst/p_lsstSwig.i

%import "lsst/afw/image/imageLib.i"
%include "lsst/afw/image/lsstImageTypes.i"     // Image/Mask types and typedefs

%include "lsst/meas/deblender/deblender.h"

##SWIG_SHARED_PTR(DefectPtrT, lsst::meas::algorithms::Defect);

%define %instantiate_templates(NAME, TYPE)
		%template(Deblender ## NAME) lsst::meas::deblender::Deblender<lsst::afw::image::Image<TYPE> >;
		%template(SDSSDeblender ## NAME) lsst::meas::deblender::SDSSDeblender<lsst::afw::image::Image<TYPE> >;
%enddef

%instantiate_templates(F, float)

/******************************************************************************/
// Local Variables: ***
// eval: (setq indent-tabs-mode nil) ***
// End: ***
