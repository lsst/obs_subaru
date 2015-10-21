#!/usr/bin/env python

import unittest
import lsst.utils.tests as utilsTests
import lsst.afw.display.ds9 as ds9
import lsst.pex.policy as pexPolicy

import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.cameraGeom.utils as cameraGeomUtils

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def trimCcd(ccd, ccdImage=""):
    """Trim a Ccd and maybe the image of the untrimmed Ccd"""

    if ccdImage == "":
        ccdImage = cameraGeomUtils.makeImageFromCcd(ccd)

    if ccd.isTrimmed():
        return ccdImage

    ccd.setTrimmed(True)

    if ccdImage is not None:
        trimmedImage = ccdImage.Factory(ccd.getAllPixels().getDimensions())
        for a in ccd:
            data =      ccdImage.Factory(ccdImage, a.getDataSec(False))
            tdata = trimmedImage.Factory(trimmedImage, a.getDataSec())
            tdata[:] = data
    else:
        trimmedImage = None

    return trimmedImage

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class SuprimecamGeomTestCase(unittest.TestCase):
    """A test case for Suprime Cam camera geometry"""

    def setUp(self):
        SuprimecamGeomTestCase.ampSerial = [0] # an array so we pass the value by reference

        policyFile = pexPolicy.DefaultPolicyFile("afw", "CameraGeomDictionary.paf", "policy")
        defPolicy = pexPolicy.Policy.createPolicy(policyFile, policyFile.getRepositoryPath(), True)

        polFile = pexPolicy.DefaultPolicyFile("obs_subaru",
                                              "Full_Suprimecam_geom.paf",
                                              "suprimecam")
        self.geomPolicy = pexPolicy.Policy.createPolicy(polFile)
        self.geomPolicy.mergeDefaults(defPolicy.getDictionary())

    def xtestDictionary(self):
        """Test the camera geometry dictionary"""

        if True:
            for r in self.geomPolicy.getArray("Raft"):
                print "raft", r
            for c in self.geomPolicy.getArray("Ccd"):
                print "ccd", c
            for a in self.geomPolicy.getArray("Amp"):
                print "amp", a

    def testCamera(self):
        camera = cameraGeomUtils.makeCamera(self.geomPolicy)
        # XXX This is interactive
        #ds9.setDefaultFrame(0)
        #cameraGeomUtils.showCamera(camera, )

    def tearDown(self):
        del self.geomPolicy

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(SuprimecamGeomTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(exit=False):
    """Run the tests"""

    utilsTests.run(suite(), exit)

if __name__ == "__main__":
    run(True)

