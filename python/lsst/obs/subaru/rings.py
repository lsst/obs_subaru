"""
Process images using Kawanomoto-san's clever d(lnI)/dr algorithm.

E.g.

import lsst.daf.persistence as dafPersist
import lsst.afw.cameraGeom.utils as cgUtils
import lsst.obs.subaru.rings as rings

butler = dafPersist.Butler("/home/astro/hsc/hsc/SUPA")

bin=64

frame = 1
visit = 129102
im = cgUtils.showCamera(butler.get("camera"), rings.LnGradImage(butler, bin=bin, visit=visit, verbose=True),
                        frame=frame, bin=bin, title=visit, overlay=True)

grad = {}
if False:
   grad[visit] = rings.radialProfile(butler, visit=visit, bin=bin)
else:
   # This has the side effect of reading the visits and plotting
   r, profs = rings.profilePca(butler, visits=[129085, 129102], bin=bin, grad=grad, plotProfile=True)

rings.plotRadial(r, profs, xlim=(-100, 5500), ylim=(0.8, 1.03))
"""
from __future__ import absolute_import, division, print_function
from builtins import next
from builtins import range
from builtins import object

import multiprocessing
import os
import re
import sys

import numpy as np

import lsst.afw.cameraGeom.utils as cgUtils
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.display.ds9 as ds9
import lsst.afw.display.utils as ds9Utils
import lsst.analysis.utils as utils

try:
    pyplot
except NameError:
    import matplotlib.pyplot as pyplot
    pyplot.interactive(1)


def fitPlane(mi, niter=3, tol=1e-5, nsigma=5, returnResidualImage=False):
    """Fit a plane to the image im using a linear fit with an nsigma clip"""

    width, height = mi.getDimensions()
    BAD = afwImage.Mask.getPlaneBitMask("BAD")

    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    X, Y = X.flatten(), Y.flatten()
    one = np.ones_like(X)

    A = np.array([one, X, Y]).transpose()
    isMaskedImage = hasattr(mi, "getImage")
    im = mi.getImage() if isMaskedImage else mi
    ima = im.getArray().flatten()

    sctrl = afwMath.StatisticsControl()
    sctrl.setAndMask(afwImage.Mask.getPlaneBitMask("BAD"))
    z, dzdx, dzdy = afwMath.makeStatistics(mi, afwMath.MEANCLIP, sctrl).getValue(), 0, 0  # initial guess

    returnResidualImage = True
    if returnResidualImage:
        res = mi.clone()                # residuals from plane fit
        resim = res.getImage() if isMaskedImage else res

    for i in range(niter):
        plane = z + dzdx*X + dzdy*Y

        resim.getArray()[:] = im.getArray() - plane.reshape(height, width)

        if isMaskedImage:
            stats = afwMath.makeStatistics(res, afwMath.STDEVCLIP, sctrl)
            stdev = stats.getValue(afwMath.STDEVCLIP)
            good = np.where(np.logical_and(np.abs(res.getImage().getArray().flatten()) < nsigma*stdev,
                                           np.bitwise_and(res.getMask().getArray().flatten(), BAD) == 0))[0]
            b, residuals = np.linalg.lstsq(A[good], ima[good])[:2]
        else:
            b, residuals = np.linalg.lstsq(A, ima)[:2]

        if np.all(residuals < tol):
            break

        z, dzdx, dzdy = b

    if returnResidualImage:             # need to update with latest values
        plane = z + dzdx*X + dzdy*Y
        resim.getArray()[:] = im.getArray() - plane.reshape(height, width)

        return b, res
    else:
        return b, None


def fitPatches(exp, nx=4, ny=8, bin=None, frame=None, returnResidualImage=False,
               r=None, lnGrad=None, theta=None):
    """Fit planes to a set of patches of an image im

    If r, theta, and lnGrad are provided they should be lists (more accurately, support .append),
    and they have the values of r, theta, and dlnI/dr from this image appended.
    """

    width, height = exp.getDimensions()

    if bin is not None:
        nx, ny = width//bin, height//bin

    xsize = width//nx
    ysize = height//ny

    if frame is not None:
        frame0 = frame
        ds9.mtv(exp, title="im", frame=frame) if True else None
        frame += 1

    if hasattr(exp, "getMaskedImage"):
        mi = exp.getMaskedImage()
        ccd = exp.getDetector()
    else:
        mi = exp
        ccd = None

    try:
        mi = mi.convertF()
    except AttributeError:
        pass

    if r is not None or lnGrad is not None:
        assert ccd is not None, "I need a CCD to set r and the logarithmic gradient"
        assert r is not None and lnGrad is not None, "Please provide both r and lnGrad"

    z = afwImage.ImageF(nx, ny)
    za = z.getArray()
    dlnzdx = afwImage.ImageF(nx, ny)
    dlnzdxa = dlnzdx.getArray()
    dlnzdy = afwImage.ImageF(nx, ny)
    dlnzdya = dlnzdy.getArray()
    dlnzdr = afwImage.ImageF(nx, ny)
    dlnzdra = dlnzdr.getArray()
    if returnResidualImage:
        residualImage = mi.clone()
        try:
            residualImage = residualImage.convertF()
        except AttributeError:
            pass
    else:
        residualImage = None

    with ds9.Buffering():
        for iy in range(ny):
            for ix in range(nx):
                bbox = afwGeom.BoxI(afwGeom.PointI(ix*xsize, iy*ysize), afwGeom.ExtentI(xsize, ysize))
                if False and frame is not None:
                    ds9Utils.drawBBox(bbox, frame=frame0)

                b, res = fitPlane(mi[bbox].getImage(), returnResidualImage=returnResidualImage, niter=5)

                b[1:] /= b[0]
                za[iy, ix], dlnzdxa[iy, ix], dlnzdya[iy, ix] = b

                if returnResidualImage:
                    residualImage[bbox][:] = res

                if ccd:
                    cen = afwGeom.PointD(bbox.getBegin() + bbox.getDimensions()/2)
                    x, y = ccd.getPositionFromPixel(cen).getMm()  # I really want pixels here
                    t = np.arctan2(y, x)
                    dlnzdra[iy, ix] = np.cos(t)*dlnzdxa[iy, ix] + np.sin(t)*dlnzdya[iy, ix]

                    if r is not None:
                        if not (ix in (0, xsize - 1) or iy in (0, ysize - 1)):
                            r.append(np.hypot(x, y))
                            lnGrad.append(dlnzdra[iy, ix])
                            if theta is not None:
                                theta.append(t)

    if frame is not None:
        if False:
            ds9.mtv(z, title="z", frame=frame)
            frame += 1
            ds9.mtv(dlnzdx, title="dlnz/dx", frame=frame)
            frame += 1
            ds9.mtv(dlnzdy, title="dlnz/dy", frame=frame)
            frame += 1
        ds9.mtv(residualImage, title="res", frame=frame)
        frame += 1
        ds9.mtv(dlnzdr, title="dlnz/dr %s" % (ccd.getId().getSerial() if ccd else ""), frame=frame)
        frame += 1

    return dlnzdx, dlnzdy, dlnzdr, residualImage


class FitPatchesWork(object):
    """Given a bin factor and dataId, return r, theta, and lnGrad arrays"""

    def __init__(self, butler, bin, verbose=False):
        self.butler = butler
        self.bin = bin
        self.verbose = verbose

    def __call__(self, dataId):
        if self.verbose:
            print("Visit %(visit)d CCD%(ccd)03d\r" % dataId)

        raw = self.butler.get("raw", immediate=True, **dataId)
        try:
            raw = cgUtils.trimExposure(raw, subtractBias=True, rotate=True)
        except:
            return None

        if False:
            msk = raw.getMaskedImage().getMask()
            BAD = msk.getPlaneBitMask("BAD")
            msk |= BAD
            msk[15:-15, 20:-20] &= ~BAD

        r, lnGrad, theta = [], [], []
        fitPatches(raw, bin=self.bin, r=r, lnGrad=lnGrad, theta=theta)

        return r, theta, lnGrad


def fitRadialParabola(mi, niter=3, tol=1e-5, nsigma=5, returnResidualImage=False):
    """Fit a radial parabola (centered at (0, 0) to the image im using a linear fit with an nsigma clip"""

    width, height = mi.getDimensions()
    BAD = afwImage.Mask.getPlaneBitMask("BAD")

    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    R = np.hypot(X + mi.getX0(), Y + mi.getY0())
    R = R.flatten()
    one = np.ones_like(R)

    A = np.array([one, R, R**2]).transpose()
    isMaskedImage = hasattr(mi, "getImage")
    im = mi.getImage() if isMaskedImage else mi
    ima = im.getArray().flatten()

    sctrl = afwMath.StatisticsControl()
    sctrl.setAndMask(afwImage.Mask.getPlaneBitMask("BAD"))
    c0, c1, c2 = afwMath.makeStatistics(mi, afwMath.MEANCLIP, sctrl).getValue(), 0, 0  # initial guess
    c00 = c0

    returnResidualImage = True
    if returnResidualImage:
        res = mi.clone()                # residuals from plane fit
        resim = res.getImage() if isMaskedImage else res

    for i in range(niter):
        fit = c0 + c1*R + c2*R**2

        resim.getArray()[:] = im.getArray() - fit.reshape(height, width)

        stats = afwMath.makeStatistics(res, afwMath.STDEVCLIP, sctrl)
        stdev = stats.getValue(afwMath.STDEVCLIP)
        good = np.abs(resim.getArray().flatten()) < nsigma*stdev

        if isMaskedImage:
            good = np.where(np.logical_and(good,
                                           np.bitwise_and(res.getMask().getArray().flatten(), BAD) == 0))[0]
        else:
            good = np.where(np.logical_and(good, np.isfinite(ima)))[0]

        b, residuals = np.linalg.lstsq(A[good], ima[good])[:2]

        if np.all(residuals < tol):
            break

        c0, c1, c2 = b

    if returnResidualImage:             # need to update with latest values
        fit = (c0 - c00) + c1*R + c2*R**2  # n.b. not c0
        resim.getArray()[:] = im.getArray() - fit.reshape(height, width)

        return b, res
    else:
        return b, None


class LnGradImage(cgUtils.GetCcdImage):
    """A class to return an Image dlnI/dr for a CCD, e.g.

    import lsst.afw.cameraGeom.utils as cgUtils
    bin = 32
    camera = butler.get("camera")
    cgUtils.showCamera(camera, LnGradImage(butler, bin=bin, visit=903442), frame=0, bin=bin)
    """

    def __init__(self, butler, bin=256, background=np.nan, verbose=False, *args, **kwargs):
        """Initialise
        gravity  If the image returned by the butler is trimmed (e.g. some of the SuprimeCam CCDs)
                 Specify how to fit the image into the available space; N => align top, W => align left
        background  The value of any pixels that lie outside the CCDs
        """
        super(LnGradImage, self).__init__(*args)
        self.butler = butler
        self.verbose = verbose
        self.kwargs = kwargs
        self.bin = bin
        self.imageIsBinned = True       # i.e. images returned by getImage are already binned

        self.isRaw = False

        self.gravity = None
        self.background = background

    def getImage(self, ccd, amp=None, imageFactory=None):
        """Return an image of dlnI/dr"""

        ccdNum = ccd.getId().getSerial()

        try:
            if self.kwargs.get("ccd") is not None and ccdNum not in self.kwargs.get("ccd"):
                raise RuntimeError

            dataId = self.kwargs
            if "ccd" in dataId:
                dataId = self.kwargs.copy()
                del dataId["ccd"]

            raw = cgUtils.trimExposure(self.butler.get("raw", ccd=ccdNum, immediate=True,
                                                       **dataId).convertF(),
                                       subtractBias=True, rotate=True)

            if False:
                msk = raw.getMaskedImage().getMask()
                BAD = msk.getPlaneBitMask("BAD")
                msk |= BAD
                msk[15:-15, 0:-20] &= ~BAD
        except Exception as e:
            if self.verbose and str(e):
                print(e)

            ccdImage = afwImage.ImageF(*ccd.getAllPixels(self.isTrimmed).getDimensions())
            if self.bin:
                ccdImage = afwMath.binImage(ccdImage, self.bin)

            return ccdImage

        return fitPatches(raw, bin=self.bin, returnResidualImage=False)[2]


def reconstructPatch(patch, z, dzdx, dzdy):
    width, height = patch.getDimensions()

    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    patch.getArray()[:] = z + X*dzdx + Y*dzdy


def reconstruct(xsize, ysize, dzdx, dzdy):
    nx, ny = dzdx.getDimensions()
    recon = afwImage.ImageF(nx*xsize, ny*ysize)
    for iy in range(ny):
        patch = None
        for ix in range(nx):
            if patch is None:
                if iy == 0:
                    z = 0
                else:
                    z = 0
            else:
                if iy > 0:
                    patch = recon[ix*xsize:(ix + 1)*xsize, (iy - 1)*ysize:iy*ysize]
                    z = patch[:, -1].getArray().T
                else:
                    z = patch[-1, :].getArray()

            patch = recon[ix*xsize:(ix + 1)*xsize, iy*ysize:(iy + 1)*ysize]
            reconstructPatch(patch, z, float(dzdx[ix, iy]), float(dzdy[ix, iy]))

    return recon


def radialProfile(butler, visit, ccds=None, bin=128, nJob=None, plot=False):
    """Return arrays giving the radius, logarithmic derivative, and theta for all patches in the camera"""

    if ccds is None:
        ccds = butler.queryMetadata('raw', 'visit', 'ccd', visit=visit)
    #
    # If we're using multiprocessing we need to create the jobs now
    #
    verbose = True
    if nJob:
        pool = multiprocessing.Pool(nJob)

    fitPatchesWorkArgs = []
    for ccdNum in ccds:
        dataId = dict(visit=visit, ccd=ccdNum)
        fitPatchesWorkArgs.append(dataId)

    r, lnGrad, theta = [], [], []
    worker = FitPatchesWork(butler, bin, verbose)
    if not nJob:
        for dataId in fitPatchesWorkArgs:
            _r, _theta, _lnGrad = worker(dataId)
            r += _r
            theta += _theta
            lnGrad += _lnGrad
    else:
        # We use map_async(...).get(9999) instead of map(...) to workaround a python bug
        # in handling ^C in subprocesses (http://bugs.python.org/issue8296)
        for _r, _theta, _lnGrad in pool.map_async(worker, fitPatchesWorkArgs).get(9999):
            r += _r
            theta += _theta
            lnGrad += _lnGrad

        pool.close()
        pool.join()

        pool.close()
        pool.join()

    r = np.array(r)
    lnGrad = np.array(lnGrad)
    theta = np.array(theta)

    if plot:
        plotRadial(r, lnGrad, theta)

    return r, lnGrad, theta


def makeRadial(r, lnGrad, nBin=100, profile=False, rmax=None, median=True):
    """
Return r and lnGrad binned into nBin bins.  If profile is True, integrate the lnGrad to get the radial profile
    """

    bins = np.linspace(0, min(18000, np.max(r)), nBin)
    binWidth = bins[1] - bins[0]

    rbar = np.empty_like(bins)
    lng = np.empty_like(bins)

    for i in range(len(bins)):
        inBin = np.where(np.abs(r - bins[i]) < 0.5*binWidth)
        rbar[i] = np.mean(r[inBin])
        if len(inBin[0]):
            if median:
                val = np.median(lnGrad[inBin])
            else:
                val = np.mean(lnGrad[inBin])
        else:
            val = np.nan
        lng[i] = val

    if rmax:
        ok = np.where(rbar < rmax)
        rbar = rbar[ok]
        lng = lng[ok]

    if profile:
        lng[np.where(np.logical_not(np.isfinite(lng)))] = 0.0
        lng = np.exp(np.cumsum(binWidth*lng))

    return rbar, lng


def plotRadial(r, lnGrad, theta=None, title=None, profile=False, showMedians=False, showMeans=False,
               tieIndex=None,
               marker="o", nBin=100, binAngle=0, alpha=1.0, xmin=-100, ctype='black', overplot=False,
               label=None, xlabel=None, ylabel=None, xlim=(-100, 18000), ylim=None):
    """
    N.b. theta is in radians, but binAngle is in degrees
    """

    if not overplot:
        pyplot.clf()

    norm = pyplot.Normalize(-180, 180)
    cmap = pyplot.cm.rainbow
    scalarMap = pyplot.cm.ScalarMappable(norm=norm, cmap=cmap)

    if profile or showMedians or showMeans:
        if binAngle > 0:
            angleBin = np.pi/180*(np.linspace(-180, 180 - binAngle, 360.0/binAngle, binAngle))
            binAngle *= np.pi/180

        lng0 = None
        for a in [None] if binAngle <= 0 else reversed(angleBin):
            if a is None:
                rbar, lng = makeRadial(r, lnGrad, nBin, profile=profile, median=not showMeans)
                color, label = ctype, label if label else title
            else:
                inBin = np.where(np.abs(theta - a) < 0.5*binAngle)

                rbar, lng = makeRadial(r[inBin], lnGrad[inBin], nBin, profile=profile)

                color = scalarMap.to_rgba(a*180/np.pi)
                label = "%6.1f" % (a*180/np.pi)

            if tieIndex is not None:
                if a is None:
                    lng /= lng[tieIndex]
                else:
                    if lng0 is None:
                        if tieIndex == 0:
                            lng /= lng[0]
                        lng0 = lng
                    else:
                        lng *= lng0[tieIndex]/lng[tieIndex]

            kwargs = dict(markersize=5.0, markeredgewidth=0, alpha=alpha, label=label)
            if color:
                kwargs["color"] = color

            pyplot.plot(rbar, lng, marker, **kwargs)
        if binAngle > 0:
            pyplot.legend(loc="lower left")
    else:
        if theta is None:
            pyplot.plot(r, lnGrad, 'o', alpha=alpha, markeredgewidth=0)
        else:
            pyplot.scatter(r, lnGrad, c=theta*180/np.pi, norm=norm, cmap=cmap, alpha=alpha,
                           marker='o', s=2.0, edgecolors="none")
            pyplot.colorbar()

    pyplot.xlim(xlim)
    if profile and ylim is None:
        ylim = (0.5, 1.05)
    pyplot.ylim(ylim)
    pyplot.xlabel(xlabel if xlabel else "R/pixels")
    pyplot.ylabel(ylabel if ylabel else "I" if profile else "d lnI/d r")
    if title:
        pyplot.title(title)

    pyplot.show()


def profilePca(butler, visits=None, ccds=None, bin=64, nBin=30, nPca=2, grad={},
               fluxes=None, bad=None,
               rmax=None, xlim=(None, None), ylim=(None, None), showLegend=True,
               plotProfile=False, plotFit=False, plotResidual=False, plotPca=False, plotCoeff=False,
               plotFlux=False):
    """N.b. we fit a constant and nPca components"""
    if visits:
        visits = list(visits)
        for v in visits:
            if v in grad and len(grad[v][0]) == 0:
                del grad[v]

        for v in visits[:]:
            if v not in grad:
                grad[v] = radialProfile(butler, visit=v, ccds=ccds, bin=bin)
            if len(grad[v][0]) == 0:
                print("Failed to read any data from visit %d" % v, file=sys.stderr)
                del grad[v]
                del visits[visits.index(v)]
    else:
        visits = list(sorted(grad.keys()))  # sorted returns a generator

    if bad:
        print("Skipping", bad)
        visits = [v for v in visits if v not in bad]

    if len(visits) < 2:
        raise RuntimeError("Please specify at least two valid visits")

    rbar, prof = makeRadial(*grad[visits[0]][0:2], nBin=nBin, profile=True, rmax=rmax)
    nprof = len(visits)
    npt = len(rbar)

    profiles = np.empty((npt, nprof))

    rbar = 0
    for i, v in enumerate(visits):
        r, profiles[:, i] = makeRadial(*grad[v][0:2], nBin=nBin, profile=True, rmax=rmax)
        rbar += r

    rbar /= nprof

    plotting = []                       # False, but modified by plotInit
    xlabel, ylabel = "R/pixels", ""

    def plotInit(plotting=plotting):
        plotting.append(True)
        pyplot.clf()

        def generateColors():
            colors = ["red", "green", "blue", "cyan", "magenta", "yellow", "black",
                      "orange", "brown", "gray", "darkGreen", "chartreuse"]
            i = 0
            while True:
                yield colors[i%len(colors)]
                i += 1

        return generateColors()

    if plotProfile:
        plotInit()
        for i, v in enumerate(visits):
            pyplot.plot(rbar, profiles[:, i], label="%d" % v)
        if showLegend:
            pyplot.legend(loc="best")
        pyplot.show()
    #
    # Normalise profiles
    #
    subtractMean = True

    nprofiles = profiles.copy()
    for i in range(nprof):
        p = nprofiles[:, i]
        if subtractMean:
            p -= np.mean(p)
        p /= np.std(p)
    #
    # Find covariance and eigenvectors using svd
    #
    U, s, Vt = np.linalg.svd(nprofiles, full_matrices=False)
    V = Vt.T

    # sort the PCs by descending order of the singular values (i.e. by the
    # proportion of total variance they explain)
    ind = np.argsort(s)[::-1]
    U = U[:, ind]
    s = s[ind]
    V = V[:, ind]

    if subtractMean:
        # Add a constant term (as we subtracted the mean)
        assert nprof > 2, "I need at least 3 input profiles"
        U[:, 1:nPca + 1] = U[:, 0:nPca]
        U[:, 0] = 1
        nFit = nPca + 1
    else:
        nFit = nPca

    if plotPca:
        plotInit()
        pyplot.title("PCA Components")
        for i in range(nFit):
            y = U[:, i]
            y /= y[0] if i == 1 else np.max(y)

            pyplot.plot(rbar, y, label="%d %s" % (i, "" if i == 0 else ("%.4f" % (s[i]/s[1]))))
        if showLegend:
            pyplot.legend(loc="best")

    fitProfiles = profiles.copy()
    b = np.empty((nFit, nprof))
    for i in range(nprof):
        b[:, i] = np.linalg.lstsq(U[:, 0:nFit], profiles[:, i])[0]
        fitProfiles[:, i] = np.sum(b[:, i]*U[:, 0:nFit], axis=1)

    if plotCoeff:
        colors = plotInit()
        for i, v in enumerate(visits):
            pyplot.plot(b[1][i]/b[0][i],
                        fluxes[v] if plotFlux else b[2][i]/b[0][i],
                        "o", label="%d" % (v), c=next(colors))

        if showLegend:
            pyplot.legend(loc="best", ncol=2, numpoints=1, columnspacing=0)
        xlabel = "$C_1/C_0$"
        ylabel = "flux" if plotFlux else "$C_2/C_0$"

    if plotFit or plotResidual:
        plotInit()
        pyplot.title(("Const + " if subtractMean else "") + ("%d PCA components" % nPca))
        ylabel = "I - model" if plotResidual else "model"
        for i, v in enumerate(visits):
            y = fitProfiles[:, i] - profiles[:, i] if plotResidual else fitProfiles[:, i]
            pyplot.plot(rbar, y, label="%d" % (v))
        if showLegend:
            pyplot.legend(loc="best", ncol=2, numpoints=1, columnspacing=1)

    if plotting:
        pyplot.xlabel(xlabel)
        pyplot.ylabel(ylabel)
        pyplot.xlim(xlim)
        pyplot.ylim(ylim)

    return rbar, profiles


def plotProfiles(butler, visits, outputPlotFile=None, bin=64, nBin=30, binAngle=45, marker='o',
                 tieIndex=3, ctype=['k'], nJob=10, grad={}):
    try:
        iter(visits)
    except TypeError:
        visits = [visits]

    if outputPlotFile:
        from matplotlib.backends.backend_pdf import PdfPages
        pp = PdfPages(outputPlotFile)
    else:
        pp = None

    for i, visit in enumerate(visits):
        print("Processing %d" % (visit))
        if visit not in grad:
            grad[visit] = radialProfile(butler, visit=visit, bin=bin, nJob=nJob)

        md = butler.get("raw_md", visit=visit, ccd=0)
        title = "%s %s %d binned %d" % (md.get("OBJECT"), afwImage.Filter(md).getName(), visit, bin)
        plotRadial(*grad[visit], title=title, alpha=1.0, ctype=ctype[i%len(ctype)],
                   overplot=True if (pp is None and i > 0) else True,
                   profile=True, nBin=nBin, binAngle=binAngle, marker=marker, tieIndex=tieIndex)
        if pp:
            try:
                pp.savefig()
            except ValueError:          # thrown if we failed to actually plot anything
                pass
            pyplot.clf()

    if pp:
        pp.close()
    else:
        pyplot.show()

    return grad


def medianFilterImage(img, nx, ny=None):
    raise RuntimeError("medianFilterImage is moved to analysis.utils")

try:
    labels
except NameError:
    labels = {904670: "El=90", 904672: "El=60", 904678: "El=45", 904674: "El=30", 904676: "El=15",
              904524: "domeflat", 904526: "domeflat", 904528: "domeflat", 904530: "domeflat",
              904532: "domeflat",
              904778: "skyflat", 904780: "skyflat", 904782: "skyflat",
              }
    bad = {902636: "only half of camera read",
           902996: "looks like a dark",
           }


def diffs(mosaics, visits, refVisit=None, scale=True, raw=None,
          rng=20, IRatioMax=1.0, frame0=0, verbose=False):
    """Display a series of differences and/or scaled differences
    (scale = True, False, (True, False), ...)
    """

    visits = list(visits)               # in case it's an iterator, and to get a copy

    if refVisit is None:
        refVisit = visits[0]

    ref = mosaics[refVisit]
    nameRef = str(refVisit)
    refMedian = float(np.median(ref.getArray()))

    visits = [v for v in visits if v != refVisit]

    try:
        scale[0]
    except TypeError:
        scale = (scale,)

    frame = frame0
    goodVisits = [refVisit]
    for v2 in visits:
        for s in scale:
            nameA, nameB = str(v2), nameRef
            diff = mosaics[v2].clone()

            IRatio = np.median(mosaics[v2].getArray())/refMedian
            if IRatioMax and IRatio > IRatioMax and IRatio < 1/IRatioMax:
                if verbose:
                    print("Skipping %d [ratio = %.2f]" % (v2, IRatio))
                break

            if s == scale[0]:
                goodVisits.append(v2)

            if s:
                diff /= IRatio
                nameA = "%s/%.2f" % (nameA, IRatio)

            diffa = diff.getArray()
            good = np.where(np.isfinite(diffa))
            v2Median = np.mean(diffa[good])

            if raw:
                title = nameA
            else:
                diff -= ref
                if IRatio < 1:
                    diff *= -1
                    nameA, nameB = nameB, nameA

                if verbose:
                    print("%s %5.1f%% %-s" % (v2, 100*np.mean(diffa[good])/v2Median, labels.get(v2, "")))

                title = "%s - %s" % (nameA, nameB)
                if not s:
                    title += " [rat=%.2f]" % (IRatio)

            if True:
                ds9.mtv(diff, title=title, frame=frame)
            else:
                ds9.erase(frame=frame)

            ds9.dot(title, int(0.5*diff.getWidth()), int(0.05*diff.getHeight()),
                    frame=frame, ctype=ds9.RED)
            if v2 in labels:
                ds9.dot(labels[v2], int(0.5*diff.getWidth()), int(0.95*diff.getHeight()),
                        frame=frame, ctype=ds9.RED)

            s0 = 0 if (s and not raw) else afwMath.makeStatistics(diff, afwMath.MEDIAN).getValue()
            s0 -= 0.5*rng
            ds9.ds9Cmd("scale linear; scale limits %g %g" % (s0, s0 + rng), frame=frame)

            frame += 1

    return list(sorted(goodVisits))


def imagePca(mosaics, visits=None, nComponent=3, log=False, rng=30,
             showEigen=True, showResiduals=False, showOriginal=True, showRecon=False,
             normalizeEimages=True, scale=False, frame0=0, verbose=False):

    if showOriginal and showRecon:
        raise RuntimeError("You may not specify both showOriginal and showRecon")

    try:
        rng[0]
    except TypeError:
        rng = [rng, rng]

    if not visits:
        visits = sorted(mosaics.keys())

    mosaics = mosaics.copy()
    for v in visits:
        mosaics[v] = mosaics[v].clone()

    pca = afwImage.ImagePcaF()
    mask = None
    for v in visits:
        im = mosaics[v]
        if not mask:
            mask = im.Factory(im.getDimensions())
            maska = mask.getArray()
            X, Y = np.meshgrid(np.arange(mask.getWidth()), np.arange(mask.getHeight()))
            maska[np.where(np.hypot(X - 571, Y - 552) > 531)] = np.nan
            del maska

            mask[168: 184, 701:838] = np.nan
            mask[667: 733, 420:556] = np.nan
            mask[653: 677, 548:570] = np.nan
            mask[1031:1047, 274:414] = np.nan

            if False:
                mask[866:931, 697:828] = np.nan
                mask[267:334, 145:276] = np.nan

        im += mask

        pca.addImage(im, afwMath.makeStatistics(im, afwMath.SUM).getValue())
    pca.analyze()

    eValues = np.array(pca.getEigenValues())
    eValues /= eValues[0]
    eImages = pca.getEigenImages()
    #
    # Fiddle eigen images (we don't care about orthogonality)
    #
    if False:
        f10 = 0.1
        f20 = -0.3
        f30 = 0.55
        eImages[1].getArray()[:] += f10*eImages[0].getArray()
        eImages[2].getArray()[:] += f20*eImages[0].getArray()
        eImages[3].getArray()[:] += f30*eImages[0].getArray()

    if nComponent:
        eImages = eImages[:nComponent]
    else:
        nComponent = len(eImages)
    #
    # Normalize
    #
    good = np.where(np.isfinite(eImages[0].getArray()))
    if normalizeEimages:
        for eim in eImages:
            eima = eim.getArray()
            eima /= 1e-3*np.sqrt(np.sum(eima[good]**2))
    #
    # Plot/display eigen values/images
    #
    frame = frame0
    if showEigen:
        for i in range(len(eImages)):
            ds9.mtv(eImages[i], frame=frame, title="%d %.2g" % (i, eValues[i]))
            ds9.ds9Cmd("scale linear; scale mode zscale")
            frame += 1

        pyplot.clf()
        if log:
            eValues = np.log10(eValues)
        pyplot.plot(eValues)
        pyplot.plot(eValues, marker='o')
        pyplot.xlim(-0.5, len(eValues))
        pyplot.ylim(max(-5, pyplot.ylim()[0]), 0.05 if log else 1.05)
        pyplot.xlabel("n")
        pyplot.ylabel(r"$lg(\lambda)$" if log else r"$\lambda$")
        pyplot.show()

    if showOriginal or showRecon or showResiduals:
        #
        # Expand input images in the (modified) eImages
        #
        A = np.empty(nComponent*nComponent).reshape((nComponent, nComponent))
        b = np.empty(nComponent)
        for v in visits:
            im = mosaics[v]
            if scale:
                im /= afwMath.makeStatistics(im, afwMath.MEANCLIP).getValue()

            for i in range(nComponent):
                b[i] = np.dot(eImages[i].getArray()[good], im.getArray()[good])

                for j in range(i, nComponent):
                    A[i, j] = np.dot(eImages[i].getArray()[good], eImages[j].getArray()[good])
                    A[j, i] = A[i, j]

            x = np.linalg.solve(A, b)
            # print v, A, b, x
            print("%d [%s] %s" % (v, ", ".join(["%9.2e" % _ for _ in x/x[0]]), labels.get(v, "")))

            recon = eImages[0].clone()
            recon *= x[0]
            recona = recon.getArray()
            for i in range(1, nComponent):
                recona += x[i]*eImages[i].getArray()

            mtv = True
            if showOriginal or showRecon:
                if mtv:
                    ds9.mtv(im if showOriginal else recon, frame=frame, title=v)
                else:
                    ds9.erase(frame=frame)
                s0 = afwMath.makeStatistics(recon, afwMath.MEDIAN).getValue()
                s0 -= 0.5*rng[0]
                ds9.ds9Cmd("scale linear; scale limits %g %g" % (s0, s0 + rng[0]), frame=frame)
                ds9.dot("%s %d" % ("orig" if showOriginal else "resid", v),
                        int(0.5*im.getWidth()), int(0.15*im.getHeight()), frame=frame, ctype=ds9.RED)
                if v in labels:
                    ds9.dot(labels[v], int(0.5*im.getWidth()), int(0.85*im.getHeight()),
                            frame=frame, ctype=ds9.RED)

                frame += 1

            if showResiduals:
                recon -= im
                if mtv:
                    ds9.mtv(recon, frame=frame)
                else:
                    ds9.erase(frame=frame)
                s0 = 0
                s0 -= 0.5*rng[1]
                ds9.ds9Cmd("scale linear; scale limits %g %g" % (s0, s0 + rng[1]), frame=frame)

                frame += 1

    return eImages


def mosaicIo(dirName, mosaics=None, mode=None):
    if not mode:
        mode = "w" if mosaics else "r"
    if mosaics is None:
        mosaics = {}

    if mode == "r":
        import glob
        files = glob.glob(os.path.join(dirName, "*.fits")) + glob.glob(os.path.join(dirName, "*.fits.gz"))
        for f in files:
            v = re.search(r"^([^.]+)", os.path.basename(f)).group(1)
            try:
                v = int(v)
            except:
                pass
            mosaics[v] = afwImage.ImageF(f)
    elif mode == "w":
        if not os.path.isdir(dirName):
            os.makedirs(dirName)

        for k, v in mosaics.items():
            v.writeFits(os.path.join(dirName, "%s.fits" % k))
    else:
        raise RuntimeError("Please use a mode of r or w")

    return mosaics


def correctVignettingAndDistortion(camera, mosaics, bin=32):
    """Correct a dict of mosaics IN PLACE for vignetting and distortion"""
    im = list(mosaics.values())[0]

    X, Y = bin*np.meshgrid(np.arange(im.getWidth()), np.arange(im.getHeight()))
    X -= 18208.0
    Y -= 17472.0

    vig = utils.getVignetting(X, Y)               # Vignetting correction
    correction = utils.getPixelArea(camera, X, Y)  # Jacobian correction
    correction *= vig

    for im in mosaics.values():
        im /= correction


def makeMos(butler, mos, frame0=0, bin=32, nJob=20, visits=[]):
    if not visits:
        visits = [904288, 904320, 904330, 904520, 904534, 904536, 904538, 904670, 904672,
                  904674, 904676, 904678, 904786, 904788, 904790, 904792, 904794, 905034, 905036]

    frame = frame0
    for visit in visits:
        if visit in mos:
            continue

        if visit in bad:
            print("Skipping bad visit %d: %s" % (visit, bad[visit]))
            continue

        global labels
        try:
            md = butler.get("raw_md", visit=visit, ccd=10)
            labels[visit] = afwImage.Filter(md).getName()
        except RuntimeError as e:
            print(e)

        mos[visit] = cgUtils.showCamera(butler.get("camera"),
                                        cgUtils.ButlerImage(butler, visit=visit,
                                                            callback=utils.trimRemoveCrCallback,
                                                            verbose=True),
                                        nJob=nJob, frame=frame, bin=bin,
                                        title=visit, overlay=True, names=False)
        frame += 1


def flattenBackground(im, nx=2, ny=2, scale=False):
    """Fit and subtract an nx*ny background model"""

    bctrl = afwMath.BackgroundControl(nx, ny)
    bkgd = afwMath.makeBackground(im, bctrl).getImageF(afwMath.Interpolate.LINEAR)
    mean = np.mean(bkgd.getArray())

    im -= bkgd
    im += float(mean)

    if scale:
        im /= mean


def plotRadialProfile(mos, visits, butler=None, showMedians=True, showMeans=False,
                      title="", xlim=None, ylim=None,
                      binning=1, tieIndex=None, normalize=True, LaTeX=False, dirName=None, savePlot=False,
                      marker='-', labelVisit=False, showLegend=True):
    """
plotRadialProfile(mos, range(902686, 902702+1, 2) + range(902612, 902634+1, 2) + [904006, 904008, 904036, 904038, 902876], butler=butler, xlim=(-10, 850), tieIndex=40

visits may be a filter name
    """
    colors = ["red", "blue", "green", "cyan", "magenta", "yellow", "black", "brown", "orchid", "orange"]

    abell2163 = []
    abell2163 += list(range(902924, 902928+1, 2))  # g
    abell2163 += list(range(903266, 903274+1, 2))  # r
    abell2163 += [v for v in range(902876, 902894+1, 2) if v not in (902884, 902886, 902890)]  # i

    dth_a = []
    dth_a += list(range(904410, 904450+1, 4)) + list(range(904456, 904474+1, 2))  # y

    dth_16h = []
    dth_16h += list(range(902800, 902860 + 1, 6)) + list(range(902862, 902870+1, 2))  # i

    stripe82l = []
    stripe82l += list(range(902936, 902942+1, 2))            # g
    stripe82l += list(range(903332, 903338+1, 2))            # r
    stripe82l += list(range(904006, 904008+1, 2)) + list(range(904036, 904038+1, 2))  # i
    stripe82l += list(range(904350, 904400+1, 2))           # y

    science = abell2163 + dth_a + dth_16h + stripe82l

    darkDome = []
    darkDome += list(range(904326, 904330+1, 2))  # g
    darkDome += [904520] + list(range(904534, 904538+1, 2)) + list(range(904670,
                                                              904678+1, 2)) + list(range(904786, 904794+1, 2))  # i

    domeflats = []
    domeflats += list(range(903036, 903044+1, 2))  # g
    domeflats += list(range(903440, 903452+1, 2))  # r
    domeflats += list(range(902686, 902704+1, 2))  # i
    domeflats += [904606, 904608, 904626, 904628]  # i
    domeflats += list(range(905420, 905428+1, 2))  # i
    domeflats += list(range(904478, 904490+1, 2))  # y
    domeflats += list(range(904742, 904766+1, 2))  # NB921

    skyflats = []
    skyflats += list(range(902976, 903002+1, 2))          # g
    skyflats += list(range(903420, 903432 + 1, 2))        # r
    skyflats += list(range(902612, 902634+1, 2))          # i
    skyflats = [v for v in skyflats if v != 902996]  # remove failed exposures

    filterName = None
    if isinstance(visits, str):         # get all visits taken with that title
        filterName = visits.lower()

        if not title:
            title = filterName

        visits = []
        for v in mos.keys():
            try:
                int(v)                  # only int keys can really be visit numbers
            except:
                continue

            if v < 0:                   # a processed version of a visit
                continue

            if v in [902976]:           # too faint
                continue

            if v in darkDome:
                continue

            if False:
                raw = butler.get("raw", visit=v, ccd=10)
                rawFilterName = raw.getFilter().getName()
            elif False:
                md = butler.get("raw_md", visit=v, ccd=10)  # why does this fail?
                rawFilterName = afwImage.Filter(md).getName()
            else:
                rawFilterName = butler.get("raw_md", visit=v, ccd=10).get("FILTER01").lower()
                if re.search(r"^hsc-", rawFilterName):
                    rawFilterName = rawFilterName[-1:]
                if rawFilterName == "z" and v in range(904672, 905068+1, 2):  # z wasn't available
                    rawFilterName = "i"                                       # error in header

            if rawFilterName == filterName.lower():
                visits.append(v)

    if visits:
        if not filterName:
            if butler:
                filterName = butler.get("raw_md", visit=v, ccd=10).get("FILTER01").lower()
                if re.search(r"^hsc-", filterName):
                    filterName = filterName[-1:]
                if filterName == "z" and v in range(904672, 905068+1, 2):  # z wasn't available
                    filterName = "i"                                       # error in header
            else:
                filterName = "unknown"
    else:
        raise RuntimeError("Please provide at least on visit for me to plot")

    if LaTeX:
        print(r"""\begin{longtable}{lllll}
visit & median flux & exposure time & line type & line colour \\
\hline""")

    title += " (forced to agree at %dth point)" % (tieIndex)

    pyplot.clf()
    for i, v in enumerate(sorted(visits)):
        label = [str(v)]
        im = mos[v].clone()
        v = abs(v)                      # -v => processed version of v

        if v in domeflats:
            flattenBackground(im)

        if False:
            im = fitRadialParabola(im)[1]

        ima = im.getArray()
        med = np.median(ima)
        if normalize:
            ima /= med

        if not butler:
            exposureTime = float("nan")
        else:
            raw = butler.get("raw", visit=v, ccd=10)
            filter = raw.getFilter()
            exposureTime = raw.getInfo().getVisitInfo().getExposureTime()
            if labelVisit:
                pass
            else:
                label = []

                if False:
                    label.append('domeflat' if v in domeflats else
                                 'science ' if v in science else
                                 'skyflat ' if v in skyflats else
                                 'other   ')

                if False:
                    label.append(r"%5.0f" % med)
                    label.append(r"%3gs" % exposureTime)
                else:
                    label.append(r"%3.0f e/s" % (med/exposureTime))

                if False:
                    label.append(r"%s" % filter.getName())

        if i == 0:
            width, height = im.getDimensions()
            x, y = np.meshgrid(np.arange(width), np.arange(height))
            x += im.getX0()
            y += im.getY0()
            r = np.hypot(x, y)
            theta = np.arctan2(y, x)

        if False and i < 10:
            print("Skipping", v)
            continue

        if xlim is None:
            xlim = (0, np.max(r))
        if ylim is None:
            f = 0.05                    # plot in += 100*f%
            m = np.median(ima)
            ylim = (m*(1 - f), m*(1 + f))

        label = " ".join(label)

        if False:
            marker = '-' if i < len(colors) else '-.' if i < 2*len(colors) else '--'
        else:
            marker = '-' if v in science else '-.' if v in domeflats else '--' if v in skyflats else 'o'
        ctype = colors[i%len(colors)] if True else \
            'green' if v in domeflats else \
            'black' if v in science else \
            'red' if v in skyflats else \
            'blue'

        if True:
            columns = (v, med, exposureTime, marker, ctype)
            if LaTeX:
                print(r"%s & %6.0f & %3.0f & %2s & %s \\" % columns)
            else:
                print(filter.getName(), ("%s %6.0f %3.0f  %2s %s" % columns))

        plotRadial(binning*r.flatten(), ima.flatten(), theta.flatten(), title=title, alpha=1.0,
                   showMedians=showMedians, showMeans=showMeans, xlim=xlim, ylim=ylim,
                   marker=marker, ctype=ctype, tieIndex=tieIndex,
                   label=label, xlabel="R (pixels)", ylabel="Relative Intensity", overplot=i > 0)

        if False and i == 3:
            break

    if LaTeX:
        print(r"""\caption{\label{%sLegend} Index of lines in %s radial profiles}
\end{longtable}""" % (filterName, filterName))

    if showLegend:
        legend = pyplot.legend(loc="best", ncol=3,
                               borderpad=0.1, labelspacing=0, handletextpad=0, handlelength=2,
                               borderaxespad=0)
        if legend:
            legend.draggable(True)

    if savePlot:
        fileName = "%s-rings-%d.png" % (filterName, tieIndex)
        if dirName:
            fileName = os.path.join(os.path.expanduser(dirName), fileName)
        pyplot.savefig(fileName)
        print("Wrote %s" % fileName)
