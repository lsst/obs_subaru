#!/usr/bin/env python

import os
import pwd

import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage

from lsst.daf.butlerUtils import CameraMapper
import lsst.pex.policy as pexPolicy

# Fix an image, putting the bias on the right hand side of the science data
def fixImage(template, numCols, numRows, dataBox, biasBox):
    if template is None:
        return None

    Image = type(template)
    new = Image(numCols, numRows)

    data = Image(template, dataBox, afwImage.LOCAL)
    newDataBox = afwGeom.BoxI(afwGeom.Point2I(0, 0), dataBox.getDimensions())
    newData = Image(new, newDataBox, afwImage.LOCAL)
    newData <<= data

    bias = Image(template, biasBox, afwImage.LOCAL)
    newBiasBox = afwGeom.BoxI(biasBox)
    newBiasBox.shift(afwGeom.Extent2I(dataBox.getDimensions().getX(), 0))
    newBias = Image(new, newBiasBox, afwImage.LOCAL)
    newBias <<= bias

    return new

class SuprimecamMapper(CameraMapper):
    def __init__(self, mit=False, rerun=None, outRoot=None, **kwargs):
        policyFile = pexPolicy.DefaultPolicyFile("obs_subaru", "SuprimecamMapper.paf", "policy")
        policy = pexPolicy.Policy(policyFile)

        self.mit = mit
        if self.mit:
            policy.set("camera", "../suprimecam/description/Full_Suprimecam_MIT_geom.paf")
            policy.set("defects", "../suprimecam/description/mit_defects")

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

    def std_raw(self, item, dataId):
        exposure = super(SuprimecamMapper, self).std_raw(item, dataId)

        # Furusawa-san writes about the MIT detectors (used in Suprime-Cam prior to July 2008): """There are
        # two set of CCDs which have different geometry. The two kinds can be discriminated by the keyword
        # S_XFLIP or DET-ID as below.
        # 
        # DET-ID S_XFLIP S_YFLIP EFP-MIN1  OVERSCAN
        # 0 T T 33  LEFT
        # 1 T T 33  LEFT
        # 2 T F 33  LEFT
        # 3 T F 33  LEFT
        # 4 T T 33  LEFT
        # 5 F T 1  RIGHT
        # 6 F F 1  RIGHT
        # 7 F F 1  RIGHT
        # 8 F F 1  RIGHT
        # 9 T T 33  LEFT
        # """
        #
        # Unfortunately, this sort of CCD-dependent layout cannot be accounted for using the current
        # cameraGeometry framework --- while it allows for each CCD to have a different orientation, it
        # assumes that all CCDs are laid out (i.e., position of bias relative to the data) in the same manner.
        # To get around this, we will re-format the tricky CCDs as we read them.

        if (not self.mit) or exposure.getDetector().getId().getSerial() in (5, 6, 7, 8):
            # No correction to be made
            return exposure

        # MIT CCD 0, 1, 2, 3, 4, 9
        # Bias is on the left; put it on the right
        mi = exposure.getMaskedImage()
        image = mi.getImage()
        Image = type(image)
        ccd = exposure.getDetector().getId().getSerial()

        biasColStart = 0                # Starting column for bias
        dataColStart = 32               # Starting column for data
        numCols = 2048                  # Number of data columns
        numRows = 4100                  # Number of data (and bias) rows
        numBias = 32                    # Number of bias columns

        dataBox = afwGeom.Box2I(afwGeom.Point2I(dataColStart, 0), afwGeom.Extent2I(numCols, numRows))
        biasBox = afwGeom.Box2I(afwGeom.Point2I(biasColStart, 0), afwGeom.Extent2I(numBias, numRows))

        newImage = fixImage(mi.getImage(), numCols + numBias, numRows, dataBox, biasBox)
        newMask = fixImage(mi.getMask(), numCols + numBias, numRows, dataBox, biasBox)
        newVariance = fixImage(mi.getVariance(), numCols + numBias, numRows, dataBox, biasBox)

        assert newImage is not None
        newMi = type(mi)(newImage, newMask, newVariance)
        exposure.setMaskedImage(newMi)

        return exposure
