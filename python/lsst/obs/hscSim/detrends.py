import math
import numpy as np

from lsst.pex.config import Field
from hsc.pipe.tasks.detrends import FlatCombineConfig, FlatCombineTask
import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom

class HscFlatCombineConfig(FlatCombineConfig):
    xCenter = Field(dtype=float, default=0,   doc="Center of vignetting pattern, in x (focal plane coords)")
    yCenter = Field(dtype=float, default=300, doc="Center of vignetting pattern, in y (focal plane coords)")
    radius = Field(dtype=float, default=18300, doc="Radius of vignetting pattern, in focal plane coords",
                   check=lambda x: x >= 0)
    maskPlane = Field(dtype=str, default="BAD", doc="Mask plane to set")

class HscFlatCombineTask(FlatCombineTask):
    """Mask the vignetted area"""
    ConfigClass = HscFlatCombineConfig
    def run(self, sensorRefList, *args, **kwargs):
        """Mask vignetted pixels after combining

        This returns an Exposure instead of an Image, but upstream shouldn't
        care, as it just dumps it out via the Butler.
        """
        combined = super(HscFlatCombineTask, self).run(sensorRefList, *args, **kwargs)
        mi = afwImage.makeMaskedImage(combined)
        mi.getMask().set(0)

        # Retrieve the detector
        # XXX It's unfortunate that we have to read an entire image to get the detector, but there's no
        # public API in the butler to get the same.
        image = sensorRefList[0].get("postISRCCD")
        detector = image.getDetector()
        del image

        self.maskVignetting(mi.getMask(), detector)

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
        for i, j in ((0, 0), (0, h-1), (w-1, 0), (w-1, h-1)):
            x,y = detector.getPositionFromPixel(afwGeom.PointD(i, j)).getMm()
            if math.hypot(x - self.config.xCenter, y - self.config.yCenter) > self.config.radius:
                numCorners += 1
        if numCorners == 0:
            # Nothing to be masked
            self.log.info("Detector %s is unvignetted" % detector.getId().getSerial())
            return
        if numCorners == 4:
            # Everything to be masked
            # We ignore the question of how we're getting any data from a CCD that's totally vignetted...
            self.log.warn("Detector %s is completely vignetted" % detector.getId().getSerial())
            mask |= bitMask
            return
        # We have to go pixel by pixel
        x = np.empty((h, w))
        y = np.empty_like(x)
        for j in range(mask.getHeight()):
            for i in range(mask.getWidth()):
                x[j, i], y[j, i] = detector.getPositionFromPixel(afwGeom.PointD(i, j)).getMm()
        R = np.hypot(x - self.config.xCenter, y - self.config.yCenter)
        isBad = R > self.config.radius
        self.log.info("Detector %s has %f%% pixels vignetted" %
                      (detector.getId().getSerial(), isBad.sum()/float(isBad.size)*100.0))
        maskArray = mask.getArray()
        xx, yy = np.meshgrid(np.arange(w), np.arange(h))
        maskArray[yy[isBad], xx[isBad]] |= bitMask
