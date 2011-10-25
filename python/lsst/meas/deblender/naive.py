import math

import lsst.afw.image as afwImage
import lsst.afw.geom  as afwGeom
import lsst.afw.math  as afwMath

class PerFootprint(object):
    pass
class PerPeak(object):
    pass

def deblend(footprints, peaks, maskedImage, psf, psffwhm):
    print 'Naive deblender starting'
    print 'footprints', footprints
    print 'maskedImage', maskedImage
    print 'psf', psf

    results = []

    img = maskedImage.getImage()
    for fp,pks in zip(footprints,peaks):
        bb = fp.getBBox()
        W,H = bb.getWidth(), bb.getHeight()
        x0,y0 = bb.getMinX(), bb.getMinY()

        fpres = PerFootprint()
        results.append(fpres)
        fpres.peaks = []

        print 'Footprint x0,y0', x0,y0
        print 'W,H', W,H
        #for pk in fp.getPeaks():
        for pk in pks:

            pkres = PerPeak()
            fpres.peaks.append(pkres)

            template = afwImage.MaskedImageF(W,H)
            template.setXY0(x0,y0)
            cx,cy = pk.getIx(), pk.getIy()
            timg = template.getImage()
            p = img.get(cx,cy)
            timg.set(cx - x0, cy - y0, p)
            for dy in range(H-(cy-y0)):
                for dx in range(-(cx-x0), W-(cx-x0)):
                    if dy == 0 and dx == 0:
                        continue
                    # twofold rotational symmetry
                    xa,ya = cx+dx, cy+dy
                    xb,yb = cx-dx, cy-dy
                    if not fp.contains(afwGeom.Point2I(xa, ya)):
                        continue
                    if not fp.contains(afwGeom.Point2I(xb, yb)):
                        continue
                    pa = img.get(xa, ya)
                    pb = img.get(xb, yb)
                    mn = min(pa,pb)
                    # Monotonic?
                    timg.set(xa - x0, ya - y0, mn)
                    timg.set(xb - x0, yb - y0, mn)

            pkres.timg = timg

        # Now try fitting a PSF + smooth background model to the template.
        # Smooth background = ....?
        # ... constant, linear, (quadratic?) terms with least-squares fit?

        for pk,pkres in zip(pks, fpres.peaks):
            print 'PSF FWHM', psffwhm
            R0 = int(math.ceil(psffwhm * 2.))
            R1 = int(math.ceil(psffwhm * 3.))
            #R1 = int(math.ceil(psffwhm * 4.))
            print 'R0', R0, 'R1', R1
            S = 2 * R1
            print 'S = ', S

            cx,cy = pk.getFx(), pk.getFy()
            psfimg = psf.computeImage(afwGeom.Point2D(cx, cy))
            #afwGeom.Extent2I(S, S))
            bbox = psfimg.getBBox(afwImage.PARENT)
            print 'PSF bbox X:', bbox.getMinX(), bbox.getMaxX()
            print 'PSF bbox Y:', bbox.getMinY(), bbox.getMaxY()
            bbox.clip(fp.getBBox())
            print 'clipped bbox X:', bbox.getMinX(), bbox.getMaxX()
            print 'clipped bbox Y:', bbox.getMinY(), bbox.getMaxY()
            px0,py0 = psfimg.getX0(), psfimg.getY0()
            print 'px0,py0', px0,py0

            #xlo,xhi = bbox.getMinX(),bbox.getMaxX()
            #ylo,yhi = bbox.getMinY(),bbox.getMaxY()
            xlo = int(math.floor(cx - R1))
            ylo = int(math.floor(cy - R1))
            xhi = int(math.ceil (cx + R1))
            yhi = int(math.ceil (cy + R1))

            bb = fp.getBBox()
            xlo = max(xlo, bb.getMinX())
            ylo = max(ylo, bb.getMinY())
            xhi = min(xhi, bb.getMaxX())
            yhi = min(yhi, bb.getMaxY())

            import numpy as np

            # Number of terms -- PSF, constant, X, Y
            NT = 4
            NT2 = 7
            # Number of pixels -- at most
            NP = (1 + yhi - ylo) * (1 + xhi - xlo)

            A = np.zeros((NP, NT))
            A2 = np.zeros((NP, NT2))
            b = np.zeros(NP)
            w = np.zeros(NP)
            ipix = 0
            for y in range(ylo, yhi+1):
                for x in range(xlo, xhi+1):
                    R = np.hypot(x - cx, y - cy)
                    if R > R1:
                        continue

                    # in psf bbox?
                    if not bbox.contains(afwGeom.Point2I(x,y)):
                        continue

                    rw = 1.
                    # ramp down weights from R0 to R1.
                    if R > R0:
                        rw = (R - R0) / (R1 - R0)

                    p = psfimg.get(x-px0,y-py0)
                    im = img.get(x, y)

                    A[ipix,0] = p
                    A[ipix,2] = x-cx
                    A[ipix,3] = y-cy
                    b[ipix] = im
                    ## FIXME -- include image variance!
                    w[ipix] = np.sqrt(rw)
                    
                    A2[ipix,4] = (x-cx)**2
                    A2[ipix,5] = (x-cx)*(y-cy)
                    A2[ipix,6] = (y-cy)**2

                    ipix += 1
            A[:,1] = 1.

            A2[:,:4] = A[:,:4]

            # actual number of pixels
            NP = ipix
            A = A[:NP,:]
            A2 = A2[:NP,:]
            b = b[:NP]
            w = w[:NP]
            print 'Npix', NP
            A  *= w[:,np.newaxis]
            A2 *= w[:,np.newaxis]
            b  *= w
            
            X,r,rank,s = np.linalg.lstsq(A, b)
            print 'X', X

            X2,r,rank,s = np.linalg.lstsq(A2, b)
            print 'X2', X2

            SW,SH = 1+xhi-xlo, 1+yhi-ylo
            print 'Stamp size', SW, SH
            stamp = afwImage.ImageF(SW,SH)
            psfmod = afwImage.ImageF(SW,SH)
            model = afwImage.ImageF(SW,SH)
            model2 = afwImage.ImageF(SW,SH)
            for y in range(ylo, yhi+1):
                for x in range(xlo, xhi+1):
                    R = np.hypot(x - cx, y - cy)
                    if R > R1:
                        continue

                    # psf
                    if not bbox.contains(afwGeom.Point2I(x,y)):
                        p = 0
                    else:
                        p = psfimg.get(x - px0, y - py0)
                    im = img.get(x, y)

                    stamp.set(x-xlo, y-ylo, im)
                    psfmod.set(x-xlo, y-ylo, p)
                    m = (p * X[0] +
                         1. * X[1] +
                         (x - cx) * X[2] +
                         (y - cy) * X[3])
                    model.set(x-xlo, y-ylo, m)
                    m = (p * X2[0] +
                         1. * X2[1] +
                         (x - cx) * X2[2] +
                         (y - cy) * X2[3] +
                         (x - cx)**2 * X2[4] +
                         (x - cx)*(y - cy) * X2[5] +
                         (y - cy)**2 * X2[6])
                    model2.set(x-xlo, y-ylo, m)

            pkres.R0 = R0
            pkres.R1 = R1
            pkres.stamp = stamp
            #pkres.psfimg = psfimg
            pkres.psfimg = psfmod
            pkres.model = model
            pkres.model2 = model2
            pkres.stampxy0 = xlo,ylo
            pkres.stampxy1 = xhi,yhi
            pkres.center = cx,cy
            
        # Now apportion flux according to the templates ?
        timgs = [pkres.timg for pkres in fpres.peaks]
        portions = [afwImage.ImageF(W,H) for t in timgs]
        for y in range(H):
            for x in range(W):
                tvals = [t.get(x,y) for t in timgs]
                #S = sum([max(0, t) for t in tvals])
                S = sum([abs(t) for t in tvals])
                if S == 0:
                    continue
                for t,p in zip(tvals,portions):
                    #p.set(x,y, img.get(x0+x, y0+y) * max(0,tvals[i])/S)
                    p.set(x,y, img.get(x0+x, y0+y) * abs(t)/S)

        for r,p in zip(fpres.peaks, portions):
            r.portion = p

    return results
