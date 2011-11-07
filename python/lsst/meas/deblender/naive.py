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
    varimg = maskedImage.getVariance()
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

        for pki,(pk,pkres) in enumerate(zip(pks, fpres.peaks)):
            print 'PSF FWHM', psffwhm
            # full-weight radius
            R0 = int(math.ceil(psffwhm * 1.))
            # ramp down to zero weight at this radius...
            R1 = int(math.ceil(psffwhm * 1.5))
            print 'R0', R0, 'R1', R1
            S = 2 * R1
            print 'S = ', S

            cx,cy = pk.getFx(), pk.getFy()
            psfimg = psf.computeImage(afwGeom.Point2D(cx, cy))

            # distance to neighbouring peak in order to put it in the model
            R2 = R1 + psfimg.getWidth()/2.

            bbox = psfimg.getBBox(afwImage.PARENT)
            print 'PSF bbox X:', bbox.getMinX(), bbox.getMaxX()
            print 'PSF bbox Y:', bbox.getMinY(), bbox.getMaxY()
            bbox.clip(fp.getBBox())
            print 'clipped bbox X:', bbox.getMinX(), bbox.getMaxX()
            print 'clipped bbox Y:', bbox.getMinY(), bbox.getMaxY()
            px0,py0 = psfimg.getX0(), psfimg.getY0()
            print 'px0,py0', px0,py0

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

            # find other peaks within range...
            otherpsfs = []
            for j,pk2 in enumerate(pks):
                if j == pki:
                    continue
                if (pk2.getFx() - cx)**2 + (pk2.getFy() - cy)**2 > R2**2:
                    continue
                psfimg = psf.computeImage(afwGeom.Point2D(pk2.getFx(), pk2.getFy()))
                otherpsfs.append(psfimg)
            print len(otherpsfs), 'other peaks within range'

            # Number of terms -- PSF, constant, X, Y, + other PSFs, + PSF dx, dy
            NT = 4 + len(otherpsfs) + 2
            # Number of pixels -- at most
            NP = (1 + yhi - ylo) * (1 + xhi - xlo)

            # indices of terms
            I_psf = 0
            I_dx = 4
            I_dy = 5

            A = np.zeros((NP, NT))
            b = np.zeros(NP)
            w = np.zeros(NP)
            ipixes = np.zeros((NP,2), dtype=int)
            ipix = 0
            sumr = 0.
            for y in range(ylo, yhi+1):
                for x in range(xlo, xhi+1):
                    R = np.hypot(x - cx, y - cy)
                    if R > R1:
                        continue

                    # in psf bbox?
                    if not bbox.contains(afwGeom.Point2I(x,y)):
                        p = 0
                        #continue
                    else:
                        p = psfimg.get(x-px0,y-py0)

                    if bbox.contains(afwGeom.Point2I(x-1,y)) and bbox.contains(afwGeom.Point2I(x+1,y)):
                        dx = (psfimg.get(x+1-px0,y-py0) - psfimg.get(x-1-px0,y-py0)) / 2.
                    else:
                        dx = 0.

                    if bbox.contains(afwGeom.Point2I(x,y-1)) and bbox.contains(afwGeom.Point2I(x,y+1)):
                        dy = (psfimg.get(x-px0,y+1-py0) - psfimg.get(x-px0,y-1-py0)) / 2.
                    else:
                        dy = 0.

                    rw = 1.
                    # ramp down weights from R0 to R1.
                    if R > R0:
                        rw = (R - R0) / (R1 - R0)

                    im = img.get(x, y)

                    A[ipix,I_psf] = p
                    A[ipix,1] = 1.
                    A[ipix,2] = x-cx
                    A[ipix,3] = y-cy
                    A[ipix,I_dx] = dx
                    A[ipix,I_dy] = dy
                    ipixes[ipix,:] = (x-xlo,y-ylo)
                    
                    for j,opsf in enumerate(otherpsfs):
                        obbox = opsf.getBBox(afwImage.PARENT)
                        obbox.clip(fp.getBBox())
                        opx0,opy0 = opsf.getX0(), opsf.getY0()
                        if obbox.contains(afwGeom.Point2I(x,y)):
                            A[ipix, 6+j] = opsf.get(x-opx0, y-opy0)

                    b[ipix] = im
                    # ramp weight * ( 1 / image variance )
                    w[ipix] = np.sqrt(rw / varimg.get(x,y))
                    sumr += rw
                    
                    ipix += 1

            # actual number of pixels
            NP = ipix
            A = A[:NP,:]
            b = b[:NP]
            w = w[:NP]
            ipixes = ipixes[:NP,:]
            print 'Npix', NP
            print 'sumr', sumr
            Aw  = A * w[:,np.newaxis]
            b  *= w

            print 'A shape', A.shape
            print 'Aw shape', Aw.shape

            X,r,rank,s = np.linalg.lstsq(Aw, b)
            print 'X', X
            print 'rank', rank
            print 'r', r
            # r is weighted chi-squared = sum_pix ramp * (model - data)**2/sigma**2

            chisq = r[0]
            dof = sumr - len(X)
            print 'Chi-squared', chisq
            print 'Degrees of freedom', dof
            if False:
                import scipy.stats
                pval = scipy.stats.chi2.sf(chisq, dof)
                print 'p value:', pval
            print 'chisq/dof =', chisq/dof

            SW,SH = 1+xhi-xlo, 1+yhi-ylo
            print 'Stamp size', SW, SH
            stamp = afwImage.ImageF(SW,SH)
            psfmod = afwImage.ImageF(SW,SH)
            psfderivmod = afwImage.ImageF(SW,SH)
            model = afwImage.ImageF(SW,SH)
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

            psfm2 = np.zeros((SH,SW))
            m2 = np.zeros((SH,SW))
            print 'ipixes shape', ipixes.shape
            print 'A shape', A.shape
            print 'X shape', X.shape
            for i in range(NT):
                m2[ipixes[:,1],ipixes[:,0]] += A[:,i] * X[i]
            for i in [I_psf, I_dx, I_dy]:
                psfm2[ipixes[:,1],ipixes[:,0]] += A[:,i] * X[i]
            for y in range(ylo, yhi+1):
                for x in range(xlo, xhi+1):
                    model.set(x-xlo, y-ylo, m2[y-ylo,x-xlo])
                    psfderivmod.set(x-xlo, y-ylo, psfm2[y-ylo,x-xlo])
            psfderivmod.setXY0(xlo,ylo)
            pkres.R0 = R0
            pkres.R1 = R1
            pkres.stamp = stamp
            #pkres.psfimg = psfimg
            pkres.psfimg = psfmod
            pkres.psfderivimg = psfderivmod
            pkres.model = model
            pkres.stampxy0 = xlo,ylo
            pkres.stampxy1 = xhi,yhi
            pkres.center = cx,cy

            pkres.chisq = chisq
            pkres.dof = dof

            # MAGIC
            ispsf = (chisq / dof) < 1.5
            pkres.deblend_as_psf = ispsf

            if ispsf:
                # replace the template image by the PSF + derivatives image.
                print 'Deblending as PSF; setting template to PSF (+ spatial derivatives) model'
                print 
                pkres.timg = psfderivmod
            
        # Now apportion flux according to the templates ?
        timgs = [pkres.timg for pkres in fpres.peaks]
        portions = [afwImage.ImageF(W,H) for t in timgs]
        for y in range(H):
            for x in range(W):
                #tvals = [t.get(x,y) for t in timgs]
                tvals = []
                pt = afwGeom.Point2I(x,y)
                for t in timgs:
                    tbb = t.getBBox(afwImage.PARENT)
                    tx0,ty0 = t.getX0(), t.getY0()
                    if tbb.contains(pt):
                        tvals.append(t.get(x-tx0,y-ty0))
                    else:
                        tvals.append(0)
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
