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

"""Unit tests for HSC gen2 to gen3 conversion.
"""

import glob
import os
import subprocess
import tempfile
import unittest

import lsst.utils.tests
from lsst.obs.base.gen2to3 import convertTests
import lsst.obs.subaru
import lsst.obs.hsc


testDataPackage = "testdata_subaru"
try:
    testDataDirectory = lsst.utils.getPackageDir(testDataPackage)
except LookupError:
    testDataDirectory = None


def createGen2Repo(inputPath):
    """
    Construct a gen2 repository for `HscMapper` containing a given set
    of input data and return its path.

    Parameters
    ----------
    inputPath : `str`
        Location of data files to ingest.

    Returns
    -------
    path : `str`
        Path to newly created gen2 repo, with ingested files.
    """
    repoPath = tempfile.mkdtemp()
    with open(os.path.join(repoPath, "_mapper"), "w") as _mapper:
        print("lsst.obs.hsc.HscMapper", file=_mapper)
    ingest_cmd = "hscIngestImages.py"
    files = glob.glob(os.path.join(inputPath, "*.fits.gz"))
    cmd = [ingest_cmd, repoPath, *files]
    print(f"Running command: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)
    return repoPath


@unittest.skipIf(testDataDirectory is None, f"{testDataPackage} not setup")
class ConvertGen2To3TestCase(convertTests.ConvertGen2To3TestCase,
                             lsst.utils.tests.TestCase):

    instrumentClassName = "lsst.obs.subaru.HyperSuprimeCam"

    def setUp(self):
        rawpath = os.path.join(testDataDirectory, 'hsc/raw')
        self.gen2calib = os.path.join(testDataDirectory, 'hsc/calib')
        self.gen2root = createGen2Repo(rawpath)
        self.config = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                                   "config", "convert2to3Config.py")

        self.biases = [{"detector": 50, "instrument": "HSC"}]
        self.flats = [{"detector": 50, "instrument": "HSC", "physical_filter": "HSC-I"}]
        self.darks = [{"detector": 50, "instrument": "HSC"}]
        super().setUp()


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
