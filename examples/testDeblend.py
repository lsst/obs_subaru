import optparse
import pyfits

from lsst.meas.deblender import deblender
from lsst.meas.deblender import naive as naive_deblender
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.pex.policy as pexPolicy

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

    import lsst.meas.algorithms as measAlg
    bb = foots[0].getBBox()
    xc = int((bb.getMinX() + bb.getMaxX()) / 2.)
    yc = int((bb.getMinY() + bb.getMaxY()) / 2.)
    pa = measAlg.PsfAttributes(psf, xc, yc)
    psfw = pa.computeGaussianWidth(measAlg.PsfAttributes.ADAPTIVE_MOMENT)
    print 'PSF width:', psfw
    psf_fwhm = 2.35 * psfw

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
            mn,mx = I.min(), I.max()
            ima = dict(interpolation='nearest', origin='lower', vmin=mn, vmax=mx)

            for j,pkres in enumerate(fpres.peaks):
                #(templ,port,bg,mod,mod2) in enumerate(zip(templs,ports,bgs,mods,mods2)):
                timg = pkres.timg
                #templ.writeFits('templ-f%i-t%i.fits' % (i, j))

                T = afwimgtonumpy(pkres.timg)
                P = afwimgtonumpy(pkres.portion)
                S = afwimgtonumpy(pkres.stamp)
                M = afwimgtonumpy(pkres.model)
                M2 = afwimgtonumpy(pkres.model2)

                NR,NC = 2,3
                plt.clf()

                plt.subplot(NR,NC,1)
                plt.imshow(I, **ima)
                plt.title('Image')
                plt.colorbar()

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
                plt.imshow(S, **ima)
                plt.title('near-peak cutout')
                #plt.imshow(psfbg, **ima)
                #plt.title('PSF+bg model')

                plt.subplot(NR,NC,5)
                plt.imshow(M, **ima)
                plt.title('near-peak model')

                plt.subplot(NR,NC,6)
                plt.imshow(M2, **ima)
                plt.title('near-peak model (2)')

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
    


if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option('--image', dest='imgfn', help='Image filename')
    parser.add_option('--psf', dest='psffn', help='PSF filename')
    parser.add_option('--sources', dest='srcfn', help='Source filename')
    
    opt,args = parser.parse_args()

    img = afwImage.ExposureF(opt.imgfn)

    srcs = pyfits.open(opt.srcfn)[1].data
    x = srcs.field('x').astype(float)
    y = srcs.field('y').astype(float)

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
    print 'per:', persistence
    print 'per.getRetrieveStorage'
    print help(persistence.getRetrieveStorage)
    print type(storageType)
    print type(loc)
    storageList.append(persistence.getRetrieveStorage(storageType, loc))
    obj = persistence.unsafeRetrieve("Psf", storageList, additionalData)
    print obj



    perspol = pexPolicy.Policy()
    pers = Persistence.getPersistence(perspol)
    location = ButlerLocation(cname, pyname, storageName, path, dataId)
    additionalData = location.getAdditionalData()
    storageName = location.getStorageName()
    locations = location.getLocations()
    #logLoc = LogicalLocation(locationString, additionalData)

    print 'storageName', storageName
    results = []
    for locationString in locations:
        print 'location string', locationString
        logLoc = LogicalLocation(locationString, additionalData)
        print 'logLoc', logLoc
        storageList = StorageList()
        print 'pers', pers
        storage = pers.getRetrieveStorage(storageName, logLoc)
        print 'storage', storage
        storageList.append(storage)
        itemData = pers.unsafeRetrieve(location.getCppType(), storageList, additionalData)
        finalItem = pythonType.swigConvert(itemData)
        results.append(finalItem)

    print results

    afwDet.PsfFormatter(pexPolicy.Policy())
    #psf = afwDet.Psf(opt.psffn)    
