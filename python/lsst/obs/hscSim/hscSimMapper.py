#!/usr/bin/env python

import os

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class HscSimMapper(CameraMapper):
    """Provides abstract-physical mapping for HSC Simulation data"""
    
    def __init__(self, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "HscSimMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)

        # Default to using $SUPRIME_DATA_DIR. Otherwise require root= arg
        if not 'root' in kwargs:
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'),
                                              'HSC')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")

        super(HscSimMapper, self).__init__(policy, policyFile.getRepositoryPath(), **kwargs)

        self.filters = {
            "W-J-B"   : "B",
            "W-S-G+"  : "g",
            "W-S-R+"  : "r",
            "W-S-I+"  : "i",
            "W-S-Z+"  : "z",
            "W-S-ZR"  : "y",
            }

    def _extractAmpId(self, dataId):
        return (self._extractDetectorName(dataId), 0, 0)

    def _extractDetectorName(self, dataId):
        return int("%(ccd)d" % dataId)

