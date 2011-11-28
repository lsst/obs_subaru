import matplotlib
matplotlib.use('Agg')
import optparse
import pyfits

#from lsst.meas.deblender import deblender
from lsst.meas.deblender import naive as naive_deblender
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.pex.policy as pexPolicy
import lsst.meas.algorithms as measAlg

import pylab as plt
import numpy as np

def afwimgtonumpy(I, x0=0, y0=0, W=None,H=None):
    if W is None:
        W = I.getWidth()
    if H is None:
        H = I.getHeight()
    img = np.zeros((H,W))
    imbb = I.getBBox(afwImage.PARENT)
    imx0,imy0 = imbb.getMinX(), imbb.getMinY()
    for ii in range(H):
        for jj in range(W):
            if imbb.contains(afwGeom.Point2I(jj+x0, ii+y0)):
                img[ii, jj] = I.get(jj+x0-imx0, ii+y0-imy0)
    return img

def footprintsFromPython(pyfoots):
    import lsst.afw.detection as afwDet

    #fplist = afwDet.FootprintList()
    fplist = afwDet.FootprintContainerT()
    #pklist = afwDet.PeakContainerT()
    pklist = []
    for bb,pks,spans in pyfoots:
        fp = afwDet.Footprint()
        for (x0,x1,y) in spans:
            fp.addSpan(y,x0,x1)
        # UMMM... no way to set Peaks in a Footprint?!
        #
        thispks = []
        for fx,fy in pks:
            thispks.append(afwDet.Peak(fx,fy))
        pklist.append(thispks)
        
        fplist.push_back(fp)
    return fplist, pklist


def footprintsToPython(fps, keepbb=None):
    pyfoots = []
    if keepbb is not None:
        x0,y0 = keepbb.getMinX(), keepbb.getMinY()
    else:
        x0,y0 = 0,0
    for f in fps:
        bbox = f.getBBox()
        if keepbb is not None:
            if not bbox.overlaps(keepbb):
                continue
        #bb = (bbox.getMinX(), bbox.getMinY(), bbox.getMaxX(), bbox.getMaxY())
        bb = (bbox.getMinX()-x0, bbox.getMinY()-y0, bbox.getMaxX()-x0, bbox.getMaxY()-y0)
        pks = []
        for p in f.getPeaks():
            #pks.append((p.getFx(), p.getFy()))
            pks.append((p.getFx()-x0, p.getFy()-y0))
        spans = []
        for s in f.getSpans():
            #spans.append((s.getX0(), s.getX1(), s.getY()))
            spans.append((s.getX0()-x0, s.getX1()-x0, s.getY()-y0))
        pyfoots.append((bb, pks, spans))
    return pyfoots


def testDeblend(foots, pks, mi, psf):

    I = afwimgtonumpy(mi.getImage())
    print 'numpy image:', I

    flatpks = []
    for pklist in pks:
        flatpks += pklist
    print 'flat peaks:', flatpks
    print len(flatpks), 'peaks'

    print 'Plotting...'
    plt.clf()
    plt.imshow(I, origin='lower', interpolation='nearest',
               vmin=-50, vmax=400)
    ax = plt.axis()
    plt.gray()
    plt.plot([pk.getFx() for pk in flatpks], [pk.getFy() for pk in flatpks], 'r.')
    plt.axis(ax)
    fn = 'test-srcs.png'
    plt.savefig(fn)
    print 'saved plot', fn

    bb = foots[0].getBBox()
    xc = int((bb.getMinX() + bb.getMaxX()) / 2.)
    yc = int((bb.getMinY() + bb.getMaxY()) / 2.)

    if not hasattr(psf, 'getFwhm'):
        pa = measAlg.PsfAttributes(psf, xc, yc)
        psfw = pa.computeGaussianWidth(measAlg.PsfAttributes.ADAPTIVE_MOMENT)
        print 'PSF width:', psfw
        psf_fwhm = 2.35 * psfw
    else:
        psf_fwhm = psf.getFwhm(xc, yc)
        
    if False:
        print 'Calling deblender...'
        objs = deblender.deblend(foots, pks, mi, psf)
        print 'got', objs
        for obj in objs:
            print 'Object:'
            print obj

    if True:
        print 'Calling naive deblender...'

        ### HACK
        print 'ONLY LOOKING AT LAST FOOTPRINT'
        foots = [foots[-1]]
        pks = [pks[-1]]

        results = naive_deblender.deblend(foots, pks, mi, psf, psf_fwhm)

        print
        print 'deblender finished'
        print

        for i,(foot,fpres) in enumerate(zip(foots,results)):
            sumP = None

            W,H = foot.getBBox().getWidth(), foot.getBBox().getHeight()
            x0,y0 = foot.getBBox().getMinX(), foot.getBBox().getMinY()
            I = afwimgtonumpy(mi.getImage(), x0, y0, W, H)
            ss = np.sort(I.ravel())
            mn,mx = [ss[int(p*len(ss))] for p in [0.1, 0.95]]
            print 'mn,mx', mn,mx

            plt.clf()
            plt.imshow(I, origin='lower', interpolation='nearest',
                       vmin=-50, vmax=400)
            plt.gray()
            plt.savefig('test-foot%03i.png' % i)

            #I_nopsf = afwimgtonumpy(mi.getImage(), x0, y0, W, H)
            I_nopsf = I.copy()
            pks_notpsf = []
            pks_psf = []

            ima = dict(interpolation='nearest', origin='lower', vmin=mn, vmax=mx)
            imb = dict(interpolation='nearest', origin='lower')

            # build the not-psf source and peak image.
            for j,pkres in enumerate(fpres.peaks):
                pk = pks[i][j]
                print 'peak:', pk
                print 'x,y', pk.getFx(), pk.getFy()
                #if not hasattr(pkres, 'timg'):
                #    # probably out-of-bounds
                #    continue
                if hasattr(pkres, 'deblend_as_psf') and pkres.deblend_as_psf:
                #if pkres.deblend_as_psf:
                    #mm = pkres.model
                    mm = pkres.psfderivimg
                    MM = afwimgtonumpy(mm)
                    mx0,my0 = mm.getX0(), mm.getY0()
                    mW,mH = mm.getWidth(), mm.getHeight()
                    print 'Peak', j, 'deblended as PSF.'
                    #plt.clf()
                    #plt.subplot(1,2,1)
                    #plt.imshow(I_nopsf
                    I_nopsf[my0:my0+mH, mx0:mx0+mW] -= MM
                    pks_psf.append(pk)
                else:
                    pks_notpsf.append(pk)

            imbb = mi.getBBox(afwImage.PARENT)
            imx0,imy0 = imbb.getMinX(), imbb.getMinY()

            plt.clf()
            plt.imshow(I_nopsf, origin='lower', interpolation='nearest',
                       vmin=-50, vmax=400)
            ax = plt.axis()
            plt.gray()

            dx = x0 - imx0
            dy = y0 - imy0
            print 'dx,dy', dx,dy
            plt.plot([pk.getFx()-dx for pk in pks_notpsf], [pk.getFy()-dy for pk in pks_notpsf], 'r.')
            plt.plot([pk.getFx()-dx for pk in pks_psf],    [pk.getFy()-dy for pk in pks_psf],    'g+')
            plt.axis(ax)
            plt.savefig('test-foot%03i-notpsf.png' % i)
            

            for j,pkres in enumerate(fpres.peaks):
                if not hasattr(pkres, 'timg'):
                    # probably out-of-bounds
                    continue
                timg = pkres.timg
                #templ.writeFits('templ-f%i-t%i.fits' % (i, j))
                pk = pks[i][j]
                print 'peak:', pk
                print 'x,y', pk.getFx(), pk.getFy()
                print 'footprint x0,y0', x0,y0
                xs0,ys0 = pkres.stampxy0
                print 'stamp xy0', xs0, ys0
                xs1,ys1 = pkres.stampxy1
                cx,cy = pkres.center

                T = afwimgtonumpy(pkres.timg)
                P = afwimgtonumpy(pkres.portion)
                S = afwimgtonumpy(pkres.stamp)
                PSF = afwimgtonumpy(pkres.psfimg)
                M = afwimgtonumpy(pkres.model)

                ss = np.sort(S.ravel())
                cmn,cmx = [ss[int(p*len(ss))] for p in [0.1, 0.99]]
                imc = dict(interpolation='nearest', origin='lower', vmin=cmn, vmax=cmx)
                d = (cmx-cmn)/2.
                imd = dict(interpolation='nearest', origin='lower', vmin=-d, vmax=d)

                NR,NC = 2,4
                plt.clf()

                plt.subplot(NR,NC,1)
                plt.imshow(I, **ima)
                ax = plt.axis()
                plt.plot([pk.getFx()], [pk.getFy()], 'r+')
                plt.plot([xs0,xs0,xs1,xs1,xs0], [ys0,ys1,ys1,ys0,ys0], 'r-')
                plt.axis(ax)
                plt.title('Image')
                #plt.colorbar()

                plt.subplot(NR,NC,2)
                plt.imshow(T, **ima)
                plt.title('Template')

                plt.subplot(NR,NC,3)
                plt.imshow(P, **ima)
                plt.title('Flux portion')

                plt.subplot(NR,NC,5)
                plt.imshow(S, **imc)
                ax = plt.axis()
                plt.plot([pk.getFx() - xs0], [pk.getFy() - ys0], 'r+')
                plt.axis(ax)
                plt.title('near-peak cutout')

                plt.subplot(NR,NC,6)
                plt.imshow(M, **imc)
                plt.title('PSF model: chisq/dof=%.2f' % (pkres.chisq/pkres.dof))

                plt.subplot(NR,NC,8)
                plt.imshow(S-M, **imd)
                plt.title('resids')

                plt.savefig('templ-f%i-t%i.png' % (i,j))

                if sumP is None:
                    sumP = P
                else:
                    sumP += P

            if sumP is not None:
                plt.clf()
                NR,NC = 1,2
                plt.subplot(NR,NC,1)
                plt.imshow(I, **ima)
                plt.subplot(NR,NC,2)
                plt.imshow(sumP, **ima)
                plt.savefig('sump-f%i.png' % i)


    

'''
class PsfDuck(object):
    def __init__(self, psf):
        self.psf = psf
    def computeImage(self, ccdxy, sz):
        pass
'''

class OffsetPsf(object):
    def __init__(self, psf, x0=0, y0=0):
        self.psf = psf
        self.x0 = int(x0)
        self.y0 = int(y0)
    def computeImage(self, ccdxy, sz=None):
        xy = afwGeom.Point2D(ccdxy)
        print 'psf computeimage xy:', xy
        xy.shift(afwGeom.Extent2D(float(self.x0), float(self.y0)))
        args = []
        if sz is not None:
            args.append(sz)
        psfimg = self.psf.computeImage(xy, *args)
        ix0 = psfimg.getX0()
        iy0 = psfimg.getY0()
        print 'ix0,iy0', ix0,iy0
        psfimg.setXY0(ix0-self.x0, iy0-self.y0)
        return psfimg
    def getFwhm(self, x, y):
        pa = measAlg.PsfAttributes(self.psf, self.x0 + x, self.y0 + y)
        psfw = pa.computeGaussianWidth(measAlg.PsfAttributes.ADAPTIVE_MOMENT)
        return 2.35 * psfw
        

if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option('--image', dest='imgfn', help='Image filename')
    parser.add_option('--psf', dest='psffn', help='PSF filename')
    parser.add_option('--psfx0', dest='psfx0', type=int, help='PSF offset x', default=0)
    parser.add_option('--psfy0', dest='psfy0', type=int, help='PSF offset y', default=0)
    parser.add_option('--footprints', dest='footfn', help='Read footprints from this python pickle file')
    parser.add_option('--sources', dest='srcfn', help='Source filename')
    
    opt,args = parser.parse_args()

    img = afwImage.ExposureF(opt.imgfn)

    print 'img xy0', img.getX0(), img.getY0()
    # 690, 2090
    img.setXY0(afwGeom.Point2I(0,0))


    print img.getWidth(), img.getHeight()
    #psfimg = pyfits.open(opt.psffn)[1].data

    if True:
        from lsst.daf.persistence import StorageList, LogicalLocation, ReadProxy
        from lsst.daf.persistence import Butler, Mapper, Persistence
        from lsst.daf.persistence import ButlerLocation
        import lsst.daf.base as dafBase

        storageType = 'BoostStorage'
        cname = 'lsst.afw.detection.Psf'
        pyname = 'Psf'
        path = opt.psffn
        dataId = {}

        loc = LogicalLocation(opt.psffn)
        storageList = StorageList()
        additionalData = dafBase.PropertySet()
        persistence = Persistence.getPersistence(pexPolicy.Policy())
        storageList.append(persistence.getRetrieveStorage(storageType, loc))
        obj = persistence.unsafeRetrieve("Psf", storageList, additionalData)
        print obj
        psf = afwDet.Psf.swigConvert(obj)
        print 'psf', psf
        
        psfimg = psf.computeImage()
        print 'Natural size:', psfimg.getWidth(), psfimg.getHeight()

        psf = OffsetPsf(psf, opt.psfx0, opt.psfy0)


    if opt.footfn:
        import cPickle
        f = open(opt.footfn, 'rb')
        pyfp = cPickle.load(f)
        # necessary?
        f.close()

        foots,pks = footprintsFromPython(pyfp)
        #print 'Got footprints:', foots
        #print 'Got peaks:', pks
        print 'Got', len(foots), 'footprints'
        print 'Got', len(pks), 'peaks'
        
    elif opt.srcfn:
        srcs = pyfits.open(opt.srcfn)[1].data
        x = srcs.field('x').astype(float)
        y = srcs.field('y').astype(float)
        # FITS to LSST
        x -= 1
        y -= 1
        print 'source x range', x.min(),x.max()
        print '       y range', y.min(),y.max()
        # [0,250]

        # single footprint for the whole image.
        bb = afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(img.getWidth(), img.getHeight()))
        pks = list(zip(x,y))
        spans = [(0, img.getWidth()-1, yi) for yi in range(img.getHeight())]
        foots,pks = footprintsFromPython([(bb,pks,spans)])
        print 'Footprints', foots
        print 'foot:', foots[0]
        print foots[0].getBBox()
        print 'Peaks:', pks

    else:
        assert(False)

    mi = img.getMaskedImage()
    print 'MI xy0', mi.getX0(), mi.getY0()

    testDeblend(foots, pks, mi, psf)
    
