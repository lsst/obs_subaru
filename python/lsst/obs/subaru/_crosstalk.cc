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

#include "lsst/obs/subaru/Crosstalk.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace obs {
namespace subaru {

// Note that _crosstalk is not related to the crosstalk Python module;
// unfortunately they do have the same name.
PYBIND11_MODULE(_crosstalk, mod) {
    py::module::import("lsst.afw.image");

    mod.def("subtractCrosstalk", subtractCrosstalk, "mi"_a, "nAmp"_a, "coeffs1List"_a, "coeffs2List"_a,
            "gainsPreampSig"_a);
}

}  // subaru
}  // obs
}  // lsst
