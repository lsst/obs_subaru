import matplotlib
matplotlib.use('Agg')
from matplotlib.patches import Ellipse
import pylab as plt
import numpy as np

import os
import math
import lsst.pipe.base as pipeBase
import lsst.pipe.tasks.processCcd as procCcd
import lsst.daf.persistence as dafPersist
import lsst.obs.suprimecam as obsSc

import lsst.pex.logging as pexLog
import lsst.daf.base as dafBase
import lsst.afw.table as afwTable
import lsst.afw.math  as afwMath
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet

debugPlots = True
if debugPlots:
    #
    # Plot the image that is passed to the measurement algorithms
    #
    import pylab as plt
    import numpy as np
    import lsst.meas.algorithms as measAlg
    import lsst.afw.math as afwMath
    class DebugSourceMeasTask(measAlg.SourceMeasurementTask):
        def __init__(self, *args, **kwargs):
            self.prefix = kwargs.pop('prefix', '')
            self.plotmasks = kwargs.pop('plotmasks', True)
            self.plotregion = None
            super(DebugSourceMeasTask, self).__init__(*args, **kwargs)
        def __str__(self):
            return 'DebugSourceMeasTask'
        def _plotimage(self, im):
            if self.plotregion is not None:
                xlo,xhi,ylo,yhi = self.plotregion
            plt.clf()
            if not isinstance(im, np.ndarray):
                im = im.getArray()
            if self.plotregion is not None:
                plt.imshow(im[ylo:yhi, xlo:xhi],
                           extent=[xlo,xhi,ylo,yhi], **self.plotargs)
            else:
                plt.imshow(im, **self.plotargs)
            plt.gray()
        def savefig(self, fn):
            plotfn = '%s%s.png' % (self.prefix, fn)
            plt.savefig(plotfn)
            print 'wrote', plotfn
        def preMeasureHook(self, exposure, sources):
            measAlg.SourceMeasurementTask.preMeasureHook(self, exposure, sources)
            mi = exposure.getMaskedImage()
            im = mi.getImage()
            s = afwMath.makeStatistics(im, afwMath.STDEVCLIP + afwMath.MEANCLIP)
            mn = s.getValue(afwMath.MEANCLIP)
            std = s.getValue(afwMath.STDEVCLIP)
            lo = mn -  3*std
            hi = mn + 20*std
            self.plotargs = dict(interpolation='nearest', origin='lower',
                                 vmin=lo, vmax=hi)
            #self.plotregion = (100, 500, 100, 500)
            #self.nplots = 0
            self._plotimage(im)
            self.savefig('pre')

            # Save the parents & children in order.
            self.plotorder = []
            fams = getFamilies(sources)
            for p,ch in fams:
                self.plotorder.append(p.getId())
                self.plotorder.extend([c.getId() for c in ch])
            print 'Will save', len(self.plotorder), 'plots'

            # remember the deblend parents
            # pset = set()
            # for src in sources:
            #     p = src.getParent()
            #     if not p:
            #         continue
            #     pset.add(p)
            # self.parents = list(pset)
            
        def postMeasureHook(self, exposure, sources):
            measAlg.SourceMeasurementTask.postMeasureHook(self, exposure, sources)
            mi = exposure.getMaskedImage()
            im = mi.getImage()
            self._plotimage(im)
            self.savefig('post')
        def preSingleMeasureHook(self, exposure, sources, i):
            measAlg.SourceMeasurementTask.preSingleMeasureHook(self, exposure, sources, i)
            if i != -1:
                return
            if not self.plotmasks:
                return
            mi = exposure.getMaskedImage()
            mask = mi.getMask()
            print 'Mask planes:'
            mask.printMaskPlanes()
            oldargs = self.plotargs
            args = oldargs.copy()
            args.update(vmin=0, vmax=1)
            self.plotargs = args
            ma = mask.getArray()
            for i in range(mask.getNumPlanesUsed()):
                bitmask = (1 << i)
                mim = ((ma & bitmask) > 0)
                self._plotimage(mim)
                plt.title('Mask plane %i' % i)
                self.savefig('mask-bit%02i' % i)
            self.plotargs = oldargs
        def postSingleMeasureHook(self, exposure, sources, i):
            measAlg.SourceMeasurementTask.postSingleMeasureHook(self, exposure, sources, i)
            src = sources[i]
            if self.plotregion is not None:
                xlo,xhi,ylo,yhi = self.plotregion
                x,y = src.getX(), src.getY()
                if x < xlo or x > xhi or y < ylo or y > yhi:
                    return
                if (not np.isfinite(x)) or (not np.isfinite(y)):
                    return
            #if not (src.getId() in self.parents or src.getParent()):
            if not src.getId() in self.plotorder:
                print 'source', src.getId(), 'not blended'
                return
            iplot = self.plotorder.index(src.getId())
            print 'saving', iplot

            bb = src.getFootprint().getBBox()
            bb.grow(50)
            #x0 = max(0, bb.getMinX())
            #x1 = min(exposure.getWidth(), bb.getMaxX())
            #y0 = max(0, bb.getMinY())
            #y1 = min(exposure.getHeight(), bb.getMaxY())
            bb.clip(exposure.getBBox())
            #self.plotregion = (x0,x1,y0,y1)
            self.plotregion = getExtent(bb)
            
            mi = exposure.getMaskedImage()
            im = mi.getImage()
            self._plotimage(im)
            #self.savefig('meas%04i' % self.nplots)
            self.savefig('meas%04i' % iplot)
            mask = mi.getMask()
            thisbitmask = mask.getPlaneBitMask('THISDET')
            otherbitmask = mask.getPlaneBitMask('OTHERDET')
            ma = mask.getArray()
            thisim  = ((ma & thisbitmask) > 0)
            otherim = ((ma & otherbitmask) > 0)
            mim = (thisim * 1.) + (otherim * 0.4)
            oldargs = self.plotargs
            args = oldargs.copy()
            args.update(vmin=0, vmax=1)
            self.plotargs = args
            self._plotimage(mim)
            self.plotargs = oldargs
            self.savefig('meas%04i-mask' % iplot)
            #self.savefig('meas%02i-mask' % self.nplots)
            #self.nplots += 1
            ###
            self.plotregion = None


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

def runDeblend(sourcefn, heavypat, forcedet):
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

    conf.calibrate.measurement.doApplyApCorr = False
    conf.measurement.doApplyApCorr = False
    conf.validate()

    for dr in dataRef:
        print 'dr', dr
        # Only do ISR, Calibration, and Detection if necessary...
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

        if forcedet:
            print 'Forcing detection'
            doDetect = True

        conf.doIsr            = doIsr
        conf.doCalibrate      = doCalib
        conf.doWriteCalibrate = doCalib
        conf.doDetection      = doDetect

        if False and doDetect:
            # Run processCcd only up to detection, with deblending and
            # measurement turned off, so that we can save the sources,
            # without the meas & deblend additions to the schema, as
            # they are at that point.
            conf.doDeblend = False
            conf.doMeasurement = False
            conf.doWriteSources = True
            print 'Running up to detection...'
            proc = procCcd.ProcessCcdTask(config=conf, name='ProcessCcd')
            proc.log.setThreshold(pexLog.Log.DEBUG)
            res = proc.run(dr)
            srcs = res.sources
            #srcs = dr.get('src')
            conf.doIsr       = False
            conf.doCalibrate = False
            conf.doDetection = False

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

        conf.doDeblend = True
        conf.doMeasurement = True
        if debugPlots:
            conf.doMeasurement = False
        # Don't write out sources to the standard place
        #conf.doWriteSources = False

        conf.doWriteSources = True
        conf.measurement.doRemoveOtherSources = True

        proc = procCcd.ProcessCcdTask(config=conf, name='ProcessCcd')
        proc.log.setThreshold(pexLog.Log.DEBUG)

        if debugPlots:
            proc.measurement = DebugSourceMeasTask(proc.schema,
                                                   algMetadata=proc.algMetadata,
                                                   config=conf.measurement,
                                                   plotmasks = False)
            proc.measurement.prefix = 'measure-'

        res = proc.run(dr, sources=srcs)
        srcs = res.sources

        if debugPlots:
            print 'Running measurement...'
            proc.measurement.run(res.exposure, res.sources)

        print 'Writing FITS...'
        srcs.writeFits(sourcefn)

        # Write heavy footprints as well.
        for src in srcs:
            if not src.getParent():
                continue
            k = src.getId()
            h = afwDet.cast_HeavyFootprintF(src.getFootprint())
            bb = h.getBBox()
            mim = afwImage.MaskedImageF(bb.getWidth(), bb.getHeight())
            mim.setXY0(bb.getMinX(), bb.getMinY())
            h.insert(mim)
            fn = heavypat % k
            mim.writeFits(fn)
            print 'Wrote', fn

    return srcs

def getEllipses(src, nsigs=[1.], **kwargs):
    xc = src.getX()
    yc = src.getY()
    x2 = src.getIxx()
    y2 = src.getIyy()
    xy = src.getIxy()
    # SExtractor manual v2.5, pg 29.
    a2 = (x2 + y2)/2. + np.sqrt(((x2 - y2)/2.)**2 + xy**2)
    b2 = (x2 + y2)/2. - np.sqrt(((x2 - y2)/2.)**2 + xy**2)
    theta = np.rad2deg(np.arctan2(2.*xy, (x2 - y2)) / 2.)
    a = np.sqrt(a2)
    b = np.sqrt(b2)
    ells = []
    for nsig in nsigs:
        ells.append(Ellipse([xc,yc], 2.*a*nsig, 2.*b*nsig, angle=theta, **kwargs))
    return ells

def drawEllipses(src, **kwargs):
    els = getEllipses(src, **kwargs)
    for el in els:
        plt.gca().add_artist(el)
    return els

if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--force', '-f', dest='force', action='store_true', default=False,
                      help='Force re-running the deblender?')
    parser.add_option('--force-det', '-d', dest='forcedet', action='store_true', default=False,
                      help='Force re-running the detection stage?')
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
        cat = runDeblend(opt.sourcefn, opt.heavypat, opt.forcedet)
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
    psfkey = schema.find("deblend.deblended-as-psf").key

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
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        m = 0.25
        pax = [pext[0]-m, pext[1]+m, pext[2]-m, pext[3]+m]
        plt.title('parent %i' % parent.getId())
        Rx,Ry = [],[]
        tts = []
        ccs = []
        xys = []
        for i,im in enumerate(chims):
            child = children[i]
            plt.subplot(R, C, i+2)
            fp = child.getFootprint()
            bb = fp.getBBox()
            ext = getExtent(bb)
            myimshow(im, extent=ext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            tt = 'child %i' % child.getId()
            ispsf = child.get(psfkey)
            if ispsf:
                cc = 'g'
                cc2 = (0.1,0.5,0.1)
                tt += ' (psf)'
            else:
                cc = 'r'
                cc2 = (0.8,0.1,0.1)
            tts.append(tt)
            ccs.append(cc)
            plt.title(tt)
            # bounding box
            xx = [ext[0],ext[1],ext[1],ext[0],ext[0]]
            yy = [ext[2],ext[2],ext[3],ext[3],ext[2]]
            plt.plot(xx, yy, '-', color=cc)
            Rx.append(xx)
            Ry.append(yy)
            # peak(s)
            px = [pk.getFx() for pk in fp.getPeaks()]
            py = [pk.getFy() for pk in fp.getPeaks()]
            plt.plot(px, py, 'x', color=cc2)
            xys.append((px, py, cc2))
            # centroid
            plt.plot([child.getX()], [child.getY()], 'x', color=cc)
            xys.append(([child.getX()], [child.getY()], cc))
            # ellipse
            if not ispsf:
                drawEllipses(child, ec=cc, fc='none', alpha=0.7)
            plt.axis(pax)
        # Go back to the parent plot and add child bboxes
        plt.subplot(R, C, 1)
        for rx,ry,cc in zip(Rx, Ry, ccs):
            plt.plot(rx, ry, '-', color=cc)
        # add child centers and ellipses...
        for x,y,cc in xys:
            plt.plot(x, y, 'x',color=cc)
        for child,cc in zip(children,ccs):
            ispsf = child.get(psfkey)
            if ispsf:
                continue
            drawEllipses(child, ec=cc, fc='none', alpha=0.7)
        plt.plot([parent.getX()],[parent.getY()],'x',color='b')
        drawEllipses(parent, ec='b', fc='none', alpha=0.7)
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
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.title(tts[i])
            cc = ccs[i]
            x,y,nil = xys[i]
            plt.plot([x], [y], 'x', color=cc)
            ispsf = child.get(psfkey)
            if not ispsf:
                drawEllipses(child, ec=cc, fc='none', alpha=0.7)
            plt.axis(ext)
        plt.savefig('deblend-%04i-b.png' % j)

        
