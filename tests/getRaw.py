#!/usr/bin/env python

import os

import unittest
import lsst.utils.tests as utilsTests

from lsst.pex.policy import Policy
import lsst.daf.persistence as dafPersist
from lsst.obs.suprimecam import SuprimecamMapper

class GetRawTestCase(unittest.TestCase):
    """Testing butler raw image retrieval"""

    def setUp(self):
        datadir = os.getenv("OBS_TEST_SUBARU_DIR")
        assert datadir, "obs_test_subaru is not setup"
        self.bf = dafPersist.ButlerFactory(mapper=SuprimecamMapper(
            root=os.path.join(datadir, "science"),
            calibRoot=os.path.join(datadir, "calib"))
        self.butler = self.bf.create()

    def tearDown(self):
        del self.butler
        del self.bf

    def testRaw(self):
        """Test retrieval of raw image"""
        raw = self.butler.get("raw", visit=127073, ccd=2)
    
        print "width: ",              raw.getWidth()
        print "height: ",             raw.getHeight()
        print "detector(amp) name: ", raw.getDetector().getId().getName()
        
        self.assertEqual(raw.getWidth(), 2272) # untrimmed
        self.assertEqual(raw.getHeight(), 4273) # untrimmed
    
        self.assertEqual(raw.getFilter().getFilterProperty().getName(), "i") 
        self.assertEqual(raw.getDetector().getId().getName(), "Fio")

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
