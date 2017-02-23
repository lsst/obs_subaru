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

#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/detection/Footprint.h"
#include "lsst/afw/detection/Peak.h"

#include "lsst/meas/deblender/BaselineUtils.h"

namespace py = pybind11;
using namespace pybind11::literals;

namespace lsst {
namespace meas {
namespace deblender {

namespace {
template <typename ImagePixelT, typename MaskPixelT = lsst::afw::image::MaskPixel,
          typename VariancePixelT = lsst::afw::image::VariancePixel>
void declareBaselineUtils(py::module& mod, const std::string& suffix) {
    using MaskedImageT = lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>;
    using ImagePtrT = std::shared_ptr<lsst::afw::image::Image<ImagePixelT>>;
    using FootprintPtrT = std::shared_ptr<lsst::afw::detection::Footprint>;

    py::class_<BaselineUtils<ImagePixelT, MaskPixelT, VariancePixelT>> cls(
            mod, ("BaselineUtils" + suffix).c_str());
    cls.def_static("symmetrizeFootprint",
                   &BaselineUtils<ImagePixelT, MaskPixelT, VariancePixelT>::symmetrizeFootprint, "foot"_a,
                   "cx"_a, "cy"_a);
    // The C++ function returns a std::pair return value but also takes a referenced boolean
    // (patchedEdges) that is modified by the function and used by the python API,
    // so we wrap this in a lambda to combine the std::pair and patchedEdges in a tuple
    // that is returned to python.
    cls.def_static(
            "buildSymmetricTemplate",
            [](MaskedImageT const& img, lsst::afw::detection::Footprint const& foot,
               lsst::afw::detection::PeakRecord const& pk, double sigma1, bool minZero, bool patchEdges) {
                bool patchedEdges;
                std::pair<ImagePtrT, FootprintPtrT> result;

                result = BaselineUtils<ImagePixelT, MaskPixelT, VariancePixelT>::buildSymmetricTemplate(
                        img, foot, pk, sigma1, minZero, patchEdges, &patchedEdges);
                return py::make_tuple(result.first, result.second, patchedEdges);
            });
    cls.def_static("medianFilter", &BaselineUtils<ImagePixelT, MaskPixelT, VariancePixelT>::medianFilter,
                   "img"_a, "outimg"_a, "halfsize"_a);
    cls.def_static("makeMonotonic", &BaselineUtils<ImagePixelT, MaskPixelT, VariancePixelT>::makeMonotonic,
                   "img"_a, "pk"_a);
    // apportionFlux expects an empty vector containing HeavyFootprint pointers that is modified
    // in the function. But when a list is passed to pybind11 in place of the vector,
    // the changes are not passed back to python. So instead we create the vector in this lambda and
    // include it in the return value.
    cls.def_static("apportionFlux",
                   [](MaskedImageT const& img, lsst::afw::detection::Footprint const& foot,
                      std::vector<typename std::shared_ptr<lsst::afw::image::Image<ImagePixelT>>> templates,
                      std::vector<std::shared_ptr<lsst::afw::detection::Footprint>> templ_footprints,
                      ImagePtrT templ_sum, std::vector<bool> const& ispsf, std::vector<int> const& pkx,
                      std::vector<int> const& pky, int strayFluxOptions, double clipStrayFluxFraction) {
                       using HeavyFootprintPtrList =
                               std::vector<std::shared_ptr<typename lsst::afw::detection::HeavyFootprint<
                                       ImagePixelT, MaskPixelT, VariancePixelT>>>;

                       std::vector<typename std::shared_ptr<
                               lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>>>
                               result;
                       HeavyFootprintPtrList strays;
                       result = BaselineUtils<ImagePixelT, MaskPixelT, VariancePixelT>::apportionFlux(
                               img, foot, templates, templ_footprints, templ_sum, ispsf, pkx, pky, strays,
                               strayFluxOptions, clipStrayFluxFraction);

                       return py::make_tuple(result, strays);
                   });
    cls.def_static("hasSignificantFluxAtEdge",
                   &BaselineUtils<ImagePixelT, MaskPixelT, VariancePixelT>::hasSignificantFluxAtEdge, "img"_a,
                   "sfoot"_a, "thresh"_a);
    cls.def_static("getSignificantEdgePixels",
                   &BaselineUtils<ImagePixelT, MaskPixelT, VariancePixelT>::getSignificantEdgePixels, "img"_a,
                   "sfoot"_a, "thresh"_a);
    // There appears to be an issue binding to a static const member of a templated type, so for now
    // we just use the values constants
    cls.attr("ASSIGN_STRAYFLUX") = py::cast(0x1);
    cls.attr("STRAYFLUX_TO_POINT_SOURCES_WHEN_NECESSARY") = py::cast(0x2);
    cls.attr("STRAYFLUX_TO_POINT_SOURCES_ALWAYS") = py::cast(0x4);
    cls.attr("STRAYFLUX_R_TO_FOOTPRINT") = py::cast(0x8);
    cls.attr("STRAYFLUX_NEAREST_FOOTPRINT") = py::cast(0x10);
    cls.attr("STRAYFLUX_TRIM") = py::cast(0x20);
};
}  // namespace

PYBIND11_PLUGIN(_baselineUtils) {
    py::module::import("lsst.afw.image");
    py::module::import("lsst.afw.detection");
    py::module mod("_baselineUtils", "Python wrapper for meas_deblender _baselineUtils library");

    declareBaselineUtils<float>(mod, "F");

    return mod.ptr();
}
}
}
}  // lsst::meas::deblender
