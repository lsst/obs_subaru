import lsst.obs.lsstSim as obsLsst
from driver import *


if __name__ == '__main__':
    mapperArgs = dict(root='/lsst/home/dstn/lsst/lsst_dm_stack_demo-Winter2012/input')
    mapper = obsLsst.LsstSimMapper(**mapperArgs)
    butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    butler = butlerFactory.create()
    print 'Butler', butler
    # input/raw/v886894611-fr/E001/R23/S11/imsim_886894611_R23_S11_C02_E001.fits.gz
    dataRef = butler.subset('raw', dataId = dict(visit=886894611, raft='2,3', sensor='1,1'))
    print 'dataRef:', dataRef
    #dataRef.butlerSubset = dataRef
    #print 'dataRef:', dataRef
    print 'len(dataRef):', len(dataRef)
    for dr in dataRef:
        print '  ', dr

    conf = procCcd.ProcessCcdConfig()
    #conf.doIsr = False
    #conf.doCalibrate = False
    conf.doIsr = True
    conf.doCalibrate = True

    conf.doDetection = True
    conf.doMeasurement = True
    conf.doWriteSources = True
    conf.doWriteCalibrate = False
    

    conf.validate()

    #print 'Conf', conf
    print 'Conf.doIsr:', conf.doIsr

    proc = ProcessCcdTask(config=conf)
    for dr in dataRef:
        proc.run(dr)
    

