#!/usr/bin/env python

import os
import numpy

from lsst.pex.config import Field
from lsst.pipe.base import Task, Struct
from lsst.ip.isr import IsrTask
from lsst.ip.isr import isr as lsstIsr
import lsst.pex.config as pexConfig
import lsst.afw.cameraGeom as afwCG
import lsst.afw.detection as afwDetection
import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom
import lsst.afw.math as afwMath
from lsst.obs.subaru.crosstalkYagi import YagiCrosstalkTask
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

class NullCrosstalkTask(Task):
    ConfigClass = pexConfig.Config
    def run(self, exposure):
        self.log.info("Not performing any crosstalk correction")

class SubaruIsrConfig(IsrTask.ConfigClass):
    qa = pexConfig.ConfigField(doc="QA-related config options", dtype=QaConfig)
    doSaturation = pexConfig.Field(doc="Mask saturated pixels?", dtype=bool, default=True)
    doWidenSaturationTrails = pexConfig.Field(doc="Widen bleed trails based on their width?",
                                              dtype=bool, default=True)
    doOverscan = pexConfig.Field(doc="Do overscan subtraction?", dtype=bool, default=True)
    doVariance = pexConfig.Field(doc="Calculate variance?", dtype=bool, default=True)
    doDefect = pexConfig.Field(doc="Mask defect pixels?", dtype=bool, default=False)
    doGuider = pexConfig.Field(
        dtype = bool,
        doc = "Trim guider shadow",
        default = True,
    )
    crosstalk = pexConfig.ConfigurableField(target=NullCrosstalkTask, doc="Crosstalk correction")
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
    doApplyGains = pexConfig.Field(
        dtype = bool,
        doc = """Correct the amplifiers for their gains

N.b. this is intended to be used *instead* of doFlat; it's useful if you're measuring system throughput
""",
        default = False,
    )
    normalizeGains = pexConfig.Field(
        dtype = bool,
        doc = """Normalize all the amplifiers in each CCD to have the same gain

This does not measure the gains, it simply forces the median of each amplifier to be equal
after applying the nominal gain
""",
        default = False,
    )
    removePcCards = Field(dtype=bool, doc='Remove any PC cards in the header', default=True)
    fwhmForBadColumnInterpolation = pexConfig.Field(
        dtype = float,
        doc = "FWHM of PSF used when interpolating over bad columns (arcsec)",
        default = 1.0,
    )
    doSetBadRegions = pexConfig.Field(
        dtype = bool,
        doc = "Should we set the level of all BAD patches of the chip to the chip's average value?",
        default = True,
        )
    badStatistic = pexConfig.ChoiceField(
        dtype = str,
        doc = "How to estimate the average value for BAD regions.",
        default = 'MEANCLIP',
        allowed = {
            "MEANCLIP": "Correct using the (clipped) mean of good data",
            "MEDIAN": "Correct using the median of the good data",
            },
        )
    doTweakFlat = pexConfig.Field(dtype=bool, doc="Tweak flats to match observed amplifier ratios?",
                                  default=False)
    def validate(self):
        super(SubaruIsrConfig, self).validate()
        if self.doFlat and self.doApplyGains:
            raise ValueError("You may not specify both self.doFlat and self.doApplyGains")

    def setDefaults(self):
        super(SubaruIsrConfig, self).setDefaults()
        # Relative gains in the camera should be taken out by the flat-field, not by "gain" values.
        self.assembleCcd.doRenorm = False # Don't multiply by the gain

class SubaruIsrTask(IsrTask):

    ConfigClass = SubaruIsrConfig

    def __init__(self, *args, **kwargs):
        super(SubaruIsrTask, self).__init__(*args, **kwargs)
        self.makeSubtask("crosstalk")

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

        self.maskAndInterpDefect(ccdExposure)

        if self.config.qa.doWriteOss:
            sensorRef.put(ccdExposure, "ossImage")
        if self.config.qa.doThumbnailOss:
            self.writeThumbnail(sensorRef, "ossThumb", ccdExposure)

        if self.config.doBias:
            self.biasCorrection(ccdExposure, sensorRef)
        if self.config.doLinearize:
            self.linearize(ccdExposure)
        if self.config.doCrosstalk:
            self.crosstalk.run(ccdExposure)
        if self.config.doDark:
            self.darkCorrection(ccdExposure, sensorRef)
        if self.config.doFlat:
            self.flatCorrection(ccdExposure, sensorRef)
        if self.config.doApplyGains:
            self.applyGains(ccdExposure, self.config.normalizeGains)
        if self.config.doWidenSaturationTrails:
            self.widenSaturationTrails(ccdExposure.getMaskedImage().getMask())
        if self.config.doSaturation:
            self.saturationInterpolation(ccdExposure)
        if self.config.doDefect:
            self.maskDefect(ccdExposure, self.config.fwhmForBadColumnInterpolation)

        if self.config.doFringe:
            self.fringe.run(ccdExposure, sensorRef)
        if self.config.doSetBadRegions:
            self.setBadRegions(ccdExposure)

        self.maskAndInterpNan(ccdExposure)

        if self.config.qa.doWriteFlattened:
            sensorRef.put(ccdExposure, "flattenedImage")
        if self.config.qa.doThumbnailFlattened:
            self.writeThumbnail(sensorRef, "flattenedThumb", ccdExposure)

        self.measureBackground(ccdExposure)

        if self.config.doGuider:
            self.guider(ccdExposure)

        if self.config.doWrite:
            sensorRef.put(ccdExposure, "postISRCCD")
        
        self.display("postISRCCD", ccdExposure)

        return Struct(exposure=ccdExposure)

    def applyGains(self, ccdExposure, normalizeGains):
        ccd = afwCG.cast_Ccd(ccdExposure.getDetector())
        ccdImage = ccdExposure.getMaskedImage()

        medians = []
        for a in ccd:
            sim = ccdImage.Factory(ccdImage, a.getDataSec())
            sim *= a.getElectronicParams().getGain()

            if normalizeGains:
                medians.append(numpy.median(sim.getImage().getArray()))

        if normalizeGains:
            median = numpy.median(numpy.array(medians))
            for i, a in enumerate(ccd):
                sim = ccdImage.Factory(ccdImage, a.getDataSec())
                sim *= median/medians[i]

    def widenSaturationTrails(self, mask):
        """Grow the saturation trails by an amount dependent on the width of the trail"""

        extraGrowDict = {}
        for i in range(1, 6):
            extraGrowDict[i] = 0
        for i in range(6, 8):
            extraGrowDict[i] = 1
        for i in range(8, 10):
            extraGrowDict[i] = 3
        extraGrowMax = 4

        if extraGrowMax <= 0:
            return

        saturatedBit = mask.getPlaneBitMask('SAT')

        xmin, ymin = mask.getBBox(afwImage.PARENT).getMin()
        width = mask.getWidth()

        thresh = afwDetection.Threshold(saturatedBit, afwDetection.Threshold.BITMASK)
        fpList = afwDetection.FootprintSet(mask, thresh).getFootprints()

        for fp in fpList:
            for s in fp.getSpans():
                x0, x1 = s.getX0(), s.getX1()

                extraGrow = extraGrowDict.get(x1 - x0 + 1, extraGrowMax)
                if extraGrow > 0:
                    y = s.getY() - ymin
                    x0 -= xmin + extraGrow
                    x1 -= xmin - extraGrow

                    if x0 < 0: x0 = 0
                    if x1 >= width - 1: x1 = width - 1

                    for x in range(x0, x1 + 1):
                        mask.set(x, y, saturatedBit)

    def setBadRegions(self, exposure):
        """Set all BAD areas of the chip to the average of the rest of the exposure

        @param[in,out]  exposure    exposure to process; must include both DataSec and BiasSec pixels
        """
        if self.config.badStatistic == "MEDIAN":
            statistic = afwMath.MEDIAN
        elif self.config.badStatistic == "MEANCLIP":
            statistic = afwMath.MEANCLIP
        else:
            raise RuntimeError("Impossible method %s of bad region correction" % self.config.badStatistic)

        mi = exposure.getMaskedImage()
        mask = mi.getMask()
        BAD = mask.getPlaneBitMask("BAD")
        fs = afwDetection.FootprintSet(mask, afwDetection.createThreshold(BAD, "bitmask"))

        sctrl = afwMath.StatisticsControl()
        sctrl.setAndMask(BAD)
        value = afwMath.makeStatistics(mi, statistic, sctrl).getValue()

        afwDetection.setImageFromFootprintList(mi.getImage(), fs.getFootprints(), value)

        self.log.log(self.log.INFO, "Set %d BAD pixels to %.2f" %
                     (sum([f.getNpix() for f in fs.getFootprints()]), value))

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
        maskVal = exposure.getMaskedImage().getMask().getPlaneBitMask(["BAD","SAT","DETECTED"])
        statsControl.setAndMask(maskVal)
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

        good = numpy.where(numpy.isfinite(skyLevels))
        skyMedian = numpy.median(skyLevels[good])
        flatness =  (skyLevels[good] - skyMedian) / skyMedian
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


    def guider(self, exposure):
        raise NotImplementedError(
            "Guider shadow trimming is enabled but no generic implementation is present"
            )

    def linearize(self, exposure):
        """Correct for non-linearity

        @param exposure Exposure to process
        """
        assert exposure, "No exposure provided"

        image = exposure.getMaskedImage()

        ccd = afwCG.cast_Ccd(exposure.getDetector())

        linearized = False              # did we apply linearity corrections?
        for amp in ccd:
            linearity = amp.getElectronicParams().getLinearity()

            ampImage = image[amp.getDataSec()]
            width, height = ampImage.getDimensions()

            imageTypeMax = 65535        # should be a method on the image
            setSuspectPixels = linearity.maxCorrectable < imageTypeMax # there might be some

            if not setSuspectPixels and linearity.coefficient == 0.0:
                continue                # nothing to do

            linearized = True
            #
            # We may have a max correctable level even if we make no attempt to correct
            #
            if setSuspectPixels:
                afwDetection.FootprintSet(ampImage,
                                          afwDetection.Threshold(linearity.maxCorrectable), "SUSPECT")

            if linearity.type == afwCG.Linearity.PROPORTIONAL:
                if linearity.threshold != 0.0:
                    raise RuntimeError(
                        ("The threshold for PROPORTIONAL linearity corrections must be 0; saw %g" +
                         " for ccd %s amp %s") % (linearity.threshold, ccd.getId(), amp.getId()))
                
                ampArr = ampImage.getImage().getArray()
                ampArr += linearity.coefficient*ampArr
            else:
                raise NotImplementedError("Unimplemented linearity type: %d", linearity.type)

        if linearized:
            self.log.log(self.log.INFO, "Applying linearity corrections to Ccd %s" % (ccd.getId()))

    def maskDefect(self, ccdExposure, fwhm=1.0):
        """Mask defects using mask plane "BAD"

        @param[in,out]  ccdExposure     exposure to process
        @param          fwhm            fwhm to use when interpolating (arcsec)

        @warning: call this after CCD assembly, since defects may cross amplifier boundaries
        """
        maskedImage = ccdExposure.getMaskedImage()
        ccd = afwCG.cast_Ccd(ccdExposure.getDetector())
        defectBaseList = ccd.getDefects()
        defectList = measAlg.DefectListT()
        # mask bad pixels in the camera class
        # create master list of defects and add those from the camera class
        for d in defectBaseList:
            bbox = d.getBBox()
            nd = measAlg.Defect(bbox)
            defectList.append(nd)
        lsstIsr.maskPixelsFromDefectList(maskedImage, defectList, maskName='BAD')
        wcs = ccdExposure.getWcs()
        if wcs:
            fwhm /= wcs.pixelScale().asArcseconds()
        lsstIsr.interpolateDefectList(maskedImage, defectList, fwhm)

    def flatCorrection(self, exposure, dataRef):
        """Apply flat correction in-place

        This version allows tweaking the flat-field to match the observed
        ratios of background flux in the amplifiers.  This may be necessary
        if the gains drift or the levels are slightly affected by non-linearity
        or similar.  Note that this tweak may not work if there is significant
        structure in the image, and especially if the structure varies over
        the amplifiers.

        @param[in,out]  exposure        exposure to process
        @param[in]      dataRef         data reference at same level as exposure
        """
        flatfield = self.getDetrend(dataRef, "flat")

        if self.config.doTweakFlat:
            data = []
            flatData = []
            flatAmpList = []
            bad = exposure.getMaskedImage().getMask().getPlaneBitMask(["BAD", "SAT"])
            stats = afwMath.StatisticsControl()
            stats.setAndMask(bad)
            for amp in afwCG.cast_Ccd(exposure.getDetector()):
                box = amp.getDataSec(True)
                dataAmp = afwImage.MaskedImageF(exposure.getMaskedImage(), box, afwImage.LOCAL).clone()
                flatAmp = afwImage.MaskedImageF(flatfield.getMaskedImage(), box, afwImage.LOCAL)
                flatAmpList.append(flatAmp)
                dataAmp /= flatAmp
                data.append(afwMath.makeStatistics(dataAmp, afwMath.MEDIAN, stats).getValue())

            data = numpy.array(data)
            tweak = data/data.sum()*len(data)
            self.log.warn("Tweaking flat-field to match observed amplifier ratios: %s" % tweak)
            for i, flat in enumerate(flatAmpList):
                flat *= tweak[i]

        lsstIsr.flatCorrection(exposure.getMaskedImage(), flatfield.getMaskedImage(),
                               scalingType="USER", userScale=1.0)


class SuprimecamIsrConfig(SubaruIsrConfig):
    def setDefaults(self):
        super(SuprimecamIsrConfig, self).setDefaults()
        self.crosstalk.retarget(YagiCrosstalkTask)

class SuprimeCamIsrTask(SubaruIsrTask):
    ConfigClass = SuprimecamIsrConfig

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

class SuprimeCamMitIsrConfig(SubaruIsrTask.ConfigClass):
    pass

class SuprimeCamMitIsrTask(SubaruIsrTask):
    
    ConfigClass = SuprimeCamMitIsrConfig

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
