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
 * @brief Class for modeling the HSC distortion with an XYTransform API
 */
#ifndef LSST_OBS_SUBARU_HSCDISTORTION_H
#define LSST_OBS_SUBARU_HSCDISTORTION_H

#include <vector>

#include "lsst/afw/geom/XYTransform.h"
#include "lsst/afw/geom/Point.h"

namespace lsst {
namespace obs {
namespace subaru {

// Would have preferred to use an afw Polynomial, but we have the
// coefficients from a different scheme.
class DistortionPolynomial
{
public:
    typedef std::vector<double> Coeffs;

    /// Construct with custom coefficients
    DistortionPolynomial(int xOrder, int yOrder, Coeffs const& xCoeffs, Coeffs const& yCoeffs);

    /// Copy Ctor
    DistortionPolynomial(DistortionPolynomial const& other);

    /// Accessors
    int getXOrder() const { return _xOrder; }
    int getYOrder() const { return _yOrder; }
    Coeffs const& getXCoeffs() const { return _xCoeffs; }
    Coeffs const& getYCoeffs() const { return _yCoeffs; }

    afw::geom::Point2D operator()(afw::geom::Point2D const& position) const;

private:
    int const _xOrder, _yOrder;      ///< Polynomial order in x and y
    Coeffs const _xCoeffs, _yCoeffs; ///< Coefficients for polynomials in x and y
};

/**
 * @brief An XYTransform to model the HSC distortion
 */
class HscDistortion : public afw::geom::XYTransform
{
public:
    HscDistortion(DistortionPolynomial const& skyToCcd, DistortionPolynomial const& ccdToSky,
                  afw::geom::Angle const& plateScale=1.0*afw::geom::arcseconds,
                  double tolerance=5.0e-3, int maxIter=10);
    virtual PTR(afw::geom::XYTransform) clone() const;
    virtual afw::geom::Point2D forwardTransform(afw::geom::Point2D const &point) const;
    virtual afw::geom::Point2D reverseTransform(afw::geom::Point2D const &point) const;

private:
    DistortionPolynomial _skyToCcd;     ///< Polynomial for converting sky position to CCD position
    DistortionPolynomial _ccdToSky;     ///< Polynomial for converting CCD position to sky position
    double _inversionTolerance;         ///< Tolerance for inversion
    int _maxIterations;                 ///< Maximum number of iterations
    double _scaling;                    ///< Central plate scale, arcsec/mm
};



}}}
#endif
