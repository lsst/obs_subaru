import optparse
import pyfits

from lsst.meas.deblender import deblender
from lsst.meas.deblender import naive as naive_deblender
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.pex.policy as pexPolicy
from tractorPreprocess import footprintsFromPython
import lsst.meas.algorithms as measAlg

import pylab as plt
import numpy as np

def afwimgtonumpy(I, x0=0, y0=0, W=None,H=None):
    if W is None:
        W = I.getWidth()
    if H is None:
        H = I.getHeight()
    img = np.zeros((H,W))
    for ii in range(H):
        for jj in range(W):
            img[ii, jj] = I.get(jj+x0, ii+y0)
    return img


def testDeblend(foots, pks, mi, psf):
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
        results = naive_deblender.deblend(foots, pks, mi, psf, psf_fwhm)

        for i,(foot,fpres) in enumerate(zip(foots,results)):
            #(foot,templs,ports,bgs,mods,mods2) in enumerate(zip(foots,allt,allp,allbg,allmod,allmod2)):
            sumP = None

            W,H = foot.getBBox().getWidth(), foot.getBBox().getHeight()
            x0,y0 = foot.getBBox().getMinX(), foot.getBBox().getMinY()
            I = afwimgtonumpy(mi.getImage(), x0, y0, W, H)
            #mn,mx = I.min(), I.max()
            #mn,mx = [np.percentile(I.ravel(), p) for p in [25,95]]
            ss = np.sort(I.ravel())
            mn,mx = [ss[int(p*len(ss))] for p in [0.1, 0.95]]
            print 'mn,mx', mn,mx
            
            ima = dict(interpolation='nearest', origin='lower', vmin=mn, vmax=mx)
            imb = dict(interpolation='nearest', origin='lower')

            for j,pkres in enumerate(fpres.peaks):
                #(templ,port,bg,mod,mod2) in enumerate(zip(templs,ports,bgs,mods,mods2)):
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

                NR,NC = 2,3
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

                #plt.subplot(NR,NC,4)
                #plt.imshow(B, **ima)
                #plt.title('Background-subtracted')

                #backgr = T - B
                #psfbg = backgr + M

                plt.subplot(NR,NC,4)
                plt.imshow(S, **imc)
                ax = plt.axis()
                plt.plot([pk.getFx() - xs0], [pk.getFy() - ys0], 'r+')
                plt.axis(ax)
                plt.title('near-peak cutout')
                #plt.imshow(psfbg, **ima)
                #plt.title('PSF+bg model')

                #plt.subplot(NR,NC,5)
                #plt.imshow(PSF, **imb)
                #plt.title('PSF model')

                plt.subplot(NR,NC,5)
                plt.imshow(M, **imc)
                plt.title('PSF model: chisq/dof=%.2f' % (pkres.chisq/pkres.dof))

                #import scipy.stats
                #pval = scipy.stats.chi2.sf(X2, dof)

                plt.subplot(NR,NC,6)
                plt.imshow(S-M, **imd)
                plt.title('resids')

                #plt.subplot(NR,NC,5)
                #res = B-M
                #mx = np.abs(res).max()
                #plt.imshow(res, interpolation='nearest', origin='lower', vmin=-mx, vmax=mx)
                #plt.colorbar()
                #plt.title('Residuals')

                # plt.subplot(NR,NC,6)
                # py = pks[i][j].getIy()
                # plt.plot(T[py-y0,:], 'r-')
                # #plt.plot(T[py-y0+1,:], 'b-')
                # #plt.plot(T[py-y0-1,:], 'g-')
                # plt.plot(psfbg[py-y0,:], 'b-')
                # plt.title('Template slices')

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
    parser.add_option('--sources', dest='srcfn', help='Source filename')
    
    opt,args = parser.parse_args()

    img = afwImage.ExposureF(opt.imgfn)

    print 'img xy0', img.getX0(), img.getY0()
    # 690, 2090
    img.setXY0(afwGeom.Point2I(0,0))

    srcs = pyfits.open(opt.srcfn)[1].data
    x = srcs.field('x').astype(float)
    y = srcs.field('y').astype(float)
    # FITS to LSST
    x -= 1
    y -= 1
    print 'source x range', x.min(),x.max()
    print '       y range', y.min(),y.max()
    # [0,250]

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


    # single footprint for the whole image.
    bb = afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(img.getWidth(), img.getHeight()))
    pks = list(zip(x,y))
    spans = [(0, img.getWidth()-1, yi) for yi in range(img.getHeight())]
    foots,pks = footprintsFromPython([(bb,pks,spans)])
    print 'Footprints', foots
    print 'foot:', foots[0]
    print foots[0].getBBox()
    print 'Peaks:', pks

    mi = img.getMaskedImage()
    print 'MI xy0', mi.getX0(), mi.getY0()
    plt.clf()
    I = afwimgtonumpy(mi.getImage())
    plt.imshow(I, origin='lower', interpolation='nearest',
               vmin=-50, vmax=400)
    flatpks = []
    for pklist in pks:
        flatpks += pklist
    ax = plt.axis()
    plt.gray()
    plt.plot([pk.getFx() for pk in flatpks], [pk.getFy() for pk in flatpks], 'r.')
    plt.axis(ax)
    plt.savefig('test-srcs.png')

    testDeblend(foots, pks, mi, psf)
    
