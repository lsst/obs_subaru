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
import os.path
import numbers
import unittest

import lsst.utils.tests
import lsst.pipe.tasks.photoCal as photoCal


def setup_module(module):
    lsst.utils.tests.init()


class ColortermOverrideTestCase(unittest.TestCase):
    """Test that the HSC colorterms have been set consistently."""
    def setUp(self):
        """Test that colorterms specific to HSC override correctly"""
        colortermsFile = os.path.join(os.path.dirname(__file__), "../config", "colorterms.py")
        self.photoCalConf = photoCal.PhotoCalConfig()
        self.photoCalConf.colorterms.load(colortermsFile)

    def testHscColorterms(self):
        """Test that the colorterm libraries are formatted correctly"""
        hscReferenceFilters = ["g", "r", "i", "z", "y"]
        hscPhysicalFilters = ["HSC-G", "HSC-R", "HSC-I", "HSC-Z", "HSC-Y"]
        for filter in hscPhysicalFilters:
            ct = self.photoCalConf.colorterms.getColorterm(filter, photoCatName="hsc")  # exact match
            self.assertIn(ct.primary, hscReferenceFilters)
            self.assertIn(ct.secondary, hscReferenceFilters)
            self.assertIsInstance(ct.c0, numbers.Number)
            self.assertIsInstance(ct.c1, numbers.Number)
            self.assertIsInstance(ct.c2, numbers.Number)

    def testSdssColorterms(self):
        """Test that the colorterm libraries are formatted correctly"""
        sdssReferenceFilters = ["g", "r", "i", "z", "y"]
        hscPhysicalFilters = ["HSC-G", "HSC-R", "HSC-I", "HSC-I2", "HSC-Z", "HSC-Y", "NB0816", "NB0921"]
        for filter in hscPhysicalFilters:
            ct = self.photoCalConf.colorterms.getColorterm(filter, photoCatName="sdss")  # exact match
            self.assertIn(ct.primary, sdssReferenceFilters)
            self.assertIn(ct.secondary, sdssReferenceFilters)
            self.assertIsInstance(ct.c0, numbers.Number)
            self.assertIsInstance(ct.c1, numbers.Number)
            self.assertIsInstance(ct.c2, numbers.Number)

    def testPs1Colorterms(self):
        """Test that the colorterm libraries are formatted correctly"""
        ps1ReferenceFilters = ["g", "r", "i", "z", "y"]
        hscPhysicalFilters = ["HSC-G", "HSC-R", "HSC-R2", "HSC-I", "HSC-I2", "HSC-Z", "HSC-Y",
                              "IB0945", "NB0387", "NB0468", "NB0515", "NB0527", "NB0656",
                              "NB0718", "NB0816", "NB0921", "NB0973", "NB1010"]

        for filter in hscPhysicalFilters:
            ct = self.photoCalConf.colorterms.getColorterm(filter, photoCatName="ps1")  # exact match
            self.assertIn(ct.primary, ps1ReferenceFilters)
            self.assertIn(ct.secondary, ps1ReferenceFilters)
            self.assertIsInstance(ct.c0, numbers.Number)
            self.assertIsInstance(ct.c1, numbers.Number)
            self.assertIsInstance(ct.c2, numbers.Number)


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
