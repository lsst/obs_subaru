# This file is part of daf_butler.
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

"""Tests of the HyperSuprimeCam instrument class.
"""

import unittest

import lsst.utils.tests
import lsst.obs.subaru.gen3.hsc
from lsst.obs.base.instrument_tests import InstrumentTests, InstrumentTestData


class TestHyperSuprimeCam(InstrumentTests, lsst.utils.tests.TestCase):
    def setUp(self):
        physical_filters = {"HSC-G",
                            "HSC-R",
                            "HSC-R2",
                            "HSC-I",
                            "HSC-I2",
                            "HSC-Z",
                            "HSC-Y",
                            "ENG-R1",
                            "NB0387",
                            "NB0400",
                            "NB0468",
                            "NB0515",
                            "NB0527",
                            "NB0656",
                            "NB0718",
                            "NB0816",
                            "NB0921",
                            "NB0926",
                            "IB0945",
                            "NB0973",
                            "NB1010",
                            "SH",
                            "PH",
                            "NONE"}
        self.data = InstrumentTestData(name="HSC",
                                       nDetectors=112,
                                       firstDetectorName="1_53",
                                       physical_filters=physical_filters)

        self.instrument = lsst.obs.subaru.gen3.hsc.HyperSuprimeCam()


class MemoryTester(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == '__main__':
    lsst.utils.tests.init()
    unittest.main()
