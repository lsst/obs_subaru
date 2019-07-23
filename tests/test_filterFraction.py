#
# LSST Data Management System
#
# Copyright 2017 AURA/LSST.
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
from lsst.afw.image import ExposureF, CoaddInputs
from lsst.afw.table import ExposureCatalog, SourceCatalog, Point2DKey
from lsst.afw.geom import makeCdMatrix, makeSkyWcs
from lsst.geom import degrees, Point2D, Box2I, Point2I, SpherePoint
from lsst.daf.base import PropertyList
from lsst.meas.base import FatalAlgorithmError
from lsst.obs.subaru.filterFraction import FilterFractionPlugin


class FilterFractionTest(lsst.utils.tests.TestCase):

    def setUp(self):
        # Set up a Coadd with CoaddInputs tables that have blank filter columns to be filled
        # in by later test code.
        self.coadd = ExposureF(30, 90)
        # WCS is arbitrary, since it'll be the same for all images
        wcs = makeSkyWcs(crpix=Point2D(0, 0), crval=SpherePoint(45.0, 45.0, degrees),
                         cdMatrix=makeCdMatrix(scale=0.17*degrees))
        self.coadd.setWcs(wcs)
        schema = ExposureCatalog.Table.makeMinimalSchema()
        self.filterKey = schema.addField("filter", type=str, doc="", size=16)
        weightKey = schema.addField("weight", type=float, doc="")
        # First input image covers the first 2/3, second covers the last 2/3, so they
        # overlap in the middle 1/3.
        inputs = ExposureCatalog(schema)
        self.input1 = inputs.addNew()
        self.input1.setId(1)
        self.input1.setBBox(Box2I(Point2I(0, 0), Point2I(29, 59)))
        self.input1.setWcs(wcs)
        self.input1.set(weightKey, 2.0)
        self.input2 = inputs.addNew()
        self.input2.setId(2)
        self.input2.setBBox(Box2I(Point2I(0, 30), Point2I(29, 89)))
        self.input2.setWcs(wcs)
        self.input2.set(weightKey, 3.0)
        # Use the same catalog for visits and CCDs since the algorithm we're testing only cares
        # about CCDs.
        self.coadd.getInfo().setCoaddInputs(CoaddInputs(inputs, inputs))

        # Set up a catalog with centroids and a FilterFraction plugin.
        # We have one record in each region (first input only, both inputs, second input only)
        schema = SourceCatalog.Table.makeMinimalSchema()
        centroidKey = Point2DKey.addFields(schema, "centroid", doc="position", unit="pixel")
        schema.getAliasMap().set("slot_Centroid", "centroid")
        self.plugin = FilterFractionPlugin(config=FilterFractionPlugin.ConfigClass(),
                                           schema=schema, name="subaru_FilterFraction",
                                           metadata=PropertyList())
        catalog = SourceCatalog(schema)
        self.record1 = catalog.addNew()
        self.record1.set(centroidKey, Point2D(14.0, 14.0))
        self.record12 = catalog.addNew()
        self.record12.set(centroidKey, Point2D(14.0, 44.0))
        self.record2 = catalog.addNew()
        self.record2.set(centroidKey, Point2D(14.0, 74.0))

    def tearDown(self):
        del self.coadd
        del self.input1
        del self.input2
        del self.record1
        del self.record2
        del self.record12
        del self.plugin

    def testSingleFilter(self):
        """Test that we get FilterFraction=1 for filters with only one version."""
        self.input1.set(self.filterKey, "g")
        self.input2.set(self.filterKey, "g")
        self.plugin.measure(self.record1, self.coadd)
        self.plugin.measure(self.record12, self.coadd)
        self.plugin.measure(self.record2, self.coadd)
        self.assertEqual(self.record1.get("subaru_FilterFraction_unweighted"), 1.0)
        self.assertEqual(self.record12.get("subaru_FilterFraction_unweighted"), 1.0)
        self.assertEqual(self.record2.get("subaru_FilterFraction_unweighted"), 1.0)
        self.assertEqual(self.record1.get("subaru_FilterFraction_weighted"), 1.0)
        self.assertEqual(self.record12.get("subaru_FilterFraction_weighted"), 1.0)
        self.assertEqual(self.record2.get("subaru_FilterFraction_weighted"), 1.0)

    def testTwoFiltersI(self):
        """Test that we get the right answers for a mix of i and i2."""
        self.input1.set(self.filterKey, "i")
        self.input2.set(self.filterKey, "i2")
        self.plugin.measure(self.record1, self.coadd)
        self.plugin.measure(self.record12, self.coadd)
        self.plugin.measure(self.record2, self.coadd)
        self.assertEqual(self.record1.get("subaru_FilterFraction_unweighted"), 0.0)
        self.assertEqual(self.record12.get("subaru_FilterFraction_unweighted"), 0.5)
        self.assertEqual(self.record2.get("subaru_FilterFraction_unweighted"), 1.0)
        self.assertEqual(self.record1.get("subaru_FilterFraction_weighted"), 0.0)
        self.assertEqual(self.record12.get("subaru_FilterFraction_weighted"), 0.6)
        self.assertEqual(self.record2.get("subaru_FilterFraction_weighted"), 1.0)

    def testTwoFiltersR(self):
        """Test that we get the right answers for a mix of r and r2."""
        self.input1.set(self.filterKey, "r")
        self.input2.set(self.filterKey, "r2")
        self.plugin.measure(self.record1, self.coadd)
        self.plugin.measure(self.record12, self.coadd)
        self.plugin.measure(self.record2, self.coadd)
        self.assertEqual(self.record1.get("subaru_FilterFraction_unweighted"), 0.0)
        self.assertEqual(self.record12.get("subaru_FilterFraction_unweighted"), 0.5)
        self.assertEqual(self.record2.get("subaru_FilterFraction_unweighted"), 1.0)
        self.assertEqual(self.record1.get("subaru_FilterFraction_weighted"), 0.0)
        self.assertEqual(self.record12.get("subaru_FilterFraction_weighted"), 0.6)
        self.assertEqual(self.record2.get("subaru_FilterFraction_weighted"), 1.0)

    def testInvalidCombination(self):
        """Test that we get a fatal exception for weird combinations of filters."""
        self.input1.set(self.filterKey, "i")
        self.input2.set(self.filterKey, "r")
        with self.assertRaises(FatalAlgorithmError):
            self.plugin.measure(self.record12, self.coadd)


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
