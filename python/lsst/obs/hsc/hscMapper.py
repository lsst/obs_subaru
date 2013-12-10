#!/usr/bin/env python

import os
import pwd

from lsst.daf.butlerUtils import CameraMapper
import lsst.afw.image.utils as afwImageUtils
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.geom as afwGeom
import lsst.pex.policy as pexPolicy
from .hscLib import HscDistortion

try: # just to let meas_mosaic be an optional dependency
    from lsst.meas.mosaic import applyMosaicResults
except ImportError:
    applyMosaicResults = None

class HscMapper(CameraMapper):
    """Provides abstract-physical mapping for HSC data"""
    
    def __init__(self, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "HscMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)
        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'HSC')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        if not kwargs.get('calibRoot', None):
            kwargs['calibRoot'] = os.path.join(kwargs['root'], 'CALIB')

        super(HscMapper, self).__init__(policy, policyFile.getRepositoryPath(), **kwargs)
        
        # Ensure each dataset type of interest knows about the full range of keys available from the registry
        keys = {'field': str,
                'visit': int,
                'filter': str,
                'ccd': int,
                'dateObs': str,
                'taiObs': str,
                'expTime': float,
                'pointing': int,
                }
        for name in ("raw",
                     # processCcd outputs
                     "postISRCCD", "calexp", "postISRCCD", "src", "icSrc", "icMatch", "icMatchFull",
                     "srcMatch", "srcMatchFull",
                     # processCcd QA
                     "ossThumb", "flattenedThumb", "calexpThumb", "plotMagHist", "plotSeeingRough",
                     "plotSeeingRobust", "plotSeeingMap", "plotEllipseMap", "plotEllipticityMap",
                     "plotFwhmGrid", "plotEllipseGrid", "plotEllipticityGrid", "plotPsfSrcGrid",
                     "plotPsfModelGrid", "fitsFwhmGrid", "fitsEllipticityGrid", "fitsEllPaGrid",
                     "fitsPsfSrcGrid", "fitsPsfModelGrid", "tableSeeingMap", "tableSeeingGrid",
                     # forcedPhot outputs
                     "forced_src",
                     # Warp
                     "coaddTempExp",
                     ):
            self.mappings[name].keyDict.update(keys)

        # Distortion isn't pluggable, so we'll put in our own
        elevation = 45 * afwGeom.degrees
        distortion = HscDistortion(elevation)
        self.camera.setDistortion(distortion)
        
        # SDSS g': http://www.naoj.org/Observing/Instruments/SCam/txt/g.txt
        # SDSS r': http://www.naoj.org/Observing/Instruments/SCam/txt/r.txt
        # SDSS i': http://www.naoj.org/Observing/Instruments/SCam/txt/i.txt
        # SDSS z': http://www.naoj.org/Observing/Instruments/SCam/txt/z.txt
        # y-band: Shimasaku et al., 2005, PASJ, 57, 447

        afwImageUtils.resetFilters()
        afwImageUtils.defineFilter(name='g', lambdaEff=477, alias=['W-S-G+', 'HSC-G'])
        afwImageUtils.defineFilter(name='r', lambdaEff=623, alias=['W-S-R+', 'HSC-R'])
        afwImageUtils.defineFilter(name='i', lambdaEff=775, alias=['W-S-I+', 'HSC-I'])
        afwImageUtils.defineFilter(name='z', lambdaEff=925, alias=['W-S-Z+', 'HSC-Z'])
        afwImageUtils.defineFilter(name='y', lambdaEff=990, alias=['W-S-ZR', 'HSC-Y'])
        afwImageUtils.defineFilter(name='N921', lambdaEff=921, alias=['NB0921'])
        afwImageUtils.defineFilter(name='SH', lambdaEff=0, alias=['SH',])
        afwImageUtils.defineFilter(name='PH', lambdaEff=0, alias=['PH',])
        afwImageUtils.defineFilter(name='None', lambdaEff=0)
        afwImageUtils.defineFilter(name='Unrecognised', lambdaEff=0)

        self.filters = {
            "W-S-G+"  : "g",
            "W-S-R+"  : "r",
            "W-S-I+"  : "i",
            "W-S-Z+"  : "z",
            "W-S-ZR"  : "y",
            "HSC-G"   : "g",
            "HSC-R"   : "r",
            "HSC-I"   : "i",
            "HSC-Z"   : "z",
            "HSC-Y"   : "y",
            "SH"   : "None",
            "PH"   : "None",
            "NONE"    : "None",
            "UNRECOGNISED": "i",
            }

        # next line makes a dict that maps filter names to sequential integers (arbitrarily sorted),
        # for use in generating unique IDs for sources.
        self.filterIdMap = dict(zip(self.filters, range(len(self.filters))))

    def map(self, datasetType, dataId, write=False):
        """Need to strip 'flags' argument from map

        We want the 'flags' argument passed to the butler to work (it's
        used to change how the reading/writing is done), but want it
        removed from the mapper (because it doesn't correspond to a
        registry column).
        """
        copyId = dataId.copy()
        copyId.pop("flags", None)
        return super(HscMapper, self).map(datasetType, copyId, write=write)


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

    def std_raw_md(self, md, dataId):
        if False:            # no std_raw_md in baseclass
            md = super(HscMapper, self).std_raw_md(md, dataId) # not present in baseclass
        #
        # We need to flip the WCS defined by the metadata in case anyone ever constructs a Wcs from it
        #
        wcs = afwImage.makeWcs(md)
        self._flipChipsLR(None, wcs, dataId, dims=afwGeom.ExtentI(md.get("NAXIS1"), md.get("NAXIS2")))
        wcsR = afwImage.Wcs(wcs.getSkyOrigin().getPosition(), wcs.getPixelOrigin(), wcs.getCDMatrix()*0.992)
        wcsMd = wcsR.getFitsMetadata()

        for k in wcsMd.names():
            md.set(k, wcsMd.get(k))

        return md
    
    def std_raw(self, item, dataId):
        exp = super(HscMapper, self).std_raw(item, dataId)

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

    def build_ubercalexp(self, dataId, butler):
        if applyMosaicResults is None:
            raise RuntimeError("Cannot apply mosaic outputs to calexp: meas_mosaic could not be imported")
        dataRef = butler.dataRef("calexp", **dataId)
        return applyMosaicResults(dataRef)

    def _computeCoaddExposureId(self, dataId, singleFilter):
        """Compute the 64-bit (long) identifier for a coadd.

        @param dataId (dict)       Data identifier with tract and patch.
        @param singleFilter (bool) True means the desired ID is for a single- 
                                   filter coadd, in which case dataId
                                   must contain filter.
        """
        # FIXME: this bit allocation is for LSST's huge-tract skymaps, and needs
        # to be updated to something more appropriate for Subaru
        # (actually, it shouldn't be the mapper's job at all; see #2797).
        tract = long(dataId['tract'])
        if tract < 0 or tract >= 128:
            raise RuntimeError('tract not in range [0,128)')
        patchX, patchY = map(int, dataId['patch'].split(','))
        for p in (patchX, patchY):
            if p < 0 or p >= 2**13:
                raise RuntimeError('patch component not in range [0, 8192)')
        id = (tract * 2**13 + patchX) * 2**13 + patchY
        if singleFilter:
            return id * 8 + self.filterIdMap[dataId['filter']]
        return id

    def bypass_deepCoaddId(self, datasetType, pythonType, location, dataId):
        return self._computeCoaddExposureId(dataId, True)
    def bypass_deepCoaddId_bits(self, datasetType, pythonType, location, dataId):
        return 1 + 7 + 13*2 + 3

    # The following allow grabbing a 'psf' from the butler directly, without having to get it from a calexp
    def map_psf(self, dataId, write=False):
        if write:
            raise RuntimeError("Writing a psf directly is no longer permitted: write as part of a calexp")
        copyId = dataId.copy()
        copyId['bbox'] = afwGeom.Box2I(afwGeom.Point2I(0,0), afwGeom.Extent2I(1,1))
        return self.map_calexp_sub(copyId)
    def std_psf(self, calexp, dataId):
        return calexp.getPsf()

    @classmethod
    def getEupsProductName(cls):
        return "obs_subaru"

    @classmethod
    def getCameraName(cls):
        return "hsc"
