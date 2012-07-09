#!/usr/bin/env python

from lsst.pex.config import Field
from lsst.pipe.base import Struct
from lsst.ip.isr import IsrTask
from lsst.ip.isr import isr as lsstIsr
import lsst.afw.cameraGeom as afwCG
import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom

class SubaruIsrConfig(IsrTask.ConfigClass):
    doSaturation = Field(doc="Mask saturated pixels?", dtype=bool, default=True)
    doOverscan = Field(doc="Do overscan subtraction?", dtype=bool, default=True)
    doVariance = Field(doc="Calculate variance?", dtype=bool, default=True)

class SubaruIsrTask(IsrTask):

    ConfigClass = SubaruIsrConfig

    def run(self, sensorRef):
        self.log.log(self.log.INFO, "Performing ISR on sensor %s" % (sensorRef.dataId))
        ccdExposure = sensorRef.get('raw')
        ccdExposure = self.convertIntToFloat(ccdExposure)
        ccd = afwCG.cast_Ccd(ccdExposure.getDetector())

        for amp in ccd:
            if self.config.doSaturation:
                self.saturationDetection(ccdExposure, amp)
            if self.config.doOverscan:
                self.overscanCorrection(ccdExposure, amp)
            if self.config.doVariance:
                # Ideally, this should be done after bias subtraction,
                # but CCD assembly demands a variance plane
                ampExposure = ccdExposure.Factory(ccdExposure, amp.getDiskDataSec(), afwImage.PARENT)
                self.updateVariance(ampExposure, amp)

        ccdExposure = self.assembleCcd.assembleCcd(ccdExposure)
        ccd = afwCG.cast_Ccd(ccdExposure.getDetector())

        if self.config.doBias:
            self.biasCorrection(ccdExposure, sensorRef)
        if self.config.doDark:
            self.darkCorrection(ccdExposure, sensorRef)
        if self.config.doFlat:
            self.flatCorrection(ccdExposure, sensorRef)

        if self.config.doWrite:
            sensorRef.put(ccdExposure, "postISRCCD")
        
        self.display("postISRCCD", ccdExposure)

        return Struct(exposure=ccdExposure)

class SuprimeCamIsrTask(SubaruIsrTask):

    def run(self, *args, **kwargs):
        result = super(SuprimeCamIsrTask, self).run(*args, **kwargs)
        self.guider(result.exposure)
        return result
    
    def guider(self, exposure):
        """Mask defects and trim guider shadow

        @param exposure Exposure to process
        """
        assert exposure, "No exposure provided"
        
        ccd = afwCG.cast_Ccd(exposure.getDetector()) # This is Suprime-Cam so we know the Detector is a Ccd
        ccdNum = ccd.getId().getSerial()
        if ccdNum not in [0, 1, 2, 6, 7]:
            # No need to mask
            return

        md = exposure.getMetadata()
        if not md.exists("S_AG-X"):
            self.log.log(self.log.WARN, "No autoguider position in exposure metadata.")
            return

        xGuider = md.get("S_AG-X")
        if ccdNum in [1, 2, 7]:
            maskLimit = int(60.0 * xGuider - 2300.0) # From SDFRED
        elif ccdNum in [0, 6]:
            maskLimit = int(60.0 * xGuider - 2000.0) # From SDFRED

        
        mi = exposure.getMaskedImage()
        height = mi.getHeight()
        if height < maskLimit:
            # Nothing to mask!
            return

        if False:
            # XXX This mask plane isn't respected by background subtraction or source detection or measurement
            self.log.log(self.log.INFO, "Masking autoguider shadow at y > %d" % maskLimit)
            mask = mi.getMask()
            bbox = afwGeom.Box2I(afwGeom.Point2I(0, maskLimit - 1),
                                 afwGeom.Point2I(mask.getWidth() - 1, height - 1))
            badMask = mask.Factory(mask, bbox, afwImage.LOCAL)
            
            mask.addMaskPlane("GUIDER")
            badBitmask = mask.getPlaneBitMask("GUIDER")
            
            badMask |= badBitmask
        else:
            # XXX Temporary solution until a mask plane is respected by downstream processes
            self.log.log(self.log.INFO, "Removing pixels affected by autoguider shadow at y > %d" % maskLimit)
            bbox = afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(mi.getWidth(), maskLimit))
            good = mi.Factory(mi, bbox, afwImage.LOCAL)
            exposure.setMaskedImage(good)
