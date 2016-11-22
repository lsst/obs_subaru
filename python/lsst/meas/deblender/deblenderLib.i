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
#include <memory>
#include "lsst/log/Log.h"
#include "lsst/meas/deblender/Baseline.h"
#include "lsst/afw/table.h"
#include "lsst/afw/detection.h"
#include "lsst/afw/cameraGeom.h"
#include "lsst/afw/math.h"

#include "lsst/afw/image.h"
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
%initializeNumPy(meas_deblender)

%include "lsst/base.h"                  // PTR(); should be in p_lsstSwig.i

%lsst_exceptions();

%import "lsst/afw/table/io/ioLib.i"
%import "lsst/afw/image/imageLib.i"
%import "lsst/afw/detection/detectionLib.i"

%apply bool *OUTPUT { bool *patchedEdges };

%include "lsst/meas/deblender/Baseline.h"

%template(BaselineUtilsF) lsst::meas::deblender::BaselineUtils<float>;

%template(pairImageFPtrAndFootprintPtr) std::pair<lsst::meas::deblender::BaselineUtils<float>::ImagePtrT, lsst::meas::deblender::BaselineUtils<float>::FootprintPtrT>;

/******************************************************************************/
// Local Variables: ***
// eval: (setq indent-tabs-mode nil) ***
// End: ***
