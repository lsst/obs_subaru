// -*- lsst-c++ -*-
/**
 * @author Dustin Lang
 *
 */
#include <iostream>
#include <vector>

#include "lsst/meas/deblender/deblender.h"

// Photo
namespace lsst { namespace meas { namespace deblender { namespace photo {
extern "C" {
#include "phMeasureObj.h"
}
}}}}

namespace image = lsst::afw::image;
namespace deblender = lsst::meas::deblender;
namespace photo = lsst::meas::deblender::photo;
//namespace ex    = lsst::pex::exceptions;


template<typename ImageT>
std::vector<typename ImageT::Ptr>
deblender::SDSSDeblender<ImageT>::deblend(std::vector<typename ImageT::Ptr> &images) {
	std::vector<typename ImageT::Ptr> children;

    //photo::OBJC myobjc;
    //photo::FIELDPARAMS myfp;
    //photo::OBJC* objc = &myobjc;
    //photo::FIELDPARAMS* fp = &myfp;
    char* filters = new char[images.size()+1];
    for (size_t i=0; i<images.size(); i++)
        filters[i] = 'r';
    filters[images.size()] = '\0';

    photo::OBJC* objc = photo::phObjcNew(images.size());
    photo::FIELDPARAMS* fp = photo::phFieldparamsNew(filters);


	int res = photo::phObjcDeblend(objc, fp);

    std::cout << "Result: " << res << std::endl;

    delete filters;
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
