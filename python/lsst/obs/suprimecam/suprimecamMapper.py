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
        afwImageUtils.defineFilter('NONE', lambdaEff=0)

        # Johnson filters
        afwImageUtils.defineFilter('U',  lambdaEff=300,  alias=['W-J-U'])
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
        afwImageUtils.defineFilter("NA503", lambdaEff=503, alias=['N-A-L503'])
        afwImageUtils.defineFilter("NA651", lambdaEff=651, alias=['N-A-L651'])
        afwImageUtils.defineFilter("NA656", lambdaEff=656, alias=['N-A-L656'])
        afwImageUtils.defineFilter("NA659", lambdaEff=659, alias=['N-A-L659'])
        afwImageUtils.defineFilter("NA671", lambdaEff=671, alias=['N-A-L671'])

        afwImageUtils.defineFilter('NB1006', lambdaEff=1006, alias=['N-B-1006'])
        afwImageUtils.defineFilter('NB1010', lambdaEff=1010, alias=['N-B-1010'])
        afwImageUtils.defineFilter('NB100',  lambdaEff=100,  alias=['N-B-L100'])
        afwImageUtils.defineFilter('NB359',  lambdaEff=359,  alias=['N-B-L359'])
        afwImageUtils.defineFilter('NB387',  lambdaEff=387,  alias=['N-B-L387'])
        afwImageUtils.defineFilter('NB413',  lambdaEff=413,  alias=['N-B-L413'])
        afwImageUtils.defineFilter('NB497',  lambdaEff=497,  alias=['N-B-L497'])
        afwImageUtils.defineFilter('NB515',  lambdaEff=515,  alias=['N-B-L515'])
        afwImageUtils.defineFilter('NB570',  lambdaEff=570,  alias=['N-B-L570'])
        afwImageUtils.defineFilter('NB704',  lambdaEff=704,  alias=['N-B-L704'])
        afwImageUtils.defineFilter('NB711',  lambdaEff=711,  alias=['N-B-L711'])
        afwImageUtils.defineFilter('NB816',  lambdaEff=816,  alias=['N-B-L816'])
        afwImageUtils.defineFilter('NB818',  lambdaEff=818,  alias=['N-B-L818'])
        afwImageUtils.defineFilter('NB912',  lambdaEff=912,  alias=['N-B-L912'])
        afwImageUtils.defineFilter('NB921',  lambdaEff=921,  alias=['N-B-L921'])
        afwImageUtils.defineFilter('NB973',  lambdaEff=973,  alias=['N-B-L973'])

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
        afwImageUtils.defineFilter("L856", lambdaEff=856, alias=['I-A-L856'])

        # Unknown/custom filters
        afwImageUtils.defineFilter("B030", lambdaEff=0, alias=['G-A-B030'])
        afwImageUtils.defineFilter("R030", lambdaEff=0, alias=['G-A-R030'])
        afwImageUtils.defineFilter("P550", lambdaEff=0, alias=['P-A-L550'])
        afwImageUtils.defineFilter("SN01", lambdaEff=0, alias=['S-A-SN01'])
        afwImageUtils.defineFilter("SN02", lambdaEff=0, alias=['S-A-SN02'])
        afwImageUtils.defineFilter("Y",    lambdaEff=0, alias=['W-A-Y'])
        afwImageUtils.defineFilter("ZB",   lambdaEff=0, alias=['W-S-ZB'])


        self.filters = {
            "W-J-U"   : "U",
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
            'N-A-L503': "NA503",
            'N-A-L651': "NA651",
            'N-A-L656': "NA656",
            'N-A-L659': "NA659",
            'N-A-L671': "NA671",
            'N-B-1006': 'NB1006',
            'N-B-1010': 'NB1010',
            'N-B-L100': 'NB100',
            'N-B-L359': 'NB359',
            'N-B-L387': 'NB387',
            'N-B-L413': 'NB413',
            'N-B-L497': 'NB497',
            'N-B-L515': 'NB515',
            'N-B-L570': 'NB570',
            'N-B-L704': 'NB704',
            'N-B-L711': 'NB711',
            'N-B-L816': 'NB816',
            'N-B-L818': 'NB818',
            'N-B-L912': 'NB912',
            'N-B-L921': 'NB921',
            'N-B-L973': 'NB973',
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
            'G-A-B030': "B030",
            'G-A-R030': "R030",
            'P-A-L550': "P550",
            'S-A-SN01': "SN01",
            'S-A-SN02': "SN02",
            'W-A-Y'   : "Y",
            'W-S-ZB'  : "ZB",
            }


        # next line makes a dict that maps filter names to sequential integers (arbitrarily sorted),
        # for use in generating unique IDs for sources.
        self.filterIdMap = dict(zip(self.filters, range(len(self.filters))))

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

    def _computeStackExposureId(self, dataId):
        """Compute the 64-bit (long) identifier for a Stack exposure.

        @param dataId (dict) Data identifier with stack, patch, filter
        """
        nFilters = len(self.filters)
        nPatches = 1000000
        return (long(dataId["stack"]) * nFilters * nPatches +
                long(dataId["patch"]) * nFilters +
                self.filterIdMap[dataId["filter"]])

    def bypass_stackExposureId(self, datasetType, pythonType, location, dataId):
        return self._computeStackExposureId(dataId)

    def bypass_stackExposureId_bits(self, datasetType, pythonType, location, dataId):
        # log_2 [nFilters(=30) * nPatches(=10^6) * nStack(=10^5)]
        return 42

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
            kwargs['calibRoot'] = os.path.join(kwargs['root'], 'CALIB')
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
