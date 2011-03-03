#!/usr/bin/env python

import os
import pwd

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class HscSimMapper(CameraMapper):
    """Provides abstract-physical mapping for HSC Simulation data"""
    
    def __init__(self, rerun=None, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "HscSimMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)

        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'HSC')
                kwargs['calibRoot'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'CALIB')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")

        self.rerun = rerun if rerun is not None else pwd.getpwuid(os.geteuid())[0]

        super(HscSimMapper, self).__init__(policy, policyFile.getRepositoryPath(),
                                           provided=['rerun'], **kwargs)

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

    def _transformId(self, dataId):
        actualId = dataId.copy()
        if actualId.has_key("rerun"):
            del actualId["rerun"]
        return actualId

    def _mapActualToPath(self, template, dataId):
        actualId = self._transformId(dataId)
        actualId['rerun'] = self.rerun
        return template % actualId
