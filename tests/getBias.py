#!/usr/bin/env python

import os

import unittest
import lsst.utils.tests as utilsTests

from lsst.obs.subaru import SuprimeMapper

import lsst.daf.persistence as dafPersist


class GetBiasTestCase(unittest.TestCase):
    """Testing butler bias image retrieval"""

    def setUp(self):
        policy = Policy.createPolicy("./policy/SuprimeMapper.paf")
        self.bf = dafPersist.ButlerFactory(mapper=SuprimeMapper(
            policy=policy,root="./tests/science",
	    calibRoot="./tests/calib"))
	
        self.butler = self.bf.create()


    def tearDown(self):
        del self.butler
        del self.bf

    def testBias(self):
        """Test retrieval of bias image"""
	
        raw = self.butler.get("bias", visit=100525, ccd=1, taiObs="2008-08-22", mystery="001", amp=0)
	print "width: ",              raw.getWidth()
	print "height: ",             raw.getHeight()
	print "detector(amp) name: ", raw.getDetector().getId().getName()
	print "detector(ccd) name: ", raw.getDetector().getParent().getId().getName()
        self.assertEqual(raw.getWidth(), 2048)  #trimmed
        self.assertEqual(raw.getHeight(), 4177) #trimmed
        self.assertEqual(raw.getDetector().getId().getName(), "ID0")
        self.assertEqual(raw.getDetector().getParent().getId().getName(), "Kiki")

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(GetBiasTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit = False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
