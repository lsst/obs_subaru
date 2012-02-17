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
#include "lsst/meas/deblender/Baseline.h"
#include "lsst/pex/logging.h"
#include "lsst/afw/cameraGeom.h"
%}

%inline %{
namespace lsst { namespace afw {
        namespace detection { }
        namespace image { }
} }
using namespace lsst;
using namespace lsst::afw::image;
using namespace lsst::afw::detection;
%}

%include "lsst/p_lsstSwig.i"
%include "lsst/base.h"                  // PTR(); should be in p_lsstSwig.i
 //%include "lsst/daf/base/persistenceMacros.i"

%lsst_exceptions();

%import "lsst/afw/image/imageLib.i"
%import "lsst/afw/detection/detectionLib.i"
%include "lsst/afw/image/lsstImageTypes.i"     // Image/Mask types and typedefs

%include "lsst/meas/deblender/Baseline.h"
%template(BaselineUtilsF) lsst::meas::deblender::BaselineUtils<float>;

/******************************************************************************/
// Local Variables: ***
// eval: (setq indent-tabs-mode nil) ***
// End: ***
