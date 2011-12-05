import math
import numpy as np

import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.afw.geom  as afwGeom
import lsst.afw.math  as afwMath

class PerFootprint(object):
    pass
class PerPeak(object):
    def __init__(self):
        self.out_of_bounds = False
        self.deblend_as_psf = False

#def psfFit(psfimg, R0, R1, stamp, xlo,xhi,ylo,yhi,
#           otherpsfs, dodxdy):

def deblend(footprints, peaks, maskedImage, psf, psffwhm):
    print 'Baseline deblender starting'

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
    results = []
    for fp,pks in zip(footprints,peaks):
        fpres = PerFootprint()
        results.append(fpres)
        fpres.peaks = []
        for pki,pk in enumerate(pks):
            pkres = PerPeak()
            fpres.peaks.append(pkres)

    # First find peaks that are well-fit by a PSF + background model.
    for fp,pks,fpres in zip(footprints,peaks,results):

        fbb = fp.getBBox()
        assert(imbb.contains(fbb))

        for pki,(pk,pkres) in enumerate(zip(pks, fpres.peaks)):

            print
            print 'Peak', pki

            # Fit a PSF + smooth background model (linear) to a 
            # small region around the peak
            # The small region is a disk out to R0, plus a ramp with decreasing weight
            # down to R1.
            R0 = int(math.ceil(psffwhm * 1.))
            # ramp down to zero weight at this radius...
            R1 = int(math.ceil(psffwhm * 1.5))
            S = 2 * R1
            #print 'R0', R0, 'R1', R1, 'S = ', S
            cx,cy = pk.getFx(), pk.getFy()
            psfimg = psf.computeImage(afwGeom.Point2D(cx, cy))
            # R2: distance to neighbouring peak in order to put it
            # into the model
            R2 = R1 + psfimg.getWidth()/2.

            pbb = psfimg.getBBox(afwImage.PARENT)
            pbb.clip(fp.getBBox())
            px0,py0 = psfimg.getX0(), psfimg.getY0()

            # The bounding-box of the local region we are going to fit ("stamp")
            xlo = int(math.floor(cx - R1))
            ylo = int(math.floor(cy - R1))
            xhi = int(math.ceil (cx + R1))
            yhi = int(math.ceil (cy + R1))
            stampbb = afwGeom.Box2I(afwGeom.Point2I(xlo,ylo), afwGeom.Point2I(xhi,yhi))
            stampbb.clip(fbb)
            stamp = img.Factory(img, stampbb, afwImage.PARENT, True)
            assert(stamp.getBBox(afwImage.PARENT) == stampbb)

            xlo,xhi = stampbb.getMinX(), stampbb.getMaxX()
            ylo,yhi = stampbb.getMinY(), stampbb.getMaxY()
            if xlo >= xhi or ylo >= yhi:
                print 'Skipping this one'
                pkres.out_of_bounds = True
                continue
            
            # find other peaks within range...
            otherpsfs = []
            for j,pk2 in enumerate(pks):
                if j == pki:
                    continue
                if pk.getF().distanceSquared(pk2.getF()) > R2**2:
                    continue
                opsfimg = psf.computeImage(pk2.getF())
                otherpsfs.append(opsfimg)
            print len(otherpsfs), 'other peaks within range'

            # Number of terms -- PSF flux, constant, X, Y, + other PSF fluxes
            NT1 = 4 + len(otherpsfs)
            # + PSF dx, dy
            NT2 = NT1 + 2
            # Number of pixels -- at most
            NP = (1 + yhi - ylo) * (1 + xhi - xlo)

            # indices of terms
            I_psf = 0
            # start of other psf fluxes
            I_opsf = 4
            I_dx = NT1 + 0
            I_dy = NT1 + 1

            A = np.zeros((NP, NT2))
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
                        stamp.set0(x, y, 0.)
                        continue
                    xy = afwGeom.Point2I(x,y)
                    # in psf bbox?
                    if pbb.contains(xy):
                        p = psfimg.get0(x,y)
                    else:
                        p = 0

                    if (pbb.contains(afwGeom.Point2I(x-1,y)) and
                        pbb.contains(afwGeom.Point2I(x+1,y))):
                        dx = (psfimg.get0(x+1,y) - psfimg.get0(x-1,y)) / 2.
                    else:
                        dx = 0.
                        
                    if (pbb.contains(afwGeom.Point2I(x,y-1)) and
                        pbb.contains(afwGeom.Point2I(x,y+1))):
                        dy = (psfimg.get0(x,y+1) - psfimg.get0(x,y-1)) / 2.
                    else:
                        dy = 0.

                    rw = 1.
                    # ramp down weights from R0 to R1.
                    if R > R0:
                        rw = (R - R0) / (R1 - R0)

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
                        if obbox.contains(afwGeom.Point2I(x,y)):
                            A[ipix, I_opsf + j] = opsf.get0(x, y)

                    b[ipix] = img.get0(x, y)
                    # ramp weight * ( 1 / image variance )
                    w[ipix] = np.sqrt(rw / varimg.get0(x,y))
                    sumr += rw
                    ipix += 1

            # actual number of pixels within range
            NP = ipix
            A = A[:NP,:]
            b = b[:NP]
            w = w[:NP]
            ipixes = ipixes[:NP,:]
            Aw  = A * w[:,np.newaxis]
            bw  = b * w

            X1,r1,rank1,s1 = np.linalg.lstsq(Aw[:,:NT1], bw)
            X2,r2,rank2,s2 = np.linalg.lstsq(Aw, bw)
            print 'X1', X1
            print 'X2', X2
            print 'ranks', rank1, rank2
            print 'r', r1, r2
            # r is weighted chi-squared = sum_pix ramp * (model - data)**2/sigma**2
            if len(r1) > 0:
                chisq1 = r1[0]
            else:
                chisq1 = 1e30
            if len(r2) > 0:
                chisq2 = r2[0]
            else:
                chisq2 = 1e30

            dof1 = sumr - len(X1)
            dof2 = sumr - len(X2)
            # could fail...
            assert(dof1 > 0)
            assert(dof2 > 0)
            print 'Chi-squareds', chisq1, chisq2
            print 'Degrees of freedom', dof1, dof2
            q1 = chisq1/dof1
            q2 = chisq2/dof2
            print 'chisq/dof =', q1,q2

            # MAGIC number
            ispsf1 = (q1 < 1.5)
            ispsf2 = (q2 < 1.5)

            # check that the fit PSF spatial derivative terms aren't too big
            if ispsf2:
                fdx, fdy = X2[I_dx], X2[I_dy]
                f0 = X2[I_psf]
                # as a fraction of the PSF flux
                dx = fdx/f0
                dy = fdy/f0
                print 'isPSF -- check derivatives.'
                #print 'fdx,fdy', fdx,fdy, 'vs f0', f0
                print '  dx,dy', dx,dy
                ispsf2 = ispsf2 and (abs(dx) < 1. and abs(dy) < 1.)
                print '  -> still ok?', ispsf2
            if ispsf2:
                # shift the PSF by that amount and re-evaluate it.
                psfimg2 = psf.computeImage(afwGeom.Point2D(cx + dx, cy + dy))
                # clip
                pbb2 = psfimg2.getBBox(afwImage.PARENT)
                pbb2.clip(fbb)
                print 'psfimg2', psfimg2
                print 'pbb2', pbb2
                psfimg2 = psfimg2.Factory(psfimg2, pbb2, afwImage.PARENT)
                # yuck!
                Ab = A[:,:NT1].copy()
                for ipix,(x,y) in enumerate(ipixes):
                    xi,yi = int(x)+xlo, int(y)+ylo
                    if pbb2.contains(afwGeom.Point2I(xi,yi)):
                        p = psfimg2.get0(xi,yi)
                    else:
                        p = 0
                    Ab[ipix,I_psf] = p
                Aw  = Ab * w[:,np.newaxis]
                Xb,rb,rankb,sb = np.linalg.lstsq(Aw, bw)
                print 'Xb', Xb
                if len(rb) > 0:
                    chisqb = rb[0]
                else:
                    chisqb = 1e30
                print 'chisq', chisqb
                dofb = sumr - len(Xb)
                print 'dof', dofb
                qb = chisqb / dofb
                print 'chisq/dof', qb
                ispsf2 = (qb < 1.5)
                q2 = qb
                X2 = Xb


            # Which one do we keep?
            if (((ispsf1 and ispsf2) and (q2 < q1)) or
                (ispsf2 and not ispsf1)):
                Xpsf = X2
                chisq = chisq2
                dof = dof2
            else:
                # (arbitrarily set to X1 when neither fits well)
                Xpsf = X1
                chisq = chisq1
                dof = dof1
            ispsf = (ispsf1 or ispsf2)
                
            SW,SH = 1+xhi-xlo, 1+yhi-ylo
            psfmod = afwImage.ImageF(SW,SH)
            psfmod.setXY0(xlo,ylo)
            psfderivmod = afwImage.ImageF(SW,SH)
            psfderivmod.setXY0(xlo,ylo)
            model = afwImage.ImageF(SW,SH)
            model.setXY0(xlo,ylo)

            psfm2 = np.zeros((SH,SW))
            m2 = np.zeros((SH,SW))
            #print 'ipixes shape', ipixes.shape
            #print 'A shape', A.shape
            #print 'X1 shape', X1.shape
            #print 'X2 shape', X2.shape
            for i in range(len(Xpsf)):
                m2[ipixes[:,1],ipixes[:,0]] += A[:,i] * Xpsf[i]
            for i in [I_psf, I_dx, I_dy]:
                if i < len(Xpsf):
                    psfm2[ipixes[:,1],ipixes[:,0]] += A[:,i] * Xpsf[i]

            for y in range(ylo, yhi+1):
                for x in range(xlo, xhi+1):
                    model.set0(x, y, m2[y-ylo,x-xlo])
                    psfderivmod.set0(x, y, psfm2[y-ylo,x-xlo])
            for ii in range(NP):
                x,y = ipixes[ii,:]
                psfmod.set(int(x),int(y), float(A[ii, I_psf] * Xpsf[I_psf]))

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
            pkres.X = Xpsf
            pkres.otherpsfs = len(otherpsfs)
            pkres.w = w
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
        x1,y1 = bb.getMaxX(), bb.getMaxY()
        print 'Creating templates for footprint at x0,y0', x0,y0
        print 'W,H', W,H
        for pki,(pk,pkres) in enumerate(zip(pks, fpres.peaks)):
            if pkres.out_of_bounds:
                print 'Skipping out-of-bounds peak', pki
                continue
            if pkres.deblend_as_psf:
                print 'Skipping PSF peak', pki
                continue
            print 'computing template for peak', pki

            #print 'creating heavy footprint...'
            #heavy = afwDet.makeHeavyFootprint(fp, img)
            #print 'n peaks:', len(heavy.getPeaks())

            template = afwImage.MaskedImageF(W,H)
            template.setXY0(x0,y0)
            cx,cy = pk.getIx(), pk.getIy()
            print 'cx,cy =', cx,cy
            if not imbb.contains(afwGeom.Point2I(cx,cy)):
                print 'center is not inside image; skipping'
                pkres.out_of_bounds = True
                continue
            timg = template.getImage()
            p = img.get0(cx,cy)
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

                        pa = img.get0(xa, ya)
                        pb = img.get0(xb, yb)

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
            if pkres.out_of_bounds:
                print 'Skipping out-of-bounds peak', pki
                continue
            timg = pkres.timg
            timgs.append(timg)
            #if timg.getWidth() != W or timg.getHeight() != H:
            #    import pdb; pdb.set_trace()

            # NOTE! portions, timgs and the footprint are _currently_ all the same size, 
            p = afwImage.ImageF(W,H)
            p.setXY0(timg.getXY0())
            portions.append(p)
            pkres.portion = p

        print 'apportioning among', len(timgs), 'templates'
        for y in range(y0,y1+1):
            for x in range(x0,x1+1):
                impt = afwGeom.Point2I(x, y)
                if not imbb.contains(impt):
                    continue

                tvals = []
                for t in timgs:
                    tbb = t.getBBox(afwImage.PARENT)
                    tx0,ty0 = t.getX0(), t.getY0()
                    if tbb.contains(impt):
                        tvals.append(t.get(x-tx0,y-ty0))
                    else:
                        tvals.append(0)
                #S = sum([max(0, t) for t in tvals])
                S = sum([abs(t) for t in tvals])
                if S == 0:
                    continue
                impix = img.get0(x, y)
                for t,p in zip(tvals,portions):
                    #p.set(x,y, img.get(x0+x, y0+y) * max(0,tvals[i])/S)
                    if t != 0:
                        p.set0(x, y, impix * abs(t)/S)

    return results
