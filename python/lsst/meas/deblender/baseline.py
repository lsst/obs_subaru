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
        self.out_of_bounds = False      # Used to mean "Bad Peak" rather than just out of bounds
        self.deblend_as_psf = False

    def __str__(self):
        return 'Per-peak deblend result: out_of_bounds: %s, deblend_as_psf: %s' % (self.out_of_bounds, self.deblend_as_psf)
        
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
            maxNumberOfPeaks=0,
            dropTinyFootprints=True,
            findStrayFlux=True,
            assignStrayFlux=True,
            strayFluxToPointSources='necessary',
            rampFluxAtEdge=False,
            patchEdges=False
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

    maxNumberOfPeaks: if positive, only deblend the brightest maxNumberOfPeaks peaks in the parent
    '''
    # Import C++ routines
    import lsst.meas.deblender as deb
    butils = deb.BaselineUtilsF

    validStrayPtSrc = ['never', 'necessary', 'always']
    if not strayFluxToPointSources in validStrayPtSrc:
        raise ValueError(('strayFluxToPointSources: value \"%s\" not in the set of allowed values: '
                          % strayFluxToPointSources) + str(validStrayPtSrc))
    
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
    if maxNumberOfPeaks > 0:
        peaks = peaks[0:maxNumberOfPeaks]

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

    for nm in ['SYMM_1SIG', 'SYMM_3SIG', 'MONOTONIC_1SIG']:
        bit = mask.addMaskPlane(nm)
        val = mask.getPlaneBitMask(nm)

    imbb = img.getBBox(afwImage.PARENT)
    bb = fp.getBBox()
    assert(imbb.contains(bb))
    W,H = bb.getWidth(), bb.getHeight()
    x0,y0 = bb.getMinX(), bb.getMinY()
    x1,y1 = bb.getMaxX(), bb.getMaxY()

    if fit_psfs:
        # Find peaks that are well-fit by a PSF + background model.
        _fit_psfs(fp, peaks, res, log, psf, psffwhm, img, varimg,
                  psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b,
                  dropTiny = dropTinyFootprints)

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
        S = butils.buildSymmetricTemplate(maskedImage, fp, pk, sigma1, True,
                                          patchEdges)
        # SWIG doesn't know how to unpack an std::pair into a 2-tuple...
        t1, tfoot = S[0], S[1]

        if t1 is None:
            log.logdebug('Peak %i at (%i,%i): failed to build symmetric template' % (pkres.pki, cx,cy))
            pkres.out_of_bounds = True
            continue

        # for debugging purposes: copy the original symmetric template
        pkres.symm = t1.getImage().Factory(t1.getImage(), True)

        if rampFluxAtEdge:
            if butils.hasSignificantFluxAtEdge(t1.getImage(), tfoot,
                                               3. * sigma1):
                log.logdebug('Found significant flux at template edge.')
                # Compute the max of:
                #  -symmetric-template-clipped image * PSF
                #  -footprint-clipped image
                # Ie, extend the template by the PSF and "fill in" the
                # footprint.
                # Then find the symmetric template of that image.

                # The size we'll grow by
                S = psffwhm * 1.5
                # make it an odd integer
                S = (int(S + 0.5) / 2) * 2 + 1

                tbb = tfoot.getBBox()
                tbb.grow(S)

                # footprint-clipped image
                fcim = t1.Factory(tbb)
                fpcopy = afwDet.Footprint(fp)
                fpcopy.clipTo(tbb)
                theavy = afwDet.makeHeavyFootprint(fpcopy, maskedImage)
                theavy.insert(fcim)
                
                # template-clipped original image
                tcim = t1.Factory(tbb)
                theavy = afwDet.makeHeavyFootprint(tfoot, maskedImage)
                theavy.insert(tcim)

                # pixels on the edge of the template
                edgepix = butils.getSignificantEdgePixels(t1.getImage(),
                                                          tfoot, -1e6)
                # Debugging -- make picture of edge pixels
                #hedge = afwDet.makeHeavyFootprint(edgepix, t1)
                #eim = t1.Factory(edgepix.getBBox())
                #hedge.insert(eim)

                # instantiate PSF image
                xc = int((x0 + x1)/2)
                yc = int((y0 + y1)/2)
                psfim = psf.computeImage(afwGeom.Point2D(xc, yc))
                pbb = psfim.getBBox(afwImage.PARENT)
                # shift it to by centered on zero
                lx,ly = pbb.getMinX(), pbb.getMinY()
                psfim.setXY0(lx - xc, ly - yc)
                pbb = psfim.getBBox(afwImage.PARENT)
                # clip to S
                Sbox = afwGeom.Box2I(afwGeom.Point2I(-S, -S),
                                     afwGeom.Extent2I(2*S+1, 2*S+1))
                print 'PSF bb', pbb
                print 'Sbox', Sbox
                if not Sbox.contains(pbb):
                    #print 'Clipping PSF image'
                    psfim = psfim.Factory(psfim, Sbox, afwImage.PARENT,
                                          True)
                    pbb = psfim.getBBox(afwImage.PARENT)
                px0 = pbb.getMinX()
                px1 = pbb.getMaxX()
                py0 = pbb.getMinY()
                py1 = pbb.getMaxY()

                # Compute the ramp
                ramped = t1.Factory(tbb)
                Tout = ramped.getImage().getArray()
                Tin  = t1.getImage().getArray()
                tx0,ty0 = t1.getX0(), t1.getY0()
                ox0,oy0 = ramped.getX0(), ramped.getY0()
                P = psfim.getArray()
                P /= P.max()
                # For each edge pixel, Tout = max(Tout, edgepix * PSF)
                for span in edgepix.getSpans():
                    y = span.getY()
                    for x in range(span.getX0(), span.getX1()+1):
                        sy0, sy1 = y+py0 - oy0, y+py1+1 - oy0
                        sx0, sx1 = x+px0 - ox0, x+px1+1 - ox0
                        Tout[sy0:sy1, sx0:sx1] = (
                            np.maximum(Tout[sy0:sy1, sx0:sx1],
                                       Tin[y-ty0, x-tx0] * P))

                # Fill in the "fcim" (which has the right variance and
                # mask planes) with the ramped pixels
                # [where it is zero / outside the footprint]
                # debugging: make a copy
                #maximg = t1.Factory(fcim, True)
                maximg = fcim
                I = (maximg.getImage().getArray() == 0)
                maximg.getImage().getArray()[I] = ramped.getImage().getArray()[I]
                
                fpcopy = afwDet.growFootprint(fpcopy, S)
                fpcopy.normalize()
                
                rtn = butils.buildSymmetricTemplate(maximg, fpcopy, pk, sigma1, True, patchEdges)
                # silly SWIG
                t2, tfoot2 = rtn[0], rtn[1]
                
                # This template footprint may extend outside the parent
                # footprint -- or the image -- clip it.
                tfoot2.clipTo(imbb)
                tfoot2.clipTo(bb)
                tbb = tfoot2.getBBox()
                # clip template to bbox
                t2 = t2.Factory(t2, tbb, afwImage.PARENT, True)

                # Copy for debugging
                pkres.symm = t2.getImage().Factory(t2.getImage(), True)
                t1 = t2
                tfoot = tfoot2

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
            else:
                log.logdebug('Not median-filtering template %i: size %i x %i' %
                             (pkres.pki, t1.getWidth(), t1.getHeight()))
                
        if monotonic_template:
            log.logdebug('Making template %i monotonic' % pkres.pki)
            butils.makeMonotonic(t1, fp, pk, sigma1)

        pkres.tmimg = t1
        pkres.timg = t1.getImage()
        pkres.tfoot = tfoot

    tmimgs = []
    tfoots = []
    dpsf = []
    pkxy = []
    pkx = []
    pky = []
    ibi = [] # in-bounds indices
    for pkres in res.peaks:
        if pkres.out_of_bounds:
            continue
        tmimgs.append(pkres.tmimg)
        tfoots.append(pkres.tfoot)
        # for stray flux...
        dpsf.append(pkres.deblend_as_psf)
        pk = pkres.peak
        pkxy.append((pk.getIx(), pk.getIy()))
        pkx.append(pk.getIx())
        pky.append(pk.getIy())

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
        # print 'Template weights:', X1
        del A
        del b

        for mim,i,w in zip(tmimgs, ibi, X1):
            im = mim.getImage()
            im *= w
            res.peaks[i].tweight = w


    #
    # FIXME -- Remove templates that are too similar (via dot-product test)
    #
    
    # Now apportion flux according to the templates
    log.logdebug('Apportioning flux among %i templates' % len(tmimgs))
    sumimg = afwImage.ImageF(bb.getDimensions())
    strayflux = butils.getEmptyStrayFluxList()
    strayopts = 0
    if findStrayFlux:
        strayopts |= butils.ASSIGN_STRAYFLUX
        if strayFluxToPointSources == 'necessary':
            strayopts |= butils.STRAYFLUX_TO_POINT_SOURCES_WHEN_NECESSARY
        elif strayFluxToPointSources == 'always':
            strayopts |= butils.STRAYFLUX_TO_POINT_SOURCES_ALWAYS
            
    strayopts |= butils.STRAYFLUX_R_TO_FOOTPRINT
    
    ports = butils.apportionFlux(maskedImage, fp, tmimgs, tfoots, sumimg,
                                 dpsf, pkx, pky, strayflux, strayopts)
    
    ii = 0
    for j, (pk, pkres) in enumerate(zip(peaks, res.peaks)):
        if pkres.out_of_bounds:
            continue
        pkres.mportion = ports[ii]
        pkres.portion = ports[ii].getImage()
        ii += 1
        heavy = afwDet.makeHeavyFootprint(pkres.tfoot, pkres.mportion)
        heavy.getPeaks().push_back(pk)

        if findStrayFlux:
            # NOTE that due to a swig bug (https://github.com/swig/swig/issues/59)
            # we CANNOT iterate over "strayflux", but must index into it.
            stray = strayflux[j]
        else:
            stray = None

        pkres.heavy1 = heavy
        pkres.stray = stray

        if stray:
            pkres.heavy2 = butils.mergeHeavyFootprints(heavy, stray)
        else:
            pkres.heavy2 = heavy

        if assignStrayFlux:
            pkres.heavy = pkres.heavy2
        else:
            pkres.heavy = pkres.heavy1
            
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
              psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b,
              **kwargs):
    '''
    Fit a PSF + smooth background models to a small region around each
    peak (plus other peaks that are nearby) in the given list of
    peaks.

    fp: Footprint containing the Peaks to model
    peaks: a list of all the Peak objects within this Footprint
    fpres: a PerFootprint result object where results will be placed

    log: pex Log object
    psf: afw PSF object
    psffwhm: FWHM of the PSF, in pixels
    img: umm, the Image in which this Footprint finds itself
    varimg: Variance plane
    psf_chisq_cut*: floats: cuts in chisq-per-pixel at which to accept the PSF model

    Results go into the fpres.peaks objects.
    
    '''
    fbb = fp.getBBox()
    cpsf = CachingPsf(psf)

    fmask = afwImage.MaskU(fbb)
    fmask.setXY0(fbb.getMinX(), fbb.getMinY())
    afwDet.setMaskFromFootprint(fmask, fp, 1)

    # pk.getF() -- retrieving the floating-point location of the peak
    # -- actually shows up in the profile if we do it in the loop, so
    # grab them all here.
    peakF = [pk.getF() for pk in peaks]

    for pki,(pk,pkres,pkF) in enumerate(zip(peaks, fpres.peaks, peakF)):
        log.logdebug('Peak %i' % pki)
        _fit_psf(fp, fmask, pk, pkF, pkres, fbb, peaks, peakF, log, cpsf,
                 psffwhm, img, varimg,
                 psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b,
                 **kwargs)


def _fit_psf(fp, fmask, pk, pkF, pkres, fbb, peaks, peaksF, log, psf,
             psffwhm, img, varimg,
             psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b,
             dropTiny = True
             ):
    '''
    Fit a PSF + smooth background model (linear) to a small region around the peak.

    fp: Footprint
    fmask: the Mask plane for pixels in the Footprint
    pk: the Peak within the Footprint that we are going to fit a PSF model for
    pkF: the floating-point coordinates of this peak
    pkres: a PerPeak object that will hold the results for this peak
    fbb: the bounding-box of this Footprint (a Box2I)
    peaks: a list of all the Peak objects within this Footprint
    peaksF: a python list of the floating-point (x,y) peak locations
    log: pex Log object
    psf: afw PSF object
    psffwhm: FWHM of the PSF, in pixels
    img: umm, the Image in which this Footprint finds itself
    varimg: Variance plane
    psf_chisq_cut*: floats: cuts in chisq-per-pixel at which to accept the PSF model
    
    '''
    # The small region is a disk out to R0, plus a ramp with decreasing weight
    # down to R1.
    R0 = int(math.ceil(psffwhm * 1.))
    # ramp down to zero weight at this radius...
    R1 = int(math.ceil(psffwhm * 1.5))
    S = 2 * R1
    cx,cy = pkF.getX(), pkF.getY()
    psfimg = psf.computeImage(cx, cy)
    # R2: distance to neighbouring peak in order to put it
    # into the model
    R2 = R1 + min(psfimg.getWidth(), psfimg.getHeight())/2.

    import lsstDebug
    # __name__ is lsst.meas.deblender.baseline
    debugPlots = lsstDebug.Info(__name__).plots
    debugPsf = lsstDebug.Info(__name__).psf

    pbb = psfimg.getBBox(afwImage.PARENT)

    #print 'PSFimg bb x[%i,%i], y[%i,%i]' % (
    #    pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY())
    #print 'Foot bb x[%i,%i], y[%i,%i]' % (
    #    fbb.getMinX(), fbb.getMaxX(), fbb.getMinY(), fbb.getMaxY())

    pbb.clip(fbb)
    px0,py0 = psfimg.getX0(), psfimg.getY0()

    #print 'Clipped PSFimg bb x[%i,%i], y[%i,%i]' % (
    #    pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY())
    
    # The bounding-box of the local region we are going to fit ("stamp")
    xlo = int(math.floor(cx - R1))
    ylo = int(math.floor(cy - R1))
    xhi = int(math.ceil (cx + R1))
    yhi = int(math.ceil (cy + R1))
    stampbb = afwGeom.Box2I(afwGeom.Point2I(xlo,ylo), afwGeom.Point2I(xhi,yhi))
    stampbb.clip(fbb)
    xlo,xhi = stampbb.getMinX(), stampbb.getMaxX()
    ylo,yhi = stampbb.getMinY(), stampbb.getMaxY()
    if xlo > xhi or ylo > yhi:
        log.logdebug('Skipping this peak: out of bounds')
        pkres.out_of_bounds = True
        return

    #print 'Clipped stamp bb x[%i,%i], y[%i,%i]' % (
    #    stampbb.getMinX(), stampbb.getMaxX(), stampbb.getMinY(), stampbb.getMaxY())
    #print 'Stamp bb x[%i,%i], y[%i,%i]' % (xlo,xhi,ylo,yhi)
    
    # drop tiny footprints too?
    if dropTiny and (xhi < (xlo + 2) or yhi < (ylo + 2)):
        log.logdebug('Skipping this peak: tiny footprint / close to edge')
        pkres.out_of_bounds = True
        return

    # find other peaks within range...
    otherpeaks = []
    for pk2,pkF2 in zip(peaks, peaksF):
        if pk2 == pk:
            continue
        if pkF.distanceSquared(pkF2) > R2**2:
            continue
        opsfimg = psf.computeImage(pkF2.getX(), pkF2.getY())
        if not opsfimg.getBBox().overlaps(stampbb): # Local PSF may be a different size than central
            continue
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
    # I_sky = 1
    # I_sky_ramp_x = 2
    # I_sky_ramp_y = 3
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
    px0,px1 = pbb.getMinX(), pbb.getMaxX()
    py0,py1 = pbb.getMinY(), pbb.getMaxY()

    # Compute the "valid" pixels within our region-of-interest
    valid = (fmask_sub > 0)
    xx,yy = np.arange(xlo, xhi+1), np.arange(ylo, yhi+1)
    #RR = (XX - cx)**2 + (YY - cy)**2
    RR = ((xx - cx)**2)[np.newaxis,:] + ((yy - cy)**2)[:,np.newaxis]
    valid *= (RR <= R1**2)
    valid *= (var_sub > 0)
    NP = valid.sum()

    if NP == 0:
        log.warn('Skipping peak at (%.1f,%.1f): NP = 0' % (cx,cy))
        pkres.out_of_bounds = True
        return

    XX,YY = np.meshgrid(xx, yy)
    ipixes = np.vstack((XX[valid] - xlo, YY[valid] - ylo)).T

    # ipixes = np.empty((NP, 2))
    # dx = np.arange(0, xhi-xlo+1)
    # i0 = 0
    # for y,vr in enumerate(valid):
    #     n = vr.sum()
    #     ipixes[i0:i0+n, 0] = dx[vr]
    #     ipixes[i0:i0+n, 1] = y
    #     i0 += n
    #assert(np.all(ipixes == ipixes2))

    inpsfx = (xx >= px0) * (xx <= px1)
    inpsfy = (yy >= py0) * (yy <= py1)
    inpsf = np.outer(inpsfy, inpsfx)
    indx = np.outer(inpsfy, (xx > px0) * (xx < px1))
    indy = np.outer((yy > py0) * (yy < py1), inpsfx)

    del inpsfx
    del inpsfy

    def _overlap(xlo, xhi, xmin, xmax):
        assert xlo <= xmax and xhi >= xmin and xlo <= xhi and xmin <= xmax, "No overlap"
        xloclamp = max(xlo, xmin)
        Xlo = xloclamp - xlo
        xhiclamp = min(xhi, xmax)
        Xhi = Xlo + (xhiclamp - xloclamp)
        assert(xloclamp >= 0)
        assert(Xlo >= 0)
        return (xloclamp, xhiclamp+1, Xlo, Xhi+1)
    
    A = np.zeros((NP, NT2))
    # Constant term
    A[:,1] = 1.
    # Sky slope terms: dx, dy
    A[:,2] = ipixes[:,0] + (xlo-cx)
    A[:,3] = ipixes[:,1] + (ylo-cy)

    px0,px1 = pbb.getMinX(), pbb.getMaxX()
    py0,py1 = pbb.getMinY(), pbb.getMaxY()

    #print 'xlo,xhi', xlo,xhi
    #print 'px0,px1', px0,px1

    sx1,sx2,sx3,sx4 = _overlap(xlo, xhi, px0, px1)
    sy1,sy2,sy3,sy4 = _overlap(ylo, yhi, py0, py1)
    dpx0,dpy0 = px0 - xlo, py0 - ylo
    psfsub = psfarr[sy3 - dpy0 : sy4 - dpy0, sx3 - dpx0: sx4 - dpx0]
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
    A[inpsf[valid], I_psf] = psfsub[vsub]

    # dx
    oldsx = (sx1,sx2,sx3,sx4)
    sx1,sx2,sx3,sx4 = _overlap(xlo, xhi, px0+1, px1-1)
    psfsub = (psfarr[sy3 - dpy0 : sy4 - dpy0, sx3 - dpx0 + 1: sx4 - dpx0 + 1] -
              psfarr[sy3 - dpy0 : sy4 - dpy0, sx3 - dpx0 - 1: sx4 - dpx0 - 1]) / 2.
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
    A[indx[valid], I_dx] = psfsub[vsub]

    # dy
    (sx1,sx2,sx3,sx4) = oldsx
    sy1,sy2,sy3,sy4 = _overlap(ylo, yhi, py0+1, py1-1)
    psfsub = (psfarr[sy3 - dpy0 + 1: sy4 - dpy0 + 1, sx3 - dpx0: sx4 - dpx0] -
              psfarr[sy3 - dpy0 - 1: sy4 - dpy0 - 1, sx3 - dpx0: sx4 - dpx0]) / 2.
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
    A[indy[valid], I_dy] = psfsub[vsub]

    # other PSFs...
    for j,opsf in enumerate(otherpeaks):
        obb = opsf.getBBox(afwImage.PARENT)
        ino = np.outer((yy >= obb.getMinY()) * (yy <= obb.getMaxY()),
                       (xx >= obb.getMinX()) * (xx <= obb.getMaxX()))
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

    Aw  = A * w[:,np.newaxis]
    bw  = b * w

    if debugPlots:
        import pylab as plt
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
    #
    # The SVD can fail if there are NaNs in the matrices; this should
    # really be handled upstream
    try:
        # NT1 is number of terms without dx,dy;
        # X1 is the result without decenter
        X1,r1,rank1,s1 = np.linalg.lstsq(Aw[:,:NT1], bw)
        # X2 is with decenter
        X2,r2,rank2,s2 = np.linalg.lstsq(Aw, bw)
    except np.linalg.LinAlgError, e:
        log.log(log.WARN, "Failed to fit PSF to child: %s" % e)
        pkres.out_of_bounds = True
        return

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
        # clip image to bbox
        px0,py0 = psfimg2.getX0(), psfimg2.getY0()
        psfarr = psfimg2.getArray()[pbb2.getMinY()-py0: 1+pbb2.getMaxY()-py0,
                                    pbb2.getMinX()-px0: 1+pbb2.getMaxX()-px0]
        px0,py0 = pbb2.getMinX(), pbb2.getMinY()
        px1,py1 = pbb2.getMaxX(), pbb2.getMaxY()

        # yuck!  Update the PSF terms in the least-squares fit matrix.
        Ab = A[:,:NT1]

        sx1,sx2,sx3,sx4 = _overlap(xlo, xhi, px0, px1)
        sy1,sy2,sy3,sy4 = _overlap(ylo, yhi, py0, py1)
        dpx0,dpy0 = px0 - xlo, py0 - ylo
        psfsub = psfarr[sy3 - dpy0 : sy4 - dpy0, sx3 - dpx0: sx4 - dpx0]
        vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
        xx,yy = np.arange(xlo, xhi+1), np.arange(ylo, yhi+1)
        inpsf = np.outer((yy >= py0) * (yy <= py1), (xx >= px0) * (xx <= px1))
        Ab[inpsf[valid], I_psf] = psfsub[vsub]

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
    if debugPsf:
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

        pkres.psf0img = psfimg
        pkres.psfimg = psfmod
        pkres.psfderivimg = psfderivmod
        pkres.model = model

    # Save things we learned about this peak for posterity...
    pkres.R0 = R0
    pkres.R1 = R1
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

        # Hmm.... here we create a model footprint and MaskedImage BUT
        # the footprint covers the little region we fit, which is
        # different than the PSF size.  We could make this code much
        # simpler and maybe more sensible by just creating a circular
        # Footprint centered on the PSF.

        modelfp = afwDet.Footprint()
        for (x,y) in ipixes:
            modelfp.addSpan(int(y+ylo), int(x+xlo), int(x+xlo))
        modelfp.normalize()

        # Instantiate the PSF model and clip it to modelfp.
        psfimg = psf.computeImage(cx, cy)
        SW,SH = 1+xhi-xlo, 1+yhi-ylo
        psfmod = afwImage.MaskedImageF(SW,SH)
        psfmod.setXY0(xlo,ylo)
        # Scale by fit flux.
        psfimg *= Xpsf[I_psf]
        psfmim = afwImage.MaskedImageF(psfimg.convertF())
        heavy = afwDet.makeHeavyFootprint(modelfp, psfmim)
        heavy.insert(psfmod)

        pkres.tmimg = psfmod
        pkres.tfoot = modelfp
