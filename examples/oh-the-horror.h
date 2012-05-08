#include <utility>
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/detection.h"

template <typename ImagePixelT,
	typename MaskPixelT=lsst::afw::image::MaskPixel,
	typename VariancePixelT=lsst::afw::image::VariancePixel>
	class Horror {
	public:

	typedef typename lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr MaskedImagePtrT;
	typedef typename lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> MaskedImageT;
	typedef typename lsst::afw::detection::Footprint FootprintT;
	typedef typename lsst::afw::detection::Footprint::Ptr FootprintPtrT;

	static
	lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>
    test1() {
		return lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>();
	}

	static
	typename lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr
    test2() {
		return typename lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr(
			new lsst::afw::image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>());
	}

	static
	MaskedImageT
    test3() {
		return MaskedImageT();
	}

	static
	MaskedImagePtrT
    test4() {
		return MaskedImagePtrT(new MaskedImageT());
	}

	static
	std::pair<MaskedImagePtrT, FootprintPtrT>
    test5() {
		return std::pair<MaskedImagePtrT, FootprintPtrT>(
			MaskedImagePtrT(new MaskedImageT()),
			FootprintPtrT(new FootprintT));
	}

};
