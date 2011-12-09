
#include "lsst/meas/deblender/Baseline.h"
#include "lsst/afw/geom/Box.h"

namespace image = lsst::afw::image;
namespace det = lsst::afw::detection;
namespace deblend = lsst::meas::deblender;
namespace geom = lsst::afw::geom;

bool span_ptr_compare(det::Span::Ptr sp1, det::Span::Ptr sp2) {
	return (*sp1 < *sp2);
}



template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
typename deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::MaskedImageT::Ptr
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::buildSymmetricTemplate(
	MaskedImageT const& img,
	det::Footprint const& foot,
	det::Peak const& peak) {

	typedef typename image::Image<ImagePixelT> ImageT;
	typedef typename image::Image<ImagePixelT>::Ptr ImagePtrT;
    //typedef ImageT::const_xy_locator xy_loc;
	typedef typename MaskedImageT::const_xy_locator xy_loc;

	typedef typename det::Footprint::SpanList SpanList;

	///// FIXME -- this should be the size of the FOOTPRINT,
	// not the IMAGE.
 	//MaskedImagePtrT timg(new MaskedImageT(img.getDimensions()));
	//timg->setXY0(img.getXY0());
	geom::Box2I fbb = foot.getBBox();
	MaskedImagePtrT timg(new MaskedImageT(fbb.getDimensions()));
	timg->setXY0(fbb.getMinX(), fbb.getMinY());

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

	/*
	 xy_loc pix = img.xy_at(cx, cy);
	 xy_loc::cached_location_t nw = pix.cache_location(-1,-1);
	 xy_loc::cached_location_t n  = pix.cache_location( 0,-1);
	 xy_loc::cached_location_t ne = pix.cache_location( 1,-1);
	 xy_loc::cached_location_t w  = pix.cache_location(-1, 0);
	 xy_loc::cached_location_t c  = pix.cache_location( 0, 0);
	 xy_loc::cached_location_t e  = pix.cache_location( 1, 0);
	 xy_loc::cached_location_t sw = pix.cache_location(-1, 1);
	 xy_loc::cached_location_t s  = pix.cache_location( 0, 1);
	 xy_loc::cached_location_t se = pix.cache_location( 1, 1);
	 */

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
	printf("Span containing peak (%i,%i): (x=[%i,%i], y=%i)\n", cx, cy, sp->getX0(), sp->getX1(), sp->getY());

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

	printf("spans.begin: %i, end %i\n", (int)(spans.begin()-s0), (int)(spans.end()-s0));
	printf("peakspan: %i\n", (int)(peakspan-s0));

	/*
	 For every pixel that has a symmetric partner,
	 set both pixels to their min.
	 */

	while ((fwd < spans.end()) && (back >= s0)) {
		// For row "dy", find the range of x values?
		printf("dy = %i\n", dy);
		SpanList::const_iterator f0 = fwd;
		SpanList::const_iterator f1 = fwd;
		int fy = cy + dy;
		int fx0 = (*fwd)->getX0();
		int fx1;// = (*fwd)->getX1();
		for (; f1 != spans.end(); ++f1) {
			if ((*f1)->getY() != fy)
				break;
			fx1 = (*f1)->getX1();
		}
		printf("fwd: %i\n", (int)(fwd-s0));
		printf("f0:  %i\n", (int)(f0-s0));
		printf("f1:  %i\n", (int)(f1-s0));

		SpanList::const_iterator b0 = back;
		SpanList::const_iterator b1 = back;
		int by = cy - dy;
		int bx0;// = (*back)->getX0();
		int bx1 = (*back)->getX1();
		for (; b0 >= s0; b0--) {
			if ((*b0)->getY() != by)
				break;
			bx0 = (*b0)->getX0();
		}
		printf("back: %i\n", (int)(back-s0));
		printf("b0:   %i\n", (int)(b0-s0));
		printf("b1:   %i\n", (int)(b1-s0));

		SpanList::const_iterator sp;
		for (sp=f0; sp<f1; ++sp)
			printf("  forward: %i, x = [%i, %i], y = %i\n", (int)(sp-s0), (*sp)->getX0(), (*sp)->getX1(), (*sp)->getY());
		for (sp=b1; sp>b0; --sp)
			printf("  back   : %i, x = [%i, %i], y = %i\n", (int)(sp-s0), (*sp)->getX0(), (*sp)->getX1(), (*sp)->getY());

		printf("dy=%i: fy=%i, fx=[%i, %i].  by=%i, bx=[%i, %i]\n",
			   dy, fy, fx0, fx1, by, bx0, bx1);

		int dx0, dx1;
		dx0 = std::max(fx0 - cx, cx - bx1);
		dx1 = std::min(fx1 - cx, cx - bx0);
		printf("dx = [%i, %i]  --> forward [%i, %i], back [%i, %i]\n",
			   dx0, dx1, cx+dx0, cx+dx1, cx-dx1, cx-dx0);

		for (int dx=dx0; dx<=dx1; dx++) {
			int fx = cx + dx;
			int bx = cx - dx;
			printf("  dx = %i,  fx=%i, bx=%i\n", dx, fx, bx);

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

			if (fwd != f1)
				printf("    fwd : y=%i, x=[%i,%i]\n", (*fwd)->getY(), (*fwd)->getX0(), (*fwd)->getX1());
			else
				printf("    fwd : done\n");
			if (back != b0)
				printf("    back: y=%i, x=[%i,%i]\n", (*back)->getY(), (*back)->getX0(), (*back)->getX1());
			else
				printf("    back: done\n");

			// HACK -- MaskedPixel...
			ImagePtrT theimg = img.getImage();
			ImagePtrT targetimg = timg->getImage();
			// FIXME -- one of these conditions is redundant...
			if ((fwd != f1) && (*fwd)->contains(fx) && (back != b0) && (*back)->contains(bx)) {
				ImagePixelT pix = std::min(theimg->get0(fx, fy), theimg->get0(bx,by));
				targetimg->set0(fx, fy, pix);
				targetimg->set0(bx, by, pix);
				/*
				 } else if ((fwd != f1) && (*fwd)->contains(fx)) {
				 targetimg->set0(fx, fy, theimg->get0(fx, fy));
				 } else if ((back != b0) && (*back)->contains(bx)) {
				 targetimg->set0(bx, by, theimg->get0(bx, by));
				 */
			}
			// else: gap in both the forward and reverse directions.
		}

		printf("fwd : %i.  f0=%i, f1=%i\n", (int)(fwd-s0), (int)(f0-s0), (int)(f1-s0));
		printf("back: %i.  b0=%i, b1=%i\n", (int)(back-s0), (int)(b0-s0), (int)(b1-s0));

		fwd = f1;
		back = b0;
		dy++;
	}




	//for (det::SpanList::iterator it=spans.begin(); 


	/*
	 for (int y = 1; y != in.getHeight() - 1; ++y) {
	 // "dot" means "cursor location" in emacs
	 xy_loc dot = in.xy_at(1, y), end = in.xy_at(in.getWidth() - 1, y); 
	 for (ImageT::x_iterator optr = out2->row_begin(y) + 1; dot != end; ++dot.x(), ++optr) {
	 *optr = dot[nw] + 2*dot[n] +   dot[ne] +
	 2*dot[w]  + 4*dot[c] + 2*dot[e] +
	 dot[sw] + 2*dot[s] +   dot[se];
	 }
	 }
	 */


	return timg;
}


// Instantiate
template class deblend::BaselineUtils<float>;

