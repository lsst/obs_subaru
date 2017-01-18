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
import os
import numpy as np
import unittest

import lsst.utils.tests
import lsst.afw.image as afwImage
import lsst.afw.table as afwTable
from lsst.meas.algorithms.detection import SourceDetectionTask
from lsst.meas.base import SingleFrameMeasurementTask
import lsst.meas.deblender as measDeb

try:
    type(display)
except NameError:
    display = False

DATA_DIR = os.path.join(os.path.dirname(os.path.realpath(__file__)), "data")


class IncludeTestCase(lsst.utils.tests.TestCase):
    """ Test case for DM-1738: test that include method successfully
    expands footprint to include the union of itself and all others provided.

    In the current version of meas_deblender, children's Footprints can extend outside
    that of the parent.  The replace-by-noise code thus fails to reinstate the pixels
    that are outside the parent but within the children.  Calling include() prior
    to noise replacement solves this issue.

    The test data used here (./data/ticket1738.fits) is a cutout of an HSC observed M31
    field in which this pathology occured:
    --id field=M31 dateObs=2014-11-24 visit=14770 ccd=27
    bbox = afwGeom.Box2I(afwGeom.Point2I(720, 460), afwGeom.Extent2I(301, 301))

    See data/ticket1738_noInclude.png or data/ticket1738_noInclude.fits for a visual
    of this pathology).

    """

    def setUp(self):
        self.calexpOrig = afwImage.ExposureF(os.path.join(DATA_DIR, "ticket1738.fits"))
        self.calexp = afwImage.ExposureF(os.path.join(DATA_DIR, "ticket1738.fits"))

    def tearDown(self):
        del self.calexpOrig
        del self.calexp

    def testInclude(self):
        schema = afwTable.SourceTable.makeMinimalSchema()

        # Create the detection task
        config = SourceDetectionTask.ConfigClass()
        config.reEstimateBackground = False  # Turn off so that background does not change from orig
        detectionTask = SourceDetectionTask(config=config, schema=schema)

        # Create the deblender Task
        debConfig = measDeb.SourceDeblendConfig()
        debTask = measDeb.SourceDeblendTask(schema, config=debConfig)

        # Create the measurement Task
        config = SingleFrameMeasurementTask.ConfigClass()
        measureTask = SingleFrameMeasurementTask(schema, config=config)

        # Create the output table
        tab = afwTable.SourceTable.make(schema)

        # Process the data
        result = detectionTask.run(tab, self.calexp)
        sources = result.sources

        # Run the deblender
        debTask.run(self.calexp, sources)

        # Run the measurement task: this where the replace-with-noise occurs
        measureTask.run(sources, self.calexp)

        plotOnFailure = False
        if display:
            plotOnFailure = True

        # The relative differences ranged from 0.02 to ~2.  This rtol is somewhat
        # random, but will certainly catch the pathology if it occurs.
        self.assertFloatsAlmostEqual(self.calexpOrig.getMaskedImage().getImage().getArray(),
                                     self.calexp.getMaskedImage().getImage().getArray(),
                                     rtol=1E-3, printFailures=False, plotOnFailure=plotOnFailure)


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()

if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
