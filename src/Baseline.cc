#include <list>

#include "lsst/meas/deblender/Baseline.h"
#include "lsst/pex/logging.h"
#include "lsst/afw/geom/Box.h"
#include "lsst/meas/algorithms/detail/SdssShape.h"

#include <boost/math/special_functions/round.hpp>

using boost::math::iround;

namespace image = lsst::afw::image;
namespace det = lsst::afw::detection;
namespace deblend = lsst::meas::deblender;
namespace geom = lsst::afw::geom;
namespace malg = lsst::meas::algorithms;
namespace pexLog = lsst::pex::logging;


static bool span_ptr_compare(PTR(det::Span) sp1, PTR(det::Span) sp2) {
    return (*sp1 < *sp2);
}

/**
 Run a spatial median filter over the given input *img*, writing the
 results to *out*.  *halfsize* is half the box size of the filter; ie,
 a halfsize of 50 means that each output pixel will be the median of
 the pixels in a 101 x 101-pixel box in the input image.

 Margins are handled horribly: the median is computed only for pixels
 more than *halfsize* away from the edges; pixels near the edges are
 simply copied from the input *img* to *out*.

 Mask and variance planes are, likewise, simply copied from *img* to
 *out*.
 */
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
medianFilter(MaskedImageT const& img,
             MaskedImageT & out,
             int halfsize) {
    int S = halfsize*2 + 1;
    int SS = S*S;
    typedef typename MaskedImageT::xy_locator xy_loc;
    xy_loc pix = img.xy_at(halfsize,halfsize);
    std::vector<typename xy_loc::cached_location_t> locs;
    for (int i=0; i<S; ++i) {
        for (int j=0; j<S; ++j) {
            locs.push_back(pix.cache_location(j-halfsize, i-halfsize));
        }
    }
    int W = img.getWidth();
    int H = img.getHeight();
    ImagePixelT vals[S*S];
    for (int y=halfsize; y<H-halfsize; ++y) {
        xy_loc inpix = img.xy_at(halfsize, y), end = img.xy_at(W-halfsize, y);
        for (typename MaskedImageT::x_iterator optr = out.row_begin(y) + halfsize;
             inpix != end; ++inpix.x(), ++optr) {
            for (int i=0; i<SS; ++i)
                vals[i] = inpix[locs[i]].image();
            std::nth_element(vals, vals+SS/2, vals+SS);
            optr.image() = vals[SS/2];
            optr.mask() = inpix.mask();
            optr.variance() = inpix.variance();
        }
    }

    // grumble grumble margins
    for (int y=0; y<2*halfsize; ++y) {
        int iy = y;
        if (y >= halfsize)
            iy = H - 1 - (y-halfsize);
        typename MaskedImageT::x_iterator optr = out.row_begin(iy);
        typename MaskedImageT::x_iterator iptr = img.row_begin(iy), end=img.row_end(iy);
        for (; iptr != end; ++iptr,++optr)
            *optr = *iptr;
    }
    for (int y=halfsize; y<H-halfsize; ++y) {
        typename MaskedImageT::x_iterator optr = out.row_begin(y);
        typename MaskedImageT::x_iterator iptr = img.row_begin(y), end=img.row_begin(y)+halfsize;
        for (; iptr != end; ++iptr,++optr)
            *optr = *iptr;
        iptr = img.row_begin(y) + ((W-1) - halfsize);
        end  = img.row_begin(y) + (W-1);
        optr = out.row_begin(y) + ((W-1) - halfsize);
        for (; iptr != end; ++iptr,++optr)
            *optr = *iptr;
    }

}

/**
 Given an image *mimg* and Peak location *peak*, overwrite *mimg* so
 that pixels further from the peak have values smaller than those
 close to the peak; make the profile monotonic-decreasing.

 The exact algorithm is a little more complicated than that.  The
 basic idea is of "casting a shadow" from a pixel to pixels farther
 from the peak in the same direction.  Done naively, this results in
 very narrow "shadows" and ragged profiles.  A tweak is to make the
 shadows "fatter" -- make a pixel shadow a wedge of pixels -- but if
 one does this naively, the wedge gets wider and wider too quickly.
 The algorithm works out from the peak in square "rings" of pixels, so
 if a pixel shadows a wedge 30 degrees wide, in the next ring of
 pixels the shadowed pixel at largest angle from the shadowing pixel
 will shade a yet-larger wedge, expanding the shadowing angle.  To
 reduce this effect, we work in chunks of 5 pixels in radius, only
 copying the intermediate pixels to the "shadowing" image at the end
 of each chunk.

 Currently the mask and variance planes of the input are totally
 ignored.

 For illustration, run tests/monotonic.py and look at im*.png
 */
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
makeMonotonic(
    MaskedImageT & mimg,
    det::Peak const& peak) {

    int cx = peak.getIx();
    int cy = peak.getIy();
    int ix0 = mimg.getX0();
    int iy0 = mimg.getY0();
    int iW = mimg.getWidth();
    int iH = mimg.getHeight();

    ImagePtrT img = mimg.getImage();
    ImagePtrT shadowingImg = ImagePtrT(new ImageT(*img, true));

    int DW = std::max(cx - mimg.getX0(), mimg.getX0() + mimg.getWidth() - cx);
    int DH = std::max(cy - mimg.getY0(), mimg.getY0() + mimg.getHeight() - cy);

    const int S = 5;

    // Work out from the peak in chunks of "S" pixels.
    int s;
    for (s = 0; s < std::max(DW,DH); s += S) {
        int p;
        for (p=0; p<S; p++) {
            // visit pixels with L_inf distance = s + p from the
            // center (ie, the s+p'th square ring of pixels)
            // L is the half-length of the ring (box).
            int L = s+p;
            int x = L, y = -L;
            int dx = 0, dy = 0; // initialized here to satisfy the
                                // compiler; initialized for real
                                // below (first time through loop)
            /*
            int i;
            int leg;
            for (i=0; i<(8*L); i++, x += dx, y += dy) {
                if (i % (2*L) == 0) {
                    leg = (i/(2*L));
                    dx = ( leg    % 2) * (-1 + 2*(leg/2));
                    dy = ((leg+1) % 2) * ( 1 - 2*(leg/2));
                }
             */

            /*
             We visit pixels in a box of "radius" L, in this order:

             L=1:

             4 3 2
             5   1
             6 7 0

             L=2:

             8  7  6  5  4
             9           3
             10          2
             11          1
             12 13 14 15 0

             Note that the number of pixel visited is 8*L, and that we
             change "dx" or "dy" each "2*L" steps.
             */
            for (int i=0; i<(8*L); i++, x += dx, y += dy) {
                // time to change directions?  (Note that this runs
                // the first time through the loop, initializing dx,dy
                // appropriately.)
                if (i % (2*L) == 0) {
                    int leg = (i/(2*L));
                    // dx = [ 0, -1,  0, 1 ][leg]
                    dx = ( leg    % 2) * (-1 + 2*(leg/2));
                    // dy = [ 1,  0, -1, 0 ][leg]
                    dy = ((leg+1) % 2) * ( 1 - 2*(leg/2));
                }
                //printf("  i=%i, leg=%i, dx=%i, dy=%i, x=%i, y=%i\n", i, leg, dx, dy, x, y);
                int px = cx + x - ix0;
                int py = cy + y - iy0;
                // If the shadowing pixel is out of bounds, nothing to do.
                if (px < 0 || px >= iW || py < 0 || py >= iH)
                    continue;
                // The pixel casting the shadow
                ImagePixelT pix = (*shadowingImg)(px,py);

                // Cast this pixel's shadow S pixels long in a cone.
                // We compute the range of slopes (or inverse-slopes)
                // shadowed by the pixel, [ds0,ds1]
                double ds0, ds1;
                // Range of slopes shadowed
                const double A = 0.3;
                int shx, shy;
                int psx, psy;
                // Are we traversing a vertical edge of the box?
                if (dx == 0) {
                    // (if so, then "x" is +- L, so no div-by-zero)
                    ds0 = (double(y) / double(x)) - A;
                    ds1 = ds0 + 2.0 * A;
                    // cast the shadow on column x + sign(x)*shx
                    for (shx=1; shx<=S; shx++) {
                        int xsign = (x>0?1:-1);
                        // the column being shadowed
                        psx = cx + x + (xsign*shx) - ix0;
                        if (psx < 0 || psx >= iW)
                            continue;
                        // shadow covers a range of y values based on slope
                        for (shy  = iround(shx * ds0);
                             shy <= iround(shx * ds1); shy++) {
                            psy = cy + y + xsign*shy - iy0;
                            if (psy < 0 || psy >= iH)
                                continue;
                            (*img)(psx, psy) = std::min((*img)(psx, psy), pix);
                        }
                    }

                } else {
                    // We're traversing a horizontal edge of the box; y = +-L
                    ds0 = (double(x) / double(y)) - A;
                    ds1 = ds0 + 2.0 * A;
                    // Cast shadow on row y + sign(y)*shy
                    for (shy=1; shy<=S; shy++) {
                        int ysign = (y>0?1:-1);
                        psy = cy + y + (ysign*shy) - iy0;
                        if (psy < 0 || psy >= iH)
                            continue;
                        // shadow covers a range of x vals based on slope
                        for (shx  = iround(shy * ds0);
                             shx <= iround(shy * ds1); shx++) {
                            psx = cx + x + ysign*shx - ix0;
                            if (psx < 0 || psx >= iW)
                                continue;
                            (*img)(psx, psy) = std::min((*img)(psx, psy), pix);
                        }
                    }
                }
            }
        }
        //*shadowingImg <<= *img;
        shadowingImg->operator<<=(*img);
    }
}

template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
_find_stray_flux(det::Footprint const& foot,
                 ImagePtrT tsum,
                 MaskedImageT const& img,
                 int strayFluxOptions,
                 std::vector<det::Footprint::Ptr> tfoots,
                 std::vector<bool> const& ispsf,
                 std::vector<int>  const& pkx,
                 std::vector<int>  const& pky,
                 double clipStrayFluxFraction,
                 std::vector<boost::shared_ptr<typename det::HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT> > > & strays
                 ) {

    typedef typename det::Footprint::SpanList SpanList;
    typedef typename det::HeavyFootprint<ImagePixelT, MaskPixelT, VariancePixelT> HeavyFootprint;
    typedef typename boost::shared_ptr< HeavyFootprint > HeavyFootprintPtrT;

    // when doing stray flux: the footprints and pixels, which we'll
    // combine into the return 'strays' HeavyFootprint at the end.
    std::vector<det::Footprint::Ptr > strayfoot;
    std::vector<std::vector<ImagePixelT> > straypix;
    std::vector<std::vector<MaskPixelT> > straymask;
    std::vector<std::vector<VariancePixelT> > strayvar;

    int ix0 = img.getX0();
    int iy0 = img.getY0();
    geom::Box2I sumbb = tsum->getBBox(image::PARENT);
    int sumx0 = sumbb.getMinX();
    int sumy0 = sumbb.getMinY();

    for (size_t i=0; i<tfoots.size(); ++i) {
        strayfoot.push_back(det::Footprint::Ptr());
        straypix.push_back(std::vector<ImagePixelT>());
        straymask.push_back(std::vector<MaskPixelT>());
        strayvar.push_back(std::vector<VariancePixelT>());
    }

    // Go through the (parent) Footprint looking for stray flux:
    // pixels that are not claimed by any template, and positive.
    const SpanList& spans = foot.getSpans();
    for (SpanList::const_iterator s = spans.begin(); s < spans.end(); s++) {
        int y = (*s)->getY();
        int x0 = (*s)->getX0();
        int x1 = (*s)->getX1();
        typename ImageT::x_iterator tsum_it =
            tsum->row_begin(y - sumy0) + (x0 - sumx0);
        typename MaskedImageT::x_iterator in_it =
            img.row_begin(y - iy0) + (x0 - ix0);
        double contrib[tfoots.size()];

        for (int x = x0; x <= x1; ++x, ++tsum_it, ++in_it) {
            // Skip pixels that are covered by at least one
            // template (*tsum_it > 0) or the input is not
            // positive (*in_it <= 0).
            if ((*tsum_it > 0) || (*in_it).image() <= 0) {
                continue;
            }
            //printf("Pixel at (%i,%i) has stray flux: %g\n", x, y, (float)*in_it);

            if (strayFluxOptions & STRAYFLUX_R_TO_FOOTPRINT) {
                // By 1/r^2 to nearest pixel within the footprint
                for (size_t i=0; i<tfoots.size(); ++i) {
                    double minr2 = 1e12;
                    const SpanList tspans = tfoots[i]->getSpans();
                    for (SpanList::const_iterator ts = tspans.begin();
                         ts < tspans.end(); ++ts) {
                        geom::Span* sp = ts->get();
                        int mindx;
                        // span is to right of pixel?
                        int dx = sp->getX0() - x;
                        if (dx >= 0) {
                            mindx = dx;
                        } else {
                            // span is to left of pixel?
                            dx = x - sp->getX1();
                            if (dx >= 0) {
                                mindx = dx;
                            } else {
                                // span contains pixel (in x direction)
                                mindx = 0;
                            }
                        }
                        int dy = sp->getY() - y;
                        minr2 = std::min(minr2, (double)(mindx*mindx + dy*dy));
                    }
                    printf("stray flux at (%i,%i): dist to t %i is %g\n", x, y, i, sqrt(minr2));
                    contrib[i] = 1. / (1. + minr2);
                }

            } else {
                // Split the stray flux by 1/r^2 ...
                for (size_t i=0; i<tfoots.size(); ++i) {
                    int dx, dy;
                    dx = pkx[i] - x;
                    dy = pky[i] - y;
                    contrib[i] = 1. / (1. + dx*dx + dy*dy);
                }
            }

            // Round 1:
            bool always = (strayFluxOptions &
                           STRAYFLUX_TO_POINT_SOURCES_ALWAYS);
            // are we going to assign stray flux to ptsrcs?
            bool ptsrcs = always;

            double csum = 0.;
            for (size_t i=0; i<tfoots.size(); ++i) {
                // Skip deblended-as-PSF
                if ((!always) && ispsf.size() && ispsf[i]) {
                    continue;
                }
                csum += contrib[i];
            }
            if ((csum == 0.) &&
                (strayFluxOptions &
                 STRAYFLUX_TO_POINT_SOURCES_WHEN_NECESSARY)) {
                // No extended sources -- assign to pt sources
                //log.debugf("necessary to assign stray flux to point sources");
                ptsrcs = true;
                for (size_t i=0; i<tfoots.size(); ++i) {
                    csum += contrib[i];
                }
            }

            // Drop small contributions...
            double strayclip = (clipStrayFluxFraction * csum);
            csum = 0.;
            for (size_t i=0; i<tfoots.size(); ++i) {
                // skip ptsrcs?
                if ((!ptsrcs) && ispsf.size() && ispsf[i]) {
                    contrib[i] = 0.;
                    continue;
                }
                // skip small contributions
                if (contrib[i] < strayclip) {
                    //printf("clipping %g to zero\n", contrib[i]);
                    contrib[i] = 0.;
                    continue;
                }
                csum += contrib[i];
            }

            printf("stray flux at (%i,%i): contribs [", x, y);
            for (size_t i=0; i<tfoots.size(); ++i) {
                printf(" %g", contrib[i]/csum);
            }
            printf("]\n");

            for (size_t i=0; i<tfoots.size(); ++i) {
                if (contrib[i] == 0.) {
                    continue;
                }
                // the stray flux to give to template i
                double p = (contrib[i] / csum) * (*in_it).image();
                if (!strayfoot[i]) {
                    strayfoot[i] = det::Footprint::Ptr(new det::Footprint());
                }
                strayfoot[i]->addSpanInSeries(y, x, x);
                straypix[i].push_back(p);
                straymask[i].push_back((*in_it).mask());
                strayvar[i].push_back((*in_it).variance());
            }
        }
    }

    // Store the stray flux in HeavyFootprints
    for (size_t i=0; i<tfoots.size(); ++i) {
        if (!strayfoot[i]) {
            strays.push_back(HeavyFootprintPtrT());
        } else {
            /// Hmm, this is a little bit dangerous: we're assuming that
            /// the HeavyFootprint stores its pixels in the same order that
            /// we iterate over them above (ie, lexicographic).
            HeavyFootprintPtrT heavy(new HeavyFootprint(*strayfoot[i]));
            ndarray::Array<ImagePixelT,1,1> himg = heavy->getImageArray();
            typename std::vector<ImagePixelT>::const_iterator spix;
            typename std::vector<MaskPixelT>::const_iterator smask;
            typename std::vector<VariancePixelT>::const_iterator svar;
            typename ndarray::Array<ImagePixelT,1,1>::Iterator hpix;
            typename ndarray::Array<MaskPixelT,1,1>::Iterator mpix;
            typename ndarray::Array<VariancePixelT,1,1>::Iterator vpix;

            assert((size_t)strayfoot[i]->getNpix() == straypix[i].size());

            for (spix = straypix[i].begin(),
                     smask = straymask[i].begin(),
                     svar  = strayvar [i].begin(),
                     hpix = himg.begin(),
                     mpix = heavy->getMaskArray().begin(),
                     vpix = heavy->getVarianceArray().begin();
                 spix != straypix[i].end();
                 ++spix, ++smask, ++svar, ++hpix, ++mpix, ++vpix) {
                *hpix = *spix;
                *mpix = *smask;
                *vpix = *svar;
            }
            strays.push_back(heavy);
        }
    }
}

template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
_sum_templates(std::vector<MaskedImagePtrT> timgs,
               ImagePtrT tsum) {
    geom::Box2I sumbb = tsum->getBBox(image::PARENT);
    int sumx0 = sumbb.getMinX();
    int sumy0 = sumbb.getMinY();

    // Compute  tsum = the sum of templates
    for (size_t i=0; i<timgs.size(); ++i) {
        MaskedImagePtrT timg = timgs[i];
        geom::Box2I tbb = timg->getBBox(image::PARENT);
        int tx0 = tbb.getMinX();
        int ty0 = tbb.getMinY();
        // To handle "ramped" templates that can extend outside the
        // parent, clip the bbox.  Note that we saved tx0,ty0 BEFORE
        // doing this!
        tbb.clip(sumbb);
        int copyx0 = tbb.getMinX();
        // Here we iterate over the template bbox -- we could instead
        // iterate over the "tfoot"s.
        for (int y=tbb.getMinY(); y<=tbb.getMaxY(); ++y) {
            typename MaskedImageT::x_iterator in_it = timg->row_begin(y - ty0) +
                (copyx0 - tx0);
            typename MaskedImageT::x_iterator inend = in_it + tbb.getWidth();
            typename ImageT::x_iterator tsum_it =
                tsum->row_begin(y - sumy0) + (copyx0 - sumx0);
            for (; in_it != inend; ++in_it, ++tsum_it) {
                *tsum_it += std::max((ImagePixelT)0., (*in_it).image());
            }
        }
    }

}

/**
 Splits flux in a given image *img*, within a given footprint *foot*,
 among a number of templates *timgs*,*tfoots*.  This is where actual
 "deblending" takes place.

 *timgs* and *tfoots* MUST be the same length.

 Flux is assigned to templates according to their relative heights at
 each pixel.

 If *strayFluxOptions* includes *ASSIGN_STRAYFLUX*, then "stray flux"
 -- flux in the parent footprint that is not covered by any of the
 template footprints -- is assigned to templates based on their
 1/(1+r^2) distance.

 If *strayFluxOptions* includes *STRAYFLUX_R_TO_FOOTPRINT*, the stray
 flux is distributed to the footprints based on 1/(1+r^2) of the
 minimum distance from the stray flux to footprint; otherwise, it's
 based on (1/(1+r^2) from the peaks.

 If *strayFluxOptions* includes *STRAYFLUX_TO_POINT_SOURCES_ALWAYS*,
 then point sources are always included in the 1/(1+r^2) splitting.
 Otherwise, if *STRAYFLUX_TO_POINT_SOURCES_WHEN_NECESSARY*, point
 sources are included only if there are no extended sources nearby.

 If any stray-flux portion is less than 0.1%, it is clipped to zero.

 When doing stray flux, the "strays" arg is used as an extra return
 value, the stray flux assigned to each template.

 When doing stray flux, the *ispsf*, *pkx*, and *pky* arrays are
 required.  They give the peak x,y coords plus whether the peak is
 believed (by the deblender) to be a point source.  *pkx* and *pky*
 MUST be the same length as *timgs*.  If *ispsf* has nonzero length,
 it MUST be the same length as *timgs*.

 If *tsum* is given, is it set to the sum of max(0, template).

 The return value is a vector of MaskedImages containing the flux
 assigned to each template.

 */
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
std::vector<typename PTR(image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>)>
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
apportionFlux(MaskedImageT const& img,
              det::Footprint const& foot,
              std::vector<MaskedImagePtrT> timgs,
              std::vector<det::Footprint::Ptr> tfoots,
              ImagePtrT tsum,
              std::vector<bool> const& ispsf,
              std::vector<int>  const& pkx,
              std::vector<int>  const& pky,
              std::vector<boost::shared_ptr<typename det::HeavyFootprint<ImagePixelT,MaskPixelT,VariancePixelT> > > & strays,
              int strayFluxOptions,
              double clipStrayFluxFraction
    ) {
    typedef typename det::Footprint::SpanList SpanList;

    if (timgs.size() != tfoots.size()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Template images must be the same length as template footprints (%d vs %d)") % timgs.size() % tfoots.size()).str());
    }

    for (size_t i=0; i<timgs.size(); ++i) {
        if (!timgs[i]->getBBox(image::PARENT).contains(tfoots[i]->getBBox())) {
            throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                              "Template image MUST contain template footprint");
        }
        if (!img.getBBox(image::PARENT).contains(foot.getBBox())) {
            throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                              "Image bbox MUST contain parent footprint");
        }
        // template bounding-boxes *can* extend outside the parent
        // footprint if we are ramping templates with significant flux
        // at the edges.  We handle this below.
    }

    // the apportioned flux return value
    std::vector<MaskedImagePtrT> portions;

    pexLog::Log log(pexLog::Log::getDefaultLog(),
                    "lsst.meas.deblender.apportionFlux");
    bool findStrayFlux = (strayFluxOptions & ASSIGN_STRAYFLUX);

    int ix0 = img.getX0();
    int iy0 = img.getY0();
    geom::Box2I fbb = foot.getBBox();

    if (!tsum) {
        tsum = ImagePtrT(new ImageT(fbb.getDimensions()));
        tsum->setXY0(fbb.getMinX(), fbb.getMinY());
    }

    if (!tsum->getBBox(image::PARENT).contains(foot.getBBox())) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                          "Template sum image MUST contain parent footprint");
    }

    geom::Box2I sumbb = tsum->getBBox(image::PARENT);
    int sumx0 = sumbb.getMinX();
    int sumy0 = sumbb.getMinY();

    _sum_templates(timgs, tsum);

    // Compute flux portions
    for (size_t i=0; i<timgs.size(); ++i) {
        MaskedImagePtrT timg = timgs[i];
        // Initialize return value:
        MaskedImagePtrT port(new MaskedImageT(timg->getDimensions()));
        port->setXY0(timg->getXY0());
        portions.push_back(port);

        // Split flux = image * template / tsum
        geom::Box2I tbb = timg->getBBox(image::PARENT);
        int tx0 = tbb.getMinX();
        int ty0 = tbb.getMinY();
        // As above
        tbb.clip(sumbb);
        int copyx0 = tbb.getMinX();
        for (int y=tbb.getMinY(); y<=tbb.getMaxY(); ++y) {
            typename MaskedImageT::x_iterator in_it =
                img.row_begin(y - iy0) + (copyx0 - ix0);
            typename MaskedImageT::x_iterator tptr =
                timg->row_begin(y - ty0) + (copyx0 - tx0);
            typename MaskedImageT::x_iterator tend = tptr + tbb.getWidth();
            typename ImageT::x_iterator tsum_it =
                tsum->row_begin(y - sumy0) + (copyx0 - sumx0);
            typename MaskedImageT::x_iterator out_it =
                port->row_begin(y - ty0) + (copyx0 - tx0);
            for (; tptr != tend; ++tptr, ++in_it, ++out_it, ++tsum_it) {
                if (*tsum_it == 0) {
                    continue;
                }
                double frac = std::max((ImagePixelT)0., (*tptr).image()) / (*tsum_it);
                //if (frac == 0) {
                // treat mask planes differently?
                // }
                out_it.mask()     = (*in_it).mask();
                out_it.variance() = (*in_it).variance();
                out_it.image()    = (*in_it).image() * frac;
            }
        }
    }

    if (findStrayFlux) {
        if ((ispsf.size() > 0) && (ispsf.size() != timgs.size())) {
            throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                              (boost::format("'ispsf' must be the same length as templates (%d vs %d)") % ispsf.size() % timgs.size()).str());
        }
        if ((pkx.size() != timgs.size()) || (pky.size() != timgs.size())) {
            throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                              (boost::format("'pkx' and 'pky' must be the same length as templates (%d,%d vs %d)") % pkx.size() % pky.size() % timgs.size()).str());
        }
        _find_stray_flux(foot, tsum, img, strayFluxOptions, tfoots,
                         ispsf, pkx, pky, clipStrayFluxFraction, strays);
    }
    return portions;
}


/**
 This is a convenience class used in symmetrizeFootprint, wrapping the
 idea of iterating through a SpanList either forward or backward, and
 looking at dx,dy coordinates relative to a center cx,cy coordinate.
 This makes the symmetrizeFootprint code much tidier and more
 symmetric-looking; the operations on the forward and backward
 iterators are mostly the same.
 */
class RelativeSpanIterator {
public:
    typedef det::Footprint::SpanList SpanList;

    RelativeSpanIterator() {}

    RelativeSpanIterator(SpanList::const_iterator const& real,
                         SpanList const& arr,
                         int cx, int cy, bool forward=true) 
        : _real(real), _cx(cx), _cy(cy), _forward(forward)
        {
            if (_forward) {
                _end = arr.end();
            } else {
                _end = arr.begin();
            }
        }

    bool operator==(const SpanList::const_iterator & other) {
        return _real == other;
    }
    bool operator!=(const SpanList::const_iterator & other) {
        return _real != other;
    }
    bool operator<=(const SpanList::const_iterator & other) {
        return _real <= other;
    }
    bool operator<(const SpanList::const_iterator & other) {
        return _real < other;
    }
    bool operator>=(const SpanList::const_iterator & other) {
        return _real >= other;
    }
    bool operator>(const SpanList::const_iterator & other) {
        return _real > other;
    }

    bool operator==(RelativeSpanIterator & other) {
        return (_real == other._real) &&
            (_cx == other._cx) && (_cy == other._cy) &&
            (_forward == other._forward);
    }
    bool operator!=(RelativeSpanIterator & other) {
        return !(*this == other);
    }

    RelativeSpanIterator operator++() {
        if (_forward) {
            _real++;
        } else {
            _real--;
        }
        return *this;
    }
    RelativeSpanIterator operator++(int dummy) {
        RelativeSpanIterator result = *this;
        ++(*this);
        return result;
    }

    bool notDone() {
        if (_forward) {
            return _real < _end;
        } else {
            return _real >= _end;
        }
    }

    int dxlo() {
        if (_forward) {
            return (*_real)->getX0() - _cx;
        } else {
            return _cx - (*_real)->getX1();
        }
    }
    int dxhi() {
        if (_forward) {
            return (*_real)->getX1() - _cx;
        } else {
            return _cx - (*_real)->getX0();
        }
    }
    int dy() {
        return std::abs((*_real)->getY() - _cy);
    }
    int x0() {
        return (*_real)->getX0();
    }
    int x1() {
        return (*_real)->getX1();
    }
    int y() {
        return (*_real)->getY();
    }

private:
    SpanList::const_iterator _real;
    SpanList::const_iterator _end;
    int _cx;
    int _cy;
    bool _forward;
};


/*
 // Check symmetrizeFootprint by computing truth naively.
     // compute correct answer dumbly
     det::Footprint truefoot;
     geom::Box2I bbox = foot.getBBox();
     for (int y=bbox.getMinY(); y<=bbox.getMaxY(); y++) {
     for (int x=bbox.getMinX(); x<=bbox.getMaxX(); x++) {
     int dy = y - cy;
     int dx = x - cx;
     int x2 = cx - dx;
     int y2 = cy - dy;
     if (foot.contains(geom::Point2I(x,  y)) &&
     foot.contains(geom::Point2I(x2, y2))) {
     truefoot.addSpan(y, x, x);
     }
     }
     }
     truefoot.normalize();

     SpanList sp1 = truefoot.getSpans();
     SpanList sp2 = sfoot->getSpans();
     for (SpanList::const_iterator spit1 = sp1.begin(),
     spit2 = sp2.begin();
     spit1 != sp1.end() && spit2 != sp2.end();
     spit1++, spit2++) {
     //printf("\n");
     printf(" true   y %i, x [%i, %i]\n", (*spit1)->getY(), (*spit1)->getX0(), (*spit1)->getX1());
     printf(" sfoot  y %i, x [%i, %i]\n", (*spit2)->getY(), (*spit2)->getX0(), (*spit2)->getX1());
     if (((*spit1)->getY()  != (*spit2)->getY()) ||
     ((*spit1)->getX0() != (*spit2)->getX0()) ||
     ((*spit1)->getX1() != (*spit2)->getX1())) {
     printf("*******************************************\n");
     }
     }
     assert(sp1.size() == sp2.size());
     for (SpanList::const_iterator spit1 = sp1.begin(),
     spit2 = sp2.begin();
     spit1 != sp1.end() && spit2 != sp2.end();
     spit1++, spit2++) {
     assert((*spit1)->getY()  == (*spit2)->getY());
     assert((*spit1)->getX0() == (*spit2)->getX0());
     assert((*spit1)->getX1() == (*spit2)->getX1());
     }
 */

/**
 Given a Footprint *foot* and peak *cx*,*cy*, returns a Footprint that
 is symmetric around the peak (with twofold rotational symmetry) --
 the AND of the two symmetric halves.
 */
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
PTR(lsst::afw::detection::Footprint)
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
symmetrizeFootprint(
    det::Footprint const& foot,
    int cx, int cy) {

    typedef typename det::Footprint::SpanList SpanList;

    PTR(det::Footprint) sfoot(new det::Footprint);
    const SpanList spans = foot.getSpans();
    assert(foot.isNormalized());

    pexLog::Log log(pexLog::Log::getDefaultLog(),
                    "lsst.meas.deblender.symmetrizeFootprint");

    // Find the Span containing the peak.
    PTR(det::Span) target(new det::Span(cy, cx, cx));
    SpanList::const_iterator peakspan =
        std::lower_bound(spans.begin(), spans.end(), target, span_ptr_compare);
    // lower_bound returns the first position where "target" could be inserted;
    // ie, the first Span larger than "target".  The Span containing "target"
    // should be peakspan-1.
    PTR(det::Span) sp;
    if (peakspan == spans.begin()) {
        sp = *peakspan;
        if (!sp->contains(cx, cy)) {
            log.warnf("Failed to find span containing (%i,%i): before the beginning of this footprint", cx, cy);
            return PTR(det::Footprint)();
        }
    } else {
        peakspan--;
        sp = *peakspan;

        if (!(sp->contains(cx,cy))) {
            geom::Box2I fbb = foot.getBBox();
            log.warnf("Failed to find span containing (%i,%i): nearest is %i, [%i,%i].  Footprint bbox is [%i,%i],[%i,%i]",
                      cx, cy, sp->getY(), sp->getX0(), sp->getX1(),
                      fbb.getMinX(), fbb.getMaxX(), fbb.getMinY(), fbb.getMaxY());
            return det::Footprint::Ptr();
        }
    }
    log.debugf("Span containing (%i,%i): (x=[%i,%i], y=%i)",
               cx, cy, sp->getX0(), sp->getX1(), sp->getY());

    // The symmetric templates are essentially an AND of the footprint
    // pixels and its 180-degree-rotated self, rotated around the
    // peak (cx,cy).
    //
    // We iterate forward and backward simultaneously, starting from
    // the span containing the peak and moving out, row by row.
    // 
    // In the loop below, we search for the next pair of Spans that
    // overlap (in "dx" from the center), output the overlapping
    // portion of the Spans, and advance either the "fwd" or "back"
    // iterator.  When we fail to find an overlapping pair of Spans,
    // we move on to the next row.
    //
    // [The following paragraph is somewhat obsoleted by the
    // RelativeSpanIterator class, which performs some of the renaming
    // and the dx,dy coords.]
    //
    // '''In reading the code, "forward", "advancing", etc, are all
    // from the perspective of the "fwd" iterator (the one going
    // forward through the Span list, from low to high Y and then low
    // to high X).  It will help to imagine making a copy of the
    // footprint and rotating it around the center pixel by 180
    // degrees, so that "fwd" and "back" are both iterating the same
    // direction; we're then just finding the AND of those two
    // iterators, except we have to work in dx,dy coordinates rather
    // than original x,y coords, and the accessors for "back" are
    // opposite.'''

    RelativeSpanIterator fwd (peakspan, spans, cx, cy, true);
    RelativeSpanIterator back(peakspan, spans, cx, cy, false);

    int dy = 0;
    while (fwd.notDone() && back.notDone()) {
        // forward and backward "y"; just symmetric around cy
        int fy = cy + dy;
        int by = cy - dy;
        // delta-x of the beginnings of the spans, for "fwd" and "back"
        int fdxlo =  fwd.dxlo();
        int bdxlo = back.dxlo();

        // First find:
        //    fend -- first span in the next row, or end(); ie,
        //            the end of this row in the forward direction
        //    bend -- the end of this row in the backward direction
        RelativeSpanIterator fend, bend;
        for (fend = fwd; fend.notDone(); ++fend) {
            if (fend.dy() != dy)
                break;
        }
        for (bend = back; bend.notDone(); ++bend) {
            if (bend.dy() != dy)
                break;
        }

        log.debugf("dy=%i, fy=%i, fx=[%i, %i],   by=%i, fx=[%i, %i],  fdx=%i, bdx=%i",
                   dy, fy, fwd.x0(), fwd.x1(), by, back.x0(), back.x1(),
                   fdxlo, bdxlo);

        // Find possibly-overlapping span
        if (bdxlo > fdxlo) {
            log.debugf("Advancing forward.");
            // While the "forward" span is entirely to the "left" of the "backward" span,
            // (in dx coords), ie, |---fwd---X   X---back---|
            // and we are comparing the edges marked X
            while ((fwd != fend) && (fwd.dxhi() < bdxlo)) {
                fwd++;
                if (fwd == fend) {
                    log.debugf("Reached fend");
                } else {
                    log.debugf("Advanced to forward span %i, [%i, %i]",
                               fy, fwd.x0(), fwd.x1());
                }
            }
        } else if (fdxlo > bdxlo) {
            log.debugf("Advancing backward.");
            // While the "backward" span is entirely to the "left" of the "foreward" span,
            // (in dx coords), ie, |---back---X   X---fwd---|
            // and we are comparing the edges marked X
            while ((back != bend) && (back.dxhi() < fdxlo)) {
                back++;
                if (back == bend) {
                    log.debugf("Reached bend");
                } else {
                    log.debugf("Advanced to backward span %i, [%i, %i]",
                               by, back.x0(), back.x1());
                }
            }
        }

        if ((back == bend) || (fwd == fend)) {
            // We reached the end of the row without finding spans that could
            // overlap.  Move onto the next dy.
            if (back == bend) {
                log.debugf("Reached bend");
            }
            if (fwd == fend) {
                log.debugf("Reached fend");
            }
            back = bend;
            fwd  = fend;
            dy++;
            continue;
        }

        // Spans may overlap -- find the overlapping part.
        int dxlo = std::max(fwd.dxlo(), back.dxlo());
        int dxhi = std::min(fwd.dxhi(), back.dxhi());
        if (dxlo <= dxhi) {
            log.debugf("Adding span fwd %i, [%i, %i],  back %i, [%i, %i]",
                       fy, cx+dxlo, cx+dxhi, by, cx-dxhi, cx-dxlo);
            sfoot->addSpan(fy, cx + dxlo, cx + dxhi);
            sfoot->addSpan(by, cx - dxhi, cx - dxlo);
        }

        // Advance the one whose "hi" edge is smallest
        if (fwd.dxhi() < back.dxhi()) {
            fwd++;
            if (fwd == fend) {
                log.debugf("Stepped to fend");
            } else {
                log.debugf("Stepped forward to span %i, [%i, %i]",
                           fwd.y(), fwd.x0(), fwd.x1());
            }
        } else {
            back++;
            if (back == bend) {
                log.debugf("Stepped to bend");
            } else {
                log.debugf("Stepped backward to span %i, [%i, %i]",
                           back.y(), back.x0(), back.x1());
            }
        }

        if ((back == bend) || (fwd == fend)) {
            // Reached the end of the row.  On to the next dy!
            if (back == bend) {
                log.debugf("Reached bend");
            }
            if (fwd == fend) {
                log.debugf("Reached fend");
            }
            back = bend;
            fwd  = fend;
            dy++;
            continue;
        }

    }
    sfoot->normalize();
    return sfoot;
}

/**
 Given an *img*, footprint *foot*, and *peak*, creates a symmetric
 template around the peak; produce a MaskedImage and Footprint
 describing a footprint where:
   output pixels (cx + dx, cy + dy) and (cx - dx, cy - dy)
   = min(input pixels (cx + dx, cy + dy) and (cx - dx, cy - dy))

 For pixels that are pulled down by more than 1 sigma, the SYMM_1SIG
 bit is set; for pixels pulled down by more than 3 sigma, SYMM_3SIG.

 If *patchEdge* is true and if the footprint touches pixels with the
 EDGE bit set, then for spans whose symmetric mirror are outside the
 image, the symmetric footprint is grown to include them and their
 pixel values are stored.
 */
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
std::pair<typename PTR(lsst::afw::image::MaskedImage<ImagePixelT,MaskPixelT,VariancePixelT>),
          typename PTR(lsst::afw::detection::Footprint) >
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
buildSymmetricTemplate(
    MaskedImageT const& img,
    det::Footprint const& foot,
    det::Peak const& peak,
    double sigma1,
    bool minZero,
    bool patchEdge,
    bool* patchedEdges) {

    typedef typename MaskedImageT::const_xy_locator xy_loc;
    typedef typename det::Footprint::SpanList SpanList;

    *patchedEdges = false;

    int cx = peak.getIx();
    int cy = peak.getIy();

    pexLog::Log log(pexLog::Log::getDefaultLog(),
                    "lsst.meas.deblender.symmetricFootprint");

    FootprintPtrT sfoot = symmetrizeFootprint(foot, cx, cy);
    if (!sfoot) {
        return std::pair<MaskedImagePtrT, FootprintPtrT>(MaskedImagePtrT(), sfoot);
    }
    const SpanList spans = sfoot->getSpans();

    // does this footprint touch an EDGE?
    bool touchesEdge = false;
    if (patchEdge) {
        log.debugf("Checking footprint for EDGE bits");
        MaskPtrT mask = img.getMask();
        bool edge = false;
        MaskPixelT edgebit = mask->getPlaneBitMask("EDGE");
        for (SpanList::const_iterator fwd=spans.begin();
             fwd != spans.end(); ++fwd) {
            int x0 = (*fwd)->getX0();
            int x1 = (*fwd)->getX1();
            typename MaskT::x_iterator xiter =
                mask->x_at(x0 - mask->getX0(), (*fwd)->getY() - mask->getY0());
            for (int x=x0; x<=x1; ++x, ++xiter) {
                if ((*xiter) & edgebit) {
                    edge = true;
                    break;
                }
            }
            if (edge)
                break;
        }
        if (edge) {
            log.debugf("Footprint includes an EDGE pixel.");
            touchesEdge = true;
        }
    }

    // The result image:
    MaskedImagePtrT timg(new MaskedImageT(sfoot->getBBox()));

    MaskPixelT symm1sig = img.getMask()->getPlaneBitMask("SYMM_1SIG");
    MaskPixelT symm3sig = img.getMask()->getPlaneBitMask("SYMM_3SIG");

    SpanList::const_iterator fwd  = spans.begin();
    SpanList::const_iterator back = spans.end()-1;

    ImagePtrT theimg = img.getImage();
    ImagePtrT targetimg  = timg->getImage();
    MaskPtrT  targetmask = timg->getMask();

    for (; fwd <= back; fwd++, back--) {
        int fy = (*fwd)->getY();
        int by = (*back)->getY();

        for (int fx=(*fwd)->getX0(), bx=(*back)->getX1();
             fx <= (*fwd)->getX1();
             fx++, bx--) {
            // FIXME -- CURRENTLY WE IGNORE THE MASK PLANE!  options
            // include ORing the mask bits, or being clever about
            // ignoring some masked pixels, or copying the mask bits
            // of the min pixel

            // FIXME -- we could do this with image iterators instead.
            // But first profile to show that it's necessary and an
            // improvement.
            ImagePixelT pixf = theimg->get0(fx, fy);
            ImagePixelT pixb = theimg->get0(bx, by);
            ImagePixelT pix = std::min(pixf, pixb);
            if (minZero) {
                pix = std::max(pix, static_cast<ImagePixelT>(0));
            }
            targetimg->set0(fx, fy, pix);
            targetimg->set0(bx, by, pix);

            // Set the "symm1sig" mask bit for pixels that have been
            // pulled down at least 1 sigma by the symmetry
            // constraint, and the "symm3sig" mask bit for pixels
            // pulled down by 3 sigma or more.
            if (pixf >= pix + 1.*sigma1) {
                targetmask->set0(fx, fy, targetmask->get0(fx, fy) | symm1sig);
                if (pixf >= pix + 3.*sigma1) {
                    targetmask->set0(fx, fy, targetmask->get0(fx, fy) | symm3sig);
                }
            }
            if (pixb >= pix + 1.*sigma1) {
                targetmask->set0(bx, by, targetmask->get0(bx, by) | symm1sig);
                if (pixb >= pix + 3.*sigma1) {
                    targetmask->set0(bx, by, targetmask->get0(bx, by) | symm3sig);
                }
            }
        }
    }


    if (touchesEdge) {
        // Find spans whose mirrors fall outside the image bounds,
        // grow the footprint to include those spans, and plug in
        // their pixel values.
        geom::Box2I bb = sfoot->getBBox();

        // Actually, it's not necessarily the IMAGE bounds that count
        //-- the footprint may not go right to the image edge.
        //geom::Box2I imbb = img.getBBox(image::PARENT);
        geom::Box2I imbb = foot.getBBox();

        log.debugf("Footprint touches EDGE: start bbox [%i,%i],[%i,%i]",
                   bb.getMinX(), bb.getMaxX(), bb.getMinY(), bb.getMaxY());
        // original footprint spans
        const SpanList ospans = foot.getSpans();
        for (fwd = ospans.begin(); fwd != ospans.end(); ++fwd) {
            int y = (*fwd)->getY();
            int x = (*fwd)->getX0();
            // mirrored coords
            int ym = cy + (cy - y);
            int xm = cx + (cx - x);
            if (!imbb.contains(geom::Point2I(xm, ym))) {
                bb.include(geom::Point2I(x, y));
            }
            x = (*fwd)->getX1();
            xm = cx + (cx - x);
            if (!imbb.contains(geom::Point2I(xm, ym))) {
                bb.include(geom::Point2I(x, y));
            }
        }
        log.debugf("Footprint touches EDGE: grown bbox [%i,%i],[%i,%i]",
                   bb.getMinX(), bb.getMaxX(), bb.getMinY(), bb.getMaxY());

        // New template image
        MaskedImagePtrT timg2(new MaskedImageT(bb));
        copyWithinFootprint(*sfoot, timg, timg2);

        log.debugf("Symmetric footprint spans:");
        const SpanList sspans = sfoot->getSpans();
        for (fwd = sspans.begin(); fwd != sspans.end(); ++fwd) {
            log.debugf("  %s", (*fwd)->toString().c_str());
        }

        // copy original 'img' pixels for the portion of spans whose
        // mirrors are out of bounds.
        for (fwd = ospans.begin(); fwd != ospans.end(); ++fwd) {
            int y   = (*fwd)->getY();
            int x0  = (*fwd)->getX0();
            int x1 = (*fwd)->getX1();
            // mirrored coords
            int ym  = cy + (cy - y);
            int xm0 = cx + (cx - x0);
            int xm1 = cx + (cx - x1);
            bool in0 = imbb.contains(geom::Point2I(xm0, ym));
            bool in1 = imbb.contains(geom::Point2I(xm1, ym));
            if (in0 && in1) {
                // both endpoints of the symmetric span are in bounds; nothing to do
                continue;
            }
            // clip to the part of the span where the mirror is out of bounds
            if (in0) {
                // the mirror of x0 is in-bounds; move x0 to be the first pixel
                // whose mirror would be out-of-bounds
                x0 = cx + (cx - (imbb.getMinX() - 1));
            }
            if (in1) {
                x1 = cx + (cx - (imbb.getMaxX() + 1));
            }
            log.debugf("Span y=%i, x=[%i,%i] has mirror (%i,[%i,%i]) out-of-bounds; clipped to %i,[%i,%i]",
                       y, (*fwd)->getX0(), (*fwd)->getX1(), ym, xm1, xm0, y, x0, x1);
            typename MaskedImageT::x_iterator initer =
                img.x_at(x0 - img.getX0(), y - img.getY0());
            typename MaskedImageT::x_iterator outiter =
                timg2->x_at(x0 - timg2->getX0(), y - timg2->getY0());
            for (int x=x0; x<=x1; ++x, ++outiter, ++initer) {
                *outiter = *initer;
            }
            sfoot->addSpan(y, x0, x1);
        }
        sfoot->normalize();
        timg = timg2;
    }

    *patchedEdges = touchesEdge;
    return std::pair<MaskedImagePtrT, FootprintPtrT>(timg, sfoot);
}

/**
 Returns true if the given Footprint *sfoot* in image *img* has flux
 above value *thresh* at its edge.
 */
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
bool
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
hasSignificantFluxAtEdge(ImagePtrT img,
                         det::Footprint::Ptr sfoot,
                         ImagePixelT thresh) {
    typedef typename det::Footprint::SpanList SpanList;

    pexLog::Log log(pexLog::Log::getDefaultLog(),
                    "lsst.meas.deblender.hasSignificantFluxAtEdge");

    // Find edge template pixels with significant flux -- perhaps
    // because their symmetric pixels were outside the footprint?
    // (clipped by an image edge, etc)

    const SpanList spans = sfoot->getSpans();
    for (SpanList::const_iterator sp = spans.begin();
         sp != spans.end(); ++sp) {
        // We first search for significant pixels, and then check
        // whether they are on the edge of the footprint.
        // We have to check above and below all pixels and left and
        // right of the end pixels.  Faster to do this in image space?
        // (Inserting footprint into image, operating on image,
        // re-grabbing footprint, like growFootprint) Or cache the
        // span starts and ends of a sliding window of rows?
        int y  = (*sp)->getY();
        int x0 = (*sp)->getX0();
        int x1 = (*sp)->getX1();
        int x;
        typename ImageT::const_x_iterator xiter;
        for (xiter = img->x_at(x0 - img->getX0(), y - img->getY0()), x=x0; 
             x<=x1; ++x, ++xiter) {

            assert(img->getBBox(image::PARENT).contains(geom::Point2I(x, y)));

            if (*xiter < thresh)
                // not significant
                continue;
            // Since the footprint is normalized, all span endpoints
            // are on the boundary.
            if ((x != x0) && (x != x1) &&
                sfoot->contains(geom::Point2I(x, y-1)) &&
                sfoot->contains(geom::Point2I(x, y+1))) {
                // not edge
                continue;
            }
            log.debugf("Found significant template-edge pixel: %i,%i = %f > %f",
                       x, y, (float)*xiter, (float)thresh);
            return true;
        }
    }
    return false;
}

/**
 Returns a list of pixels that are on the edge of the given Footprint
 *sfoot* in image *img*, above threshold *thresh*.
 */
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
boost::shared_ptr<det::Footprint>
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
getSignificantEdgePixels(ImagePtrT img,
                         det::Footprint::Ptr sfoot,
                         ImagePixelT thresh) {
    typedef typename det::Footprint::SpanList SpanList;
    pexLog::Log log(pexLog::Log::getDefaultLog(),
                    "lsst.meas.deblender.getSignificantEdgePixels");
    sfoot->normalize();
    const SpanList spans = sfoot->getSpans();
    SpanList::const_iterator sp;
    det::Footprint::Ptr edgepix(new det::Footprint());

    for (sp = spans.begin(); sp != spans.end(); sp++) {
        int y  = (*sp)->getY();
        int x0 = (*sp)->getX0();
        int x1 = (*sp)->getX1();
        int x;
        typename ImageT::const_x_iterator xiter;
        for (xiter = img->x_at(x0 - img->getX0(), y - img->getY0()), x=x0;
             x<=x1; ++x, ++xiter) {
            if (*xiter < thresh)
                // not significant
                continue;
            // Since the footprint is normalized, all span endpoints
            // are on the boundary.
            if ((x != x0) && (x != x1) &&
                sfoot->contains(geom::Point2I(x, y-1)) &&
                sfoot->contains(geom::Point2I(x, y+1))) {
                // not edge
                continue;
            }
            log.debugf("Found significant edge pixel: %i,%i = %f",
                       x, y, (float)*xiter);
            edgepix->addSpanInSeries(y, x, x);
        }
    }
    return edgepix;
}

// Instantiate
template class deblend::BaselineUtils<float>;
