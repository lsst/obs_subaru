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

//%import "lsst/afw/image/imageLib.i"
%include "lsst/afw/image/imageLib.i"
%include "lsst/afw/image/lsstImageTypes.i"     // Image/Mask types and typedefs

%inline %{
namespace lsst { namespace afw {
        namespace detection { }
} }
%}

SWIG_SHARED_PTR(DeblendedObjectFPtr, lsst::meas::deblender::DeblendedObject<lsst::afw::image::Image<float> >);

%include "lsst/meas/deblender/deblender.h"

%define %instantiate_templates(NAME, TYPE)
 // Already defined in imageLib.i
 // %template(Image ## NAME) lsst::afw::image::Image<TYPE>;
%template(Deblender ## NAME) lsst::meas::deblender::Deblender<lsst::afw::image::Image<TYPE> >;
%template(SDSSDeblender ## NAME) lsst::meas::deblender::SDSSDeblender<lsst::afw::image::Image<TYPE> >;
%template(DeblendedObject ## NAME) lsst::meas::deblender::DeblendedObject<lsst::afw::image::Image<TYPE> >;
//%template(VectorDeblendedObject ## NAME) std::vector< lsst::meas::deblender::DeblendedObject<lsst::afw::image::Image<TYPE> >::Ptr >;
%enddef

%instantiate_templates(F, float)

%template(DeblendedObjectPtrFList) std::vector<lsst::meas::deblender::DeblendedObject<lsst::afw::image::Image<float> >::Ptr>;

%include "lsst/afw/detection/footprints.i"

%template(PeakContainerListT) std::vector<std::vector<lsst::afw::detection::Peak::Ptr> >;

SWIG_SHARED_PTR(MaskedImageFPtrT, lsst::afw::image::MaskedImage<lsst::afw::image::ImageBase<float>::Pixel, lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel>);
SWIG_SHARED_PTR(PsfPtr, boost::shared_ptr<lsst::meas::algorithms::PSF>);

/******************************************************************************/
// Local Variables: ***
// eval: (setq indent-tabs-mode nil) ***
// End: ***
