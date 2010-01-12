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
#include "phFake.h"
#include "phDgpsf.h"
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

    char* filters = new char[images.size()+1];
    int i;
    for (size_t i=0; i<images.size(); i++)
        filters[i] = 'r';
    filters[images.size()] = '\0';

    // Photo init.
    photo::initSpanObjmask();
    photo::phInitProfileExtract();
    photo::REGION *scr0, *scr1, *scr2, *scr3;
    scr0 = photo::shRegNew("scr0", 3000, 3000, photo::TYPE_U16);
    scr1 = photo::shRegNew("scr1", 3000, 3000, photo::TYPE_U16);
    scr2 = photo::shRegNew("scr2", 3000, 3000, photo::TYPE_U16);
    scr3 = photo::shRegNew("scr3", 3000, 3000, photo::TYPE_U16);
    photo::phDeblendSet(scr0, scr1, scr2, scr3);

    photo::OBJC* objc = photo::phObjcNew(images.size());
    photo::FIELDPARAMS* fp = photo::phFieldparamsNew(filters);

    std::printf("filters: %s\n", filters);

    // Find peaks in the image...
    // requires:
    //  --image noise estimate
    //  --background-subtraction/sky estimate
    //  --PSF model

    int W, H;

    const int softbias = 1000;

         W = images[0]->getWidth();
         H = images[0]->getHeight();
    
    // "detect" two peaks
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
            pk->peak = 1000;
            if (i == 0) {
                //pk->colc = 28;
                pk->colc = 26;
                pk->rowc = 32;
                pk->cpeak = pk->colc;
                pk->rpeak = pk->rowc;
            } else {
                //pk->colc = 38;
                pk->colc = 40;
                pk->rowc = 32;
                pk->cpeak = pk->colc;
                pk->rpeak = pk->rowc;
            }
            pk->rowcErr = 0.5;
            pk->colcErr = 0.5;
            peaks->peaks[i] = pk;
            std::printf("Set peak %i to %p\n", i, peaks->peaks[i]);
        }
    objc->peaks = peaks;

         std::FILE* fid = stdout;

     for (size_t i=0; i<images.size(); i++) {
         photo::OBJECT1* o1 = photo::phObject1New();
            std::printf("Object1 %i:\n", (int)i);
            photo::phObject1PrintPretty(o1, fid);
            o1->mask = photo::phObjmaskFromRect(0, 0, W, H);
            // "makeRegion" can't handle TYPE_FL64
            // photo in general can't handle anything other than TYPE_U16.
            o1->region = photo::shRegNew("", H, W, photo::TYPE_U16);
            // MUST be a SPANMASK*
            o1->region->mask = (photo::MASK*)photo::phSpanmaskNew(H, W);
            printf("Default flags: 0x%x\n", o1->flags);

            // HACK
            o1->rowc = H/2;
            o1->colc = W/2;
            o1->rowcErr = 0.5;
            o1->colcErr = 0.5;
            o1->sky = 100;

            objc->color[i] = o1;
        }
              objc->rowc = H/2;
          objc->colc = W/2;
          objc->rowcErr = 0.5;
          objc->colcErr = 0.5;
          

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
             fp->frame[i].data = photo::shRegNew("", H, W, photo::TYPE_U16);
             std::printf("pixels:\n");
             for (int j=0; j<H; j++) {
                 //photo::FL32* row = fp->frame[i].data->rows_fl32[j];
                 photo::U16* row = fp->frame[i].data->rows_u16[j];
                 for (int k=0; k<W; k++) {
                     row[k] = (*images[i])(k,j) + softbias;
                     std::printf("%i ", (int)row[k]);
                 }
             }
             std::printf("\n");
             photo::DGPSF* psf = photo::phDgpsfNew();
             // make a single-gaussian.
             double psfsigma = 3.0; // pixels -- matching Deblender.cc:"s" and 
             // also fake.c:psfsigma

             // HACK...
             psf->width = 0.396 * 2.35 * psfsigma; // FWHM in arcsec.
             psf->a = 1.0/(2.0 * M_PI * psfsigma*psfsigma);
             psf->sigmax1 = 2.0;
             psf->sigmay1 = 2.0;
             psf->sigmax2 = 2.0;
             psf->sigmay2 = 2.0;
             psf->b = 0.0;
             psf->p0 = 0.0;
             psf->chisq = 1.0;
             fp->frame[i].psf = psf;

             /* the detection
              * threshold in the smoothed image is ffo_threshold, and a star of peak
              * intensity I0 would have a peak of I0/2 in the smoothed image, hence the
              * value of I0_min
              */
             fp->frame[i].ffo_threshold = 10.0;

             std::printf("  set psf.width to %g (arcsec FWHM)\n", fp->frame[i].psf->width);

             fp->frame[i].dark_variance = photo::phBinregionNewFromConst(1.0, 1, 1, 1, 1, MAX_U16);


             // Estimate background = median pixel.
             // just dump pix into an stl vector
             std::vector<float> vpix;
             for (int j=0; j<H; j++)
                 for (int k=0; k<W; k++)
                     vpix.push_back((*images[i])(k,j));
             // partition
             nth_element(vpix.begin(), vpix.begin() + vpix.size()/2, vpix.end());
             float med = *(vpix.begin() + vpix.size()/2);
             std::printf("Estimating median = %g\n", med);
             // Used in deblend.c : setup_normal
             fp->frame[i].bkgd = med;


         }
              /* from fpParam.par: */
              fp->deblend_min_peak_spacing = 2;
              fp->deblend_psf_Lmin = 2; // ???!
          fp->deblend_psf_nann = 3;
          fp->deblend_psf_rad_max = 12;
          fp->deblend_npix_max = 0;
          // deblend.c:1299: max cosine between templates.
          fp->deblend_inner_max	= 0.5;

          // Set atlas image *after* copying image data to fp. (?)
          objc->aimage = photo::phAtlasImageNewFromObjc(objc);            
          objc->aimage->master_mask = photo::phObjmaskFromRect(0, 0, W, H);
          // ??
          //objc->aimage->pix[0] = photo::phAiPixNew(objc->aimage->master_mask, 1, 0);
          //photo::phAtlasImageCut(objc, -1, fp, 500, 0, NULL);
          for (size_t i=0; i<images.size(); i++)
              photo::phAtlasImageSetFromRegion(objc->aimage, i, fp->frame[i].data);

          photo::printObjc(objc);

         

          std::printf("fp->ncolor %i\n", fp->ncolor);
          std::printf("  filters %s\n", fp->filters);
          std::printf("  ref_band_index %i\n", fp->ref_band_index);
          std::printf("  canonical_band_index %i\n", fp->canonical_band_index);
          
          std::cout << "phObjcMakeChildrenFAKE..." << std::endl;
          photo::phObjcMakeChildrenFake(objc, fp);

          std::printf("after phObjcMakeChildren:\n");
          photo::printObjc(objc);

    std::cout << "Objc to be deblended:" << std::endl;
    photo::phObjcPrintPretty(objc, "");

	int res = photo::phObjcDeblend(objc, fp);

    std::cout << "After phObjcDeblend:" << std::endl;
    photo::phObjcPrintPretty(objc, "");
    photo::printObjc(objc);

    
    for (photo::OBJC* o = photo::phObjcDescendentNext(objc);
         o;
         o = phObjcDescendentNext((const photo::OBJC*)NULL)) {
        if (o->children)
            continue;
        typename ImageT::Ptr img = typename ImageT::Ptr(new ImageT(W, H, 0)); //ImageT(W, H, 0));
        photo::REGION* reg = photo::shRegNew("", H, W, photo::TYPE_U16);
        shRegClear(reg);
        // HACK -- return 2d vec of obj,color
        int c = 0;
        float bg = fp->frame[c].bkgd + softbias;
        photo::phRegionSetFromAtlasImage(o->aimage, c, reg, 0, 0, 0, 0, 1);
        for (int j=0; j<H; j++) {
            photo::U16* row = reg->rows_u16[j];
            for (int k=0; k<W; k++) {
                if (row[k])
                    (*img)(k, j) = (float)row[k] - bg;
                std::printf("%i ", (int)row[k]);
            }
        }
        children.push_back(img);
    }

    std::cout << "Result: " << res << std::endl;

    // uninitialize photo...
    photo::phDeblendUnset();

    delete filters;
	return children;
}


/*
 * Explicit Instantiations
 *
 */
template class deblender::SDSSDeblender<image::Image<double> >;
template class deblender::SDSSDeblender<image::Image<float > >;
