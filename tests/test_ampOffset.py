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
import numpy as np

import lsst.utils.tests
import lsst.afw.image as afwImage
from lsst.obs.subaru import HyperSuprimeCam
from lsst.obs.subaru.ampOffset import SubaruAmpOffsetConfig, SubaruAmpOffsetTask


class AmpOffsetTest(lsst.utils.tests.TestCase):

    def setUp(self):
        instrument = HyperSuprimeCam()
        self.camera = instrument.getCamera()

    def tearDown(self):
        del self.camera

    def buildExposure(self, addBackground=False):
        exp = afwImage.ExposureF(self.camera[0].getBBox())
        exp.setDetector(self.camera[0])
        amps = exp.getDetector().getAmplifiers()
        exp.image[amps[0].getBBox()] = -2.5
        exp.image[amps[1].getBBox()] = -1.0
        exp.image[amps[2].getBBox()] = 1.0
        exp.image[amps[3].getBBox()] = 2.5
        if addBackground:
            exp.image.array += 100
        return exp

    def testAmpOffset(self):
        exp = self.buildExposure(addBackground=False)
        config = SubaruAmpOffsetConfig()
        config.doBackground = False
        config.doDetection = False
        task = SubaruAmpOffsetTask(config=config)
        task.run(exp)
        meta = exp.getMetadata().toDict()
        self.assertEqual(np.sum(exp.image.array), 0)
        self.assertAlmostEqual(meta['PEDESTAL1'], -2.5, 7)
        self.assertAlmostEqual(meta['PEDESTAL2'], -1.0, 7)
        self.assertAlmostEqual(meta['PEDESTAL3'], 1.0, 7)
        self.assertAlmostEqual(meta['PEDESTAL4'], 2.5, 7)

    def testAmpOffsetWithBackground(self):
        exp = self.buildExposure(addBackground=True)
        config = SubaruAmpOffsetConfig()
        config.doBackground = True
        config.doDetection = True
        task = SubaruAmpOffsetTask(config=config)
        task.run(exp)
        meta = exp.getMetadata().toDict()
        self.assertAlmostEqual(meta['PEDESTAL1'], -meta['PEDESTAL4'], 7)
        self.assertAlmostEqual(meta['PEDESTAL2'], -meta['PEDESTAL3'], 7)


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
