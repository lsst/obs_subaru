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
				class Deblender {
			public:
				virtual std::vector<typename ImageT::Ptr> deblend(std::vector<typename ImageT::Ptr> &images) = 0;
				virtual ~Deblender() {}
			};
			
			template<typename ImageT>
				class SDSSDeblender : public Deblender<ImageT> {
				//class SDSSDeblender : public Deblender {
			public:
				virtual std::vector<typename ImageT::Ptr> deblend(std::vector<typename ImageT::Ptr> &images);
				virtual ~SDSSDeblender() {}
			};
			
		}}}

#endif
