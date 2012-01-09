#!/usr/bin/env python

import os

import unittest
import lsst.utils.tests as utilsTests

from lsst.pex.policy import Policy
import lsst.daf.persistence as dafPersist
from lsst.obs.hscSim import HscSimMapper

import lsst.afw.display.ds9 as ds9
import lsst.afw.display.utils as displayUtils

import lsst.afw.geom as afwGeom
import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.cameraGeom.utils as cameraGeomUtils
try:
    type(display)
except NameError:
    display = False


def getButler(datadir):
    bf = dafPersist.ButlerFactory(mapper=HscSimMapper(root=os.path.join(datadir, "hsc")))
    return bf.create()


class GetRawTestCase(unittest.TestCase):
    """Testing butler raw image retrieval"""

    def setUp(self):
        self.datadir = os.getenv("TESTDATA_SUBARU_DIR")
        self.expId = 204
        self.ccdList = (0, 100)
        self.rotated = (False, True)
        self.untrimmedSize = (2272, 4273)
        self.trimmedSize = (2048, 4096)
        self.ampSize = (512, 4096)

        assert self.datadir, "testdata_subaru is not setup"

    def testRaw(self):
        """Test retrieval of raw image"""

        frame = 0
        for ccdNum, rotated in zip(self.ccdList, self.rotated):
            butler = getButler(self.datadir)
            raw = butler.get("raw", visit=self.expId, ccd=ccdNum)
            ccd = cameraGeom.cast_Ccd(raw.getDetector())

            print "Visit: ", self.expId
            print "width: ",              raw.getWidth()
            print "height: ",             raw.getHeight()
            print "detector(amp) name: ", ccd.getId().getName()

            self.assertEqual(raw.getWidth(), self.untrimmedSize[0])
            self.assertEqual(raw.getHeight(), self.untrimmedSize[1])
            self.assertEqual(raw.getFilter().getFilterProperty().getName(), "g")
            self.assertEqual(ccd.getId().getName(), "hsc%03d" % ccdNum)

            # CCD size
            trimmed = ccd.getAllPixelsNoRotation(True).getDimensions()
            self.assertEqual(trimmed.getX(), self.trimmedSize[0])
            self.assertEqual(trimmed.getY(), self.trimmedSize[1])

            # Size in camera coordinates
            camera = ccd.getAllPixels(True).getDimensions()
            self.assertEqual(camera.getX(), self.trimmedSize[0] if not rotated else self.trimmedSize[1])
            self.assertEqual(camera.getY(), self.trimmedSize[1] if not rotated else self.trimmedSize[0])

            for i, amp in enumerate(ccd):
                amp = cameraGeom.cast_Amp(amp)

                # Amplifier on CCD
                datasec = amp.getAllPixelsNoRotation(True)
                self.assertEqual(datasec.getWidth(), self.ampSize[0])
                self.assertEqual(datasec.getHeight(), self.ampSize[1])
                self.assertEqual(datasec.getMinX(), (3-i) * self.ampSize[0])
                self.assertEqual(datasec.getMinY(), 0)

                # Amplifier on disk
                datasec = amp.getDiskDataSec()
                self.assertEqual(datasec.getWidth(), self.ampSize[0])
                self.assertEqual(datasec.getHeight(), self.ampSize[1])

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
