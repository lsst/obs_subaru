import math
import os
import pylab as plt
import numpy as np
from matplotlib.patches import Ellipse

import lsst.daf.persistence as dafPersist
import lsst.afw.math as afwMath
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.afw.table as afwTable
import lsst.meas.algorithms as measAlg
import lsst.obs.suprimecam as obsSc

class DebugSourceMeasTask(measAlg.SourceMeasurementTask):
    '''Plot the image that is passed to the measurement algorithms'''
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
        if not src.getId() in self.plotorder:
            print 'source', src.getId(), 'not blended'
            return
        iplot = self.plotorder.index(src.getId())
        if iplot > 100:
            print 'skipping', iplot
            return
        if self.plotregion is not None:
            xlo,xhi,ylo,yhi = self.plotregion
            x,y = src.getX(), src.getY()
            if x < xlo or x > xhi or y < ylo or y > yhi:
                return
            if (not np.isfinite(x)) or (not np.isfinite(y)):
                return
        #if not (src.getId() in self.parents or src.getParent()):
        print 'saving', iplot

        if src.getParent():
            bb = sources.find(src.getParent()).getFootprint().getBBox()
        else:
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



def plotDeblendFamily(mi, parent, kids, srcs, sigma1, plotb=False, ellipses=True):
    schema = srcs.getSchema()
    psfkey = schema.find("deblend.deblended-as-psf").key
    xkey = schema.find('centroid.naive.x').key
    ykey = schema.find('centroid.naive.y').key

    pid = parent.getId()
    pim = footprintToImage(parent.getFootprint(), mi).getArray()
    chims = [footprintToImage(ch.getFootprint()).getArray() for ch in kids]
    N = 1 + len(chims)
    S = math.ceil(math.sqrt(N))
    C = S
    R = math.ceil(float(N) / C)
    def nlmap(X):
        return np.arcsinh(X / (3.*sigma1))
    def myimshow(im, **kwargs):
        kwargs = kwargs.copy()
        mn = kwargs.get('vmin', -5*sigma1)
        kwargs['vmin'] = nlmap(mn)
        mx = kwargs.get('vmax', 100*sigma1)
        kwargs['vmax'] = nlmap(mx)
        plt.imshow(nlmap(im), **kwargs)
    imargs = dict(interpolation='nearest', origin='lower',
                  vmax=pim.max())
    plt.figure(figsize=(8,8))
    plt.clf()
    plt.subplots_adjust(left=0.05, right=0.95, bottom=0.05, top=0.9,
                        wspace=0.05, hspace=0.1)

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
    stys = []
    xys = []
    for i,im in enumerate(chims):
        child = kids[i]
        plt.subplot(R, C, i+2)
        fp = child.getFootprint()
        bb = fp.getBBox()
        ext = getExtent(bb)

        if plotb:
            ima = imargs.copy()
            ima.update(vmax=max(3.*sigma1, im.max()))
        else:
            ima = imargs

        myimshow(im, extent=ext, **ima)
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        tt = 'child %i' % child.getId()
        ispsf = child.get(psfkey)
        print 'child', child.getId(), 'ispsf:', ispsf
        if ispsf:
            sty1 = dict(color='g')
            sty2 = dict(color=(0.1,0.5,0.1), lw=2, alpha=0.5)
            tt += ' (psf)'
        else:
            sty1 = dict(color='r')
            sty2 = dict(color=(0.8,0.1,0.1), lw=2, alpha=0.5)
        tts.append(tt)
        stys.append(sty1)
        plt.title(tt)
        # bounding box
        xx = [ext[0],ext[1],ext[1],ext[0],ext[0]]
        yy = [ext[2],ext[2],ext[3],ext[3],ext[2]]
        plt.plot(xx, yy, '-', **sty1)
        Rx.append(xx)
        Ry.append(yy)
        # peak(s)
        px = [pk.getFx() for pk in fp.getPeaks()]
        py = [pk.getFy() for pk in fp.getPeaks()]
        plt.plot(px, py, 'x', **sty2)
        xys.append((px, py, sty2))
        # centroid
        x,y = child.get(xkey), child.get(ykey)
        plt.plot([x], [y], 'x', **sty1)
        xys.append(([x], [y], sty1))
        # ellipse
        if ellipses and not ispsf:
            drawEllipses(child, ec=sty1['color'], fc='none', alpha=0.7)

        if plotb:
            plt.axis(ext)
        else:
            plt.axis(pax)
            
    # Go back to the parent plot and add child bboxes
    plt.subplot(R, C, 1)
    for rx,ry,sty in zip(Rx, Ry, stys):
        plt.plot(rx, ry, '-', **sty)
    # add child centers and ellipses...
    for x,y,sty in xys:
        plt.plot(x, y, 'x', **sty)
    if ellipses:
        for child,sty in zip(kids,stys):
            ispsf = child.get(psfkey)
            if ispsf:
                continue
            drawEllipses(child, ec=sty['color'], fc='none', alpha=0.7)
    plt.plot([parent.get(xkey)],[parent.get(ykey)],'x',color='b')
    if ellipses:
        drawEllipses(parent, ec='b', fc='none', alpha=0.7)
    plt.axis(pax)



def footprintToImage(fp, mi=None):
    if mi is not None:
        fp = afwDet.makeHeavyFootprint(fp, mi)
    else:
        fp = afwDet.cast_HeavyFootprintF(fp)
    bb = fp.getBBox()
    im = afwImage.ImageF(bb.getWidth(), bb.getHeight())
    im.setXY0(bb.getMinX(), bb.getMinY())
    fp.insert(im)
    return im

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

def cutCatalog(cat, ndeblends, keepids=None):
    fams = getFamilies(cat)
    if keepids:
        print 'Keeping ids:', keepids
        print 'parent ids:', [p.getId() for p,kids in fams]
        fams = [(p,kids) for (p,kids) in fams if p.getId() in keepids]
    if ndeblends:
        # We want to select the first "ndeblends" parents and all their children.
        fams = fams[:ndeblends]
        
    keepcat = afwTable.SourceCatalog(cat.getTable())
    for p,kids in fams:
        keepcat.append(p)
        for k in kids:
            keepcat.append(k)
    keepcat.sort()
    return keepcat

def readCatalog(sourcefn, heavypat, ndeblends=0, dataref=None, keepids=None,
                patargs=dict()):
    if sourcefn is None:
        cat = dataref.get('src')
        assert(cat)
    else:
        if not os.path.exists(sourcefn):
            print 'No source catalog:', sourcefn
            return None
        print 'Reading catalog:', sourcefn
        cat = afwTable.SourceCatalog.readFits(sourcefn)
        print len(cat), 'sources'
    cat.sort()

    if ndeblends or keepids:
        cat = cutCatalog(cat, ndeblends, keepids)
        print 'Cut to', len(cat), 'sources'

    print 'Reading heavyFootprints...'
    for src in cat:
        if not src.getParent():
            continue
        dd = patargs.copy()
        dd.update(id=src.getId())
        heavyfn = heavypat % dd
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

def getDataref(visit=126969, ccd=5, single=True):
    butler = getButler()
    print 'Butler', butler
    dataRef = butler.subset('raw', dataId = dict(visit=visit, ccd=ccd))
    print 'dataRef:', dataRef
    print 'len(dataRef):', len(dataRef)
    for dr in dataRef:
        print '  ', dr
    if single:
        assert(len(dataRef) == 1)
        # dataRef doesn't support indexing, but it does support iteration?
        dr = None
        for dr in dataRef:
            break
        assert(dr)
        return dr
    return dataRef

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

def get_sigma1(mi):
    stats = afwMath.makeStatistics(mi.getVariance(), mi.getMask(), afwMath.MEDIAN)
    sigma1 = math.sqrt(stats.getValue(afwMath.MEDIAN))
    return sigma1


