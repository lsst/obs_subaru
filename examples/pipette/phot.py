#!/usr/bin/env python

import numpy
import math

import lsst.afw.geom as afwGeom
import lsst.meas.algorithms as measAlg
import lsst.meas.utils.sourceDetection as muDetection
import lsst.meas.utils.sourceMeasurement as muMeasurement

import lsst.pipette.process as pipProc
import lsst.pipette.background as pipBackground

from lsst.pipette.timer import timecall

import lsst.meas.deblender.naive as deblendNaive

class Photometry(pipProc.Process):
    def __init__(self, thresholdMultiplier=1.0, Background=pipBackground.Background, *args, **kwargs):
        super(Photometry, self).__init__(*args, **kwargs)
        self._Background = Background
        self._thresholdMultiplier = thresholdMultiplier
        return

    def run(self, exposure, psf, apcorr=None, wcs=None):
        """Run photometry

        @param exposure Exposure to process
        @param psf PSF for photometry
        @param apcorr Aperture correction to apply
        @param wcs WCS to apply
        @return
        - sources: Measured sources
        - footprintSet: Set of footprints
        """
        assert exposure, "No exposure provided"
        assert psf, "No psf provided"

        self.imports()
        
        footprintSet = self.detect(exposure, psf)

        do = self.config['do']['phot']
        if do['background']:
            bg, exposure = self.background(exposure); del bg
        
        try:
            foots = footprintSet.getFootprints()
            peaks = []
            nPeaks = 0
            for foot in foots:
                pks = []
                for pk in foot.getPeaks():
                    pks.append(pk)
                    nPeaks += 1
                peaks.append(pks)

            # UTTER BS -- CPL
            bb = foots[0].getBBox()
            xc = int((bb.getMinX() + bb.getMaxX()) / 2.)
            yc = int((bb.getMinY() + bb.getMaxY()) / 2.)

            try:
                psf_fwhm = psf.getFwhm(xc, yc)
            except AttributeError, e:
                pa = measAlg.PsfAttributes(psf, xc, yc)
                psfw = pa.computeGaussianWidth(measAlg.PsfAttributes.ADAPTIVE_MOMENT)
                self.log.log(self.log.WARN, 'PSF width fixed at %0.1f because of: %s:' % (psfw, e))
                psf_fwhm = 2.35 * psfw

            mi = exposure.getMaskedImage()
            bootPrints = deblendNaive.deblend(foots, peaks, mi, psf, psf_fwhm)
            self.log.log(self.log.WARN, "deblending turned %d/%d footprints/peaks into %d objects: %s." % (len(foots), nPeaks,
                                                                                                           len(bootPrints), bootPrints))
        except Exception, e:
            self.log.log(self.log.WARN, "no deblending, sorry: %s" % (e))
            import pdb; pdb.set_trace()
            
        sources = self.measure(exposure, footprintSet, psf, apcorr=apcorr, wcs=wcs)

        self.display('phot', exposure=exposure, sources=sources, pause=True)
        return sources, footprintSet

    def imports(self):
        """Import modules (so they can register themselves)"""
        if self.config.has_key('imports'):
            for modName in self.config['imports']:
                try:
                    module = self.config['imports'][modName]
                    self.log.log(self.log.INFO, "Importing %s (%s)" % (modName, module))
                    exec("import " + module)
                except ImportError, err:
                    self.log.log(self.log.WARN, "Failed to import %s (%s): %s" % (modName, module, err))


    @timecall
    def background(self, exposure):
        """Background subtraction

        @param exposure Exposure to process
        @return Background, Background-subtracted exposure
        """
        background = self._Background(config=self.config, log=self.log)
        return background.run(exposure)


    @timecall
    def detect(self, exposure, psf):
        """Detect sources

        @param exposure Exposure to process
        @param psf PSF for detection
        @return Positive source footprints
        """
        assert exposure, "No exposure provided"
        assert psf, "No psf provided"
        policy = self.config['detect']
        posSources, negSources = muDetection.detectSources(exposure, psf, policy.getPolicy(),
                                                           extraThreshold=self._thresholdMultiplier)
        numPos = len(posSources.getFootprints()) if posSources is not None else 0
        numNeg = len(negSources.getFootprints()) if negSources is not None else 0
        if numNeg > 0:
            self.log.log(self.log.WARN, "%d negative sources found and ignored" % numNeg)
        self.log.log(self.log.INFO, "Detected %d sources to %g sigma." % (numPos, policy['thresholdValue']))
        return posSources

    @timecall
    def measure(self, exposure, footprintSet, psf, apcorr=None, wcs=None):
        """Measure sources

        @param exposure Exposure to process
        @param footprintSet Set of footprints to measure
        @param psf PSF for measurement
        @param apcorr Aperture correction to apply
        @param wcs WCS to apply
        @return Source list
        """
        assert exposure, "No exposure provided"
        assert footprintSet, "No footprintSet provided"
        assert psf, "No psf provided"
        policy = self.config['measure'].getPolicy()
        footprints = []                    # Footprints to measure
        num = len(footprintSet.getFootprints())
        self.log.log(self.log.INFO, "Measuring %d positive sources" % num)
        footprints.append([footprintSet.getFootprints(), False])
        sources = muMeasurement.sourceMeasurement(exposure, psf, footprints, policy)

        if wcs is not None:
            muMeasurement.computeSkyCoords(wcs, sources)

        if not self.config['do']['calibrate']['applyApcorr']: # actually apply the aperture correction?
            apcorr = None

        if apcorr is not None:
            self.log.log(self.log.INFO, "Applying aperture correction to %d sources" % len(sources))
            for source in sources:
                x, y = source.getXAstrom(), source.getYAstrom()

                for getter, getterErr, setter, setterErr in (
                    ('getPsfFlux', 'getPsfFluxErr', 'setPsfFlux', 'setPsfFluxErr'),
                    ('getInstFlux', 'getInstFluxErr', 'setInstFlux', 'setInstFluxErr'),
                    ('getModelFlux', 'getModelFluxErr', 'setModelFlux', 'setModelFluxErr')):
                    flux = getattr(source, getter)()
                    fluxErr = getattr(source, getterErr)()
                    if (numpy.isfinite(flux) and numpy.isfinite(fluxErr)):
                        corr, corrErr = apcorr.computeAt(x, y)
                        getattr(source, setter)(flux * corr)
                        getattr(source, setterErr)(numpy.sqrt(corr**2 * fluxErr**2 + corrErr**2 * flux**2))

        if self._display and self._display.has_key('psfinst') and self._display['psfinst']:
            import matplotlib.pyplot as plt
            psfMag = -2.5 * numpy.log10(numpy.array([s.getPsfFlux() for s in sources]))
            instMag = -2.5 * numpy.log10(numpy.array([s.getInstFlux() for s in sources]))
            fig = plt.figure(1)
            fig.clf()
            ax = fig.add_axes((0.1, 0.1, 0.8, 0.8))
            ax.set_autoscale_on(False)
            ax.set_ybound(lower=-1.0, upper=1.0)
            ax.set_xbound(lower=-17, upper=-7)
            ax.plot(psfMag, psfMag-instMag, 'ro')
            ax.axhline(0.0)
            ax.set_title('psf - inst')
            plt.show()

        return sources


class Rephotometry(Photometry):
    def run(self, exposure, footprints, psf, apcorr=None, wcs=None):
        """Photometer footprints that have already been detected

        @param exposure Exposure to process
        @param footprints Footprints to rephotometer
        @param psf PSF for photometry
        @param apcorr Aperture correction to apply
        @param wcs WCS to apply
        """
        return self.measure(exposure, footprints, psf, apcorr=apcorr, wcs=wcs)

    def background(self, exposure, psf):
        raise NotImplementedError("This method is deliberately not implemented: it should never be run!")

    def detect(self, exposure, psf):
        raise NotImplementedError("This method is deliberately not implemented: it should never be run!")



class PhotometryDiff(Photometry):
    def detect(self, exposure, psf):
        """Detect sources in a diff --- care about both positive and negative

        @param exposure Exposure to process
        @param psf PSF for detection
        @return Source footprints
        """
        assert exposure, "No exposure provided"
        assert psf, "No psf provided"
        policy = self.config['detect']
        posSources, negSources = muDetection.detectSources(exposure, psf, policy.getPolicy())
        numPos = len(posSources.getFootprints()) if posSources is not None else 0
        numNeg = len(negSources.getFootprints()) if negSources is not None else 0
        self.log.log(self.log.INFO, "Detected %d positive and %d negative sources to %g sigma." % 
                     (numPos, numNeg, policy['thresholdValue']))

        for f in negSources.getFootprints():
            posSources.getFootprints().push_back(f)

        return posSources
