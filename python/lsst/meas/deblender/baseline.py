import pylab as plt



import math
import numpy as np

import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.afw.geom  as afwGeom
import lsst.afw.math  as afwMath

# Result objects; will probably go away as we push more results
# into earlier-created Source objects, but useful for now for
# hauling debugging results around.
class PerFootprint(object):
    # .peaks: list of PerPeak objects
    pass
class PerPeak(object):
    def __init__(self):
        self.out_of_bounds = False
        self.deblend_as_psf = False

def deblend(footprint, maskedImage, psf, psffwhm,
            psf_chisq_cut1 = 1.5,
            psf_chisq_cut2 = 1.5,
            psf_chisq_cut2b = 1.5,
            fit_psfs = True,
            median_smooth_template=True,
            monotonic_template=True,
            lstsq_weight_templates=False,
            log=None, verbose=False,
            sigma1=None,
            ):
    '''
    Deblend a single ``footprint`` in a ``maskedImage``.

    Each ``Peak`` in the ``Footprint`` will produce a deblended child.

    psf_chisq_cut1, psf_chisq_cut2: used when deciding whether a given
    peak looks like a point source.  We build a small local model of
    background + peak + neighboring peaks.  These are the
    chi-squared-per-degree-of-freedom cuts (1=without and 2=with terms
    allowing the peak position to move); larger values will result in
    more peaks being classified as PSFs.

    psf_chisq_cut2b: if the with-decenter fit is accepted, we then
    apply the computed dx,dy to the source position and re-fit without
    decentering.  The model is accepted if its chi-squared-per-DOF is
    less than this value.
    '''
    # Import C++ routines
    import lsst.meas.deblender as deb
    butils = deb.BaselineUtilsF

    if log is None:
        import lsst.pex.logging as pexLogging
        loglvl = pexLogging.Log.INFO
        if verbose:
            loglvl = pexLogging.Log.DEBUG
        log = pexLogging.Log(pexLogging.Log.getDefaultLog(), 'meas_deblender.baseline',
                             loglvl)

    img = maskedImage.getImage()
    varimg = maskedImage.getVariance()
    mask = maskedImage.getMask()

    fp = footprint
    fp.normalize()
    peaks = fp.getPeaks()

    if sigma1 is None:
        # FIXME -- just within the bbox?
        stats = afwMath.makeStatistics(varimg, mask, afwMath.MEDIAN)
        sigma1 = math.sqrt(stats.getValue(afwMath.MEDIAN))

    # prepare results structure
    res = PerFootprint()
    res.peaks = []
    for pki,pk in enumerate(peaks):
        pkres = PerPeak()
        pkres.peak = pk
        pkres.pki = pki
        res.peaks.append(pkres)

    #print 'Initial mask planes:'
    #mask.printMaskPlanes()
    for nm in ['SYMM_1SIG', 'SYMM_3SIG', 'MONOTONIC_1SIG']:
        bit = mask.addMaskPlane(nm)
        val = mask.getPlaneBitMask(nm)
        #print 'Added', nm, '=', val
    #print 'New mask planes:'
    #mask.printMaskPlanes()

    imbb = img.getBBox(afwImage.PARENT)
    bb = fp.getBBox()
    assert(imbb.contains(bb))
    W,H = bb.getWidth(), bb.getHeight()
    x0,y0 = bb.getMinX(), bb.getMinY()
    x1,y1 = bb.getMaxX(), bb.getMaxY()

    if fit_psfs:
        # Find peaks that are well-fit by a PSF + background model.
        _fit_psfs(fp, peaks, res, log, psf, psffwhm, img, varimg,
                  psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)

    log.logdebug('Creating templates for footprint at x0,y0,W,H = (%i,%i, %i,%i)' % (x0,y0,W,H))
    for pkres in res.peaks:
        if pkres.out_of_bounds or pkres.deblend_as_psf:
            continue
        pk = pkres.peak
        cx,cy = pk.getIx(), pk.getIy()
        if not imbb.contains(afwGeom.Point2I(cx,cy)):
            log.logdebug('Peak center is not inside image; skipping %i' % pkres.pki)
            pkres.out_of_bounds = True
            continue
        log.logdebug('computing template for peak %i at (%i,%i)' % (pkres.pki, cx, cy))
        S = butils.buildSymmetricTemplate(maskedImage, fp, pk, sigma1, True)
        # SWIG doesn't know how to unpack an std::pair into a 2-tuple...
        t1, tfoot = S[0], S[1]
        if t1 is None:
            log.logdebug('Peak %i at (%i,%i): failed to build symmetric template' % (pkres.pki, cx,cy))
            pkres.out_of_bounds = True
            continue

        # for debugging purposes: copy the original symmetric template
        pkres.symm = t1.getImage().Factory(t1.getImage(), True)

        # Smooth / filter
        if False:
            sig = 0.5
            G = afwMath.GaussianFunction1D(sig)
            S = 1+int(math.ceil(sig*4.))
            kern = afwMath.SeparableKernel(S, S, G, G)
            # Place output back into input -- so create a copy first
            t1im = t1.getImage()
            inimg = t1im.Factory(t1im, True)
            outimg = t1im
            ctrl = afwMath.ConvolutionControl()
            ctrl.setDoCopyEdge(True)
            afwMath.convolve(outimg, inimg, kern, ctrl)

        if median_smooth_template:
            if t1.getWidth() > 5 and t1.getHeight() > 5:
                log.logdebug('Median filtering template %i' % pkres.pki)
                # We want the output to go in "t1", so copy it into "inimg" for input
                inimg = t1.Factory(t1, True)
                outimg = t1
                butils.medianFilter(inimg, outimg, 2)

                # save for debugging purposes
                pkres.median = t1.getImage().Factory(t1.getImage(), True)

        if monotonic_template:
            log.logdebug('Making template %i monotonic' % pkres.pki)
            butils.makeMonotonic(t1, fp, pk, sigma1)

        pkres.tmimg = t1
        pkres.timg = t1.getImage()
        pkres.tfoot = tfoot

    tmimgs = []
    ibi = [] # in-bounds indices
    for pkres in res.peaks:
        if pkres.out_of_bounds:
            continue
        tmimgs.append(pkres.tmimg)
        ibi.append(pkres.pki)

    if lstsq_weight_templates:
        # Reweight the templates by doing a least-squares fit to the image
        A = np.zeros((W * H, len(tmimgs)))
        b = img.getArray()[y0:y1+1, x0:x1+1].ravel()

        for mim,i in zip(tmimgs, ibi):
            ibb = mim.getBBox(afwImage.PARENT)
            ix0,iy0 = ibb.getMinX(), ibb.getMinY()
            pkres = res.peaks[i]
            foot = pkres.tfoot
            ima = mim.getImage().getArray()
            for sp in foot.getSpans():
                sy, sx0, sx1 = sp.getY(), sp.getX0(), sp.getX1()
                imrow = ima[sy-iy0, sx0-ix0 : 1+sx1-ix0]
                r0 = (sy-y0)*W
                A[r0 + sx0-x0: r0+1+sx1-x0, i] = imrow
        X1,r1,rank1,s1 = np.linalg.lstsq(A, b)
        print 'Template weights:', X1
        del A
        del b

        for mim,i,w in zip(tmimgs, ibi, X1):
            im = mim.getImage()
            im *= w
            res.peaks[i].tweight = w

    # Now apportion flux according to the templates
    log.logdebug('Apportioning flux among %i templates' % len(tmimgs))
    ports = butils.apportionFlux(maskedImage, fp, tmimgs)
    ii = 0
    for (pk, pkres) in zip(peaks, res.peaks):
        if pkres.out_of_bounds:
            continue
        pkres.mportion = ports[ii]
        pkres.portion = ports[ii].getImage()
        ii += 1
        heavy = afwDet.makeHeavyFootprint(pkres.tfoot, pkres.mportion)
        heavy.getPeaks().push_back(pk)
        pkres.heavy = heavy

    return res

class CachingPsf(object):
    def __init__(self, psf):
        self.cache = {}
        self.psf = psf
    def computeImage(self, cx, cy):
        im = self.cache.get((cx,cy), None)
        if im is not None:
            return im
        im = self.psf.computeImage(afwGeom.Point2D(cx,cy))
        self.cache[(cx,cy)] = im
        return im


def _fit_psfs(fp, peaks, fpres, log, psf, psffwhm, img, varimg,
              psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b):
    fbb = fp.getBBox()
    cpsf = CachingPsf(psf)

    fmask = afwImage.MaskU(fbb)
    fmask.setXY0(fbb.getMinX(), fbb.getMinY())
    afwDet.setMaskFromFootprint(fmask, fp, 1)

    for pki,(pk,pkres) in enumerate(zip(peaks, fpres.peaks)):
        log.logdebug('Peak %i' % pki)
        _fit_psf(fp, fmask, pk, pkres, fbb, peaks, log, cpsf, psffwhm, img, varimg,
                 psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)


def _fit_psf(fp, fmask, pk, pkres, fbb, peaks, log, psf, psffwhm, img, varimg,
             psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b):
    # Fit a PSF + smooth background model (linear) to a 
    # small region around the peak
    # The small region is a disk out to R0, plus a ramp with decreasing weight
    # down to R1.
    R0 = int(math.ceil(psffwhm * 1.))
    # ramp down to zero weight at this radius...
    R1 = int(math.ceil(psffwhm * 1.5))
    S = 2 * R1
    cx,cy = pk.getFx(), pk.getFy()
    psfimg = psf.computeImage(cx, cy)
    # R2: distance to neighbouring peak in order to put it
    # into the model
    R2 = R1 + psfimg.getWidth()/2.

    pbb = psfimg.getBBox(afwImage.PARENT)
    pbb.clip(fbb)
    px0,py0 = psfimg.getX0(), psfimg.getY0()

    # The bounding-box of the local region we are going to fit ("stamp")
    xlo = int(math.floor(cx - R1))
    ylo = int(math.floor(cy - R1))
    xhi = int(math.ceil (cx + R1))
    yhi = int(math.ceil (cy + R1))
    stampbb = afwGeom.Box2I(afwGeom.Point2I(xlo,ylo), afwGeom.Point2I(xhi,yhi))
    stampbb.clip(fbb)
    xlo,xhi = stampbb.getMinX(), stampbb.getMaxX()
    ylo,yhi = stampbb.getMinY(), stampbb.getMaxY()
    if xlo >= xhi or ylo >= yhi:
        log.logdebug('Skipping this peak: out of bounds')
        pkres.out_of_bounds = True
        return

    # Cut out the postage stamp we are going to fit.
    #stamp = img.Factory(img, stampbb, afwImage.PARENT, True)
    #assert(stamp.getBBox(afwImage.PARENT) == stampbb)

    # find other peaks within range...
    otherpeaks = []
    for pk2 in peaks:
        if pk2 == pk:
            continue
        if pk.getF().distanceSquared(pk2.getF()) > R2**2:
            continue
        opsfimg = psf.computeImage(pk2.getFx(), pk2.getFy())
        otherpeaks.append(opsfimg)
    log.logdebug('%i other peaks within range' % len(otherpeaks))

    # Now we are going to do a least-squares fit for the flux
    # in this PSF, plus a decenter term, a linear sky, and
    # fluxes of nearby sources (assumed point sources).  Build
    # up the matrix...
    # Number of terms -- PSF flux, constant, X, Y, + other PSF fluxes
    NT1 = 4 + len(otherpeaks)
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

    # Build the matrix "A", rhs "b" and weight "w".

    ix0,iy0 = img.getX0(), img.getY0()
    fx0,fy0 = fbb.getMinX(), fbb.getMinY()
    fslice = (slice(ylo-fy0, yhi-fy0+1), slice(xlo-fx0, xhi-fx0+1))
    islice = (slice(ylo-iy0, yhi-iy0+1), slice(xlo-ix0, xhi-ix0+1))
    fmask_sub = fmask.getArray()[fslice]
    var_sub = varimg.getArray()[islice]
    img_sub = img.getArray()[islice]

    # Clip the PSF image to match its bbox
    psfarr = psfimg.getArray()[pbb.getMinY()-py0: 1+pbb.getMaxY()-py0,
                               pbb.getMinX()-px0: 1+pbb.getMaxX()-px0]
    px0 = pbb.getMinX()
    py0 = pbb.getMinY()

    # Compute the "valid" pixels within our region-of-interest
    infp = (fmask_sub > 0)
    XX,YY = np.meshgrid(np.arange(xlo, xhi+1), np.arange(ylo, yhi+1))
    RR = (XX - cx)**2 + (YY - cy)**2
    inr1 = (RR <= R1**2)
    invar = (var_sub > 0)
    valid = (infp * inr1 * invar)

    ipixes = np.vstack((XX[valid] - xlo, YY[valid] - ylo)).T
    NP = valid.sum()

    inpsfx = ((XX >= pbb.getMinX()) * (XX <= pbb.getMaxX()))
    inpsfy = ((YY >= pbb.getMinY()) * (YY <= pbb.getMaxY()))
    inpsf = (inpsfx * inpsfy)
    indx = (inpsfy * ((XX > pbb.getMinX()) * (XX < pbb.getMaxX())))
    indy = (inpsfx * ((YY > pbb.getMinY()) * (YY < pbb.getMaxY())))

    def _overlap(xlo, xhi, xmin, xmax):
        xloclamp = max(xlo, xmin)
        Xlo = xloclamp - xlo
        xhiclamp = min(xhi, xmax)
        Xhi = Xlo + (xhiclamp - xloclamp)
        return (xloclamp, xhiclamp+1, Xlo, Xhi+1)

    sx1,sx2,sx3,sx4 = _overlap(xlo, xhi, pbb.getMinX(), pbb.getMaxX())
    sy1,sy2,sy3,sy4 = _overlap(ylo, yhi, pbb.getMinY(), pbb.getMaxY())
    dpx0,dpy0 = px0 - xlo, py0 - ylo
    psfsub = psfarr[sy3 - dpy0 : sy4 - dpy0, sx3 - dpx0: sx4 - dpx0]
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]

    A = np.zeros((NP, NT2))
    A[inpsf[valid], I_psf] = psfsub[vsub]
    A[:,1] = 1.
    A[:,2] = XX[valid] - cx
    A[:,3] = YY[valid] - cy

    # dx
    oldsx = (sx1,sx2,sx3,sx4)
    sx1,sx2,sx3,sx4 = _overlap(xlo, xhi, pbb.getMinX()+1, pbb.getMaxX()-1)
    psfsub = (psfarr[sy3 - dpy0 : sy4 - dpy0, sx3 - dpx0 + 1: sx4 - dpx0 + 1] -
              psfarr[sy3 - dpy0 : sy4 - dpy0, sx3 - dpx0 - 1: sx4 - dpx0 - 1]) / 2.
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
    A[indx[valid], I_dx] = psfsub[vsub]

    # dy
    (sx1,sx2,sx3,sx4) = oldsx
    sy1,sy2,sy3,sy4 = _overlap(ylo, yhi, pbb.getMinY()+1, pbb.getMaxY()-1)
    psfsub = (psfarr[sy3 - dpy0 + 1: sy4 - dpy0 + 1, sx3 - dpx0: sx4 - dpx0] -
              psfarr[sy3 - dpy0 - 1: sy4 - dpy0 - 1, sx3 - dpx0: sx4 - dpx0]) / 2.
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
    A[indy[valid], I_dy] = psfsub[vsub]

    # other PSFs...
    for j,opsf in enumerate(otherpeaks):
        obb = opsf.getBBox(afwImage.PARENT)
        ino = ((XX >= obb.getMinX()) * (XX <= obb.getMaxX()) *
               (YY >= obb.getMinY()) * (YY <= obb.getMaxY()))
        dpx0,dpy0 = obb.getMinX() - xlo, obb.getMinY() - ylo
        sx1,sx2,sx3,sx4 = _overlap(xlo, xhi, obb.getMinX(), obb.getMaxX())
        sy1,sy2,sy3,sy4 = _overlap(ylo, yhi, obb.getMinY(), obb.getMaxY())
        opsfarr = opsf.getArray()
        psfsub = opsfarr[sy3 - dpy0 : sy4 - dpy0, sx3 - dpx0: sx4 - dpx0]
        vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
        A[ino[valid], I_opsf + j] = psfsub[vsub]

    b = img_sub[valid]

    rw = np.ones_like(RR)
    ii = (RR > R0**2)
    rr = np.sqrt(RR[ii])
    rw[ii] = 1. - ((rr - R0) / (R1-R0))
    w = np.sqrt(rw[valid] / var_sub[valid])
    sumr = np.sum(rw[valid])

    del rw
    del ii

    ########################################

    (A2,b2,w2,sumr2,ipixes2) = (A,b,w,sumr,ipixes)

    # Matrix, rhs and weight
    A = np.zeros((NP, NT2))
    b = np.zeros(NP)
    w = np.zeros(NP)
    
    # pixel indices
    ipixes = np.zeros((NP,2), dtype=int)
    ipix = 0
    sumr = 0.
    # for each postage stamp pixel...
    for y in range(ylo, yhi+1):
        for x in range(xlo, xhi+1):
            xy = afwGeom.Point2I(x,y)
            # within the parent footprint?
            if not fp.contains(xy):
                #stamp.set0(x, y, 0.)
                continue
            # is it within the radius we're going to fit (ie,
            # circle inscribed in the postage stamp square)?
            R = np.hypot(x - cx, y - cy)
            if R > R1:
                #stamp.set0(x, y, 0.)
                continue

            v = varimg.get0(x,y)
            if v == 0:
                # This pixel must not have been set... why?
                continue

            # Get the PSF contribution at this pixel
            if pbb.contains(xy):
                p = psfimg.get0(x,y)
            else:
                p = 0

            # Get the decenter (dx,dy) terms at this pixel
            # FIXME: we do symmetric finite difference; should
            # probably use the sinc-shifted PSF instead!
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
            # Ramp down weights for pixels between R0 and R1.
            rw = 1.
            if R > R0:
                rw = 1. - (R - R0) / (R1 - R0)
            # PSF flux
            A[ipix,I_psf] = p
            # constant sky
            A[ipix,1] = 1.
            # linear sky
            A[ipix,2] = x-cx
            A[ipix,3] = y-cy
            # decentered PSF
            A[ipix,I_dx] = dx
            A[ipix,I_dy] = dy
            # other nearby peak PSFs
            ipixes[ipix,:] = (x-xlo,y-ylo)
            for j,opsf in enumerate(otherpeaks):
                obbox = opsf.getBBox(afwImage.PARENT)
                if obbox.contains(afwGeom.Point2I(x,y)):
                    A[ipix, I_opsf + j] = opsf.get0(x, y)
            b[ipix] = img.get0(x, y)
            # weight = ramp weight * ( 1 / image variance )
            w[ipix] = np.sqrt(rw / v)
            sumr += rw
            ipix += 1

    # NP: actual number of pixels within range
    NP = ipix
    A = A[:NP,:]
    b = b[:NP]
    w = w[:NP]
    ipixes = ipixes[:NP,:]
    Aw  = A * w[:,np.newaxis]
    bw  = b * w

    ########################################

    assert(np.all(A2 == A))
    assert(np.all(w2 == w))
    assert(np.all(b2 == b))
    assert(np.all(sumr2 == sumr))
    assert(np.all(ipixes2 == ipixes))

    ########################################

    plt.clf()
    N = NT2 + 2
    R,C = 2, (N+1) / 2
    for i in range(NT2):
        im1 = np.zeros((1+yhi-ylo, 1+xhi-xlo))
        im1[ipixes[:,1], ipixes[:,0]] = A[:, i]
        plt.subplot(R, C, i+1)
        plt.imshow(im1, interpolation='nearest', origin='lower')

    plt.subplot(R, C, NT2+1)
    im1 = np.zeros((1+yhi-ylo, 1+xhi-xlo))
    im1[ipixes[:,1], ipixes[:,0]] = b
    plt.imshow(im1, interpolation='nearest', origin='lower')
    plt.subplot(R, C, NT2+2)
    im1 = np.zeros((1+yhi-ylo, 1+xhi-xlo))
    im1[ipixes[:,1], ipixes[:,0]] = w
    plt.imshow(im1, interpolation='nearest', origin='lower')
    plt.savefig('A.png')




    # We do fits with and without the decenter terms.
    # Since the dx,dy terms are at the end of the matrix,
    # we can do that just by trimming off those elements.
    X1,r1,rank1,s1 = np.linalg.lstsq(Aw[:,:NT1], bw)
    X2,r2,rank2,s2 = np.linalg.lstsq(Aw, bw)
    #print 'X1', X1
    #print 'X2', X2
    #print 'ranks', rank1, rank2
    #print 'r', r1, r2
    # r is weighted chi-squared = sum over pixels:  ramp * (model - data)**2/sigma**2
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
    #print 'Chi-squareds', chisq1, chisq2
    #print 'Degrees of freedom', dof1, dof2
    # This can happen if we're very close to the edge (?)
    if dof1 <= 0 or dof2 <= 0:
        log.logdebug('Skipping this peak: bad DOF %g, %g' % (dof1, dof2))
        pkres.out_of_bounds = True
        return

    q1 = chisq1/dof1
    q2 = chisq2/dof2
    log.logdebug('PSF fits: chisq/dof = %g, %g' % (q1,q2))

    ispsf1 = (q1 < psf_chisq_cut1)
    ispsf2 = (q2 < psf_chisq_cut2)

    # check that the fit PSF spatial derivative terms aren't too big
    if ispsf2:
        fdx, fdy = X2[I_dx], X2[I_dy]
        f0 = X2[I_psf]
        # as a fraction of the PSF flux
        dx = fdx/f0
        dy = fdy/f0
        ispsf2 = ispsf2 and (abs(dx) < 1. and abs(dy) < 1.)
        log.logdebug('isPSF2 -- checking derivatives: dx,dy = %g, %g -> %s' %
                     (dx,dy, str(ispsf2)))

    # Looks like a shifted PSF: try actually shifting the PSF by that amount
    # and re-evaluate the fit.
    if ispsf2:
        psfimg2 = psf.computeImage(cx + dx, cy + dy)
        # clip
        pbb2 = psfimg2.getBBox(afwImage.PARENT)
        pbb2.clip(fbb)
        psfimg2 = psfimg2.Factory(psfimg2, pbb2, afwImage.PARENT)
        # yuck!  Update the PSF terms in the least-squares fit matrix.
        Ab = A[:,:NT1].copy()
        for ipix,(x,y) in enumerate(ipixes):
            xi,yi = int(x)+xlo, int(y)+ylo
            if pbb2.contains(afwGeom.Point2I(xi,yi)):
                p = psfimg2.get0(xi,yi)
            else:
                p = 0
            Ab[ipix,I_psf] = p
        Aw  = Ab * w[:,np.newaxis]
        # re-solve...
        Xb,rb,rankb,sb = np.linalg.lstsq(Aw, bw)
        #print 'Xb', Xb
        if len(rb) > 0:
            chisqb = rb[0]
        else:
            chisqb = 1e30
        #print 'chisq', chisqb
        dofb = sumr - len(Xb)
        #print 'dof', dofb
        qb = chisqb / dofb
        #print 'chisq/dof', qb
        ispsf2 = (qb < psf_chisq_cut2b)
        q2 = qb
        X2 = Xb
        log.logdebug('shifted PSF: new chisq/dof = %g; good? %s' % (qb, ispsf2))

    # Which one do we keep?
    if (((ispsf1 and ispsf2) and (q2 < q1)) or
        (ispsf2 and not ispsf1)):
        Xpsf = X2
        chisq = chisq2
        dof = dof2
        log.logdebug('Keeping shifted-PSF model')
        cx += dx
        cy += dy
    else:
        # (arbitrarily set to X1 when neither fits well)
        Xpsf = X1
        chisq = chisq1
        dof = dof1
        log.logdebug('Keeping unshifted PSF model')
    ispsf = (ispsf1 or ispsf2)
        
    # Save the PSF models in images for posterity.
    SW,SH = 1+xhi-xlo, 1+yhi-ylo
    psfmod = afwImage.ImageF(SW,SH)
    psfmod.setXY0(xlo,ylo)
    psfderivmodm = afwImage.MaskedImageF(SW,SH)
    psfderivmod = psfderivmodm.getImage()
    psfderivmod.setXY0(xlo,ylo)
    model = afwImage.ImageF(SW,SH)
    model.setXY0(xlo,ylo)
    for i in range(len(Xpsf)):
        V = A[:,i]*Xpsf[i]
        for (x,y),v in zip(ipixes, A[:,i]*Xpsf[i]):
            ix,iy = int(x),int(y)
            model.set(ix, iy, model.get(ix,iy) + float(v))
            if i in [I_psf, I_dx, I_dy]:
                psfderivmod.set(ix, iy, psfderivmod.get(ix,iy) + float(v))
    for ii in range(NP):
        x,y = ipixes[ii,:]
        psfmod.set(int(x),int(y), float(A[ii, I_psf] * Xpsf[I_psf]))
    modelfp = afwDet.Footprint()
    for (x,y) in ipixes:
        modelfp.addSpan(int(y+ylo), int(x+xlo), int(x+xlo))
    modelfp.normalize()

    # Save things we learned about this peak for posterity...
    pkres.R0 = R0
    pkres.R1 = R1
    #pkres.stamp = stamp
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
    pkres.otherpsfs = len(otherpeaks)
    pkres.w = w
    pkres.psfflux = Xpsf[I_psf]
    pkres.deblend_as_psf = bool(ispsf)

    if ispsf:
        # replace the template image by the PSF + derivatives
        # image.
        log.logdebug('Deblending as PSF; setting template to PSF model')
        pkres.timg = psfderivmod
        pkres.tmimg = psfderivmodm
        pkres.tfoot = modelfp

    
