#include "lsst/obs/hsc/HscDistortion.h"

#ifdef HAVE_HSC_DISTEST

#include "hsc/meas/match/distest.h"

#include "lsst/afw/geom/Point.h"

lsst::afw::geom::Point2D lsst::obs::hsc::HscDistortion::_transform(
    lsst::afw::geom::Point2D const& p,
    bool forward
    ) const
{
    float x, y;                     // Transformed position
    if (!forward) {
        ::hsc::meas::match::getUndistortedPosition(p.getX(), p.getY(), &x, &y, _elevation.asDegrees());
    } else if (_iterative) {
        ::hsc::meas::match::getDistortedPositionIterative(p.getX(), p.getY(), &x, &y, _elevation.asDegrees());
    } else {
        ::hsc::meas::match::getDistortedPosition(p.getX(), p.getY(), &x, &y, _elevation.asDegrees());
    }
    return afw::geom::Point2D(x, y);
}

#else
#include "lsst/pex/exceptions.h"

lsst::afw::geom::Point2D lsst::obs::hsc::HscDistortion::_transform(
    lsst::afw::geom::Point2D const& p,
    bool forward
    ) const
{
    throw LSST_EXCEPT(
        lsst::pex::exceptions::RuntimeError,
        "HscDistortion was not compiled with access to the HSC distEst package"
        );
}


#endif
