// -*- lsst-c++ -*-
/**
 * @author Dustin Lang
 */
#include <iostream>

#include <sys/param.h>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Deblender

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

//#include "lsst/afw/image/Image.h"
//#include "lsst/meas/algorithms/PSF.h"
//#include "lsst/afw/math/Background.h"

#include "lsst/afw/image.h"
#include "lsst/afw/math.h"
#include "lsst/afw/detection.h"
#include "lsst/afw/geom.h"
#include "lsst/afw/geom/Box.h"
#include "lsst/meas/algorithms.h"

#include "lsst/meas/deblender/deblender.h"

namespace afwImage = lsst::afw::image;
namespace afwDet = lsst::afw::detection;
namespace afwGeom = lsst::afw::geom;
namespace measAlg = lsst::meas::algorithms;
namespace afwMath = lsst::afw::math;
namespace deblender = lsst::meas::deblender;

typedef float PixelT;
typedef afwImage::Image<PixelT> Image;
typedef afwImage::Image<double> ImageD;
typedef afwImage::MaskedImage<PixelT> MImage;
typedef afwDet::Psf PSF;
typedef afwMath::Kernel Kernel;

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

#define FOR_EACH_PIXEL_OFTYPE(img, it, T)                                 \
    for (T::iterator it = img->begin(), end = img->end(); it != end; it++)

//template afwImage::Image<PixelT>::Ptr afwMath::Background::getImage() const;

BOOST_AUTO_TEST_CASE(TwoStarDeblend) {
    int const W = 64;
    int const H = 64;

    float cx[] = { 28.4, 38.1 };
    float cy[] = { 32.8, 32.5 };
    float flux[] = {2000, 4000};
    // single-Gaussian PSF sigma
    float truepsfsigma = 2;
    // sky level
    double sky = 100;
    // sky noise level, gaussian approximation
    double skysigma = sqrt(sky);
    // number of peaks
    int NP = sizeof(cx)/sizeof(float);

    PSF::Ptr truepsf = afwDet::createPsf("DoubleGaussian", 10, 10, truepsfsigma, 0, 0.0);
    PSF::Ptr psf = truepsf;
    float psfsigma = truepsfsigma;

    // generate sky + noise.
    Image::Ptr skyimg(new Image(afwGeom::Extent2I(W, H)));
    // for (Image::iterator it = skyimg->begin(), end = skyimg->end(); it != end; it++)
    FOR_EACH_PIXEL(skyimg, it)
        // ~ Poisson
        *it = sky + random_gaussian() * skysigma;

    // generate star images (PSFs)
    std::vector<Image::Ptr> starimgs;
    for (int i=0; i<NP; i++) {
        Image::Ptr starimg(new Image(afwGeom::Extent2I(W, H)));

        ImageD::Ptr psfd = psf->computeImage(afwGeom::Point2D(cx[i],cy[i]));
        std::cout << "PSF image size is " << psfd->getWidth() << " x " << psfd->getHeight() << std::endl;
        std::cout << "  origin " << psfd->getX0() << ", " << psfd->getY0() << std::endl;

        /*
         char fn[64];
         sprintf(fn, "psf-%02i.fits", i);
         std::string sfn(fn);
         psfd->writeFits(sfn);
         std::cout << "wrote to " << sfn << std::endl;
         */

        int x0, y0;
        x0 = psfd->getX0();
        y0 = psfd->getY0();
        for (int y=0; y<psfd->getHeight(); y++) {
            for (int x=0; x<psfd->getWidth(); x++) {
                (*starimg)(x+x0, y+y0) += (*psfd)(x,y);
            }
        }

        // normalize and scale by flux -- wrong if source is close to the edge.
        double imgsum = 0;
        for (Image::iterator it = starimg->begin(), end = starimg->end(); it != end; it++)
            imgsum += *it;
        std::cout << "Image sum: " << imgsum << std::endl;
        for (Image::iterator it = starimg->begin(), end = starimg->end(); it != end; it++)
            *it *= flux[i] / imgsum;
        starimgs.push_back(starimg);
    }

    // sum the sky and star images
    MImage::Ptr mimage(new MImage(afwGeom::Extent2I(W, H)));
    Image::Ptr img = mimage->getImage();
    Image::iterator imgit = img->begin();
    FOR_EACH_PIXEL(skyimg, it) {
        *imgit += *it;
        imgit++;
    }
    for (int i=0; i<NP; i++) {
        imgit = img->begin();
        FOR_EACH_PIXEL(starimgs[i], it) {
            *imgit += *it;
            imgit++;
        }
    }
    // DEBUG - save the image to disk
    img->writeFits("test2.fits");

    // give it roughly correct variance
    //*(mimage->getVariance()) = sky;
    imgit = img->begin();
    FOR_EACH_PIXEL_OFTYPE(mimage->getVariance(), it, MImage::Variance) {
        *it = MAX(skysigma, sqrt(*imgit));
    }

    // make a deep copy for further processing...
    MImage::Ptr mimgcopy(new MImage(*mimage, true));
    // background subtraction...
    afwMath::BackgroundControl bctrl(afwMath::Interpolate::AKIMA_SPLINE);
    bctrl.setNxSample(5);
    bctrl.setNySample(5);
    afwMath::Background bg = afwMath::makeBackground(*mimgcopy, bctrl);
    Image::Ptr bgimg = bg.getImage<PixelT>();

    (*mimgcopy) -= (*bgimg);
    mimgcopy->writeFits("bgsub1.fits");

    // smooth image by PSF and do detection to generate sets of Footprints and Peaks
    MImage::Ptr convolvedImage(new MImage(mimage->getDimensions()));
    convolvedImage->setXY0(mimage->getXY0());

    Kernel::Ptr kernel = psf->getKernel();
    afwMath::convolve(*convolvedImage, //->getImage(), 
                      *mimgcopy,
                      *kernel);
    //true,
    //convolvedImage->getMask()->getMaskPlane("EDGE"));
    convolvedImage->getImage()->writeFits("conv1.fits");

    int psfw = psf->getKernel()->getWidth();
    int psfh = psf->getKernel()->getHeight();
    afwGeom::PointI llc(psfw/2, psfh/2);
    afwGeom::PointI urc(convolvedImage->getWidth()  - 1 - psfw/2,
                        convolvedImage->getHeight() - 1 - psfh/2);
    afwGeom::Box2I bbox(llc, urc);
    std::printf("Bbox: (x0,y0)=(%i,%i), (x1,y1)=(%i,%i)\n",
                bbox.getMinX(), bbox.getMinY(),
                bbox.getMaxX(), bbox.getMaxY());
    MImage::Ptr middle(new MImage(*convolvedImage, bbox, afwImage::PARENT));
    middle->getImage()->writeFits("middle.fits");
    std::printf("saved middle.fits\n");

    float thresholdValue = 10;
    int minPixels = 5;
    int nGrow = int(psfsigma);

    afwDet::Threshold threshold = afwDet::createThreshold(thresholdValue);
    afwDet::FootprintSet<PixelT>::Ptr detections =
        afwDet::makeFootprintSet(*middle, threshold, "DETECTED", minPixels);
    std::cout << "Detections: " << detections->toString() << std::endl;
    std::cout << "Detections: " << detections->getFootprints()->size() << std::endl;
    detections = afwDet::makeFootprintSet(*detections, nGrow, false);
    std::cout << "Detections: " << detections->toString() << std::endl;
    std::cout << "Detections: " << detections->getFootprints()->size() << std::endl;
    //detections->setMask(mimage->getMask(), "DETECTED");
    std::vector<afwDet::Footprint::Ptr> footprints = *(detections->getFootprints());
    std::cout << "N Footprints: " << footprints.size() << std::endl;
    std::cout << "Footprint bboxes: " << std::endl;
    for (size_t i=0; i<footprints.size(); i++) {
        afwGeom::Box2I bb = footprints[i]->getBBox();
        std::printf("  (%i,%i) to (%i,%i)\n", bb.getMinX(), bb.getMinY(), bb.getMaxX(), bb.getMaxY());
    }


    std::vector<std::vector<afwDet::Peak::Ptr> > peaks;
    // HACK -- plug in exact Peak locations.
    std::vector<afwDet::Peak::Ptr> im0peaks;
    for (int i=0; i<NP; i++)
        im0peaks.push_back(afwDet::Peak::Ptr(new afwDet::Peak(cx[i], cy[i])));
    peaks.push_back(im0peaks);

    deblender::SDSSDeblender<Image> db;

    mimage->getImage()->writeFits("input.fits");

    std::vector<deblender::DeblendedObject<Image>::Ptr> childList =
        db.deblend(footprints, peaks, mimage, psf);

    BOOST_CHECK_EQUAL(childList.size(), 2U);

    for (size_t i=0; i<childList.size(); i++) {
        std::printf("child %i: %p\n", (int)i, childList[i].get());
        deblender::DeblendedObject<Image>::Ptr child = childList[i];
        for (size_t c=0; c<child->images.size(); c++) {
            Image::Ptr cimg = child->images[c];
            std::printf("child %i, color %i: offset x,y = (%i,%i), size %i x %i\n", (int)i, (int)c, child->x0, child->y0, cimg->getWidth(), cimg->getHeight());
            char fn[256];
            sprintf(fn, "test-child%02i-color%02i.fits", (int)i, (int)c);
            Image::Ptr pimg(new Image(afwGeom::Extent2I(W, H)));
            child->addToImage(pimg, c);
            std::printf("saving as %s\n", fn);
            pimg->writeFits(fn);
        }
    }

    std::vector<Image::Ptr> imgList;
    imgList.push_back(img);

    // check that the deblended children (approximately) sum to the original image (flux-preserving)
    for (size_t i=0; i<imgList.size(); i++) {
        Image::Ptr pimg(new Image(afwGeom::Extent2I(W, H)));
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

        Image::Ptr diffimg(new Image(afwGeom::Extent2I(W, H)));
        *diffimg += *pimg;
        *diffimg += *bgimg;
        *diffimg -= *imgList[i];

        Image::Ptr diffgood(new Image(*diffimg,
                                      afwGeom::Box2I(afwGeom::Point2I(mxlo, mylo),
                                                     afwGeom::Point2I(W-mxhi-mxlo, H-myhi-mylo)),
                                      afwImage::PARENT));

        int checkedpix = 0;

        double dlo =  1e100;
        double dhi = -1e100;
        double dmean = 0;
        double dvar = 0;
        FOR_EACH_PIXEL(diffgood, it) {
            dlo = MIN(dlo, *it);
            dhi = MAX(dhi, *it);
            dmean += *it;
            checkedpix++;
        }
        dmean /= checkedpix;
        FOR_EACH_PIXEL(diffgood, it) {
            dvar += (*it - dmean)*(*it - dmean);
        }
        dvar /= checkedpix;
        std::printf("differences: mean %g, variance %g, range [%g, %g]\n", dmean, dvar, dlo, dhi);

        diffgood->writeFits("diffgood.fits");

        // variance within 10% of skysigma**2.
        BOOST_CHECK_CLOSE(dvar, skysigma*skysigma, 10.);
        // mean within 0.1*skysigma of zero.
        BOOST_CHECK(fabs(dmean) < skysigma*0.1);
        // extrema within 5-sigma of zero.
        BOOST_CHECK(dlo > -5*skysigma);
        BOOST_CHECK(dhi <  5*skysigma);

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

