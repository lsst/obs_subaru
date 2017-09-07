#ifndef LSST_OBS_SUPRIMECAM_CROSSTALK_H
#define LSST_OBS_SUPRIMECAM_CROSSTALK_H

#include <vector>
#include "lsst/afw/image/MaskedImage.h"

/*
 * @file Crosstalk.h
 * @brief C++ implementation of crosstalk correction with procedure by Yagi+2012
 * @ingroup obs_subaru
 * @author Hisanori Furusawa
 *
 */

namespace lsst {
namespace obs {
namespace subaru {

typedef std::vector<double> CoeffVector;

/// Subtract the crosstalk from an image
void subtractCrosstalk(
    lsst::afw::image::MaskedImage<float> & mi, ///< Image to correct
    std::size_t nAmp,                          ///< Number of amplifiers
    std::vector<CoeffVector> const & coeffs1List, ///< Coefficients for primary crosstalk
    std::vector<CoeffVector> const & coeffs2List, ///< Coefficients for secondary crosstalk
    CoeffVector const & gainsPreampSig            ///< Relative gains (pre-amp and SIG board)
    );

}}} // namespace lsst::obs::subaru

#endif
