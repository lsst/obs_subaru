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
#include "phPeaks.h"
#include "phObjc.h"
#include "phSpanUtil.h"
#include "region.h"
}
}}}}

namespace image = lsst::afw::image;
namespace deblender = lsst::meas::deblender;
namespace photo = lsst::meas::deblender::photo;
//namespace ex    = lsst::pex::exceptions;


static void printObjc(photo::OBJC* o) {
    std::printf("Objc:\n  parent %p\n  children %p\n  sibbs %p\n",
                o->parent, o->children, o->sibbs);
    std::printf("  ncolor %i\n", o->ncolor);
}

template<typename ImageT>
std::vector<typename ImageT::Ptr>
deblender::SDSSDeblender<ImageT>::deblend(std::vector<typename ImageT::Ptr> &images) {
	std::vector<typename ImageT::Ptr> children;

    char* filters = new char[images.size()+1];
    int i;
    for (size_t i=0; i<images.size(); i++)
        filters[i] = 'r';
    filters[images.size()] = '\0';

    photo::OBJC* objc = photo::phObjcNew(images.size());
    photo::FIELDPARAMS* fp = photo::phFieldparamsNew(filters);

    // Find peaks in the image...
    // requires:
    //  --image noise estimate
    //  --background-subtraction/sky estimate
    //  --PSF model

    // "detect" two peaks in each image.
    photo::PEAKS* peaks = photo::phPeaksNew(2);
    peaks->npeak = 2;
    for (i=0; i<peaks->npeak; i++) {
            photo::PEAK* pk = photo::phPeakNew();
            pk->flags |= (PEAK_BAND0 | PEAK_CANONICAL);
            /*
             PEAK_BAND1 | 
             PEAK_BAND2 | 
             PEAK_BAND3 | 
             PEAK_BAND4);
             */
            peaks->peaks[i] = pk;
            std::printf("Set peak %i to %p\n", i, peaks->peaks[i]);
        }
    objc->peaks = peaks;

         std::FILE* fid = stdout;

    for (size_t i=0; i<images.size(); i++) {
            int W = images[i]->getWidth();
            int H = images[i]->getHeight();
            photo::OBJECT1* o1 = photo::phObject1New();
            std::printf("Object1 %i:\n", (int)i);
            photo::phObject1PrintPretty(o1, fid);
            // OBJMASK* (arg: number of spans)
            //o1->mask = phObjmaskNew(1);
            o1->mask = photo::phObjmaskFromRect(0, 0, W, H);
            // "makeRegion" can't handle TYPE_FL64
            o1->region = photo::shRegNew("", H, W, photo::TYPE_FL32);
            //o1->region->mask = photo::shMaskNew("", H, W);
            // MUST be a SPANMASK*
            o1->region->mask = (photo::MASK*)photo::phSpanmaskNew(H, W);
            objc->color[i] = o1;
        }

    printObjc(objc);


    std::cout << "phObjcMakeChildren..." << std::endl;
    photo::phObjcMakeChildren(objc, fp);

    /*
     for(i = 0; i < peaks->npeak; i++) {
     photo::phObjcChildNew(objc, peaks->peaks[i], fp, 1);
     photo::phObjcChildNew(objc, peaks->peaks[i], fp, 1);
     }
     */
    printObjc(objc);

    std::cout << "Objc to be deblended:" << std::endl;
    photo::phObjcPrintPretty(objc, "");

	int res = photo::phObjcDeblend(objc, fp);

    std::cout << "Result: " << res << std::endl;

    delete filters;
	return children;
}


/*
 * Explicit Instantiations
 *
 */
template class deblender::SDSSDeblender<image::Image<double> >;
template class deblender::SDSSDeblender<image::Image<float > >;
