// -*- lsst-c++ -*-

/* 
 * LSST Data Management System
 * Copyright 2008-2015 LSST Corporation.
 * 
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the LSST License Statement and 
 * the GNU General Public License along with this program.  If not, 
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#include "lsst/obs/subaru/DistEstXYTransform.h"
#include "lsst/obs/hsc/distest.h"
#include "boost/make_shared.hpp"

namespace lsst {
namespace obs {
namespace subaru {

DistEstXYTransform::DistEstXYTransform(afw::geom::Angle const &elevation, double const plateScale)
    : afw::geom::XYTransform(), _elevation(elevation), _plateScale(afw::geom::arcsecToRad(plateScale))
{ }

PTR(afw::geom::XYTransform) DistEstXYTransform::clone() const
{
    return boost::make_shared<DistEstXYTransform> (_elevation, afw::geom::radToArcsec(_plateScale));
}

afw::geom::Point2D DistEstXYTransform::forwardTransform(afw::geom::Point2D const &point) const
{
    float x, y;
    lsst::obs::hsc::getUndistortedPosition(point.getX(), point.getY(), &x, &y, _elevation.asDegrees());
    return afw::geom::Point2D(x*_plateScale,y*_plateScale);
}

afw::geom::Point2D DistEstXYTransform::reverseTransformIterative(afw::geom::Point2D const &point) const
{
    float x, y;
    lsst::obs::hsc::getDistortedPositionIterative(point.getX(), point.getY(), &x, &y, _elevation.asDegrees());
    return afw::geom::Point2D(x/_plateScale,y/_plateScale);
}

afw::geom::Point2D DistEstXYTransform::reverseTransform(afw::geom::Point2D const &point) const
{ 
    float x, y;
    lsst::obs::hsc::getDistortedPosition(point.getX(), point.getY(), &x, &y, _elevation.asDegrees());
    return afw::geom::Point2D(x/_plateScale,y/_plateScale);
}

}}}
