import os
import re

from lsst.daf.persistence         import Mapper, ButlerLocation, LogicalLocation
import lsst.daf.butlerUtils       as butlerUtils
import lsst.afw.image             as afwImage
import lsst.afw.cameraGeom        as afwCameraGeom
import lsst.afw.cameraGeom.utils  as cameraGeomUtils
import lsst.afw.image.utils       as imageUtils
import lsst.pex.logging           as pexLog
import lsst.pex.policy            as pexPolicy
from lsst.obs.camera              import CameraMapper


class SuprimeMapper(CameraMapper):
    
    def __init__(self, policy=None, root=".", registry=None, calibRoot=None):
        CameraMapper.__init__(self, policy=policy, root=root, registry=registry, calibRoot=calibRoot)

        self.keys = ["field", "visit", "ccd", "amp", "filter", "mystery", "taiObs" ]
        self.keys += ["filter", "expTime"]

        self.filterMap = {
            "W-S-I+"  : "i",
            "W-S-ZR"  : "z2",
            "W-S-Z+"  : "z"
         }

        self.filterIdMap = {
	    'u': 0, 'g': 1, 'r': 2, 'i': 3, 'z': 4, 'y': 5, 'z2': 5}

    def _extractDetectorName(self, dataId):
	miyazakiNames = ["Nausicaa", "Kiki", "Fio", "Sophie", "Sheeta",
			 "Satsuki", "Chihiro", "Clarisse", "Ponyo", "San"]
        #return "Suprime %(ccd)d" % dataId
	ccdTmp = int("%(ccd)d" % dataId)
	return miyazakiNames[ccdTmp]
