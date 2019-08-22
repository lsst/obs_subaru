__all__ = []

import numpy as np
import astshim as ast

from lsst.geom import arcseconds
from lsst.afw.geom import transformRegistry, TransformPoint2ToPoint2
from lsst.pex.config import Config, Field, ListField


class HscDistortionConfig(Config):
    """Configuration for distortion calculation for HSC

    All values here are as used in the original distortion calculation library
    used for HSC, distEst.

    The coefficients come from a polynomial fit to the distortion using real
    HSC data.  They were computed in units of Focal Plane "pixels", which
    effectively assumes a scaling of 1 mm per Focal Plane pixel.  LSST prefers
    to use the proper (actual physical) scaling units, so the function
    makeHscDistortion() scales the coeffs from the original FP in "pixels"
    to their equivalent in the actual focal plane scale of HSC in mm.

    According to Yuki Okura, a 9th order polynomial is required to model the rapid changes
    to the distortion at the edges of the field.
    """
    ccdToSkyOrder = Field(dtype=int,
                          doc="Polynomial order for conversion from focal plane position to field angle",
                          default=9)
    xCcdToSky = ListField(dtype=float,
                          doc=("Coefficients for converting x,y focal plane position to x field angle "
                               "(computed assuming a focal plane scale of 1 mm/pixel)"),
                          default=[-0.00047203560468,
                                   -1.5427988883e-05,
                                   -5.95865625284e-10,
                                   2.22594651446e-13,
                                   -1.51583805989e-17,
                                   -6.08317204253e-21,
                                   8.06561200947e-26,
                                   3.10689047143e-29,
                                   -1.2634302141e-34,
                                   -4.78814991762e-38,
                                   -0.000210433037672 + 1.0,
                                   4.66142798084e-09,
                                   -1.0957097072e-10,
                                   -4.6906678848e-17,
                                   4.22216001121e-20,
                                   1.87541306804e-25,
                                   -3.11027701152e-28,
                                   -1.25424492312e-34,
                                   3.70673801604e-37,
                                   -6.85371734365e-09,
                                   -1.26203995922e-14,
                                   -5.4517702042e-17,
                                   -9.94551136649e-22,
                                   9.09298367647e-25,
                                   2.45558081417e-29,
                                   -1.92962666077e-33,
                                   -6.7994922883e-38,
                                   -1.04551853816e-10,
                                   -1.95257983584e-17,
                                   2.66068980901e-20,
                                   2.05500547863e-25,
                                   -7.00557780822e-28,
                                   -1.27106133492e-33,
                                   1.17427552052e-36,
                                   5.48308544761e-17,
                                   -6.6977079558e-21,
                                   -3.88079500249e-26,
                                   -1.12430203404e-29,
                                   -2.3228732246e-33,
                                   5.83996936659e-38,
                                   -2.55808760342e-20,
                                   -5.51546375606e-26,
                                   -4.43037636885e-28,
                                   3.26328244851e-34,
                                   1.30831113179e-36,
                                   -2.3965595459e-25,
                                   7.38850410175e-29,
                                   6.31709084288e-34,
                                   -5.69997824021e-38,
                                   -3.49962832225e-30,
                                   9.21035992064e-35,
                                   4.48010296471e-37,
                                   2.36522390769e-34,
                                   -1.7686068989e-37,
                                   -8.6880691822e-38,
                                   ])
    yCcdToSky = ListField(dtype=float,
                          doc=("Coefficients for converting x,y focal plane position to y field angle "
                               "(computed assuming a focal plane scale of 1 mm/pixel)"),
                          default=[-2.27525408678e-05,
                                   -0.000149438556393 + 1.0,
                                   1.47288649136e-09,
                                   -1.07681558891e-10,
                                   -4.52745194926e-17,
                                   5.33446374932e-21,
                                   1.59765278412e-25,
                                   -1.35281754124e-28,
                                   -2.58952055468e-34,
                                   1.18384181522e-37,
                                   -1.54279888831e-05,
                                   5.10149451107e-09,
                                   -2.20369366154e-12,
                                   -8.12440053288e-17,
                                   3.16674570469e-20,
                                   2.36720490323e-25,
                                   -1.54887554063e-28,
                                   -2.18878587707e-34,
                                   2.42019175449e-37,
                                   -1.36641013138e-09,
                                   -1.08210753878e-10,
                                   3.24065404366e-17,
                                   2.21741676333e-20,
                                   -3.30404486918e-25,
                                   -4.7223051146e-28,
                                   8.48620744583e-34,
                                   5.84549240581e-37,
                                   1.65013522193e-12,
                                   -2.04698537311e-16,
                                   1.36596617211e-20,
                                   8.77160647683e-25,
                                   -1.36731060152e-28,
                                   -1.43968368509e-33,
                                   4.00898492827e-37,
                                   -2.27951193984e-17,
                                   1.16796604208e-20,
                                   -6.53927583976e-25,
                                   -4.41168731276e-28,
                                   1.38404520921e-33,
                                   8.26267449077e-37,
                                   -1.75167734408e-20,
                                   1.35671719277e-24,
                                   -5.56167978363e-29,
                                   -2.43608580718e-33,
                                   9.3744233119e-38,
                                   8.31436843296e-26,
                                   -1.73569476217e-28,
                                   1.90770699097e-33,
                                   4.98143401516e-37,
                                   6.57627509385e-29,
                                   -2.64064071957e-33,
                                   1.56461570921e-37,
                                   -1.50783715462e-34,
                                   1.98549941035e-37,
                                   -8.74305862185e-38,
                                   ])
    tolerance = Field(dtype=float, default=5.0e-3, doc="Tolerance for inversion (pixels)")  # Much less than 1
    maxIter = Field(dtype=int, default=10, doc="Maximum iterations for inversion")  # Usually sufficient
    plateScale = Field(dtype=float, default=11.2, doc="Plate scale (arcsec/mm)")
    pixelSize = Field(dtype=float, default=0.015, doc="Pixel scale (mm/pixel)")


def makeAstPolyMapCoeffs(order, xCoeffs, yCoeffs):
    """Convert polynomial coefficients in HSC format to AST PolyMap format

    Paramaters
    ----------
    order: `int`
        Polynomial order
    xCoeffs, yCoeffs: `list` of `float`
        Forward or inverse polynomial coefficients for the x and y axes
        of output, in this order:
            x0y0, x0y1, ...x0yN, x1y0, x1y1, ...x1yN-1, ...
        where N is the polynomial order.

    Returns
    -------
    Forward or inverse coefficients for `astshim.PolyMap`
    as a 2-d numpy array.
    """
    nCoeffs = (order + 1) * (order + 2) // 2
    if len(xCoeffs) != nCoeffs:
        raise ValueError("found %s xCcdToSky params; need %s" % (len(xCoeffs), nCoeffs))
    if len(yCoeffs) != nCoeffs:
        raise ValueError("found %s yCcdToSky params; need %s" % (len(yCoeffs), nCoeffs))

    coeffs = np.zeros([nCoeffs * 2, 4])
    i = 0
    for nx in range(order + 1):
        for ny in range(order + 1 - nx):
            coeffs[i] = [xCoeffs[i], 1, nx, ny]
            coeffs[i + nCoeffs] = [yCoeffs[i], 2, nx, ny]
            i += 1
    assert i == nCoeffs
    return coeffs


def makeHscDistortion(config):
    """Make an HSC distortion transform

    Note that inverse coefficients provided, but they are not accurate enough
    to use: test_distortion.py reports an error of 2.8 pixels
    (HSC uses pixels for its focal plane units) when transforming
    from pupil to focal plane. That explains why the original HSC model uses
    the inverse coefficients in conjunction with iteration.

    Parameters
    ----------
    config: `lsst.obs.subaru.HscDistortionConfig`
        Distortion coefficients

    Returns
    -------
    focalPlaneToPupil: `lsst.afw.geom.TransformPoint2ToPoint2`
        Transform from focal plane to field angle coordinates
    """
    forwardCoeffs = makeAstPolyMapCoeffs(config.ccdToSkyOrder, config.xCcdToSky, config.yCcdToSky)

    # Convert coefficients from assumed 1 mm plateScale to actual 11.2 plateScale
    pixelScale = config.plateScale*config.pixelSize*arcseconds  # in arcsec/pixel
    for coeff in forwardCoeffs:
        coeff[0] = coeff[0]/(config.pixelSize**(coeff[2] + coeff[3]))

    # Note that the actual error can be somewhat larger than TolInverse;
    # the max error I have seen is less than 2, so I scale conservatively
    ccdToSky = ast.PolyMap(forwardCoeffs, 2, "IterInverse=1, TolInverse=%s, NIterInverse=%s" %
                           (config.tolerance / 2.0, config.maxIter))
    fullMapping = ccdToSky.then(ast.ZoomMap(2, pixelScale.asRadians()))
    return TransformPoint2ToPoint2(fullMapping)


makeHscDistortion.ConfigClass = HscDistortionConfig
transformRegistry.register("hsc", makeHscDistortion)
