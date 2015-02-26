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

/**
 * \file
 * @brief Class wrapping the distEst distortion class to present the XYTransform API
 */
#ifndef LSST_OBS_SUBARU_DISTESTXYTRANSFORM_H
#define LSST_OBS_SUBARU_DISTESTXYTRANSFORM_H

#include "lsst/daf/base.h"
#include "lsst/afw/geom/AffineTransform.h"
#include "lsst/afw/geom/Angle.h"
#include "lsst/afw/geom/XYTransform.h"

namespace lsst {
namespace obs {
namespace subaru {


/**
 * @brief An XYTransform to wrap the distEst distortion class for inclusion in an XYTransformMap.
 */
class DistEstXYTransform : public afw::geom::XYTransform
{
public:
    DistEstXYTransform(afw::geom::Angle const &elevation, double const plateScale);
    virtual PTR(afw::geom::XYTransform) clone() const;
    virtual afw::geom::Point2D forwardTransform(afw::geom::Point2D const &point) const;
    virtual afw::geom::Point2D reverseTransformIterative(afw::geom::Point2D const &point) const;
    virtual afw::geom::Point2D reverseTransform(afw::geom::Point2D const &point) const;

private:
    afw::geom::Angle _elevation;
    double _plateScale;
};



}}}
#endif
