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
namespace afwDet = lsst::afw::detection;
//namespace ex    = lsst::pex::exceptions;

template<typename ImageT>
deblender::SDSSDeblender<ImageT>::SDSSDeblender() {
    std::printf("SDSSDeblender() constructor\n");
}


template<typename ImageT>
std::vector<typename deblender::DeblendedObject<ImageT>::Ptr >
deblender::SDSSDeblender<ImageT>::OLDdeblend(std::vector<typename ImageT::Ptr> &images) {

	std::vector<typename DeblendedObject<ImageT>::Ptr > children;

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
             //psf->a = 1.0/(2.0 * M_PI * psfsigma*psfsigma);
             // required by assertion deblend.c:660
             psf->a = 1.0;
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
        int cw, ch;
        int cx0, cy0;
        std::vector<typename ImageT::Ptr> cimgs;
        for (size_t c=0; c<images.size(); c++) {
            photo::OBJMASK* mask = o->aimage->pix[c]->mask;
            cw = mask->cmax - mask->cmin + 1;
            ch = mask->rmax - mask->rmin + 1;
            cx0 = mask->cmin;
            cy0 = mask->rmin;
            std::printf("offset (%i,%i), size (%i,%i)\n", cx0, cy0, cw, ch);
            typename ImageT::Ptr img(new ImageT(cw, ch, 0));
            photo::REGION* reg = photo::shRegNew("", ch, cw, photo::TYPE_U16);
            shRegClear(reg);
            float bg = fp->frame[c].bkgd + softbias;
            photo::phRegionSetFromAtlasImage(o->aimage, c, reg, cx0, cy0, 0, 0, 1);
            for (int j=0; j<ch; j++) {
                photo::U16* row = reg->rows_u16[j];
                for (int k=0; k<cw; k++) {
                    if (row[k])
                        (*img)(k, j) = (float)row[k] - bg;
                    std::printf("%i ", (int)row[k]);
                }
            }
            cimgs.push_back(img);
        }
        typename DeblendedObject<ImageT>::Ptr dbobj(new DeblendedObject<ImageT>());
        dbobj->x0 = cx0;
        dbobj->y0 = cy0;
        dbobj->images = cimgs;
        children.push_back(dbobj);
    }

    std::cout << "Result: " << res << std::endl;

    // uninitialize photo...
    photo::phDeblendUnset();

    delete filters;
	return children;
}


template<typename ImageT>
std::vector<typename deblender::DeblendedObject<ImageT>::Ptr >
deblender::SDSSDeblender<ImageT>::deblend(
    std::vector< afwDet::Footprint::Ptr > footprints,
    std::vector< std::vector< afwDet::Peak::Ptr > > peaks,
    boost::shared_ptr<typename lsst::afw::image::MaskedImage<typename ImageT::Pixel > > the_mimage,
    boost::shared_ptr<typename lsst::meas::algorithms::PSF > the_psf
) {

    // Hmmm, must figure out the right API!
    std::vector<boost::shared_ptr<typename lsst::afw::image::MaskedImage<typename ImageT::Pixel > > > mimages;
    std::vector<boost::shared_ptr<typename lsst::meas::algorithms::PSF > > psfs;
    mimages.push_back(the_mimage);
    psfs.push_back(the_psf);


    std::vector<typename DeblendedObject<ImageT>::Ptr > children;
    std::printf("Hello world!\n");

    int i, j, k;
    // number of images
    int NI;
    char* filters;
    // total number of peaks.
    int NP;

    // Photo's objects:
    // scratch space for photo
    photo::REGION *scr0, *scr1, *scr2, *scr3;
    photo::OBJC* objc;
    photo::FIELDPARAMS* fp;
    photo::PEAKS* phpeaks;

    // must match photo's SOFT_BIAS.
    const int softbias = 1000;

    NI = mimages.size();
    filters = new char[NI+1];
    for (i=0; i<NI; i++)
        filters[i] = 'r';
    filters[NI] = '\0';

    // Photo init.
    photo::initSpanObjmask();
    photo::phInitProfileExtract();
    scr0 = photo::shRegNew("scr0", 3000, 3000, photo::TYPE_U16);
    scr1 = photo::shRegNew("scr1", 3000, 3000, photo::TYPE_U16);
    scr2 = photo::shRegNew("scr2", 3000, 3000, photo::TYPE_U16);
    scr3 = photo::shRegNew("scr3", 3000, 3000, photo::TYPE_U16);
    photo::phDeblendSet(scr0, scr1, scr2, scr3);

    objc = photo::phObjcNew(NI);
    fp = photo::phFieldparamsNew(filters);

    NP = 0;
    for (i=0; i<(int)peaks.size(); i++)
        NP += peaks[i].size();

    std::printf("Deblending: image0 is %i x %i, %i footprints, %i peaks\n",
                mimages[0]->getImage()->getWidth(),
                mimages[0]->getImage()->getHeight(),
                (int)footprints.size(), NP);

    // afwDet::Peak -> photo::PEAK

    // peaks get merged into one flat list...?
    phpeaks = photo::phPeaksNew(NP);
    phpeaks->npeak = NP;

    k = 0;
    for (i=0; i<(int)peaks.size(); i++)
        for (j=0; j<(int)peaks[i].size(); j++) {
            afwDet::Peak::Ptr pk = peaks[i][j];

            photo::PEAK* phpk = photo::phPeakNew();
            phpk->flags |= (PEAK_BAND0 | PEAK_CANONICAL);
            /*
             PEAK_BAND1 | 
             PEAK_BAND2 | 
             PEAK_BAND3 | 
             PEAK_BAND4);
             */
            // HACK -- peak height?
            phpk->peak = 1000;
            phpk->colc = pk->getFx();
            phpk->rowc = pk->getFy();
            // we probably want to round rather than truncate...
            //phpk->cpeak = pk->getIx();
            //phpk->rpeak = pk->getIy();
            phpk->cpeak = static_cast<int>(0.5 + pk->getFx());
            phpk->rpeak = static_cast<int>(0.5 + pk->getFy());
            // FIXME -- {row,col}cErr
            phpk->rowcErr = 0.5;
            phpk->colcErr = 0.5;
            phpeaks->peaks[k] = phpk;
            k++;
        }
    objc->peaks = phpeaks;

    //std::FILE* fid = stdout;

    // afwImage::MaskedImage  -->  photo::FRAMEPARAM, FIELDPARAM

    fp->ref_band_index = 0;
    fp->canonical_band_index = 0;
    // max # of children to deblend
    fp->nchild_max = 1000;

    for (i=0; i<NI; i++) {
        photo::DGPSF* phpsf;
        int W, H;
        typename ImageT::Ptr image = mimages[i]->getImage();

        fp->frame[i].colBinning = 1;
        fp->frame[i].rowBinning = 1;
        fp->frame[i].toGCC = photo::atTransNew();

        // afwImage::Image -->  photo::REGION
        // copy the input image into photo-land...
        W = image->getWidth();
        H = image->getHeight();
        fp->frame[i].data = photo::shRegNew("", H, W, photo::TYPE_U16);
        //std::printf("pixels:\n");
        for (j=0; j<H; j++) {
            photo::U16* row = fp->frame[i].data->rows_u16[j];
            for (k=0; k<W; k++) {
                row[k] = (*image)(k,j) + softbias;
                //std::printf("%i ", (int)row[k]);
            }
        }
        //std::printf("\n");

        // measAlg::PSF  -->  photo::DGPSF

        phpsf = photo::phDgpsfNew();
        //   HACK!!
        // make a single-gaussian.
        double psfsigma = 2.0;
        // in pixels -- matching Deblender.cc:"s" and also fake.c:psfsigma
        phpsf->width = 0.396 * 2.35 * psfsigma; // FWHM in arcsec.
        // HACK!!!
        phpsf->sigmax1 = psfsigma;
        phpsf->sigmay1 = psfsigma;
        phpsf->sigmax2 = psfsigma;
        phpsf->sigmay2 = psfsigma;
        // required by assertion deblend.c:660
        phpsf->a = 1.0;

        phpsf->b = 0.0;
        phpsf->p0 = 0.0;
        phpsf->chisq = 1.0;
        fp->frame[i].psf = phpsf;

        // required by improve_template
        fp->frame[i].smooth_sigma = psfsigma;

        /* the detection threshold in the smoothed image is ffo_threshold, and a
         * star of peak intensity I0 would have a peak of I0/2 in the smoothed
         * image, hence the value of I0_min
         */
        // HACK
        fp->frame[i].ffo_threshold = 10.0;
        // --> I0_min[i] = ffo_threshold * 2 (deblend.c:590)

        //std::printf("  set psf.width to %g (arcsec FWHM)\n", fp->frame[i].psf->width);

        // HACK!
        fp->frame[i].dark_variance = photo::phBinregionNewFromConst(1.0, 1, 1, 1, 1, MAX_U16);

        // Estimate background = median pixel.
        float med;
        {
            // just dump pix into an stl vector
            std::vector<float> vpix;
            for (int j=0; j<H; j++)
                for (int k=0; k<W; k++)
                    vpix.push_back((*image)(k,j));
            // partition
            nth_element(vpix.begin(), vpix.begin() + vpix.size()/2, vpix.end());
            med = *(vpix.begin() + vpix.size()/2);
            std::printf("Estimating median of image %i = %g\n", i, med);
        }
        // Used in deblend.c : setup_normal
        fp->frame[i].bkgd = med;
    }
    
    // from fpParam.par:
    // minimum number of detections to create a child
    fp->deblend_min_detect = 1;
    fp->deblend_min_peak_spacing = 2;
    fp->deblend_psf_Lmin = 0.2; // ???!
    fp->deblend_psf_nann = 3;
    fp->deblend_psf_rad_max = 12;
    fp->deblend_npix_max = 0;
    //deblend_allowed_unassigned 0.05	# max permitted fraction of unassigned flux
    // deblend.c:1299: max cosine between templates.
    fp->deblend_inner_max	= 0.5;


    // afwDet::Footprint -> photo::OBJECT1

    for (i=0; i<NI; i++) {
        photo::OBJECT1* o1 = photo::phObject1New();
        image::BBox bbox = footprints[i]->getBBox();
        afwDet::Footprint::SpanList spans;

        //o1->region = photo::shRegNew("", H, W, photo::TYPE_U16);

        // Footprint bounding-box --> REGION
        o1->region = photo::shSubRegNew("", fp->frame[i].data,
                                        bbox.getHeight(), bbox.getWidth(),
                                        bbox.getY0(), bbox.getX0(),
                                        photo::READ_ONLY);
        // MUST be a SPANMASK*
        o1->region->mask = (photo::MASK*)photo::phSpanmaskNew(bbox.getHeight(), bbox.getWidth());

        std::printf("Footprint %i: offset (%i, %i), size %i x %i  (max %i, %i)\n",
                    i, bbox.getX0(), bbox.getY0(), bbox.getWidth(), bbox.getHeight(),
                    bbox.getX0() + bbox.getWidth(), bbox.getY0() + bbox.getHeight());

        // Footprint spans --> OBJMASK
        spans = footprints[i]->getSpans();
        o1->mask = photo::phObjmaskNew(spans.size());
        for (j=0; j<(int)spans.size(); j++) {
            photo::SPAN* s = o1->mask->s + j;
            s->y  = spans[j]->getY ();
            s->x1 = spans[j]->getX0();
            s->x2 = spans[j]->getX1();
            //std::printf("footprint %i, span %i: y %i, x [%i, %i]\n", i, j, s->y, s->x1, s->x2);
        }
        o1->mask->nspan = spans.size();
        photo::phObjmaskBBSet(o1->mask);

        std::printf(" o1->mask has limits (%i,%i) to (%i,%i); col0,row0 = (%i, %i)\n",
                    o1->mask->cmin, o1->mask->rmin, o1->mask->cmax, o1->mask->rmax, o1->mask->col0, o1->mask->row0);

        // HACK
        o1->sky = 100;
        /*
         o1->rowc = H/2;
         o1->colc = W/2;
         o1->rowcErr = 0.5;
         o1->colcErr = 0.5;
         */

        objc->color[i] = o1;
    }
    /*
     objc->rowc = H/2;
     objc->colc = W/2;
     objc->rowcErr = 0.5;
     objc->colcErr = 0.5;
     */

    // ATLAS_IMAGE ?????
    objc->aimage = photo::phAtlasImageNewFromObjc(objc);            
    //objc->aimage->master_mask = photo::phObjmaskFromRect(0, 0, W, H);
    photo::OBJMASK* mm = objc->aimage->master_mask;
    assert(mm == NULL);
    for (i=0; i<NI; i++) {
        photo::OBJECT1* o1 = objc->color[i];
        if (i == 0)
            mm = photo::phObjmaskCopy(o1->mask, 0, 0);
        else
            mm = photo::phObjmaskUnion(mm, o1->mask);
    }
    // ?
    photo::phObjmaskBBSet(mm);
    std::printf("Set ATLAS_IMAGE master_mask to have limits (%i,%i) to (%i,%i); col0,row0 = (%i, %i)\n",
                mm->cmin, mm->rmin, mm->cmax, mm->rmax, mm->col0, mm->row0);
    objc->aimage->master_mask = mm;

    for (i=0; i<NI; i++) {
        photo::phAtlasImageSetFromRegion(objc->aimage, i, fp->frame[i].data);
    }
    photo::printObjc(objc);
    /*
     std::printf("fp->ncolor %i\n", fp->ncolor);
     std::printf("  filters %s\n", fp->filters);
     std::printf("  ref_band_index %i\n", fp->ref_band_index);
     std::printf("  canonical_band_index %i\n", fp->canonical_band_index);
     std::cout << "phObjcMakeChildrenFAKE..." << std::endl;
     */
    photo::phObjcMakeChildrenFake(objc, fp);

    /*
     std::printf("after phObjcMakeChildren:\n");
     photo::printObjc(objc);
     std::printf("Objc to be deblended:\n");
     photo::phObjcPrintPretty(objc, "");
     */
	int res = photo::phObjcDeblend(objc, fp);

    std::cout << "After phObjcDeblend:" << std::endl;
    photo::phObjcPrintPretty(objc, "");
    photo::printObjc(objc);

    // OBJC* family --> vector of DeblendedObject
    for (photo::OBJC* o = photo::phObjcDescendentNext(objc);
         o;
         o = phObjcDescendentNext((const photo::OBJC*)NULL)) {
        if (o->children)
            continue;
        int cw, ch;
        int cx0, cy0;
        std::vector<typename ImageT::Ptr> cimgs;
        for (i=0; i<NI; i++) {
            photo::OBJMASK* mask = o->aimage->pix[i]->mask;
            cw = mask->cmax - mask->cmin + 1;
            ch = mask->rmax - mask->rmin + 1;
            cx0 = mask->cmin;
            cy0 = mask->rmin;
            std::printf("offset (%i,%i), size (%i,%i)\n", cx0, cy0, cw, ch);

            // FIXME -- OBJMASK --> Footprint, rather than REGION --> Image

            typename ImageT::Ptr img(new ImageT(cw, ch, 0));
            photo::REGION* reg = photo::shRegNew("", ch, cw, photo::TYPE_U16);
            shRegClear(reg);
            float bg = fp->frame[i].bkgd + softbias;
            photo::phRegionSetFromAtlasImage(o->aimage, i, reg, cy0, cx0, 0, 0, 1);
            for (int j=0; j<ch; j++) {
                photo::U16* row = reg->rows_u16[j];
                for (int k=0; k<cw; k++) {
                    if (row[k])
                        (*img)(k, j) = (float)row[k] - bg;
                    //std::printf("%i ", (int)row[k]);
                }
            }
            cimgs.push_back(img);
        }
        typename DeblendedObject<ImageT>::Ptr dbobj(new DeblendedObject<ImageT>());
        dbobj->x0 = cx0;
        dbobj->y0 = cy0;
        dbobj->images = cimgs;
        children.push_back(dbobj);
    }

    std::cout << "Result: " << res << std::endl;

    // uninitialize photo...
    photo::phDeblendUnset();
    // FIXME -- more cleanup...!

    delete filters;
	return children;
}



template<typename ImageT>
void
deblender::DeblendedObject<ImageT>::addToImage(typename ImageT::Ptr image, int color) {
    assert(color >= 0);
    assert(color < (int)this->images.size());
    assert(this->x0 >= 0);
    assert(this->y0 >= 0);
    typename ImageT::Ptr myimg = this->images[color];
    assert(image->getWidth() >= this->x0 + myimg->getWidth());
    assert(image->getHeight() >= this->y0 + myimg->getHeight());
    for (int j=0; j<myimg->getHeight(); j++) {
        for (int i=0; i<myimg->getWidth(); i++) {
            (*image)(this->x0 + i, this->y0 + j) += (*myimg)(i, j);
        }
    }
}


/*
 * Explicit Instantiations
 *
 */
template class deblender::SDSSDeblender<image::Image<double> >;
template class deblender::SDSSDeblender<image::Image<float > >;

template class deblender::DeblendedObject<image::Image<double> >;
template class deblender::DeblendedObject<image::Image<float > >;
