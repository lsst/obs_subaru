
// -*- lsst-c++ -*-

%define subaruLib_DOCSTRING
"
Interface class for subaru crosstalk correction
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.obs.subaru", docstring=subaruLib_DOCSTRING) subaruLib

%{
#include "lsst/base.h"
#include "lsst/daf/base.h"
#include "lsst/afw/geom.h"
#include "lsst/pex/logging.h"
//#include "lsst/afw/geom/Point.h"
//#include "lsst/afw/geom/Extent.h"
#include "lsst/afw/cameraGeom.h"
#include "lsst/afw/image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/obs/subaru/Crosstalk.h"
%}

%include "lsst/p_lsstSwig.i"

%include "std_vector.i"
namespace std {
  %template(VecDouble) vector<double>;
  %template(VecVecDouble) vector< vector<double> >;
  %template(VecInt) vector<int>;
  %template(VecVecInt) vector< vector<int> >;
}

%import "lsst/pex/logging/loggingLib.i"
%import "lsst/daf/base/baseLib.i"
%import "lsst/afw/geom/geomLib.i"
%import "lsst/afw/cameraGeom/cameraGeomLib.i"
%import "lsst/afw/image/imageLib.i"

//%shared_ptr(lsst::obs::hscSim::HscDistortion)

%include "lsst/obs/subaru/Crosstalk.h"
//%template(vectorCalib) std::vector<boost::shared_ptr<const lsst::afw::image::Calib> >;

