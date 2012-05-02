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

    virtual afw::geom::LinearTransform computeQuadrupoleTransform(afw::geom::Point2D const &p0,
                                                                  bool forward) const {
        afw::geom::Point2D const& p1 = p0 + afw::geom::Extent2D(1, 1);
        afw::geom::Extent2D const& diff = _transform(p1, forward) - _transform(p0, forward);

        return afw::geom::LinearTransform::LinearTransform::makeScaling(diff.getX(), diff.getY());
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
