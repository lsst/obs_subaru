import os

import lsst.log
from lsst.obs.base import CameraMapper
from lsst.daf.persistence import ButlerLocation, Policy
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.geom as afwGeom
import lsst.geom as geom
from lsst.ip.isr import LinearizeSquared
import lsst.pex.exceptions
from .makeHscRawVisitInfo import MakeHscRawVisitInfo
from .hscPupil import HscPupilFactory
from .hscFilters import HSC_FILTER_DEFINITIONS


class HscMapper(CameraMapper):
    """Provides abstract-physical mapping for HSC data"""
    packageName = "obs_subaru"

    MakeRawVisitInfoClass = MakeHscRawVisitInfo

    PupilFactoryClass = HscPupilFactory

    _cameraCache = None  # Camera object, cached to speed up instantiation time

    @classmethod
    def addFilters(cls):
        HSC_FILTER_DEFINITIONS.defineFilters()

    def __init__(self, **kwargs):
        policyFile = Policy.defaultPolicyFile("obs_subaru", "HscMapper.yaml", "policy")
        policy = Policy(policyFile)
        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'HSC')
            except Exception:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        if not kwargs.get('calibRoot', None):
            calibSearch = [os.path.join(kwargs['root'], 'CALIB')]
            if "repositoryCfg" in kwargs:
                calibSearch += [os.path.join(cfg.root, 'CALIB') for cfg in kwargs["repositoryCfg"].parents if
                                hasattr(cfg, "root")]
                calibSearch += [cfg.root for cfg in kwargs["repositoryCfg"].parents if hasattr(cfg, "root")]
            for calibRoot in calibSearch:
                if os.path.exists(os.path.join(calibRoot, "calibRegistry.sqlite3")):
                    kwargs['calibRoot'] = calibRoot
                    break
            if not kwargs.get('calibRoot', None):
                lsst.log.Log.getLogger("HscMapper").warn("Unable to find calib root directory")

        super(HscMapper, self).__init__(policy, os.path.dirname(policyFile), **kwargs)

        self._linearize = LinearizeSquared()

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
                     "postISRCCD", "calexp", "postISRCCD", "src", "icSrc", "icMatch",
                     "srcMatch",
                     # mosaic outputs
                     "wcs", "fcr",
                     # processCcd QA
                     "ossThumb", "flattenedThumb", "calexpThumb", "plotMagHist", "plotSeeingRough",
                     "plotSeeingRobust", "plotSeeingMap", "plotEllipseMap", "plotEllipticityMap",
                     "plotFwhmGrid", "plotEllipseGrid", "plotEllipticityGrid", "plotPsfSrcGrid",
                     "plotPsfModelGrid", "fitsFwhmGrid", "fitsEllipticityGrid", "fitsEllPaGrid",
                     "fitsPsfSrcGrid", "fitsPsfModelGrid", "tableSeeingMap", "tableSeeingGrid",
                     # forcedPhot outputs
                     "forced_src",
                     ):
            self.mappings[name].keyDict.update(keys)

        self.addFilters()

        self.filters = {}
        for filt in HSC_FILTER_DEFINITIONS:
            self.filters[filt.physical_filter] = afwImage.Filter(filt.physical_filter).getCanonicalName()
        self.defaultFilterName = "UNRECOGNISED"

        #
        # The number of bits allocated for fields in object IDs, appropriate for
        # the default-configured Rings skymap.
        #
        # This shouldn't be the mapper's job at all; see #2797.

        HscMapper._nbit_tract = 16
        HscMapper._nbit_patch = 5
        HscMapper._nbit_filter = 6

        HscMapper._nbit_id = 64 - (HscMapper._nbit_tract + 2*HscMapper._nbit_patch + HscMapper._nbit_filter)

        if len(afwImage.Filter.getNames()) >= 2**HscMapper._nbit_filter:
            raise RuntimeError("You have more filters defined than fit into the %d bits allocated" %
                               HscMapper._nbit_filter)

    def _makeCamera(self, *args, **kwargs):
        """Make the camera object

        This implementation layers a cache over the parent class'
        implementation. Caching the camera improves the instantiation
        time for the HscMapper because parsing the camera's Config
        involves a lot of 'stat' calls (through the tracebacks).
        """
        if not self._cameraCache:
            self._cameraCache = CameraMapper._makeCamera(self, *args, **kwargs)
        return self._cameraCache

    @classmethod
    def clearCache(cls):
        """Clear the camera cache

        This is principally intended to help memory leak tests pass.
        """
        cls._cameraCache = None

    def map(self, datasetType, dataId, write=False):
        """Need to strip 'flags' argument from map

        We want the 'flags' argument passed to the butler to work (it's
        used to change how the reading/writing is done), but want it
        removed from the mapper (because it doesn't correspond to a
        registry column).
        """
        copyId = dataId.copy()
        copyId.pop("flags", None)
        location = super(HscMapper, self).map(datasetType, copyId, write=write)

        if 'flags' in dataId:
            location.getAdditionalData().set('flags', dataId['flags'])

        return location

    @staticmethod
    def _flipChipsLR(exp, wcs, detectorId, dims=None):
        """Flip the chip left/right or top/bottom. Process either/and the pixels and wcs

        Most chips are flipped L/R, but the rotated ones (100..103) are flipped T/B
        """
        flipLR, flipTB = (False, True) if detectorId in (100, 101, 102, 103) else (True, False)
        if exp:
            exp.setMaskedImage(afwMath.flipImage(exp.getMaskedImage(), flipLR, flipTB))
        if wcs:
            ampDimensions = exp.getDimensions() if dims is None else dims
            ampCenter = geom.Point2D(ampDimensions/2.0)
            wcs = afwGeom.makeFlippedWcs(wcs, flipLR, flipTB, ampCenter)

        return exp, wcs

    def std_raw_md(self, md, dataId):
        """We need to flip the WCS defined by the metadata in case anyone ever
        constructs a Wcs from it.
        """
        wcs = afwGeom.makeSkyWcs(md)
        wcs = self._flipChipsLR(None, wcs, dataId['ccd'],
                                dims=afwImage.bboxFromMetadata(md).getDimensions())[1]
        # NOTE: we don't know where the 0.992 magic constant came from. It was copied over from hscSimMapper.
        wcsR = afwGeom.makeSkyWcs(crpix=wcs.getPixelOrigin(),
                                  crval=wcs.getSkyOrigin(),
                                  cdMatrix=wcs.getCdMatrix()*0.992)
        wcsMd = wcsR.getFitsMetadata()

        for k in wcsMd.names():
            md.set(k, wcsMd.getScalar(k))

        return md

    def _createSkyWcsFromMetadata(self, exposure):
        # Overridden to flip chips as necessary to get sensible SkyWcs.
        metadata = exposure.getMetadata()
        try:
            wcs = afwGeom.makeSkyWcs(metadata, strip=True)
            exposure, wcs = self._flipChipsLR(exposure, wcs, exposure.getDetector().getId())
            exposure.setWcs(wcs)
        except lsst.pex.exceptions.TypeError as e:
            # See DM-14372 for why this is debug and not warn (e.g. calib files without wcs metadata).
            self.log.debug("wcs set to None; missing information found in metadata to create a valid wcs:"
                           " %s", e.args[0])

        # ensure any WCS values stripped from the metadata are removed in the exposure
        exposure.setMetadata(metadata)

    def std_dark(self, item, dataId):
        exposure = self._standardizeExposure(self.calibrations['dark'], item, dataId, trimmed=False)
        visitInfo = afwImage.VisitInfo(exposureTime=1.0, darkTime=1.0)
        exposure.getInfo().setVisitInfo(visitInfo)
        return exposure

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
        return 32  # just a guess, but this leaves plenty of space for sources

    def map_linearizer(self, dataId, write=False):
        """Map a linearizer."""
        if self._linearize is None:
            raise RuntimeError("No linearizer available.")
        actualId = self._transformId(dataId)
        return ButlerLocation(
            pythonType="lsst.ip.isr.LinearizeSquared",
            cppType="Config",
            storageName="PickleStorage",
            locationList="ignored",
            dataId=actualId,
            mapper=self,
            storage=self.rootStorage)

    def bypass_linearizer(self, datasetType, pythonType, butlerLocation, dataId):
        """Return the linearizer.
        """
        if self._linearize is None:
            raise RuntimeError("No linearizer available.")
        return self._linearize

    def _computeCoaddExposureId(self, dataId, singleFilter):
        """Compute the 64-bit (long) identifier for a coadd.

        @param dataId (dict)       Data identifier with tract and patch.
        @param singleFilter (bool) True means the desired ID is for a single-
                                   filter coadd, in which case dataId
                                   must contain filter.
        """

        tract = int(dataId['tract'])
        if tract < 0 or tract >= 2**HscMapper._nbit_tract:
            raise RuntimeError('tract not in range [0,%d)' % (2**HscMapper._nbit_tract))
        patchX, patchY = [int(patch) for patch in dataId['patch'].split(',')]
        for p in (patchX, patchY):
            if p < 0 or p >= 2**HscMapper._nbit_patch:
                raise RuntimeError('patch component not in range [0, %d)' % 2**HscMapper._nbit_patch)
        oid = (((tract << HscMapper._nbit_patch) + patchX) << HscMapper._nbit_patch) + patchY
        if singleFilter:
            return (oid << HscMapper._nbit_filter) + afwImage.Filter(dataId['filter']).getId()
        return oid

    def bypass_deepCoaddId_bits(self, *args, **kwargs):
        """The number of bits used up for patch ID bits"""
        return 64 - HscMapper._nbit_id

    def bypass_deepCoaddId(self, datasetType, pythonType, location, dataId):
        return self._computeCoaddExposureId(dataId, True)

    def bypass_deepMergedCoaddId_bits(self, *args, **kwargs):
        """The number of bits used up for patch ID bits"""
        return 64 - HscMapper._nbit_id

    def bypass_deepMergedCoaddId(self, datasetType, pythonType, location, dataId):
        return self._computeCoaddExposureId(dataId, False)

    # The following allow grabbing a 'psf' from the butler directly, without having to get it from a calexp
    def map_psf(self, dataId, write=False):
        if write:
            raise RuntimeError("Writing a psf directly is no longer permitted: write as part of a calexp")
        copyId = dataId.copy()
        copyId['bbox'] = geom.Box2I(geom.Point2I(0, 0), geom.Extent2I(1, 1))
        return self.map_calexp_sub(copyId)

    def std_psf(self, calexp, dataId):
        return calexp.getPsf()

    @classmethod
    def getCameraName(cls):
        return "hsc"
