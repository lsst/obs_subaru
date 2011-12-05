// -*- LSST-C++ -*-
#if !defined(LSST_DEBLENDER_BASELINE_H)
#define LSST_DEBLENDER_BASELINE_H
//!

#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/detection.h"

namespace lsst {
    namespace meas {
        namespace deblender {

            template <typename ImagePixelT,
                      typename MaskPixelT=lsst::afw::image::MaskPixel,
                      typename VariancePixelT=lsst::afw::image::VariancePixel>
            class BaselineUtils {

            public:
                typedef typename lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> MaskedImageT;
                typedef typename lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr MaskedImagePtrT;

                static typename MaskedImageT::Ptr
                buildSymmetricTemplate(MaskedImageT const& img,
                                       lsst::afw::detection::Footprint const& foot,
                                       lsst::afw::detection::Peak const& pk);
                
            };

            /*
             // rtn type:
             lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr
             // 
             buildSymmetricTemplate(
             lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> const& img,
             lsst::afw::detection::Footprint const& foot,
             lsst::afw::detection::Peak const& pk);
             */
        }
    }
}

#endif
