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
im = cgUtils.showCamera(butler.get("camera"), rings.LnGradImage(butler, bin=bin, visit=visit, verbose=True), frame=frame, bin=bin, title=visit, overlay=True)

grad = {}
if False:
   grad[visit] = rings.radialProfile(butler, visit=visit, bin=bin)
else:
   # This has the side effect of reading the visits and plotting
   r, profs = rings.profilePca(butler, visits=[129085, 129102], bin=bin, grad=grad, plotProfile=True)

rings.plotRadial(r, profs, xlim=(-100, 5500), ylim=(0.8, 1.03))
"""

import sys
from lsstImports import *
try:
    pyplot
except NameError:
    import matplotlib.pyplot as pyplot
    pyplot.interactive(1)
import numpy.linalg

def fitPlane(mi, niter=3, tol=1e-5, nsigma=5, returnResidualImage=False):
    """Fit a plane to the image im using a linear fit with an nsigma clip"""

    width, height = mi.getDimensions()
    BAD = afwImage.MaskU.getPlaneBitMask("BAD")

    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    X, Y = X.flatten(), Y.flatten()
    one = np.ones_like(X)

    A = np.array([one, X, Y]).transpose()
    isMaskedImage = hasattr(mi, "getImage")
    im = mi.getImage() if isMaskedImage else mi
    ima = im.getArray().flatten()

    sctrl = afwMath.StatisticsControl()
    sctrl.setAndMask(afwImage.MaskU.getPlaneBitMask("BAD"))
    z, dzdx, dzdy = afwMath.makeStatistics(mi, afwMath.MEANCLIP, sctrl).getValue(), 0, 0 # initial guess

    returnResidualImage = True
    if returnResidualImage:
        res = mi.clone()                # residuals from plane fit
        resim = res.getImage() if isMaskedImage else res

    for i in range(niter):
        plane = z + dzdx*X + dzdy*Y

        resim.getArray()[:] = im.getArray() - plane.reshape(height, width)

        if isMaskedImage:
            stats = afwMath.makeStatistics(res, afwMath.MEANCLIP | afwMath.STDEVCLIP, sctrl)
            mean, stdev = stats.getValue(afwMath.MEANCLIP), stats.getValue(afwMath.STDEVCLIP)
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

If r and lnGrad are provided they should be lists (more accurately, support .append), and they have the
values of r and dlnI/dr from this image appended.
    """

    width, height = exp.getDimensions()

    if bin is not None:
        nx, ny = width//bin, height//bin

    xsize = width//nx
    ysize = height//ny

    if frame is not None:
        frame0 = frame
        ds9.mtv(exp,   title="im",    frame=frame) if True else None; frame += 1

    if hasattr(exp, "getMaskedImage"):
        mi = exp.getMaskedImage()
        ccd = afwCamGeom.cast_Ccd(exp.getDetector())
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

    z = afwImage.ImageF(nx, ny);      za = z.getArray()
    dlnzdx = afwImage.ImageF(nx, ny); dlnzdxa = dlnzdx.getArray()
    dlnzdy = afwImage.ImageF(nx, ny); dlnzdya = dlnzdy.getArray()
    dlnzdr = afwImage.ImageF(nx, ny); dlnzdra = dlnzdr.getArray()
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

                b, res = fitPlane(mi[bbox].getImage(), returnResidualImage=returnResidualImage, niter=3)
                
                b[1:] /= b[0]
                za[iy, ix], dlnzdxa[iy, ix], dlnzdya[iy, ix] = b

                if returnResidualImage:
                    residualImage[bbox] <<= res

                if ccd:
                    cen = afwGeom.PointD(bbox.getBegin() + bbox.getDimensions()/2)
                    x, y = ccd.getPositionFromPixel(cen).getMm() # I really want pixels here
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
            ds9.mtv(z,    title="z",     frame=frame); frame += 1
            ds9.mtv(dlnzdx, title="dlnz/dx", frame=frame); frame += 1
            ds9.mtv(dlnzdy, title="dlnz/dy", frame=frame); frame += 1
        ds9.mtv(residualImage, title="res",     frame=frame); frame += 1
        ds9.mtv(dlnzdr, title="dlnz/dr %s" % (ccd.getId().getSerial() if ccd else ""), frame=frame); frame += 1

    return dlnzdx, dlnzdy, dlnzdr, residualImage

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

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
            if False and ccdNum not in range(0, 100,5):
                raise RuntimeError("skipping CCD %d" % ccdNum)
                pass
            if ccdNum not in (33, 34, 35, 41, 42, 43, 49, 50, 51,):
                pass
            if False:
                raise RuntimeError("skipping CCD %d" % ccdNum)

            if self.kwargs.get("ccd") is not None and not ccdNum in self.kwargs.get("ccd"):
                raise RuntimeError

            dataId = self.kwargs
            if dataId.has_key("ccd"):
                dataId = self.kwargs.copy()
                del dataId["ccd"]

            raw = cgUtils.trimExposure(self.butler.get("raw", ccd=ccdNum, immediate=True, **dataId).convertF(),
                                       subtractBias=True)
            nQuarter = ccd.getOrientation().getNQuarter()
            msk = raw.getMaskedImage().getMask()
            if False:
                BAD=msk.getPlaneBitMask("BAD")
                msk |= BAD
                msk[15:-15, 0:-20] &= ~BAD

            if nQuarter != 0:
                raw.setMaskedImage(afwMath.rotateImageBy90(raw.getMaskedImage(), nQuarter))
        except Exception, e:
            if self.verbose and str(e):
                print e

            ccdImage = afwImage.ImageF(*ccd.getAllPixels(self.isTrimmed).getDimensions())
            if self.bin:
                ccdImage = afwMath.binImage(ccdImage, self.bin)

            return ccdImage

        return fitPatches(raw, bin=self.bin, returnResidualImage=False)[2]

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

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

def radialProfile(butler, visit, ccds=None, bin=128, frame=None, plot=False):
    """Return arrays giving the radius, logarithmic derivative, and theta for all patches in the camera"""
    r, lnGrad, theta = [], [], []

    if ccds is None:
        ccds = butler.queryMetadata('raw', 'visit', 'ccd', visit=visit)

    for ccdNum in ccds:
        if len(ccds) > 1:
            print "%d CCD %03d\r" % (visit, ccdNum), ; sys.stdout.flush()

        raw = cgUtils.trimExposure(butler.get("raw", visit=visit, ccd=ccdNum), subtractBias=True)
        ccd = afwCamGeom.cast_Ccd(raw.getDetector())
        nQuarter = ccd.getOrientation().getNQuarter()
        if nQuarter != 0:
            raw.setMaskedImage(afwMath.rotateImageBy90(raw.getMaskedImage(), nQuarter))

        if False:
            msk=raw.getMaskedImage().getMask()
            BAD=msk.getPlaneBitMask("BAD"); msk |= BAD; msk[15:-15, 20:-20] &= ~BAD

        dlnzdx, dlnzdy, dlnzdr = fitPatches(raw, bin=bin, r=r, lnGrad=lnGrad, theta=theta)[0:3]

        if frame is not None:
            ds9.mtv(dlnzdr, title="dlnz/dr %s" % (ccd.getId().getSerial()), frame=frame)
                
    if len(ccds) > 1:
        print ""
        
    r = np.array(r); lnGrad = np.array(lnGrad); theta = np.array(theta)

    if plot:
        plotRadial(r, lnGrad, theta)

    return r, lnGrad, theta
    
def makeRadial(r, lnGrad, nBin=100, profile=False, rmax=None):
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
        lng[i] = np.median(lnGrad[inBin]) if len(inBin[0]) else np.nan

    if rmax:
        ok = np.where(rbar < rmax)
        rbar = rbar[ok]
        lng = lng[ok]

    if profile:
        lng[np.where(np.logical_not(np.isfinite(lng)))] = 0.0
        lng = np.exp(np.cumsum(binWidth*lng))

    return rbar, lng

def plotRadial(r, lnGrad, theta=None, title=None, profile=False, showMedians=False,
               nBin=100, binAngle=0, alpha=1.0, xmin=-100,
               xlim = (-100, 18000), ylim = None):
    """
    N.b. theta is in radians, but binAngle is in degrees
    """

    pyplot.clf()

    norm = pyplot.Normalize(-180, 180)
    cmap = pyplot.cm.rainbow
    scalarMap = pyplot.cm.ScalarMappable(norm=norm, cmap=cmap)
    
    if profile or showMedians:
        if False:
            import scipy.ndimage
            scipy.ndimage.filters.median_filter(ref, size=[size], output=sky, mode='reflect')

        bins = np.linspace(0, min(18000, np.max(r)), nBin)
        binWidth = bins[1] - bins[0]

        if binAngle <= 0:
            rbar, lng = makeRadial(r, lnGrad, nBin, profile=profile)

            pyplot.plot(rbar, lng, 'o', markersize=5.0, markeredgewidth=0, color='black', alpha=alpha)
        else:
            angleBin = np.pi/180*(np.linspace(-180, 180 - binAngle, 360.0/binAngle, binAngle) + 0.5*binAngle)
            binAngle *= np.pi/180

            for a in reversed(angleBin):
                inBin = np.where(np.abs(theta - a) < 0.5*binAngle)

                rbar, lng = makeRadial(r[inBin], lnGrad[inBin], nBin, profile=profile)

                color = scalarMap.to_rgba(a*180/np.pi)

                pyplot.plot(rbar, lng, 'o', markersize=5.0, markeredgewidth=0, color=color, alpha=alpha,
                            label="%6.1f" % (a*180/np.pi))

            pyplot.legend(loc="lower left")
    else:
        if theta:
            pyplot.scatter(r, lnGrad, c=theta*180/np.pi, norm=norm, cmap=cmap, alpha=alpha,
                           marker='o', s=2.0, edgecolors="none")
            pyplot.colorbar()
        else:
            pyplot.plot(r, lnGrad, 'o', alpha=alpha, markeredgewidth=0)

    pyplot.xlim(xlim)
    if profile and ylim is None:
        ylim = (0.5, 1.05)
    pyplot.ylim(ylim)
    pyplot.xlabel("R/pixels")
    pyplot.ylabel("I" if profile else "d lnI/d r")
    if title:
        pyplot.title(title)
        
    pyplot.show()


def profilePca(butler, visits=None, ccds=None, bin=64, nBin=30, nPca=2, grad={},
               rmax=None, xlim=(None, None), ylim=(None, None), showLegend=True,
               plotProfile=False, plotFit=False, plotResidual=False, plotPca=False, plotCoeff=False):

    if visits:
        for v in visits:
            if not grad.has_key(v):
                grad[v] = radialProfile(butler, visit=v, ccds=ccds, bin=bin)
    else:
        visits = list(sorted(grad.keys())) # sorted returns a generator

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

    if plotProfile:
        plotInit()
        for i, v in enumerate(visits):
            pyplot.plot(rbar, profiles[:, i], label="%d" % v)
        if showLegend:
            pyplot.legend(loc="upper right")
        pyplot.show()
    #
    # Normalise profiles
    #
    nprofiles = profiles.copy()
    for i in range(nprof):
        p = nprofiles[:, i]
        #p -= np.mean(p)
        p /= np.std(p)

    if False:
        plotInit()
        for i, v in enumerate(visits):
            pyplot.plot(rbar, nprofiles[:, i], label="%d" % v)
        if showLegend:
            pyplot.legend(loc="upper right")
        pyplot.show()
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

    if plotPca:
        plotInit()
        pyplot.title("PCA Components")
        for i in range(nPca):
            y = U[:, i]
            y /= y[0] if i == 0 else np.max(y)

            pyplot.plot(rbar, y, label="%d %.4f" % (i, s[i]/s[0]))
        if showLegend:
            pyplot.legend() # loc="upper center")

    fitProfiles = profiles.copy()
    b = np.empty((nPca, nprof))
    for i in range(nprof):
        b[:, i] = np.linalg.lstsq(U[:, 0:nPca], profiles[:, i])[0]
        fitProfiles[:, i] = np.sum(b[:, i]*U[:, 0:nPca], axis=1)

    if plotCoeff:
        plotInit()
        for i, v in enumerate(visits):
            if v not in range(903420, 903438+1):
                pass # continue
            if v in (903434, 903438, ):
                #continue
                pass

            pyplot.plot(b[1][i]/(1 if True else b[0][i]), b[2][i]/(1 if True else b[0][i]), "o", label="%d" % (v))
            print v, b[:, i] # , profiles[0:10, i]

        if showLegend:
            pyplot.legend(loc="lower right", ncol=2, numpoints=1, columnspacing=0)
        xlabel = "$C_1/C_0$"
        ylabel = "$C_2/C_0$"
        
    if plotFit or plotResidual:
        plotInit()
        pyplot.title( ("%d PCA components" % nPca))
        ylabel = "I - model" if plotResidual else "model"
        for i, v in enumerate(visits):
            if v not in (903424, 903434, 903436, 903438, ):
                pass
                #continue
            if  v in (903434, 903438, ):
                pass
                #continue

            y = fitProfiles[:, i] - profiles[:, i] if plotResidual else fitProfiles[:, i]
            pyplot.plot(rbar, y, label="%d" % (v))
        if showLegend:
            pyplot.legend(loc="lower left", ncol=2, numpoints=1, columnspacing=1)

    if plotting:
        pyplot.xlabel(xlabel)
        pyplot.ylabel(ylabel)
        pyplot.xlim(xlim)
        pyplot.ylim(ylim)

    return rbar, profiles
