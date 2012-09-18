#!/usr/bin/env python

import os
import numpy

from lsst.pex.config import Field
from lsst.pipe.base import Struct
from lsst.ip.isr import IsrTask
from lsst.ip.isr import isr as lsstIsr
import lsst.pex.config as pexConfig
import lsst.afw.cameraGeom as afwCG
import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom
import lsst.afw.math as afwMath
from . import crosstalk
import lsst.afw.cameraGeom as cameraGeom
import lsst.meas.algorithms as measAlg

try:
    import hsc.fitsthumb as fitsthumb
except ImportError:
    fitsthumb = None

class QaFlatnessConfig(pexConfig.Config):
    meshX = pexConfig.Field(
        dtype = int,
        doc = 'Mesh size in X (pix) to calculate count statistics',
        default = 256,
        )
    meshY = pexConfig.Field(
        dtype = int,
        doc = 'Mesh size in Y (pix) to calculate count statistics',
        default = 256,
        )
    doClip = pexConfig.Field(
        dtype = bool,
        doc = 'Do we clip outliers in calculate count statistics?',
        default = True,
        )
    clipSigma = pexConfig.Field(
        dtype = float,
        doc = 'How many sigma is used to clip outliers in calculate count statistics?',
        default = 3.0,
        )
    nIter = pexConfig.Field(
        dtype = int,
        doc = 'How many times do we iterate clipping outliers in calculate count statistics?',
        default = 3,
        )

class QaConfig(pexConfig.Config):
    flatness = pexConfig.ConfigField(dtype=QaFlatnessConfig, doc="Qa.flatness")
    doWriteOss = pexConfig.Field(doc="Write OverScan-Subtracted image?", dtype=bool, default=False)
    doThumbnailOss = pexConfig.Field(doc="Write OverScan-Subtracted thumbnail?", dtype=bool, default=True)
    doWriteFlattened = pexConfig.Field(doc="Write flattened image?", dtype=bool, default=False)
    doThumbnailFlattened = pexConfig.Field(doc="Write flattened thumbnail?", dtype=bool, default=True)

class SubaruIsrConfig(IsrTask.ConfigClass):
    qa = pexConfig.ConfigField(doc="QA-related config options", dtype=QaConfig)
    doSaturation = pexConfig.Field(doc="Mask saturated pixels?", dtype=bool, default=True)
    doOverscan = pexConfig.Field(doc="Do overscan subtraction?", dtype=bool, default=True)
    doVariance = pexConfig.Field(doc="Calculate variance?", dtype=bool, default=True)
    doDefect = pexConfig.Field(doc="Mask defect pixels?", dtype=bool, default=False)
    doGuider = pexConfig.Field(
        dtype = bool,
        doc = "Trim guider shadow",
        default = True,
    )
    doCrosstalk = pexConfig.Field(
        dtype = bool,
        doc = "Correct for crosstalk",
        default = True,
    )
    doLinearize = pexConfig.Field(
        dtype = bool,
        doc = "Correct for nonlinearity of the detector's response (ignored if coefficients are 0.0)",
        default = True,
    )
    linearizationThreshold = pexConfig.Field(
        dtype = float,
        doc = "Minimum pixel value (in electrons) to apply linearity corrections",
        default = 0.0,
    )
    linearizationCoefficient = pexConfig.Field(
        dtype = float,
        doc = "Linearity correction coefficient",
        default = 0.0,
    )
    removePcCards = Field(dtype=bool, doc='Remove any PC cards in the header', default=True)

class SubaruIsrTask(IsrTask):

    ConfigClass = SubaruIsrConfig

    def run(self, sensorRef):
        self.log.log(self.log.INFO, "Performing ISR on sensor %s" % (sensorRef.dataId))
        ccdExposure = sensorRef.get('raw')

        if self.config.removePcCards: # Remove any PC00N00M cards in the header
            raw_md = sensorRef.get("raw_md")
            nPc = 0
            for i in (1, 2,):
                for j in (1, 2,):
                    k = "PC%03d%03d" % (i, j)
                    for md in (raw_md, ccdExposure.getMetadata()):
                        if md.exists(k):
                            md.remove(k)
                            nPc += 1

            if nPc:
                self.log.log(self.log.INFO, "Recreating Wcs after stripping PC00n00m" % (sensorRef.dataId))
                ccdExposure.setWcs(afwImage.makeWcs(raw_md))

        ccdExposure = self.convertIntToFloat(ccdExposure)
        ccd = afwCG.cast_Ccd(ccdExposure.getDetector())

        for amp in ccd:
            self.measureOverscan(ccdExposure, amp)
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

        if self.config.qa.doWriteOss:
            sensorRef.put(ccdExposure, "ossImage")
        if self.config.qa.doThumbnailOss:
            self.writeThumbnail(sensorRef, "ossThumb", ccdExposure)

        if self.config.doDefect:
            self.maskDefect(ccdExposure)

        if self.config.doBias:
            self.biasCorrection(ccdExposure, sensorRef)
        if self.config.doDark:
            self.darkCorrection(ccdExposure, sensorRef)
        if self.config.doFlat:
            self.flatCorrection(ccdExposure, sensorRef)

        if self.config.qa.doWriteFlattened:
            sensorRef.put(ccdExposure, "flattenedImage")
        if self.config.qa.doThumbnailFlattened:
            self.writeThumbnail(sensorRef, "flattenedThumb", ccdExposure)

        self.measureBackground(ccdExposure)

        if self.config.doCrosstalk:
            self.crosstalk(ccdExposure)
        if self.config.doLinearize:
            self.linearize(ccdExposure)
        if self.config.doGuider:
            self.guider(ccdExposure)

        if self.config.doWrite:
            sensorRef.put(ccdExposure, "postISRCCD")
        
        self.display("postISRCCD", ccdExposure)

        return Struct(exposure=ccdExposure)

    def writeThumbnail(self, dataRef, dataset, exposure, format='png', width=500, height=0):
        """Write out exposure to a snapshot file named outfile in the given image format and size.
        """
        if fitsthumb is None:
            self.log.log(self.log.WARN,
                         "Cannot write thumbnail image; hsc.fitsthumb could not be imported.")
            return
        filename = dataRef.get(dataset + "_filename")[0]
        directory = os.path.dirname(filename)
        if not os.path.exists(directory):
            try:
                os.makedirs(directory)
            except OSError, e:
                # Don't fail if directory exists due to race
                if e.errno != 17:
                    raise e
        image = exposure.getMaskedImage().getImage()
        fitsthumb.createFitsThumb(image, filename, format, width, height, True)

    def measureOverscan(self, ccdExposure, amp):
        clipSigma = 3.0
        nIter = 3
        levelStat = afwMath.MEDIAN
        sigmaStat = afwMath.STDEVCLIP
        
        sctrl = afwMath.StatisticsControl(clipSigma, nIter)
        expImage = ccdExposure.getMaskedImage().getImage()
        overscan = expImage.Factory(expImage, amp.getDiskBiasSec())
        stats = afwMath.makeStatistics(overscan, levelStat | sigmaStat, sctrl)
        ampNum = amp.getId().getSerial()
        metadata = ccdExposure.getMetadata()
        metadata.set("OSLEVEL%d" % ampNum, stats.getValue(levelStat))
        metadata.set("OSSIGMA%d" % ampNum, stats.getValue(sigmaStat))


    def measureBackground(self, exposure):
        statsControl = afwMath.StatisticsControl(self.config.qa.flatness.clipSigma,
                                                 self.config.qa.flatness.nIter)
        maskedImage = exposure.getMaskedImage()
        stats = afwMath.makeStatistics(maskedImage, afwMath.MEDIAN | afwMath.STDEVCLIP, statsControl)
        skyLevel = stats.getValue(afwMath.MEDIAN)
        skySigma = stats.getValue(afwMath.STDEVCLIP)
        self.log.info("Flattened sky level: %f +/- %f" % (skyLevel, skySigma))
        metadata = exposure.getMetadata()
        metadata.set('SKYLEVEL', skyLevel)
        metadata.set('SKYSIGMA', skySigma)

        # calcluating flatlevel over the subgrids 
        stat = afwMath.MEANCLIP if self.config.qa.flatness.doClip else afwMath.MEAN
        meshXHalf = int(self.config.qa.flatness.meshX/2.)
        meshYHalf = int(self.config.qa.flatness.meshY/2.)
        nX = int((exposure.getWidth() + meshXHalf) / self.config.qa.flatness.meshX)
        nY = int((exposure.getHeight() + meshYHalf) / self.config.qa.flatness.meshY)
        skyLevels = numpy.zeros((nX,nY))

        for j in range(nY):
            yc = meshYHalf + j * self.config.qa.flatness.meshY
            for i in range(nX):
                xc = meshXHalf + i * self.config.qa.flatness.meshX

                xLLC = xc - meshXHalf
                yLLC = yc - meshYHalf
                xURC = xc + meshXHalf - 1
                yURC = yc + meshYHalf - 1

                bbox = afwGeom.Box2I(afwGeom.Point2I(xLLC, yLLC), afwGeom.Point2I(xURC, yURC))
                miMesh = maskedImage.Factory(exposure.getMaskedImage(), bbox, afwImage.LOCAL)

                skyLevels[i,j] = afwMath.makeStatistics(miMesh, stat, statsControl).getValue()

        skyMedian = numpy.median(skyLevels)
        flatness =  (skyLevels - skyMedian) / skyMedian
        flatness_rms = numpy.std(flatness)
        flatness_min = flatness.min()
        flatness_max = flatness.max() 
        flatness_pp = flatness_max - flatness_min

        self.log.info("Measuring sky levels in %dx%d grids: %f" % (nX, nY, skyMedian))
        self.log.info("Sky flatness in %dx%d grids - pp: %f rms: %f" % (nX, nY, flatness_pp, flatness_rms))

        metadata.set('FLATNESS_PP', flatness_pp)
        metadata.set('FLATNESS_RMS', flatness_rms)
        metadata.set('FLATNESS_NGRIDS', '%dx%d' % (nX, nY))
        metadata.set('FLATNESS_MESHX', self.config.qa.flatness.meshX)
        metadata.set('FLATNESS_MESHY', self.config.qa.flatness.meshY)


    def crosstalk(self, exposure):
        raise NotImplementedError("Crosstalk correction is enabled but no generic implementation is present")

    def guider(self, exposure):
        raise NotImplementedError(
            "Guider shadow trimming is enabled but no generic implementation is present"
            )

    def linearize(self, exposure):
        """Correct for non-linearity

        @param exposure Exposure to process
        """
        assert exposure, "No exposure provided"

        image = exposure.getMaskedImage().getImage()

        ccd = afwCG.cast_Ccd(exposure.getDetector())

        for amp in ccd:
            if False:
                linear_threshold = amp.getElectronicParams().getLinearizationThreshold()
                linear_c = amp.getElectronicParams().getLinearizationCoefficient()
            else:
                linearizationCoefficient = self.config.linearizationCoefficient
                linearizationThreshold = self.config.linearizationThreshold

            if linearizationCoefficient == 0.0:     # nothing to do
                continue

            self.log.log(self.log.INFO,
                         "Applying linearity corrections to Ccd %s Amp %s" % (ccd.getId(), amp.getId()))

            if linearizationThreshold > 0:
                log10_thresh = math.log10(linearizationThreshold)

            ampImage = image.Factory(image, amp.getDataSec(), afwImage.LOCAL)

            width, height = ampImage.getDimensions()

            if linearizationThreshold <= 0:
                tmp = ampImage.Factory(ampImage, True)
                tmp.scaledMultiplies(linearizationCoefficient, ampImage)
                ampImage += tmp
            else:
                for y in range(height):
                    for x in range(width):
                        val = ampImage.get(x, y)
                        if val > linearizationThreshold:
                            val += val*linearizationCoefficient*(math.log10(val) - log10_thresh)
                            ampImage.set(x, y, val)

    def maskDefect(self, ccdExposure):
        """Mask defects using mask plane "BAD"

        @param[in,out]  ccdExposure     exposure to process
        
        @warning: call this after CCD assembly, since defects may cross amplifier boundaries
        """
        maskedImage = ccdExposure.getMaskedImage()
        ccd = cameraGeom.cast_Ccd(ccdExposure.getDetector())
        defectBaseList = ccd.getDefects()
        defectList = measAlg.DefectListT()
        # mask bad pixels in the camera class
        # create master list of defects and add those from the camera class
        for d in defectBaseList:
            bbox = d.getBBox()
            nd = measAlg.Defect(bbox)
            defectList.append(nd)
        lsstIsr.maskPixelsFromDefectList(maskedImage, defectList, maskName='BAD')

class HamamatsuIsrTaskConfig(SubaruIsrTask.ConfigClass):
    crosstalkCoeffs = pexConfig.ConfigField(
        dtype = crosstalk.CrosstalkCoeffsConfig,
        doc = "Crosstalk coefficients",
    )
    crosstalkMaskPlane = pexConfig.Field(
        dtype = str,
        doc = "Name of Mask plane for crosstalk corrected pixels",
        default = "CROSSTALK",
    )
    minPixelToMask = pexConfig.Field(
        dtype = float,
        doc = "Minimum pixel value (in electrons) to cause crosstalkMaskPlane bit to be set",
        default = 45000,
        )

class SuprimeCamIsrTaskConfig(HamamatsuIsrTaskConfig):
    pass

class SuprimeCamIsrTask(SubaruIsrTask):
    
    ConfigClass = SuprimeCamIsrTaskConfig

    def crosstalk(self, exposure):
        coeffs = self.config.crosstalkCoeffs.getCoeffs()

        if numpy.any(coeffs):
            self.log.log(self.log.INFO, "Applying crosstalk corrections to CCD %s" %
                         (exposure.getDetector().getId()))

            crosstalk.subtractXTalk(exposure.getMaskedImage(), coeffs,
                                    self.config.minPixelToMask, self.config.crosstalkMaskPlane)

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

class SuprimeCamMitIsrTaskConfig(SubaruIsrTask.ConfigClass):
    pass

class SuprimeCamMitIsrTask(SubaruIsrTask):
    
    ConfigClass = SuprimeCamMitIsrTaskConfig

    def guider(self, exposure):
        """Mask defects and trim guider shadow

        @param exposure Exposure to process
        """
        assert exposure, "No exposure provided"
        
        ccd = afwCG.cast_Ccd(exposure.getDetector()) # This is Suprime-Cam so we know the Detector is a Ccd
        ccdNum = ccd.getId().getSerial()
        if ccdNum not in [0, 1, 4, 5, 9]:
            # No need to mask
            return

        md = exposure.getMetadata()
        if not md.exists("S_AG-X"):
            self.log.log(self.log.WARN, "No autoguider position in exposure metadata.")
            return

        xGuider = md.get("S_AG-X")
        if ccdNum in [1, 4, 5]:
            maskLimit = int(60.0 * xGuider - 2300.0) # From SDFRED
        elif ccdNum in [0, 9]:
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
