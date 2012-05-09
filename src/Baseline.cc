
#include "lsst/meas/deblender/Baseline.h"
#include "lsst/pex/logging.h"
#include "lsst/afw/geom/Box.h"

#include "lsst/meas/algorithms/detail/SdssShape.h"

namespace image = lsst::afw::image;
namespace det = lsst::afw::detection;
namespace deblend = lsst::meas::deblender;
namespace geom = lsst::afw::geom;
namespace malg = lsst::meas::algorithms;
namespace pexLog = lsst::pex::logging;

template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
std::vector<double>
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::
fitEllipse(ImageT const& image, double bkgd, double xc, double yc) {
	double shiftmax = 5.0;
	malg::detail::SdssShapeImpl shape;
	bool ok = malg::detail::getAdaptiveMoments(image, bkgd, xc, yc, shiftmax, &shape);
	assert(ok);
	std::vector<double> vals;
	vals.push_back(shape.getX());
	vals.push_back(shape.getY());
	vals.push_back(shape.getI0());
	vals.push_back(shape.getIxx());
	vals.push_back(shape.getIyy());
	vals.push_back(shape.getIxy());
	return vals;
}

bool span_ptr_compare(det::Span::Ptr sp1, det::Span::Ptr sp2) {
	return (*sp1 < *sp2);
}

static double sinsq(double dx, double dy) {
	return (dy*dy) / (dy*dy + dx*dx);
}

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
	//typename xy_loc::cached_location_t locs[SS];
	for (int i=0; i<S; ++i) {
		for (int j=0; j<S; ++j) {
			//locs[i*S + j] = pix.cache_location(j-halfsize, i-halfsize);
			locs.push_back(pix.cache_location(j-halfsize, i-halfsize));
		}
	}
	int W = img.getWidth();
	int H = img.getHeight();
	ImagePixelT vals[S*S];
	for (int y=halfsize; y<=H-halfsize; ++y) {
        xy_loc inpix = img.xy_at(halfsize, y), end = img.xy_at(W-halfsize, y);
        for (typename MaskedImageT::x_iterator optr = out.row_begin(y) + halfsize;
             inpix != end; ++inpix.x(), ++optr) {
			for (int i=0; i<SS; ++i)
				vals[i] = inpix[locs[i]].image();
			std::nth_element(vals, vals+SS/2, vals+SS);
			//std::nth_element(vals.begin(), vals.begin()+SS/2, vals.end());
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


template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::makeMonotonic(
	MaskedImageT & mimg,
	det::Footprint const& foot,
	det::Peak const& peak,
	double sigma1) {

	// FIXME -- ignore the Footprint and just operate on the whole image

	int cx,cy;
	cx = peak.getIx();
	cy = peak.getIy();
	int ix0 = mimg.getX0();
	int iy0 = mimg.getY0();
	int iW = mimg.getWidth();
	int iH = mimg.getHeight();

	ImagePtrT img = mimg.getImage();
	MaskPtrT mask = mimg.getMask();

	MaskPixelT mono1sig = mask->getPlaneBitMask("MONOTONIC_1SIG");

	int DW = std::max(cx - mimg.getX0(), mimg.getX0() + mimg.getWidth() - cx);
	int DH = std::max(cy - mimg.getY0(), mimg.getY0() + mimg.getHeight() - cy);

	for (int dy=0; dy<DH; ++dy) {
		for (int dx=0; dx<DW; ++dx) {
			// find the pixels that "shadow" this pixel
			// alo = sin^2(angle)
			double alo,ahi;
			// center of pixel...
			alo = ahi = sinsq(dx, dy);
			/*
			 if (dy > 0 && dx > 0)
			 // bottom-left
			 alo = std::min(alo, sinsq(dx-0.5, dy-0.5));
			 */
			// bottom-right
			if (dy > 0)
				alo = std::min(alo, sinsq(dx+0.5, dy-0.5));
			// top-left
			if (dx > 0)
				ahi = std::max(ahi, sinsq(dx-0.5, dy+0.5));

			double a;
			bool w_shadow = (dx > 0);
			if (w_shadow) {
				a = sinsq(dx - 1, dy);
				w_shadow = w_shadow && (a >= alo) && (a <= ahi);
			}
			bool sw_shadow = (dx > 0) && (dy > 0);
			if (sw_shadow) {
				a = sinsq(dx - 1, dy - 1);
				sw_shadow = sw_shadow && (a >= alo) && (a <= ahi);
			}
			bool s_shadow = (dy > 0);
			if (s_shadow) {
				a = sinsq(dx, dy - 1);
				s_shadow = s_shadow && (a >= alo) && (a <= ahi);
			}

			//printf("cx,cy=(%i,%i), cx,cy-ixy0=(%i,%i), dx,dy = (%i, %i).  Shadowed by: %s %s %s\n", cx, cy, cx-ix0, cy-iy0, dx, dy, (w_shadow ? "W":" "), (sw_shadow ? "SW":"  "), (s_shadow ? "S":" "));

			/*
			 for (int signdx=-1; signdx<=1; signdx+=2) {
			 int dx = absdx * signdx;
			 }
			 */

			for (int signdx=1; signdx>=-1; signdx-=2) {
				if (dx == 0 && signdx == -1)
					break;
				for (int signdy=1; signdy>=-1; signdy-=2) {
					if (dy == 0 && signdy == -1)
						break;

					int px = cx + signdx*dx - ix0;
					int py = cy + signdy*dy - iy0;
					if (px < 0 || px >= iW || py < 0 || py >= iH)
						continue;
					ImagePixelT pix = (*img)(px,py);
					ImagePixelT minpix = pix;
					//printf("pix (%i,%i): %f\n", px, py, pix);
					assert(pix > -100000);
					if (w_shadow && (px-signdx >= 0) && (px-signdx < iW)) {
						minpix = std::min(minpix, (*img)(px - signdx, py));
						//printf("  w pix (%i,%i): %f -> %f\n", px-signdx, py, (*img)(px - signdx, py), pix);
					}
					if (s_shadow && (py-signdy >= 0) && (py-signdy < iH)) {
						minpix = std::min(minpix, (*img)(px, py - signdy));
						//printf("  s pix (%i,%i): %f -> %f\n", px, py-signdy, (*img)(px, py - signdy), pix);
					}
					if (sw_shadow && (px-signdx >= 0) && (px-signdx < iW) &&
						(py-signdy >= 0) && (py-signdy < iH)) {
						minpix = std::min(minpix, (*img)(px - signdx, py - signdy));
						//printf("  sw pix (%i,%i): %f -> %f\n", px-signdx, py-signdy, (*img)(px - signdx, py - signdy), pix);
					}
					(*img)(px,py) = minpix;
					if (pix > minpix + sigma1)
						(*mask)(px,py) |= mono1sig;
					assert(minpix > -100000);
				}
			}
		}
	}



}



template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
std::vector<typename image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr>
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::apportionFlux(MaskedImageT const& img,
																			 det::Footprint const& foot,
																			 std::vector<typename image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr> timgs) {
	std::vector<MaskedImagePtrT> portions;
	int ix0 = img.getX0();
	int iy0 = img.getY0();
	geom::Box2I fbb = foot.getBBox();
	ImageT sumimg(fbb.getDimensions());
	int sx0 = fbb.getMinX();
	int sy0 = fbb.getMinY();
	for (size_t i=0; i<timgs.size(); ++i) {
		MaskedImagePtrT timg = timgs[i];
		geom::Box2I tbb = timg->getBBox(image::PARENT);
		int tx0 = tbb.getMinX();
		//int tx1 = tbb.getMaxX();
		int ty0 = tbb.getMinY();
		for (int y=tbb.getMinY(); y<=tbb.getMaxY(); ++y) {
			typename MaskedImageT::x_iterator inptr = timg->row_begin(y - ty0);
			typename MaskedImageT::x_iterator inend = inptr + tbb.getWidth();
			typename ImageT::x_iterator sumptr = sumimg.row_begin(y - sy0) + (tx0 - sx0);
			for (; inptr != inend; ++inptr, ++sumptr) {
				//*sumptr += fabs((*inptr).image());
				*sumptr += std::max((ImagePixelT)0., (*inptr).image());
			}
		}
	}

	for (size_t i=0; i<timgs.size(); ++i) {
		MaskedImagePtrT timg = timgs[i];
		MaskedImagePtrT port(new MaskedImageT(timg->getDimensions()));
		port->setXY0(timg->getXY0());
		portions.push_back(port);
		geom::Box2I tbb = timg->getBBox(image::PARENT);
		int tx0 = tbb.getMinX();
		//int tx1 = tbb.getMaxX();
		int ty0 = tbb.getMinY();
		for (int y=tbb.getMinY(); y<=tbb.getMaxY(); ++y) {
			typename MaskedImageT::x_iterator inptr = img.row_begin(y - iy0) + (tx0 - ix0);
			typename MaskedImageT::x_iterator tptr = timg->row_begin(y - ty0);
			typename MaskedImageT::x_iterator tend = tptr + tbb.getWidth();
			typename ImageT::x_iterator sumptr = sumimg.row_begin(y - sy0) + (tx0 - sx0);
			typename MaskedImageT::x_iterator outptr = port->row_begin(y - ty0);
			for (; tptr != tend; ++tptr, ++inptr, ++outptr, ++sumptr) {
				if (*sumptr == 0)
					continue;
				outptr.mask()     = (*inptr).mask();
				outptr.variance() = (*inptr).variance();
				//outptr.image()    = (*inptr).image() * fabs((*tptr).image()) / (*sumptr);
				outptr.image()    = (*inptr).image() * std::max((ImagePixelT)0., (*tptr).image()) / (*sumptr);
			}
		}
	}
	return portions;
}

template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
lsst::afw::detection::Footprint::Ptr
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::symmetrizeFootprint(
    lsst::afw::detection::Footprint const& foot,
    int cx, int cy) {
	typedef typename det::Footprint::SpanList SpanList;

    det::Footprint::Ptr sfoot(new det::Footprint);
	const SpanList spans = foot.getSpans();
    assert(foot.isNormalized());

    pexLog::Log log(pexLog::Log::getDefaultLog(), "lsst.meas.deblender",
					//pexLog::Log::DEBUG);
					pexLog::Log::INFO);

	/*
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
	 */

	// Find the Span containing the peak.
	det::Span::Ptr target(new det::Span(cy, cx, cx));
	SpanList::const_iterator peakspan =
        std::lower_bound(spans.begin(), spans.end(), target, span_ptr_compare);
	// lower_bound returns the first position where "target" could be inserted;
	// ie, the first Span larger than "target".  The Span containing "target"
	// should be peakspan-1.
	if (peakspan == spans.begin()) {
		log.warnf("Failed to find span containing (%i,%i): before the beginning of this footprint");
		return det::Footprint::Ptr();
	}
	peakspan--;
	det::Span::Ptr sp = *peakspan;
	if (!(sp->contains(cx,cy))) {
		geom::Box2I fbb = foot.getBBox();
		log.warnf("Failed to find span containing (%i,%i): nearest is %i, [%i,%i].  Footprint bbox is [%i,%i],[%i,%i]",
				  cx, cy, sp->getY(), sp->getX0(), sp->getX1(),
				  fbb.getMinX(), fbb.getMaxX(), fbb.getMinY(), fbb.getMaxY());
		return det::Footprint::Ptr();
	}
	assert(sp->contains(cx,cy));
	log.debugf("Span containing (%i,%i): (x=[%i,%i], y=%i)",
               cx, cy, sp->getX0(), sp->getX1(), sp->getY());

	SpanList::const_iterator fwd  = peakspan;
	SpanList::const_iterator back = peakspan;
	int dy = 0;
	while ((fwd < spans.end()) && (back >= spans.begin())) {
		int fy = cy + dy;
		int by = cy - dy;
		// forward and backward delta-xs of the beginnings of the spans
		int fdx = (*fwd)->getX0() - cx;
		int bdx = cx - (*back)->getX1();

		SpanList::const_iterator fend;
		for (fend = fwd; fend != spans.end(); ++fend) {
			if ((*fend)->getY() != fy)
				break;
		}
		SpanList::const_iterator bend;
		for (bend=back; bend >= spans.begin(); --bend) {
			if ((*bend)->getY() != by)
				break;
		}
		log.debugf("dy=%i, fy=%i, fx=[%i, %i],   by=%i, fx=[%i, %i],  fdx=%i, bdx=%i",
				   dy, fy, (*fwd)->getX0(), (*fwd)->getX1(), by, (*back)->getX0(), (*back)->getX1(), fdx, bdx);

		if (bdx > fdx) {
			log.debugf("Advancing forward.");
			int fx = cx + bdx;
			// advance "fwd" until the first possibly-overlapping span is found.
			while ((fwd != fend) && (*fwd)->getX1() < fx) {
				fwd++;
				if (fwd == fend) {
					log.debugf("Reached fend");
				} else {
					log.debugf("Advanced to forward span %i, [%i, %i]",
							   (*fwd)->getY(), (*fwd)->getX0(), (*fwd)->getX1());
				}
			}
		} else if (fdx > bdx) {
			log.debugf("Advancing backward.");
			int bx = cx - fdx;
			// advance "back" until the first possibly-overlapping span is found.
			while ((back != bend) && (*back)->getX0() > bx) {
				back--;
				if (back == bend) {
					log.debugf("Reached bend");
				} else {
					log.debugf("Advanced to backward span %i, [%i, %i]",
							   (*back)->getY(), (*back)->getX0(), (*back)->getX1());
				}
			}
		}

		if ((back == bend) || (fwd == fend)) {
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

		int dxlo = std::max((*fwd)->getX0() - cx, cx - (*back)->getX1());
		int dxhi = std::min((*fwd)->getX1() - cx, cx - (*back)->getX0());
		//log.debugf("fy,by %i,%i, dx [%i, %i] --> [%i, %i], [%i, %i]\n", fy, by,
		//dxlo, dxhi, cx+dxlo, cx+dxhi, cx-dxhi, cx-dxlo);
		if (dxlo <= dxhi) {
			log.debugf("Adding span fwd %i, [%i, %i],  back %i, [%i, %i]",
					   fy, cx+dxlo, cx+dxhi, by, cx-dxhi, cx-dxlo);
			sfoot->addSpan(fy, cx + dxlo, cx + dxhi);
			sfoot->addSpan(by, cx - dxhi, cx - dxlo);
		}

		// Advance the one whose "hi" edge is smallest
		fdx = (*fwd)->getX1() - cx;
		bdx = cx - (*back)->getX0();
		if (fdx < bdx) {
			fwd++;
			if (fwd == fend) {
				log.debugf("Stepped to fend\n");
			} else {
				log.debugf("Stepped forward to span %i, [%i, %i]",
						   (*fwd)->getY(), (*fwd)->getX0(), (*fwd)->getX1());
			}
		} else {
			back--;
			if (back == bend) {
				log.debugf("Stepped to bend\n");
			} else {
				log.debugf("Stepped backward to span %i, [%i, %i]",
						   (*back)->getY(), (*back)->getX0(), (*back)->getX1());
			}
		}

		if ((back == bend) || (fwd == fend)) {
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

	/*
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

    return sfoot;
}

template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
std::pair<typename lsst::afw::image::MaskedImage<ImagePixelT,MaskPixelT,VariancePixelT>::Ptr,
		  typename lsst::afw::detection::Footprint::Ptr>
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::buildSymmetricTemplate(
	MaskedImageT const& img,
	det::Footprint const& foot,
	det::Peak const& peak,
	double sigma1) {

	typedef typename MaskedImageT::const_xy_locator xy_loc;
	typedef typename det::Footprint::SpanList SpanList;
	//typedef std::pair<MaskedImagePtrT, FootprintPtrT>

	int cx = peak.getIx();
	int cy = peak.getIy();
    FootprintPtrT sfoot = symmetrizeFootprint(foot, cx, cy);
	if (!sfoot) {
		return std::pair<MaskedImagePtrT, FootprintPtrT>(MaskedImagePtrT(), sfoot);
	}

    pexLog::Log log(pexLog::Log::getDefaultLog(), "lsst.meas.deblender", pexLog::Log::INFO);
    //bool debugging = (log.getThreshold() <= pexLog::Log::DEBUG);

    // The result image will be at most as large as the footprint
	geom::Box2I fbb = foot.getBBox();
	MaskedImageT timg(fbb.getDimensions());
	timg.setXY0(fbb.getMinX(), fbb.getMinY());

	MaskPixelT symm1sig = img.getMask()->getPlaneBitMask("SYMM_1SIG");
	MaskPixelT symm3sig = img.getMask()->getPlaneBitMask("SYMM_3SIG");

	const SpanList spans = sfoot->getSpans();

	SpanList::const_iterator fwd  = spans.begin();
	SpanList::const_iterator back = spans.end()-1;

    ImagePtrT theimg = img.getImage();
    ImagePtrT targetimg  = timg.getImage();
    MaskPtrT  targetmask = timg.getMask();

	for (; fwd <= back; fwd++, back--) {
		int fy = (*fwd)->getY();
		int by = (*back)->getY();

		for (int fx=(*fwd)->getX0(), bx=(*back)->getX1(); fx <= (*fwd)->getX1(); fx++, bx--) {
			// FIXME -- CURRENTLY WE IGNORE THE MASK PLANE!
            // options include ORing the mask bits, or being clever about ignoring
            // some masked pixels, or copying the mask bits of the min pixel
			// FIXME -- one of these conditions is redundant...

			// FIXME -- we could do this with image iterators
			// instead.  But first profile to show that it's
			// necessary and an improvement.
			ImagePixelT pixf = theimg->get0(fx, fy);
			ImagePixelT pixb = theimg->get0(bx, by);
			ImagePixelT pix = std::min(pixf, pixb);
			targetimg->set0(fx, fy, pix);
			targetimg->set0(bx, by, pix);

			// Set the "symm1sig" mask bit for pixels that have
			// been pulled down by the symmetry constraint, and
			// the "symm3sig" mask bit for pixels pulled down by 3
			// sigma.
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

    // Clip the result image to "tbb" (via this deep copy, ugh)
    MaskedImagePtrT rimg(new MaskedImageT(timg, sfoot->getBBox(), image::PARENT, true));

	/*
	 SpanList sp2 = sfoot->getSpans();
	 for (SpanList::const_iterator spit2 = sp2.begin(); spit2 != sp2.end(); spit2++) {
	 //outfoot.addSpan(*spit2);
	 outfoot.addSpan((*spit2)->getY(), (*spit2)->getX0(), (*spit2)->getX1());
	 }
	 */

	return std::pair<MaskedImagePtrT, FootprintPtrT>(rimg, sfoot);
    //return rimg;
}

/*
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
std::pair<boost::shared_ptr<lsst::afw::image::MaskedImage<ImagePixelT,MaskPixelT,VariancePixelT> >, lsst::afw::detection::Footprint>
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::buildSymmetricTemplate2(
	MaskedImageT const& img,
	det::Footprint const& foot,
	det::Peak const& peak,
	double sigma1) {
    det::Footprint fp;
    MaskedImagePtrT mi;
    return std::pair<MaskedImagePtrT, det::Footprint>(mi, fp);
}
 */

// Instantiate
template class deblend::BaselineUtils<float>;

/*
template class deblend::MaskedImageAndFootprint<float>;

template class std::pair<boost::shared_ptr<lsst::afw::image::MaskedImage<float> >, lsst::afw::detection::Footprint>;
 */
