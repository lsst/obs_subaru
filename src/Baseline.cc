
#include "lsst/meas/deblender/Baseline.h"
#include "lsst/afw/geom/Box.h"

#include "lsst/meas/algorithms/detail/SdssShape.h"

namespace image = lsst::afw::image;
namespace det = lsst::afw::detection;
namespace deblend = lsst::meas::deblender;
namespace geom = lsst::afw::geom;
namespace malg = lsst::meas::algorithms;

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
        for (typename MaskedImageT::x_iterator optr = out.row_begin(y) + halfsize; inpix != end; ++inpix.x(), ++optr) {
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
typename deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::MaskedImagePtrT
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::buildSymmetricTemplate(
	MaskedImageT const& img,
	det::Footprint const& foot,
	det::Peak const& peak,
	double sigma1) {

    //typedef ImageT::const_xy_locator xy_loc;
	typedef typename MaskedImageT::const_xy_locator xy_loc;
	typedef typename det::Footprint::SpanList SpanList;

	geom::Box2I fbb = foot.getBBox();
	MaskedImagePtrT timg(new MaskedImageT(fbb.getDimensions()));
	timg->setXY0(fbb.getMinX(), fbb.getMinY());

	MaskPixelT symm1sig = img.getMask()->getPlaneBitMask("SYMM_1SIG");
	MaskPixelT symm3sig = img.getMask()->getPlaneBitMask("SYMM_3SIG");

	// Copy the image under the footprint.
	const SpanList spans = foot.getSpans();
	/*
	int inx0 = img.getX0();
	int iny0 = img.getY0();
	int outx0 = timg->getX0();
	int outy0 = timg->getY0();
	for (SpanList::const_iterator sp = spans.begin(); sp != spans.end(); ++sp) {
		int iy = (*sp)->getY();
		int ix0 = (*sp)->getX0();
		int ix1 = (*sp)->getX1();
		typename MaskedImageT::x_iterator inptr = img.row_begin(iy - iny0);
		typename MaskedImageT::x_iterator inend = inptr;
		inptr += ix0 - inx0;
		inend += ix1 - inx0 + 1;
		typename MaskedImageT::x_iterator outptr = timg->row_begin(iy - outy0) + (ix0 - outx0);
		for (; inptr != inend; ++inptr, ++outptr)
			*outptr = *inptr;
	}
	 */

	int cx,cy;
	cx = peak.getIx();
	cy = peak.getIy();

	// Find the Span containing the peak.
	det::Span::Ptr target(new det::Span(cy, cx, cx));
	//SpanList::const_iterator peakspan = std::lower_bound(spans.begin(), spans.end(), target);
	SpanList::const_iterator peakspan = std::lower_bound(spans.begin(), spans.end(), target, span_ptr_compare);

	// lower_bound returns the first position where "target" could be inserted;
	// ie, the first Span larger than "target".  The Span containing "target"
	// should be peakspan-1.
	// "Returns the furthermost Span i such that, for every Span j in
	//  [first, i), *j < target."
	if (peakspan == spans.begin()) {
		det::Span::Ptr sp = *peakspan;
		assert(!sp->contains(cx,cy));
		printf("Span containing peak not found.\n");
		assert(0);
	}
	peakspan--;
	det::Span::Ptr sp = *peakspan;
	assert(sp->contains(cx,cy));
	// printf("Span containing peak (%i,%i): (x=[%i,%i], y=%i)\n", cx, cy, sp->getX0(), sp->getX1(), sp->getY());

	SpanList::const_iterator fwd = peakspan;
	SpanList::const_iterator back = peakspan;
	int dy = 0;

	/*
	 For every pixel "pix" inside the Footprint:

	 If the symmetric pixel "symm" is inside the Footprint:
	 -set some Mask bit?
	 -set "pix" and "symm" to their min

	 If the symmetric pixel "symm" is outside the Footprint:
	 -set some Mask bit?
	 -copy "pix" from the input.
	 */
	const SpanList::const_iterator s0 = spans.begin();

	// printf("spans.begin: %i, end %i\n", (int)(spans.begin()-s0), (int)(spans.end()-s0));
	// printf("peakspan: %i\n", (int)(peakspan-s0));

	/*
	 For every pixel that has a symmetric partner,
	 set both pixels to their min.
	 */

	while ((fwd < spans.end()) && (back >= s0)) {
		// For row "dy", find the range of x values?
		// printf("dy = %i\n", dy);
		//SpanList::const_iterator f0 = fwd;
		SpanList::const_iterator f1 = fwd;
		int fy = cy + dy;
		int fx0 = (*fwd)->getX0();
		int fx1 = -1 ;// = (*fwd)->getX1();
		for (; f1 != spans.end(); ++f1) {
			if ((*f1)->getY() != fy)
				break;
			fx1 = (*f1)->getX1();
		}
		assert(fx1 != -1);
		// printf("fwd: %i\n", (int)(fwd-s0));
		// printf("f0:  %i\n", (int)(f0-s0));
		// printf("f1:  %i\n", (int)(f1-s0));

		SpanList::const_iterator b0 = back;
		//SpanList::const_iterator b1 = back;
		int by = cy - dy;
		int bx0 = -1; // = (*back)->getX0();
		int bx1 = (*back)->getX1();
		for (; b0 >= s0; b0--) {
			if ((*b0)->getY() != by)
				break;
			bx0 = (*b0)->getX0();
		}
		assert(bx0 != -1);
		// printf("back: %i\n", (int)(back-s0));
		// printf("b0:   %i\n", (int)(b0-s0));
		// printf("b1:   %i\n", (int)(b1-s0));

		SpanList::const_iterator sp;
		/*
		 for (sp=f0; sp<f1; ++sp)
		 printf("  forward: %i, x = [%i, %i], y = %i\n", (int)(sp-s0), (*sp)->getX0(), (*sp)->getX1(), (*sp)->getY());
		 for (sp=b1; sp>b0; --sp)
		 printf("  back   : %i, x = [%i, %i], y = %i\n", (int)(sp-s0), (*sp)->getX0(), (*sp)->getX1(), (*sp)->getY());
		 */

		//printf("dy=%i: fy=%i, fx=[%i, %i].  by=%i, bx=[%i, %i]\n", dy, fy, fx0, fx1, by, bx0, bx1);

		int dx0, dx1;
		dx0 = std::max(fx0 - cx, cx - bx1);
		dx1 = std::min(fx1 - cx, cx - bx0);
		//printf("dx = [%i, %i]  --> forward [%i, %i], back [%i, %i]\n", dx0, dx1, cx+dx0, cx+dx1, cx-dx1, cx-dx0);

		for (int dx=dx0; dx<=dx1; dx++) {
			int fx = cx + dx;
			int bx = cx - dx;
			//printf("  dx = %i,  fx=%i, bx=%i\n", dx, fx, bx);

			// We test all dx values within the valid range, but we
			// have to be a bit careful since there may be gaps in one
			// or both directions.

			if ((fwd != f1) && (fx > (*fwd)->getX1()))
				fwd++;
			if ((back != b0) && (bx < (*back)->getX0()))
				back--;

			// They shouldn't both be "done".
			if (fwd == f1) {
				assert(back != b0);
			}
			if (back == b0) {
				assert(fwd != f1);
			}

			/*
			 if (fwd != f1)
			 printf("    fwd : y=%i, x=[%i,%i]\n", (*fwd)->getY(), (*fwd)->getX0(), (*fwd)->getX1());
			 else
			 printf("    fwd : done\n");
			 if (back != b0)
			 printf("    back: y=%i, x=[%i,%i]\n", (*back)->getY(), (*back)->getX0(), (*back)->getX1());
			 else
			 printf("    back: done\n");
			 */

			// HACK -- MaskedPixel...
			ImagePtrT theimg = img.getImage();
			ImagePtrT targetimg = timg->getImage();
			MaskPtrT targetmask = timg->getMask();
			// FIXME -- one of these conditions is redundant...
			if ((fwd != f1) && (*fwd)->contains(fx) && (back != b0) && (*back)->contains(bx)) {
				ImagePixelT pixf = theimg->get0(fx, fy);
				ImagePixelT pixb = theimg->get0(bx, by);
				ImagePixelT pix = std::min(pixf, pixb);
				targetimg->set0(fx, fy, pix);
				targetimg->set0(bx, by, pix);
				// Set the "symm1sig" mask bit for pixels that have been pulled down by the
				// symmetry constraint.
				if (pixf >= pix + 1.*sigma1)
					targetmask->set0(fx, fy, targetmask->get0(fx, fy) | symm1sig);
				if (pixb >= pix + 1.*sigma1)
					targetmask->set0(bx, by, targetmask->get0(bx, by) | symm1sig);

				if (pixf >= pix + 3.*sigma1)
					targetmask->set0(fx, fy, targetmask->get0(fx, fy) | symm3sig);
				if (pixb >= pix + 3.*sigma1)
					targetmask->set0(bx, by, targetmask->get0(bx, by) | symm3sig);
				

			}
			// else: gap in both the forward and reverse directions.
		}

		//printf("fwd : %i.  f0=%i, f1=%i\n", (int)(fwd-s0), (int)(f0-s0), (int)(f1-s0));
		//printf("back: %i.  b0=%i, b1=%i\n", (int)(back-s0), (int)(b0-s0), (int)(b1-s0));

		fwd = f1;
		back = b0;
		dy++;
	}

	return timg;
}


// Instantiate
template class deblend::BaselineUtils<float>;

