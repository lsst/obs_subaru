#ifndef LSST_OBS_HSCSIM_HSC_DISTORTION_H
#define LSST_OBS_HSCSIM_HSC_DISTORTION_H


#include "lsst/afw/cameraGeom.h"

/*
 * @file HscDistortion.h
 * @brief Provide interface class for HSC "distEst" package
 * @ingroup obs_subaru
 * @author Paul Price
 *
 */


namespace lsst {
namespace obs {
namespace hscSim {

/**
 * @class Interface class for HSC "distEst" distortion
 */
class HscDistortion : public afw::cameraGeom::Distortion {
public:
    HscDistortion(afw::geom::Angle const& elevation, bool iterative=true) :
        afw::cameraGeom::Distortion(), _elevation(elevation), _iterative(iterative) {}
    virtual afw::geom::LinearTransform computePointTransform(afw::geom::Point2D const &p,
                                                             bool forward) const {
        afw::geom::Point2D const& transformed = _transform(p, forward);
        return afw::geom::LinearTransform::makeScaling(transformed.getX() / p.getX(),
                                                       transformed.getY() / p.getY());
    }
    /// Estimate the LinearTransform approximating the distortion at a point
    virtual afw::geom::LinearTransform computeQuadrupoleTransform(
        afw::geom::Point2D const &_p00,  ///< The point at which we want the Quadrupole
        bool forward                    ///< true if we want to apply the distortion
                                                                 ) const {
        //
        // We want to know the mapping from (x, y) -> (X, Y)
        // We'll estimate the needed derivatives (dX/dy etc.) numerically
        //
        double const eps = 1.0;         // step for numerical derivative; assumes were working in pix ~ 10^4
        afw::geom::Point2D const& p00 = _p00 - afw::geom::Extent2D(0.5*eps, 0.5*eps); // center the differences
        afw::geom::Point2D const& p01 = p00 + afw::geom::Extent2D(0, eps);
        afw::geom::Point2D const& p10 = p00 + afw::geom::Extent2D(eps, 0);
        afw::geom::Extent2D const& diff01 = (_transform(p01, forward) - _transform(p00, forward))/eps;
        afw::geom::Extent2D const& diff10 = (_transform(p10, forward) - _transform(p00, forward))/eps;

        return afw::geom::LinearTransform((afw::geom::LinearTransform::Matrix() <<
                                           diff10.getX(), diff10.getY(), diff01.getX(), diff01.getY()
                                          ).finished());
    }

    virtual std::string prynt() const { return std::string("HscDistortion derived class"); }

private:
    /// Transform a position
    afw::geom::Point2D _transform(
        afw::geom::Point2D const& p,    /// Position to transform
        bool forward                    /// Transform is forward?
        ) const;

    afw::geom::Angle _elevation;     // Telescope elevation for distortion model
    bool _iterative;                 // Use iterative solution?
};

}}} // namespace

#endif
