#!/usr/bin/env python

import os
import pwd

import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class SuprimecamMapper(CameraMapper):
    def __init__(self, mit=False, rerun=None, outRoot=None, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "SuprimecamMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)

        self.mit = mit
        if self.mit:
            policy.set("camera", "../suprimecam/Full_Suprimecam_MIT_geom.paf")
            policy.set("defects", "../suprimecam/mit_defects")

        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA')
                kwargs['calibRoot'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA', 'CALIB')
                if self.mit:
                    kwargs['calibRoot'] += "_MIT"                
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        if outRoot is None:
            if policy.exists('outRoot'):
                outRoot = policy.get('outRoot')
            else:
                outRoot = kwargs['root']

        self.rerun = rerun if rerun is not None else pwd.getpwuid(os.geteuid())[0]
        self.outRoot = outRoot

        super(SuprimecamMapper, self).__init__(policy, policyFile.getRepositoryPath(),
                                               provided=['rerun', 'outRoot'], **kwargs)

        self.filters = {
            "W-J-B"   : "B",
            "W-J-V"   : "V",
            "W-C-RC"  : "R",
            "W-C-IC"  : "I",
            "W-S-G+"  : "g",
            "W-S-R+"  : "r",
            "W-S-I+"  : "i",
            "W-S-Z+"  : "z",
            "W-S-ZR"  : "y",
            }

    def _extractAmpId(self, dataId):
        return (self._extractDetectorName(dataId), 0, 0)

    def _extractDetectorName(self, dataId):
        #return "Suprime %(ccd)d" % dataId
        miyazakiNames = ["Nausicaa", "Kiki", "Fio", "Sophie", "Sheeta",
                         "Satsuki", "Chihiro", "Clarisse", "Ponyo", "San"]
        mitNames = ["w67c1", "w6c1", "si005s", "si001s",  "si002s", "si006s", "w93c2", "w9c2", "w4c5", "w7c3"]
        ccdTmp = int("%(ccd)d" % dataId)
        return mitNames[ccdTmp] if self.mit else miyazakiNames[ccdTmp]

    def _transformId(self, dataId):
        actualId = dataId.copy()
        if actualId.has_key("rerun"):
            del actualId["rerun"]
        return actualId

    def _mapActualToPath(self, template, dataId):
        actualId = self._transformId(dataId)
        actualId['rerun'] = self.rerun
        actualId['outRoot'] = self.outRoot
        return template % actualId
