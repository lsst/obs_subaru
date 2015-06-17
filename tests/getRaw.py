#!/usr/bin/env python

import os

import unittest
import lsst.utils.tests as utilsTests

import lsst.daf.persistence as dafPersist
from lsst.obs.suprimecam import SuprimecamMapper

import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.cameraGeom.utils as cameraGeomUtils
try:
    type(display)
except NameError:
    display = False


def getButler(datadir, mit=False):
    bf = dafPersist.ButlerFactory(mapper=SuprimecamMapper(mit=mit, root=os.path.join(datadir, "science"),
                                                          calibRoot=os.path.join(datadir, "calib")))
    return bf.create()


class GetRawTestCase(unittest.TestCase):
    """Testing butler raw image retrieval"""

    def setUp(self):
        self.datadir = os.getenv("TESTDATA_SUBARU_DIR")
        self.sizes = { True: (2080, 4100), # Old Suprime-Cam (MIT detectors)
                       False: (2272, 4273) # New Suprime-Cam (Hamamatsu detectors)
                       }
        self.names = { True: ['w67c1', 'w6c1', 'si005s', 'si001s', 'si002s',
                              'si006s', 'w93c2', 'w9c2', 'w4c5', 'w7c3'],
                       False: ['Nausicaa', 'Kiki', 'Fio', 'Sophie', 'Sheeta',
                               'Satsuki', 'Chihiro', 'Clarisse', 'Ponyo', 'San']
                       }
        assert self.datadir, "testdata_subaru is not setup"

    def testRaw(self):
        """Test retrieval of raw image"""

        frame = 0
        for expId, mit in [(22657, True), (127073, False)]:
            for ccdNum in [2, 5]:
                butler = getButler(self.datadir, mit)
                raw = butler.get("raw", visit=expId, ccd=ccdNum)

                print "Visit: ", expId
                print "MIT detector: ", mit
                print "width: ",              raw.getWidth()
                print "height: ",             raw.getHeight()
                print "detector(amp) name: ", raw.getDetector().getId().getName()

                self.assertEqual(raw.getWidth(), self.sizes[mit][0]) # untrimmed
                self.assertEqual(raw.getHeight(), self.sizes[mit][1]) # untrimmed

                self.assertEqual(raw.getFilter().getFilterProperty().getName(), "i")
                self.assertEqual(raw.getDetector().getId().getName(), self.names[mit][ccdNum])

                if display:
                    ccd = raw.getDetector()
                    for amp in ccd:
                        amp = cameraGeom.cast_Amp(amp)
                        print ccd.getId(), amp.getId(), amp.getDataSec().toString(), \
                              amp.getBiasSec().toString(), amp.getElectronicParams().getGain()
                    cameraGeomUtils.showCcd(ccd, ccdImage=raw, frame=frame)
                    frame += 1

    def testPackageName(self):
        self.assertEqual(self.butler.mapper.packageName, "obs_subaru")


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
