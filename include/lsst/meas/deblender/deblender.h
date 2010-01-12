// -*- LSST-C++ -*-
#if !defined(LSST_DEBLENDER_H)
#define LSST_DEBLENDER_H
//!

#include <list>

#include "lsst/afw/image/Image.h"

namespace image = lsst::afw::image;

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
				virtual std::vector<typename DeblendedObject<ImageT>::Ptr > deblend(std::vector<typename ImageT::Ptr> &images) = 0;
				virtual ~Deblender() {}
			};
			
			template<typename ImageT>
				class SDSSDeblender : public Deblender<ImageT> {
			public:
				virtual std::vector<typename DeblendedObject<ImageT>::Ptr > deblend(std::vector<typename ImageT::Ptr> &images);
				//virtual std::vector<typename ImageT::Ptr> deblend(std::vector<typename ImageT::Ptr> &images);
				virtual ~SDSSDeblender() {}
			};
			
		}}}

#endif
