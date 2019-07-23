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
import math
import os.path
import unittest
import pickle

import lsst.utils.tests
from lsst.obs.hsc import HscMapper
from lsst.pipe.base import Struct
from lsst.afw.geom import transformRegistry
from lsst.afw.cameraGeom import FOCAL_PLANE, FIELD_ANGLE, PIXELS

# Set SAVE_DATA True to save new distortion data; this will make the test fail,
# to remind you to set it False before committing the code.
SAVE_DATA = False

DataFileName = "data/distortionData.pickle"  # path relative to the tests directory


class HscDistortionTestCase(lsst.utils.tests.TestCase):
    """Testing HscDistortion implementation

    HscDistortion is based on the HSC package "distEst".  We
    test that it produces the same results.
    """
    def testDistortion(self):
        """Test that the distortion data matches the saved data or create new data

        If SAVE_DATA is true then save newly created data and then fail the test
        in order to prevent anyone from committing the test with SAVE_DATA true!

        Otherwise create new data and compare to the saved data
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

        fieldAngleToFocalPlaneTolerance = transformRegistry["hsc"].ConfigClass().tolerance

        with open(dataPath, "rb") as dataFile:
            savedData = pickle.load(dataFile)
        maxRoundTripFocalPlaneError = 0
        maxRoundTripPixPosError = 0
        for detectorName, ccdData in newData.items():
            savedCcdData = savedData[detectorName]
            self.assertEqual(ccdData.serial, savedCcdData.serial)
            for pixPosKey, cornerData in ccdData.cornerDict.items():
                savedCornerData = savedCcdData.cornerDict[pixPosKey]
                self.assertEqual(cornerData.pixPos, savedCornerData.pixPos)
                self.assertPairsAlmostEqual(cornerData.focalPlane, savedCornerData.focalPlane)
                self.assertPairsAlmostEqual(cornerData.fieldAngle, savedCornerData.fieldAngle)
                maxRoundTripFocalPlaneError = max(
                    maxRoundTripFocalPlaneError,
                    math.hypot(*(cornerData.focalPlaneRoundTrip - cornerData.focalPlane))
                )
                self.assertPairsAlmostEqual(cornerData.focalPlaneRoundTrip, cornerData.focalPlane,
                                            maxDiff=fieldAngleToFocalPlaneTolerance)
                maxRoundTripPixPosError = max(maxRoundTripPixPosError,
                                              math.hypot(*(cornerData.pixPosRoundTrip - cornerData.pixPos)))
                self.assertPairsAlmostEqual(cornerData.pixPosRoundTrip, cornerData.pixPos)
        print("maxRoundTripFocalPlaneError =", maxRoundTripFocalPlaneError)
        print("maxRoundTripPixPosError =", maxRoundTripPixPosError)

    def makeDistortionData(self):
        """Make distortion data

        The data format is a dict of detector name: ccdData, where
        ccdData is a Struct containing these fields:
        - serial: detector.getSerial
        - cornerDict: a dict of pixPosKey, cornerData, where:
            - pixPosKey: self.asKey(pixPos) where pixPos is pixel position
            - cornerData is Struct contains these fields (all of type lsst.geom.Point2D):
                - pixPos: pixel position
                - focalPlane: focal plane position computed from pixPos
                - fieldAngle: fieldAngle position computed from focalPlane
                - focalPlaneRoundTrip: focal plane position computed from fieldAngle
                - pixPosRoundTrip: pixel position computed from focalPlane
        """
        camera = HscMapper(root=".", calibRoot=".").camera
        focalPlaneToFieldAngle = camera.getTransformMap().getTransform(FOCAL_PLANE, FIELD_ANGLE)
        data = {}  # dict of detector name: CcdData
        for detector in camera:
            # for each corner of each CCD:
            # - get pixel position
            # - convert to focal plane coordinates using the detector and record it
            # - convert to field angle (this is the conversion that uses HscDistortion) and record it
            # - convert back to focal plane (testing inverse direction of HscDistortion) and record it
            # - convert back to pixel position and record it; pixel <-> focal plane is affine
            #   so there is no reason to doubt the inverse transform, but there is no harm
            pixelsToFocalPlane = detector.getTransform(PIXELS, FOCAL_PLANE)
            cornerDict = {}
            for pixPos in detector.getCorners(PIXELS):
                pixPos = pixPos
                focalPlane = pixelsToFocalPlane.applyForward(pixPos)
                fieldAngle = focalPlaneToFieldAngle.applyForward(focalPlane)
                focalPlaneRoundTrip = focalPlaneToFieldAngle.applyInverse(fieldAngle)
                pixPosRoundTrip = pixelsToFocalPlane.applyInverse(focalPlane)
                cornerDict[self.toKey(pixPos)] = Struct(
                    pixPos = pixPos,
                    focalPlane = focalPlane,
                    fieldAngle = fieldAngle,
                    focalPlaneRoundTrip = focalPlaneRoundTrip,
                    pixPosRoundTrip = pixPosRoundTrip,
                )

            data[detector.getName()] = Struct(serial=detector.getSerial(), cornerDict=cornerDict)

        return data

    def toKey(self, pixPos):
        return "(%0.1f, %0.1f)" % tuple(pixPos)


class TestMemory(lsst.utils.tests.MemoryTestCase):
    def setUp(self):
        HscMapper.clearCache()
        lsst.utils.tests.MemoryTestCase.setUp(self)


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
