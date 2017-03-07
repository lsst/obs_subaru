/*
 * LSST Data Management System
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 * See the COPYRIGHT file
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
 * see <https://www.lsstcorp.org/LegalNotices/>.
 */
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "lsst/afw/geom/XYTransform.h"
#include "lsst/obs/subaru/HscDistortion.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace obs {
namespace subaru {

PYBIND11_PLUGIN(_hscDistortion) {
    py::module mod("_hscDistortion", "Python wrapper for afw _hscDistortion library");

    py::class_<DistortionPolynomial, std::shared_ptr<DistortionPolynomial>> clsDistortionPolynomial(
            mod, "DistortionPolynomial");

    clsDistortionPolynomial.def(py::init<int, int, typename DistortionPolynomial::Coeffs const&,
                                         typename DistortionPolynomial::Coeffs const&>(),
                                "xOrder"_a, "yOrder"_a, "xCoeffs"_a, "yCoeffs"_a);
    clsDistortionPolynomial.def(py::init<DistortionPolynomial const&>());

    clsDistortionPolynomial.def("getXOrder", &DistortionPolynomial::getXOrder);
    clsDistortionPolynomial.def("getYOrder", &DistortionPolynomial::getYOrder);
    clsDistortionPolynomial.def("getXCoeffs", &DistortionPolynomial::getXCoeffs);
    clsDistortionPolynomial.def("getYCoeffs", &DistortionPolynomial::getYCoeffs);

    clsDistortionPolynomial.def("__call__", &DistortionPolynomial::operator());

    py::class_<HscDistortion, std::shared_ptr<HscDistortion>, afw::geom::XYTransform> clsHscDistortion(
            mod, "HscDistortion");

    clsHscDistortion.def(py::init<DistortionPolynomial const&, DistortionPolynomial const&,
                                  afw::geom::Angle const&, double, int>(),
                         "skyToCcd"_a, "ccdToSky"_a, "plateScale"_a = 1.0 * afw::geom::arcseconds,
                         "tolerance"_a = 5.0e-3, "maxIter"_a = 10);

    clsHscDistortion.def("clone", &HscDistortion::clone);
    clsHscDistortion.def("forwardTransform", &HscDistortion::forwardTransform);
    clsHscDistortion.def("reverseTransform", &HscDistortion::reverseTransform);

    return mod.ptr();
}
}
}
}  // lsst::obs::subaru
