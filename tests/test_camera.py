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
import unittest

import lsst.utils.tests
from lsst.obs.subaru import HyperSuprimeCam


class CameraTestCase(lsst.utils.tests.TestCase):

    def setUp(self):
        hsc = HyperSuprimeCam()
        self.camera = hsc.getCamera()

    def tearDown(self):
        del self.camera

    def testName(self):
        self.assertEqual(self.camera.getName(), "HSC")

    def testNumCcds(self):
        self.assertEqual(len(list(self.camera.getIdIter())), 112)

    def testCcdSize(self):
        for ccd in self.camera:
            self.assertEqual(ccd.getBBox().getWidth(), 2048)
            self.assertEqual(ccd.getBBox().getHeight(), 4176)


class TestMemory(lsst.utils.tests.MemoryTestCase):
    def setUp(self):
        lsst.utils.tests.MemoryTestCase.setUp(self)


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
