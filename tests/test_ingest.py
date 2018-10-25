# This file is part of obs_subaru.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (http://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
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
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Unit tests for Gen3 HSC raw data ingest.
"""

import unittest
import tempfile
import os
import shutil
import lsst.utils.tests

from lsst.afw.image import LOCAL
from lsst.afw.geom import Box2I, Point2I
from lsst.daf.butler import Butler
from lsst.obs.hsc import HscMapper
from lsst.obs.subaru.gen3.hsc import HyperSuprimeCamRawIngestTask, HyperSuprimeCam

testDataPackage = "testdata_subaru"
try:
    testDataDirectory = lsst.utils.getPackageDir(testDataPackage)
except lsst.pex.exceptions.NotFoundError:
    testDataDirectory = None


TESTDIR = os.path.dirname(__file__)


@unittest.skipIf(testDataDirectory is None, "testdata_subaru must be set up")
class HscIngestTestCase(lsst.utils.tests.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.mapper = HscMapper(root=TESTDIR)  # root doesn't actually matter here

    def setUp(self):
        # Use a temporary working directory
        self.root = tempfile.mkdtemp(dir=TESTDIR)
        Butler.makeRepo(self.root)
        self.butler = Butler(self.root, run="raw")
        # Register the instrument and its static metadata
        # Use of Gen2 HscMapper is a temporary workaround until we
        # have a Gen3-only way to make a cameraGeom.Camera.
        HyperSuprimeCam(self.mapper).register(self.butler.registry)
        # Make a default config for test methods to play with
        self.config = HyperSuprimeCamRawIngestTask.ConfigClass()
        self.config.onError = "break"
        self.file = os.path.join(testDataDirectory, "hsc", "raw", "HSCA90402512.fits.gz")
        self.dataId = dict(instrument="HSC", exposure=904024, detector=50)

    def tearDown(self):
        if os.path.exists(self.root):
            shutil.rmtree(self.root, ignore_errors=True)

    def runIngest(self, files=None):
        if files is None:
            files = [self.file]
        task = HyperSuprimeCamRawIngestTask(config=self.config, butler=self.butler)
        task.log.setLevel(task.log.FATAL)  # silence logs, since we expect a lot of warnings
        task.run(files)

    def runIngestTest(self, files=None):
        self.runIngest(files)
        parameters = dict(origin=LOCAL, bbox=Box2I(Point2I(0, 0), Point2I(2, 2)))
        exposure = self.butler.get("raw", self.dataId, parameters=parameters)
        metadata = self.butler.get("raw.metadata", self.dataId)
        image = self.butler.get("raw.image", self.dataId, parameters=parameters)
        self.assertImagesEqual(exposure.image, image)
        self.assertEqual(metadata.toDict(), exposure.getMetadata().toDict())

    def testSymLink(self):
        self.config.transfer = "symlink"
        self.runIngestTest()

    def testCopy(self):
        self.config.transfer = "copy"
        self.runIngestTest()

    def testHardLink(self):
        self.config.transfer = "hardlink"
        self.runIngestTest()

    def testInPlace(self):
        # hardlink into repo root manually
        newPath = os.path.join(self.root, os.path.basename(self.file))
        os.link(self.file, newPath)
        self.config.transfer = None
        self.runIngestTest([newPath])

    def testOnConflictFail(self):
        self.config.transfer = "symlink"
        self.config.conflict = "fail"
        self.runIngest()   # this one shd
        with self.assertRaises(Exception):
            self.runIngest()  # this ont

    def testOnConflictIgnore(self):
        self.config.transfer = "symlink"
        self.config.conflict = "ignore"
        self.runIngest()   # this one should succeed
        n1, = self.butler.registry.query("SELECT COUNT(*) FROM Dataset")
        self.runIngest()   # this ong should silently fail
        n2, = self.butler.registry.query("SELECT COUNT(*) FROM Dataset")
        self.assertEqual(n1, n2)

    def testOnConflictStash(self):
        self.config.transfer = "symlink"
        self.config.conflict = "ignore"
        self.config.stash = "stash"
        self.runIngest()   # this one should write to 'rawn
        self.runIngest()   # this one should write to 'stashn
        dt = self.butler.registry.getDatasetType("raw.metadata")
        ref1 = self.butler.registry.find(self.butler.collection, dt, self.dataId)
        ref2 = self.butler.registry.find("stash", dt, self.dataId)
        self.assertNotEqual(ref1.id, ref2.id)
        self.assertEqual(self.butler.get(ref1).toDict(), self.butler.getDirect(ref2).toDict())

    def testOnErrorBreak(self):
        self.config.transfer = "symlink"
        self.config.onError = "break"
        # Failing to ingest this nonexistent file after ingesting the valid one should
        # leave the valid one in the registry, despite raising an exception.
        with self.assertRaises(Exception):
            self.runIngest(files=[self.file, "nonexistent.fits"])
        dt = self.butler.registry.getDatasetType("raw.metadata")
        self.assertIsNotNone(self.butler.registry.find(self.butler.collection, dt, self.dataId))

    def testOnErrorContinue(self):
        self.config.transfer = "symlink"
        self.config.onError = "continue"
        # Failing to ingest nonexistent files before and after ingesting the
        # valid one should leave the valid one in the registry and not raise
        # an exception.
        self.runIngest(files=["nonexistent.fits", self.file, "still-not-here.fits"])
        dt = self.butler.registry.getDatasetType("raw.metadata")
        self.assertIsNotNone(self.butler.registry.find(self.butler.collection, dt, self.dataId))

    def testOnErrorRollback(self):
        self.config.transfer = "symlink"
        self.config.onError = "rollback"
        # Failing to ingest nonexistent files after ingesting the
        # valid one should leave the registry empty.
        with self.assertRaises(Exception):
            self.runIngest(file=[self.file, "nonexistent.fits"])
        try:
            dt = self.butler.registry.getDatasetType("raw.metadata")
        except KeyError:
            # If we also rollback registering the DatasetType, that's fine,
            # but not required.
            pass
        else:
            self.assertIsNotNone(self.butler.registry.find(self.butler.collection, dt, self.dataId))


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
