#include <cassert>

#include "boost/format.hpp"
#include "lsst/pex/exceptions.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/obs/subaru/Crosstalk.h"

// C++ implementation of crosstalk correction with procedure by Yagi+2012

namespace image = lsst::afw::image;

typedef std::vector<double> PixelVector;

namespace {

int nAmp = 4;

PixelVector getCrosstalkX1(int x, int nxAmp = 512) {
    // Return the primary X positions in CCD which affect the count of input x pixel
    // x     :  xpos in Ccd, affected by the returned pixels
    // nxAmp :  nx per amp

    PixelVector ctx1List(nAmp);
    ctx1List[0] = x;
    ctx1List[1] = 2 * nxAmp - x - 1;
    ctx1List[2] = 2 * nxAmp + x;
    ctx1List[3] = 4 * nxAmp - x - 1;

    return ctx1List;
}

PixelVector getCrosstalkX2(int x, int nxAmp = 512) {
    // Return the 2ndary X positions (dX=2) in CCD which affect the count of input x pixel
    //      Those X pixels are read by 2-pixel ealier than the primary pixels.
    // x     :  xpos in Ccd, affected by the returned pixels
    // nxAmp :  nx per amp

    // ch0,ch2: ctx2 = ctx1 - 2,  ch1,ch3: ctx2 = ctx1 + 2
    PixelVector ctx2List(nAmp);
    ctx2List[0] =  x - 2;
    ctx2List[1] = 2 * nxAmp - x - 1 + 2;
    ctx2List[2] = 2 * nxAmp + x - 2;
    ctx2List[3] = 4 * nxAmp - x - 1 + 2;

    return ctx2List;
}

template<typename T>
void checkSize(std::vector<T> const& vector, std::size_t expected, std::string const& description)
{
    if (vector.size() != expected) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthError,
                          (boost::format("%s size (%d) is not %d") %
                           description % vector.size() % expected).str());
    }
}


} // anonymous namespace

void lsst::obs::subaru::subtractCrosstalk(
    image::MaskedImage<float> & mi,
    std::size_t nAmp,
    std::vector<CoeffVector> const & coeffs1List,
    std::vector<CoeffVector> const & coeffs2List,
    CoeffVector const & gainsPreampSig
    )
{
    typedef image::Image<float> Image;

    std::size_t nx = mi.getWidth();
    std::size_t ny = mi.getHeight();
    std::size_t nxAmp = nx / nAmp;

    if (nAmp != 4) {
        throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterError,
                          (boost::format("nAmp (%d) is not 4") % nAmp).str());
    }
    checkSize(coeffs1List, nAmp, "coeffs1List");
    checkSize(coeffs2List, nAmp, "coeffs2List");
    checkSize(gainsPreampSig, nAmp, "gainsPreampSig");
    for (std::size_t i = 0; i < nAmp; ++i) {
        checkSize(coeffs1List[i], nAmp, (boost::format("coeffs1List[%d]") % i).str());
        checkSize(coeffs2List[i], nAmp, (boost::format("coeffs2List[%d]") % i).str());
    }

    Image img0 = Image(*(mi.getImage()), true); // create a copy of image before correction
    PTR(Image) img = mi.getImage();

    for (std::size_t j = 0; j < ny; ++j) {

        for (std::size_t i = 2; i < nxAmp; ++i) {

            PixelVector const& ctx1List = getCrosstalkX1(i, nxAmp);
            PixelVector const& ctx2List = getCrosstalkX2(i, nxAmp);
            assert(ctx1List.size() == nAmp);
            assert(ctx2List.size() == nAmp);
            CoeffVector crosstalkOffsets(nAmp, 0.0);

            for (std::size_t ch1 = 0; ch1 < nAmp; ++ch1) { // channel of pixel being affected

                CoeffVector coeffs1 = coeffs1List[ch1]; // array of [[A-D]->A, [A-D]->B, [A-D]->C, [A-D]->D]
                CoeffVector coeffs2 = coeffs2List[ch1];

                // estimation of crosstalk amounts
                for (std::size_t ch2 = 0; ch2 < nAmp; ++ch2 ) { // channel of pixel affecting the pixel in ch1

                    int ctx1 = ctx1List[ch2];
                    int ctx2 = ctx2List[ch2];
                    double v1 = img0(ctx1, j);
                    double v2 = img0(ctx2, j);

                    //std::cout << "ceffs1[" << ch2 << "]:" << coeffs1.at(ch2) << " v1:" << v1;
                    crosstalkOffsets[ch1] += (v1*coeffs1[ch2] + v2*coeffs2[ch2]) *
                        gainsPreampSig[ch2] / gainsPreampSig[ch1];
                }

                // correction of crosstalk
                int x = ctx1List[ch1];
                int y = j;

                (*img)(x, y) -= static_cast<float>(crosstalkOffsets[ch1]);

            }
        }
    }

}
