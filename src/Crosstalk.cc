#include "lsst/obs/subaru/Crosstalk.h"
#include "lsst/afw/image/MaskedImage.h"
//#include "lsst/afw/geom/Point.h"

// C++ implementation of crosstalk correction with procedure by Yagi+2012

//namespace geom = lsst::afw::geom;
namespace image = lsst::afw::image;

int nAmp = 4;

std::vector<int> lsst::obs::subaru::getCrosstalkX1(int x, int nxAmp = 512) {
    // Return the primary X positions in CCD which affect the count of input x pixel
    // x     :  xpos in Ccd, affected by the returned pixels
    // nxAmp :  nx per amp

    std::vector<int> ctx1List(nAmp);
    ctx1List.at(0) = x;
    ctx1List.at(1) = 2 * nxAmp - x - 1;
    ctx1List.at(2) = 2 * nxAmp + x;
    ctx1List.at(3) = 4 * nxAmp - x - 1;

    return ctx1List;
}

std::vector<int> lsst::obs::subaru::getCrosstalkX2(int x, int nxAmp = 512) {
    // Return the 2ndary X positions (dX=2) in CCD which affect the count of input x pixel
    //      Those X pixels are read by 2-pixel ealier than the primary pixels.
    // x     :  xpos in Ccd, affected by the returned pixels
    // nxAmp :  nx per amp

    // ch0,ch2: ctx2 = ctx1 - 2,  ch1,ch3: ctx2 = ctx1 + 2
    std::vector<int>  ctx2List(nAmp);
    ctx2List.at(0) =  x - 2;
    ctx2List.at(1) = 2 * nxAmp - x - 1 + 2;
    ctx2List.at(2) = 2 * nxAmp + x - 2;
    ctx2List.at(3) = 4 * nxAmp - x - 1 + 2;

    return ctx2List;
}

void lsst::obs::subaru::subtractCrosstalk(
    image::MaskedImage<float> & mi, 
                       int nAmp,
                       std::vector< std::vector<double> > const & coeffs1List,
                       std::vector< std::vector<double> > const & coeffs2List,
                       std::vector<double> const & gainsPreampSig
    )
{


    int nx = mi.getWidth();
    int ny = mi.getHeight();
    int nxAmp = nx / nAmp;

    image::Image<float> img0 = image::Image<float>(*(mi.getImage()), true); // create a copy of image before correction
    image::Image<float>::Ptr img = mi.getImage();

    for(int j = 0; j < ny; ++j) {

        for( int i = 2; i < nxAmp; ++i) {

            std::vector<int> ctx1List = getCrosstalkX1(i, nxAmp=nxAmp);
            std::vector<int> ctx2List = getCrosstalkX2(i, nxAmp=nxAmp);

            std::vector<double> crosstalkOffsets(nAmp, 0.0);

            for( int ch1 = 0; ch1 < nAmp; ++ch1 ) { // channel of pixel being affected

                std::vector<double> coeffs1 = coeffs1List.at(ch1); // array of [[A-D]->A, [A-D]->B, [A-D]->C, [A-D]->D]
                std::vector<double> coeffs2 = coeffs2List.at(ch1);

                // estimation of crosstalk amounts
                for( int ch2 = 0; ch2 < nAmp; ++ch2 ) { // channel of pixel affecting the pixel in ch1

                    int ctx1 = ctx1List.at(ch2);
                    int ctx2 = ctx2List.at(ch2);
                    double v1 = img0(ctx1, j);
                    double v2 = img0(ctx2, j);

                    //std::cout << "ceffs1[" << ch2 << "]:" << coeffs1.at(ch2) << " v1:" << v1;
                    crosstalkOffsets.at(ch1) += (coeffs1.at(ch2) * v1 + coeffs2.at(ch2) * v2) * gainsPreampSig.at(ch2) / gainsPreampSig.at(ch1);
                }

                // correction of crosstalk
                int x = ctx1List.at(ch1);
                int y = j;

                // print >> sys.stderr, "correction: ch=%d x=%8.2f y=%8.2f" % (ch1, x, y)
                (*img)(x, y) -= static_cast<float>(crosstalkOffsets.at(ch1));

            }
        }
    }

}
