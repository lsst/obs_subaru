#!/usr/bin/env python
#
# LSST Data Management System
# See COPYRIGHT file.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <https://www.lsstcorp.org/LegalNotices/>.
#
import numpy as np

import lsst.pex.exceptions
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom

# Import C++ routines
from ._baseline import BaselineUtilsF as butils

class DeblenderPlugin(object):
    """Class to define plugins for the deblender.

    The new deblender executes a series of plugins specified by the user.
    Each plugin defines the function to be executed, the keyword arguments required by the function,
    and whether or not certain portions of the deblender might need to be rerun as a result of
    the function.
    """
    def __init__(self, func, onReset=None, maxIterations=50, **kwargs):
        """Initialize a deblender plugin
        
        Parameters
        ----------
        func: `function`
            Function to run when the plugin is executed. The function should always take
            `debResult`, a `DeblenderResult` that stores the deblender result, and
            `log`, an `lsst.log`, as the first two arguments, as well as any additional
            keyword arguments (that must be specified in ``kwargs``).
            The function should also return ``modified``, a `bool` that tells the deblender whether
            or not any templates have been modified by the function.
            If ``modified==True``, the deblender will go back to step ``onReset``,
            unless the has already been run ``maxIterations``.
        onReset: `int`
            Index of the deblender plugin to return to if ``func`` modifies any templates.
            The default is ``None``, which does not re-run any plugins.
        maxIterations: `int`
            Maximum number of times the deblender will reset when the current plugin
            returns ``True``.
        """
        self.func = func
        self.kwargs = kwargs
        self.onReset = onReset
        self.maxIterations = maxIterations
        self.kwargs = kwargs
        self.iterations = 0

    def run(self, debResult, log):
        """Execute the current plugin

        Once the plugin has finished, check to see if part of the deblender must be executed again.
        """
        log.trace("Executing %s", self.func.__name__)
        reset = self.func(debResult, log, **self.kwargs)
        if reset:
            self.iterations += 1
            if self.iterations < self.maxIterations:
                return self.onReset
        return None

    def __str__(self):
        return ("<Deblender Plugin: func={0}, kwargs={1}".format(self.func.__name__, self.kwargs))
    def __repr__(self):
        return self.__str__()


def fitPsfs(debResult, log, psfChisqCut1=1.5, psfChisqCut2=1.5, psfChisqCut2b=1.5, tinyFootprintSize=2):
    """Fit a PSF + smooth background model (linear) to a small region around each peak
    
    This function will iterate over all filters in deblender result but does not compare
    results across filters.
    DeblendedPeaks that pass the cuts have their templates modified to the PSF + background model
    and their ``deblendedAsPsf`` property set to ``True``.
    
    This will likely be replaced in the future with a function that compares the psf chi-squared cuts
    so that peaks flagged as point sources will be considered point sources in all bands.

    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.
    psfChisqCut*: `float`, optional
        ``psfChisqCut1`` is the maximum chi-squared-per-degree-of-freedom allowed for a peak to
        be considered a PSF match without recentering.
        A fit is also made that includes terms to recenter the PSF.
        ``psfChisqCut2`` is the same as ``psfChisqCut1`` except it determines the restriction on the
        fit that includes recentering terms.
        If the peak is a match for a re-centered PSF, the PSF is repositioned at the new center and
        the peak footprint is fit again, this time to the new PSF.
        If the resulting chi-squared-per-degree-of-freedom is less than ``psfChisqCut2b`` then it
        passes the re-centering algorithm.
        If the peak passes both the re-centered and fixed position cuts, the better of the two is accepted,
        but parameters for all three psf fits are stored in the ``DebldendedPeak``.
        The default for ``psfChisqCut1``, ``psfChisqCut2``, and ``psfChisqCut2b`` is ``1.5``.
    tinyFootprintSize: `float`, optional
        The PSF model is shrunk to the size that contains the original footprint.
        If the bbox of the clipped PSF model for a peak is smaller than ``max(tinyFootprintSize,2)``
        then ``tinyFootprint`` for the peak is set to ``True`` and the peak is not fit.
        The default is 2.
    
    Returns
    -------
    modified: `bool`
        If any templates have been assigned to PSF point sources then ``modified`` is ``True``,
        otherwise it is ``False``.
    """
    from .baseline import CachingPsf
    modified = False
    # Loop over all of the filters to build the PSF
    for fidx in debResult.filters:
        dp = debResult.deblendedParents[fidx]
        peaks = dp.fp.getPeaks()
        cpsf = CachingPsf(dp.psf)

        # create mask image for pixels within the footprint
        fmask = afwImage.MaskU(dp.bb)
        fmask.setXY0(dp.bb.getMinX(), dp.bb.getMinY())
        afwDet.setMaskFromFootprint(fmask, dp.fp, 1)

        # pk.getF() -- retrieving the floating-point location of the peak
        # -- actually shows up in the profile if we do it in the loop, so
        # grab them all here.
        peakF = [pk.getF() for pk in peaks]

        for pki, (pk, pkres, pkF) in enumerate(zip(peaks, dp.peaks, peakF)):
            log.trace('Filter %s, Peak %i', fidx, pki)
            ispsf = _fitPsf(dp.fp, fmask, pk, pkF, pkres, dp.bb, peaks, peakF, log, cpsf, dp.psffwhm,
                            dp.img, dp.varimg, psfChisqCut1, psfChisqCut2, psfChisqCut2b, tinyFootprintSize)
            modified = modified or ispsf
    return modified

def _fitPsf(fp, fmask, pk, pkF, pkres, fbb, peaks, peaksF, log, psf, psffwhm,
            img, varimg, psfChisqCut1, psfChisqCut2, psfChisqCut2b,
            tinyFootprintSize=2,
            ):
    """Fit a PSF + smooth background model (linear) to a small region around a peak.

    See fitPsfs for a more thorough description, including all parameters not described below.

    Parameters
    ----------
    fp: `afw.detection.Footprint`
        Footprint containing the Peaks to model.
    fmask: `afw.image.MaskU`
        The Mask plane for pixels in the Footprint
    pk: `afw.detection.PeakRecord`
        The peak within the Footprint that we are going to fit with PSF model
    pkF: `afw.geom.Point2D`
        Floating point coordinates of the peak.
    pkres: `meas.deblender.DeblendedPeak`
        Peak results object that will hold the results.
    fbb: `afw.geom.Box2I`
        Bounding box of ``fp``
    peaks: `afw.detection.PeakCatalog`
            Catalog of peaks contained in the parent footprint.
    peaksF: list of `afw.geom.Point2D`
        List of floating point coordinates of all of the peaks.
    psf: list of `afw.detection.Psf`s
        Psf of the ``maskedImage`` for each band.
    psffwhm: list pf `float`s
        FWHM of the ``maskedImage``'s ``psf`` in each band.
    img: `afw.image.ImageF`
        The image that contains the footprint.
    varimg: `afw.image.ImageF`
        The variance of the image that contains the footprint.

    Results
    -------
    ispsf: `bool`
        Whether or not the peak matches a PSF model.
    """
    import lsstDebug

    # my __name__ is lsst.meas.deblender.baseline
    debugPlots = lsstDebug.Info(__name__).plots
    debugPsf = lsstDebug.Info(__name__).psf

    # The small region is a disk out to R0, plus a ramp with
    # decreasing weight down to R1.
    R0 = int(np.ceil(psffwhm*1.))
    # ramp down to zero weight at this radius...
    R1 = int(np.ceil(psffwhm*1.5))
    cx, cy = pkF.getX(), pkF.getY()
    psfimg = psf.computeImage(cx, cy)
    # R2: distance to neighbouring peak in order to put it into the model
    R2 = R1 + min(psfimg.getWidth(), psfimg.getHeight())/2.

    pbb = psfimg.getBBox()
    pbb.clip(fbb)
    px0, py0 = psfimg.getX0(), psfimg.getY0()

    # Make sure we haven't been given a substitute PSF that's nowhere near where we want, as may occur if
    # "Cannot compute CoaddPsf at point (xx,yy); no input images at that point."
    if not pbb.contains(afwGeom.Point2I(int(cx), int(cy))):
        pkres.setOutOfBounds()
        return

    # The bounding-box of the local region we are going to fit ("stamp")
    xlo = int(np.floor(cx - R1))
    ylo = int(np.floor(cy - R1))
    xhi = int(np.ceil(cx + R1))
    yhi = int(np.ceil(cy + R1))
    stampbb = afwGeom.Box2I(afwGeom.Point2I(xlo, ylo), afwGeom.Point2I(xhi, yhi))
    stampbb.clip(fbb)
    xlo, xhi = stampbb.getMinX(), stampbb.getMaxX()
    ylo, yhi = stampbb.getMinY(), stampbb.getMaxY()
    if xlo > xhi or ylo > yhi:
        log.trace('Skipping this peak: out of bounds')
        pkres.setOutOfBounds()
        return

    # drop tiny footprints too?
    if min(stampbb.getWidth(), stampbb.getHeight()) <= max(tinyFootprintSize, 2):
        # Minimum size limit of 2 comes from the "PSF dx" calculation, which involves shifting the PSF
        # by one pixel to the left and right.
        log.trace('Skipping this peak: tiny footprint / close to edge')
        pkres.setTinyFootprint()
        return

    # find other peaks within range...
    otherpeaks = []
    for pk2, pkF2 in zip(peaks, peaksF):
        if pk2 == pk:
            continue
        if pkF.distanceSquared(pkF2) > R2**2:
            continue
        opsfimg = psf.computeImage(pkF2.getX(), pkF2.getY())
        if not opsfimg.getBBox(afwImage.LOCAL).overlaps(stampbb):
            continue
        otherpeaks.append(opsfimg)
        log.trace('%i other peaks within range', len(otherpeaks))

    # Now we are going to do a least-squares fit for the flux in this
    # PSF, plus a decenter term, a linear sky, and fluxes of nearby
    # sources (assumed point sources).  Build up the matrix...
    # Number of terms -- PSF flux, constant sky, X, Y, + other PSF fluxes
    NT1 = 4 + len(otherpeaks)
    # + PSF dx, dy
    NT2 = NT1 + 2
    # Number of pixels -- at most
    NP = (1 + yhi - ylo)*(1 + xhi - xlo)
    # indices of columns in the "A" matrix.
    I_psf = 0
    I_sky = 1
    I_sky_ramp_x = 2
    I_sky_ramp_y = 3
    # offset of other psf fluxes:
    I_opsf = 4
    I_dx = NT1 + 0
    I_dy = NT1 + 1

    # Build the matrix "A", rhs "b" and weight "w".
    ix0, iy0 = img.getX0(), img.getY0()
    fx0, fy0 = fbb.getMinX(), fbb.getMinY()
    fslice = (slice(ylo-fy0, yhi-fy0+1), slice(xlo-fx0, xhi-fx0+1))
    islice = (slice(ylo-iy0, yhi-iy0+1), slice(xlo-ix0, xhi-ix0+1))
    fmask_sub = fmask .getArray()[fslice]
    var_sub = varimg.getArray()[islice]
    img_sub = img.getArray()[islice]

    # Clip the PSF image to match its bbox
    psfarr = psfimg.getArray()[pbb.getMinY()-py0: 1+pbb.getMaxY()-py0,
                               pbb.getMinX()-px0: 1+pbb.getMaxX()-px0]
    px0, px1 = pbb.getMinX(), pbb.getMaxX()
    py0, py1 = pbb.getMinY(), pbb.getMaxY()

    # Compute the "valid" pixels within our region-of-interest
    valid = (fmask_sub > 0)
    xx, yy = np.arange(xlo, xhi+1), np.arange(ylo, yhi+1)
    RR = ((xx - cx)**2)[np.newaxis, :] + ((yy - cy)**2)[:, np.newaxis]
    valid *= (RR <= R1**2)
    valid *= (var_sub > 0)
    NP = valid.sum()

    if NP == 0:
        log.warn('Skipping peak at (%.1f, %.1f): no unmasked pixels nearby', cx, cy)
        pkres.setNoValidPixels()
        return

    # pixel coords of valid pixels
    XX, YY = np.meshgrid(xx, yy)
    ipixes = np.vstack((XX[valid] - xlo, YY[valid] - ylo)).T

    inpsfx = (xx >= px0)*(xx <= px1)
    inpsfy = (yy >= py0)*(yy <= py1)
    inpsf = np.outer(inpsfy, inpsfx)
    indx = np.outer(inpsfy, (xx > px0)*(xx < px1))
    indy = np.outer((yy > py0)*(yy < py1), inpsfx)

    del inpsfx
    del inpsfy

    def _overlap(xlo, xhi, xmin, xmax):
        assert((xlo <= xmax) and (xhi >= xmin) and
               (xlo <= xhi) and (xmin <= xmax))
        xloclamp = max(xlo, xmin)
        Xlo = xloclamp - xlo
        xhiclamp = min(xhi, xmax)
        Xhi = Xlo + (xhiclamp - xloclamp)
        assert(xloclamp >= 0)
        assert(Xlo >= 0)
        return (xloclamp, xhiclamp+1, Xlo, Xhi+1)

    A = np.zeros((NP, NT2))
    # Constant term
    A[:, I_sky] = 1.
    # Sky slope terms: dx, dy
    A[:, I_sky_ramp_x] = ipixes[:, 0] + (xlo-cx)
    A[:, I_sky_ramp_y] = ipixes[:, 1] + (ylo-cy)

    # whew, grab the valid overlapping PSF pixels
    px0, px1 = pbb.getMinX(), pbb.getMaxX()
    py0, py1 = pbb.getMinY(), pbb.getMaxY()
    sx1, sx2, sx3, sx4 = _overlap(xlo, xhi, px0, px1)
    sy1, sy2, sy3, sy4 = _overlap(ylo, yhi, py0, py1)
    dpx0, dpy0 = px0 - xlo, py0 - ylo
    psf_y_slice = slice(sy3 - dpy0, sy4 - dpy0)
    psf_x_slice = slice(sx3 - dpx0, sx4 - dpx0)
    psfsub = psfarr[psf_y_slice, psf_x_slice]
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
    A[inpsf[valid], I_psf] = psfsub[vsub]

    # PSF dx -- by taking the half-difference of shifted-by-one and
    # shifted-by-minus-one.
    oldsx = (sx1, sx2, sx3, sx4)
    sx1, sx2, sx3, sx4 = _overlap(xlo, xhi, px0+1, px1-1)
    psfsub = (psfarr[psf_y_slice, sx3 - dpx0 + 1: sx4 - dpx0 + 1] -
              psfarr[psf_y_slice, sx3 - dpx0 - 1: sx4 - dpx0 - 1])/2.
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
    A[indx[valid], I_dx] = psfsub[vsub]
    # revert x indices...
    (sx1, sx2, sx3, sx4) = oldsx

    # PSF dy
    sy1, sy2, sy3, sy4 = _overlap(ylo, yhi, py0+1, py1-1)
    psfsub = (psfarr[sy3 - dpy0 + 1: sy4 - dpy0 + 1, psf_x_slice] -
              psfarr[sy3 - dpy0 - 1: sy4 - dpy0 - 1, psf_x_slice])/2.
    vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
    A[indy[valid], I_dy] = psfsub[vsub]

    # other PSFs...
    for j, opsf in enumerate(otherpeaks):
        obb = opsf.getBBox()
        ino = np.outer((yy >= obb.getMinY())*(yy <= obb.getMaxY()),
                       (xx >= obb.getMinX())*(xx <= obb.getMaxX()))
        dpx0, dpy0 = obb.getMinX() - xlo, obb.getMinY() - ylo
        sx1, sx2, sx3, sx4 = _overlap(xlo, xhi, obb.getMinX(), obb.getMaxX())
        sy1, sy2, sy3, sy4 = _overlap(ylo, yhi, obb.getMinY(), obb.getMaxY())
        opsfarr = opsf.getArray()
        psfsub = opsfarr[sy3 - dpy0: sy4 - dpy0, sx3 - dpx0: sx4 - dpx0]
        vsub = valid[sy1-ylo: sy2-ylo, sx1-xlo: sx2-xlo]
        A[ino[valid], I_opsf + j] = psfsub[vsub]

    b = img_sub[valid]

    # Weights -- from ramp and image variance map.
    # Ramp weights -- from 1 at R0 down to 0 at R1.
    rw = np.ones_like(RR)
    ii = (RR > R0**2)
    rr = np.sqrt(RR[ii])
    rw[ii] = np.maximum(0, 1. - ((rr - R0)/(R1 - R0)))
    w = np.sqrt(rw[valid]/var_sub[valid])
    # save the effective number of pixels
    sumr = np.sum(rw[valid])
    log.debug('sumr = %g', sumr)

    del ii

    Aw = A*w[:, np.newaxis]
    bw = b*w

    if debugPlots:
        import pylab as plt
        plt.clf()
        N = NT2 + 2
        R, C = 2, (N+1)/2
        for i in range(NT2):
            im1 = np.zeros((1+yhi-ylo, 1+xhi-xlo))
            im1[ipixes[:, 1], ipixes[:, 0]] = A[:, i]
            plt.subplot(R, C, i+1)
            plt.imshow(im1, interpolation='nearest', origin='lower')
        plt.subplot(R, C, NT2+1)
        im1 = np.zeros((1+yhi-ylo, 1+xhi-xlo))
        im1[ipixes[:, 1], ipixes[:, 0]] = b
        plt.imshow(im1, interpolation='nearest', origin='lower')
        plt.subplot(R, C, NT2+2)
        im1 = np.zeros((1+yhi-ylo, 1+xhi-xlo))
        im1[ipixes[:, 1], ipixes[:, 0]] = w
        plt.imshow(im1, interpolation='nearest', origin='lower')
        plt.savefig('A.png')

    # We do fits with and without the decenter (dx,dy) terms.
    # Since the dx,dy terms are at the end of the matrix,
    # we can do that just by trimming off those elements.
    #
    # The SVD can fail if there are NaNs in the matrices; this should
    # really be handled upstream
    try:
        # NT1 is number of terms without dx,dy;
        # X1 is the result without decenter
        X1, r1, rank1, s1 = np.linalg.lstsq(Aw[:, :NT1], bw)
        # X2 is with decenter
        X2, r2, rank2, s2 = np.linalg.lstsq(Aw, bw)
    except np.linalg.LinAlgError as e:
        log.warn("Failed to fit PSF to child: %s", e)
        pkres.setPsfFitFailed()
        return

    log.debug('r1 r2 %s %s', r1, r2)

    # r is weighted chi-squared = sum over pixels: ramp * (model -
    # data)**2/sigma**2
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
    log.debug('dof1, dof2 %g %g', dof1, dof2)

    # This can happen if we're very close to the edge (?)
    if dof1 <= 0 or dof2 <= 0:
        log.trace('Skipping this peak: bad DOF %g, %g', dof1, dof2)
        pkres.setBadPsfDof()
        return

    q1 = chisq1/dof1
    q2 = chisq2/dof2
    log.trace('PSF fits: chisq/dof = %g, %g', q1, q2)
    ispsf1 = (q1 < psfChisqCut1)
    ispsf2 = (q2 < psfChisqCut2)

    pkres.psfFit1 = (chisq1, dof1)
    pkres.psfFit2 = (chisq2, dof2)

    # check that the fit PSF spatial derivative terms aren't too big
    if ispsf2:
        fdx, fdy = X2[I_dx], X2[I_dy]
        f0 = X2[I_psf]
        # as a fraction of the PSF flux
        dx = fdx/f0
        dy = fdy/f0
        ispsf2 = ispsf2 and (abs(dx) < 1. and abs(dy) < 1.)
        log.trace('isPSF2 -- checking derivatives: dx,dy = %g, %g -> %s', dx, dy, str(ispsf2))
        if not ispsf2:
            pkres.psfFitBigDecenter = True

    # Looks like a shifted PSF: try actually shifting the PSF by that amount
    # and re-evaluate the fit.
    if ispsf2:
        psfimg2 = psf.computeImage(cx + dx, cy + dy)
        # clip
        pbb2 = psfimg2.getBBox()
        pbb2.clip(fbb)

        # Make sure we haven't been given a substitute PSF that's nowhere near where we want, as may occur if
        # "Cannot compute CoaddPsf at point (xx,yy); no input images at that point."
        if not pbb2.contains(afwGeom.Point2I(int(cx + dx), int(cy + dy))):
            ispsf2 = False
        else:
            # clip image to bbox
            px0, py0 = psfimg2.getX0(), psfimg2.getY0()
            psfarr = psfimg2.getArray()[pbb2.getMinY()-py0:1+pbb2.getMaxY()-py0,
                                        pbb2.getMinX()-px0:1+pbb2.getMaxX()-px0]
            px0, py0 = pbb2.getMinX(), pbb2.getMinY()
            px1, py1 = pbb2.getMaxX(), pbb2.getMaxY()

            # yuck!  Update the PSF terms in the least-squares fit matrix.
            Ab = A[:, :NT1]

            sx1, sx2, sx3, sx4 = _overlap(xlo, xhi, px0, px1)
            sy1, sy2, sy3, sy4 = _overlap(ylo, yhi, py0, py1)
            dpx0, dpy0 = px0 - xlo, py0 - ylo
            psfsub = psfarr[sy3-dpy0:sy4-dpy0, sx3-dpx0:sx4-dpx0]
            vsub = valid[sy1-ylo:sy2-ylo, sx1-xlo:sx2-xlo]
            xx, yy = np.arange(xlo, xhi+1), np.arange(ylo, yhi+1)
            inpsf = np.outer((yy >= py0)*(yy <= py1), (xx >= px0)*(xx <= px1))
            Ab[inpsf[valid], I_psf] = psfsub[vsub]

            Aw = Ab*w[:, np.newaxis]
            # re-solve...
            Xb, rb, rankb, sb = np.linalg.lstsq(Aw, bw)
            if len(rb) > 0:
                chisqb = rb[0]
            else:
                chisqb = 1e30
            dofb = sumr - len(Xb)
            qb = chisqb/dofb
            ispsf2 = (qb < psfChisqCut2b)
            q2 = qb
            X2 = Xb
            log.trace('shifted PSF: new chisq/dof = %g; good? %s', qb, ispsf2)
            pkres.psfFit3 = (chisqb, dofb)

    # Which one do we keep?
    if (((ispsf1 and ispsf2) and (q2 < q1)) or
            (ispsf2 and not ispsf1)):
        Xpsf = X2
        chisq = chisq2
        dof = dof2
        log.debug('dof %g', dof)
        log.trace('Keeping shifted-PSF model')
        cx += dx
        cy += dy
        pkres.psfFitWithDecenter = True
    else:
        # (arbitrarily set to X1 when neither fits well)
        Xpsf = X1
        chisq = chisq1
        dof = dof1
        log.debug('dof %g', dof)
        log.trace('Keeping unshifted PSF model')

    ispsf = (ispsf1 or ispsf2)

    # Save the PSF models in images for posterity.
    if debugPsf:
        SW, SH = 1+xhi-xlo, 1+yhi-ylo
        psfmod = afwImage.ImageF(SW, SH)
        psfmod.setXY0(xlo, ylo)
        psfderivmodm = afwImage.MaskedImageF(SW, SH)
        psfderivmod = psfderivmodm.getImage()
        psfderivmod.setXY0(xlo, ylo)
        model = afwImage.ImageF(SW, SH)
        model.setXY0(xlo, ylo)
        for i in range(len(Xpsf)):
            for (x, y), v in zip(ipixes, A[:, i]*Xpsf[i]):
                ix, iy = int(x), int(y)
                model.set(ix, iy, model.get(ix, iy) + float(v))
                if i in [I_psf, I_dx, I_dy]:
                    psfderivmod.set(ix, iy, psfderivmod.get(ix, iy) + float(v))
        for ii in range(NP):
            x, y = ipixes[ii, :]
            psfmod.set(int(x), int(y), float(A[ii, I_psf]*Xpsf[I_psf]))
        modelfp = afwDet.Footprint(fp.getPeaks().getSchema())
        for (x, y) in ipixes:
            modelfp.addSpan(int(y+ylo), int(x+xlo), int(x+xlo))
        modelfp.normalize()

        pkres.psfFitDebugPsf0Img = psfimg
        pkres.psfFitDebugPsfImg = psfmod
        pkres.psfFitDebugPsfDerivImg = psfderivmod
        pkres.psfFitDebugPsfModel = model
        pkres.psfFitDebugStamp = img.Factory(img, stampbb, True)
        pkres.psfFitDebugValidPix = valid  # numpy array
        pkres.psfFitDebugVar = varimg.Factory(varimg, stampbb, True)
        ww = np.zeros(valid.shape, np.float)
        ww[valid] = w
        pkres.psfFitDebugWeight = ww  # numpy
        pkres.psfFitDebugRampWeight = rw

    # Save things we learned about this peak for posterity...
    pkres.psfFitR0 = R0
    pkres.psfFitR1 = R1
    pkres.psfFitStampExtent = (xlo, xhi, ylo, yhi)
    pkres.psfFitCenter = (cx, cy)
    log.debug('saving chisq,dof %g %g', chisq, dof)
    pkres.psfFitBest = (chisq, dof)
    pkres.psfFitParams = Xpsf
    pkres.psfFitFlux = Xpsf[I_psf]
    pkres.psfFitNOthers = len(otherpeaks)

    if ispsf:
        pkres.setDeblendedAsPsf()

        # replace the template image by the PSF + derivatives
        # image.
        log.trace('Deblending as PSF; setting template to PSF model')

        # Instantiate the PSF model and clip it to the footprint
        psfimg = psf.computeImage(cx, cy)
        # Scale by fit flux.
        psfimg *= Xpsf[I_psf]
        psfimg = psfimg.convertF()

        # Clip the Footprint to the PSF model image bbox.
        fpcopy = afwDet.Footprint(fp)
        psfbb = psfimg.getBBox()
        fpcopy.clipTo(psfbb)
        bb = fpcopy.getBBox()

        # Copy the part of the PSF model within the clipped footprint.
        psfmod = afwImage.ImageF(bb)
        afwDet.copyWithinFootprintImage(fpcopy, psfimg, psfmod)
        # Save it as our template.
        fpcopy.clipToNonzero(psfmod)
        fpcopy.normalize()
        pkres.setTemplate(psfmod, fpcopy)

        # DEBUG
        pkres.setPsfTemplate(psfmod, fpcopy)
    
    return ispsf

def buildSymmetricTemplates(debResult, log, patchEdges=False, setOrigTemplate=True):
    """Build a symmetric template for each peak in each filter

    Given ``maskedImageF``, ``footprint``, and a ``DebldendedPeak``, creates a symmetric template
    (``templateImage`` and ``templateFootprint``) around the peak for all peaks not flagged as
    ``skip`` or ``deblendedAsPsf``.
    
    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.
    patchEdges: `bool`, optional
        If True and if the parent Footprint touches pixels with the ``EDGE`` bit set,
        then grow the parent Footprint to include all symmetric templates.

    Returns
    -------
    modified: `bool`
        If any peaks are not skipped or marked as point sources, ``modified`` is ``True.
        Otherwise ``modified`` is ``False``.
    """
    modified = False
    # Create the Templates for each peak in each filter
    for fidx in debResult.filters:
        dp = debResult.deblendedParents[fidx]
        imbb = dp.img.getBBox()
        log.trace('Creating templates for footprint at x0,y0,W,H = %i, %i, %i, %i)', dp.x0, dp.y0, dp.W, dp.H)

        for peaki, pkres in enumerate(dp.peaks):
            log.trace('Deblending peak %i of %i', peaki, len(dp.peaks))
            # TODO: Check debResult to see if the peak is deblended as a point source
            # when comparing all bands, not just a single band
            if pkres.skip or pkres.deblendedAsPsf:
                continue
            modified = True
            pk = pkres.peak
            cx, cy = pk.getIx(), pk.getIy()
            if not imbb.contains(afwGeom.Point2I(cx, cy)):
                log.trace('Peak center is not inside image; skipping %i', pkres.pki)
                pkres.setOutOfBounds()
                continue
            log.trace('computing template for peak %i at (%i, %i)', pkres.pki, cx, cy)
            timg, tfoot, patched = butils.buildSymmetricTemplate(dp.maskedImage, dp.fp, pk, dp.avgNoise,
                                                       True, patchEdges)
            if timg is None:
                log.trace('Peak %i at (%i, %i): failed to build symmetric template', pkres.pki, cx, cy)
                pkres.setFailedSymmetricTemplate()
                continue

            if patched:
                pkres.setPatched()

            # possibly save the original symmetric template
            if setOrigTemplate:
                pkres.setOrigTemplate(timg, tfoot)
            pkres.setTemplate(timg, tfoot)
    return modified

def rampFluxAtEdge(debResult, log, patchEdges=False):
    """Adjust flux on the edges of the template footprints.
    
    Using the PSF, a peak ``Footprint`` with pixels on the edge of ``footprint``
    is grown by the ``psffwhm``*1.5 and filled in with ramped pixels.
    The result is a new symmetric footprint template for the peaks near the edge.
    
    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.
    patchEdges: `bool`, optional
        If True and if the parent Footprint touches pixels with the ``EDGE`` bit set,
        then grow the parent Footprint to include all symmetric templates.

    Returns
    -------
    modified: `bool`
        If any peaks have their templates modified to include flux at the edges,
        ``modified`` is ``True``.
    """
    modified = False
    # Loop over all filters
    for fidx in debResult.filters:
        dp = debResult.deblendedParents[fidx]
        log.trace('Checking for significant flux at edge: sigma1=%g', dp.avgNoise)
        
        for peaki, pkres in enumerate(dp.peaks):
            if pkres.skip or pkres.deblendedAsPsf:
                continue
            timg, tfoot = pkres.templateImage, pkres.templateFootprint
            if butils.hasSignificantFluxAtEdge(timg, tfoot, 3*dp.avgNoise):
                log.trace("Template %i has significant flux at edge: ramping", pkres.pki)
                try:
                    (timg2, tfoot2, patched) = _handle_flux_at_edge(log, dp.psffwhm, timg, tfoot, dp.fp,
                                                                    dp.maskedImage, dp.x0, dp.x1,
                                                                    dp.y0, dp.y1, dp.psf, pkres.peak,
                                                                    dp.avgNoise, patchEdges)
                except lsst.pex.exceptions.Exception as exc:
                    if (isinstance(exc, lsst.pex.exceptions.InvalidParameterError)
                            and "CoaddPsf" in str(exc)):
                        pkres.setOutOfBounds()
                        continue
                    raise
                pkres.setRampedTemplate(timg2, tfoot2)
                if patched:
                    pkres.setPatched()
                pkres.setTemplate(timg2, tfoot2)
                modified = True
    return modified

def _handle_flux_at_edge(log, psffwhm, t1, tfoot, fp, maskedImage,
                         x0, x1, y0, y1, psf, pk, sigma1, patchEdges
    ):
    """Extend a template by the PSF to fill in the footprint.

    Using the PSF, a footprint that touches the edge is passed to the function
    and is grown by the psffwhm*1.5 and filled in with ramped pixels.

    Parameters
    ----------
    log: `log.Log`
        LSST logger for logging purposes.
    psffwhm: `float`
        PSF FWHM in pixels.
    t1: `afw.image.ImageF`
        The image template that contains the footprint to extend.
    tfoot: `afw.detection.Footprint`
        Symmetric Footprint to extend.
    fp: `afw.detection.Footprint`
        Parent Footprint that is being deblended.
    maskedImage: `afw.image.MaskedImageF`
        Full MaskedImage containing the parent footprint ``fp``.
    x0,y0: `init`
        Minimum x,y for the bounding box of the footprint ``fp``.
    x1,y1: `int`
        Maximum x,y for the bounding box of the footprint ``fp``.
    psf: `afw.detection.Psf`
        PSF of the image.
    pk: `afw.detection.PeakRecord`
        The peak within the Footprint whose footprint is being extended.
    sigma1: `float`
        Estimated noise level in the image.
    patchEdges: `bool`
        If ``patchEdges==True`` and if the footprint touches pixels with the
        ``EDGE`` bit set, then for spans whose symmetric mirror are outside the
        image, the symmetric footprint is grown to include them and their
        pixel values are stored.

    Results
    -------
    t2: `afw.image.ImageF`
        Image of the extended footprint.
    tfoot2: `afw.detection.Footprint`
        Extended Footprint.
    patched: `bool`
        If the footprint touches an edge pixel, ``patched`` will be set to ``True``.
        Otherwise ``patched`` is ``False``.
    """
    log.trace('Found significant flux at template edge.')
    # Compute the max of:
    #  -symmetric-template-clipped image * PSF
    #  -footprint-clipped image
    # Ie, extend the template by the PSF and "fill in" the footprint.
    # Then find the symmetric template of that image.

    # The size we'll grow by
    S = psffwhm*1.5
    # make it an odd integer
    S = int((S + 0.5)/2)*2 + 1

    tbb = tfoot.getBBox()
    tbb.grow(S)

    # (footprint+margin)-clipped image;
    # we need the pixels OUTSIDE the footprint to be 0.
    fpcopy = afwDet.Footprint(fp)
    fpcopy = afwDet.growFootprint(fpcopy, S)
    fpcopy.clipTo(tbb)
    fpcopy.normalize()
    padim = maskedImage.Factory(tbb)
    afwDet.copyWithinFootprintMaskedImage(fpcopy, maskedImage, padim)

    # find pixels on the edge of the template
    edgepix = butils.getSignificantEdgePixels(t1, tfoot, -1e6)

    # instantiate PSF image
    xc = int((x0 + x1)/2)
    yc = int((y0 + y1)/2)
    psfim = psf.computeImage(afwGeom.Point2D(xc, yc))
    pbb = psfim.getBBox()
    # shift PSF image to be centered on zero
    lx, ly = pbb.getMinX(), pbb.getMinY()
    psfim.setXY0(lx - xc, ly - yc)
    pbb = psfim.getBBox()
    # clip PSF to S, if necessary
    Sbox = afwGeom.Box2I(afwGeom.Point2I(-S, -S), afwGeom.Extent2I(2*S+1, 2*S+1))
    if not Sbox.contains(pbb):
        # clip PSF image
        psfim = psfim.Factory(psfim, Sbox, afwImage.PARENT, True)
        pbb = psfim.getBBox()
    px0 = pbb.getMinX()
    px1 = pbb.getMaxX()
    py0 = pbb.getMinY()
    py1 = pbb.getMaxY()

    # Compute the ramped-down edge pixels
    ramped = t1.Factory(tbb)
    Tout = ramped.getArray()
    Tin = t1.getArray()
    tx0, ty0 = t1.getX0(), t1.getY0()
    ox0, oy0 = ramped.getX0(), ramped.getY0()
    P = psfim.getArray()
    P /= P.max()
    # For each edge pixel, Tout = max(Tout, edgepix * PSF)
    for span in edgepix.getSpans():
        y = span.getY()
        for x in range(span.getX0(), span.getX1()+1):
            slc = (slice(y+py0 - oy0, y+py1+1 - oy0),
                   slice(x+px0 - ox0, x+px1+1 - ox0))
            Tout[slc] = np.maximum(Tout[slc], Tin[y-ty0, x-tx0]*P)

    # Fill in the "padim" (which has the right variance and
    # mask planes) with the ramped pixels, outside the footprint
    I = (padim.getImage().getArray() == 0)
    padim.getImage().getArray()[I] = ramped.getArray()[I]

    t2, tfoot2, patched = butils.buildSymmetricTemplate(padim, fpcopy, pk, sigma1, True, patchEdges)

    # This template footprint may extend outside the parent
    # footprint -- or the image.  Clip it.
    # NOTE that this may make it asymmetric, unlike normal templates.
    imbb = maskedImage.getBBox()
    tfoot2.clipTo(imbb)
    tbb = tfoot2.getBBox()
    # clip template image to bbox
    t2 = t2.Factory(t2, tbb, afwImage.PARENT, True)

    return t2, tfoot2, patched

def medianSmoothTemplates(debResult, log, medianFilterHalfsize=2):
    """Applying median smoothing filter to the template images for every peak in every filter.
    
    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.
    medianFilterHalfSize: `int`, optional
        Half the box size of the median filter, i.e. a ``medianFilterHalfSize`` of 50 means that
        each output pixel will be the median of  the pixels in a 101 x 101-pixel box in the input image.
        This parameter is only used when ``medianSmoothTemplate==True``, otherwise it is ignored.

    Returns
    -------
    modified: `bool`
        Whether or not any templates were modified.
        This will be ``True`` as long as there is at least one source that is not flagged as a PSF.
    """
    modified = False
    # Loop over all filters
    for fidx in debResult.filters:
        dp = debResult.deblendedParents[fidx]
        for peaki, pkres in enumerate(dp.peaks):
            if pkres.skip or pkres.deblendedAsPsf:
                continue
            modified = True
            timg, tfoot = pkres.templateImage, pkres.templateFootprint
            filtsize = medianFilterHalfsize*2 + 1
            if timg.getWidth() >= filtsize and timg.getHeight() >= filtsize:
                log.trace('Median filtering template %i', pkres.pki)
                # We want the output to go in "t1", so copy it into
                # "inimg" for input
                inimg = timg.Factory(timg, True)
                butils.medianFilter(inimg, timg, medianFilterHalfsize)
                # possible save this median-filtered template
                pkres.setMedianFilteredTemplate(timg, tfoot)
            else:
                log.trace('Not median-filtering template %i: size %i x %i smaller than required %i x %i',
                          pkres.pki, timg.getWidth(), timg.getHeight(), filtsize, filtsize)
            pkres.setTemplate(timg, tfoot)
    return modified

def makeTemplatesMonotonic(debResult, log):
    """Make the templates monotonic.

    The pixels in the templates are modified such that pixels further from the peak will
    have values smaller than those closer to the peak.

    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.

    Returns
    -------
    modified: `bool`
        Whether or not any templates were modified.
        This will be ``True`` as long as there is at least one source that is not flagged as a PSF.
    """
    modified = False
    # Loop over all filters
    for fidx in debResult.filters:
        dp = debResult.deblendedParents[fidx]
        for peaki, pkres in enumerate(dp.peaks):
            if pkres.skip or pkres.deblendedAsPsf:
                continue
            modified = True
            timg, tfoot = pkres.templateImage, pkres.templateFootprint
            pk = pkres.peak
            log.trace('Making template %i monotonic', pkres.pki)
            butils.makeMonotonic(timg, pk)
            pkres.setTemplate(timg, tfoot)
    return modified

def clipFootprintsToNonzero(debResult, log):
    """Clip non-zero spans in the template footprints for every peak in each filter.
    
    Peak ``Footprint``s are clipped to the region in the image containing non-zero values
    by dropping spans that are completely zero and moving endpoints to non-zero pixels
    (but does not split spans that have internal zeros).
    
    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.

    Returns
    -------
    modified: `bool`
        Whether or not any templates were modified.
        This will be ``True`` as long as there is at least one source that is not flagged as a PSF.
    """
    modified = False
    # Loop over all filters
    for fidx in debResult.filters:
        dp = debResult.deblendedParents[fidx]
        for peaki, pkres in enumerate(dp.peaks):
            if pkres.skip or pkres.deblendedAsPsf:
                continue
            modified = True
            timg, tfoot = pkres.templateImage, pkres.templateFootprint
            tfoot.clipToNonzero(timg)
            tfoot.normalize()
            if not tfoot.getBBox().isEmpty() and tfoot.getBBox() != timg.getBBox(afwImage.PARENT):
                timg = timg.Factory(timg, tfoot.getBBox(), afwImage.PARENT, True)
            pkres.setTemplate(timg, tfoot)
    return False

def weightTemplates(debResult, log):
    """Weight the templates to best fit the observed image in each filter
    
    This function re-weights the templates so that their linear combination best represents
    the observed image in that filter.
    In the future it may be useful to simultaneously weight all of the filters together.
    
    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.

    Returns
    -------
    modified: `bool`
        ``weightTemplates`` does not actually modify the ``Footprint`` templates other than
        to add a weight to them, so ``modified`` is always ``False``.
    """
    # Weight the templates by doing a least-squares fit to the image
    log.trace('Weighting templates')
    for fidx in debResult.filters:
        _weightTemplates(debResult.deblendedParents[fidx])
    return False

def _weightTemplates(dp):
    """Weight the templates to best match the parent Footprint in a single filter

    This includes weighting both regular templates and point source templates
    
    Parameter
    ---------
    dp: `DeblendedParent`
        The deblended parent to re-weight
    
    Returns
    -------
    None
    """
    nchild = np.sum([pkres.skip is False for pkres in dp.peaks])
    A = np.zeros((dp.W*dp.H, nchild))
    parentImage = afwImage.ImageF(dp.bb)
    afwDet.copyWithinFootprintImage(dp.fp, dp.img, parentImage)
    b = parentImage.getArray().ravel()

    index = 0
    for pkres in dp.peaks:
        if pkres.skip:
            continue
        childImage = afwImage.ImageF(dp.bb)
        afwDet.copyWithinFootprintImage(dp.fp, pkres.templateImage, childImage)
        A[:, index] = childImage.getArray().ravel()
        index += 1

    X1, r1, rank1, s1 = np.linalg.lstsq(A, b)
    del A
    del b

    index = 0
    for pkres in dp.peaks:
        if pkres.skip:
            continue
        pkres.templateImage *= X1[index]
        pkres.setTemplateWeight(X1[index])
        index += 1

def reconstructTemplates(debResult, log, maxTempDotProd=0.5):
    """Remove "degenerate templates"
    
    If galaxies have substructure, such as face-on spirals, the process of identifying peaks can
    "shred" the galaxy into many pieces. The templates of shredded galaxies are typically quite
    similar because they represent the same galaxy, so we try to identify these "degenerate" peaks
    by looking at the inner product (in pixel space) of pairs of templates.
    If they are nearly parallel, we only keep one of the peaks and reject the other.
    If only one of the peaks is a PSF template, the other template is used,
    otherwise the one with the maximum template value is kept.
    
    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.
    maxTempDotProd: `float`, optional
        All dot products between templates greater than ``maxTempDotProd`` will result in one
        of the templates removed.

    Returns
    -------
    modified: `bool`
        If any degenerate templates are found, ``modified`` is ``True``.
    """
    log.trace('Looking for degnerate templates')

    foundReject = False
    for fidx in debResult.filters:
        dp = debResult.deblendedParents[fidx]
        nchild = np.sum([pkres.skip is False for pkres in dp.peaks])
        indexes = [pkres.pki for pkres in dp.peaks if pkres.skip is False]

        # We build a matrix that stores the dot product between templates.
        # We convert the template images to HeavyFootprints because they already have a method
        # to compute the dot product.
        A = np.zeros((nchild, nchild))
        maxTemplate = []
        heavies = []
        for pkres in dp.peaks:
            if pkres.skip:
                continue
            heavies.append(afwDet.makeHeavyFootprint(pkres.templateFootprint,
                                                     afwImage.MaskedImageF(pkres.templateImage)))
            maxTemplate.append(np.max(pkres.templateImage.getArray()))

        for i in range(nchild):
            for j in range(i + 1):
                A[i, j] = heavies[i].dot(heavies[j])

        # Normalize the dot products to get the cosine of the angle between templates
        for i in range(nchild):
            for j in range(i):
                norm = A[i, i]*A[j, j]
                if norm <= 0:
                    A[i, j] = 0
                else:
                    A[i, j] /= np.sqrt(norm)

        # Iterate over pairs of objects and find the maximum non-diagonal element of the matrix.
        # Exit the loop once we find a single degenerate pair greater than the threshold.
        rejectedIndex = -1
        for i in range(nchild):
            currentMax = 0.
            for j in range(i):
                if A[i, j] > currentMax:
                    currentMax = A[i, j]
                    if currentMax > maxTempDotProd:
                        foundReject = True
                        rejectedIndex = j

            if foundReject:
                break

        del A

        # If one of the objects is identified as a PSF keep the other one, otherwise keep the one
        # with the maximum template value
        if foundReject:
            keep = indexes[i]
            reject = indexes[rejectedIndex]
            exitLoop = False
            if dp.peaks[keep].deblendedAsPsf and dp.peaks[reject].deblendedAsPsf is False:
                keep = indexes[rejectedIndex]
                reject = indexes[i]
            elif dp.peaks[keep].deblendedAsPsf is False and dp.peaks[reject].deblendedAsPsf:
                reject = indexes[rejectedIndex]
                keep = indexes[i]
            else:
                if maxTemplate[rejectedIndex] > maxTemplate[i]:
                        keep = indexes[rejectedIndex]
                        reject = indexes[i]
            log.trace('Removing object with index %d : %f.  Degenerate with %d' % (reject, currentMax,
                                                                                   keep))
            dp.peaks[reject].skip = True
            dp.peaks[reject].degenerate = True
    
    return foundReject

def apportionFlux(debResult, log, assignStrayFlux=True, strayFluxAssignment='r-to-peak',
                  strayFluxToPointSources='necessary', clipStrayFluxFraction=0.001, 
                  getTemplateSum=False):
    """Apportion flux to all of the peak templates in each filter

    Divide the ``maskedImage`` flux amongst all of the templates based on the fraction of 
    flux assigned to each ``template``.
    Leftover "stray flux" is assigned to peaks based on the other parameters.

    Parameters
    ----------
    debResult: `lsst.meas.deblender.baseline.DeblenderResult`
        Container for the final deblender results.
    log: `log.Log`
        LSST logger for logging purposes.
    assignStrayFlux: `bool`, optional
        If True then flux in the parent footprint that is not covered by any of the
        template footprints is assigned to templates based on their 1/(1+r^2) distance.
        How the flux is apportioned is determined by ``strayFluxAssignment``.
    strayFluxAssignment: `string`, optional
        Determines how stray flux is apportioned.
        * ``trim``: Trim stray flux and do not include in any footprints
        * ``r-to-peak`` (default): Stray flux is assigned based on (1/(1+r^2) from the peaks
        * ``r-to-footprint``: Stray flux is distributed to the footprints based on 1/(1+r^2) of the
          minimum distance from the stray flux to footprint
        * ``nearest-footprint``: Stray flux is assigned to the footprint with lowest L-1 (Manhattan)
          distance to the stray flux
    strayFluxToPointSources: `string`, optional
        Determines how stray flux is apportioned to point sources
        * ``never``: never apportion stray flux to point sources
        * ``necessary`` (default): point sources are included only if there are no extended sources nearby
        * ``always``: point sources are always included in the 1/(1+r^2) splitting
    clipStrayFluxFraction: `float`, optional
        Minimum stray-flux portion.
        Any stray-flux portion less than ``clipStrayFluxFraction`` is clipped to zero.
    getTemplateSum: `bool`, optional
        As part of the flux calculation, the sum of the templates is calculated.
        If ``getTemplateSum==True`` then the sum of the templates is stored in the result
        (a `DeblendedFootprint`).

    Returns
    -------
    modified: `bool`
        Apportion flux always modifies the templates, so ``modified`` is always ``True``.
        However, this should likely be the final step and it is unlikely that 
        any deblender plugins will be re-run.
    """
    validStrayPtSrc = ['never', 'necessary', 'always']
    validStrayAssign = ['r-to-peak', 'r-to-footprint', 'nearest-footprint', 'trim']
    if strayFluxToPointSources not in validStrayPtSrc:
        raise ValueError((('strayFluxToPointSources: value \"%s\" not in the set of allowed values: ') %
                          strayFluxToPointSources) + str(validStrayPtSrc))
    if strayFluxAssignment not in validStrayAssign:
        raise ValueError((('strayFluxAssignment: value \"%s\" not in the set of allowed values: ') %
                          strayFluxAssignment) + str(validStrayAssign))

    for fidx in debResult.filters:
        dp = debResult.deblendedParents[fidx]
        # Prepare inputs to "apportionFlux" call.
        # template maskedImages
        tmimgs = []
        # template footprints
        tfoots = []
        # deblended as psf
        dpsf = []
        # peak x,y
        pkx = []
        pky = []
        # indices of valid templates
        ibi = []
        bb = dp.fp.getBBox()

        for peaki, pkres in enumerate(dp.peaks):
            if pkres.skip:
                continue
            tmimgs.append(pkres.templateImage)
            tfoots.append(pkres.templateFootprint)
            # for stray flux...
            dpsf.append(pkres.deblendedAsPsf)
            pk = pkres.peak
            pkx.append(pk.getIx())
            pky.append(pk.getIy())
            ibi.append(pkres.pki)

        # Now apportion flux according to the templates
        log.trace('Apportioning flux among %i templates', len(tmimgs))
        sumimg = afwImage.ImageF(bb)
        # .getDimensions())
        # sumimg.setXY0(bb.getMinX(), bb.getMinY())

        strayopts = 0
        if strayFluxAssignment == 'trim':
            assignStrayFlux = False
            strayopts |= butils.STRAYFLUX_TRIM
        if assignStrayFlux:
            strayopts |= butils.ASSIGN_STRAYFLUX
            if strayFluxToPointSources == 'necessary':
                strayopts |= butils.STRAYFLUX_TO_POINT_SOURCES_WHEN_NECESSARY
            elif strayFluxToPointSources == 'always':
                strayopts |= butils.STRAYFLUX_TO_POINT_SOURCES_ALWAYS

            if strayFluxAssignment == 'r-to-peak':
                # this is the default
                pass
            elif strayFluxAssignment == 'r-to-footprint':
                strayopts |= butils.STRAYFLUX_R_TO_FOOTPRINT
            elif strayFluxAssignment == 'nearest-footprint':
                strayopts |= butils.STRAYFLUX_NEAREST_FOOTPRINT

        portions, strayflux = butils.apportionFlux(dp.maskedImage, dp.fp, tmimgs, tfoots, sumimg, dpsf,
                                        pkx, pky, strayopts, clipStrayFluxFraction)

        # Shrink parent to union of children
        if strayFluxAssignment == 'trim':
            dp.fp.include(tfoots, True)

        # Store the template sum in the deblender result
        if getTemplateSum:
            debResult.setTemplateSums(sumimg, fidx)
        
        # Save the apportioned fluxes
        ii = 0
        print("STRAY FLUX", strayflux)
        for j, (pk, pkres) in enumerate(zip(dp.fp.getPeaks(), dp.peaks)):
            if pkres.skip:
                continue
            pkres.setFluxPortion(portions[ii])

            if assignStrayFlux:
                # NOTE that due to a swig bug (https://github.com/swig/swig/issues/59)
                # we CANNOT iterate over "strayflux", but must index into it.
                stray = strayflux[ii]
            else:
                stray = None
            ii += 1

            pkres.setStrayFlux(stray)

        # Set child footprints to contain the right number of peaks.
        for j, (pk, pkres) in enumerate(zip(dp.fp.getPeaks(), dp.peaks)):
            if pkres.skip:
                continue

            for foot, add in [(pkres.templateFootprint, True), (pkres.origFootprint, True),
                              (pkres.strayFlux, False)]:
                if foot is None:
                    continue
                pks = foot.getPeaks()
                pks.clear()
                if add:
                    pks.append(pk)
    return True