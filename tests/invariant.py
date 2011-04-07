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


import unittest
import lsst.utils.tests as utilsTests

import math

import hsc.meas.match.distest as distest

TOLERANCE = 0.01                        # Tolerance for difference after reversing, pixels

class InvarianceTestCase(unittest.TestCase):
    """A test for invariance in the forward+backward transformation"""

    def testInvariance(self):
        # These are the centers of the CCDs
        for x0, y0 in [(-9514.66, -95.9925), (7466.67, -4347.66), (-7391.99, -95.9925), (5344.01, -4347.66),
                       (-5269.33, -95.9925), (3221.34, -4347.66), (-3146.66, -95.9925), (1098.67, -4347.66), 
                       (-1023.99, -95.9925), (-1023.99, -4347.66), (1098.67, -95.9925), (-3146.66, -4347.66), 
                       (3221.34, -95.9925), (-5269.33, -4347.66), (5344.01, -95.9925), (-7391.99, -4347.66), 
                       (7466.67, -95.9925), (-9514.66, -4347.66), (9589.34, -95.9925), (-11637.3, -4347.66), 
                       (11712.0, -95.9925), (-13760.0, -4347.66), (13834.7, -95.9925), (-15882.7, -4347.66), 
                       (-9514.66, 4372.34), (7466.67, -8815.99), (-7391.99, 4372.34), (5344.01, -8815.99), 
                       (-5269.33, 4372.34), (3221.34, -8815.99), (-3146.66, 4372.34), (1098.67, -8815.99), 
                       (-1023.99, 4372.34), (-1023.99, -8815.99), (1098.67, 4372.34), (-3146.66, -8815.99), 
                       (3221.34, 4372.34), (-5269.33, -8815.99), (5344.01, 4372.34), (-7391.99, -8815.99), 
                       (7466.67, 4372.34), (-9514.66, -8815.99), (9589.34, 4372.34), (-11637.3, -8815.99), 
                       (11712.0, 4372.34), (-13760.0, -8815.99), (13834.7, 4372.34), (-15882.7, -8815.99), 
                       (-9514.66, 8840.67), (7466.67, -13284.3), (-7391.99, 8840.67), (5344.01, -13284.3), 
                       (-5269.33, 8840.67), (3221.34, -13284.3), (-3146.66, 8840.67), (1098.67, -13284.3), 
                       (-1023.99, 8840.67), (-1023.99, -13284.3), (1098.67, 8840.67), (-3146.66, -13284.3), 
                       (3221.34, 8840.67), (-5269.33, -13284.3), (5344.01, 8840.67), (-7391.99, -13284.3), 
                       (7466.67, 8840.67), (-9514.66, -13284.3), (9589.34, 8840.67), (-11637.3, -13284.3), 
                       (11712.0, 8840.67), (-13760.0, -13284.3), (-7391.99, 13309.0), (5344.01, -17752.7), 
                       (-5269.33, 13309.0), (3221.34, -17752.7), (-3146.66, 13309.0), (1098.67, -17752.7), 
                       (-1023.99, 13309.0), (-1023.99, -17752.7), (1098.67, 13309.0), (-3146.66, -17752.7), 
                       (3221.34, 13309.0), (-5269.33, -17752.7), (5344.01, 13309.0), (-7391.99, -17752.7), 
                       (9589.34, -4564.33), (-11637.3, 120.674), (11712.0, -4564.33), (-13760.0, 120.674), 
                       (13834.7, -4564.33), (-15882.7, 120.674), (9589.34, -9032.66), (-11637.3, 4589.01), 
                       (11712.0, -9032.66), (-13760.0, 4589.01), (13834.7, -9032.66), (-15882.7, 4589.01), 
                       (9589.34, -13501.0), (-11637.3, 9057.34), (11712.0, -13501.0), (-13760.0, 9057.34), 
                       (-7467.7, 13309.0), (11642.67, 13309.0), (11642.67, -15575.7), (-7467.7, -15575.7)]:
            for elev in [30, 45, 60, 90]:
                xDist, yDist = distest.getDistortedPosition(x0, y0, elev)
                x1, y1 = distest.getUndistortedPosition(xDist, yDist, elev)
                diff = math.hypot(x1 - x0, y1 - y0)
                self.assertLess(diff, TOLERANCE, "Not invariant at elev %f: %f,%f --> %f,%f --> %f,%f (%f)" % \
                                (elev, x0, y0, xDist, yDist, x1, y1, diff))

def suite():
    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(InvarianceTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit = False):
    utilsTests.run(suite(), shouldExit)

if __name__ == '__main__':
    run(True)
