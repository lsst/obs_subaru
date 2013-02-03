#!/usr/bin/env python

import os
import pwd

from lsst.daf.butlerUtils import CameraMapper
import lsst.afw.image.utils as afwImageUtils
import lsst.afw.cameraGeom as cameraGeom
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

        for raft in self.camera:
            raft = cameraGeom.cast_Raft(raft)
            for ccd in raft:
                ccd = cameraGeom.cast_Ccd(ccd)
        
                if ccd.getId().getSerial() in range(100, 104):
                    w, h = ccd.getAllPixels(True).getDimensions()
                    xc, yc = ccd.getCenterPixel()
                    xc += h/2 if xc < 0 else -h/2
                    yc += h/2 if yc < 0 else -h/2
                    ccd.setCenterPixel(afwGeom.PointD(xc, yc))

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

    @staticmethod
    def _flipChipsLR(exp, wcs, dataId, dims=None):
        """Flip the chip left/right or top/bottom. Process either/and the pixels and wcs
Most chips are flipped L/R, but the rotated ones (100..103) are flipped T/B
        """
        flipLR, flipTB = (False, True) if dataId['ccd'] in (100, 101, 102, 103) else (True, False)
        if exp:
            exp.setMaskedImage(afwMath.flipImage(exp.getMaskedImage(), flipLR, flipTB))
        if wcs:
            wcs.flipImage(flipLR, flipTB, exp.getDimensions() if dims is None else dims)
        
        return exp

    def std_flat(self, exp, dataId):
        if False:                       # no std_flat in baseclass
            exp = super(HscSimMapper, self).std_flat(exp, dataId)

        return self._flipChipsLR(exp, exp.getWcs(), dataId)

    def std_raw_md(self, md, dataId):
        if False:            # no std_raw_md in baseclass
            md = super(HscSimMapper, self).std_raw_md(md, dataId) # not present in baseclass
        #
        # We need to flip the WCS defined by the metadata in case anyone ever constructs a Wcs from it
        #
        wcs = afwImage.makeWcs(md)
        self._flipChipsLR(None, wcs, dataId, dims=afwGeom.ExtentI(md.get("NAXIS1"), md.get("NAXIS2")))
        wcsMd = wcs.getFitsMetadata()

        for k in wcsMd.names():
            md.set(k, wcsMd.get(k))

        return md
    
    def std_raw(self, item, dataId):
        exp = super(HscSimMapper, self).std_raw(item, dataId)

        return self._flipChipsLR(exp, exp.getWcs(), dataId)
    
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
