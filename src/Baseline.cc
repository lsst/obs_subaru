
#include "lsst/meas/deblender/Baseline.h"

namespace image = lsst::afw::image;
namespace det = lsst::afw::detection;
namespace deblend = lsst::meas::deblender;

/*
template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>::Ptr
buildSymmetricTemplate(
	image::MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT> const& img,
	det::Footprint const& foot,
	det::Peak const& pk) {
	
}
 */

template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
typename deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::MaskedImageT::Ptr
deblend::BaselineUtils<ImagePixelT,MaskPixelT,VariancePixelT>::buildSymmetricTemplate(
	//deblend::BaselineUtils::MaskedImageT const& img,
	MaskedImageT const& img,
	det::Footprint const& foot,
	det::Peak const& peak) {

	MaskedImagePtrT rtn(new MaskedImageT(img, true));
	return rtn;
}


// Instantiate
template class deblend::BaselineUtils<float>;

