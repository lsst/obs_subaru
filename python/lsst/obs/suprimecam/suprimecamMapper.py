#!/usr/bin/env python

import os
import pwd

import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.image.utils as afwImageUtils

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class SuprimecamMapper(CameraMapper):
    def __init__(self, mit=False, outputRoot=None, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "SuprimecamMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)

        self.mit = mit
        if self.mit:
            policy.set("camera", "../suprimecam/Full_Suprimecam_MIT_geom.paf")
            policy.set("defects", "../suprimecam/mit_defects")

        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")

        if not kwargs.get('calibRoot', None):
            try:
                kwargs['calibRoot'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA', 'CALIB')
                if self.mit:
                    kwargs['calibRoot'] += "_MIT"                
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
            
        if outputRoot is None:
            if policy.exists('outRoot'):
                outputRoot = policy.get('outRoot')
            else:
                outputRoot = kwargs['root']

        self.outRoot = outputRoot

        super(SuprimecamMapper, self).__init__(policy, policyFile.getRepositoryPath(),
                                               provided=['outRoot'],
                                               outputRoot=outputRoot, **kwargs)

        afwImageUtils.defineFilter('B',  lambdaEff=400,  alias=['W-J-B'])
        afwImageUtils.defineFilter('V',  lambdaEff=550,  alias=['W-J-V'])
        afwImageUtils.defineFilter('R',  lambdaEff=650,  alias=['W-C-RC'])
        afwImageUtils.defineFilter('VR', lambdaEff=600,  alias=['W-J-VR'])
        afwImageUtils.defineFilter('I',  lambdaEff=800,  alias=['W-C-IC'])
        afwImageUtils.defineFilter('g',  lambdaEff=450,  alias=['W-S-G+'])
        afwImageUtils.defineFilter('r',  lambdaEff=600,  alias=['W-S-R+'])
        afwImageUtils.defineFilter('i',  lambdaEff=770,  alias=['W-S-I+'])
        afwImageUtils.defineFilter('z',  lambdaEff=900,  alias=['W-S-Z+'])
        afwImageUtils.defineFilter('y',  lambdaEff=1000, alias=['W-S-ZR'])

        self.filters = {
            "W-J-B"   : "B",
            "W-J-V"   : "V",
            "W-J-VR"  : "VR",
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
        return actualId

    def _mapActualToPath(self, template, dataId):
        actualId = self._transformId(dataId)
        actualId['outRoot'] = self.outRoot
        return template % actualId
