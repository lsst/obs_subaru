// -*- lsst-c++ -*-

%define hscLib_DOCSTRING
"
Interface class for HSC distortion
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.obs.hsc", docstring=hscLib_DOCSTRING) hscLib

%{
#include "lsst/base.h"
#include "lsst/afw/geom/Point.h"
#include "lsst/afw/geom/Extent.h"
%}

%include "lsst/p_lsstSwig.i"
%import "lsst/afw/cameraGeom/cameraGeomLib.i"
%import "lsst/afw/geom/geomLib.i"

