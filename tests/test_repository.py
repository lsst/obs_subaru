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
import os
import unittest
from glob import glob
from shutil import rmtree
from subprocess import check_call
from tempfile import mkdtemp

import lsst.afw.geom as afwGeom
import lsst.daf.persistence as dafPersist
import lsst.utils
from lsst.daf.base import DateTime
from lsst.obs.base import MakeRawVisitInfo
from lsst.afw.image import RotType
from lsst.afw.coord import IcrsCoord, Coord
from lsst.afw.geom import degrees
from lsst.obs.hsc import HscMapper

testDataPackage = "testdata_subaru"
try:
    testDataDirectory = lsst.utils.getPackageDir(testDataPackage)
except lsst.pex.exceptions.NotFoundError as err:
    testDataDirectory = None


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
    ingest_cmd = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "bin", "hscIngestImages.py")
    check_call([ingest_cmd, repoPath] + glob(os.path.join(inputPath, "*.fits.gz")))
    return repoPath


class IngestRawTestCase(lsst.utils.tests.TestCase):
    """Ensure that we can ingest raw data to a repository."""
    @unittest.skipIf(testDataDirectory is None, "%s is not available" % testDataPackage)
    def setUp(self):
        rawPath = os.path.join(testDataDirectory, "hsc", "raw")
        self.repoPath = createDataRepository("lsst.obs.hsc.HscMapper", rawPath)

    def tearDown(self):
        rmtree(self.repoPath)

    def testIngest(self):
        self.assertTrue(os.path.exists(os.path.join(self.repoPath, "registry.sqlite3")))

    def testMapperName(self):
        name = dafPersist.Butler.getMapperClass(root=self.repoPath).packageName
        self.assertEqual(name, "obs_subaru")


class GetDataTestCase(lsst.utils.tests.TestCase):
    """Demonstrate retrieval of data from a repository."""
    @unittest.skipIf(testDataDirectory is None, "%s is not available" % testDataPackage)
    def setUp(self):
        rawPath = os.path.join(testDataDirectory, "hsc", "raw")
        calibPath = os.path.join(testDataDirectory, "hsc", "calib")
        self.repoPath = createDataRepository("lsst.obs.hsc.HscMapper", rawPath)
        self.butler = dafPersist.Butler(root=self.repoPath, calibRoot=calibPath)

        # The following properties of the provided data are known a priori.
        self.visit = 904024
        self.ccdNum = 50
        self.filter = 'i'
        self.rawSize = (2144, 4241)
        self.ccdSize = (2048, 4176)
        self.exptime = 30.0
        self.darktime = self.exptime  # No explicit darktime
        dateBeg = DateTime(56598.26106374757, DateTime.MJD, DateTime.UTC)
        dateAvgNSec = dateBeg.nsecs() + int(0.5e9*self.exptime)
        self.dateAvg = DateTime(dateAvgNSec, DateTime.TAI)
        self.boresightRaDec = IcrsCoord('21:22:59.982', '+00:30:00.07')
        self.boresightAzAlt = Coord(226.68922661*degrees, 63.04359233*degrees)
        self.boresightAirmass = 1.121626027604189
        self.boresightRotAngle = 270.0*degrees
        self.rotType = RotType.SKY
        self.obs_longitude = -155.476667*degrees
        self.obs_latitude = 19.825556*degrees
        self.obs_elevation = 4139
        self.weath_airTemperature = MakeRawVisitInfo.centigradeFromKelvin(272.35)
        self.weath_airPressure = MakeRawVisitInfo.pascalFromMmHg(621.7)
        self.weath_humidity = 33.1
        # NOTE: if we deal with DM-8053 and get UT1 implemented, ERA will change slightly.
        lst = 340.189608333*degrees
        self.era = lst - self.obs_longitude

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

        visitInfo = raw.getInfo().getVisitInfo()
        self.assertAlmostEqual(visitInfo.getDate().get(), self.dateAvg.get())
        self.assertAnglesNearlyEqual(visitInfo.getEra(), self.era)
        self.assertCoordsNearlyEqual(visitInfo.getBoresightRaDec(), self.boresightRaDec)
        self.assertCoordsNearlyEqual(visitInfo.getBoresightAzAlt(), self.boresightAzAlt)
        self.assertAlmostEqual(visitInfo.getBoresightAirmass(), self.boresightAirmass)
        self.assertAnglesNearlyEqual(visitInfo.getBoresightRotAngle(), self.boresightRotAngle)
        self.assertEqual(visitInfo.getRotType(), self.rotType)
        self.assertEqual(visitInfo.getExposureTime(), self.exptime)
        self.assertEqual(visitInfo.getDarkTime(), self.darktime)
        observatory = visitInfo.getObservatory()
        self.assertAnglesNearlyEqual(observatory.getLongitude(), self.obs_longitude)
        self.assertAnglesNearlyEqual(observatory.getLatitude(), self.obs_latitude)
        self.assertAlmostEqual(observatory.getElevation(), self.obs_elevation)
        weather = visitInfo.getWeather()
        self.assertAlmostEqual(weather.getAirTemperature(), self.weath_airTemperature)
        self.assertAlmostEqual(weather.getAirPressure(), self.weath_airPressure)
        self.assertAlmostEqual(weather.getHumidity(), self.weath_humidity)

    def testPupil(self):
        """Test retrieval of pupil (without checking value)"""
        raw = self.butler.get("raw", visit=self.visit, ccd=self.ccdNum)
        visitInfo = raw.getInfo().getVisitInfo()
        camera = self.butler.get("camera", visit=self.visit, ccd=self.ccdNum)
        size = 16.4
        npix = 1024

        pupilFactory = camera.getPupilFactory(visitInfo, size, npix)
        self.assertIsInstance(pupilFactory, lsst.obs.hsc.hscPupil.HscPupilFactory)

        pupil = pupilFactory.getPupil(afwGeom.Point2D(0.0, 0.0))
        self.assertEqual(pupil.size, size)
        self.assertEqual(pupil.scale, size/npix)
        self.assertEqual(pupil.illuminated.shape, (npix, npix))

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
        self.assertEqual(dark.getInfo().getVisitInfo().getExposureTime(), 1.0)

    def testFlat(self):
        """Test retrieval of flat"""
        flat = self.butler.get("flat", visit=self.visit, ccd=self.ccdNum)
        self.assertEqual(flat.getDimensions(), afwGeom.Extent2I(*self.ccdSize))
        self.assertEqual(flat.getDetector().getId(), self.ccdNum)

    def testLinearizer(self):
        self.assertTrue(self.butler.get('linearizer', ccdnum=1))


class TestMemory(lsst.utils.tests.MemoryTestCase):
    def setUp(self):
        HscMapper.clearCache()
        lsst.utils.tests.MemoryTestCase.setUp(self)


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
