#!/usr/bin/env python

import math
import numpy

import lsst.pex.logging as pexLog
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.sdqa as sdqa
import lsst.meas.algorithms as measAlg
import lsst.meas.algorithms.apertureCorrection as maApCorr
import lsst.meas.astrom as measAst
import lsst.meas.astrom.sip as astromSip
import lsst.meas.astrom.verifyWcs as astromVerify
import lsst.meas.photocal as photocal
import lsst.pipette.util as pipUtil
import lsst.pipette.process as pipProc
import lsst.pipette.repair as pipRepair
import lsst.pipette.phot as pipPhot
import lsst.pipette.background as pipBackground
import lsst.pipette.distortion as pipDist
import lsst.pipette.config as pipConfig

from lsst.pipette.timer import timecall


def propagateFlag(flag, old, new):
    """Propagate a flag from one source to another"""
    if old.getFlagForDetection() & flag:
        new.setFlagForDetection(new.getFlagForDetection() | flag)


class Calibrate(pipProc.Process):
    def __init__(self, Repair=pipRepair.Repair, Photometry=pipPhot.Photometry,
                 Background=pipBackground.Background, Rephotometry=pipPhot.Rephotometry,
                 *args, **kwargs):
        super(Calibrate, self).__init__(*args, **kwargs)
        self._Repair = Repair
        self._Photometry = Photometry
        self._Background = Background
        self._Rephotometry = Rephotometry

    def run(self, exposure, defects=None, background=None):
        """Calibrate an exposure: PSF, astrometry and photometry

        @param exposure Exposure to calibrate
        @param defects List of defects on exposure
        @param background Background model
        @return
        - psf: Point spread function
        - apcorr: Aperture correction
        - sources: Sources used in calibration
        - matches: Astrometric matches
        - matchMeta: Metadata for astrometric matches
        """
        assert exposure is not None, "No exposure provided"

        do = self.config['do']['calibrate']

        psf, wcs = self.fakePsf(exposure)

        self.repair(exposure, psf, defects=defects, preserve=True)

        if do['psf'] or do['astrometry'] or do['zeropoint']:
            sources, footprints = self.phot(exposure, psf)
        else:
            sources, footprints = None, None

        if do['psf']:
            psf, cellSet = self.psf(exposure, sources)
        else:
            psf, cellSet = None, None

        if do['psf'] and do['apcorr']:
            apcorr = self.apCorr(exposure, cellSet) # calculate the aperture correction; we may use it later
        else:
            apcorr = None

        # Wash, rinse, repeat with proper PSF

        if do['psf']:
            self.repair(exposure, psf, defects=defects, preserve=False)

        if do['background']:
            self.background(exposure, footprints=footprints, background=background)

        if do['psf'] and (do['astrometry'] or do['zeropoint']):
            newSources = self.rephot(exposure, footprints, psf, apcorr=apcorr)
            for old, new in zip(sources, newSources):
                for flag in (measAlg.Flags.STAR, measAlg.Flags.PSFSTAR):
                    propagateFlag(flag, old, new)
            sources = newSources;  del newSources

        if do['distortion']:
            dist = self.distortion(exposure)
        else:
            dist = None

        if do['astrometry'] or do['zeropoint']:
            distSources, llc, size = self.distort(exposure, sources, distortion=dist)
            matches, matchMeta = self.astrometry(exposure, sources, distSources,
                                                 distortion=dist, llc=llc, size=size)
            self.undistort(exposure, sources, matches, distortion=dist)
            self.verifyAstrometry(exposure, matches)
        else:
            matches, matchMeta = None, None

        if matches is not None and do['colorterms']:
            self.colorterms(exposure, matches, matchMeta)

        if do['zeropoint']:
            self.zeropoint(exposure, matches)

        self.display('calibrate', exposure=exposure, sources=sources, matches=matches)
        return psf, apcorr, sources, matches, matchMeta



    def fakePsf(self, exposure):
        """Initialise the calibration procedure by setting the PSF and WCS

        @param exposure Exposure to process
        @return PSF, WCS
        """
        assert exposure, "No exposure provided"
        
        wcs = exposure.getWcs()
        assert wcs, "No wcs in exposure"

        calibrate = self.config['calibrate']
        model = calibrate['model']
        fwhm = calibrate['fwhm'] / wcs.pixelScale().asArcseconds()
        size = calibrate['size']
        psf = afwDet.createPsf(model, size, size, fwhm/(2*math.sqrt(2*math.log(2))))
        return psf, wcs


    @timecall
    def repair(self, exposure, psf, defects=None, preserve=False):
        """Repair CCD problems (defects, CRs)

        @param exposure Exposure to process
        @param psf Point Spread Function
        @param defects Defect list, or None
        @param preserve Preserve bad pixels?
        """
        repair = self._Repair(keepCRs=preserve, config=self.config, log=self.log)
        repair.run(exposure, psf, defects=defects)


    @timecall
    def phot(self, exposure, psf, apcorr=None):
        """Perform photometry

        @param exposure Exposure to process
        @param psf Point Spread Function
        @param apcorr Aperture correction, or None
        @param wcs World Coordinate System, or None
        @return Source list
        """
        thresholdMultiplier = self.config['calibrate']['thresholdValue']
        phot = self._Photometry(config=self.config, log=self.log, thresholdMultiplier=thresholdMultiplier)
        return phot.run(exposure, psf)


    @timecall
    def psf(self, exposure, sources):
        """Measure the PSF

        @param exposure Exposure to process
        @param sources Measured sources on exposure
        """
        assert exposure, "No exposure provided"
        assert sources, "No sources provided"
        psfPolicy = self.config['psf']
        selName   = psfPolicy['selectName']
        selPolicy = psfPolicy['select'].getPolicy()
        algName   = psfPolicy['algorithmName']
        algPolicy = psfPolicy['algorithm'].getPolicy()
        self.log.log(self.log.INFO, "Measuring PSF")

        #
        # Run an extra detection step to mask out faint stars
        #
        if False:
            print "RHL is cleaning faint sources"

            import lsst.afw.math as afwMath

            sigma = 1.0
            gaussFunc = afwMath.GaussianFunction1D(sigma)
            gaussKernel = afwMath.SeparableKernel(15, 15, gaussFunc, gaussFunc)

            im = exposure.getMaskedImage().getImage()
            convolvedImage = im.Factory(im.getDimensions())
            afwMath.convolve(convolvedImage, im, gaussKernel)
            del im

            fs = afwDet.makeFootprintSet(convolvedImage, afwDet.createThreshold(4, "stdev"))
            fs = afwDet.makeFootprintSet(fs, 3, True)
            fs.setMask(exposure.getMaskedImage().getMask(), "DETECTED")

        starSelector = measAlg.makeStarSelector(selName, selPolicy)
        psfCandidateList = starSelector.selectStars(exposure, sources)

        psfDeterminer = measAlg.makePsfDeterminer(algName, algPolicy)
        psf, cellSet = psfDeterminer.determinePsf(exposure, psfCandidateList)

        # The PSF candidates contain a copy of the source, and so we need to explicitly propagate new flags
        for cand in psfCandidateList:
            cand = measAlg.cast_PsfCandidateF(cand)
            src = cand.getSource()
            if src.getFlagForDetection() & measAlg.Flags.PSFSTAR:
                ident = src.getId()
                src = sources[ident]
                assert src.getId() == ident
                src.setFlagForDetection(src.getFlagForDetection() | measAlg.Flags.PSFSTAR)

        exposure.setPsf(psf)
        return psf, cellSet


    @timecall
    def apCorr(self, exposure, cellSet):
        """Measure aperture correction

        @param exposure Exposure to process
        @param cellSet Set of cells of PSF stars
        """
        assert exposure, "No exposure provided"
        assert cellSet, "No cellSet provided"
        policy = self.config['apcorr'].getPolicy()
        control = maApCorr.ApertureCorrectionControl(policy)

        class FakeSdqa(dict):
            def set(self, name, value):
                self[name] = value
            
        sdqaRatings = FakeSdqa()
                
        corr = maApCorr.ApertureCorrection(exposure, cellSet, sdqaRatings, control, self.log)
        x, y = exposure.getWidth() / 2.0, exposure.getHeight() / 2.0
        value, error = corr.computeAt(x, y)
        self.log.log(self.log.INFO, "Aperture correction using %d/%d stars: %f +/- %f" %
                     (sdqaRatings["numAvailStars"], sdqaRatings["numGoodStars"], value, error))
        return corr


    @timecall
    def background(self, exposure, footprints=None, background=None):
        """Subtract background from exposure.

        @param exposure Exposure to process
        @param footprints Source footprints to mask
        @param background Background to restore before subtraction
        """
        if background is not None:
            if isinstance(background, afwMath.mathLib.Background):
                background = value.getImageF()
            exposure += background

        # Mask footprints on exposure
        if footprints is not None:
            # XXX Not implemented --- not sure how to do this in a way background subtraction will respect
            pass
        
        # Subtract background
        bg = self._Background(config=self.config, log=self.log)
        bg.run(exposure)


    @timecall
    def rephot(self, exposure, footprints, psf, apcorr=None):
        """Rephotometer exposure

        @param exposure Exposure to process
        @param footprints Footprints to rephotometer
        @param psf Point Spread Function
        @param apcorr Aperture correction, or None
        @param wcs World Coordinate System, or None
        @return Source list
        """
        rephot = self._Rephotometry(config=self.config, log=self.log)
        return rephot.run(exposure, footprints, psf, apcorr)


    def distortion(self, exposure):
        """Generate appropriate optical distortion

        @param exposure Exposure from which to get CCD
        @return Distortion
        """
        assert exposure, "No exposure provided"
        ccd = pipUtil.getCcd(exposure)
        dist = pipDist.createDistortion(ccd, self.config['distortion'])
        return dist


    @timecall
    def distort(self, exposure, sources, distortion=None):
        """Distort source positions before solving astrometry

        @param exposure Exposure to process
        @param sources Sources with undistorted (actual) positions
        @param distortion Distortion to apply
        @return Distorted sources, lower-left corner, size of distorted image
        """
        assert exposure, "No exposure provided"
        assert sources, "No sources provided"

        if distortion is not None:
            self.log.log(self.log.INFO, "Applying distortion correction.")
            distSources = distortion.actualToIdeal(sources)
            
            # Get distorted image size so that astrometry_net does not clip.
            xMin, xMax, yMin, yMax = float("INF"), float("-INF"), float("INF"), float("-INF")
            for x, y in ((0.0, 0.0), (0.0, exposure.getHeight()), (exposure.getWidth(), 0.0),
                         (exposure.getWidth(), exposure.getHeight())):
                point = afwGeom.Point2D(x, y)
                point = distortion.actualToIdeal(point)
                x, y = point.getX(), point.getY()
                if x < xMin: xMin = x
                if x > xMax: xMax = x
                if y < yMin: yMin = y
                if y > yMax: yMax = y
            xMin = int(xMin)
            yMin = int(yMin)
            llc = (xMin, yMin)
            size = (int(xMax - xMin + 0.5), int(yMax - yMin + 0.5))
            for s in distSources:
                s.setXAstrom(s.getXAstrom() - xMin)
                s.setYAstrom(s.getYAstrom() - yMin)
        else:
            distSources = sources
            size = (exposure.getWidth(), exposure.getHeight())
            llc = (0, 0)

        self.display('distortion', exposure=exposure, sources=distSources, pause=True)
        return distSources, llc, size


    @timecall
    def astrometry(self, exposure, sources, distSources, distortion=None, llc=(0,0), size=None):
        """Solve astrometry to produce WCS

        @param exposure Exposure to process
        @param sources Sources as measured (actual) positions
        @param distSources Sources with undistorted (ideal) positions
        @param distortion Distortion model
        @param llc Lower left corner (minimum x,y)
        @param size Size of exposure
        @return Star matches, match metadata
        """
        assert exposure, "No exposure provided"
        assert distSources, "No sources provided"

        self.log.log(self.log.INFO, "Solving astrometry")

        if size is None:
            size = (exposure.getWidth(), exposure.getHeight())

        try:
            menu = self.config['filters']
            filterName = menu[exposure.getFilter().getName()]
            if isinstance(filterName, pipConfig.Config):
                filterName = filterName['primary']
            self.log.log(self.log.INFO, "Using catalog filter: %s" % filterName)
        except:
            self.log.log(self.log.WARN, "Unable to determine catalog filter from lookup table using %s" %
                         exposure.getFilter().getName())
            filterName = None

        if distortion is not None:
            # Removed distortion, so use low order
            oldOrder = self.config['astrometry']['sipOrder']
            self.config['astrometry']['sipOrder'] = 2

        log = pexLog.Log(self.log, "astrometry")
        astrom = measAst.determineWcs(self.config['astrometry'].getPolicy(), exposure, distSources,
                                      log=log, forceImageSize=size, filterName=filterName)

        if distortion is not None:
            self.config['astrometry']['sipOrder'] = oldOrder

        if astrom is None:
            raise RuntimeError("Unable to solve astrometry for %s", exposure.getDetector().getId())

        wcs = astrom.getWcs()
        matches = astrom.getMatches()
        matchMeta = astrom.getMatchMetadata()
        if matches is None or len(matches) == 0:
            raise RuntimeError("No astrometric matches for %s", exposure.getDetector().getId())
        self.log.log(self.log.INFO, "%d astrometric matches for %s" % \
                     (len(matches), exposure.getDetector().getId()))
        exposure.setWcs(wcs)

        # Apply WCS to sources
        for index, source in enumerate(sources):
            distSource = distSources[index]
            sky = wcs.pixelToSky(distSource.getXAstrom() - llc[0], distSource.getYAstrom() - llc[1])
            source.setRaDec(sky)

            #point = afwGeom.Point2D(distSource.getXAstrom() - llc[0], distSource.getYAstrom() - llc[1])
            # in square degrees
            #areas.append(wcs.pixArea(point))

        self.display('astrometry', exposure=exposure, sources=sources, matches=matches)

        return matches, matchMeta

    def colorterms(self, exposure, matches, matchMeta):
        natural = exposure.getFilter().getName() # Natural band
        filterData = self.config['filters']
        if not isinstance(filterData, pipConfig.Config):
            # No data to do anything
            return
        filterData = filterData[natural]
        if not isinstance(filterData, pipConfig.Config):
            # No data to do anything
            return
        primary = filterData['primary'] # Primary band for correction
        secondary = filterData['secondary'] # Secondary band for correction

        polyData = filterData.getPolicy().getDoubleArray('polynomial') # Polynomial correction
        polyData.reverse()            # Numpy wants decreasing powers
        polynomial = numpy.poly1d(polyData)

        policy = self.config['astrometry'].getPolicy()

        # We already have the 'primary' magnitudes in the matches
        secondaries = measAst.readReferenceSourcesFromMetadata(matchMeta, log=self.log, policy=policy,
                                                               filterName=secondary)
        secondariesDict = dict()
        for s in secondaries:
            secondariesDict[s.getId()] = (s.getPsfFlux(), s.getPsfFluxErr())
        del secondaries

        polyString = ["%f (%s-%s)^%d" % (polynomial[order+1], primary, secondary, order+1) for
                      order in range(polynomial.order)]
        self.log.log(self.log.INFO, "Adjusting reference magnitudes: %f + %s" % (polynomial[0],
                                                                                 " + ".join(polyString)))

        for m in matches:
            index = m.first.getId()
            primary = -2.5 * math.log10(m.first.getPsfFlux())
            primaryErr = m.first.getPsfFluxErr()
            
            secondary = -2.5 * math.log10(secondariesDict[index][0])
            secondaryErr = secondariesDict[index][1]

            diff = polynomial(primary - secondary)
            m.first.setPsfFlux(math.pow(10.0, -0.4*(primary + diff)))
            # XXX Ignoring the error for now


    @timecall
    def undistort(self, exposure, sources, matches, distortion=None):
        """Undistort matches after solving astrometry, resolving WCS

        @param exposure Exposure of interest
        @param sources Sources on image (no distortion applied)
        @param matches Astrometric matches
        @param distortion Distortion model
        """
        assert exposure, "No exposure provided"
        assert sources, "No sources provided"
        assert matches, "No matches provided"

        if distortion is None:
            # No need to do anything
            return

        # Undo distortion in matches
        self.log.log(self.log.INFO, "Removing distortion correction.")
        # Undistort directly, assuming:
        # * astrometry matching propagates the source identifier (to get original x,y)
        # * distortion is linear on very very small scales (to get x,y of catalogue)
        for m in matches:
            dx = m.first.getXAstrom() - m.second.getXAstrom()
            dy = m.first.getYAstrom() - m.second.getYAstrom()
            orig = sources[m.second.getId()]
            m.second.setXAstrom(orig.getXAstrom())
            m.second.setYAstrom(orig.getYAstrom())
            m.first.setXAstrom(m.second.getXAstrom() + dx)
            m.first.setYAstrom(m.second.getYAstrom() + dy)

        # Re-fit the WCS with the distortion undone
        if self.config['astrometry']['calculateSip']:
            self.log.log(self.log.INFO, "Refitting WCS with distortion removed")
            sip = astromSip.CreateWcsWithSip(matches, exposure.getWcs(), self.config['astrometry']['sipOrder'])
            wcs = sip.getNewWcs()
            self.log.log(self.log.INFO, "Astrometric scatter: %f arcsec (%s non-linear terms)" %
                         (sip.getScatterOnSky().asArcseconds(), "with" if wcs.hasDistortion() else "without"))
            exposure.setWcs(wcs)
            
            # Apply WCS to sources
            for index, source in enumerate(sources):
                sky = wcs.pixelToSky(source.getXAstrom(), source.getYAstrom())
                source.setRaDec(sky)
        else:
            self.log.log(self.log.WARN, "Not calculating a SIP solution; matches may be suspect")
        
        self.display('astrometry', exposure=exposure, sources=sources, matches=matches)


    def verifyAstrometry(self, exposure, matches):
        """Verify astrometry solution

        @param exposure Exposure of interest
        @param matches Astrometric matches
        """
        verify = dict()                    # Verification parameters
        verify.update(astromSip.sourceMatchStatistics(matches))
        verify.update(astromVerify.checkMatches(matches, exposure, log=self.log))
        for k, v in verify.items():
            exposure.getMetadata().set(k, v)


    @timecall
    def zeropoint(self, exposure, matches):
        """Photometric calibration

        @param exposure Exposure to process
        @param matches Matched sources
        """
        assert exposure, "No exposure provided"
        assert matches, "No matches provided"

        zp = photocal.calcPhotoCal(matches, log=self.log, goodFlagValue=0)
        self.log.log(self.log.INFO, "Photometric zero-point: %f" % zp.getMag(1.0))
        exposure.getCalib().setFluxMag0(zp.getFlux(0))
        return


class CalibratePsf(Calibrate):
    """Calibrate only the PSF for an image.
    Explicitly turns off other functions.
    """
    def run(self, *args, **kwargs):
        oldDo = self.config['do']['calibrate'].copy()
        newDo = self.config['do']['calibrate']

        newDo['background'] = False
        newDo['distortion'] = False
        newDo['astrometry'] = False
        newDo['zeropoint'] = False

        psf, apcorr, sources, matches, matchMeta = super(CalibratePsf, self).run(*args, **kwargs)

        self.config['do']['calibrate'] = oldDo

        return psf, apcorr, sources
