import matplotlib
matplotlib.use('Agg')
from matplotlib.patches import Ellipse
import pylab as plt
import numpy as np
import os
import math

import lsst.daf.persistence as dafPersist
import lsst.pex.logging as pexLog
import lsst.afw.table as afwTable
import lsst.afw.math  as afwMath
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.meas.algorithms as measAlg
import lsst.pipe.tasks.processCcd as procCcd
import lsst.obs.suprimecam as obsSc

from utils import *

class DebugDeblendTask(measAlg.SourceDeblendTask):
    def __init__(self, *args, **kwargs):
        self.prefix = kwargs.pop('prefix', '')
        self.drill = kwargs.pop('drill', [])
        if not len(self.drill):
            self.drill = None
        #self.plotmasks = kwargs.pop('plotmasks', True)
        #self.plotregion = None
        self.plotnum = 0
        super(DebugDeblendTask, self).__init__(*args, **kwargs)

    def run(self, exposure, sources, psf):
        if self.drill:
            keep = afwTable.SourceCatalog(sources.getTable())
            for src in sources:
                if src.getId() in self.drill:
                    keep.append(src)
            keep.sort()
            sources = keep
        self.deblend(exposure, sources, psf)

    def savefig(self, fn):
        plotfn = '%s%s.png' % (self.prefix, fn)
        plt.savefig(plotfn)
        print 'wrote', plotfn

    def preSingleDeblendHook(self, exposure, srcs, i, fp, psf, psf_fwhm, sigma1):
        mi = exposure.getMaskedImage()
        parent = srcs[i]
        pid = parent.getId()
        plt.clf()
        fp = parent.getFootprint()
        pext = getExtent(fp.getBBox())
        imargs = dict(interpolation='nearest', origin='lower')
        pim = footprintToImage(parent.getFootprint(), mi).getArray()
        plt.imshow(pim, extent=pext, **imargs)
        plt.gray()
        plt.title('parent %i' % pid)
        pks = fp.getPeaks()
        plt.plot([pk.getIx() for pk in pks], [pk.getIy() for pk in pks], 'rx')
        plt.axis(pext)
        self.savefig('%04i-pim' % self.plotnum)

        plt.clf()
        mask = mi.getMask()
        mim = mask.getArray()
        bitmask = mask.getPlaneBitMask('DETECTED')
        plt.imshow((mim & bitmask) > 0, **imargs)
        plt.gray()
        plt.title('parent %i DET' % pid)
        pks = fp.getPeaks()
        plt.plot([pk.getIx() for pk in pks], [pk.getIy() for pk in pks], 'rx')
        plt.axis(pext)
        self.savefig('%04i-pmask' % self.plotnum)

    def postSingleDeblendHook(self, exposure, srcs, i, npre, kids, fp, psf, psf_fwhm, sigma1, res):
        mi = exposure.getMaskedImage()
        parent = srcs[i]
        plotDeblendFamily(mi, parent, kids, srcs, sigma1, ellipses=False)
        self.savefig('%04i-posta' % self.plotnum)
        plotDeblendFamily(mi, parent, kids, srcs, sigma1, plotb=True, ellipses=False)
        self.savefig('%04i-postb' % self.plotnum)
        self.plotnum += 1

def runDeblend(dataRef, sourcefn, forceisr=False, forcecalib=False, forcedet=False,
               verbose=False, drill=[],
               debugDeblend=True, debugMeas=True):
    dr = dataRef
    mapper = datarefToMapper(dr)

    conf = procCcd.ProcessCcdConfig()

    conf.calibrate.doComputeApCorr = False
    conf.calibrate.doAstrometry = False
    conf.calibrate.doPhotoCal = False

    # the default Cosmic Ray parameters don't work for Subaru images
    cr = conf.calibrate.repair.cosmicray
    cr.minSigma = 10.
    cr.min_DN = 500.
    cr.niteration = 3
    cr.nCrPixelMax = 1000000

    conf.calibrate.measurement.doApplyApCorr = False
    conf.measurement.doApplyApCorr = False
    conf.validate()

    # Only do ISR, Calibration, and Detection if necessary (or forced)
    doIsr    = False
    doCalib  = False
    doDetect = False
    print 'Looking for calexp', mapper.map('calexp', dr.dataId)
    print 'Looking for psf', mapper.map('psf', dr.dataId)
    print 'Looking for src', mapper.map('src', dr.dataId)
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
        srcs = dr.get('src')
        print 'Srcs', srcs
    except:
        print 'No sources'
        doDetect = True
        srcs = None

    if doCalib:
        print 'Looking for postISRCCD', mapper.map('postISRCCD', dr.dataId)
        try:
            postisr = dr.get('postISRCCD')
            print 'post-ISR CCD:', postisr
        except:
            print 'No post-ISR CCD'
            doIsr = True

    if forceisr:
        print 'Forcing ISR'
        doIsr = True
    if forcecalib:
        print 'Forcing calibration'
        doCalib = True
    if forcedet:
        print 'Forcing detection'
        doDetect = True
        srcs = None

    conf.doIsr            = doIsr
    conf.doCalibrate      = doCalib
    conf.doWriteCalibrate = doCalib
    conf.doDetection      = doDetect

    if not doDetect:
        # Massage the sources into a pre-deblended state.
        print 'Got', len(srcs), 'sources'
        # Remove children from the Catalog (don't re-deblend)
        n0 = len(srcs)
        i=0
        while i < len(srcs):
            if srcs[i].getParent():
                del srcs[i]
            else:
                i += 1
        n1 = len(srcs)
        if n1 != n0:
            print "Dropped %i of %i 'child' sources (%i remaining)" % ((n0-n1), n0, n1)
        # Make sure the IdFactory exists and doesn't duplicate IDs
        # (until JimB finishes #2083)
        f = srcs.getTable().getIdFactory()
        if f is None:
            f = afwTable.IdFactory.makeSimple()
            srcs.getTable().setIdFactory(f)
        maxid = max([src.getId() for src in srcs])
        f.notify(maxid)
        print 'Max ID in catalog:', maxid

        # still set conf.doDetection = True here so the schema gets populated properly...
        conf.doDetection = True

    conf.doDeblend = True
    conf.doMeasurement = True
    conf.doWriteSources = False
    conf.measurement.doRemoveOtherSources = True

    if debugDeblend:
        conf.doDeblend = False
        # We have to add the schema elements in the right order...
        conf.doMeasurement = False
    if debugMeas:
        conf.doMeasurement = False

    proc = procCcd.ProcessCcdTask(config=conf, name='ProcessCcd')
    if verbose:
        proc.log.setThreshold(pexLog.Log.DEBUG)

    if debugDeblend:
        log = None
        if verbose:
            log = pexLog.Log(proc.log, 'deblend')
            log.setThreshold(pexLog.Log.DEBUG)
        proc.deblend = DebugDeblendTask(proc.schema,
                                        config=conf.deblend,
                                        log=log,
                                        prefix = 'deblend-',
                                        drill=drill)
        conf.doDeblend = True
        if not debugMeas:
            # Put in the normal measurement subtask; we have to do this to ensure
            # the schema elements get added in the right order.
            proc.measurement = measAlg.SourceMeasurementTask(proc.schema,
                                                             algMetadata=proc.algMetadata,
                                                             config=conf.measurement)
            conf.doMeasurement = True

    if debugMeas:
        log = None
        if verbose:
            log = pexLog.Log(proc.log, 'measurement')
            log.setThreshold(pexLog.Log.DEBUG)
        proc.measurement = DebugSourceMeasTask(proc.schema,
                                               algMetadata=proc.algMetadata,
                                               config=conf.measurement,
                                               log=log,
                                               plotmasks = False,
                                               prefix = 'measure-')
        conf.doMeasurement = True

    #print 'proc.schema:', proc.schema
    #print 'srcs.schema:', srcs.getSchema()
    #print
    #print 'Schemas equal (proc.schema, srcs.schema):', (proc.schema == srcs.getSchema())
    #print
    if srcs:
        assert(proc.schema == srcs.getSchema())

    conf.doDetection = doDetect

    res = proc.run(dr, sources=srcs)

    #print 'proc.schema:', proc.schema
    #print 'srcs.schema:', srcs.getSchema()
    #print 'res.sources schema:', res.sources.getSchema()
    #print
    #print 'Schemas equal (proc.schema, srcs.schema):', (proc.schema == srcs.getSchema())
    #print 'Schemas equal (proc.schema, res.sources.schema):', (proc.schema == res.sources.getSchema())
    #print 'Schemas equal (srcs.schema, res.sources.schema):', (srcs.schema == res.sources.getSchema())
    #print
    if srcs:
        assert(proc.schema == srcs.getSchema())
        assert(srcs.getSchema() == res.sources.getSchema())
    assert(proc.schema == res.sources.getSchema())

    srcs = res.sources

    srcs.getTable().setWriteHeavyFootprints(True)
    dr.put(srcs, 'src')

    print 'processCCD.run() finished; srcs should have been written to:'
    print '  ', mapper.map('src', dr.dataId)

    return srcs

def main():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--root', '-r', dest='root', help='Root directory for Subaru data')
    parser.add_option('--outroot', '-o', dest='outroot', help='Output root directory for Subaru data')
    parser.add_option('--force', '-f', dest='force', action='store_true', default=False,
                      help='Force re-running the deblender?')
    parser.add_option('--force-det', '-d', dest='forcedet', action='store_true', default=False,
                      help='Force re-running the detection stage?')
    parser.add_option('--force-calib', dest='forcecalib', action='store_true', default=False,
                      help='Force re-running the Calibration stage?')
    parser.add_option('--force-isr', dest='forceisr', action='store_true', default=False,
                      help='Force re-running the ISR stage?')
    parser.add_option('-s', '--sources', dest='sourcefn', default=None,
                      help='Output filename for source table (FITS)')
    parser.add_option('-H', '--heavy', dest='heavypat', default=None, 
                      help='Output filename pattern for heavy footprints (with %i pattern); FITS.  "yes" for heavy-VVVV-CC-IDID.fits')
    parser.add_option('--nkeep', '-n', dest='nkeep', default=0, type=int,
                      help='Cut to the first N deblend families')
    parser.add_option('--drill', '-D', dest='drill', action='append', type=int, default=[],
                      help='Drill down on individual source IDs')
    parser.add_option('--no-deblend-plots', dest='deblendplots', action='store_false', default=True,
                      help='Do not make deblend plots')
    parser.add_option('--no-measure-plots', dest='measplots', action='store_false', default=True,
                      help='Do not make measurement plots')
    parser.add_option('--no-after-plots', dest='noafterplots', action='store_true',
                      help='Do not make post-deblend+measure plots')
    parser.add_option('--no-plots', dest='noplots', action='store_true', help='No plots at all; --no-deblend-plots, --no-measure-plots, --no-after-plots')
    parser.add_option('--visit', dest='visit', type=int, default=108792, help='Suprimecam visit id')
    parser.add_option('--ccd', dest='ccd', type=int, default=5, help='Suprimecam CCD number')
    parser.add_option('-v', dest='verbose', action='store_true')
    opt,args = parser.parse_args()

    dr = getDataref(visit=opt.visit, ccd=opt.ccd, rootdir=opt.root, outrootdir=opt.outroot)

    if opt.heavypat == 'yes':
        opt.heavypat = 'heavy-%(visit)i-%(ccd)i-%(id)04i.fits'

    if opt.noplots:
        opt.deblendplots = False
        opt.measplots = False
        opt.noafterplots = True
    
    cat = None
    if not opt.force:
        cat = readCatalog(opt.sourcefn, opt.heavypat, ndeblends=opt.nkeep, dataref=dr,
                          patargs=dict(visit=opt.visit, ccd=opt.ccd))
    if cat is None:
        cat = runDeblend(dr, opt.sourcefn,
                         opt.forceisr, opt.forcecalib, opt.forcedet,
                         opt.verbose, opt.drill, opt.deblendplots, opt.measplots)

        if opt.sourcefn:
            print 'Writing FITS to', opt.sourcefn
            cat.writeFits(opt.sourcefn)

        # Write heavy footprints
        if opt.heavypat is not None:
            for src in cat:
                if not src.getParent():
                    continue
                k = src.getId()
                mim = footprintToImage(src.getFootprint())
                fn = opt.heavypat % dict(visit=opt.visit, ccd=opt.ccd, id=k)
                mim.writeFits(fn)
                print 'Wrote', fn
        
    if opt.nkeep:
        cat = cutCatalog(cat, opt.nkeep)

    if opt.noafterplots:
        return

    exposure = dr.get('calexp')
    print 'Exposure', exposure
    mi = exposure.getMaskedImage()
    sigma1 = get_sigma1(mi)
    fams = getFamilies(cat)
    for j,(parent,children) in enumerate(fams):
        plotDeblendFamily(mi, parent, children, cat, sigma1, ellipses=True)
        fn = 'deblend-%04i-a.png' % j
        plt.savefig(fn)
        print 'saved', fn
        plotDeblendFamily(mi, parent, children, cat, sigma1, ellipses=True, plotb=True)
        fn = 'deblend-%04i-b.png' % j
        plt.savefig(fn)
        print 'saved', fn

if __name__ == '__main__':
    main()
    
