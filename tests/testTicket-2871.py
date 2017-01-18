#!/usr/bin/env python
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

import lsst.utils.tests
import lsst.afw.detection as afwDetection
import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom
import lsst.afw.table as afwTable
import lsst.meas.algorithms as algorithms


class DeblendTestCase(unittest.TestCase):
    """A test case for deblending"""

    def checkDeblender(self):
        try:
            import lsst.meas.deblender
        except ImportError as e:
            self.skipTest("Cannot import lsst.meas.deblender: %s" % e)

    def testFailures(self):
        """Test deblender failure flagging (#2871)

        We create a good source which is expected to pass and a bad source
        which is expected to fail because its footprint goes off the image.
        This latter case may not happen in practise, but it is useful for
        checking the plumbing of the deblender.
        """
        import lsst.meas.deblender as measDeb

        self.checkDeblender()
        xGood, yGood = 57, 86
        xBad, yBad = 0, 0  # Required to be in image so we can evaluate the PSF; will put neighbour just outside
        flux = 100.0
        dims = afwGeom.Extent2I(128, 128)

        mi = afwImage.MaskedImageF(dims)
        mi.getVariance().set(1.0)
        image = mi.getImage()
        image.set(0)
        image.set(xGood, yGood, flux)

        exposure = afwImage.makeExposure(mi)
        psf = algorithms.DoubleGaussianPsf(21, 21, 3.)
        exposure.setPsf(psf)

        schema = afwTable.SourceTable.makeMinimalSchema()

        config = measDeb.SourceDeblendConfig()
        config.catchFailures = True
        task = measDeb.SourceDeblendTask(schema, config=config)

        catalog = afwTable.SourceCatalog(schema)

        def makeSource(x, y, offset=-2, size=3.0):
            """Make a source in the catalog

            Two peaks are created: one at the specified
            position, and one offset in x,y.
            The footprint is of the nominated size.
            """
            src = catalog.addNew()
            foot = afwDetection.Footprint(afwGeom.Point2I(x, y), size)
            foot.addPeak(x, y, flux)
            foot.addPeak(x + offset, y + offset, flux)
            src.setFootprint(foot)
            return src

        good = makeSource(xGood, yGood)
        bad = makeSource(xBad, yBad)

        task.run(exposure, catalog)

        self.assertFalse(good.get('deblend_failed'))
        self.assertTrue(bad.get('deblend_failed'))


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()

if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
