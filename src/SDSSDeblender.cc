// -*- lsst-c++ -*-
/**
 * @author Dustin Lang
 *
 */
#include <vector>

#include "lsst/meas/deblender/deblender.h"

namespace image = lsst::afw::image;
namespace deblender = lsst::meas::deblender;
//namespace ex    = lsst::pex::exceptions;


template<typename ImageT>
std::vector<typename ImageT::Ptr>
deblender::SDSSDeblender<ImageT>::deblend(std::vector<typename ImageT::Ptr> &images) {
	std::vector<typename ImageT::Ptr> children;
	return children;
}


/*
 * Explicit Instantiations
 *
 */
/*
#define INSTANTIATE_DEBLENDER(TYPE) \
    template std::vector<image::Image<TYPE>::Ptr> \
	deblender::SDSSDeblender<image::Image<TYPE> >( \
            std::vector<image::Image<TYPE>::Ptr > &images);
INSTANTIATE_DEBLENDER(double);
INSTANTIATE_DEBLENDER(float);
 */

template class deblender::SDSSDeblender<image::Image<double> >;
template class deblender::SDSSDeblender<image::Image<float > >;
