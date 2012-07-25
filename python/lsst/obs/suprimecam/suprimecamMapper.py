#!/usr/bin/env python

import os, os.path
import pwd

import lsst.daf.base as dafBase
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.image.utils as afwImageUtils

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class SuprimecamMapper(CameraMapper):
    def __init__(self, mit=False, **kwargs):
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
            kwargs['calibRoot'] = os.path.join(kwargs['root'], 'CALIB')
            if self.mit:
                kwargs['calibRoot'] += "_MIT"

        if not kwargs.get('outputRoot', None):
            kwargs['outputRoot'] = os.path.join(kwargs['root'], 'rerun', os.getlogin())
            if not os.path.isdir(kwargs['outputRoot']):
                try:
                    os.makedirs(kwargs['outputRoot'])
                except:
                    # Possibly caused by a race condition
                    pass
        
        super(SuprimecamMapper, self).__init__(policy, policyFile.getRepositoryPath(), **kwargs)

        # Johnson filters
        afwImageUtils.defineFilter('B',  lambdaEff=400,  alias=['W-J-B'])
        afwImageUtils.defineFilter('V',  lambdaEff=550,  alias=['W-J-V'])
        afwImageUtils.defineFilter('VR', lambdaEff=600,  alias=['W-J-VR'])

        # Cousins filters
        afwImageUtils.defineFilter('R',  lambdaEff=650,  alias=['W-C-RC'])
        afwImageUtils.defineFilter('I',  lambdaEff=800,  alias=['W-C-IC'])

        # Sloan filters
        afwImageUtils.defineFilter('g',  lambdaEff=450,  alias=['W-S-G+'])
        afwImageUtils.defineFilter('r',  lambdaEff=600,  alias=['W-S-R+'])
        afwImageUtils.defineFilter('i',  lambdaEff=770,  alias=['W-S-I+'])
        afwImageUtils.defineFilter('z',  lambdaEff=900,  alias=['W-S-Z+'])
        afwImageUtils.defineFilter('y',  lambdaEff=1000, alias=['W-S-ZR'])

        # Narrow-band filters
        afwImageUtils.defineFilter('NB704',  lambdaEff=704,  alias=['N-B-L704'])
        afwImageUtils.defineFilter('NB711',  lambdaEff=711,  alias=['N-B-L711'])
       
        # Intermediate-band filters
        afwImageUtils.defineFilter("L427", lambdaEff=427, alias=['I-A-L427'])
        afwImageUtils.defineFilter("L445", lambdaEff=445, alias=['I-A-L445'])
        afwImageUtils.defineFilter("L464", lambdaEff=464, alias=['I-A-L464'])
        afwImageUtils.defineFilter("L484", lambdaEff=484, alias=['I-A-L484'])
        afwImageUtils.defineFilter("L505", lambdaEff=505, alias=['I-A-L505'])
        afwImageUtils.defineFilter("L527", lambdaEff=527, alias=['I-A-L527'])
        afwImageUtils.defineFilter("L550", lambdaEff=550, alias=['I-A-L550'])
        afwImageUtils.defineFilter("L574", lambdaEff=574, alias=['I-A-L574'])
        afwImageUtils.defineFilter("L598", lambdaEff=598, alias=['I-A-L598'])
        afwImageUtils.defineFilter("L624", lambdaEff=624, alias=['I-A-L624'])
        afwImageUtils.defineFilter("L651", lambdaEff=651, alias=['I-A-L651'])
        afwImageUtils.defineFilter("L679", lambdaEff=679, alias=['I-A-L679'])
        afwImageUtils.defineFilter("L709", lambdaEff=709, alias=['I-A-L709'])
        afwImageUtils.defineFilter("L738", lambdaEff=738, alias=['I-A-L738'])
        afwImageUtils.defineFilter("L767", lambdaEff=767, alias=['I-A-L767'])
        afwImageUtils.defineFilter("L797", lambdaEff=797, alias=['I-A-L797'])
        afwImageUtils.defineFilter("L827", lambdaEff=827, alias=['I-A-L827'])
        afwImageUtils.defineFilter("L856", lambdaEff=856, alias=['I-A-L856'])

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
            "N-B-L704": "NB704",
            "N-B-L711": "NB711",
            "I-A-L427": "L427",
            "I-A-L445": "L445",
            "I-A-L464": "L464",
            "I-A-L484": "L484",
            "I-A-L505": "L505",
            "I-A-L527": "L527",
            "I-A-L550": "L550",
            "I-A-L574": "L574",
            "I-A-L598": "L598",
            "I-A-L624": "L624",
            "I-A-L651": "L651",
            "I-A-L679": "L679",
            "I-A-L709": "L709",
            "I-A-L738": "L738",
            "I-A-L767": "L767",
            "I-A-L797": "L797",
            "I-A-L827": "L827",
            "I-A-L856": "L856",
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

    def _setTimes(self, mapping, item, dataId):
        """Set the exposure time and exposure midpoint in the calib object in
        an Exposure.  Use the EXPTIME and MJD keywords (and strip out
        EXPTIME).
        @param mapping (lsst.daf.butlerUtils.Mapping)
        @param[in,out] item (lsst.afw.image.Exposure)
        @param dataId (dict) Dataset identifier"""

        md = item.getMetadata()
        calib = item.getCalib()
        if md.exists("EXPTIME"):
            expTime = md.get("EXPTIME")
            calib.setExptime(expTime)
            md.remove("EXPTIME")
        else:
            expTime = calib.getExptime()
        if md.exists("MJD"):
            obsStart = dafBase.DateTime(md.get("MJD"),
                    dafBase.DateTime.MJD, dafBase.DateTime.UTC)
            obsMidpoint = obsStart.nsecs() + long(expTime * 1000000000L / 2)
            calib.setMidTime(dafBase.DateTime(obsMidpoint))

