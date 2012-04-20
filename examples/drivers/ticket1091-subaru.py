import lsst.pipe.base as pipeBase
import lsst.pipe.tasks.processCcd as procCcd
import lsst.daf.persistence as dafPersist
import lsst.obs.suprimecam as obsSc

import lsst.daf.base as dafBase
import lsst.afw.table as afwTable
from lsst.ip.isr import IsrTask
from lsst.pipe.tasks.calibrate import CalibrateTask

def printConfig(conf):
    for k,v in conf.items():
        print '  ', k, v

if __name__ == '__main__':
    mapperArgs = dict(root='/lsst/home/dstn/lsst/ACT-data/rerun/dstn',
                      calibRoot='/lsst/home/dstn/lsst/ACT-data/CALIB')
    mapper = obsSc.SuprimecamMapper(**mapperArgs)
    butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    butler = butlerFactory.create()
    print 'Butler', butler
    dataRef = butler.subset('raw', dataId = dict(visit=126969, ccd=5))
    print 'dataRef:', dataRef
    #dataRef.butlerSubset = dataRef
    #print 'dataRef:', dataRef
    print 'len(dataRef):', len(dataRef)
    for dr in dataRef:
        print '  ', dr

            
    conf = procCcd.ProcessCcdConfig()
    #conf.measurement.doRemoveOtherSources = True

    #conf.doDetection = True
    #conf.doMeasurement = True
    #conf.doWriteSources = True
    #conf.doWriteCalibrate = True

    conf.calibrate.doComputeApCorr = False
    conf.calibrate.doAstrometry = False
    conf.calibrate.doPhotoCal = False

    # the default Cosmic Ray parameters don't work for Subaru images
    cr = conf.calibrate.repair.cosmicray
    cr.minSigma = 10.
    cr.min_DN = 500.
    cr.niteration = 3
    cr.nCrPixelMax = 1000000

    proc = procCcd.ProcessCcdTask(config=conf, name='ProcessCcd')

    conf.calibrate.measurement.doApplyApCorr = False
    conf.measurement.doApplyApCorr = False
    conf.validate()

    for dr in dataRef:
        print 'dr', dr
        # Only do ISR, Calibration, and Measurement if necessary...
        doIsr   = False
        doCalib = False
        doMeas  = False
        print 'calexp', mapper.map('calexp', dr.dataId)
        print 'psf', mapper.map('psf', dr.dataId)
        print 'icSrc', mapper.map('icSrc', dr.dataId)
        try:
            psf = dr.get('psf')
            print 'PSF:', psf
        except:
            print 'No PSF'
            doCalib = True
        try:
            expo = dr.get('calexp')
            print 'Calexp:', expo
        except:
            print 'No calexp'
            doCalib = True
        try:
            srcs = dr.get('icSrc')
            print 'Srcs', srcs
        except:
            print 'No sources'
            doMeas = True

        conf.doIsr = doIsr
        conf.doCalibrate = doCalib

        conf.doDetection = doMeas
        conf.doMeasurement = doMeas
        conf.doWriteSources = doMeas
        conf.doWriteCalibrate = doMeas

        proc.run(dr)
    
        srcs = dr.get('icSrc')
        print 'Srcs', srcs

        exposure = dr.get('calexp')
        print 'Exposure', exposure
        mi = exposure.getMaskedImage()

        psf = dr.get('psf')
        print 'PSF:', psf

        from lsst.meas.deblender.baseline import deblend
        import lsst.meas.algorithms as measAlg

        for src in srcs:
            # SourceRecords
            print '  ', src
            fp = src.getFootprint()
            print '  fp', fp
            pks = fp.getPeaks()
            print '  pks:', len(pks), pks

            bb = fp.getBBox()
            xc = int((bb.getMinX() + bb.getMaxX()) / 2.)
            yc = int((bb.getMinY() + bb.getMaxY()) / 2.)


            if not hasattr(psf, 'getFwhm'):
                pa = measAlg.PsfAttributes(psf, xc, yc)
                psfw = pa.computeGaussianWidth(measAlg.PsfAttributes.ADAPTIVE_MOMENT)
                #print 'PSF width:', psfw
                psf_fwhm = 2.35 * psfw
            else:
                psf_fwhm = psf.getFwhm(xc, yc)
            print 'Deblending...'
            X = deblend([fp], [pks], mi, psf, psf_fwhm)
            print 'Got', X
