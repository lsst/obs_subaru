import lsst.pipe.base as pipeBase
import lsst.meas.algorithms as measAlg

class SingleSourceMeasurementTask(measAlg.SourceMeasurementTask):
    '''
    A SourceMeasurementTask subclass that replaces all detected Footprints
    with noise before running the measurement algorithms.
    '''
    _DefaultName = "singleSourceMeasurement"

    @pipeBase.timeMethod
    def measure(self, exposure, sources):
        """Measure sources on an exposure, with no aperture correction.

        @param[in]     exposure Exposure to process
        @param[in,out] sources  SourceCatalog containing sources detected on this exposure.
        @return None
        """

        self.log.log(self.log.INFO, "Measuring %d sources" % len(sources))
        self.config.slots.setupTable(sources.table, prefix=self.config.prefix)
        for source in sources:
            print 'source', source
            fp = source.getFootprint()
            print '  footprint', fp
            myid = source.getId()
            print '  id', myid
            parent = source.getParent()
            print '  parent', parent
        for source in sources:
            self.measurer.apply(source, exposure)
    

