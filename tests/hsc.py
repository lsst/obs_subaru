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
import lsst.afw.math as afwMath
import lsst.afw.image as afwImage
import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.cameraGeom.utils as cameraGeomUtils

# Enable display?
try:
    type(display)
except NameError:
    display = False

# Enable assembly?
try:
    type(assemble)
    if assemble:
        try:
            from lsst.ip.isr import ccdAssemble as ca
        except ImportError, e:
            print "Unable to import lsst.ip.isr (%s): assembly disabled" % e
            assemble = False
except NameError:
    assemble = False


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

        frame = 1
        for ccdNum, rotated in zip(self.ccdList, self.rotated):
            butler = getButler(self.datadir)
            raw = butler.get("raw", visit=self.expId, ccd=ccdNum)
            ccd = raw.getDetector()

            print "Visit: ", self.expId
            print "width: ",              raw.getWidth()
            print "height: ",             raw.getHeight()
            ccdName = ccd.getId().getName()
            print "detector name: ", ccdName
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
                print "amp: %s %i"%(amp.getId(), i)

                # Amplifier on CCD
                datasec = amp.getAllPixels(True)
                self.assertEqual(datasec.getWidth(), self.ampSize[0] if not rotated else self.ampSize[1])
                self.assertEqual(datasec.getHeight(), self.ampSize[1] if not rotated else self.ampSize[0])
                self.assertEqual(datasec.getMinX(), i*self.ampSize[0] if not rotated else 0)
                self.assertEqual(datasec.getMinY(), 0 if not rotated else i*self.ampSize[0])

                # Amplifier on disk
                datasec = amp.getRawDataBBox()
                self.assertEqual(datasec.getWidth(), self.ampSize[0])
                self.assertEqual(datasec.getHeight(), self.ampSize[1])

            if display:
                ds9.mtv(raw, frame=frame, title="Raw %s" % ccdName)
                frame += 1
                for amp in ccd:
                    amp = cameraGeom.cast_Amp(amp)
                    print ccd.getId(), amp.getId(), amp.getDataSec().toString(), \
                          amp.getBiasSec().toString(), amp.getElectronicParams().getGain()
                ccdIm = afwMath.rotateImageBy90(raw.getMaskedImage().getImage(),
                                                ccd.getOrientation().getNQuarter())
                cameraGeomUtils.showCcd(ccd, ccdImage=ccdIm, frame=frame)
                frame += 1
            if assemble:
                ccdIm = ca.assembleCcd([raw], ccd, reNorm=False, imageFactory=afwImage.ImageU)
                if display:
                    cameraGeomUtils.showCcd(ccd, ccdImage=ccdIm, frame=frame)
                    frame += 1
                    # If you'd like the image in un-rotated coordinates...
                    rotCcdIm = afwMath.rotateImageBy90(ccdIm.getMaskedImage().getImage(),
                                                       -ccd.getOrientation().getNQuarter())
                    ds9.mtv(rotCcdIm, frame=frame, title="Assembled derotated %s" % ccdName)
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
