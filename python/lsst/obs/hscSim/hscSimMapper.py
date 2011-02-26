#!/usr/bin/env python

import os

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class HscSimMapper(CameraMapper):
    """Provides abstract-physical mapping for HSC Simulation data"""
    
    def __init__(self, rerun=None, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "HscSimMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)

        # Resolve root directory in order:
        #   if rerun is specified:
        #     if rerun is a path (starts with / or .) use it as root
        #     else use $SUPRIME_DATA_DIR/HSC/rerun/$rerun as root
        #
        #   else 
        #     try using $SUPRIME_DATA_DIR/HSC. Otherwise require root= arg
        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'HSC')
                kwargs['calibRoot'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'CALIB')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")

#        if rerun:
#            if rerun.startswith('.') or rerun.startswith('/'):
#                self.rerun = rerun
#            else:
#                try:
#                    self.rerun = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'HSC', 'rerun', rerun)
#                except:
#                    raise RuntimeError("Rerun must include a path, or $SUPRIME_DATA_DIR must be defined")
        self.rerun = rerun

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
