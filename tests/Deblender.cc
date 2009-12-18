// -*- lsst-c++ -*-
/**
 * @author Dustin Lang
 */
#include <iostream>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Deblender

#include "boost/test/unit_test.hpp"
#include "boost/test/floating_point_comparison.hpp"

#include "lsst/afw/image/Image.h"
#include "lsst/meas/deblender/deblender.h"

namespace image = lsst::afw::image;
namespace deblender = lsst::meas::deblender;

typedef image::Image<float> ImageF;

BOOST_AUTO_TEST_CASE(TwoStarDeblend) {
    int const W = 64;
    int const H = 64;

    // MaskedImage = Image + Mask + var Image
    // Exposure = MaskedImage + WCS
    float cx[] = { 28, 38 };
    float cy[] = { 32, 32 };
    float flux[] = {1000, 2000};
    float s = 3;
    float sky = 100;

    ImageF::Ptr img = ImageF::Ptr(new ImageF(W, H, sky));

    // add a Gaussian to the image...
    for (int y=0; y != img->getHeight(); ++y) {
        int x=0;
        for (ImageF::x_iterator ptr = img->row_begin(y), end = img->row_end(y); ptr != end; ++ptr, x++) {
            for (int i=0; i<(int)(sizeof(cx)/sizeof(float)); i++)
                *ptr += flux[i]/(s*s) * exp(-0.5 * ((x-cx[i])*(x-cx[i]) + (y-cy[i])*(y-cy[i])) / (s*s));
        }
    }
    // DEBUG - save the image to disk
    img->writeFits("test2.fits");

    std::vector<ImageF::Ptr> imgList;
    // two bands...
    //imgList.push_back(img);
    imgList.push_back(img);

    /// do detection, build region masks, ...

    deblender::SDSSDeblender<ImageF> db;
    std::vector<ImageF::Ptr> childList = db.deblend(imgList);

    BOOST_CHECK_EQUAL(childList.size(), 2U);

}

