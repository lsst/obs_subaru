#!/usr/bin/env python

import os
import pwd

from lsst.daf.butlerUtils import CameraMapper
import lsst.afw.image.utils as afwImageUtils
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.geom as afwGeom
import lsst.pex.policy as pexPolicy

class FileMapper(CameraMapper):
    """Provides abstract-physical mapping for data found in the filesystem"""
    
    def __init__(self, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "FileMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)
        if False:
            if not kwargs.get('root', None):
                raise RuntimeError("Please specify a root")
            if not kwargs.get('calibRoot', None):
                kwargs['calibRoot'] = os.path.join(kwargs['root'], 'CALIB')

        super(FileMapper, self).__init__(policy, policyFile.getRepositoryPath(), **kwargs)
        
        # Ensure each dataset type of interest knows about the full range of keys available from the registry
        keys = dict(calexp=str,
                )
        for name in ("calexp", "src"):
            self.mappings[name].keyDict = keys

        # SDSS g': http://www.naoj.org/Observing/Instruments/SCam/txt/g.txt
        # SDSS r': http://www.naoj.org/Observing/Instruments/SCam/txt/r.txt
        # SDSS i': http://www.naoj.org/Observing/Instruments/SCam/txt/i.txt
        # SDSS z': http://www.naoj.org/Observing/Instruments/SCam/txt/z.txt
        # y-band: Shimasaku et al., 2005, PASJ, 57, 447

        afwImageUtils.resetFilters()

        afwImageUtils.defineFilter(name='g', lambdaEff=477, alias=['W-S-G+'])
        afwImageUtils.defineFilter(name='r', lambdaEff=623, alias=['W-S-R+'])
        afwImageUtils.defineFilter(name='i', lambdaEff=775, alias=['W-S-I+'])
        afwImageUtils.defineFilter(name='z', lambdaEff=925, alias=['W-S-Z+'])
        afwImageUtils.defineFilter(name='y', lambdaEff=990, alias=['W-S-ZR'])
        
        self.filters = {
            "W-S-G+"  : "g",
            "W-S-R+"  : "r",
            "W-S-I+"  : "i",
            "W-S-Z+"  : "z",
            "W-S-ZR"  : "y",
            }

        # next line makes a dict that maps filter names to sequential integers (arbitrarily sorted),
        # for use in generating unique IDs for sources.
        self.filterIdMap = dict(zip(self.filters, range(len(self.filters))))
    
    def _setCcdDetector(self, *args):
        pass

    def _setFilter(self, *args):
        pass

    def _computeCcdExposureId(self, dataId):
        """Compute the 64-bit (long) identifier for a CCD exposure.

        @param dataId (dict) Data identifier with visit, ccd
        """
        return 0

        pathId = self._transformId(dataId)
        visit = pathId['visit']
        ccd = pathId['ccd']
        return visit*200 + ccd

    def bypass_ccdExposureId(self, datasetType, pythonType, location, dataId):
        return self._computeCcdExposureId(dataId)

    def bypass_ccdExposureId_bits(self, datasetType, pythonType, location, dataId):
        """How many bits are required for the maximum exposure ID"""
        return 32 # just a guess, but this leaves plenty of space for sources

    @classmethod
    def getEupsProductName(cls):
        return "obs_subaru"

    @classmethod
    def getCameraName(cls):
        return "file"

    def queryMetadata(self, datasetType, key, format, dataId):
        return tuple()
    
