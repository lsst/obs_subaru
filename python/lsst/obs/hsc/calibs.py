from __future__ import absolute_import, division, print_function
from builtins import zip
from builtins import range
import math
import numpy as np

from lsst.pex.config import Field, ListField, ConfigField
from lsst.pipe.drivers.constructCalibs import CalibCombineConfig, CalibCombineTask
from lsst.obs.hsc.vignette import VignetteConfig
import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom
import lsst.afw.cameraGeom as afwcg


class HscFlatCombineConfig(CalibCombineConfig):
    vignette = ConfigField(dtype=VignetteConfig, doc="Vignetting parameters in focal plane coordinates")
    badAmpCcdList = ListField(dtype=int, default=[], doc="List of CCD serial numbers for bad amplifiers")
    badAmpList = ListField(dtype=int, default=[], doc="List of amp serial numbers in CCD")
    maskPlane = Field(dtype=str, default="NO_DATA", doc="Mask plane to set")

    def validate(self):
        CalibCombineConfig.validate(self)
        if len(self.badAmpCcdList) != len(self.badAmpList):
            raise RuntimeError("Length of badAmpCcdList and badAmpList don't match")


class HscFlatCombineTask(CalibCombineTask):
    """Mask the vignetted area"""
    ConfigClass = HscFlatCombineConfig

    def run(self, sensorRefList, *args, **kwargs):
        """Mask vignetted pixels after combining

        This returns an Exposure instead of an Image, but upstream shouldn't
        care, as it just dumps it out via the Butler.
        """
        combined = super(HscFlatCombineTask, self).run(sensorRefList, *args, **kwargs)
        mi = afwImage.makeMaskedImage(combined.getImage())
        mi.getMask().set(0)

        # Retrieve the detector
        # XXX It's unfortunate that we have to read an entire image to get the detector, but there's no
        # public API in the butler to get the same.
        image = sensorRefList[0].get("postISRCCD")
        detector = image.getDetector()
        del image

        self.maskVignetting(mi.getMask(), detector)
        self.maskBadAmps(mi.getMask(), detector)

        return afwImage.makeExposure(mi)

    def maskVignetting(self, mask, detector):
        """Mask vignetted pixels in-place

        @param mask: Mask image to modify
        @param detector: Detector for transformation to focal plane coords
        """
        mask.addMaskPlane(self.config.maskPlane)
        bitMask = mask.getPlaneBitMask(self.config.maskPlane)
        w, h = mask.getWidth(), mask.getHeight()
        numCorners = 0 # Number of corners outside radius
        transform = detector.getTransformMap().get(detector.makeCameraSys(afwcg.FOCAL_PLANE))
        for i, j in ((0, 0), (0, h-1), (w-1, 0), (w-1, h-1)):
            x, y = transform.forwardTransform(afwGeom.Point2D(i, j))
            if math.hypot(x - self.config.vignette.xCenter,
                          y - self.config.vignette.yCenter) > self.config.vignette.radius:
                numCorners += 1
        if numCorners == 0:
            # Nothing to be masked
            self.log.info("Detector %d is unvignetted" % detector.getId())
            return
        if numCorners == 4:
            # Everything to be masked
            # We ignore the question of how we're getting any data from a CCD that's totally vignetted...
            self.log.warn("Detector %d is completely vignetted" % detector.getId())
            mask |= bitMask
            return
        # We have to go pixel by pixel
        x = np.empty((h, w))
        y = np.empty_like(x)
        for j in range(mask.getHeight()):
            for i in range(mask.getWidth()):
                x[j, i], y[j, i] = transform.forwardTransform(afwGeom.Point2D(i, j))
        R = np.hypot(x - self.config.vignette.xCenter, y - self.config.vignette.yCenter)
        isBad = R > self.config.vignette.radius
        self.log.info("Detector %d has %f%% pixels vignetted" %
                      (detector.getId(), isBad.sum()/float(isBad.size)*100.0))
        maskArray = mask.getArray()
        xx, yy = np.meshgrid(np.arange(w), np.arange(h))
        maskArray[yy[isBad], xx[isBad]] |= bitMask

    def maskBadAmps(self, mask, detector):
        """Mask bad amplifiers

        @param mask: Mask image to modify
        @param detector: Detector for amp locations
        """
        mask.addMaskPlane(self.config.maskPlane)
        bitMask = mask.getPlaneBitMask(self.config.maskPlane)
        if detector is None:
            raise RuntimeError("Detector isn't a Ccd")
        ccdSerial = detector.getId()
        if ccdSerial not in self.config.badAmpCcdList:
            return
        for ccdNum, ampNum in zip(self.config.badAmpCcdList, self.config.badAmpList):
            if ccdNum != ccdSerial:
                continue
            # Mask this amp
            found = False
            for amp in detector:
                if amp.getId() != ampNum:
                    continue
                found = True
                box = amp.getDataSec()
                subMask = mask.Factory(mask, box)
                subMask |= bitMask
                break
            if not found:
                self.log.warn("Unable to find amp=%s in ccd=%s" % (ampNum, ccdNum))
