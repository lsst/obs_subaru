#!/usr/bin/env python

import os, os.path
import pwd

import lsst.daf.base as dafBase
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.image.utils as afwImageUtils

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

class SuprimecamMapperBase(CameraMapper):

    def defineFilters(self):
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
        afwImageUtils.defineFilter("IA427", lambdaEff=427, alias=['I-A-L427'])
        afwImageUtils.defineFilter("IA445", lambdaEff=445, alias=['I-A-L445'])
        afwImageUtils.defineFilter("IA464", lambdaEff=464, alias=['I-A-L464'])
        afwImageUtils.defineFilter("IA484", lambdaEff=484, alias=['I-A-L484'])
        afwImageUtils.defineFilter("IA505", lambdaEff=505, alias=['I-A-L505'])
        afwImageUtils.defineFilter("IA527", lambdaEff=527, alias=['I-A-L527'])
        afwImageUtils.defineFilter("IA550", lambdaEff=550, alias=['I-A-L550'])
        afwImageUtils.defineFilter("IA574", lambdaEff=574, alias=['I-A-L574'])
        afwImageUtils.defineFilter("IA598", lambdaEff=598, alias=['I-A-L598'])
        afwImageUtils.defineFilter("IA624", lambdaEff=624, alias=['I-A-L624'])
        afwImageUtils.defineFilter("IA651", lambdaEff=651, alias=['I-A-L651'])
        afwImageUtils.defineFilter("IA679", lambdaEff=679, alias=['I-A-L679'])
        afwImageUtils.defineFilter("IA709", lambdaEff=709, alias=['I-A-L709'])
        afwImageUtils.defineFilter("IA738", lambdaEff=738, alias=['I-A-L738'])
        afwImageUtils.defineFilter("IA767", lambdaEff=767, alias=['I-A-L767'])
        afwImageUtils.defineFilter("IA797", lambdaEff=797, alias=['I-A-L797'])
        afwImageUtils.defineFilter("IA827", lambdaEff=827, alias=['I-A-L827'])
        afwImageUtils.defineFilter("IA856", lambdaEff=856, alias=['I-A-L856'])

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

    def _computeCcdExposureId(self, dataId):
        """Compute the 64-bit (long) identifier for a CCD exposure.

        @param dataId (dict) Data identifier with visit, ccd
        """
        pathId = self._transformId(dataId)
        visit = pathId['visit']
        ccd = pathId['ccd']
        return visit*10 + ccd

    def bypass_ccdExposureId(self, datasetType, pythonType, location, dataId):
        return self._computeCcdExposureId(dataId)

    def bypass_ccdExposureId_bits(self, datasetType, pythonType, location, dataId):
        return 32 # not really, but this leaves plenty of space for sources

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

class SuprimecamMapper(SuprimecamMapperBase):
    """
    Mapper for SuprimeCam with the newer Hamamatsu chips.
    """

    def __init__(self, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "SuprimecamMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)
        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        if not kwargs.get('calibRoot', None):
            try:
                kwargs['calibRoot'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA', 'CALIB')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        super(SuprimecamMapper, self).__init__(policy, policyFile.getRepositoryPath(), **kwargs)
        self.defineFilters()

    def _extractDetectorName(self, dataId):
        miyazakiNames = ["Nausicaa", "Kiki", "Fio", "Sophie", "Sheeta",
                         "Satsuki", "Chihiro", "Clarisse", "Ponyo", "San"]
        ccdTmp = int("%(ccd)d" % dataId)
        return miyazakiNames[ccdTmp]

class SuprimecamMapperMit(SuprimecamMapperBase):
    """
    Mapper for SuprimeCam with the older, MIT chips.
    """

    def __init__(self, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "SuprimecamMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)
        if not kwargs.get('root', None):
            try:
                kwargs['root'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        if not kwargs.get('calibRoot', None):
            try:
                kwargs['calibRoot'] = os.path.join(os.environ.get('SUPRIME_DATA_DIR'), 'SUPA', 'CALIB_MIT')
            except:
                raise RuntimeError("Either $SUPRIME_DATA_DIR or root= must be specified")
        policy.set("camera", "../suprimecam/Full_Suprimecam_MIT_geom.paf")
        policy.set("defects", "../suprimecam/mit_defects")
        super(SuprimecamMapperMit, self).__init__(policy, policyFile.getRepositoryPath(), **kwargs)
        self.defineFilters()

    def _extractDetectorName(self, dataId):
        mitNames = ["w67c1", "w6c1", "si005s", "si001s",  "si002s", "si006s", "w93c2", "w9c2", "w4c5", "w7c3"]
        ccdTmp = int("%(ccd)d" % dataId)
        return mitNames[ccdTmp]
