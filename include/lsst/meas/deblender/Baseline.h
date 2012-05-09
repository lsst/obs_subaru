// -*- LSST-C++ -*-
#if !defined(LSST_DEBLENDER_BASELINE_H)
#define LSST_DEBLENDER_BASELINE_H
//!

#include <vector>
#include <utility>

#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
//#include "lsst/afw/detection.h"
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
                typedef typename lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr MaskedImagePtrT;
                typedef typename lsst::afw::image::Image<ImagePixelT> ImageT;
                typedef typename lsst::afw::image::Image<ImagePixelT>::Ptr ImagePtrT;
                typedef typename lsst::afw::image::Mask<MaskPixelT> MaskT;
                typedef typename lsst::afw::image::Mask<MaskPixelT>::Ptr MaskPtrT;

                typedef typename lsst::afw::detection::Footprint FootprintT;
                typedef typename lsst::afw::detection::Footprint::Ptr FootprintPtrT;
                typedef typename lsst::afw::detection::HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> HeavyFootprintT;

                static std::vector<double>
                fitEllipse(ImageT const& img,
                           double bkgd, double xc, double yc);

                static
                lsst::afw::detection::Footprint::Ptr
                symmetrizeFootprint(lsst::afw::detection::Footprint const& foot,
                                    int cx, int cy);

                //typename lsst::afw::image::MaskedImage<ImagePixelT,MaskPixelT,VariancePixelT>::Ptr
                static
                std::pair<MaskedImagePtrT, FootprintPtrT>
                buildSymmetricTemplate(MaskedImageT const& img,
                                       lsst::afw::detection::Footprint const& foot,
                                       lsst::afw::detection::Peak const& pk,
                                       double sigma1);

                static void
                medianFilter(MaskedImageT const& img,
                             MaskedImageT & outimg,
                             int halfsize);

                static void
                makeMonotonic(MaskedImageT & img,
                              lsst::afw::detection::Footprint const& foot,
                              lsst::afw::detection::Peak const& pk,
                    double sigma1);

                // Spelled out for swig's benefit...
                //static std::vector<MaskedImagePtrT>
                static std::vector<typename lsst::afw::image::MaskedImage<ImagePixelT,MaskPixelT,VariancePixelT>::Ptr>
                apportionFlux(MaskedImageT const& img,
                              lsst::afw::detection::Footprint const& foot,
                              std::vector<typename lsst::afw::image::MaskedImage<ImagePixelT,MaskPixelT,VariancePixelT>::Ptr>);

            };
        }
    }
}

#endif
