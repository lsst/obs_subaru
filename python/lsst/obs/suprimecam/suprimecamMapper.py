#!/usr/bin/env python

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class SuprimecamMapper(CameraMapper):
    def __init__(self, rerun=None, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "SuprimecamMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)

        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA')
                kwargs['calibRoot'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'CALIB')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")

        self.rerun = rerun if rerun is not None else pwd.getpwuid(os.geteuid())[0]

        super(SuprimecamMapper, self).__init__(policy, policyFile.getRepositoryPath(),
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
        miyazakiNames = ["Nausicaa", "Kiki", "Fio", "Sophie", "Sheeta",
                         "Satsuki", "Chihiro", "Clarisse", "Ponyo", "San"]
        #return "Suprime %(ccd)d" % dataId
        ccdTmp = int("%(ccd)d" % dataId)
        return miyazakiNames[ccdTmp]

    def _transformId(self, dataId):
        actualId = dataId.copy()
        if actualId.has_key("rerun"):
            del actualId["rerun"]
        return actualId

    def _mapActualToPath(self, template, dataId):
        actualId = self._transformId(dataId)
        actualId['rerun'] = self.rerun
        return template % actualId

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
