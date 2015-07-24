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

#include "lsst/obs/subaru/HscDistortion.h"
#include "boost/make_shared.hpp"

namespace lsst {
namespace obs {
namespace subaru {

DistortionPolynomial::DistortionPolynomial(
    int xOrder, int yOrder, Coeffs const& xCoeffs, Coeffs const& yCoeffs
    ) :
    _xOrder(xOrder),
    _yOrder(yOrder),
    _xCoeffs(xCoeffs),
    _yCoeffs(yCoeffs)
{
    if (xOrder < 0 || yOrder < 0) {
        throw LSST_EXCEPT(pex::exceptions::InvalidParameterError,
                          (boost::format("Negative order: %d, %d") % xOrder % yOrder).str());
    }
    if (xCoeffs.size() != static_cast<size_t>((xOrder + 1)*(xOrder + 2)/2) ||
        yCoeffs.size() != static_cast<size_t>((yOrder + 1)*(yOrder + 2)/2)) {
        throw LSST_EXCEPT(pex::exceptions::InvalidParameterError,
                          (boost::format("Mismatch between polynomial order (%d,%d) and number of "
                                         "coefficients (%d,%d)") %
                           xOrder % yOrder % xCoeffs.size() % yCoeffs.size()).str());
    }
}

DistortionPolynomial::DistortionPolynomial(
    DistortionPolynomial const& other
    ) :
    _xOrder(other._xOrder),
    _yOrder(other._yOrder),
    _xCoeffs(other._xCoeffs),
    _yCoeffs(other._yCoeffs)
{}

afw::geom::Point2D DistortionPolynomial::operator()(afw::geom::Point2D const& position) const
{
    double const x = position.getX(), y = position.getY();

    std::vector<double> yPoly(_yOrder + 1);
    yPoly[0] = 1.0;
    for (int iy = 1; iy <= _yOrder; ++iy) {
        yPoly[iy] = yPoly[iy - 1]*y;
    }

    double xValue = 0.0, yValue = 0.0;
    double xPoly = 1.0;
    auto xCoeffsIter = _xCoeffs.cbegin(), yCoeffsIter = _yCoeffs.cbegin();
    for (int ix = 0; ix <= _xOrder; ++ix, xPoly *= x) {
        auto yPolyIter = yPoly.cbegin();
        for (int iy = 0; iy <= _yOrder - ix; ++iy, ++yPolyIter, ++xCoeffsIter, ++yCoeffsIter) {
            assert(xCoeffsIter != _xCoeffs.cend());
            assert(yCoeffsIter != _yCoeffs.cend());
            assert(yPolyIter != yPoly.cend());
            double const poly = xPoly * *yPolyIter;
            xValue += *xCoeffsIter * poly;
            yValue += *yCoeffsIter * poly;
        }
    }
    return afw::geom::Point2D(xValue, yValue);
}

HscDistortion::HscDistortion(
    DistortionPolynomial const& skyToCcd,
    DistortionPolynomial const& ccdToSky,
    afw::geom::Angle const& plateScale,
    double tolerance,
    int maxIter
    ) :
    _skyToCcd(skyToCcd),
    _ccdToSky(ccdToSky),
    _inversionTolerance(tolerance),
    _maxIterations(maxIter),
    _scaling(plateScale.asRadians()) // PUPIL coordinate system is in radians
{}

PTR(afw::geom::XYTransform) HscDistortion::clone() const
{
    return boost::make_shared<HscDistortion>(_skyToCcd, _ccdToSky, _scaling*afw::geom::radians,
                                             _inversionTolerance, _maxIterations);
}

afw::geom::Point2D HscDistortion::forwardTransform(afw::geom::Point2D const &point) const
{
    afw::geom::Point2D sky = _ccdToSky(point);
    sky.scale(_scaling);
    return sky;
}

afw::geom::Point2D HscDistortion::reverseTransform(afw::geom::Point2D const &point) const
{
    afw::geom::Point2D original = point;
    original.scale(1.0/_scaling);
    afw::geom::Point2D ccd = _skyToCcd(original);

    // Tweak with iteration
    // Not sure why this is necessary, but it's what was done in the original implementation in distEst.
    for (int i = 0; i < _maxIterations; ++i) {
        afw::geom::Point2D const& sky = _ccdToSky(ccd);
        afw::geom::Extent2D const& diff = original - sky;
        if (::fabs(diff.getX()) < _inversionTolerance && ::fabs(diff.getY()) < _inversionTolerance) {
            return ccd;
        }
        ccd += diff;
    }

    throw LSST_EXCEPT(pex::exceptions::RuntimeError,
                      (boost::format("Too many iterations (%d) transforming %f,%f") %
                       _maxIterations % point.getX() % point.getY()).str());
}


}}}
