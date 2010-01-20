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

// from phGaussdev
double random_gaussian() {
    double v1, v2, r, fac;
    do {
        v1 = drand48() * 2 - 1;
        v2 = drand48() * 2 - 1;
        r = v1*v1 + v2*v2;
    } while (r >= 1.0);
    fac = sqrt(-2.0*log(r)/r);
    return v2*fac;
}

BOOST_AUTO_TEST_CASE(TwoStarDeblend) {
    int const W = 64;
    int const H = 64;

    // MaskedImage = Image + Mask + var Image
    // Exposure = MaskedImage + WCS
    //float cx[] = { 28, 38 };
    float cx[] = { 26, 40 };
    float cy[] = { 32, 32 };
    float flux[] = {1000, 2000};
    float s = 3;
    float sky = 100;
    int noise = 10;

    ImageF::Ptr img(new ImageF(W, H, sky));

    // add a Gaussian to the image...
    for (int y=0; y != img->getHeight(); ++y) {
        int x=0;
        for (ImageF::x_iterator ptr = img->row_begin(y), end = img->row_end(y); ptr != end; ++ptr, x++) {
            for (int i=0; i<(int)(sizeof(cx)/sizeof(float)); i++)
                *ptr += flux[i]/(s*s) * exp(-0.5 * ((x-cx[i])*(x-cx[i]) + (y-cy[i])*(y-cy[i])) / (s*s));
        }
    }
    // add a bit of noise...
    for (int y=0; y != img->getHeight(); ++y) {
        int x=0;
        for (ImageF::x_iterator ptr = img->row_begin(y), end = img->row_end(y); ptr != end; ++ptr, x++) {
            //*ptr += (std::rand() % noise) - (float)noise/2.0;
            *ptr += random_gaussian() * noise;
        }
    }

    // DEBUG - save the image to disk
    img->writeFits("test2.fits");

    std::vector<ImageF::Ptr> imgList;
    // two bands...
    //imgList.push_back(img);
    imgList.push_back(img);

    /// do detection, build region masks, ...

    // afw::detection::Footprint ?

    deblender::SDSSDeblender<ImageF> db;
    std::vector<deblender::DeblendedObject<ImageF>::Ptr> childList = db.OLDdeblend(imgList);

    BOOST_CHECK_EQUAL(childList.size(), 2U);

    for (size_t i=0; i<childList.size(); i++) {
        std::printf("child %i: %p\n", (int)i, childList[i].get());
        deblender::DeblendedObject<ImageF>::Ptr child = childList[i];
        for (size_t c=0; c<child->images.size(); c++) {
            ImageF::Ptr cimg = child->images[c];
            std::printf("child %i, color %i: offset (%i,%i), size %i x %i\n", (int)i, (int)c, child->x0, child->y0, cimg->getWidth(), cimg->getHeight());
            char fn[256];
            sprintf(fn, "test-child%02i-color%02i.fits", (int)i, (int)c);
            //cimg->writeFits(fn);

            ImageF::Ptr pimg(new ImageF(W, H, 0));
            child->addToImage(pimg, c);
            std::printf("saving as %s\n", fn);
            pimg->writeFits(fn);
        }
    }

    // check that the deblended children sum to the original image (flux-preserving)
    for (size_t i=0; i<imgList.size(); i++) {
        ImageF::Ptr pimg(new ImageF(W, H, 0));
        for (size_t j=0; j<childList.size(); j++) {
            childList[j]->addToImage(pimg, i);
        }
        char fn[256];
        sprintf(fn, "test-sum-color%02i.fits", (int)i);
        std::printf("saving as %s\n", fn);
        pimg->writeFits(fn);

        // in this test case, there is a margin (2 pix top and left, 1 pix
        // bottom and right) this is not covered by the children.
        int mxlo = 2;
        int mylo = 2;
        int mxhi = 1;
        int myhi = 1;

        int checkedpix = 0;
        for (int y=mylo; y<(H-myhi); y++) {
            for (int x=mxlo; x<(W-mxhi); x++) {
                std::printf("pix diff %g\n", sky + (*pimg)(x,y) - (*imgList[i])(x,y));
                BOOST_CHECK(fabs(sky + (*pimg)(x,y) - (*imgList[i])(x,y)) < noise);
                checkedpix++;
            }
        }
        BOOST_CHECK_EQUAL(checkedpix, 3721);
        // the margins should be zero
        for (int y=0; y<mylo; y++)
            for (int x=0; x<W; x++) {
                BOOST_CHECK_EQUAL((*pimg)(x,y), 0);
                checkedpix++;
            }
        for (int y=(H-myhi); y<H; y++)
            for (int x=0; x<W; x++) {
                BOOST_CHECK_EQUAL((*pimg)(x,y), 0);
                checkedpix++;
            }
        for (int y=0; y<H; y++) {
            for (int x=0; x<mxlo; x++) {
                BOOST_CHECK_EQUAL((*pimg)(x,y), 0);
                checkedpix++;
            }
            for (int x=(W-mxhi); x<W; x++) {
                BOOST_CHECK_EQUAL((*pimg)(x,y), 0);
                checkedpix++;
            }
        }
        // MAGIC 9: we double-counted the corners (9 = 2*2 + 2*1 + 1*2 + 1)
        BOOST_CHECK_EQUAL(checkedpix, W*H + 9);
    }
}

