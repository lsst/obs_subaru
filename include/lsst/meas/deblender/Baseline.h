// -*- LSST-C++ -*-
#if !defined(LSST_DEBLENDER_BASELINE_H)
#define LSST_DEBLENDER_BASELINE_H
//!

#include <vector>
#include <utility>

#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/detection/Footprint.h"
#include "lsst/afw/detection/Peak.h"

namespace lsst {
    namespace meas {
        namespace deblender {

            template <typename ImagePixelT,
                      typename MaskPixelT=lsst::afw::image::MaskPixel,
                      typename VariancePixelT=lsst::afw::image::VariancePixel>
            class BaselineUtils {

            public:
                typedef typename lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> MaskedImageT;
                typedef typename PTR(lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>) MaskedImagePtrT;
                typedef typename lsst::afw::image::Image<ImagePixelT> ImageT;
                typedef typename PTR(lsst::afw::image::Image<ImagePixelT>) ImagePtrT;
                typedef typename lsst::afw::image::Mask<MaskPixelT> MaskT;
                typedef typename PTR(lsst::afw::image::Mask<MaskPixelT>) MaskPtrT;

                typedef typename lsst::afw::detection::Footprint FootprintT;
                typedef typename PTR(lsst::afw::detection::Footprint) FootprintPtrT;
                typedef typename lsst::afw::detection::HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> HeavyFootprintT;

                static std::vector<double>
                fitEllipse(ImageT const& img,
                           double bkgd, double xc, double yc);

                static
                PTR(lsst::afw::detection::Footprint)
                symmetrizeFootprint(lsst::afw::detection::Footprint const& foot,
                                    int cx, int cy);

                static
                std::pair<MaskedImagePtrT, FootprintPtrT>
                buildSymmetricTemplate(MaskedImageT const& img,
                                       lsst::afw::detection::Footprint const& foot,
                                       lsst::afw::detection::Peak const& pk,
                                       double sigma1,
                                       bool minZero);

                static void
                medianFilter(MaskedImageT const& img,
                             MaskedImageT & outimg,
                             int halfsize);

                static void
                makeMonotonic(MaskedImageT & img,
                              lsst::afw::detection::Footprint const& foot,
                              lsst::afw::detection::Peak const& pk,
                              double sigma1);

                // swig doesn't seem to understand std::vector<MaskedImagePtrT>...
                static
                std::vector<typename PTR(lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>)>
                 apportionFlux(MaskedImageT const& img,
                              lsst::afw::detection::Footprint const& foot,
                              std::vector<typename PTR(lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>)>,
                              ImagePtrT sumimg = ImagePtrT());

            };
        }
    }
}

#endif
