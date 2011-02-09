#!/usr/bin/env python

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class SuprimecamMapper(CameraMapper):
    def __init__(self, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "SuprimecamMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)
        super(SuprimecamMapper, self).__init__(policy, policyFile.getRepositoryPath(), **kwargs)

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
        miyazakiNames = ["Nausicaa", "Kiki", "Fio", "Sophie", "Sheeta",
                         "Satsuki", "Chihiro", "Clarisse", "Ponyo", "San"]
        #return "Suprime %(ccd)d" % dataId
        ccdTmp = int("%(ccd)d" % dataId)
        return miyazakiNames[ccdTmp]

### XXX Not necessary now that gains are in the camera configuration
#    def std_raw(self, item, dataId):
#        exposure = super(SuprimecamMapper, self).std_raw(item, dataId)
#
#        # Set gains from the header
#        md = exposure.getMetadata()
#        gain = md.get('GAIN')
#        for amp in cameraGeom.cast_Ccd(exposure.getDetector()):
#            amp.getElectronicParams().setGain(gain)
#            
#        return exposure
