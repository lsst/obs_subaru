// -*- lsst-c++ -*-
/**
 * @author Dustin Lang
 */
#include <iostream>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Deblender

#include "boost/test/unit_test.hpp"
#include "boost/test/floating_point_comparison.hpp"

//#include "lsst/afw/image/Image.h"
//#include "lsst/meas/algorithms/PSF.h"
//#include "lsst/afw/math/Background.h"

#include "lsst/afw/image.h"
#include "lsst/afw/math.h"
#include "lsst/afw/detection.h"
#include "lsst/meas/algorithms.h"

#include "lsst/meas/deblender/deblender.h"

namespace afwImage = lsst::afw::image;
namespace afwDet = lsst::afw::detection;
namespace measAlg = lsst::meas::algorithms;
namespace afwMath = lsst::afw::math;
namespace deblender = lsst::meas::deblender;

typedef float PixelT;
typedef afwImage::Image<PixelT> Image;
typedef afwImage::MaskedImage<PixelT> MImage;
typedef measAlg::PSF PSF;

// MaskedImage = Image + Mask + var Image
// Exposure = MaskedImage + WCS

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

#define FOR_EACH_PIXEL(img, it)                                         \
    for (Image::iterator it = img->begin(), end = img->end(); it != end; it++)

//template afwImage::Image<PixelT>::Ptr afwMath::Background::getImage() const;

BOOST_AUTO_TEST_CASE(TwoStarDeblend) {
    int const W = 64;
    int const H = 64;

    float cx[] = { 28.4, 38.1 };
    float cy[] = { 32.8, 32.5 };
    float flux[] = {2000, 4000};
    // single-Gaussian PSF sigma
    float truepsfsigma = 2;
    float sky = 100;
    float skysigma = sqrt(sky);
    // number of peaks
    int NP = sizeof(cx)/sizeof(float);

    PSF::Ptr truepsf = measAlg::createPSF("DoubleGaussian", 10, 10, truepsfsigma, 0, 0.0);
    PSF::Ptr psf = truepsf;
    float psfsigma = truepsfsigma;

    // generate sky + noise.
    Image::Ptr skyimg(new Image(W, H, 0));
    // for (Image::iterator it = skyimg->begin(), end = skyimg->end(); it != end; it++)
    FOR_EACH_PIXEL(skyimg, it)
        // ~ Poisson
        *it = sky + random_gaussian() * skysigma;

    // generate star images (PSFs)
    std::vector<Image::Ptr> starimgs;
    for (int i=0; i<NP; i++) {
        Image::Ptr starimg(new Image(W, H, 0));
        for (int y=0; y != starimg->getHeight(); ++y) {
            int x=0;
            for (Image::x_iterator ptr = starimg->row_begin(y), end = starimg->row_end(y); ptr != end; ++ptr, x++) {
                *ptr += psf->getValue(x-cx[i], y-cy[i], x, y);
            }
        }
        // normalize and scale by flux -- wrong if source is close to the edge.
        double imgsum = 0;
        for (Image::iterator it = starimg->begin(), end = starimg->end(); it != end; it++)
            imgsum += *it;
        for (Image::iterator it = starimg->begin(), end = starimg->end(); it != end; it++)
            *it *= flux[i] / imgsum;
        starimgs.push_back(starimg);
    }

    // sum the sky and star images
    MImage::Ptr mimage(new MImage(W, H));
    Image::Ptr img(new Image(W, H, 0));
    Image::iterator imgit = img->begin();
    FOR_EACH_PIXEL(skyimg, it) {
        *imgit += *it;
        imgit++;
    }
    for (int i=0; i<NP; i++) {
        Image::iterator imgit = img->begin();
        FOR_EACH_PIXEL(starimgs[i], it) {
            *imgit += *it;
            imgit++;
        }
    }

    // give it roughly correct variance
    //varimg = mimage->getVariance();
    //FOR_EACH_PIXEL(varimg, it)
    //*it = sky;
    *(mimage->getVariance()) = sky;

    // DEBUG - save the image to disk
    img->writeFits("test2.fits");

    // background subtraction...
    afwMath::BackgroundControl bctrl(afwMath::Interpolate::AKIMA_SPLINE);
    bctrl.setNxSample(5);
    bctrl.setNySample(5);
    afwMath::Background bg = afwMath::makeBackground(*img, bctrl);
    Image::Ptr bgimg = bg.getImage<PixelT>();

    (*img) -= (*bgimg);
    img->writeFits("bgsub1.fits");

    // smooth image by PSF and do detection to generate sets of Footprints and Peaks
    //MImage::Ptr convolvedImage = mimage->ImageTypeFactory.type(mimage->getDimensions());
    MImage::Ptr convolvedImage(new MImage(mimage->getDimensions()));
    convolvedImage->setXY0(mimage->getXY0());
    psf->convolve(*convolvedImage->getImage(), 
                  *img,
                  true,
                  convolvedImage->getMask()->getMaskPlane("EDGE"));
    convolvedImage->getImage()->writeFits("conv1.fits");

    afwImage::PointI llc(
                        psf->getKernel()->getWidth()/2, 
                        psf->getKernel()->getHeight()/2
                        );
    afwImage::PointI urc(
                        convolvedImage->getWidth() - 1,
                        convolvedImage->getHeight() - 1
                        );
    urc = urc - llc;
    afwImage::BBox bbox(llc, urc);
    std::printf("Bbox: (x0,y0)=(%i,%i), (x1,y1)=(%i,%i)\n",
                bbox.getX0(), bbox.getY0(),
                bbox.getX1(), bbox.getY1());
    bool deepcopy = false;
    MImage::Ptr middle(new MImage(*convolvedImage, bbox, deepcopy));
    middle->getImage()->writeFits("middle.fits");
    std::printf("saved middle.fits\n");

    float thresholdValue = 10;
    int minPixels = 5;
    int nGrow = int(psfsigma);

    afwDet::Threshold threshold = afwDet::createThreshold(thresholdValue);
    afwDet::FootprintSet<PixelT>::Ptr detections =
        afwDet::makeFootprintSet(*middle, threshold, "DETECTED", minPixels);
    std::cout << "Detections: " << detections->toString() << std::endl;
    std::cout << "Detections: " << detections->getFootprints().size() << std::endl;
    detections = afwDet::makeFootprintSet(*detections, nGrow, false);
    std::cout << "Detections: " << detections->toString() << std::endl;
    std::cout << "Detections: " << detections->getFootprints().size() << std::endl;
    //detections->setMask(mimage->getMask(), "DETECTED");
    std::vector<afwDet::Footprint::Ptr> footprints = detections->getFootprints();
    std::cout << "N Footprints: " << footprints.size() << std::endl;

    std::vector<std::vector<afwDet::Peak::Ptr> > peaks;
    // HACK -- plug in exact Peak locations.
    std::vector<afwDet::Peak::Ptr> im0peaks;
    for (int i=0; i<NP; i++)
        im0peaks.push_back(afwDet::Peak::Ptr(new afwDet::Peak(cx[i], cy[i])));
    peaks.push_back(im0peaks);

    deblender::SDSSDeblender<Image> db;

    std::vector<deblender::DeblendedObject<Image>::Ptr> childList =
        db.deblend(footprints, peaks, mimage, psf);

    BOOST_CHECK_EQUAL(childList.size(), 2U);

    for (size_t i=0; i<childList.size(); i++) {
        std::printf("child %i: %p\n", (int)i, childList[i].get());
        deblender::DeblendedObject<Image>::Ptr child = childList[i];
        for (size_t c=0; c<child->images.size(); c++) {
            Image::Ptr cimg = child->images[c];
            std::printf("child %i, color %i: offset (%i,%i), size %i x %i\n", (int)i, (int)c, child->x0, child->y0, cimg->getWidth(), cimg->getHeight());
            char fn[256];
            sprintf(fn, "test-child%02i-color%02i.fits", (int)i, (int)c);
            //cimg->writeFits(fn);

            Image::Ptr pimg(new Image(W, H, 0));
            child->addToImage(pimg, c);
            std::printf("saving as %s\n", fn);
            pimg->writeFits(fn);
        }
    }

    std::vector<Image::Ptr> imgList;
    imgList.push_back(img);

    // check that the deblended children sum to the original image (flux-preserving)
    for (size_t i=0; i<imgList.size(); i++) {
        Image::Ptr pimg(new Image(W, H, 0));
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
        double noise = skysigma;

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

