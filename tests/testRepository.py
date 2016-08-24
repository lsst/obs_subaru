#!/usr/bin/env python
#
# LSST Data Management System
#
# Copyright 2008-2016 AURA/LSST.
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

import os
import unittest
from glob import glob
from shutil import rmtree
from subprocess import check_call
from tempfile import mkdtemp

import lsst.afw.geom as afwGeom
import lsst.daf.persistence as dafPersist
import lsst.pex.exceptions as pexExcept
import lsst.utils.tests as utilsTests
from lsst.utils import getPackageDir

TEST_DATA_PACKAGE = "testdata_subaru"


def requirePackage(packageName):
    """
    Decorator to skip unit tests unless the package ``packageName`` is set up.
    """
    try:
        getPackageDir(packageName)
    except pexExcept.NotFoundError as e:
        return unittest.skip("Required package %s not available [%s]." % (packageName, str(e)))
    return lambda func: func


def createDataRepository(mapperName, inputPath):
    """
    Construct a data repository for a particular mapper containing a given set
    of input data and return its path.

    @param[in] mapperName   Name of mapper class for use with repository.
    @param[in] inputPath    Location of data files which will be ingested.
    """
    repoPath = mkdtemp()
    with open(os.path.join(repoPath, "_mapper"), "w") as _mapper:
        _mapper.write(mapperName)
    ingest_cmd = os.path.join(getPackageDir("obs_subaru"), "bin", "hscIngestImages.py")
    check_call([ingest_cmd, repoPath] + glob(os.path.join(inputPath, "*.fits.gz")))
    return repoPath


class IngestRawTestCase(utilsTests.TestCase):
    """Ensure that we can ingest raw data to a repository."""
    @requirePackage(TEST_DATA_PACKAGE)
    def setUp(self):
        rawPath = os.path.join(getPackageDir(TEST_DATA_PACKAGE), "hsc", "raw")
        self.repoPath = createDataRepository("lsst.obs.hsc.HscMapper", rawPath)

    def tearDown(self):
        rmtree(self.repoPath)

    def testIngest(self):
        self.assertTrue(os.path.exists(os.path.join(self.repoPath, "registry.sqlite3")))

    def testMapperName(self):
        name = dafPersist.Butler.getMapperClass(root=self.repoPath).packageName
        self.assertEqual(name, "obs_subaru")


class GetDataTestCase(utilsTests.TestCase):
    """Demonstrate retrieval of data from a repository."""
    @requirePackage(TEST_DATA_PACKAGE)
    def setUp(self):
        rawPath = os.path.join(getPackageDir(TEST_DATA_PACKAGE), "hsc", "raw")
        calibPath = os.path.join(getPackageDir(TEST_DATA_PACKAGE), "hsc", "calib")
        self.repoPath = createDataRepository("lsst.obs.hsc.HscMapper", rawPath)
        self.butler = dafPersist.Butler(root=self.repoPath, calibRoot=calibPath)

        # The following properties of the provided data are known a priori.
        self.visit = 904024
        self.ccdNum = 50
        self.filter = 'i'
        self.rawSize = (2144, 4241)
        self.ccdSize = (2048, 4176)

    def tearDown(self):
        del self.butler
        rmtree(self.repoPath)

    def testRaw(self):
        """Test retrieval of raw image"""
        raw = self.butler.get("raw", visit=self.visit, ccd=self.ccdNum)
        ccd = raw.getDetector()
        self.assertEqual(raw.getDimensions(), afwGeom.Extent2I(*self.rawSize))
        self.assertEqual(raw.getFilter().getFilterProperty().getName(), self.filter)
        self.assertEqual(ccd.getId(), self.ccdNum)
        self.assertEqual(ccd.getBBox().getDimensions(), afwGeom.Extent2I(*self.ccdSize))

    def testRawMetadata(self):
        """Test retrieval of raw image metadata"""
        md = self.butler.get("raw_md", visit=self.visit, ccd=self.ccdNum)
        self.assertEqual(md.get("DET-ID"), self.ccdNum)

    def testBias(self):
        """Test retrieval of bias frame"""
        bias = self.butler.get("bias", visit=self.visit, ccd=self.ccdNum)
        self.assertEqual(bias.getDimensions(), afwGeom.Extent2I(*self.ccdSize))
        self.assertEqual(bias.getDetector().getId(), self.ccdNum)

    def testDark(self):
        """Test retrieval of dark frame"""
        dark = self.butler.get("dark", visit=self.visit, ccd=self.ccdNum)
        self.assertEqual(dark.getDimensions(), afwGeom.Extent2I(*self.ccdSize))
        self.assertEqual(dark.getDetector().getId(), self.ccdNum)

    def testFlat(self):
        """Test retrieval of flat"""
        flat = self.butler.get("flat", visit=self.visit, ccd=self.ccdNum)
        self.assertEqual(flat.getDimensions(), afwGeom.Extent2I(*self.ccdSize))
        self.assertEqual(flat.getDetector().getId(), self.ccdNum)


def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(IngestRawTestCase)
    suites += unittest.makeSuite(GetDataTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)


def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
