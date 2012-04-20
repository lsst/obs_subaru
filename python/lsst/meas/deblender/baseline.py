import math
import numpy as np

import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.afw.geom  as afwGeom
import lsst.afw.math  as afwMath

# below...
#import lsst.meas.deblender as deb

# Result objects; will probably go away as we push more results
# into earlier-created Source objects, but useful for now for
# hauling debugging results around.
class PerFootprint(object):
    pass
class PerPeak(object):
    def __init__(self):
        self.out_of_bounds = False
        self.deblend_as_psf = False

def deblend(footprints, peaks, maskedImage, psf, psffwhm,
            psf_chisq_cut1 = 1.5,
            psf_chisq_cut2 = 1.5,
            psf_chisq_cut2b = 1.5,
            log=None, verbose=False,
            ):
    '''
    Deblend the given list of footprints (containing the given list of
    peaks), in the given maskedImage, with given PSF model (with given
    FWHM).

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
    if log is None:
        import lsst.pex.logging as pexLogging
        loglvl = pexLogging.Log.INFO
        if verbose:
            loglvl = pexLogging.Log.DEBUG
        log = pexLogging.Log(pexLogging.Log.getDefaultLog(), 'meas_deblender.baseline',
                             loglvl)

    import lsst.meas.deblender as deb
    butils = deb.BaselineUtilsF

    img = maskedImage.getImage()
    varimg = maskedImage.getVariance()
    mask = maskedImage.getMask()
    # find the median variance in the image...
    stats = afwMath.makeStatistics(varimg, mask, afwMath.MEDIAN)
    sigma1 = math.sqrt(stats.getValue(afwMath.MEDIAN))
    #print 'Median image sigma:', sigma1

    imbb = img.getBBox(afwImage.PARENT)
    log.logdebug('imbb (PARENT): %i,%i,%i,%i' % (imbb.getMinX(), imbb.getMaxX(), imbb.getMinY(), imbb.getMaxY()))

    # prepare results structures
    results = []
    for fp,pks in zip(footprints,peaks):
        fp.normalize()
        fpres = PerFootprint()
        results.append(fpres)
        fpres.peaks = []
        for pki,pk in enumerate(pks):
            pkres = PerPeak()
            fpres.peaks.append(pkres)

    #print 'Initial mask planes:'
    #mask.printMaskPlanes()
    for nm in ['SYMM_1SIG', 'SYMM_3SIG', 'MONOTONIC_1SIG']:
        bit = mask.addMaskPlane(nm)
        val = mask.getPlaneBitMask(nm)
        #print 'Added', nm, '=', val
    #print 'New mask planes:'
    #mask.printMaskPlanes()

    # First find peaks that are well-fit by a PSF + background model.
    for fp,pks,fpres in zip(footprints,peaks,results):
        fbb = fp.getBBox()
        assert(imbb.contains(fbb))
        for pki,(pk,pkres) in enumerate(zip(pks, fpres.peaks)):
            log.logdebug('Peak %i' % pki)
            # Fit a PSF + smooth background model (linear) to a 
            # small region around the peak
            # The small region is a disk out to R0, plus a ramp with decreasing weight
            # down to R1.
            R0 = int(math.ceil(psffwhm * 1.))
            # ramp down to zero weight at this radius...
            R1 = int(math.ceil(psffwhm * 1.5))
            S = 2 * R1
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
            xlo,xhi = stampbb.getMinX(), stampbb.getMaxX()
            ylo,yhi = stampbb.getMinY(), stampbb.getMaxY()
            if xlo >= xhi or ylo >= yhi:
                log.logdebug('Skipping this peak: out of bounds')
                pkres.out_of_bounds = True
                continue
            # Cut out the postage stamp we are going to fit.
            stamp = img.Factory(img, stampbb, afwImage.PARENT, True)
            assert(stamp.getBBox(afwImage.PARENT) == stampbb)

            # find other peaks within range...
            otherpeaks = []
            for j,pk2 in enumerate(pks):
                if j == pki:
                    continue
                if pk.getF().distanceSquared(pk2.getF()) > R2**2:
                    continue
                opsfimg = psf.computeImage(pk2.getF())
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
                    # is it within the radius we're going to fit (ie,
                    # circle inscribed in the postage stamp square)?
                    R = np.hypot(x - cx, y - cy)
                    if R > R1:
                        stamp.set0(x, y, 0.)
                        continue
                    xy = afwGeom.Point2I(x,y)
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
                        rw = (R - R0) / (R1 - R0)
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
                    w[ipix] = np.sqrt(rw / varimg.get0(x,y))
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
                continue

            q1 = chisq1/dof1
            q2 = chisq2/dof2
            log.logdebug('PSF fits: chisq/dof = %g, %g' % (q1,q2))

            # MAGIC number
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
                psfimg2 = psf.computeImage(afwGeom.Point2D(cx + dx, cy + dy))
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

            # Save things we learned about this peak for posterity...
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
            pkres.otherpsfs = len(otherpeaks)
            pkres.w = w
            pkres.deblend_as_psf = ispsf

            if ispsf:
                # replace the template image by the PSF + derivatives
                # image.
                log.logdebug('Deblending as PSF; setting template to PSF model')
                pkres.timg = psfderivmod
                pkres.tmimg = psfderivmodm

    log.logdebug('Computing templates...')

    for fp,pks,fpres in zip(footprints,peaks,results):
        bb = fp.getBBox()
        W,H = bb.getWidth(), bb.getHeight()
        x0,y0 = bb.getMinX(), bb.getMinY()
        x1,y1 = bb.getMaxX(), bb.getMaxY()
        log.logdebug('Creating templates for footprint at x0,y0,W,H = (%i,%i, %i,%i)' % (x0,y0,W,H))
        for pki,(pk,pkres) in enumerate(zip(pks, fpres.peaks)):
            if pkres.out_of_bounds:
                #log.logdebug('Skipping out-of-bounds peak %i' % pki)
                continue
            if pkres.deblend_as_psf:
                #log.logdebug('Skipping PSF peak %i' % pki)
                continue
            #print 'creating heavy footprint...'
            #heavy = afwDet.makeHeavyFootprint(fp, img)
            #print 'n peaks:', len(heavy.getPeaks())
            cx,cy = pk.getIx(), pk.getIy()
            #print 'cx,cy =', cx,cy
            if not imbb.contains(afwGeom.Point2I(cx,cy)):
                log.logdebug('Peak center is not inside image; skipping %i' % pki)
                pkres.out_of_bounds = True
                continue

            log.logdebug('computing template for peak %i' % pki)
            t1 = butils.buildSymmetricTemplate(maskedImage, fp, pk, sigma1)
            # Smooth / filter
            if False:
                sig = 0.5
                G = afwMath.GaussianFunction1D(sig)
                S = 1+int(math.ceil(sig*4.))
                #print type(S), G
                kern = afwMath.SeparableKernel(S, S, G, G)
                #smoothed = afwImage.Factory(t1.getImage().getDimensions())
                # Place output back into input -- so create a copy first
                t1im = t1.getImage()
                inimg = t1im.Factory(t1im, True)
                outimg = t1im
                ctrl = afwMath.ConvolutionControl()
                ctrl.setDoCopyEdge(True)
                afwMath.convolve(outimg, inimg, kern, ctrl)
            else:
                # Place output back into input -- so create a copy first
                log.logdebug('Median filtering template %i' % pki)
                t1im = t1 #.getImage()
                inimg = t1im.Factory(t1im, True)
                outimg = t1im
                butils.medianFilter(inimg, outimg, 2)

            log.logdebug('Making template %i monotonic' % pki)
            butils.makeMonotonic(t1, fp, pk, sigma1)

            pkres.tmimg = t1
            pkres.timg = t1.getImage()

        # Now apportion flux according to the templates
        tmimgs = []
        for pki,pkres in enumerate(fpres.peaks):
            if pkres.out_of_bounds:
                #print 'Skipping out-of-bounds peak', pki
                continue
            tmimgs.append(pkres.tmimg)
        log.logdebug('Apportioning flux among %i templates' % len(tmimgs))
        ports = butils.apportionFlux(maskedImage, fp, tmimgs)
        ii = 0
        for pki,pkres in enumerate(fpres.peaks):
            if pkres.out_of_bounds:
                #print 'Skipping out-of-bounds peak', pki
                continue
            pkres.mportion = ports[ii]
            pkres.portion = ports[ii].getImage()
            ii += 1

    for fpi,(fp,pks,fpres) in enumerate(zip(footprints,peaks,results)):
        for pki,(pk,pkres) in enumerate(zip(pks, fpres.peaks)):
            foot = fp
            if pkres.out_of_bounds:
                #print 'Skipping out-of-bounds peak', pki
                continue
            if pkres.deblend_as_psf:
                #print 'Creating fake footprint for deblended-as-PSF peak'
                p = pkres.mportion
                foot = afwDet.Footprint()
                #splist = foot.getSpans()
                x0 = p.getX0()
                y0 = p.getY0()
                W = p.getWidth()
                H = p.getHeight()
                #print 'Flux portion shape:', W, H
                for y in range(H):
                    #splist.push_back(afwDet.Span(y0+y, x0, x0+W-1))
                    foot.addSpan(y0+y, x0, x0+W-1)
                foot.normalize()
                #print 'fake footprint area:', foot.getArea()
            #print 'Creating heavy footprint for footprint', fpi, 'peak', pki
            pkres.heavy = afwDet.makeHeavyFootprint(foot, pkres.mportion)

    return results
