import pylab as plt
import numpy as np

import lsst.pex.logging as pexLog
import lsst.afw.table as afwTable
import lsst.meas.algorithms as measAlg

from utils import *

def addToParser(parser):
    if parser is None:
        from optparse import OptionParser
        parser = OptionParser()
    parser.add_option('--force', '-f', '--force-deblend', dest='force',
                      action='store_true', default=False, help='Force re-running the deblender?')
    parser.add_option('--force-det', '--force-detect', '--force-detection', '-d',
                      dest='forcedet', action='store_true', default=False,
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
    parser.add_option('--roi', dest='roi', type=float, nargs=4, help='Select an x0,x1,y0,y1 subset of the image')
    parser.add_option('--no-deblend-plots', dest='deblendplots', action='store_false', default=True,
                      help='Do not make deblend plots')
    parser.add_option('--no-measure-plots', dest='measplots', action='store_false', default=True,
                      help='Do not make measurement plots')
    parser.add_option('--no-after-plots', dest='noafterplots', action='store_true',
                      help='Do not make post-deblend+measure plots')
    parser.add_option('--no-plots', dest='noplots', action='store_true', help='No plots at all; --no-deblend-plots, --no-measure-plots, --no-after-plots')
    parser.add_option('--threads', dest='threads', type=int, help='Multiprocessing for plots?')
    parser.add_option('--overview', dest='overview', help='Produce an overview plot?')

    parser.add_option('--drop-psf', dest='drop_faint_psf', type=float,
                      help='Drop deblended children that are fit as PSFs with flux less than the given value')
    parser.add_option('--drop-single', dest='drop_single', action='store_true',
                      help='Drop deblend families that have only a single child?')
    parser.add_option('--drop-well-fit', dest='drop_well_fit', action='store_true',
                      help='Drop deblend families of PSFs that fit the data well?')

    parser.add_option('--plot-masks', dest='plot_masks', action='store_true', default=False,
                      help='Produce plots showing the mask bits set in deblend families?')

    parser.add_option('--prefix', dest='prefix', default='', help='Prefix for plot filenames')

    parser.add_option('-v', dest='verbose', action='store_true')

class DebugDeblendTask(measAlg.SourceDeblendTask):
    def __init__(self, *args, **kwargs):
        self.prefix = kwargs.pop('prefix', '')
        self.drill = kwargs.pop('drill', [])
        if not len(self.drill):
            self.drill = None
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
        plotDeblendFamily(mi, parent, kids, [], srcs, sigma1, ellipses=False)
        self.savefig('%04i-posta' % self.plotnum)
        plotDeblendFamily(mi, parent, kids, [], srcs, sigma1, plotb=True, ellipses=False)
        self.savefig('%04i-postb' % self.plotnum)
        self.plotnum += 1


def runDeblend(dataRef, sourcefn, conf, procclass, forceisr=False, forcecalib=False, forcedet=False,
               verbose=False, drill=[], debugDeblend=True, debugMeas=True,
               calibInput='postISRCCD', tweak_config=None, prefix=None,
               printMap=False, hasIsr=True):
    if prefix is None:
        prefix = ''

    dr = dataRef
    mapper = datarefToMapper(dr)

    conf.detection.returnOriginalFootprints = False
    conf.calibrate.doComputeApCorr = False
    if hasattr(conf.calibrate, 'doAstrometry'):
        conf.calibrate.doAstrometry = False
    conf.calibrate.doPhotoCal = False
    conf.calibrate.measurement.doApplyApCorr = False
    conf.measurement.doApplyApCorr = False

    conf.validate()

    # Only do ISR, Calibration, and Detection if necessary (or forced)
    doIsr    = False
    doCalib  = False
    doDetect = False
    if printMap:
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
        print 'Looking for', calibInput
        try:
            calin = dr.get(calibInput)
            print calibInput, calin
        except:
            print 'No', calibInput
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

    if hasIsr:
        conf.doIsr            = doIsr
    conf.doCalibrate      = doCalib
    conf.doWriteCalibrate = doCalib
    conf.doDetection      = doDetect

    if tweak_config:
        tweak_config(conf)

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
    conf.doWriteSources = True
    conf.doWriteHeavyFootprintsInSources = True
    conf.measurement.doReplaceWithNoise = True

    if debugDeblend:
        conf.doDeblend = False
        # We have to add the schema elements in the right order...
        conf.doMeasurement = False
    if debugMeas:
        conf.doMeasurement = False

    proc = procclass(config=conf, name='ProcessCcd')
    if verbose:
        proc.log.setThreshold(pexLog.Log.DEBUG)

    if debugDeblend:
        log = None
        if verbose:
            log = pexLog.Log(proc.log, 'deblend')
            log.setThreshold(pexLog.Log.DEBUG)
        proc.deblend = DebugDeblendTask(proc.schema, config=conf.deblend, log=log,
                                        prefix = prefix + 'deblend-', drill=drill)
        conf.doDeblend = True
        if not debugMeas:
            # Put in the normal measurement subtask; we have to do
            # this after the deblending task to ensure the schema
            # elements get added in the right order.
            proc.measurement = measAlg.SourceMeasurementTask(proc.schema,
                                                             algMetadata=proc.algMetadata,
                                                             config=conf.measurement)
            conf.doMeasurement = True

    if debugMeas:
        log = None
        if verbose:
            log = pexLog.Log(proc.log, 'measurement')
            log.setThreshold(pexLog.Log.DEBUG)
        proc.measurement = DebugSourceMeasTask(proc.schema, algMetadata=proc.algMetadata,
                                               config=conf.measurement, log=log,
                                               plotmasks = False, prefix = prefix + 'measure-')
        conf.doMeasurement = True

    if srcs:
        assert(proc.schema == srcs.getSchema())

    conf.doDetection = doDetect
    res = proc.run(dr, sources=srcs)

    if srcs:
        assert(proc.schema == srcs.getSchema())
        assert(srcs.getSchema() == res.sources.getSchema())
    assert(proc.schema == res.sources.getSchema())
    srcs = res.sources

    print 'Processing finished; srcs should have been written to:'
    if printMap:
        print '  ', mapper.map('src', dr.dataId)

    return srcs



# for multiprocess.Pool.map()
def _plotfunc((args, kwargs, fn)):
    plotDeblendFamilyReal(*args, **kwargs)
    plt.savefig(fn)
    print 'saved', fn

def t1091main(dr, opt, conf, proc, patargs={}, rundeblendargs={}, pool=None):
              
    if opt.noplots:
        opt.deblendplots = False
        opt.measplots = False
        opt.noafterplots = True
    
    cat = None
    if not opt.force:
        cat = readCatalog(opt.sourcefn, opt.heavypat, ndeblends=opt.nkeep, dataref=dr,
                          patargs=patargs)
    if cat is None:
        cat = runDeblend(dr, opt.sourcefn, conf, proc,
                         opt.forceisr, opt.forcecalib, opt.forcedet,
                         opt.verbose, opt.drill, opt.deblendplots, opt.measplots,
                         prefix=opt.prefix, **rundeblendargs)
                         

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
                args = patargs.copy()
                args.update(id=k)
                fn = opt.heavypat % args
                mim.writeFits(fn)
                print 'Wrote', fn

    if opt.roi is not None:
        print 'Selecting objects in region-of-interest', opt.roi
        xx = [src.getX() for src in cat]
        yy = [src.getY() for src in cat]
        print 'Source positions range:', min(xx), max(xx), min(yy), max(yy)
        (x0,x1,y0,y1) = opt.roi
        keep = [src for src,x,y in zip(cat,xx,yy)
                if (x >= x0) and (x <= x1) and (y >= y0) and (y <= y1)]
        print 'Keeping', len(keep), 'of', len(cat), 'sources'

        keepcat = afwTable.SourceCatalog(cat.getTable())
        for k in keep:
            keepcat.append(k)
        keepcat.sort()
        cat = keepcat

    if opt.overview is not None:
        print 'Producing overview plot...'
        exposure = dr.get('calexp')
        print 'Exposure', exposure
        mi = exposure.getMaskedImage()
        sigma1 = get_sigma1(mi)
        W,H = mi.getWidth(), mi.getHeight()
        im = mi.getImage().getArray()
        imext = getExtent(mi.getBBox(afwImage.PARENT))

        # This seems a little extreme...
        #dpi=100
        #plt.figure(figsize=(1+W/dpi, 1+H/dpi), dpi=dpi)

        plt.clf()
        plt.imshow(im, interpolation='nearest', origin='lower',
                   extent=imext, vmin=-2.*sigma1, vmax=10.*sigma1)
        plt.gray()

        if opt.roi is not None:
            plt.axis(opt.roi)

        plt.savefig(opt.overview % 'a')
        ax = plt.axis()

        cxx,cyy,pxx,pyy = [],[],[],[]
        for i,src in enumerate(cat):
            if src.getParent():
                cxx.append(src.getX())
                cyy.append(src.getY())
            else:
                pxx.append(src.getX())
                pyy.append(src.getY())
        if len(cxx):
            plt.plot(cxx, cyy, 'b+')
        if len(pxx):
            plt.plot(pxx, pyy, 'r+')
        plt.axis(ax)
        plt.savefig(opt.overview % 'b')


        plt.imshow(im, interpolation='nearest', origin='lower',
                   extent=imext, vmin=-2.*sigma1, vmax=10.*sigma1)
        plt.gray()
        for i,src in enumerate(cat):
            ext = getExtent(src.getFootprint().getBBox())
            xx = [ext[0],ext[1],ext[1],ext[0],ext[0]]
            yy = [ext[2],ext[2],ext[3],ext[3],ext[2]]
            if len(src.getFootprint().getPeaks()) == 1:
                c = 'r'
            else:
                c = 'y'
            plt.plot(xx, yy, '-', color=c)
        plt.axis(ax)
        plt.savefig(opt.overview % 'c')
        
    if opt.nkeep:
        cat = cutCatalog(cat, opt.nkeep)

    if opt.noafterplots:
        return

    but = datarefToButler(dr)
    idbits = but.get('ccdExposureId_bits')
    idmask = (1 << idbits)-1
    
    exposure = dr.get('calexp')
    psf = dr.get('psf')
    mi = exposure.getMaskedImage()
    sigma1 = get_sigma1(mi)
    fams = getFamilies(cat)

    fams = [(parent, kids, []) for parent,kids in fams]

    if opt.drop_well_fit:
        #
        threshold = 2.
        
        schema = cat.getSchema()
        psfkey = schema.find("deblend.deblended-as-psf").key
        fluxkey = schema.find('deblend.psf-flux').key
        centerkey = schema.find('deblend.psf-center').key

        keep = []
        for i,(parent,kids,dkids) in enumerate(fams):
            if not all([kid.get(psfkey) for kid in kids]):
                keep.append((parent,kids,dkids))
                continue

            data = footprintToImage(parent.getFootprint(), mi)
            bb = parent.getFootprint().getBBox()
            model = afwImage.ImageF(bb.getWidth(), bb.getHeight())
            model.setXY0(bb.getMinX(), bb.getMinY())
            x0,y0 = bb.getMinX(), bb.getMinY()

            for kid in kids:
                flux = kid.get(fluxkey)
                if flux < 0:
                    continue
                xy = kid.get(centerkey)
                psfimg = psf.computeImage(xy)
                psfimg *= flux
                
                pbb = psfimg.getBBox(afwImage.PARENT)
                pbb.clip(bb)
                psfimg = psfimg.Factory(psfimg, pbb, afwImage.PARENT)

                model.getArray()[pbb.getMinY()-y0:pbb.getMaxY()+1-y0,
                                 pbb.getMinX()-x0:pbb.getMaxX()+1-x0] += psfimg.getArray()

            unfit = np.sum((data.getArray()/sigma1)**2) / parent.getFootprint().getNpix()
            fit = np.sum(((data.getArray() - model.getArray())/sigma1)**2) / parent.getFootprint().getNpix()
                
            if fit > threshold:
                keep.append((parent, kids, dkids))
                continue

            print 'Dropped deblend: Chisq per pixel:', unfit, '->', fit

            if False:
                pa = dict(interpolation='nearest', origin='lower')
                pa1 = pa.copy()
                pa1.update(vmin=-3.*sigma1, vmax=10.*sigma1)
                pa2 = pa.copy()
                pa2.update(vmin=-5., vmax=5.)
                plt.clf()
                plt.subplot(2,2,1)
                plt.imshow(data.getArray(), **pa1)
                plt.gray()
                plt.colorbar()
                plt.title('chisq per pix: %.2f -> %.2f' % (unfit, fit))
                plt.subplot(2,2,2)
                plt.imshow(model.getArray(), **pa1)
                plt.colorbar()
                plt.subplot(2,2,3)
                plt.imshow((data.getArray() - model.getArray()) / sigma1, **pa2)
                plt.title('chi')
                plt.colorbar()
                fn = opt.prefix + 'psfmodel-%04i.png' % i
                plt.savefig(fn)
                print 'saved', fn

        fams = keep

    if opt.drop_faint_psf:
        faint = opt.drop_faint_psf
        schema = cat.getSchema()
        psfkey = schema.find("deblend.deblended-as-psf").key
        fluxkey = schema.find('deblend.psf-flux').key

        for parent,kids,dropkids in fams:
            rmkids = []
            for kid in kids:
                if kid.get(psfkey) and kid.get(fluxkey) < faint:
                    print 'Dropping PSF child with flux', kid.get(fluxkey)
                    dropkids.append(kid)
                    rmkids.append(kid)
            for kid in rmkids:
                kids.remove(kid)

    if opt.drop_single:
        fams = [(parent, kids, dkids) for (parent, kids, dkids) in fams if len(kids) > 1]

    A = []
    for j,(parent,children,dkids) in enumerate(fams):
        args = (mi, parent, children, dkids, cat, sigma1)

        fnkwargs = [
            (opt.prefix + 'deblend-%04i-a.png' % j,
             dict(ellipses=True, idmask=idmask)),
            (opt.prefix + 'deblend-%04i-b.png' % j,
             dict(ellipses=True, plotb=True, idmask=idmask)),
            ]
        if opt.plot_masks:
            fnkwargs += [(opt.prefix + 'deblend-%04i-mask-%s.png' % (j, nm.lower()),
                          dict(ellipses=False, idmask=idmask, maskbit=mi.getMask().getPlaneBitMask(nm),
                               arcsinh=False))
                         for nm in ['BAD', 'SAT', 'INTRP', 'CR', 'EDGE']]

        if pool is None:
            for fn,kwargs in fnkwargs:
                plotDeblendFamily(*args, **kwargs)
                plt.savefig(fn)
                print 'saved', fn

        else:
            for fn,kwargs in fnkwargs:
                A.append((plotDeblendFamilyPre(*args, **kwargs), kwargs, fn))

    if pool:
        pool.map(_plotfunc, A)
