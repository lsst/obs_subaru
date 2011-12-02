import math
import numpy as np

import lsst.afw.image as afwImage
import lsst.afw.geom  as afwGeom
import lsst.afw.math  as afwMath

class PerFootprint(object):
    pass
class PerPeak(object):
    def __init__(self):
        self.out_of_bounds = False

def deblend(footprints, peaks, maskedImage, psf, psffwhm):
    print 'Naive deblender starting'
    print 'footprints', footprints
    print 'maskedImage', maskedImage
    print 'psf', psf

    results = []

    img = maskedImage.getImage()
    varimg = maskedImage.getVariance()

    mask = maskedImage.getMask()
    # find the median variance in the image...
    stats = afwMath.makeStatistics(varimg, mask, afwMath.MEDIAN)
    sigma1 = math.sqrt(stats.getValue(afwMath.MEDIAN))
    #print 'Median image sigma:', sigma1

    imbb = img.getBBox(afwImage.PARENT)
    print 'imbb (PARENT)', imbb.getMinX(), imbb.getMaxX(), imbb.getMinY(), imbb.getMaxY()

    # prepare results structures
    for fp,pks in zip(footprints,peaks):
        fpres = PerFootprint()
        results.append(fpres)
        fpres.peaks = []
        for pki,pk in enumerate(pks):
            pkres = PerPeak()
            fpres.peaks.append(pkres)


    # First find peaks that are well-fit by a PSF + background model.
    for fp,pks,fpres in zip(footprints,peaks,results):
        for pki,(pk,pkres) in enumerate(zip(pks, fpres.peaks)):
            # Fit a PSF + smooth background model (linear) to a 
            # small region around the peak
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
            # R2: distance to neighbouring peak in order to put it
            # into the model
            R2 = R1 + psfimg.getWidth()/2.

            bbox = psfimg.getBBox(afwImage.PARENT)
            #print 'PSF bbox X:', bbox.getMinX(), bbox.getMaxX()
            #print 'PSF bbox Y:', bbox.getMinY(), bbox.getMaxY()
            bbox.clip(fp.getBBox())
            #print 'clipped bbox X:', bbox.getMinX(), bbox.getMaxX()
            #print 'clipped bbox Y:', bbox.getMinY(), bbox.getMaxY()
            px0,py0 = psfimg.getX0(), psfimg.getY0()
            #print 'px0,py0', px0,py0
            xlo = int(math.floor(cx - R1))
            ylo = int(math.floor(cy - R1))
            xhi = int(math.ceil (cx + R1))
            yhi = int(math.ceil (cy + R1))
            bb = fp.getBBox()
            xlo = max(xlo, bb.getMinX())
            ylo = max(ylo, bb.getMinY())
            xhi = min(xhi, bb.getMaxX())
            yhi = min(yhi, bb.getMaxY())

            print 'xlo,xhi', xlo,xhi
            print 'ylo,yhi', ylo,yhi

            xlo = max(xlo, imbb.getMinX())
            xhi = min(xhi, imbb.getMaxX())
            ylo = max(ylo, imbb.getMinY())
            yhi = min(yhi, imbb.getMaxY())
            print 'xlo,xhi', xlo,xhi
            print 'ylo,yhi', ylo,yhi
            if xlo >= xhi or ylo >= yhi:
                print 'Skipping this one'
                pkres.out_of_bounds = True
                continue
            
            # find other peaks within range...
            otherpsfs = []
            for j,pk2 in enumerate(pks):
                if j == pki:
                    continue
                if (pk2.getFx() - cx)**2 + (pk2.getFy() - cy)**2 > R2**2:
                    continue
                opsfimg = psf.computeImage(afwGeom.Point2D(pk2.getFx(),
                                                           pk2.getFy()))
                otherpsfs.append(opsfimg)
            print len(otherpsfs), 'other peaks within range'

            # Number of terms -- PSF, constant, X, Y, + other PSFs,
            # + PSF dx, dy
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
            # pixel indices
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
                    else:
                        p = psfimg.get(x-px0,y-py0)

                    if (bbox.contains(afwGeom.Point2I(x-1,y)) and
                        bbox.contains(afwGeom.Point2I(x+1,y))):
                        dx = (psfimg.get(x+1-px0,y-py0) - psfimg.get(x-1-px0,y-py0)) / 2.
                    else:
                        dx = 0.

                    if (bbox.contains(afwGeom.Point2I(x,y-1)) and
                        bbox.contains(afwGeom.Point2I(x,y+1))):
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
                    # other nearby peak PSFs
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

            # actual number of pixels within range
            NP = ipix
            A = A[:NP,:]
            b = b[:NP]
            w = w[:NP]
            ipixes = ipixes[:NP,:]
            #print 'Npix', NP
            #print 'sumr', sumr
            Aw  = A * w[:,np.newaxis]
            bw  = b * w
            #print 'A shape', A.shape
            #print 'Aw shape', Aw.shape
            X,r,rank,s = np.linalg.lstsq(Aw, bw)
            print 'X', X
            print 'rank', rank
            print 'r', r
            # r is weighted chi-squared = sum_pix ramp * (model - data)**2/sigma**2
            if len(r) > 0:
                chisq = r[0]
            else:
                chisq = 1e30
            dof = sumr - len(X)
            print 'Chi-squared', chisq
            print 'Degrees of freedom', dof
            try:
                print 'chisq/dof =', chisq/dof
            except ZeroDivisionError, e:
                print "dof is zero!"
                dof = 1                 # CPL
                
            SW,SH = 1+xhi-xlo, 1+yhi-ylo
            print 'Stamp size', SW, SH
            stamp = afwImage.ImageF(SW,SH)
            psfmod = afwImage.ImageF(SW,SH)
            psfderivmod = afwImage.ImageF(SW,SH)
            model = afwImage.ImageF(SW,SH)
            # for y in range(ylo, yhi+1):
            #     for x in range(xlo, xhi+1):
            #         R = np.hypot(x - cx, y - cy)
            #         if R > R1:
            #             continue
            #         # psf
            #         if not bbox.contains(afwGeom.Point2I(x,y)):
            #             p = 0
            #         else:
            #             p = psfimg.get(x - px0, y - py0)
            #         im = img.get(x, y)
            #         stamp.set(x-xlo, y-ylo, im)
            #         psfmod.set(x-xlo, y-ylo, p)

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
            for ii in range(NP):
                x,y = ipixes[ii,:]
                #x,y 5 0 <type 'numpy.int64'> <type 'numpy.int64'> val -80.8212890625 <type 'numpy.float64'>
                #print 'x,y', x,y, type(x),type(y), 'val', b[ii], type(b[ii])
                stamp.set(int(x),int(y), float(b[ii]))
                psfmod.set(int(x),int(y), float(A[ii, I_psf]))

            psfderivmod.setXY0(xlo,ylo)
            model.setXY0(xlo,ylo)
            stamp.setXY0(xlo,ylo)
            psfmod.setXY0(xlo,ylo)

            pkres.R0 = R0
            pkres.R1 = R1
            pkres.stamp = stamp
            pkres.psf0img = psfimg
            pkres.psfimg = psfmod
            pkres.psfderivimg = psfderivmod
            pkres.model = model
            pkres.stampxy0 = xlo,ylo
            pkres.stampxy1 = xhi,yhi
            pkres.center = cx,cy
            pkres.chisq = chisq
            pkres.dof = dof
            pkres.X = X
            pkres.otherpsfs = len(otherpsfs)
            pkres.w = w

            # MAGIC number
            ispsf = ((chisq / dof) < 1.5)
            if ispsf:
                # check that the fit PSF spatial derivative terms
                # aren't too big
                dx,dy = X[I_dx],X[I_dy]
                print 'dx,dy', dx,dy
                ispsf = ispsf and (abs(dx) < 1. and abs(dy) < 1.)
            pkres.deblend_as_psf = ispsf

            if ispsf:
                # replace the template image by the PSF + derivatives
                # image.
                # FIXME -- replace the PSF + derivs by the shifted PSF?
                print 'Deblending as PSF; setting template to PSF (+ spatial derivatives) model'
                print 
                pkres.timg = psfderivmod


    print
    print 'Computing templates...'
    print

    for fp,pks,fpres in zip(footprints,peaks,results):
        print
        bb = fp.getBBox()
        W,H = bb.getWidth(), bb.getHeight()
        x0,y0 = bb.getMinX(), bb.getMinY()
        print 'Creating templates for footprint at x0,y0', x0,y0
        print 'W,H', W,H
        for pki,(pk,pkres) in enumerate(zip(pks, fpres.peaks)):
            if hasattr(pkres, 'out_of_bounds') and pkres.out_of_bounds:
                print 'Skipping out-of-bounds peak', pki
                continue
            if hasattr(pkres, 'deblend_as_psf') and pkres.deblend_as_psf:
                print 'Skipping PSF peak', pki
                continue
            print 'computing template for peak', pki
            template = afwImage.MaskedImageF(W,H)
            template.setXY0(x0,y0)
            cx,cy = pk.getIx(), pk.getIy()
            print 'cx,cy =', cx,cy
            if not imbb.contains(afwGeom.Point2I(cx,cy)):
                print 'center is not inside image; skipping'
                pkres.out_of_bounds = True
                continue
            timg = template.getImage()
            p = img.get(cx,cy)
            timg.set(cx - x0, cy - y0, p)

            # We do dy>=0; dy<0 is handled by symmetry.
            for dy in range(H-(cy-y0)):
                # Iterate symmetrically out from dx=0
                for absdx in range(min(cx-x0, W-(cx-x0))):
                    if dy == 0 and absdx == 0:
                        continue

                    # Monotonic constraint
                    # We're working out from the center, so just need to figure out which
                    # pixel is toward the center from where we are.
                    v2 = float(absdx**2 + dy**2)
                    if dy**2/v2 < 0.25:
                        hx = 1
                        hy = 0
                    elif dy**2/v2 > 0.75:
                        hx = 0
                        hy = 1
                    else:
                        hx = hy = 1

                    for signdx in [-1,1]:
                        # don't do dx=0 twice!
                        if absdx == 0 and signdx == 1:
                            continue
                        dx = absdx * signdx
                        # twofold rotational symmetry
                        xa,ya = cx+dx, cy+dy
                        xb,yb = cx-dx, cy-dy
                        if not fp.contains(afwGeom.Point2I(xa, ya)):
                            continue
                        if not fp.contains(afwGeom.Point2I(xb, yb)):
                            continue

                        if not imbb.contains(afwGeom.Point2I(xa, ya)):
                            continue
                        if not imbb.contains(afwGeom.Point2I(xb, yb)):
                            continue

                        pa = img.get(xa, ya)
                        pb = img.get(xb, yb)

                        # monotonic constraint
                        xm = xa - (hx * signdx)
                        ym = ya - hy
                        #print 'xa,ya', xa,ya
                        #print 'xb,yb', xb,yb
                        #print 'xm,ym', xm,ym
                        #assert(fp.contains(afwGeom.Point2I(xm,ym)))
                        if not fp.contains(afwGeom.Point2I(xm,ym)):
                            continue
                        mono = timg.get(xm-x0,ym-y0)

                        #mn = min(pa,pb)
                        #mn = min(mn, mono)
                        #pa = pb = mn
                        pa = min(pa, pb + sigma1)
                        pb = min(pb, pa + sigma1)
                        pa = min(pa, mono)
                        pb = min(pb, mono)
                        timg.set(xa - x0, ya - y0, pa)
                        timg.set(xb - x0, yb - y0, pb)

            pkres.timg = timg

        # Now apportion flux according to the templates ?
        print 'Apportioning flux...'
        timgs = []
        portions = []
        for pki,pkres in enumerate(fpres.peaks):
            if hasattr(pkres, 'out_of_bounds') and pkres.out_of_bounds:
                print 'Skipping out-of-bounds peak', pki
                continue
            timg = pkres.timg
            timgs.append(timg)
            if timg.getWidth() != W or timg.getHeight() != H:
                import pdb; pdb.set_trace()

            # NOTE! portions, timgs and the footprint are _currently_ all the same size, 
            p = afwImage.ImageF(W,H)
            p.setXY0(timg.getXY0())
            portions.append(p)
            pkres.portion = p

        print 'apportioning among', len(timgs), 'templates'
        for y in range(H):
            for x in range(W):
                impt = afwGeom.Point2I(x0+x, y0+y)
                if not imbb.contains(impt):
                    continue

                tvals = []
                for t in timgs:
                    tbb = t.getBBox(afwImage.PARENT)
                    tx0,ty0 = t.getX0(), t.getY0()
                    if tbb.contains(impt):
                        tvals.append(t.get(x+x0-tx0,y+y0-ty0))
                    else:
                        tvals.append(0)
                #S = sum([max(0, t) for t in tvals])
                S = sum([abs(t) for t in tvals])
                if S == 0:
                    continue
                impix = img.get(x0+x, y0+y)
                for t,p in zip(tvals,portions):
                    #p.set(x,y, img.get(x0+x, y0+y) * max(0,tvals[i])/S)
                    p.set(x,y, impix * abs(t)/S)

        #for r,p in zip(fpres.peaks, portions):
        #    r.portion = p

    return results
