#!/usr/bin/env python

import os
import pwd

from lsst.daf.butlerUtils import CameraMapper
import lsst.afw.image.utils as afwImageUtils
import lsst.afw.geom as afwGeom
import lsst.pex.policy as pexPolicy
from .hscSimLib import HscDistortion

class HscSimMapper(CameraMapper):
    """Provides abstract-physical mapping for HSC Simulation data"""
    
    def __init__(self, rerun=None, outRoot=None, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "HscSimMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)

        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'HSC')
                kwargs['calibRoot'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'CALIB')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        if outRoot is None:
            if policy.exists('outRoot'):
                outRoot = policy.get('outRoot')
            else:
                outRoot = kwargs['root']

        self.rerun = rerun if rerun is not None else pwd.getpwuid(os.geteuid())[0]
        self.outRoot = outRoot

        super(HscSimMapper, self).__init__(policy, policyFile.getRepositoryPath(),
                                           provided=['rerun', 'outRoot'], **kwargs)
        
        # Distortion isn't pluggable, so we'll put in our own
        elevation = 45 * afwGeom.degrees
        distortion = HscDistortion(elevation)
        self.camera.setDistortion(distortion)
        
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
        actualId['outRoot'] = self.outRoot
        return template % actualId
