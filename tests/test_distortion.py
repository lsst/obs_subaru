#!/usr/bin/env python
#
# LSST Data Management System
#
# Copyright 2008-2016  AURA/LSST.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <https://www.lsstcorp.org/LegalNotices/>.
#
"""
Tests that pixel to focal plane transformations are consistent with previous
implementations. This tests is specific to HSC.
"""
from __future__ import absolute_import, division, print_function
import os.path
import unittest
import pickle
from collections import namedtuple

import lsst.utils.tests
from lsst.obs.hsc import HscMapper
from lsst.afw.cameraGeom import FOCAL_PLANE, PUPIL, PIXELS

# Set SAVE_DATA True to save new distortion data; this will make the test fail,
# to remind you to set it False before committing the code.
SAVE_DATA = False

CcdData = namedtuple("CcdData", ['serial', 'cornerDict'])
CornerData = namedtuple("CornerData", ['focalPlane', "pupil", "focalPlaneRoundTrip", "pixPosRoundTrip"])
DataFileName = "data/distortionData.pickle"  # path relative to the tests directory


class HscDistortionTestCase(lsst.utils.tests.TestCase):
    """Testing HscDistortion implementation

    HscDistortion is based on the HSC package "distEst".  We
    test that it produces the same results.
    """
    def testDistortion(self):
        """Test that the distortion data matches the saved data

        Or or save new data if SAVE_DATA is true.
        """
        newData = self.makeDistortionData()
        dataPath = os.path.join(os.path.dirname(__file__), DataFileName)

        if SAVE_DATA:
            with open(dataPath, "wb") as dataFile:
                pickle.dump(newData, dataFile, protocol=2)
            self.fail("Saved new data to %r; please set SAVE_DATA back to False" % dataPath)

        if not os.path.exists(dataPath):
            self.fail("Cannot find saved data %r; set SAVE_DATA = True and run again to save new data" %
                      dataPath)

        with open(dataPath, "rb") as dataFile:
            savedData = pickle.load(dataFile)
        for detectorName, ccdData in newData.items():
            savedCcdData = savedData[detectorName]
            self.assertEqual(ccdData.serial, savedCcdData.serial)
            for pixPos, cornerData in ccdData.cornerDict.items():
                savedCornerData = savedCcdData.cornerDict[pixPos]
                self.assertPairsAlmostEqual(cornerData.focalPlane, savedCornerData.focalPlane)
                self.assertPairsAlmostEqual(cornerData.pupil, savedCornerData.pupil)
                self.assertPairsAlmostEqual(cornerData.focalPlaneRoundTrip,
                                            savedCornerData.focalPlaneRoundTrip)
                self.assertPairsAlmostEqual(cornerData.pixPosRoundTrip, savedCornerData.pixPosRoundTrip)

    def makeDistortionData(self):
        """Make distortion data
        """
        camera = HscMapper(root=".").camera
        focalPlaneToPupil = camera.getTransformMap().get(PUPIL)
        data = {}  # dict of detector name: CcdData
        for detector in camera:
            # for each corner of each CCD:
            # - get pixel position
            # - convert to focal plane coordinates using the detector and record it
            # - convert to field angle (this is the conversion that uses HscDistortion) and record it
            # - convert back to focal plane (testing inverse direction of HscDistortion) and record it
            # - convert back to pixel position and record it; pixel <-> focal plane is affine
            #   so there is no reason to doubt the inverse transform, but there is no harm
            pixelsToFocalPlane = detector.getTransform(FOCAL_PLANE)
            cornerDict = {}
            for pixPos in detector.getCorners(PIXELS):
                focalPlane = pixelsToFocalPlane.forwardTransform(pixPos)
                pupil = focalPlaneToPupil.forwardTransform(focalPlane)
                focalPlaneRoundTrip = focalPlaneToPupil.reverseTransform(pupil)
                pixPosRoundTrip = pixelsToFocalPlane.reverseTransform(focalPlaneRoundTrip)
                # Use a tuple (x, y) as a the key instead of a Point2D, for assured reasonable hashability
                cornerDict[(pixPos[0], pixPos[1])] = CornerData(
                    focalPlane = focalPlane,
                    pupil = pupil,
                    focalPlaneRoundTrip = focalPlaneRoundTrip,
                    pixPosRoundTrip = pixPosRoundTrip,
                )

            data[detector.getName()] = CcdData(serial=detector.getSerial(), cornerDict=cornerDict)

        return data


class TestMemory(lsst.utils.tests.MemoryTestCase):
    def setUp(self):
        HscMapper.clearCache()
        lsst.utils.tests.MemoryTestCase.setUp(self)


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
