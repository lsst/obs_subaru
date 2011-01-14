%define meas_deblender_sdss_DOCSTRING
"
Python interface to SDSS photo internals, for debugging purposes
"
%enddef

%feature("autodoc", "1");

%module(package="lsst.meas.deblender", docstring=meas_deblender_sdss_DOCSTRING) sdss

%{

#include "phMeasureObj.h"
#include "phPeaks.h"
#include "phObjc.h"
#include "phSpanUtil.h"
#include "region.h"
#include "phObjc_p.h"
#include "phFake.h"
#include "phDgpsf.h"
#include "phExtract.h"

const struct cellgeom* cell_stats_get_cellgeom(const CELL_STATS* cs, int i) {
    return cs->geom + i;
}

%}

%ignore phApoApertureEvalNaive;
%ignore phApoApertureEval;
%ignore phApoApertureMomentsEval;

/*
%ignore phApoApertureEvalNaive(const REGION *reg, float r,	float asigma, float bkgd, float rowc, float colc, float *val);
%ignore phApoApertureEval(const REGION *reg, int ann, float asigma, float bkgd, float rowc, float colc, float *val);
*/

/*
%include "phMeasureObj.h"
%include "phPeaks.h"
%include "phObjc.h"
%include "phSpanUtil.h"
%include "region.h"
%include "phObjc_p.h"
%include "phFake.h"
%include "phDgpsf.h"
*/
%include "phExtract.h"

const struct cellgeom* cell_stats_get_cellgeom(const CELL_STATS* cs, int i);


