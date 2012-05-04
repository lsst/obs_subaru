import matplotlib
matplotlib.use('Agg')
import pylab as plt
import numpy as np

import os
import math
import lsst.pipe.base as pipeBase
import lsst.pipe.tasks.processCcd as procCcd
import lsst.daf.persistence as dafPersist
import lsst.obs.suprimecam as obsSc

import lsst.daf.base as dafBase
import lsst.afw.table as afwTable
import lsst.afw.math  as afwMath
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
from lsst.ip.isr import IsrTask
from lsst.pipe.tasks.calibrate import CalibrateTask

def getFamilies(cat):
    '''
    Returns [ (parent0, children0), (parent1, children1), ...]
    '''
    # parent -> [children] map.
    children = {}
    for src in cat:
        pid = src.getParent()
        if not pid:
            continue
        if pid in children:
            children[pid].append(src)
        else:
            children[pid] = [src]
    keys = children.keys()
    keys.sort()
    return [ (cat.find(pid), children[pid]) for pid in keys ]

def getExtent(bb):
    # so verbose...
    return (bb.getMinX(), bb.getMaxX(), bb.getMinY(), bb.getMaxY())

def cutCatalog(cat, ndeblends):
    # We want to select the first "ndeblends" parents and all their children.
    fams = getFamilies(cat)
    fams = fams[:ndeblends]
    keepcat = afwTable.SourceCatalog(cat.getTable())
    for p,kids in fams:
        keepcat.append(p)
        for k in kids:
            keepcat.append(k)
    keepcat.sort()
    return keepcat

def readCatalog(sourcefn, heavypat, ndeblends=0):
    if not os.path.exists(sourcefn):
        print 'No source catalog:', sourcefn
        return None
    print 'Reading catalog:', sourcefn
    cat = afwTable.SourceCatalog.readFits(sourcefn)
    print len(cat), 'sources'
    cat.sort()

    if ndeblends:
        cat = cutCatalog(cat, ndeblends)
        print 'Cut to', len(cat), 'sources'
    
    print 'Reading heavyFootprints...'
    for src in cat:
        if not src.getParent():
            continue
        heavyfn = heavypat % src.getId()
        if not os.path.exists(heavyfn):
            print 'No heavy footprint:', heavyfn
            return None
        mim = afwImage.MaskedImageF(heavyfn)
        heavy = afwDet.makeHeavyFootprint(src.getFootprint(), mim)
        src.setFootprint(heavy)
    return cat

def getMapper():
    basedir = os.path.join(os.environ['HOME'], 'lsst', 'ACT-data')
    mapperArgs = dict(root=os.path.join(basedir, 'rerun/dstn'),
                      calibRoot=os.path.join(basedir, 'CALIB'))
    mapper = obsSc.SuprimecamMapper(**mapperArgs)
    return mapper

def getButler():
    mapper = getMapper()
    butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    butler = butlerFactory.create()
    return butler

def getDataref():
    butler = getButler()
    print 'Butler', butler
    dataRef = butler.subset('raw', dataId = dict(visit=126969, ccd=5))
    print 'dataRef:', dataRef
    print 'len(dataRef):', len(dataRef)
    for dr in dataRef:
        print '  ', dr
    return dataRef

def runDeblend(sourcefn, heavypat):
    dataRef = getDataref()
    mapper = getMapper()
            
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

    proc = procCcd.ProcessCcdTask(config=conf, name='ProcessCcd')

    conf.calibrate.measurement.doApplyApCorr = False
    conf.measurement.doApplyApCorr = False
    conf.validate()

    for dr in dataRef:
        print 'dr', dr
        # Only do ISR, Calibration, and Measurement if necessary...
        doIsr    = False
        doCalib  = False
        doDetect = False
        print 'calexp', mapper.map('calexp', dr.dataId)
        print 'psf', mapper.map('psf', dr.dataId)
        print 'src', mapper.map('src', dr.dataId)
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
        # "icSrc" are created during 'calibrate'; only bright guys
        # "src"   are created during 'detection'
        # (then are passed to measurement to get filled in)
        try:
            srcs = dr.get('src')
            print 'Srcs', srcs
        except:
            print 'No sources'
            doDetect = True

        conf.doIsr            = doIsr
        conf.doCalibrate      = doCalib
        conf.doWriteCalibrate = doCalib
        conf.doDetection      = doDetect
        conf.doWriteSources   = doDetect

        conf.doMeasurement = False

        proc.run(dr)

        # Now we deblend and run measurement on the deblended hierarchy.
        srcs = dr.get('src')
        print 'Srcs', srcs

        # Make sure the IdFactory exists and doesn't duplicate IDs
        # (until Jim finishes #2083)
        f = srcs.getTable().getIdFactory()
        if f is None:
            f = afwTable.IdFactory.makeSimple()
            srcs.getTable().setIdFactory(f)
        f.notify(max([src.getId() for src in srcs]))
        f = srcs.getTable().getIdFactory()

        # Add flag bits for deblending results
        schema = srcs.getSchema()
        psfkey = schema.addField('deblend.deblended_as_psf', type='Flag',
                                 doc='Deblender thought this source looked like a PSF')
        oobkey = schema.addField('deblend.out_of_bounds', type='Flag',
                                 doc='Deblender thought this source was too close to an edge')
        schema.find('deblend.deblended_as_psf')

        schema = srcs.getSchema()
        schema.find('deblend.deblended_as_psf')

        print 'psfkey:', psfkey
        print 'oobkey:', oobkey

        exposure = dr.get('calexp')
        print 'Exposure', exposure
        mi = exposure.getMaskedImage()

        psf = dr.get('psf')
        print 'PSF:', psf

        from lsst.meas.deblender.baseline import deblend
        import lsst.meas.algorithms as measAlg

        # find the median stdev in the image...
        stats = afwMath.makeStatistics(mi.getVariance(), mi.getMask(), afwMath.MEDIAN)
        sigma1 = math.sqrt(stats.getValue(afwMath.MEDIAN))

        xkey = schema.find('centroid.naive.x').key
        ykey = schema.find('centroid.naive.y').key

        heavies = {}
        n0 = len(srcs)
        for src in srcs:
            # SourceRecords
            #print '  ', src
            fp = src.getFootprint()
            #print '  fp', fp
            pks = fp.getPeaks()
            #print '  pks:', len(pks), pks
            if len(pks) < 2:
                continue

            bb = fp.getBBox()
            xc = int((bb.getMinX() + bb.getMaxX()) / 2.)
            yc = int((bb.getMinY() + bb.getMaxY()) / 2.)

            if hasattr(psf, 'getFwhm'):
                psf_fwhm = psf.getFwhm(xc, yc)
            else:
                pa = measAlg.PsfAttributes(psf, xc, yc)
                psfw = pa.computeGaussianWidth(measAlg.PsfAttributes.ADAPTIVE_MOMENT)
                #print 'PSF width:', psfw
                psf_fwhm = 2.35 * psfw

            print 'Deblending', len(pks), 'peaks'
            X = deblend([fp], [pks], mi, psf, psf_fwhm, sigma1=sigma1)
            res = X[0]
            for pkres in res.peaks:
                child = srcs.addNew()
                child.setParent(src.getId())
                child.setFootprint(pkres.heavy)
                # == pk.getF{xy}(), for now
                x,y = pkres.center
                child.set(xkey, x)
                child.set(ykey, y)
                # Interesting, they're "numpy.bool_"s!
                #print 'deblend_as_psf:', pkres.deblend_as_psf
                #print '  ', type(pkres.deblend_as_psf)
                child.set(psfkey, bool(pkres.deblend_as_psf))
                child.set(oobkey, bool(pkres.out_of_bounds))
                heavies[child.getId()] = pkres.heavy
        n1 = len(srcs)

        print 'Deblending:', n0, 'sources ->', n1

        print 'Measuring...'
        conf.measurement.doRemoveOtherSources = True
        proc.measurement.run(exposure, srcs)
        print 'Writing FITS...'
        srcs.writeFits(sourcefn)

        # Write heavy footprints as well.
        keys = heavies.keys()
        keys.sort()
        for k in keys:
            h = heavies[k]
            bb = h.getBBox()
            mim = afwImage.MaskedImageF(bb.getWidth(), bb.getHeight())
            mim.setXY0(bb.getMinX(), bb.getMinY())
            h.insert(mim)
            fn = heavypat % k
            mim.writeFits(fn)
            print 'Wrote', fn
    return srcs


if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--force', '-f', dest='force', action='store_true', default=False,
                      help='Force re-running the deblender?')
    parser.add_option('-s', '--sources', dest='sourcefn', default='deblended-srcs.fits',
                      help='Output filename for source table (FITS)')
    parser.add_option('-H', '--heavy', dest='heavypat', default='deblend-heavy-%04i.fits',
                      help='Output filename pattern for heavy footprints (with %i pattern); FITS')
    parser.add_option('--nkeep', '-n', dest='nkeep', default=0, type=int,
                      help='Cut to the first N deblend families')
    opt,args = parser.parse_args()

    cat = None
    if not opt.force:
        cat = readCatalog(opt.sourcefn, opt.heavypat, ndeblends=opt.nkeep)
    if cat is None:
        cat = runDeblend(opt.sourcefn, opt.heavypat)
        if opt.nkeep:
            cat = cutCatalog(cat, opt.nkeep)

    # get the exposure too.
    dataRef = getDataref()
    # dataRef doesn't support indexing, but it does support iteration?
    for dr in dataRef:
        break
    #dr = dataRef[0]
    exposure = dr.get('calexp')
    print 'Exposure', exposure
    mi = exposure.getMaskedImage()

    stats = afwMath.makeStatistics(mi.getVariance(), mi.getMask(), afwMath.MEDIAN)
    sigma1 = math.sqrt(stats.getValue(afwMath.MEDIAN))

    schema = cat.getSchema()
    psfkey = schema.find("deblend.deblended_as_psf").key

    fams = getFamilies(cat)
    for j,(parent,children) in enumerate(fams):

        #print 'Parent:', parent
        #print 'Children:', len(children)
        #for c in children:
        #    print '  ', c
        # Insert the parent's footprint pixels into a new blank image
        fp = afwDet.makeHeavyFootprint(parent.getFootprint(), mi)
        bb = fp.getBBox()
        im = afwImage.ImageF(bb.getWidth(), bb.getHeight())
        im.setXY0(bb.getMinX(), bb.getMinY())
        fp.insert(im)
        pim = im.getArray()

        # Insert the children's footprints into images too
        chims = []
        for ch in children:
            fp = ch.getFootprint()
            #print 'child', ch
            #print 'fp', fp
            fp = afwDet.cast_HeavyFootprintF(fp)
            #print '-> fp', fp
            bb = fp.getBBox()
            im = afwImage.ImageF(bb.getWidth(), bb.getHeight())
            im.setXY0(bb.getMinX(), bb.getMinY())
            fp.insert(im)
            chims.append(im.getArray())

        N = 1 + len(chims)
        S = math.ceil(math.sqrt(N))
        C = S
        R = math.ceil(float(N) / C)

        #, vmin=pim.min(), vmax=pim.max())
        def nlmap(X):
            return np.arcsinh(X / (3.*sigma1))
        def myimshow(im, **kwargs):
            kwargs = kwargs.copy()
            mn = kwargs.get('vmin', -5*sigma1)
            kwargs['vmin'] = nlmap(mn)
            mx = kwargs.get('vmax', 100*sigma1)
            kwargs['vmax'] = nlmap(mx)
            # arcsinh:
            # 1->1
            # 3->2
            # 10->3
            # 100->5
            plt.imshow(nlmap(im), **kwargs)

        print 'max', pim.max()/sigma1, 'sigma'
        imargs = dict(interpolation='nearest', origin='lower',
                      vmax=pim.max())

        plt.figure(figsize=(8,8))
        plt.clf()
        plt.subplots_adjust(left=0.05, right=0.95, bottom=0.05, top=0.9,
                            wspace=0.05, hspace=0.05)

        # plot a: nonlinear map; pad each child to the parent area.
        plt.subplot(R, C, 1)
        pext = getExtent(parent.getFootprint().getBBox())
        myimshow(pim, extent=pext, **imargs)
        plt.xticks([])
        plt.yticks([])
        m = 0.25
        pax = [pext[0]-m, pext[1]+m, pext[2]-m, pext[3]+m]
        #plt.axis(pax)
        plt.title('parent %i' % parent.getId())
        plt.gray()
        Rx,Ry = [],[]
        tts = []
        ccs = []
        for i,im in enumerate(chims):
            child = children[i]
            ispsf = child.get(psfkey)
            plt.subplot(R, C, i+2)
            bb = child.getFootprint().getBBox()
            ext = getExtent(bb)
            myimshow(im, extent=ext, **imargs)
            plt.xticks([])
            plt.yticks([])
            plt.axis(pax)
            tt = 'child %i' % child.getId()
            if ispsf:
                cc = 'g'
                tt += ' (psf)'
            else:
                cc = 'r'
            tts.append(tt)
            ccs.append(cc)
            plt.title(tt)
            plt.gray()
            xx = [ext[0],ext[1],ext[1],ext[0],ext[0]]
            yy = [ext[2],ext[2],ext[3],ext[3],ext[2]]
            plt.plot(xx, yy, '-', color=cc)
            Rx.append(xx)
            Ry.append(yy)
        plt.subplot(R, C, 1)
        # plt.plot(np.array(Rx).T, np.array(Ry).T, 'r-')
        for rx,ry,cc in zip(Rx, Ry, ccs):
            plt.plot(rx, ry, '-', color=cc)
        plt.axis(pax)
        plt.savefig('deblend-%04i-a.png' % j)
            
        # plot b: keep the parent plot; plot each child with its own stretch and bounding-box
        for i,im in enumerate(chims):
            child = children[i]
            plt.subplot(R, C, i+2)
            bb = child.getFootprint().getBBox()
            ext = getExtent(bb)
            imargs.update(vmax=max(3.*sigma1, im.max()))
            myimshow(im, extent=ext, **imargs)
            plt.xticks([])
            plt.yticks([])
            plt.axis(ext)
            plt.title(tts[i])
            plt.gray()
        plt.savefig('deblend-%04i-b.png' % j)

        
