#!/usr/bin/env python
#
# LSST Data Management System
#
# Copyright 2008-2015  AURA/LSST.
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
from collections import namedtuple

import lsst.utils.tests as utilsTests
from lsst.afw.image import Filter
from lsst.obs.hsc import HscMapper


class CameraTestCase(utilsTests.TestCase):

    def setUp(self):
        self.mapper = HscMapper(root=".")
        self.camera = self.mapper.camera

    def tearDown(self):
        del self.camera
        del self.mapper

    def testName(self):
        self.assertEqual(self.camera.getName(), "HSC")

    def testNumCcds(self):
        self.assertEqual(len(list(self.camera.getIdIter())), 112)

    def testCcdSize(self):
        for ccd in self.camera:
            self.assertEqual(ccd.getBBox().getWidth(), 2048)
            self.assertEqual(ccd.getBBox().getHeight(), 4176)

    def testFilters(self):
        # Check that the mapper has defined some standard filters.
        # Note that this list is not intended to be comprehensive -- we
        # anticipate that more filters can be added without causing the test
        # to break -- but captures the standard HSC broad-band filters.
        FilterName = namedtuple("FilterName", ["alias", "canonical"])
        filterNames = (
            FilterName(alias="HSC-G", canonical="g"),
            FilterName(alias="HSC-R", canonical="r"),
            FilterName(alias="HSC-I", canonical="i"),
            FilterName(alias="HSC-Z", canonical="z"),
            FilterName(alias="HSC-Y", canonical="y"),
            FilterName(alias="NONE", canonical="UNRECOGNISED")
        )
        for filterName in filterNames:
            self.assertTrue(filterName.alias in self.mapper.filters)
            self.assertEqual(Filter(filterName.alias).getCanonicalName(), filterName.canonical)


def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(CameraTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)


def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
