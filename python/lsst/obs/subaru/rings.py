import sys
from lsstImports import *
import matplotlib.pyplot as pyplot
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

def radialProfile(butler, visit, ccds=range(100), bin=128, frame=None):
    """Return arrays giving the radius, logarithmic derivative, and theta for all patches in the camera"""
    r, lnGrad, theta = [], [], []
    for ccdNum in ccds:
        if len(ccds) > 1:
            print "CCD %03d\r" % ccdNum, ; sys.stdout.flush()

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

    plotRadial(r, lnGrad, theta)

    return r, lnGrad, theta
    
def plotRadial(r, lnGrad, theta, title=None, profile=False, showMedians=False,
               nBin=100, binAngle=30, alpha=1.0):
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

        angleBin = np.pi/180*(np.linspace(-180, 180 - binAngle, 360.0/binAngle, binAngle) + 0.5*binAngle) if binAngle else [0] 
        binAngle *= np.pi/180

        for a in reversed(angleBin):
            rbar = np.empty_like(bins)
            lng = np.empty_like(bins)

            for i in range(len(bins)):
                inBin = np.abs(r - bins[i]) < 0.5*binWidth
                #inBin = np.logical_and(inBin, lnGrad > -2e-7)
                if len(angleBin) > 1:
                    inBin = np.logical_and(inBin, np.abs(theta - a) < 0.5*binAngle)
                inBin = np.where(inBin)
                    
                rbar[i] = np.mean(r[inBin])
                lng[i] = np.median(lnGrad[inBin]) if len(inBin[0]) else 0
                if False and len(inBin[0]) > 10 and not profile:
                    lng[i] = np.nan
                color = scalarMap.to_rgba(a*180/np.pi) if binAngle else "black"
                if False and lng[i] < -0.6e-7 and rbar[i] < 10000:
                    pyplot.plot(rbar[i] + np.zeros_like(lnGrad[inBin]), lnGrad[inBin], '*',
                                color=color, markeredgewidth=0, alpha=alpha)
                    #pyplot.show(); import pdb; pdb.set_trace() 

            if profile:
                lng = np.exp(np.cumsum(binWidth*lng))

            pyplot.plot(rbar, lng, 'o', markersize=5.0, markeredgewidth=0, color=color, alpha=alpha,
                        label="%6.1f" % (a*180/np.pi))
            if len(angleBin) > 1:
                pyplot.legend(loc="lower left")
    else:
        pyplot.scatter(r, lnGrad, c=theta*180/np.pi, norm=norm, cmap=cmap, alpha=alpha,
                       marker='o', s=2.0, edgecolors="none")
        pyplot.colorbar()

    pyplot.xlim(-100, 18000); 
    if profile:
        pyplot.ylim(0.5, 1.05)
    else:
        pyplot.ylim(-15e-5, 5e-5)
    pyplot.xlabel("R/pixels")
    pyplot.ylabel("I" if profile else "d lnI/d r")
    if title:
        pyplot.title(title)
        
    pyplot.show()

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    
def test():
    mi = afwImage.MaskedImageF(10, 12)
    im = mi.getImage()
    width, height = mi.getDimensions()
    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    
    z, dzdx, dzdy = 10, 0.4, 2
    plane = z + dzdx*X + dzdy*Y

    im.getArray()[:] = plane
    im.getArray()[:] += np.random.normal(0, 1, im.getArray().shape)

    im[3, 4] += 100
    im[7, 5] -= 100

    b, res = fitPlane(mi, returnResidualImage=True)

    ds9.mtv(mi, frame=5)
    ds9.mtv(res, frame=6)

    print b

def test2():
    """Fit to an image initialised to sin(x)*cos(y) + noise"""
    xsize, ysize = 16, 16
    nx, ny = 20, 30
    mi = afwImage.MaskedImageF(nx*xsize, ny*ysize)
    im = mi.getImage()

    width, height = im.getDimensions()
    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    z, dzdx, dzdy = 1000, 2, 1

    ax = 2*np.pi/width
    ay = 4*np.pi/height
    plane = z + dzdx/ax*np.sin(ax*X) + dzdy/ay*np.cos(ay*Y)

    im.getArray()[:] = plane
    im.getArray()[:] += np.random.normal(0, 10, im.getArray().shape)
    #
    # Multiply each block by a random number (!= 0)
    #
    im0 = im.clone()
    for iy in range(ny):
        for ix in range(nx):
            im[ix*xsize:(ix + 1)*xsize, iy*ysize:(iy + 1)*ysize] *= 1 + 0.05*np.random.poisson(4)

    dlnzdx, dlnzdy, dlnzdr, residualImage = fitPatches(mi, nx, ny)

    recon = reconstruct(xsize, ysize, dzdx, dzdy)

    frame = 5
    ds9.mtv(im0,  title="im0",   frame=frame); frame += 1
    ds9.mtv(im,   title="im",    frame=frame); frame += 1
    ds9.mtv(dlnzdx, title="dlnz/dx", frame=frame); frame += 1
    ds9.mtv(dlnzdy, title="dlnz/dy", frame=frame); frame += 1
    ds9.mtv(recon, title="recon", frame=frame); frame += 1
