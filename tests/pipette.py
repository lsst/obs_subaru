#!/usr/bin/env python

#
# LSST Data Management System
# Copyright 2008, 2009, 2010 LSST Corporation.
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
# see <http://www.lsstcorp.org/LegalNotices/>.
#

import eups
import unittest
import lsst.utils.tests as utilsTests

import math

import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.geom as afwGeom
import lsst.obs.hscSim as hscSim
import lsst.pipette.config as pipConfig
import lsst.pipette.distortion as pipDist

import hsc.meas.match.distest as distest
import hsc.meas.match.hscDistortion as hscDist

TOLERANCE = 0.01                        # Tolerance for difference after reversing, pixels

class PipetteTestCase(unittest.TestCase):
    """A test for Pipette's distortion in the forward+backward transformation"""

    def setUp(self):
        Eups = eups.Eups()
        assert Eups.isSetup("pipette"), "pipette is not setup"
        assert Eups.isSetup("obs_subaru"), "obs_subaru is not setup"

        mapper = hscSim.HscSimMapper()
        self.camera = mapper.camera

        self.config = pipConfig.Config()
        self.config['class'] = "hsc.meas.match.hscDistortion.HscDistortion"

    def tearDown(self):
        del self.camera
        del self.config

    def testInvariance(self):
        for raft in self.camera:
            raft = cameraGeom.cast_Raft(raft)
            for ccd in raft:
                ccd = cameraGeom.cast_Ccd(ccd)
                dist = pipDist.createDistortion(ccd, self.config)
                self.assertTrue(isinstance(dist, hscDist.HscDistortion))
                size = ccd.getSize()
                height, width = size.getX(), size.getY()
                for x, y in ((0.0,0.0), (0.0, height), (0.0, width), (height, width), (height/2.0,width/2.0)):
                    forward = dist.actualToIdeal(afwGeom.PointD(x, y))
                    backward = dist.idealToActual(forward)
                    diff = math.hypot(backward.getX() - x, backward.getY() - y)
                    self.assertLess(diff, TOLERANCE, "Not invariant: %s %f,%f --> %s --> %s (%f)" % \
                                        (ccd.getId().getSerial(), x, y, forward, backward, diff))

def suite():
    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(PipetteTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit = False):
    utilsTests.run(suite(), shouldExit)

if __name__ == '__main__':
    run(True)
