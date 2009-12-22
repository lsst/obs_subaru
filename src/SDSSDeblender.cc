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
#include "phObjc_p.h"

int
phObjcMakeChildrenFake(OBJC *objc,		/* give this OBJC a family */
					   const FIELDPARAMS *fiparams) /* misc. parameters */
    ;
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

    // Photo init.
    photo::initSpanObjmask();

    photo::OBJC* objc = photo::phObjcNew(images.size());
    photo::FIELDPARAMS* fp = photo::phFieldparamsNew(filters);

    std::printf("filters: %s\n", filters);

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
            /*
             pk->rowc = ...;
             pk->colc = ...;
             pk->rowcErr = ...;
             pk->colcErr = ...;
             */
            peaks->peaks[i] = pk;
            std::printf("Set peak %i to %p\n", i, peaks->peaks[i]);
        }
    objc->peaks = peaks;

         std::FILE* fid = stdout;

         int W, H;
         
     for (size_t i=0; i<images.size(); i++) {
            W = images[i]->getWidth();
            H = images[i]->getHeight();
            photo::OBJECT1* o1 = photo::phObject1New();
            std::printf("Object1 %i:\n", (int)i);
            photo::phObject1PrintPretty(o1, fid);
            // OBJMASK* (arg: number of spans)
            //o1->mask = phObjmaskNew(1);
            o1->mask = photo::phObjmaskFromRect(0, 0, W, H);
            // "makeRegion" can't handle TYPE_FL64
            // photo in general can't handle anything other than TYPE_U16.
            //o1->region = photo::shRegNew("", H, W, photo::TYPE_FL32);
            o1->region = photo::shRegNew("", H, W, photo::TYPE_U16);
            //o1->region->mask = photo::shMaskNew("", H, W);
            // MUST be a SPANMASK*
            o1->region->mask = (photo::MASK*)photo::phSpanmaskNew(H, W);
            printf("Default flags: 0x%x\n", o1->flags);
            objc->color[i] = o1;
        }

              objc->aimage = photo::phAtlasImageNewFromObjc(objc);            
              //objc->aimage = photo::phAtlasImageNew(images.size());
              objc->aimage->master_mask = photo::phObjmaskFromRect(0, 0, W, H);
          objc->aimage->pix[0] = photo::phAiPixNew(objc->aimage->master_mask, 1, 0);

    printObjc(objc);


         fp->ref_band_index = 0;
         fp->canonical_band_index = 0;
          // max # of children to deblend
          fp->nchild_max = 1000;
         for (size_t i=0; i<images.size(); i++) {
             fp->frame[i].colBinning = 1;
             fp->frame[i].rowBinning = 1;
             fp->frame[i].toGCC = photo::atTransNew();
             // copy the input images into photo-land...
             //fp->frame[i].data = shRegNew("", H, W, photo::TYPE_FL32);
             fp->frame[i].data = shRegNew("", H, W, photo::TYPE_U16);
             for (int j=0; j<H; j++) {
                 //photo::FL32* row = fp->frame[i].data->rows_fl32[j];
                 photo::U16* row = fp->frame[i].data->rows_u16[j];
                 for (int k=0; k<W; k++) {
                     row[k] = (*images[i])(k,j);
                 }
             }
             fp->frame[i].psf = photo::phDgpsfNew();
         }
         fp->deblend_psf_Lmin = 0;
         

         std::printf("fp->ncolor %i\n", fp->ncolor);
         std::printf("  filters %s\n", fp->filters);
         std::printf("  ref_band_index %i\n", fp->ref_band_index);
         std::printf("  canonical_band_index %i\n", fp->canonical_band_index);
          
          /*
           std::cout << "phObjcMakeChildren..." << std::endl;
           photo::phObjcMakeChildren(objc, fp);
           */
          std::cout << "phObjcMakeChildrenFAKE..." << std::endl;
          photo::phObjcMakeChildrenFake(objc, fp);

    /*
     for(i = 0; i < peaks->npeak; i++) {
     photo::phObjcChildNew(objc, peaks->peaks[i], fp, 1);
     photo::phObjcChildNew(objc, peaks->peaks[i], fp, 1);
     }
     */
          std::printf("after phObjcMakeChildren:\n");
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
