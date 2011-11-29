#!/usr/bin/env python

import lsst.pipette.process as pipProc
import lsst.pipette.isr as pipIsr
import lsst.pipette.calibrate as pipCalib
if 'craig' is not 'good':
    import phot as pipPhot
else:
    import lsst.pipette.phot as pipPhot

from lsst.pipette.timer import timecall

class ProcessCcd(pipProc.Process):
    def __init__(self, Isr=pipIsr.Isr, Calibrate=pipCalib.Calibrate, Photometry=pipPhot.Photometry,
                 *args, **kwargs):
        """Initialise

        @param Isr Process to do ISR
        @param Calibrate Process to do calibration
        @param Photometry Process to do photometry
        """
        super(ProcessCcd, self).__init__(*args, **kwargs)
        self._Isr = Isr
        self._Calibrate = Calibrate
        self._Photometry = Photometry
        
    @timecall
    def run(self, exposureList, detrendsList=None):
        """Process a CCD.

        @param exposureList List of exposures (each is an amp from the same exposure)
        @param detrendsList List of detrend dicts (each for an amp in the CCD; with bias, dark, flat, fringe)
        @return
        - exposure: Merged exposure from input list
        - psf: Point spread function
        - apcorr: Aperture correction
        - brightSources: Sources used in calibration (typically bright)
        - sources: Fainter sources measured on the image
        - matches: Astrometric matches
        - matchMeta: Metadata for astrometric matches
        """
        assert exposureList and len(exposureList) > 0, "exposureList not provided"

        if self.config['do']['isr']['enabled']:
            exposure, defects, background = self.isr(exposureList, detrendsList)
        else:
            assert len(exposureList) == 1 # no ISR so the exposureList is just the calexp
            exposure, defects, background = exposureList[0], None, None
            
        psf, apcorr, brightSources, matches, matchMeta = self.calibrate(exposure, defects=defects)
            
        if self.config['do']['phot']['enabled']:
            sources, footprints = self.phot(exposure, psf, apcorr, wcs=exposure.getWcs())
        else:
            sources, footprints = None, None

        return exposure, psf, apcorr, brightSources, sources, matches, matchMeta
            
    @timecall
    def runFromData(self, exposure, psf, apcorr, brightSources, matches, matchMeta):
        """ Same as run(), but we are provided the calibration outputs. """
        
        if self.config['do']['phot']['enabled']:
            sources, footprints = self.phot(exposure, psf, apcorr, wcs=exposure.getWcs())
        else:
            sources, footprints = None, None

        return exposure, psf, apcorr, brightSources, sources, matches, matchMeta
            
    
    @timecall
    def isr(self, exposureList, detrendsList):
        """Perform Instrumental Signature Removal

        @param exposureList List of exposures (each is an amp from the same exposure)
        @param detrendsList List of detrend dicts (each for an amp in the CCD; with bias, dark, flat, fringe)
        """
        isr = self._Isr(config=self.config, log=self.log)
        return isr.run(exposureList, detrendsList)

    @timecall
    def calibrate(self, *args, **kwargs):
        calibrate = self._Calibrate(config=self.config, log=self.log)
        return calibrate.run(*args, **kwargs)

    @timecall
    def phot(self, exposure, psf, apcorr, wcs=None):
        phot = self._Photometry(config=self.config, log=self.log)
        return phot.run(exposure, psf, apcorr, wcs=wcs)
