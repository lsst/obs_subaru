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
from __future__ import absolute_import, division, print_function

import os
import numbers
import unittest

import lsst.utils.tests
import lsst.pipe.tasks.photoCal as photoCal


def setup_module(module):
    lsst.utils.tests.init()


class ColortermOverrideTestCase(unittest.TestCase):

    def setUp(self):
        """Test that colorterms specific to HSC override correctly"""
        colortermsFile = os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hsc", "colorterms.py")
        self.photoCalConf = photoCal.PhotoCalConfig()
        self.photoCalConf.colorterms.load(colortermsFile)

    def testHscColorterms(self):
        """Test that the colorterm libraries are formatted correctly"""
        refBands = ["g", "r", "i", "z", "y"]
        hscBands = ["g", "r", "i", "z", "y"]
        for band in hscBands:
            ct = self.photoCalConf.colorterms.getColorterm(band, photoCatName="hsc")  # exact match
            self.assertIn(ct.primary, refBands)
            self.assertIn(ct.secondary, refBands)
            self.assertIsInstance(ct.c0, numbers.Number)
            self.assertIsInstance(ct.c1, numbers.Number)
            self.assertIsInstance(ct.c2, numbers.Number)

    def testSdssColorterms(self):
        """Test that the colorterm libraries are formatted correctly"""
        sdssBands = ["g", "r", "i", "z", "y"]
        hscBands = ["g", "r", "i", "i2", "z", "y", "N816", "N921"]
        for band in hscBands:
            ct = self.photoCalConf.colorterms.getColorterm(band, photoCatName="sdss")  # exact match
            self.assertIn(ct.primary, sdssBands)
            self.assertIn(ct.secondary, sdssBands)
            self.assertIsInstance(ct.c0, numbers.Number)
            self.assertIsInstance(ct.c1, numbers.Number)
            self.assertIsInstance(ct.c2, numbers.Number)

    def testPs1Colorterms(self):
        """Test that the colorterm libraries are formatted correctly"""
        ps1Bands = ["g", "r", "i", "z", "y"]
        hscBands = ["g", "r", "r2", "i", "i2", "z", "y", "N816", "N921"]
        for band in hscBands:
            ct = self.photoCalConf.colorterms.getColorterm(band, photoCatName="ps1")  # exact match
            self.assertIn(ct.primary, ps1Bands)
            self.assertIn(ct.secondary, ps1Bands)
            self.assertIsInstance(ct.c0, numbers.Number)
            self.assertIsInstance(ct.c1, numbers.Number)
            self.assertIsInstance(ct.c2, numbers.Number)


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
