// -*- LSST-C++ -*-
#if !defined(LSST_DEBLENDER_H)
#define LSST_DEBLENDER_H
//!

#include <list>

#include "lsst/afw/image/Image.h"
#include "lsst/afw/detection.h"
//#include "lsst/afw/detection/Footprint.h"

namespace image = lsst::afw::image;
namespace det = lsst::afw::detection;

namespace lsst {
	namespace meas {
		namespace deblender {

			template<typename ImageT>
				class DeblendedObject {
            public:
                typedef boost::shared_ptr< DeblendedObject<ImageT> > Ptr;

				std::vector<typename ImageT::Ptr> images;
                // pixel offset of "image" within the parent image.
                int x0;
                int y0;

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

                // FIXME -- I'm pretty sure this is a bad idea...
                template<typename OtherPixelT>
				virtual
                //std::vector<typename DeblendedObject<ImageT>::Ptr >
                //std::vector< boost::shared_ptr< DeblendedObject<typename ImageT> > >
				std::vector<typename DeblendedObject<ImageT>::Ptr >
                deblend(
                    //std::vector< det::Footprint::Ptr > footprints,
                    std::vector< boost::shared_ptr< lsst::afw::detection::Footprint > > footprints,
                    //std::vector< std::vector< det::Peak::Ptr > > peaks
                    std::vector< std::vector< boost::shared_ptr< lsst::afw::detection::Peak > > > peaks,
                    //boost::shared_ptr<typename lsst::afw::image::MaskedImage< ImageT, lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel > > maskedImage
                    boost::shared_ptr<typename lsst::afw::image::MaskedImage< typename OtherPixelT > > maskedImage
                    //boost::shared_ptr< lsst::afw::image::MaskedImage< typename ImageT, lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel > > maskedImage
                    ) = 0;

				virtual std::vector<typename DeblendedObject<ImageT>::Ptr > OLDdeblend(std::vector<typename ImageT::Ptr> &images) = 0;

				virtual ~Deblender() {}
			};
			
			template<typename ImageT>
				class SDSSDeblender : public Deblender<ImageT> {
			public:
                SDSSDeblender();

				virtual std::vector<typename DeblendedObject<ImageT>::Ptr > OLDdeblend(std::vector<typename ImageT::Ptr> &images);
				//virtual std::vector<typename ImageT::Ptr> deblend(std::vector<typename ImageT::Ptr> &images);

                template<typename OtherPixelT>
                virtual
				std::vector<typename DeblendedObject<ImageT>::Ptr >
                deblend(
                    //std::vector< det::Footprint::Ptr > footprints,
                    std::vector< boost::shared_ptr< lsst::afw::detection::Footprint > > footprints,
                    //std::vector< std::vector< det::Peak::Ptr > > peaks
                    std::vector< std::vector< boost::shared_ptr< lsst::afw::detection::Peak > > > peaks,
                    //boost::shared_ptr<typename lsst::afw::image::MaskedImage< ImageT, lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel > > maskedImage
                    //boost::shared_ptr< lsst::afw::image::MaskedImage< typename ImageT, lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel > > maskedImage
                    //boost::shared_ptr<typename lsst::afw::image::MaskedImage< ImageT > > maskedImage
                    boost::shared_ptr<typename lsst::afw::image::MaskedImage< OtherPixelT > > maskedImage
                    );

				virtual ~SDSSDeblender() {}
			};
			
		}}}

#endif
