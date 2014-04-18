#!/usr/bin/env python

import os
import pwd

from lsst.daf.butlerUtils import CameraMapper
import lsst.afw.image.utils as afwImageUtils
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.geom as afwGeom
import lsst.pex.policy as pexPolicy

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

        # SDSS g': http://www.naoj.org/Observing/Instruments/SCam/txt/g.txt
        # SDSS r': http://www.naoj.org/Observing/Instruments/SCam/txt/r.txt
        # SDSS i': http://www.naoj.org/Observing/Instruments/SCam/txt/i.txt
        # SDSS z': http://www.naoj.org/Observing/Instruments/SCam/txt/z.txt
        # y-band: Shimasaku et al., 2005, PASJ, 57, 447

        # The order of these defineFilter commands matters as their IDs are used to generate at least some
        # object IDs (e.g. on coadds) and changing the order will invalidate old objIDs

        afwImageUtils.resetFilters()
        afwImageUtils.defineFilter(name='None', lambdaEff=0,
                                   alias=["NONE", 'Unrecognised', 'UNRECOGNISED',])
        afwImageUtils.defineFilter(name='g', lambdaEff=477, alias=['W-S-G+', 'HSC-G'])
        afwImageUtils.defineFilter(name='r', lambdaEff=623, alias=['W-S-R+', 'HSC-R'])
        afwImageUtils.defineFilter(name='r1', lambdaEff=623, alias=['109', 'ENG-R1'])
        afwImageUtils.defineFilter(name='i', lambdaEff=775, alias=['W-S-I+', 'HSC-I'])
        afwImageUtils.defineFilter(name='z', lambdaEff=925, alias=['W-S-Z+', 'HSC-Z'])
        afwImageUtils.defineFilter(name='y', lambdaEff=990, alias=['W-S-ZR', 'HSC-Y'])
        afwImageUtils.defineFilter(name='N921', lambdaEff=921, alias=['NB0921'])
        afwImageUtils.defineFilter(name='SH', lambdaEff=0, alias=['SH',])
        afwImageUtils.defineFilter(name='PH', lambdaEff=0, alias=['PH',])
        #
        # self.filters is used elsewhere, and for now we'll set it
        #
        # It's a bit hard to initialise self.filters properly until #2113 is resolved,
        # including the part that makes it possible to get all aliases
        #
        self.filters = {}
        for f in [
            "W-S-G+",
            "W-S-R+",
            "W-S-I+",
            "W-S-Z+",
            "W-S-ZR",
            "HSC-G",
            "HSC-R",
            "HSC-I",
            "HSC-Z",
            "HSC-Y",
            "ENG-R1",
            "SH",
            "PH",
            "NONE",
            "UNRECOGNISED"]:
            # Get the canonical name -- see #2113
            self.filters[f] = afwImage.Filter(afwImage.Filter(f).getId()).getName()
        #
        # The number of bits allocated for fields in object IDs, appropriate for
        # the default-configured Rings skymap.
        #
        # This shouldn't be the mapper's job at all; see #2797.

        HscMapper._nbit_tract = 16
        HscMapper._nbit_patch  = 5
        HscMapper._nbit_filter = 6

        HscMapper._nbit_id = 64 - (HscMapper._nbit_tract + 2*HscMapper._nbit_patch + HscMapper._nbit_filter)

        if len(afwImage.Filter.getNames()) >= 2**HscMapper._nbit_filter:
            raise RuntimeError("You have more filters defined than fit into the %d bits allocated" %
                               HscMapper._nbit_filter)

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

    def std_dark(self, item, dataId):
        parent = super(HscMapper, self)
        if hasattr(parent, "std_dark"):
            exp = parent.std_dark(item, dataId)
        else:
            exp = self._standardizeExposure(self.calibrations["dark"], item, dataId)
        exp.getCalib().setExptime(1.0)
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

    def _computeCoaddExposureId(self, dataId, singleFilter):
        """Compute the 64-bit (long) identifier for a coadd.

        @param dataId (dict)       Data identifier with tract and patch.
        @param singleFilter (bool) True means the desired ID is for a single- 
                                   filter coadd, in which case dataId
                                   must contain filter.
        """

        tract = long(dataId['tract'])
        if tract < 0 or tract >= 2**HscMapper._nbit_tract:
            raise RuntimeError('tract not in range [0,%d)' % (2**HscMapper._nbit_tract))
        patchX, patchY = map(int, dataId['patch'].split(','))
        for p in (patchX, patchY):
            if p < 0 or p >= 2**HscMapper._nbit_patch:
                raise RuntimeError('patch component not in range [0, %d)' % 2**HscMapper._nbit_patch)
        oid = (((tract << HscMapper._nbit_patch) + patchX) << HscMapper._nbit_patch) + patchY
        if singleFilter:
            return (oid << HscMapper._nbit_filter) + afwImage.Filter(dataId['filter']).getId()
        return oid

    def bypass_eups_versions(self, *args, **kwargs):
        return ""
    def bypass_eups_versions_tmp(self, *args, **kwargs):
        return ""

    def bypass_deepCoaddId_bits(self, *args, **kwargs):
        """The number of bits used up for patch ID bits"""
        return 64 - HscMapper._nbit_id

    def bypass_deepCoaddId(self, datasetType, pythonType, location, dataId):
        return self._computeCoaddExposureId(dataId, True)

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
