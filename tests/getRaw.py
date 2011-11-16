#!/usr/bin/env python

import os

import unittest
import lsst.utils.tests as utilsTests

from lsst.pex.policy import Policy
import lsst.daf.persistence as dafPersist
from lsst.obs.suprimecam import SuprimecamMapper

import lsst.afw.display.ds9 as ds9
import lsst.afw.display.utils as displayUtils

import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.cameraGeom.utils as cameraGeomUtils
try:
    type(display)
except NameError:
    display = False

class GetRawTestCase(unittest.TestCase):
    """Testing butler raw image retrieval"""

    def setUp(self):
        datadir = os.getenv("TESTDATA_SUBARU_DIR")
        assert datadir, "testdata_subaru is not setup"
        self.bf = dafPersist.ButlerFactory(mapper=SuprimecamMapper(
            root=os.path.join(datadir, "science"),
            calibRoot=os.path.join(datadir, "calib")))
        self.butler = self.bf.create()

    def tearDown(self):
        del self.butler
        del self.bf

    def testRaw(self):
        """Test retrieval of raw image"""

        frame = 0
        for ccdNum, ccdName in [(2, "Fio"), (5, "Satsuki")]:
            raw = self.butler.get("raw", visit=127073, ccd=ccdNum)

            print "width: ",              raw.getWidth()
            print "height: ",             raw.getHeight()
            print "detector(amp) name: ", raw.getDetector().getId().getName()

            self.assertEqual(raw.getWidth(), 2272) # untrimmed
            self.assertEqual(raw.getHeight(), 4273) # untrimmed

            self.assertEqual(raw.getFilter().getFilterProperty().getName(), "i") 
            self.assertEqual(raw.getDetector().getId().getName(), ccdName)

            if display:
                ccd = cameraGeom.cast_Ccd(raw.getDetector())
                for amp in ccd:
                    amp = cameraGeom.cast_Amp(amp)
                    print ccd.getId(), amp.getId(), amp.getDataSec().toString(), \
                          amp.getBiasSec().toString(), amp.getElectronicParams().getGain()
                cameraGeomUtils.showCcd(ccd, ccdImage=raw, frame=frame)
                frame += 1
        

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(GetRawTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit = False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
