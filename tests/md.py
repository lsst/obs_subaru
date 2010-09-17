#!/usr/bin/env python
import os
import unittest
import lsst.utils.tests as utilsTests

import lsst.daf.persistence as dafPersist
from hsc.obs.suprime import SuprimeMapper

class MetadataTestCase(unittest.TestCase):
    """Testing butler metadata retrieval"""

    def setUp(self):
        self.bf = dafPersist.ButlerFactory(mapper=SuprimeMapper(
            root=os.path.join(os.getenv('SUPRIME_DATA_DIR'), "SUPA"),
	    calibRoot=os.path.join(os.getenv('SUPRIME_DATA_DIR'), "CALIB") ))
        self.butler = self.bf.create()

    def tearDown(self):
        del self.butler
        del self.bf

    # commented out until I can get this sorted
    def xtestTiles(self):
        """Test sky tiles"""
        tiles = self.butler.queryMetadata("raw", "skyTile", ("skyTile",))
        tiles = [x[0] for x in tiles]
        tiles.sort()
        self.assertEqual(tiles, [
            99757, 99758, 99759, 100477, 100478, 100479, 181384, 181385,
            181386, 181836, 181837, 181838, 181839, 182281, 182282, 182283,
            182284, 182719, 182720
            ])

    # Commented out until I can get this sorted.
    def xtestCcdsInTiles(self):
        """Test CCDs in sky tiles"""
        ccds = self.butler.queryMetadata("raw", "ccd",
                ("visit", "ccd"), skyTile=182719)
        ccds.sort()
        self.assertEqual(ccds, [(788965, 1), (792170, 1), (792933, 1)])

        ccds = self.butler.queryMetadata("raw", "ccd",
                ("visit", "ccd"), dataId={'skyTile': 181386})
        ccds.sort()
        self.assertEqual(ccds, [
            (788965, 29), (788965, 30), (788965, 31),
            (792170, 29), (792170, 30), (792170, 31),
            (792933, 29), (792933, 30), (792933, 31)
            ])

    def testVisits(self):
        """Test visits"""
        visits = self.butler.queryMetadata("raw", "visit", ("visit",), {})
        #visits = [x[0] for x in visits] # not sure what this is about
        visits.sort()

        self.assertEqual(visits, [
	    100513, 100514, 100515, 100516, 100517, 100518,
	    100519, 100520, 100521, 100522, 100523, 100524,
	    100525, 100526, 100527, 100528, 100529, 100530,
	    101410, 101411, 101412, 101413, 101414, 101415,
	    101416, 101417, 101418, 101419, 101421, 101422,
	    101423, 101424, 101425, 101426, 101427, 101428,
	    101429, 101438, 101439, 101440, 101441, 101541,
	    101542, 101543, 101544, 101545, 101546, 101547,
	    101548
	    ])

    def testFilter(self):
        """Test filters"""
        filter = self.butler.queryMetadata("raw", "visit", ("filter",),
                visit=100525)
        self.assertEqual(filter, ['W-S-Z+'])


#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(MetadataTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit = False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
