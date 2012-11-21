// -*- lsst-c++ -*-

%define suprimecamLib_DOCSTRING
"
Interface class for suprimecam crosstalk correction
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.obs.subaru", docstring=suprimecamLib_DOCSTRING) suprimecamLib

%{
//#include "lsst/base.h"
//#include "lsst/afw/geom/Point.h"
//#include "lsst/afw/geom/Extent.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/obs/suprimecam/CrossTalk.h"
%}

%include "lsst/p_lsstSwig.i"
//%import "lsst/afw/cameraGeom/cameraGeomLib.i"
//%import "lsst/afw/geom/geomLib.i"
%import "lsst/afw/image/imageLib.i"

//%shared_ptr(lsst::obs::hscSim::HscDistortion)

%include "lsst/obs/suprimecam/CrossTalk.h"
//%template(vectorCalib) std::vector<boost::shared_ptr<const lsst::afw::image::Calib> >;

