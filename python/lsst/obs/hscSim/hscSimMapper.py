#!/usr/bin/env python

import os
import pwd

from lsst.daf.butlerUtils import CameraMapper
import lsst.afw.image.utils as afwImageUtils
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.geom as afwGeom
import lsst.pex.policy as pexPolicy
from .hscSimLib import HscDistortion

class HscSimMapper(CameraMapper):
    """Provides abstract-physical mapping for HSC Simulation data"""
    
    def __init__(self, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "HscSimMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)
        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'HSC')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        if not kwargs.get('calibRoot', None):
            kwargs['calibRoot'] = os.path.join(kwargs['root'], 'CALIB')

        super(HscSimMapper, self).__init__(policy, policyFile.getRepositoryPath(), **kwargs)
        
        # Distortion isn't pluggable, so we'll put in our own
        elevation = 45 * afwGeom.degrees
        distortion = HscDistortion(elevation)
        self.camera.setDistortion(distortion)
        
        # SDSS g': http://www.naoj.org/Observing/Instruments/SCam/txt/g.txt
        # SDSS r': http://www.naoj.org/Observing/Instruments/SCam/txt/r.txt
        # SDSS i': http://www.naoj.org/Observing/Instruments/SCam/txt/i.txt
        # SDSS z': http://www.naoj.org/Observing/Instruments/SCam/txt/z.txt
        # y-band: Shimasaku et al., 2005, PASJ, 57, 447

        afwImageUtils.defineFilter(name='g', lambdaEff=477, alias=['W-S-G+'])
        afwImageUtils.defineFilter(name='r', lambdaEff=623, alias=['W-S-R+'])
        afwImageUtils.defineFilter(name='i', lambdaEff=775, alias=['W-S-I+'])
        afwImageUtils.defineFilter(name='z', lambdaEff=925, alias=['W-S-Z+'])
        afwImageUtils.defineFilter(name='y', lambdaEff=990, alias=['W-S-ZR'])
        
        self.filters = {
            "W-J-B"   : "B",
            "W-S-G+"  : "g",
            "W-S-R+"  : "r",
            "W-S-I+"  : "i",
            "W-S-Z+"  : "z",
            "W-S-ZR"  : "y",
            }

    def std_camera(self, item, dataId):
        """Standardize a camera dataset by converting it to a camera object."""
        return self.camera

    def std_raw(self, item, dataId):
        """We flip the pixels left - right to give them on-sky orientation

N.b. This only works because the data and bias secs are symmetrical about the centre line
of the detector.  We flip chips 100--103 about their short axis, and I'm worried that this
may not be true;  but they aren't very useful chips anyway...
        """
        
        exp = super(HscSimMapper, self).std_raw(item, dataId)

        flipLR, flipTB = (False, True) if dataId['ccd'] in (100, 101, 102, 103) else (True, False)
        exp.setMaskedImage(afwMath.flipImage(exp.getMaskedImage(), flipLR, flipTB))
        exp.getWcs().flipImage(flipLR, flipTB, exp.getDimensions())
        
        return exp
    
    def _extractAmpId(self, dataId):
        return (self._extractDetectorName(dataId), 0, 0)

    def _extractDetectorName(self, dataId):
        return int("%(ccd)d" % dataId)

    def _computeCcdExposureId(self, dataId):
        """Compute the 64-bit (long) identifier for a CCD exposure.

        @param dataId (dict) Data identifier with visit, ccd
        """
        pathId = self._transformId(dataId)
        visit = pathId['visit']
        ccd = pathId['ccd']
        return visit*200 + ccd

    def bypass_ccdExposureId(self, datasetType, pythonType, location, dataId):
        return self._computeCcdExposureId(dataId)

    def bypass_ccdExposureId_bits(self, datasetType, pythonType, location, dataId):
        """How many bits are required for the maximum exposure ID"""
        return 32 # just a guess, but this leaves plenty of space for sources
