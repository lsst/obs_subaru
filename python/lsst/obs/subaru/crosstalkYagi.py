#
# LSST Data Management System
# Copyright 2008, 2009, 2010, 2011 LSST Corporation.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <http://www.lsstcorp.org/LegalNotices/>.
#
"""
Determine and apply crosstalk corrections

N.b. This code was written and tested for the 4-amplifier Hamamatsu chips used in (Hyper)?SuprimeCam,
and will need to be generalised to handle other amplifier layouts.  I don't want to do this until we
have an example.

N.b. To estimate crosstalk from the SuprimeCam data, the commands are e.g.:
import crosstalk
coeffs = crosstalk.estimateCoeffs(range(131634, 131642), range(10), threshold=1e5,
                                  plot=True, title="CCD0..9", fig=1)
crosstalk.fixCcd(131634, 0, coeffs)
"""
from __future__ import absolute_import, division, print_function

from builtins import range
import numpy as np
import lsst.afw.detection as afwDetect
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.display.ds9 as ds9
import lsst.pipe.base as pipeBase
import lsst.pex.config as pexConfig
from lsst.obs.subaru import subtractCrosstalk
from lsst.obs.subaru.crosstalk import makeList, estimateCoeffs, printCoeffs, readImage, subtractXTalk


class CrosstalkYagiCoeffsConfig(pexConfig.Config):
    """Specify crosstalk coefficients for a CCD"""

    crossTalkCoeffs1 = pexConfig.ListField(
        dtype=float,
        doc="crosstalk coefficients (primary by the high-count pixel)",
        default=[0, -0.000148, -0.000162, -0.000167,   # cAA,cAB,cAC,cAD
                 -0.000148, 0, -0.000077, -0.000162,     # cBA,cBB,cBC,cBD
                 -0.000162, -0.000077, 0, -0.000148,     # cCA,cCB,cCC,cCD
                 -0.000167, -0.000162, -0.000148, 0, ], # cDA,cDB,cDC,cDD
    )
    crossTalkCoeffs2 = pexConfig.ListField(
        dtype=float,
        doc="crosstalk coefficients (secondary by the high-count pixel + 2pix)",
        default=[0, 0.000051, 0.000050, 0.000053,
                 0.000051, 0, 0, 0.000050,
                 0.000050, 0, 0, 0.000051,
                 0.000053, 0.000050, 0.000051, 0, ],
    )

    relativeGainsPreampAndSigboard = pexConfig.ListField(
        dtype=float,
        doc="effective gain of combination of SIG board + preAmp for CCD, relatve to chB@CCD=5. \
               g2*g3 in Yagi+2012",
        default=[0.949, 0.993, 0.976, 0.996,
                 0.973, 0.984, 0.966, 0.977,
                 1.008, 0.989, 0.970, 0.976,
                 0.961, 0.966, 1.008, 0.967,
                 0.967, 0.984, 0.998, 1.000,
                 0.989, 1.000, 1.034, 1.030,
                 0.957, 1.019, 0.952, 0.979,
                 0.974, 1.015, 0.967, 0.962,
                 0.972, 0.932, 0.999, 0.963,
                 0.987, 0.985, 0.986, 1.012, ],
    )
    relativeGainsTotalBeforeOct2010 = pexConfig.ListField(
        dtype=float,
        doc="total effective gain of SIG board + preAmp + CCD on-chip amp, relatve to chB@CCD=5. \
               effective before Oct 2010  g1*g2*g3 in Yagi+2012",
        default=[1.0505, 1.06294, 1.08287, 1.06868,
                 1.06962, 1.13801, 1.21669, 1.21205,
                 0.99456, 1.01931, 1.038, 0.947461,
                 1.02245, 1.03952, 1.04192, 1.1544,
                 1.0055, 1.12767, 1.08451, 1.03309,
                 1.01532, 1.00000, 0.972106, 1.08237,
                 1.19014, 0.987111, 0.984164, 1.03382,
                 0.971049, 1.05928, 1.06713, 0.980967,
                 1.0107, 1.25209, 1.27565, 1.17183,
                 0.993771, 1.077, 1.03909, 1.03241, ],
    )

    relativeGainsTotalAfterOct2010 = pexConfig.ListField(
        dtype=float,
        doc="total effective gain of SIG board + preAmp + CCD on-chip amp, relatve to chB@CCD=5. \
               effective after Oct 2010  g1*g2*g3 in Yagi+2012",
        default=[1.0505, 1.06294, 1.08287, 1.06868,
                 1.06962, 1.13801, 1.21669, 1.21205,
                 0.99456, 1.01931, 1.038, 0.947461,
                 1.02245, 1.03952, 1.04192, 1.1544,
                 1.0055, 1.12767, 1.08451, 1.03309,
                 1.01532, 1.00000, 0.972106, 1.08237,
                 1.19014, 0.987111, 0.984164, 1.03382,
                 0.971049, 1.05928, 1.06713, 0.980967,
                 1.0107, 1.25209, 1.27565, 1.17183,
                 1.13003, 1.077, 1.03909, 1.03241, ],
    )

    shapeGainsArray = pexConfig.ListField(
        dtype=int,
        doc="Shape of gain arrays",
        default=[10, 4], # 10 CCDs and 4 channels per CCD
        minLength=1,                  # really 2, but there's a bug in pex_config
        maxLength=2,
    )

    shapeCoeffsArray = pexConfig.ListField(
        dtype=int,
        doc="Shape of coeffs arrays",
        default=[4, 4], # 4 channels and 4 mirror-symmetry patterns per channel
        minLength=1,                  # really 2, but there's a bug in pex_config
        maxLength=2,
    )

    def getCoeffs1(self):
        """Return a 2-D numpy array of crosstalk coefficients of the proper shape"""
        return np.array(self.crossTalkCoeffs1).reshape(self.shapeCoeffsArray).tolist()

    def getCoeffs2(self):
        """Return a 2-D numpy array of crosstalk coefficients of the proper shape"""
        return np.array(self.crossTalkCoeffs2).reshape(self.shapeCoeffsArray).tolist()

    def getGainsPreampSigboard(self):
        """Return a 2-D numpy array of effective gain for preamp+SIG of the proper shape"""
        return np.array(self.relativeGainsPreampAndSigboard).reshape(self.shapeGainsArray).tolist()

    def getGainsTotal(self, dateobs='2008-08-01'):
        """Return a 2-D numpy array of effective total gain of the proper shape"""
        if dateobs < '2010-10': #  may need rewritten to a more reliable way
            return np.array(self.relativeGainsTotalBeforeOct2010).reshape(self.shapeGainsArray).tolist()
        else:
            return np.array(self.relativeGainsTotalAfterOct2010).reshape(self.shapeGainsArray).tolist()

nAmp = 4


def getCrosstalkX1(x, xmax=512):
    """
    Return the primary X positions in CCD which affect the count of input x pixel
    x    :  xpos in Ccd, affected by the returned pixels
    xmax :  nx per amp
    """
    ctx1List = [x,
                2 * xmax - x - 1,
                2 * xmax + x,
                4 * xmax - x - 1, ]

    return ctx1List


def getCrosstalkX2(x, xmax=512):
    """
    Return the 2ndary X positions (dX=2) in CCD which affect the count of input x pixel
           Those X pixels are read by 2-pixel ealier than the primary pixels.
    x    :  xpos in Ccd, affected by the returned pixels
    xmax :  nx per amp
    """
    # ch0,ch2: ctx2 = ctx1 - 2,  ch1,ch3: ctx2 = ctx1 + 2
    ctx2List = [x - 2,
                2 * xmax - x - 1 + 2,
                2 * xmax + x - 2,
                4 * xmax - x - 1 + 2, ]

    return ctx2List


def getAmplifier(image, amp, ampReference=None, offset=2):
    """Extract an image of the amplifier from the CCD, along with an offset version

    The amplifier image will be flipped (if required) to match the
    orientation of a nominated reference amplifier.

    An additional image, with the nominated offset applied, is also produced.

    @param image           Image of CCD
    @param amp             Index of amplifier
    @param ampReference    Index of reference amplifier
    @param offset          Offset to apply
    @return amplifier image, offset amplifier image
    """
    height = image.getHeight()
    ampBox = afwGeom.Box2I(afwGeom.Point2I(amp*512, 0), afwGeom.Extent2I(512, height))
    ampImage = image.Factory(image, ampBox, afwImage.LOCAL)
    if ampReference is not None and amp % 2 != ampReference % 2:
        ampImage = afwMath.flipImage(ampImage, True, False)
    offBox = afwGeom.Box2I(afwGeom.Point2I(offset if amp == ampReference else 0, 0),
                           afwGeom.Extent2I(510, height))
    offsetImage = ampImage.Factory(ampImage, offBox, afwImage.LOCAL)
    return ampImage, offsetImage


def subtractCrosstalkYagi(mi, coeffs1List, coeffs2List, gainsPreampSig, minPixelToMask=45000,
                          crosstalkStr="CROSSTALK"):
    """Subtract the crosstalk from MaskedImage mi given a set of coefficients
       based on procedure presented in Yagi et al. 2012, PASP in publication; arXiv:1210.8212
       The pixels affected by signal over minPixelToMask have the crosstalkStr bit set
    """

    ####sctrl = afwMath.StatisticsControl()
    ####sctrl.setAndMask(mi.getMask().getPlaneBitMask("DETECTED"))
    ####bkgd = afwMath.makeStatistics(mi, afwMath.MEDIAN, sctrl).getValue()
    #
    # These are the pixels that are bright enough to cause crosstalk (more precisely,
    # the ones that we label as causing crosstalk; in reality all pixels cause crosstalk)
    #
    if True:
        tempStr = "TEMP"                    # mask plane used to record the bright pixels that we need to mask
        mi.getMask().addMaskPlane(tempStr)
        fs = afwDetect.FootprintSet(mi, afwDetect.Threshold(minPixelToMask), tempStr)

        mi.getMask().addMaskPlane(crosstalkStr)
        ds9.setMaskPlaneColor(crosstalkStr, ds9.MAGENTA)
        fs.setMask(mi.getMask(), crosstalkStr)  # the crosstalkStr bit will now be set
        # whenever we subtract crosstalk
        crosstalk = mi.getMask().getPlaneBitMask(crosstalkStr)

    if True:
        # This python implementation is fairly fast
        image = mi.getImage()
        xtalk = afwImage.ImageF(image.getDimensions())
        xtalk.set(0)
        for i in range(4):
            xAmp, xOff = getAmplifier(xtalk, i, i)
            for j in range(4):
                if i == j:
                    continue
                gainRatio = gainsPreampSig[j] / gainsPreampSig[i]
                jAmp, jOff = getAmplifier(image, j, i)
                xAmp.scaledPlus(gainRatio*coeffs1List[i][j], jAmp)
                xOff.scaledPlus(gainRatio*coeffs2List[i][j], jOff)

        image -= xtalk
    else:
        nAmp = 4
        subtractCrosstalk(mi, nAmp, coeffs1List, coeffs2List, gainsPreampSig)

    #
    # Clear the crosstalkStr bit in the original bright pixels, where tempStr is set
    #
    msk = mi.getMask()
    temp = msk.getPlaneBitMask(tempStr)
    xtalk_temp = crosstalk | temp
    np_msk = msk.getArray()
    np_msk[np.where(np.bitwise_and(np_msk, xtalk_temp) == xtalk_temp)] &= ~crosstalk

    try:
        msk.removeAndClearMaskPlane(tempStr, True) # added in afw #1853
    except AttributeError:
        ds9.setMaskPlaneVisibility(tempStr, False)


def main(visit=131634, ccd=None, threshold=45000, nSample=1, showCoeffs=True, fixXTalk=True,
         plot=False, title=None):
    if ccd is None:
        visitList = list(range(nSample))
        ccdList = ["simulated", ]
    else:
        ccdList = makeList(ccd)
        visitList = makeList(visit)

    coeffs = estimateCoeffs(visitList, ccdList, threshold=45000, plot=plot, title=title)

    if showCoeffs:
        printCoeffs(coeffs)

    mi = readImage(visitList[0], ccdList[0])
    if fixXTalk:
        subtractXTalk(mi, coeffs, threshold)

    return mi, coeffs


class YagiCrosstalkConfig(pexConfig.Config):
    coeffs = pexConfig.ConfigField(
        dtype=CrosstalkYagiCoeffsConfig,
        doc="Crosstalk coefficients by Yagi+ 2012",
    )
    crosstalkMaskPlane = pexConfig.Field(
        dtype=str,
        doc="Name of Mask plane for crosstalk corrected pixels",
        default="CROSSTALK",
    )
    minPixelToMask = pexConfig.Field(
        dtype=float,
        doc="Minimum pixel value (in electrons) to cause crosstalkMaskPlane bit to be set",
        default=45000,
    )


class YagiCrosstalkTask(pipeBase.Task):
    ConfigClass = YagiCrosstalkConfig

    def run(self, exposure):
        coeffs1List = self.config.coeffs.getCoeffs1() # primary crosstalk
        coeffs2List = self.config.coeffs.getCoeffs2() # secondary crosstalk
        gainsPreampSig = self.config.coeffs.getGainsPreampSigboard()
        if not np.any(coeffs1List):
            self.log.info("No crosstalk info available. Skipping crosstalk corrections to CCD %s" %
                          (exposure.getDetector().getId()))
            return

        self.log.info("Applying crosstalk corrections to CCD %s based on Yagi+2012" %
                      (exposure.getDetector().getId()))

        ccdId = int(exposure.getDetector().getId().getSerial())
        gainsPreampSigCcd = gainsPreampSig[ccdId]

        subtractCrosstalkYagi(exposure.getMaskedImage(), coeffs1List, coeffs2List,
                              gainsPreampSigCcd,
                              self.config.minPixelToMask, self.config.crosstalkMaskPlane)


if __name__ == "__main__":
    main()
