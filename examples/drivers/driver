import lsst.obs.lsstSim as obsLsst

from driver import *

import lsst.pipe.tasks.processCcdLsstSim as procCcdLsst
from lsst.ip.isr import IsrTask
from lsst.pipe.tasks.calibrate import CalibrateTask
from lsst.pipe.tasks.snapCombine import SnapCombineTask

class ProcessCcdTaskLsst(procCcdLsst.ProcessCcdLsstSimTask):
    _DefaultName = 'hackProcessCcdTaskLsst'
    def __init__(self, **kwargs):
        pipeBase.Task.__init__(self, **kwargs)
        self.makeSubtask("isr", IsrTask)
        self.makeSubtask("ccdIsr", IsrTask)
        self.makeSubtask("snapCombine", SnapCombineTask)
        self.makeSubtask("calibrate", CalibrateTask)
        self.schema = afwTable.SourceTable.makeMinimalSchema()
        self.algMetadata = dafBase.PropertyList()
        if self.config.doDetection:
            self.makeSubtask("detection", measAlg.SourceDetectionTask, schema=self.schema)
        if self.config.doMeasurement:
            self.makeSubtask("measurement", SingleSourceMeasurementTask,
                             schema=self.schema, algMetadata=self.algMetadata)



if __name__ == '__main__':
    mapperArgs = dict(root='/lsst/home/dstn/lsst/lsst_dm_stack_demo-Winter2012/input',
                      #calibRoot='/lsst/home/dstn/lsst/lsst_dm_stack_demo-Winter2012/input',
                      #calibRegistry='/lsst/home/dstn/lsst/lsst_dm_stack_demo-Winter2012/input/registry.sqlite3',
                      )
    mapper = obsLsst.LsstSimMapper(**mapperArgs)
    butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    butler = butlerFactory.create()
    print 'Butler', butler

    dataId = dict(visit=886894611, raft='2,3', sensor='1,1')

    #fid = dataId.copy()
    #fid.update(filter='r')
    #flat = butler.get('flat', dataId)
    #print 'Flat:', flat

    # input/raw/v886894611-fr/E001/R23/S11/imsim_886894611_R23_S11_C02_E001.fits.gz
    dataRef = butler.subset('raw', dataId = dataId)
    print 'dataRef:', dataRef
    print 'len(dataRef):', len(dataRef)
    for dr in dataRef:
        print '  ', dr

    conf = procCcdLsst.ProcessCcdLsstSimConfig()
    #conf.doIsr = False
    #conf.doCalibrate = False
    conf.doIsr = True
    conf.doCalibrate = True

    conf.doDetection = True
    conf.doMeasurement = True
    conf.doWriteSources = True
    conf.doWriteCalibrate = False

    conf.validate()

    proc = ProcessCcdTaskLsst(config=conf)
    for dr in dataRef:
        proc.run(dr)
    

