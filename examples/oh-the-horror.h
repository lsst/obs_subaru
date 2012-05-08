#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"

template <typename ImagePixelT,
              typename MaskPixelT=lsst::afw::image::MaskPixel,
              typename VariancePixelT=lsst::afw::image::VariancePixel>
lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>
    test1() {
    return lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>();
}
