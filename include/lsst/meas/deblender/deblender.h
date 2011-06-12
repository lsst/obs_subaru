// -*- LSST-C++ -*-
#if !defined(LSST_DEBLENDER_H)
#define LSST_DEBLENDER_H
//!

#include <list>

#include "lsst/afw/image/Image.h"
#include "lsst/afw/detection.h"
#include "lsst/meas/algorithms.h"

namespace afwImage = lsst::afw::image;
namespace afwDet = lsst::afw::detection;

namespace lsst {
    namespace meas {
        namespace deblender {

            template<typename ImageT>
            class DeblendedObject {
            public:
                typedef boost::shared_ptr< DeblendedObject<ImageT> > Ptr;

                std::vector<typename ImageT::Ptr> images;
                // pixel offset of "images" within the parent images.
                int x0;
                int y0;

                std::vector<afwDet::Footprint::Ptr> foots;

                /*
                 add this object's pixel values for "color" to the given image,
                 where the given "image" is in the parent's pixel coordinate
                 space (ie, this object's pixels are added to the rectangle
                 (x0,y0) to (x0+this->getWidth(), y0+this->getHeight()).
                 */
                void addToImage(typename ImageT::Ptr image, int color);
            };

            template<typename ImageT>
            class Deblender {
            public:

                virtual
                std::vector<typename DeblendedObject<ImageT>::Ptr >
                deblend(
                        boost::shared_ptr<std::vector< boost::shared_ptr< afwDet::Footprint > > > footprints,
                    std::vector< std::vector< boost::shared_ptr< afwDet::Peak > > > peaks,
                    boost::shared_ptr<typename afwImage::MaskedImage<typename ImageT::Pixel> > maskedImage,
                    boost::shared_ptr<typename afwDet::Psf > psf
                    ) = 0;

                virtual ~Deblender() {}
            };
			
            template<typename ImageT>
            class SDSSDeblender : public Deblender<ImageT> {
            public:
                SDSSDeblender();

                virtual
                std::vector<typename DeblendedObject<ImageT>::Ptr >
                deblend(
                        boost::shared_ptr<std::vector< boost::shared_ptr< afwDet::Footprint > > > footprints,
                    std::vector< std::vector< boost::shared_ptr< afwDet::Peak > > > peaks,
                    boost::shared_ptr<typename afwImage::MaskedImage<typename ImageT::Pixel> > maskedImage,
                    boost::shared_ptr<typename afwDet::Psf > psf
                    );

                std::vector< std::string > debugProfiles();

                virtual ~SDSSDeblender() {}
            };
			
        }}}

#endif
