#!/usr/bin/env python
#
# LSST Data Management System
# Copyright 2008-2013 LSST Corporation.
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
# see <http://www.lsstcorp.org/LegalNotices/>.
#

import lsst.pex.config as pexConfig
import lsst.pex.exceptions as pexExceptions
import lsst.pipe.base as pipeBase
import lsst.daf.base as dafBase
import lsst.afw.geom as afwGeom
import lsst.afw.math as afwMath
import lsst.afw.table as afwTable
from lsst.meas.algorithms import SourceMeasurementTask
from lsst.meas.deblender import SourceDeblendTask
from lsst.pipe.tasks.calibrate import CalibrateTask


class DeblendAndMeasureConfig(pexConfig.Config):
    doDeblend = pexConfig.Field(dtype=bool, default=True, doc = "Deblend sources?")
    doMeasurement = pexConfig.Field(dtype=bool, default=True, doc = "Measure sources?")
    doWriteSources = pexConfig.Field(dtype=bool, default=True, doc = "Write sources?")
    doWriteHeavyFootprintsInSources = pexConfig.Field(dtype=bool, default=False,
                                                      doc = "Include HeavyFootprint data in source table?")

    sourceOutputFile = pexConfig.Field(dtype=str, default=None, doc="Write sources to given filename (default: use butler)", optional=True)

    deblend = pexConfig.ConfigurableField(
        target = SourceDeblendTask,
        doc = "Split blended sources into their components",
    )
    measurement = pexConfig.ConfigurableField(
        target = SourceMeasurementTask,
        doc = "Final source measurement on low-threshold detections",
    )
    

class DeblendAndMeasureTask(pipeBase.CmdLineTask):
    ConfigClass = DeblendAndMeasureConfig
    _DefaultName = "deblendAndMeasure"

    def writeConfig(self, *args, **kwargs):
        pass

    def __init__(self, **kwargs):
        pipeBase.CmdLineTask.__init__(self, **kwargs)

    @pipeBase.timeMethod
    def run(self, dataRef):
        self.log.info("Processing %s" % (dataRef.dataId))
        calexp = dataRef.get('calexp')
        srcs = dataRef.get('src')
        print 'Calexp:', calexp
        print 'srcs:', srcs

        ## FIXME -- this whole mapping business is very fragile -- it
        ## seems to fail, eg, if you don't set "-c
        ## doMeasurement=False" when creating the input 'srcs' list.

        mapper = afwTable.SchemaMapper(srcs.getSchema())
        # map all the existing fields
        mapper.addMinimalSchema(srcs.getSchema(), True)
        schema = mapper.getOutputSchema()
        self.algMetadata = dafBase.PropertyList()
        if self.config.doDeblend:
            self.makeSubtask("deblend", schema=schema)
        if self.config.doMeasurement:
            self.makeSubtask("measurement", schema=schema, algMetadata=self.algMetadata)
        self.schema = schema

        parents = []
        for src in srcs:
            if src.getParent() == 0:
                parents.append(src)

        outsources = afwTable.SourceCatalog(schema)
        outsources.reserve(len(parents))
        outsources.extend(parents, mapper=mapper)
        srcs = outsources
        print len(srcs), 'sources before deblending'

        if self.config.doDeblend:
            self.deblend.run(calexp, srcs, calexp.getPsf())
        
        if self.config.doMeasurement:
            self.measurement.run(calexp, srcs)

        if srcs is not None and self.config.doWriteSources:
            sourceWriteFlags = (0 if self.config.doWriteHeavyFootprintsInSources
                                else afwTable.SOURCE_IO_NO_HEAVY_FOOTPRINTS)
            print 'Writing "src" outputs'
            if self.config.sourceOutputFile:
                srcs.writeFits(self.config.sourceOutputFile, flags=sourceWriteFlags)
            else:
                dataRef.put(srcs, 'src', flags=sourceWriteFlags)

if __name__ == '__main__':
    DeblendAndMeasureTask.parseAndRun()
