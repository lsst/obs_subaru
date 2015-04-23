import math
import os
import re
import sys
import matplotlib.pyplot as plt;  pyplot = plt
import numpy as np
try:
    import scipy
    import scipy.interpolate
except ImportError:
    scipy = None

import lsst.pex.config as pexConfig
import lsst.afw.cameraGeom as afwCG
import lsst.afw.cameraGeom.utils as afwCGUtils
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.display.ds9 as ds9
import lsst.afw.display.utils as ds9Utils
import lsst.meas.algorithms as measAlg
import lsst.analysis.utils as anUtils

def getNameOfSet(vals):
    """Convert a list of numbers into a string, merging consecutive values"""
    if not vals:
        return ""

    def addPairToName(valName, val0, val1):
        """Add a pair of values, val0 and val1, to the valName list"""
        sval1 = str(val1)
        if val0 != val1:
            pre = os.path.commonprefix([str(val0), sval1])
            sval1 = int(sval1[len(pre):])
        valName.append("%s-%s" % (val0, sval1) if val1 != val0 else str(val0))

    valName = []
    val0 = vals[0]; val1 = val0
    for val in vals[1:]:
        if isinstance(val, int) and val == val1 + 1:
            val1 = val
        else:
            addPairToName(valName, val0, val1)
            val0 = val; val1 = val0

    addPairToName(valName, val0, val1)

    return ", ".join(valName)


def trim(im, ccd=None):
    if not ccd:
        ccd = im.getDetector()

    for attr in ["getMaskedImage", "convertF"]:
        if hasattr(im, attr):
            im = getattr(im, attr)()

    tim = im[ccd.getAllPixels(True)].clone()

    for ia, a in enumerate(ccd):
        sim = im[a.getRawDataBBox()]
        sim -= afwMath.makeStatistics(im[a.getRawHorizontalOverscanBBox()], afwMath.MEANCLIP).getValue()
        a.setTrimmed(True)
        tim[a.getAllPixels(True)] <<= a.prepareAmpData(sim)

    return tim

def xcorrFromVisit(butler, v1, v2, ccds=[2], n=5, border=10, plot=False, zmax=0.05, fig=None, display=False):
    """Return an xcorr from a given pair of visits (and ccds)"""

    try:
        v1[0]
    except TypeError:
        v1 = [v1]
    try:
        v2[0]
    except TypeError:
        v2 = [v2]

    try:
        ccds[0]
    except TypeError:
        ccds = [ccds]

    ims = [None, None]
    means = [None, None]
    for i, vs in enumerate([v1, v2,]):
        for v in vs:
            for ccd in ccds:
                tmp = butler.get("raw", visit=v, ccd=ccd).convertF()
                if ims[i] is None:
                    ims[i] = tmp
                    im = ims[i].getMaskedImage()
                else:
                    im += tmp.getMaskedImage()

        nData = len(ccds)*len(vs)
        if nData > 1:
            im /= nData

        means[i] = afwMath.makeStatistics(im, afwMath.MEANCLIP).getValue()

        if display:
            ds9.mtv(trim(ims[i]), frame=i, title=v)

    mean = np.mean(means)

    xcorrImg = xcorr(*ims, n=n, border=border, frame=len(ims) if display else None)
    xcorrImg /= float(xcorrImg[0, 0])

    if plot:
        plotXcorr(xcorrImg, title=r"Visits %s; %s, CCDs %s  $\langle{I}\rangle = %.0f$ (%s)" %
                  (getNameOfSet(v1), getNameOfSet(v2), getNameOfSet(ccds),
                   mean, ims[0].getFilter().getName()), zmax=zmax, fig=fig)

    return xcorrImg

def getImageLevels(butler, **kwargs):
    for visit, ccd in butler.queryMetadata('raw', 'visit', ['visit', 'ccd',], **kwargs):
        exp = butler.get('raw', visit=visit, ccd=ccd)
        try:
            ccd = exp.getDetector()
            im = trim(exp.getMaskedImage().getImage(), ccd)
        except Exception, e:
            print >> sys.stderr, "Skipping %d %s: %s" % (visit, ccd.getId(), e)
            continue

        print "%d %3d %.1f" % (visit, ccd.getId().getSerial(),
                               afwMath.makeStatistics(im, afwMath.MEANCLIP).getValue())

def xcorr(im1, im2, n=5, border=10, frame=None):
    """Calculate the cross-correlation of two images im1 and im2 (using robust measures of the covariance)
    Maximum lag is n, and ignore border pixels around the outside
    """
    ims = [im1, im2]
    for i, im in enumerate(ims):
        ccd = im.getDetector()
        try:
            frameId = int(re.sub(r"^SUPA0*", "", im.getMetadata().get("FRAMEID")))
        except:
            frameId = -1
        #
        # Starting with an Exposure, MaskedImage, or Image trim the data and convert to float
        #
        for attr in ("getMaskedImage", "getImage"):
            if hasattr(im, attr):
                im = getattr(im, attr)()

        try:
            im  = im.convertF()
        except AttributeError:
            pass

        ims[i] = trim(im, ccd)

        for a in ccd:
            smi = ims[i][a.getDataSec(True)]

            mean = afwMath.makeStatistics(smi, afwMath.MEDIAN).getValue()
            gain = a.getElectronicParams().getGain()
            smi *= gain
            smi -= mean*gain

    im1, im2 = ims
    #
    # Actually diff the images
    #
    diff = im1.clone(); diff -= im2
    diff = diff[border:-border, border:-border]

    sctrl = afwMath.StatisticsControl()
    sctrl.setNumSigmaClip(5)
    #
    # Subtract background.  It should be a constant, but it isn't always (e.g. some SuprimeCam flats)
    #
    binsize = 128
    nx = diff.getWidth()//binsize
    ny = diff.getHeight()//binsize
    bctrl = afwMath.BackgroundControl(nx, ny, sctrl, afwMath.MEANCLIP)
    bkgd = afwMath.makeBackground(diff, bctrl)
    diff -= bkgd.getImageF(afwMath.Interpolate.CUBIC_SPLINE, afwMath.REDUCE_INTERP_ORDER)

    if frame is not None:
        ds9.mtv(diff, frame=frame, title="diff")

    if False:
        global diffim
        diffim = diff
    if False:
        print afwMath.makeStatistics(diff, afwMath.MEDIAN, sctrl).getValue()
        print afwMath.makeStatistics(diff, afwMath.VARIANCECLIP, sctrl).getValue(), np.var(diff.getArray())
    #
    # Measure the correlations
    #
    dim0 = diff[0: -n, : -n]
    w, h = dim0.getDimensions()

    xcorr = afwImage.ImageF(n + 1, n + 1)
    for di in range(n + 1):
        for dj in range(n + 1):
            dim_ij = diff[di:di + w, dj: dj + h].clone()
            dim_ij -= afwMath.makeStatistics(dim_ij, afwMath.MEANCLIP, sctrl).getValue()

            dim_ij *= dim0
            xcorr[di, dj] = afwMath.makeStatistics(dim_ij, afwMath.MEANCLIP, sctrl).getValue()

    return xcorr

def plotXcorr(xcorr, zmax=0.05, title=None, fig=None):
    if fig is None:
        fig = plt.figure()
    else:
        fig.clf()

    ax = fig.add_subplot(111, projection='3d')
    ax.azim = 30
    ax.elev = 20

    nx, ny = xcorr.getDimensions()
    xcorr = xcorr.getArray()

    xpos, ypos = np.meshgrid(np.arange(nx), np.arange(ny))
    xpos = xpos.flatten()
    ypos = ypos.flatten()
    zpos = np.zeros(nx*ny)
    dz = xcorr.flatten()
    dz[dz > zmax] = zmax

    ax.bar3d(xpos, ypos, zpos, 1, 1, dz, color='b', zsort='max', sort_zpos=100)
    if xcorr[0, 0] > zmax:
        ax.bar3d([0], [0], [zmax], 1, 1, 1e-4, color='c')

    ax.set_xlabel("row")
    ax.set_ylabel("column")
    ax.set_zlabel(r"$\langle{(I_i - I_j)(I_i - I_k)}\rangle/\sigma^2$")

    if title:
        fig.suptitle(title)

    fig.show()

    return fig

def findBiasLevels(butler, **dataId):
    keys = ["visit", "ccd"]
    dataType = "raw_md"
    for did in butler.queryMetadata(dataType, "visit", keys, **dataId):
        did = dict(zip(keys, did))
        if did["visit"] not in range(902030,902076+1):
            continue

        try:
            calexp = butler.get(dataType, did, immediate=True)
        except Exception, e:
            print e
            continue

        print did["visit"], calexp.get("RA2000"),  calexp.get("DEC2000")
        continue

        ccd = calexp.getDetector()
        ccdImage = calexp.getMaskedImage().getImage()
        for amp in ccd:
            bias = afwMath.makeStatistics(ccdImage[amp.getBiasSec()], afwMath.MEANCLIP).getValue()
            data = afwMath.makeStatistics(ccdImage[amp.getDataSec()], afwMath.MEANCLIP).getValue()
            print did, amp.getId().getSerial(), "%7.2f, %7.2f" % (bias, data)


def showElectronics(camera, maxCorr=1.8e5, showLinearity=False, fd=sys.stderr):
    print >> fd,  "%-12s" % (camera.getId())
    for raft in [afwCG.cast_Raft(det) for det in camera]:
        print >> fd,  "   %-12s" % (raft.getId())
        for ccd in [det for det in raft]:
            print >> fd,  "      %-12s" % (ccd.getId())
            for amp in ccd:
                ep = amp.getElectronicParams()
                if showLinearity:
                    linearity = ep.getLinearity()
                    print >> fd,  "         %s %s %9.3g %.0f" % (
                        amp.getId(),
                        ("PROPORTIONAL" if linearity.type == afwCG.Linearity.PROPORTIONAL else "Unknown"),
                        linearity.coefficient,
                        (linearity.maxCorrectable if linearity.maxCorrectable < 1e30  else np.nan))
                else:
                    print >> fd,  "         %s %.2f %.1f %.1f" % (
                        amp.getId(), ep.getGain(), ep.getSaturationLevel(), maxCorr/ep.getGain())

def estimateGains(butler, ccdNo, visits, nGrow=0, frame=None):
    """Estimate the gain for ccd ccdNo using flat fields called visits[0] and visits[1]

Grow BAD regions in the flat field by nGrow pixels (useful for vignetted chips)
    """

    if len(visits) != 2:
        raise RuntimeError("Please provide 2 visits (saw %s)" % ", ".join([str(v) for v in visits]))

    class AD(object):
        def __init__(self, amp, ampImage, mean):
            self.amp = amp
            self.ampImage = ampImage
            self.mean = mean

    flatMask = butler.get("flat", taiObs="2013-06-15", ccd=ccdNo, visit=visits[0]).getMaskedImage().getMask()
    BAD = flatMask.getPlaneBitMask("BAD")
    bad = afwDet.FootprintSet(flatMask, afwDet.Threshold(BAD, afwDet.Threshold.BITMASK))
    bad = afwDet.FootprintSet(bad, nGrow, False)
    afwDet.setMaskFromFootprintList(flatMask, bad.getFootprints(), BAD)

    sctrl = afwMath.StatisticsControl()
    sctrl.setNumSigmaClip(3)
    sctrl.setAndMask(BAD)

    ccds, ampData = {}, {}
    for v in visits:
        exp = butler.get("raw", visit=v, ccd=ccdNo)

        ccd = exp.getDetector()
        if False:
            im = afwCGUtils.trimRawCallback(exp.getMaskedImage().convertF(), ccd, correctGain=False)
        else:
            im = isrCallback(exp.getMaskedImage(), ccd, doFlatten=False, correctGain=False)
        im.getMask()[:] |= flatMask

        ampData[ccd] = []
        for a in ccd:
            aim = im[a.getAllPixels()]

            mean = afwMath.makeStatistics(aim, afwMath.MEANCLIP, sctrl).getValue()
            ampData[ccd].append(AD(a, aim, mean))

        ccds[v] = ccd
        if False and frame is not None:
            ds9.mtv(im, frame=frame)
            raw_input("Continue? ")

    ampMeans = []
    ccd0 = ccds[visits[0]]
    ccd1 = ccds[visits[1]]
    for ai in range(len(ampData[ccd0])):
        # Normalize all images to the same level
        ampMean = np.mean(np.array([ampData[ccds[v]][ai].mean for v in visits]))
        ampMeans.append(ampMean)
        for v in visits:
            ad = ampData[ccds[v]]
            ampImage = ad[ai].ampImage
            ampImage *= ampMean/ad[ai].mean
        #
        # Here we specialise to just two input images
        #
        diff = ampData[ccd1][ai].ampImage

        amp = ampData[ccd0][ai].amp

        ima = [ampData[c][ai].ampImage.getImage().getArray() for c in [ccd0, ccd1]]
        diff.getImage().getArray()[:] = (ima[0] - ima[1])/np.sqrt(0.5*(ima[0] + ima[1]))
        gain = 2/afwMath.makeStatistics(diff, afwMath.VARIANCECLIP, sctrl).getValue()
        if not np.isfinite(gain):
            gain = amp.getElectronicParams().getGain()

        print "%s %d %.2f -> %.2f" % (ccd.getId().getName(), amp.getId().getSerial(),
                                      amp.getElectronicParams().getGain(), gain)

        amp.getElectronicParams().setGain(gain)

    if frame is not None:
        ima = []
        for v in visits[0:2]:
            exp = butler.get("raw", visit=v, ccd=ccdNo)

            ccd = exp.getDetector()
            im = isrCallback(exp.getMaskedImage(), ccd, doFlatten=True)
            ima.append(im)

            ds9.mtv(im, frame=frame, title="%d %d" % (v, ccdNo))
            frame += 1

        diff = ima[0].clone()
        ima = [_.getImage().getArray() for _ in ima]
        diff.getImage().getArray()[:] = (ima[0] - ima[1])/np.sqrt(0.5*(ima[0] + ima[1]))
        ds9.mtv(diff, frame=frame, title='diff')

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def dumpRaftParams(raft, nIndent=0, dIndent=4, fd=sys.stdout, nCol=15, nRow=8):
    """Write a Raft's parameters a camera.paf file"""

    indent = "%*s" % (nIndent, "")
    dindent = "%*s" % (dIndent, "")

    print >> fd, "%sRaft: {" % (indent)
    print >> fd, "%snCol: %d\t\t%s" % (indent + dindent, nCol, "# number of columns of CCDs")
    print >> fd, "%snRow: %d\t\t%s" % (indent + dindent, nRow, "# number of rows of CCDs")

    for ccd in raft:
        dumpCcdParams(ccd, nIndent + dIndent, dIndent, fd=fd)

    print >> fd, "%s}" % (indent)

def dumpCcdParams(ccd, nIndent=0, dIndent=4, fd=sys.stdout, iC=0, iR=0):
    """Write a Ccd's parameters a camera.paf file"""
    indent = "%*s" % (nIndent, "")
    dindent = "%*s" % (dIndent, "")

    print >> fd, "%sCcd: {" % indent
    print >> fd, "%sserial: %d" % (indent + dindent, ccd.getId().getSerial())
    print >> fd, "%sname: \"%s\"" % (indent + dindent, ccd.getId().getName())
    print >> fd, "%sptype: \"default\"" % (indent + dindent)

    orient = ccd.getOrientation()

    width, height = ccd.getAllPixels(True).getWidth(), ccd.getAllPixels(True).getHeight()
    xc, yc = 0.5*(width - 1), 0.5*(height - 1) # center of CCD

    x, y = ccd.getPositionFromPixel(afwGeom.PointD(xc, yc)).getMm()

    print >> fd, "%sindex: %d %d" % (indent + dindent, iC, iR)
    print >> fd, "%soffset: %10.2f %10.2f\t%s" % (indent + dindent, x, y,
                                                  "# offset of CCD center from raft center, (x, y)")
    print >> fd, "%snQuarter: %d\t\t\t%s" % (indent + dindent, orient.getNQuarter(),
                                               "# number of quarter turns applied to CCD when put into raft")
    print >> fd, "%sorientation: %f %f %f\t\t%s" % (indent + dindent,
                                                    math.degrees(orient.getPitch()),
                                                    math.degrees(orient.getRoll()),
                                                    math.degrees(orient.getYaw()),
                                                    "# pitch, roll, yaw; degrees")
    print >> fd, "%s}" % (indent)


def dumpCcdElectronicParams(ccd, nIndent=0, dIndent=4, fd=sys.stdout):
    """Write a Ccd's ElectronicParams to the Electronics section of a camera.paf file"""
    indent = "%*s" % (nIndent, "")
    dindent = "%*s" % (dIndent, "")

    print >> fd, "%sCcd: {" % indent
    print >> fd, "%sname: \"%s\"" % (indent + dindent, ccd.getId().getName())
    print >> fd, "%sptype: \"default\"" % (indent + dindent)
    print >> fd, "%sserial: %d" % (indent + dindent, ccd.getId().getSerial())

    for i, a in enumerate(ccd):
        ep = a.getElectronicParams()
        print >> fd, "%sAmp: {" % (indent + dindent)
        print >> fd, "%sindex: %d 0" % (indent + 2*dindent, i)
        print >> fd, "%sgain: %.4f" % (indent + 2*dindent, ep.getGain())
        print >> fd, "%sreadNoise: %.1f" % (indent + 2*dindent, ep.getReadNoise())
        print >> fd, "%ssaturationLevel: %.1f" % (indent + 2*dindent, ep.getSaturationLevel())
        print >> fd, "%s}" % (indent + dindent)

    print >> fd, "%s}" % (indent)

def dumpRaftElectronicParams(raft, nIndent=0, dIndent=4, fd=sys.stdout):
    """Write a Raft's ElectronicParams to the Electronics section of a camera.paf file
    nIndent is the initial indent, dIndent the indent offset for each extra level of nesting
    """
    indent = "%*s" % (nIndent, "")
    dindent = "%*s" % (dIndent, "")

    print >> fd, "%sRaft: {" % (indent)
    print >> fd, "%sname: \"%s\"" % (indent + dindent, raft.getId().getName())

    for ccd in raft:
        dumpCcdElectronicParams(ccd, nIndent + dIndent, dIndent, fd=fd)

    print >> fd, "%s}" % (indent)

def dumpCameraElectronicParams(camera, nIndent=0, dIndent=4, outFile=None):
    """Write the camera's ElectronicParams to the Electronics section of a camera.paf file"""
    if outFile:
        fd = open(outFile, "w")
    else:
        fd = sys.stdout

    dindent = "%*s" % (dIndent, "")

    print >> fd, """\
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#
# Electronic properties of our CCDs
#"""
    print >> fd, "Electronic: {"
    for raft in camera:
        dumpRaftElectronicParams(afwCG.cast_Raft(raft), dIndent, dIndent, fd=fd)
    print >> fd, "}"


#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


def plotVignetting(r2=1.05, D=None, phi=None, relative=False, showCamera=False, contour=False,
                   pupilImage=None, fig=1, xmin=-0.05, xmax=None, ymin=None, ymax=1.05):
    r1 = 1

    fig = anUtils.getMpFigure(fig)
    axes = fig.add_axes((0.1, 0.1, 0.85, 0.80));

    xlabel = "Offset of vignetting disk (in units of primary radius)"

    title = ""
    if showCamera:
        if relative:
            title += "extra "
        title += "vignetting" if D is None else "vignetting due to occulting edge at %.2f R_prim. " % D

    title += " R_lens/R_prim = %.2f" % r2

    if showCamera:
        nradial, nangular = 50, 8

        nxy = 21
        X, Y = np.meshgrid(np.linspace(-xmax, xmax, nxy), np.linspace(-xmax, xmax, nxy))
        d = np.hypot(X, Y)
        if False:
            X = X[d < xmax]
            Y = Y[d < xmax]
            d = d[d < xmax]

        vig = calculateVignetting(d, r1, r2, analytic=False, D=D, phi=np.arctan2(Y, X), pupilImage=pupilImage)

        if relative:
            vig /= calculateVignetting(d, r1, r2, analytic=True)

        vig /= vig.max()

        if contour:
            sc = axes.contour(X, Y, vig, 15)
            axes.clabel(sc, inline=1, fontsize=10)
        else:
            sc = axes.pcolor(X, Y, vig, norm=anUtils.pyplot.Normalize(ymin, ymax))
            fig.colorbar(sc)

            import matplotlib.patches as patches
            patch = patches.Circle((0, 0), radius=xmax, transform=sc.axes.transData, facecolor='red')
            sc.set_clip_path(patch)

        axes.set_xlim(-xmax, xmax)
        axes.set_ylim(-xmax, xmax)

        ylabel = xlabel
        axes.set_aspect('equal')
    else:
        nradial, nangular = 50, 8
        d = np.linspace(0, r1 + r2, nradial)

        ylabel = "Vignetting"

        if D is None:
            axes.plot(d, calculateVignetting(d, r1, r2, analytic=True),  color='red',  label='exact')
            axes.plot(d, calculateVignetting(d, r1, r2, D, phi, False, pupilImage=pupilImage),
                      color='blue', label='Sampled')
        else:
            if phi is None:
                phi = np.linspace(0, np.pi, nangular + 1)[:-1]
            else:
                phi = [phi]

            vig = calculateVignetting(d, r1, r2, analytic=False, pupilImage=pupilImage)
            if relative:
                vig0 = vig.copy()
                ylabel = "Vignetting relative to no occultation"
            else:
                axes.plot(d, vig,  label='No occultation')

            for phi in phi:
                vig = calculateVignetting(d, r1, r2, D, phi, False, pupilImage=pupilImage)
                if relative:
                    vig /= vig0

                axes.plot(d, vig, label='$%5.1f^\circ$' % (phi*180/np.pi))

        if False:
            axes.axvline(np.sqrt(2), linestyle=':', color="black")
            axes.axhline(0.5*(1 - 2/np.pi), linestyle=':', color="black")
            axes.axhline(1 - 0.25*(1 - 2/np.pi), linestyle=':', color="black")

        axes.legend(loc="lower left")

        axes.set_xlim(xmin, xmax)
        axes.set_ylim(ymin, ymax)

    axes.set_xlabel(xlabel)
    axes.set_ylabel(ylabel)
    axes.set_title(title)
    fig.show()


def calculateVignetting(d, r1=1, r2=1, D=None, phi=0, analytic=True, pupilImage=None):
    """Calculate the vignetting based on a simple 2-circle overlap model

    The primary has radius r1, and the vignetting disk (the front element of the corrector)
    has radius r2.  The separation of the two centres is d

    If analytic, use the exact formula otherwise simulate "rays"

    If D is non-None it's the distance to an edge that occults the primary, making an angle
    phi with the line joining the primary and the centre of the occulting disk
    """
    if analytic:
        theta1 = np.arccos((r1**2 + d**2 - r2**2)/(2*r1*d)) # 1/2 angle subtended by chord where
        theta2 = np.arccos((r2**2 + d**2 - r1**2)/(2*r2*d)) #    pupil and vignetting disk cross

        missing = 0.5*(r1**2*(2*theta1 - np.sin(2*theta1)) + r2**2*(2*theta2 - np.sin(2*theta2)))
        return np.where(d <= r2 - r1, 1.0, missing/(np.pi*r1**2))
    #
    # OK, we need to do the estimate numerically
    #
    ntheta = 200
    nradial = 200

    #ntheta = 1000; nradial = 1000

    thetas = np.linspace(0, 2*np.pi, ntheta)
    r = np.sqrt(np.linspace(0, r1**2, nradial))

    if pupilImage is not None:
        pupilImage = pupilImage.clone()

        pupilImage = pupilImage.getArray()
        h, w = pupilImage.shape

        if not False:
            pupilImage[:] = 1
            if False:
                D = 0.3
                pupilImage[:, np.arange(w) > int((1 - D)*w)] = 0

    if False:
        d = np.sqrt(2)
        #d = 0
        d = float(d)
    try:
        d[0]
    except TypeError:
        d = [d]

    if len(phi) == 1 and pupilImage is None and len(r) > len(d): # faster to vectorize r than d
        for i, _d in enumerate(d):
            ngood = 0
            for theta in thetas:
                good = _d**2 + r**2 - 2*r*_d*np.cos(theta) < r2**2
                if D is not None:
                    good = np.logical_and(good, r*np.cos(theta - phi) < D)

                ngood += pupilImage[y[good], x[good]].sum()

            vig[i] = float(ngood)
    else:
        vig = np.zeros_like(d)
        for _r in r:
            for theta in thetas:
                good = d**2 + _r**2 - 2*_r*d*np.cos(theta) < r2**2
                if D is not None:
                    good = np.logical_and(good, _r*np.cos(theta - phi) < D)

                if pupilImage is None:
                    vig += good
                else:
                    x, y = _r*np.cos(theta - phi), _r*np.sin(theta - phi)
                    x = ((w - 1)*(r1 + x)/(2*r1)).astype(int)
                    y = ((h - 1)*(r1 + y)/(2*r1)).astype(int)

                    vig += np.where(good, pupilImage[y, x], 0)

    if pupilImage is None:
        vig /= float(len(thetas)*len(r))
    else:
       pupilFlux = 0.0
       for _r in r:
           x, y = _r*np.cos(thetas), _r*np.sin(thetas)
           x = ((w - 1)*(r1 + x)/(2*r1)).astype(int)
           y = ((h - 1)*(r1 + y)/(2*r1)).astype(int)

           pupilFlux += sum(pupilImage[y, x])

       vig /= pupilFlux

    return vig

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def estimatePupilIllumination(mi, centers=(( -34,   93), ( 131, -192), (-195, -192),
                                           (-197,  377), (-358,   92), ( 293,   93),),
                              R=48.8, threshold=2500, nGrow=0):
    """Estimate the pupil illumination image from pinhole camera data (pupil has radius R)

    Parameters are tuned to visit 902436/902438 (dome screen; pinhole camera)
    """
    mi = mi.clone()
    fs = afwDet.FootprintSet(mi, afwDet.Threshold(threshold))
    fs = afwDet.FootprintSet(fs, nGrow, True)

    BAD = mi.getMask().getPlaneBitMask("BAD")
    EDGE = mi.getMask().getPlaneBitMask("EDGE")
    afwDet.setMaskFromFootprintList(mi.getMask(), fs.getFootprints(), BAD) # we'll flip it in a moment

    x0, y0 = mi.getXY0()
    if False:
        ds9.mtv(mi)
        for x, y in centers:
            ds9.dot('o', x - x0, y - y0, size=R, ctype=ds9.GREEN)

    imgList = afwImage.vectorMaskedImageF()
    size = 2*(int(2*R + 0.5)//2) + 1
    for x, y in centers:
        stamp = mi[x - x0 - size//2 : x - x0 + size//2, y - y0 - size//2 : y - y0 + size//2]
        #
        # Scale to constant surface brightness
        #
        fs = afwDet.FootprintSet(stamp.getMask(), afwDet.Threshold(BAD, afwDet.Threshold.BITMASK))
        Ipupil, Npupil = 0.0, 0
        for f in fs.getFootprints():
            for s in f.getSpans():
                iy = s.getY() - stamp.getY0()
                for ix in range(s.getX0() - stamp.getX0(), s.getX1() - stamp.getX0() + 1):
                    Ipupil += stamp.get(ix, iy)[0]
                    Npupil += 1

        stamp.getImage()[:] /= Ipupil/Npupil # normalise to constant surface brightness.  Ignore Jacobian

        stamp.getMask().getArray()[:] ^= BAD
        imgList.push_back(stamp)

    sctrl = afwMath.StatisticsControl()
    sctrl.setAndMask(BAD)
    out = afwMath.statisticsStack(imgList, afwMath.MEANCLIP, sctrl)
    out.setXY0(-size//2 + 1, -size//2 + 1)
    nim = out.getImage().getArray()
    nim[np.where(np.logical_not(np.isfinite(nim)))] = 0.0
    out.getMask()[:] &= ~(BAD | EDGE)
    del imgList

    ds9.mtv(out, frame=8)
    ds9.dot('+', -out.getX0(), -out.getY0(), frame=8)
    ds9.dot('o', -out.getX0(), -out.getY0(), size=R, ctype=ds9.RED, frame=8)

    return out.getImage()

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def removeCRs(mi, background=None, crNGrow=1):
   #
   # Estimate the PSF
   #
   FWHM = 5                      # pixels
   psf = measAlg.DoubleGaussianPsf(29, 29, FWHM/(2*math.sqrt(2*math.log(2))))

   if not hasattr(mi, "getImage"):
       mi = afwImage.makeMaskedImage(mi)
       mi.getVariance()[:] = mi.getImage()

   if background is None:
       background = afwMath.makeStatistics(mi.getImage(), afwMath.MEANCLIP).getValue()
       background = min(background, 100)

   crConfig = measAlg.FindCosmicRaysConfig()
   if True:
       crConfig.nCrPixelMax = 1000000
       crConfig.minSigma = 5
       crConfig.min_DN = 150
   crs = measAlg.findCosmicRays(mi, psf, background, pexConfig.makePolicy(crConfig))

   return mi.getImage()

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def XXXremoveCRs(mi, background, maskBit=None, crThreshold=8, crNpixMax=100, crNGrow=1):
    x0, y0 = mi.getXY0()

    if hasattr(mi, "getImage"):
        im = mi.getImage()
    else:
        im = mi
    im = im.getArray()

    xv, yv = [], []
    for ng in (0, crNGrow):
        fs = afwDet.FootprintSet(mi, afwDet.Threshold(crThreshold))
        if ng > 0:
            fs = afwDet.FootprintSet(fs, ng, False)

            for foot in fs.getFootprints():
                if foot.getNpix() > crNpixMax:
                    continue
                for s in foot.getSpans():
                    y = s.getY() - y0
                    for x in range(s.getX0() - x0, s.getX1() - x0 + 1):
                        xv.append(x); yv.append(y)

        im[yv, xv] = background + 0*im[yv, xv] # keep NaNs NaN
        if maskBit is not None:
            if hasattr(mi, "getMask"):
                mi.getMask().getArray()[yv, xv] |= maskBit

    return mi

def estimateScattering(mi, threshold=2000, nGrow=2, npixMin=35, frame=None, invertDetected=False):
    """Estimate the fraction of scattered light from pinhole images
    """
    mi = mi.clone()
    x0, y0 = mi.getXY0()

    im = mi.getImage().getArray()
    background = im[np.isfinite(im)].flatten(); background.sort()
    background = float(background[int(0.5*len(background))])

    DETECTED = mi.getMask().getPlaneBitMask("DETECTED")
    CR = mi.getMask().getPlaneBitMask("CR")
    #
    if True:                            # Mask CCD 33 with its glowing amp
        mi.getImage()[1332:1460, 845:1106] = np.nan
        mi.getImage()[1332:1341, 1110:1146] = np.nan
        mi.getImage()[1282:1327, 1094:1106] = np.nan

    if True:                            # remove some bad columns
        mi.getImage()[1217, 1950:2210] = background
        mi.getImage()[1599:1773, 249] = background
        mi.getImage()[1599:1767, 248] = background
        mi.getImage()[1599:1771, 187] = background

        mi.getImage()[1923, 968] = background # cut CRs from pupils
        mi.getImage()[1451,  99] = background

    removeCRs(mi, background)
    #
    # Now find the pupil images
    #
    fs = afwDet.FootprintSet(mi, afwDet.Threshold(threshold), "", npixMin)
    fs = afwDet.FootprintSet(fs, nGrow, True)

    afwDet.setMaskFromFootprintList(mi.getMask(), fs.getFootprints(), DETECTED)

    arr = mi.getImage().getArray()
    msk = mi.getMask().getArray()

    finite = np.where(np.isfinite(arr))
    arr = arr[finite]
    msk = msk[finite]

    frac = 1 - sum(arr[np.where(msk & DETECTED)])/sum(arr)
    notDetected = np.where(np.bitwise_not(msk) & DETECTED)
    print "median: %.2f" % (np.median(arr[notDetected]))
    print "from median: %.2f%%" % (100*np.median(arr[notDetected])*len(notDetected[0])/sum(arr))

    if frame is not None:
        if invertDetected:
            mi.getMask()[:] ^= DETECTED

        ds9.mtv(mi, frame=frame)

    return frac

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def estimateDarkCurrent(butler, visits, nSkipRows=35, frame=None):
    print "CCD",
    for visit in visits:
        print "%8d" % (visit),
    print "%8s" % "mean"

    ccds = range(100)                   # don't use 100..103

    cameraDark = 0.0                    # average dark current for all ccds
    for ccdNo in ccds:
        print "%3d" % ccdNo,
        darkValues = np.empty(len(visits))
        for i, visit in enumerate(visits):
            raw = butler.get("raw", visit=visit, ccd=ccdNo)
            ccd = raw.getDetector()
            rim = raw.getMaskedImage().getImage().convertF()

            for a in ccd:
                rim[a.getAllPixels()] -= afwMath.makeStatistics(rim[a.getBiasSec()], afwMath.MEANCLIP).getValue()

            if frame is not None:
                if True:
                    ds9.mtv(rim, frame=frame)
                else:
                    ds9.erase(frame=frame)

            dark = []
            for a in ccd:
                dataSec = a.getDataSec()

                ystart = dataSec.getMaxY() + nSkipRows + 1
                if ystart > raw.getHeight() - 1:
                    ystart = dataSec.getMaxY() + 1 + 2 # +2 for CTE

                darkSec = afwGeom.BoxI(afwGeom.PointI(dataSec.getMinX(), ystart),
                                       afwGeom.PointI(dataSec.getMaxX(), raw.getHeight() - 1))
                if frame is not None:
                    ds9Utils.drawBBox(darkSec, borderWidth=0.4, frame=frame)
                dark.append(afwMath.makeStatistics(rim[darkSec], afwMath.MEANCLIP).getValue())

            val = np.median(dark)
            darkValues[i] = val

            print "%8.3f" % val,

        val = np.mean(darkValues)
        print "%8.3f" % val
        cameraDark += val

    print "%8.3f" % (cameraDark/len(ccds))

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def plotCcdQE(butler, filter, zmin=None, zmax=None, filter2=None, relative=False, canonical=None, fig=None):
    camera = butler.get("camera")

    filters, filterNames = [], []
    for f in (filter, filter2):
        filterNames.append(f)
        if not f:
            filters.append(f)
            continue

        if filter in "grizy":
            filter = "HSC-%s" % filter

        filters.append(filter.upper())
    filter,     filter2 = filters; del filters
    filterName, filterName2 = filterNames; del filterNames

    visit = butler.queryMetadata("raw_md", "visit", filter=filter)[0]
    ccdIds = butler.queryMetadata('raw', 'visit', ['ccd',], visit=visit)
    ccdIds = [i for i in ccdIds if i < 100]

    md = butler.get("raw_md", visit=visit, ccd=ccdIds[0])
    xc, yc = 0.5*md.get("NAXIS1"), 0.5*md.get("NAXIS2")
    ccdCen = afwGeom.PointD(xc, yc)

    norm = pyplot.Normalize(zmin, zmax)

    qe = np.empty_like(ccdIds); qe.dtype = float
    xmm = np.empty_like(qe)
    ymm = np.empty_like(qe)
    for i, ccdId in enumerate(ccdIds):
        ccd = afwCGUtils.findCcd(camera, afwCG.Id(ccdId))

        xmm[i], ymm[i] = ccd.getPositionFromPixel(ccdCen).getMm()
        qe[i] = getQE(ccdId, filterName)
        if filterName2:
            qe[i] /= getQE(ccdId, filterName2)

    if relative:
        qe /= np.median(qe)
    #
    # Plotting
    #
    fig = anUtils.getMpFigure(fig)
    axes = fig.add_axes((0.1, 0.1, 0.85, 0.80));

    markersize=100
    sc = axes.scatter(xmm, ymm, c=qe, norm=norm,
                      cmap=pyplot.cm.rainbow, marker="o", s=markersize, edgecolors="none")
    fig.colorbar(sc)
    axes.set_aspect('equal')
    axes.set_xlabel("X/pixels")
    axes.set_ylabel("Y/pixels")
    title = "%s-band %sQE" % (filterName, ("relative " if relative else ""))
    if filterName2:
        title += " (divided by %s)" % filterName2
    axes.set_title(title)

    fig.show()

def plotCcdZP(butler, visit, correctJacobian=False, zlim=(None, None), visit2=None,
              rmax=None, ccdIds=[], fig=None):
    try:
        visit[0]
        listOfVisits = True
    except TypeError:
        listOfVisits = False
        visit = [visit]

    camera = butler.get("camera")

    if not ccdIds:
        ccdIds = butler.queryMetadata('raw', 'visit', ['ccd',], visit=visit[0])

    zp = np.empty_like(ccdIds); zp.dtype = float
    xmm = np.empty_like(zp)
    ymm = np.empty_like(zp)
    for i, ccdId in enumerate(ccdIds):
        ccd = afwCGUtils.findCcd(camera, afwCG.Id(ccdId))

        try:
            md = butler.get("raw_md", visit=visit[0], ccd=ccdId)
            xc, yc = 0.5*md.get("NAXIS1"), 0.5*md.get("NAXIS2")
            ccdCenMm = ccd.getPositionFromPixel(afwGeom.PointD(xc, yc)).getMm()
            xmm[i], ymm[i] = ccdCenMm
        except Exception as e:
            print e
            zp[i] = np.nan
            continue

        if rmax and np.hypot(*ccdCenMm) > rmax:
            zp[i] = np.nan
            continue

        zps = []
        if i == 0:
            filters = []
        for v in (visit, visit2):
            if listOfVisits and v == visit:
                zps = []
                for v in visit:
                    try:
                        calexp_md = butler.get("calexp_md", visit=v, ccd=ccdId, immediate=True)
                    except Exception, e:
                        if v is not None:
                            print e
                        continue

                    calib = afwImage.Calib(calexp_md)
                    zps.append(calib.getMagnitude(1.0) - 2.5*np.log10(calib.getExptime()))

                if zps:
                    ZP = np.median(zps)
                else:
                    zps.append(np.nan)
                    continue
            else:
                try:
                    v = v[0]
                except TypeError:
                    pass

                if v is None:
                    zps.append(np.nan)
                    continue

                try:
                    calexp_md = butler.get("calexp_md", visit=v, ccd=ccdId, immediate=True)
                except Exception, e:
                    print e
                    zps.append(np.nan)
                    continue

                calib = afwImage.Calib(calexp_md)
                try:
                    ZP = calib.getMagnitude(1.0) - 2.5*np.log10(calib.getExptime())
                except:
                    ZP = np.nan

            if correctJacobian:
                dist = ccd.getDistortion()
                ZP -= 2.5*np.log10(dist.computeQuadrupoleTransform(ccdCenMm, False).computeDeterminant())

            zps.append(ZP)
            if i == 0:
                filters.append(afwImage.Filter(calexp_md).getName())

        zp[i] = zps[0]
        if visit2:
            zp[i] -= zps[1]

    if visit2:
        median = np.median(zp)
        zp -= median
    #
    # Plotting
    #
    fig = anUtils.getMpFigure(fig)
    axes = fig.add_axes((0.1, 0.1, 0.85, 0.80));

    markersize=100
    try:
        zlim[0]
    except TypeError:
        if zlim is None:
            zlim = (None, None)
        else:
            zlim = np.median(zp) + np.array([-zlim, zlim])

    norm = pyplot.Normalize(*zlim)

    sc = axes.scatter(xmm, ymm, c=zp, norm=norm,
                      cmap=pyplot.cm.rainbow, marker="o", s=markersize, edgecolors="none")
    fig.colorbar(sc)
    axes.set_aspect('equal')
    axes.set_xlabel("X/pixels")
    axes.set_ylabel("Y/pixels")
    title = "Zero Points for %s[%s]" % (", ".join([str(v) for v in visit]), filters[0])
    if visit2:
        title +=  " - %d[%s]" % (visit2, filters[1])
        if median != 0:
            title += " - %.2f" % median

    mat = re.search(r"/rerun/(.*)", butler.mapper.root)
    if mat:
        title += " rerun %s" % mat.group(1)

    axes.set_title(title)

    fig.show()

    return fig

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def plotCcdTemperatures(butler, visit):
    camera = butler.get("camera")
    ccdIds = butler.queryMetadata('raw', 'visit', ['ccd',], visit=visit)

    temp = np.empty_like(ccdIds); temp.dtype = float
    xmm = np.empty_like(temp)
    ymm = np.empty_like(temp)
    for i, ccdId in enumerate(ccdIds):
        ccd = afwCGUtils.findCcd(camera, afwCG.Id(ccdId))
        md = butler.get("raw_md", visit=visit, ccd=ccdId)
        if i == 0:
            xc, yc = 0.5*md.get("NAXIS1"), 0.5*md.get("NAXIS2")
            ccdCen = afwGeom.PointD(xc, yc)

        xmm[i], ymm[i] = ccd.getPositionFromPixel(ccdCen).getMm()
        temp[i] = md.get("T_CCDTV")


    pyplot.clf()

    markersize=100
    pyplot.scatter(xmm, ymm, c=temp,
                   cmap=pyplot.cm.rainbow, marker="o", s=markersize,
                   edgecolors="none")
    pyplot.colorbar()
    pyplot.axes().set_aspect('equal')
    pyplot.xlabel("X/pixels")
    pyplot.ylabel("Y/pixels")
    pyplot.title("CCD Temperatures ($^\circ$C), visit %d" % (visit))

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def makeBias(images):
    imgList = afwImage.vectorMaskedImageF()
    for im in images:
        imgList.push_back(im)

    return afwMath.statisticsStack(imgList, afwMath.MEDIAN)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def subtractBiasNoTrim(exp):
    exp = exp.clone()
    ccd = exp.getDetector()

    if hasattr(exp, "convertF"):
        exp = exp.convertF()

    im = exp.getMaskedImage()

    for a in ccd:
        im[a.getDiskAllPixels()] -= afwMath.makeStatistics(im[a.getRawHorizontalOverscanBBox()], afwMath.MEANCLIP).getValue()

    return exp

def grads(exp):
    ccd = exp.getDetector()
    im = exp.getMaskedImage().getImage()

    plt.clf()
    for a in ccd:
        ima = im[a.getRawDataBBox()].getArray()
        I = np.mean(ima, 1)/np.median(ima)

        size = 512//16
        if False:
            import scipy.ndimage

            out = np.empty_like(I)
            scipy.ndimage.filters.median_filter(I, size=[size], output=out)
        else:
            w = np.hamming(size)
            out = np.convolve(w/w.sum(), I, mode='same')

        dout = out[10:] - out[:-10]
        dout /= out[10:]
        out = dout[0:-1:size//2]

        plt.plot(out[size:-size], label=str(a.getId().getSerial()))
    plt.legend(loc="upper left")
    plt.show()

    return ima

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

levels902986 = {                          # Set from g-band sky 902986 (~ 65k e/pixel)
    0 : 43403.91,
    1 : 44305.02,
    2 : 43475.00,
    3 : 46105.14,
    4 : 43734.02,
    5 : 45130.94,
    6 : 45646.65,
    7 : 47042.65,
    8 : 43301.74,
    9 : 43843.98,
    10 : 43978.67,
    11 : 46206.36,
    12 : 44293.16,
    13 : 46904.64,
    14 : 44390.88,
    15 : 45602.04,
    16 : 46750.84,
    17 : 45602.81,
    18 : 47514.49,
    19 : 46901.52,
    20 : 46220.73,
    21 : 46797.34,
    22 : 43524.61,
    23 : 43763.25,
    24 : 46964.68,
    25 : 45742.11,
    26 : 47609.61,
    27 : 45897.85,
    28 : 44727.69,
    29 : 44373.68,
    30 : 43167.49,
    31 : 45565.83,
    32 : 46906.66,
    33 : 45385.44,
    34 : 48517.57,
    35 : 45633.72,
    36 : 44505.52,
    37 : 44076.87,
    38 : 43484.00,
    39 : 44324.83,
    40 : 45357.87,
    41 : 47269.23,
    42 : 48451.24,
    43 : 47208.25,
    44 : 45896.54,
    45 : 46746.83,
    46 : 43871.89,
    47 : 47213.46,
    48 : 47345.57,
    49 : 47184.77,
    50 : 46363.21,
    51 : 44757.63,
    52 : 47038.79,
    53 : 43994.41,
    54 : 44049.64,
    55 : 45615.36,
    56 : 47352.31,
    57 : 46203.75,
    58 : 46172.26,
    59 : 46128.25,
    60 : 44624.29,
    61 : 44380.55,
    62 : 43105.61,
    63 : 45568.49,
    64 : 47884.43,
    65 : 47685.35,
    66 : 49160.41,
    67 : 44784.35,
    68 : 46512.36,
    69 : 44967.30,
    70 : 46890.92,
    71 : 47169.37,
    72 : 45134.80,
    73 : 46785.52,
    74 : 48786.36,
    75 : 47307.23,
    76 : 45419.64,
    77 : 45155.86,
    78 : 45410.07,
    79 : 45274.47,
    80 : 49517.01,
    81 : 46178.20,
    82 : 47927.74,
    83 : 45173.39,
    84 : 45733.28,
    85 : 45101.29,
    86 : 46039.84,
    87 : 45407.47,
    88 : 45553.18,
    89 : 43998.86,
    90 : 45250.52,
    91 : 45547.59,
    92 : 48012.97,
    93 : 46665.44,
    94 : 45261.70,
    95 : 45804.45,
    96 : 43334.68,
    97 : 44616.59,
    98 : 46693.97,
    99 : 47609.93,
    100 : 43482.74,
    101 : 44730.29,
    102 : 43768.43,
    103 : 46055.49,
    104 : 38319.73,
    105 : 38908.18,
    106 : 38008.73,
    107 : 38863.93,
    108 : 39169.61,
    109 : 37313.50,
    110 : 40072.82,
    111 : 39301.70,
    }

levels902936ZP = {                              # from 902936 getCameraQE()
    0   : 0.9580,
    1   : 0.9882,
    2   : 0.9517,
    3   : 1.0362,
    4   : 0.9526,
    5   : 1.0160,
    6   : 1.0338,
    7   : 1.0793,
    8   : 0.9330,
    9   : 0.9751,
    10  : 0.9801,
    11  : 1.0519,
    12  : 0.9617,
    13  : 1.0692,
    14  : 0.9635,
    15  : 0.9865,
    16  : 1.1012,
    17  : 1.0227,
    18  : 1.0681,
    19  : 1.0499,
    20  : 1.0227,
    21  : 1.0405,
    22  : 0.9433,
    23  : 0.9810,
    24  : 1.0755,
    25  : 1.0048,
    26  : 1.0628,
    27  : 1.0015,
    28  : 0.9501,
    29  : 0.9924,
    30  : 0.9613,
    31  : 1.0421,
    32  : 1.0591,
    33  : 0.9758,
    34  : 1.0935,
    35  : 0.9775,
    36  : 0.9470,
    37  : 0.9507,
    38  : 0.9815,
    39  : 0.9937,
    40  : 0.9886,
    41  : 1.0505,
    42  : 1.0947,
    43  : 1.0412,
    44  : 1.0075,
    45  : 1.0612,
    46  : 0.9914,
    47  : 1.1059,
    48  : 1.0686,
    49  : 1.0423,
    50  : 1.0090,
    51  : 0.9299,
    52  : 1.0292,
    53  : 0.9410,
    54  : 0.9998,
    55  : 1.0404,
    56  : 1.0799,
    57  : 1.0052,
    58  : 0.9990,
    59  : 0.9894,
    60  : 0.9469,
    61  : 0.9437,
    62  : 0.9516,
    63  : 1.0368,
    64  : 1.0952,
    65  : 1.0557,
    66  : 1.1095,
    67  : 0.9421,
    68  : 1.0002,
    69  : 0.9752,
    70  : 1.0730,
    71  : 1.1096,
    72  : 0.9901,
    73  : 1.0304,
    74  : 1.0939,
    75  : 1.0374,
    76  : 0.9817,
    77  : 0.9765,
    78  : 1.0440,
    79  : 1.0137,
    80  : 1.1484,
    81  : 1.0073,
    82  : 1.0653,
    83  : 0.9808,
    84  : 1.0616,
    85  : 1.0103,
    86  : 1.0373,
    87  : 0.9865,
    88  : 0.9859,
    89  : 0.9422,
    90  : 1.0144,
    91  : 1.0388,
    92  : 1.1266,
    93  : 1.0472,
    94  : 0.9734,
    95  : 0.9847,
    96  : 0.9575,
    97  : 0.9663,
    98  : 1.0625,
    99  : 1.0741,
    100 : 0.8701,
    101 : 0.8738,
    102 : 0.9088,
    103 : 0.9374,
    104 : 0.2665,
    105 : 0.3241,
    106 : 0.6078,
    107 : 0.6145,
    108 : 0.3517,
    109 : 0.7011,
    110 : 0.7279,
    111 : 0.3120,
    }

levels902936ZP_half = {                              # from 902936 getCameraQE()
    0   : 0.9790,
    1   : 0.9941,
    2   : 0.9759,
    3   : 1.0181,
    4   : 0.9763,
    5   : 1.0080,
    6   : 1.0169,
    7   : 1.0396,
    8   : 0.9665,
    9   : 0.9876,
    10  : 0.9901,
    11  : 1.0260,
    12  : 0.9809,
    13  : 1.0346,
    14  : 0.9817,
    15  : 0.9932,
    16  : 1.0506,
    17  : 1.0113,
    18  : 1.0340,
    19  : 1.0249,
    20  : 1.0114,
    21  : 1.0203,
    22  : 0.9717,
    23  : 0.9905,
    24  : 1.0378,
    25  : 1.0024,
    26  : 1.0314,
    27  : 1.0008,
    28  : 0.9751,
    29  : 0.9962,
    30  : 0.9807,
    31  : 1.0211,
    32  : 1.0296,
    33  : 0.9879,
    34  : 1.0467,
    35  : 0.9887,
    36  : 0.9735,
    37  : 0.9753,
    38  : 0.9907,
    39  : 0.9968,
    40  : 0.9943,
    41  : 1.0253,
    42  : 1.0473,
    43  : 1.0206,
    44  : 1.0038,
    45  : 1.0306,
    46  : 0.9957,
    47  : 1.0529,
    48  : 1.0343,
    49  : 1.0211,
    50  : 1.0045,
    51  : 0.9650,
    52  : 1.0146,
    53  : 0.9705,
    54  : 0.9999,
    55  : 1.0202,
    56  : 1.0399,
    57  : 1.0026,
    58  : 0.9995,
    59  : 0.9947,
    60  : 0.9734,
    61  : 0.9718,
    62  : 0.9758,
    63  : 1.0184,
    64  : 1.0476,
    65  : 1.0279,
    66  : 1.0547,
    67  : 0.9711,
    68  : 1.0001,
    69  : 0.9876,
    70  : 1.0365,
    71  : 1.0548,
    72  : 0.9951,
    73  : 1.0152,
    74  : 1.0470,
    75  : 1.0187,
    76  : 0.9909,
    77  : 0.9882,
    78  : 1.0220,
    79  : 1.0068,
    80  : 1.0742,
    81  : 1.0036,
    82  : 1.0326,
    83  : 0.9904,
    84  : 1.0308,
    85  : 1.0051,
    86  : 1.0186,
    87  : 0.9932,
    88  : 0.9929,
    89  : 0.9711,
    90  : 1.0072,
    91  : 1.0194,
    92  : 1.0633,
    93  : 1.0236,
    94  : 0.9867,
    95  : 0.9924,
    96  : 0.9787,
    97  : 0.9832,
    98  : 1.0312,
    99  : 1.0371,
    100 : 0.9350,
    101 : 0.9369,
    102 : 0.9544,
    103 : 0.9687,
    104 : 0.6333,
    105 : 0.6620,
    106 : 0.8039,
    107 : 0.8073,
    108 : 0.6759,
    109 : 0.8505,
    110 : 0.8640,
    111 : 0.6560,
    }

levels903332ZP = {
    0   : 1.0036,
    1   : 1.0230,
    2   : 1.0144,
    3   : 1.0387,
    4   : 0.9718,
    5   : 1.0293,
    6   : 1.0182,
    7   : 1.0531,
    8   : 0.9946,
    9   : 0.9076,
    10  : 1.0284,
    11  : 1.0192,
    12  : 0.9844,
    13  : 1.0347,
    14  : 0.9955,
    15  : 0.9982,
    16  : 1.0542,
    17  : 1.0074,
    18  : 1.0442,
    19  : 1.0420,
    20  : 1.0095,
    21  : 1.0261,
    22  : 0.9908,
    23  : 0.9791,
    24  : 1.0192,
    25  : 1.0135,
    26  : 1.0373,
    27  : 0.9933,
    28  : 0.9700,
    29  : 0.9486,
    30  : 0.9960,
    31  : 0.9995,
    32  : 0.9988,
    33  : 0.9985,
    34  : 1.0527,
    35  : 0.9937,
    36  : 0.9529,
    37  : 0.9415,
    38  : 0.9839,
    39  : 0.9730,
    40  : 0.9759,
    41  : 1.0464,
    42  : 1.0657,
    43  : 0.9925,
    44  : 0.9668,
    45  : 0.9819,
    46  : 0.9851,
    47  : 1.0160,
    48  : 1.0065,
    49  : 1.0424,
    50  : 1.0439,
    51  : 0.9567,
    52  : 0.9574,
    53  : 0.9488,
    54  : 0.9904,
    55  : 0.9935,
    56  : 1.0321,
    57  : 1.0430,
    58  : 1.0246,
    59  : 0.9753,
    60  : 0.9411,
    61  : 0.9612,
    62  : 0.9996,
    63  : 0.9923,
    64  : 1.0254,
    65  : 1.0298,
    66  : 1.0372,
    67  : 0.9535,
    68  : 0.9906,
    69  : 0.9520,
    70  : 1.0269,
    71  : 1.0483,
    72  : 1.0050,
    73  : 1.0210,
    74  : 1.0176,
    75  : 0.9860,
    76  : 0.9627,
    77  : 0.9535,
    78  : 1.0235,
    79  : 1.0205,
    80  : 1.0451,
    81  : 0.9892,
    82  : 0.9936,
    83  : 0.9752,
    84  : 1.0398,
    85  : 1.0177,
    86  : 1.0156,
    87  : 0.9884,
    88  : 0.9805,
    89  : 0.9565,
    90  : 0.9978,
    91  : 1.0440,
    92  : 1.0509,
    93  : 1.0065,
    94  : 1.0004,
    95  : 0.9582,
    96  : 0.9963,
    97  : 1.0142,
    98  : 1.0289,
    99  : 1.0099,
    100 : 1.0000,
    101 : 1.0000,
    102 : 1.0000,
    103 : 1.0000,
    104 : 1.0000,
    105 : 1.0000,
    106 : 1.0000,
    107 : 1.0000,
    108 : 1.0000,
    109 : 1.0000,
    110 : 1.0000,
    111 : 1.0000,
      }

CCDQe_filter = dict(
    g = {                               # 902936ZP
        0   : 0.9941,
        1   : 1.0085,
        2   : 0.9907,
        3   : 1.0180,
        4   : 0.9870,
        5   : 1.0179,
        6   : 1.0230,
        7   : 1.0421,
        8   : 0.9748,
        9   : 1.0018,
        10  : 1.0125,
        11  : 1.0212,
        12  : 0.9840,
        13  : 1.0266,
        14  : 0.9886,
        15  : 1.0000,
        16  : 1.0621,
        17  : 1.0129,
        18  : 1.0162,
        19  : 1.0105,
        20  : 1.0032,
        21  : 1.0149,
        22  : 0.9934,
        23  : 1.0104,
        24  : 1.0330,
        25  : 0.9895,
        26  : 1.0071,
        27  : 0.9856,
        28  : 0.9796,
        29  : 0.9963,
        30  : 1.0060,
        31  : 1.0291,
        32  : 1.0162,
        33  : 0.9705,
        34  : 1.0145,
        35  : 0.9642,
        36  : 0.9635,
        37  : 0.9792,
        38  : 1.0303,
        39  : 1.0134,
        40  : 0.9890,
        41  : 0.9937,
        42  : 1.0228,
        43  : 0.9938,
        44  : 0.9908,
        45  : 1.0215,
        46  : 1.0307,
        47  : 1.0606,
        48  : 1.0210,
        49  : 0.9972,
        50  : 0.9828,
        51  : 0.9402,
        52  : 0.9913,
        53  : 0.9718,
        54  : 1.0287,
        55  : 1.0328,
        56  : 1.0296,
        57  : 0.9820,
        58  : 0.9753,
        59  : 0.9665,
        60  : 0.9560,
        61  : 0.9724,
        62  : 0.9930,
        63  : 1.0289,
        64  : 1.0330,
        65  : 0.9977,
        66  : 1.0211,
        67  : 0.9527,
        68  : 0.9823,
        69  : 0.9916,
        70  : 1.0392,
        71  : 1.0644,
        72  : 0.9903,
        73  : 0.9964,
        74  : 1.0147,
        75  : 0.9917,
        76  : 0.9795,
        77  : 0.9699,
        78  : 1.0475,
        79  : 1.0142,
        80  : 1.0510,
        81  : 0.9843,
        82  : 1.0080,
        83  : 0.9796,
        84  : 1.0563,
        85  : 1.0167,
        86  : 1.0207,
        87  : 0.9832,
        88  : 0.9830,
        89  : 0.9720,
        90  : 1.0196,
        91  : 1.0316,
        92  : 1.0621,
        93  : 1.0162,
        94  : 0.9762,
        95  : 0.9698,
        96  : 1.0061,
        97  : 0.9920,
        98  : 1.0359,
        99  : 1.0323,
        100 : 0.9413,
        101 : 0.9288,
        102 : 0.9579,
        103 : 0.9362,
        104 : 0.5774,
        105 : 1.0000,
        106 : 1.0000,
        107 : 1.0000,
        108 : 1.0000,
        109 : 1.0000,
        110 : 1.0000,
        111 : 1.0000,
    },
        r = {                               # 903332ZP
        0   : 1.0580,
        1   : 1.0616,
        2   : 1.0724,
        3   : 1.0315,
        4   : 1.0218,
        5   : 1.0521,
        6   : 1.0275,
        7   : 1.0390,
        8   : 1.0559,
        9   : 0.9448,
        10  : 1.0724,
        11  : 1.0173,
        12  : 1.0226,
        13  : 1.0170,
        14  : 1.0390,
        15  : 0.9982,
        16  : 1.0312,
        17  : 1.0191,
        18  : 1.0112,
        19  : 1.0243,
        20  : 1.0101,
        21  : 1.0019,
        22  : 1.0457,
        23  : 1.0293,
        24  : 0.9978,
        25  : 1.0155,
        26  : 0.9965,
        27  : 0.9956,
        28  : 1.0098,
        29  : 0.9816,
        30  : 1.0502,
        31  : 1.0125,
        32  : 0.9716,
        33  : 0.9966,
        34  : 0.9799,
        35  : 1.0000,
        36  : 0.9862,
        37  : 0.9944,
        38  : 1.0425,
        39  : 1.0115,
        40  : 0.9757,
        41  : 0.9888,
        42  : 0.9835,
        43  : 0.9492,
        44  : 0.9732,
        45  : 0.9778,
        46  : 1.0400,
        47  : 0.9923,
        48  : 0.9632,
        49  : 0.9803,
        50  : 0.9870,
        51  : 0.9819,
        52  : 0.9359,
        53  : 0.9958,
        54  : 1.0529,
        55  : 1.0057,
        56  : 0.9849,
        57  : 1.0070,
        58  : 0.9930,
        59  : 0.9636,
        60  : 0.9700,
        61  : 0.9945,
        62  : 1.0687,
        63  : 1.0063,
        64  : 0.9827,
        65  : 0.9808,
        66  : 0.9551,
        67  : 0.9804,
        68  : 0.9810,
        69  : 0.9793,
        70  : 1.0050,
        71  : 1.0253,
        72  : 1.0212,
        73  : 1.0104,
        74  : 0.9565,
        75  : 0.9622,
        76  : 0.9751,
        77  : 0.9860,
        78  : 1.0384,
        79  : 1.0414,
        80  : 0.9732,
        81  : 0.9867,
        82  : 0.9551,
        83  : 1.0043,
        84  : 1.0375,
        85  : 1.0443,
        86  : 1.0212,
        87  : 1.0034,
        88  : 0.9931,
        89  : 0.9985,
        90  : 1.0110,
        91  : 1.0534,
        92  : 1.0077,
        93  : 0.9935,
        94  : 1.0103,
        95  : 0.9577,
        96  : 1.0543,
        97  : 1.0401,
        98  : 1.0098,
        99  : 0.9730,
        100 : 0.9844,
        101 : 0.9824,
        102 : 0.9893,
        103 : 0.9388,
        104 : 0.5168,
        105 : 1.0000,
        106 : 1.0000,
        107 : 1.0000,
        108 : 1.0000,
        109 : 1.0000,
        110 : 1.0000,
        111 : 1.0000,
    },
    i = {                               # 902476 ZP with Jacobian, Vignetting, Filter EW applied
        0   : 1.0408,
        1   : 1.0411,
        2   : 1.0574,
        3   : 1.0296,
        4   : 1.0123,
        5   : 1.0267,
        6   : 0.9957,
        7   : 0.9989,
        8   : 1.0376,
        9   : 1.0023,
        10  : 1.0450,
        11  : 0.9749,
        12  : 1.0125,
        13  : 0.9734,
        14  : 1.0086,
        15  : 1.0000,
        16  : 1.0052,
        17  : 0.9955,
        18  : 0.9708,
        19  : 0.9779,
        20  : 0.9758,
        21  : 0.9969,
        22  : 1.0416,
        23  : 1.0316,
        24  : 0.9624,
        25  : 1.0031,
        26  : 0.9617,
        27  : 0.9771,
        28  : 1.0097,
        29  : 1.0186,
        30  : 1.0451,
        31  : 1.0062,
        32  : 0.9650,
        33  : 0.9921,
        34  : 0.9383,
        35  : 0.9924,
        36  : 1.0083,
        37  : 1.0189,
        38  : 1.0290,
        39  : 1.0091,
        40  : 1.0009,
        41  : 0.9713,
        42  : 0.9499,
        43  : 0.9629,
        44  : 0.9785,
        45  : 1.0104,
        46  : 1.0364,
        47  : 0.9785,
        48  : 0.9772,
        49  : 0.9718,
        50  : 0.9834,
        51  : 0.9973,
        52  : 0.9659,
        53  : 1.0476,
        54  : 1.0502,
        55  : 0.9940,
        56  : 0.9777,
        57  : 0.9851,
        58  : 0.9803,
        59  : 0.9912,
        60  : 0.9972,
        61  : 1.0582,
        62  : 1.0527,
        63  : 0.9842,
        64  : 0.9693,
        65  : 0.9635,
        66  : 0.9496,
        67  : 0.9986,
        68  : 1.0153,
        69  : 1.0618,
        70  : 0.9841,
        71  : 0.9908,
        72  : 1.0001,
        73  : 0.9907,
        74  : 0.9560,
        75  : 0.9722,
        76  : 1.0101,
        77  : 1.0358,
        78  : 1.0123,
        79  : 1.0137,
        80  : 0.9496,
        81  : 0.9963,
        82  : 0.9677,
        83  : 1.0194,
        84  : 1.0195,
        85  : 1.0122,
        86  : 1.0134,
        87  : 1.0163,
        88  : 1.0191,
        89  : 1.0630,
        90  : 1.0091,
        91  : 1.0188,
        92  : 0.9964,
        93  : 0.9985,
        94  : 1.0474,
        95  : 1.0231,
        96  : 1.0658,
        97  : 1.0585,
        98  : 1.0381,
        99  : 1.0226,
        100 : 0.9960,
        101 : 0.9572,
        102 : 0.9970,
        103 : 0.9810,
        104 : 0.5889,
        105 : 1.0000,
        106 : 1.0000,
        107 : 1.0000,
        108 : 1.0000,
        109 : 1.0000,
        110 : 1.0000,
        111 : 1.0000,
        },
    )
CCDQe_filter['z'] = CCDQe_filter['i']

CCDQe = CCDQe_filter

levels = None                           # no per-CCD QE
if False:
    alpha = 0.5
    keys = sorted(CCDQe.values()[0].keys())
    levels = dict(zip(keys, [(alpha*levels902936ZP[_] + (1 - alpha)*levels903332ZP[_]) for _ in keys]))

def getQE(serial, filter):
    """Return a CCD's QE in the specified band"""
    filter = filter.lower()

    levels = CCDQe.get(filter)
    if not levels:
        return 1.0

    levs = np.array(levels.values())
    avg = np.median(levs[np.where(levs > 0.5*np.median(levs))])

    return float(levels[serial])/avg

#
# These are the levels at the edges of the CCDs, so edges['r'][69][77] is the level (in r)
# on CCD69 adjacent to CCD77
#
# They were calculated with e.g.
#  for serial in range(112):
#     getLevel(serial, filter='y', calculate=True, visit=904352, butler=butler)
#
edges = {
    'g' : {                             # visit 902986
        0   : {   5 : 44306, 104 : 37610,  },
        1   : { None:None,   6 : 46971,  },
        2   : { None:None,   7 : 46522,  },
        3   : {   8 : 47069, 105 : 39388,  },
        4   : {  10 : 42527, 106 : 23536,  },
        5   : {   0 : 46063,  11 : 49339,  },
        6   : {   1 : 49059,  12 : 52975,  },
        7   : {   2 : 50431,  13 : 54348,  },
        8   : {   3 : 43752,  14 : 47313,  },
        9   : {  15 : 42405, 107 : 22693,  },
        10  : {   4 : 42320,  16 : 45780,  },
        11  : {   5 : 50587,  17 : 54170,  },
        12  : {   6 : 51351,  18 : 55398,  },
        13  : {   7 : 54008,  19 : 58442,  },
        14  : {   8 : 48295,  20 : 51847,  },
        15  : {   9 : 43426,  21 : 47322,  },
        16  : {  10 : 49299,  23 : 51560,  },
        17  : {  11 : 53474,  24 : 56686,  },
        18  : {  12 : 59491,  25 : 62995,  },
        19  : {  13 : 57827,  26 : 62300,  },
        20  : {  14 : 53941,  27 : 57298,  },
        21  : {  15 : 48828,  28 : 51521,  },
        22  : {  30 : 41548, 100 : 30153,  },
        23  : {  16 : 48589,  31 : 50411,  },
        24  : {  17 : 58544,  32 : 61622,  },
        25  : {  18 : 61290,  33 : 64380,  },
        26  : {  19 : 62943,  34 : 67033,  },
        27  : {  20 : 56731,  35 : 59574,  },
        28  : {  21 : 48994,  36 : 51460,  },
        29  : {  37 : 42531, 101 : 27543,  },
        30  : {  22 : 41149,  38 : 42291,  },
        31  : {  23 : 52333,  39 : 53867,  },
        32  : {  24 : 61889,  40 : 63445,  },
        33  : {  25 : 64353,  41 : 67441,  },
        34  : {  26 : 68249,  42 : 71271,  },
        35  : {  27 : 59498,  43 : 61502,  },
        36  : {  28 : 50963,  44 : 52676,  },
        37  : {  29 : 42008,  45 : 43274,  },
        38  : {  30 : 42661,  46 : 43570,  },
        39  : {  31 : 52617,  47 : 53817,  },
        40  : {  32 : 61395,  48 : 62901,  },
        41  : {  33 : 69898,  49 : 72219,  },
        42  : {  34 : 71025,  50 : 73726,  },
        43  : {  35 : 63100,  51 : 64980,  },
        44  : {  36 : 54036,  52 : 55086,  },
        45  : {  37 : 45947,  53 : 46694,  },
        46  : {  38 : 43936,  47 : 48805,  54 : 44313,  },
        47  : {  39 : 56766,  46 : 51909,  48 : 61770,  55 : 56901,  },
        48  : {  40 : 65077,  47 : 60430,  49 : 69089,  56 : 65292,  },
        49  : {  41 : 71414,  48 : 67781,  50 : 72910,  57 : 71100,  },
        50  : {  42 : 69440,  49 : 71575,  51 : 66261,  58 : 70202,  },
        51  : {  43 : 61055,  50 : 65596,  52 : 55796,  59 : 61896,  },
        52  : {  44 : 56233,  51 : 61239,  53 : 51073,  60 : 56767,  },
        53  : {  45 : 43757,  52 : 49086,  61 : 43882,  },
        54  : {  46 : 44570,  62 : 43542,  },
        55  : {  47 : 55081,  63 : 54518,  },
        56  : {  48 : 65775,  64 : 64441,  },
        57  : {  49 : 69397,  65 : 68764,  },
        58  : {  50 : 69568,  66 : 69158,  },
        59  : {  51 : 63647,  67 : 63489,  },
        60  : {  52 : 53776,  68 : 53474,  },
        61  : {  53 : 44297,  69 : 44481,  },
        62  : {  54 : 42502,  70 : 41861,  },
        63  : {  55 : 54454,  71 : 53408,  },
        64  : {  56 : 65220,  72 : 63672,  },
        65  : {  57 : 70834,  73 : 68600,  },
        66  : {  58 : 72978,  74 : 70794,  },
        67  : {  59 : 61150,  75 : 59213,  },
        68  : {  60 : 55588,  76 : 54399,  },
        69  : {  61 : 44530,  77 : 44047,  },
        70  : {  62 : 45403, 102 : 33276,  },
        71  : {  63 : 55619,  78 : 53283,  },
        72  : {  64 : 59808,  79 : 57493,  },
        73  : {  65 : 66951,  80 : 63554,  },
        74  : {  66 : 69311,  81 : 66217,  },
        75  : {  67 : 62708,  82 : 59949,  },
        76  : {  68 : 52581,  83 : 51129,  },
        77  : {  69 : 43869, 103 : 28754,  },
        78  : {  71 : 51723,  84 : 49040,  },
        79  : {  72 : 57783,  85 : 54502,  },
        80  : {  73 : 67179,  86 : 62950,  },
        81  : {  74 : 62291,  87 : 59109,  },
        82  : {  75 : 60623,  88 : 57332,  },
        83  : {  76 : 50033,  89 : 48076,  },
        84  : {  78 : 49152,  90 : 45722,  },
        85  : {  79 : 54400,  91 : 50624,  },
        86  : {  80 : 58942,  92 : 54435,  },
        87  : {  81 : 57439,  93 : 53267,  },
        88  : {  82 : 54020,  94 : 50700,  },
        89  : {  83 : 46592,  95 : 42914,  },
        90  : {  84 : 45310, 108 : 27685,  },
        91  : {  85 : 50862,  96 : 47249,  },
        92  : {  86 : 57047,  97 : 52682,  },
        93  : {  87 : 54924,  98 : 50788,  },
        94  : {  88 : 50228,  99 : 46679,  },
        95  : {  89 : 45097, 109 : 22301,  },
        96  : {  91 : 45264, 110 : 39028,  },
        97  : { None:None,  92 : 48806,  },
        98  : { None:None,  93 : 50235,  },
        99  : {  94 : 49353, 111 : 41895,  },
        100 : { None:None,  22 : 43491,  },
        101 : { None:None,  29 : 44517,  },
        102 : { None:None,  70 : 44688,  },
        103 : { None:None,  77 : 46294,  },
        104 : { None:None,   0 : 34785,  },
        105 : { None:None,   3 : 32317,  },
        106 : { None:None,   4 : 19904,  },
        107 : { None:None,   9 : 19302,  },
        108 : { None:None,  90 : 23450,  },
        109 : { None:None,  95 : 17579,  },
        110 : { None:None,  96 : 38660,  },
        111 : { None:None,  99 : 30150,  },
        },
    'r' : {                             # visit 904744
        0   : {   5 : 33154, 104 : 28495,  },
        1   : { None:None,   6 : 34571,  },
        2   : { None:None,   7 : 35613,  },
        3   : {   8 : 30839, 105 : 26215,  },
        4   : {  10 : 29813, 106 : 17148,  },
        5   : {   0 : 31888,  11 : 34157,  },
        6   : {   1 : 33909,  12 : 36058,  },
        7   : {   2 : 32519,  13 : 34957,  },
        8   : {   3 : 33916,  14 : 36344,  },
        9   : {  15 : 29590, 107 : 16284,  },
        10  : {   4 : 30773,  16 : 33076,  },
        11  : {   5 : 32981,  17 : 34779,  },
        12  : {   6 : 38104,  18 : 40636,  },
        13  : {   7 : 35184,  19 : 37907,  },
        14  : {   8 : 35483,  20 : 37681,  },
        15  : {   9 : 28701,  21 : 31025,  },
        16  : {  10 : 30897,  23 : 32159,  },
        17  : {  11 : 36192,  24 : 38158,  },
        18  : {  12 : 37327,  25 : 39351,  },
        19  : {  13 : 38097,  26 : 40531,  },
        20  : {  14 : 35749,  27 : 37721,  },
        21  : {  15 : 30686,  28 : 32146,  },
        22  : {  30 : 30495, 100 : 22584,  },
        23  : {  16 : 35536,  31 : 36503,  },
        24  : {  17 : 36904,  32 : 38383,  },
        25  : {  18 : 42520,  33 : 44344,  },
        26  : {  19 : 40019,  34 : 41888,  },
        27  : {  20 : 38523,  35 : 39884,  },
        28  : {  21 : 34305,  36 : 35185,  },
        29  : {  37 : 28465, 101 : 18888,  },
        30  : {  22 : 31030,  38 : 31754,  },
        31  : {  23 : 34945,  39 : 35704,  },
        32  : {  24 : 38537,  40 : 39302,  },
        33  : {  25 : 44509,  41 : 46708,  },
        34  : {  26 : 41077,  42 : 43240,  },
        35  : {  27 : 40983,  43 : 41689,  },
        36  : {  28 : 35768,  44 : 36268,  },
        37  : {  29 : 29470,  45 : 29830,  },
        38  : {  30 : 31479,  46 : 32059,  },
        39  : {  31 : 37156,  47 : 37537,  },
        40  : {  32 : 41759,  48 : 42213,  },
        41  : {  33 : 45070,  49 : 46675,  },
        42  : {  34 : 44060,  50 : 45477,  },
        43  : {  35 : 39195,  51 : 39366,  },
        44  : {  36 : 34523,  52 : 34781,  },
        45  : {  37 : 27638,  53 : 27897,  },
        46  : {  38 : 31902,  47 : 34658,  54 : 32019,  },
        47  : {  39 : 34390,  46 : 32598,  48 : 36680,  55 : 34588,  },
        48  : {  40 : 39434,  47 : 37770,  49 : 41847,  56 : 39624,  },
        49  : {  41 : 46675,  48 : 43877,  50 : 48412,  57 : 46666,  },
        50  : {  42 : 48393,  49 : 49645,  51 : 44660,  58 : 47400,  },
        51  : {  43 : 42522,  50 : 44982,  52 : 39920,  59 : 41914,  },
        52  : {  44 : 33379,  51 : 35548,  53 : 31197,  60 : 32995,  },
        53  : {  45 : 30182,  52 : 33186,  61 : 30012,  },
        54  : {  46 : 31900,  62 : 31361,  },
        55  : {  47 : 35786,  63 : 35625,  },
        56  : {  48 : 39941,  64 : 39818,  },
        57  : {  49 : 47807,  65 : 46099,  },
        58  : {  50 : 47366,  66 : 45170,  },
        59  : {  51 : 40068,  67 : 39297,  },
        60  : {  52 : 35492,  68 : 34862,  },
        61  : {  53 : 29689,  69 : 29398,  },
        62  : {  54 : 32211,  70 : 31608,  },
        63  : {  55 : 35640,  71 : 35053,  },
        64  : {  56 : 38369,  72 : 37598,  },
        65  : {  57 : 43496,  73 : 40957,  },
        66  : {  58 : 40988,  74 : 39012,  },
        67  : {  59 : 40560,  75 : 39600,  },
        68  : {  60 : 33377,  76 : 32562,  },
        69  : {  61 : 28685,  77 : 28050,  },
        70  : {  62 : 27566, 102 : 20509,  },
        71  : {  63 : 33835,  78 : 32855,  },
        72  : {  64 : 40906,  79 : 39316,  },
        73  : {  65 : 42176,  80 : 40287,  },
        74  : {  66 : 38723,  81 : 37133,  },
        75  : {  67 : 36426,  82 : 34886,  },
        76  : {  68 : 33229,  83 : 32264,  },
        77  : {  69 : 27796, 103 : 18268,  },
        78  : {  71 : 34312,  84 : 32805,  },
        79  : {  72 : 39129,  85 : 37002,  },
        80  : {  73 : 36409,  86 : 34268,  },
        81  : {  74 : 39621,  87 : 37554,  },
        82  : {  75 : 34067,  88 : 32369,  },
        83  : {  76 : 32281,  89 : 31018,  },
        84  : {  78 : 32193,  90 : 30020,  },
        85  : {  79 : 36722,  91 : 34777,  },
        86  : {  80 : 37856,  92 : 35415,  },
        87  : {  81 : 37642,  93 : 35224,  },
        88  : {  82 : 34274,  94 : 32281,  },
        89  : {  83 : 31260,  95 : 28777,  },
        90  : {  84 : 29217, 108 : 18167,  },
        91  : {  85 : 34056,  96 : 31796,  },
        92  : {  86 : 33089,  97 : 31013,  },
        93  : {  87 : 33385,  98 : 31131,  },
        94  : {  88 : 32427,  99 : 30430,  },
        95  : {  89 : 26462, 109 : 13305,  },
        96  : {  91 : 33394, 110 : 28584,  },
        97  : { None:None,  92 : 33687,  },
        98  : { None:None,  93 : 31438,  },
        99  : {  94 : 27996, 111 : 23605,  },
        100 : { None:None,  22 : 29858,  },
        101 : { None:None,  29 : 27864,  },
        102 : { None:None,  70 : 30208,  },
        103 : { None:None,  77 : 26155,  },
        104 : { None:None,   0 : 29257,  },
        105 : { None:None,   3 : 27998,  },
        106 : { None:None,   4 : 17446,  },
        107 : { None:None,   9 : 16014,  },
        108 : { None:None,  90 : 19351,  },
        109 : { None:None,  95 : 14311,  },
        110 : { None:None,  96 : 29586,  },
        111 : { None:None,  99 : 22521,  },
    },
    'i' : {                             # 902632
        0   : {   5 : 12320, 104 : 10705,  },
        1   : { None:None,   6 : 12548,  },
        2   : { None:None,   7 : 12903,  },
        3   : {   8 : 11174, 105 :  9689,  },
        4   : {  10 : 11176, 106 :  6392,  },
        5   : {   0 : 11782,  11 : 12493,  },
        6   : {   1 : 12256,  12 : 12964,  },
        7   : {   2 : 11673,  13 : 12470,  },
        8   : {   3 : 12403,  14 : 13174,  },
        9   : {  15 : 10945, 107 :  6025,  },
        10  : {   4 : 11574,  16 : 12328,  },
        11  : {   5 : 12043,  17 : 12677,  },
        12  : {   6 : 13774,  18 : 14510,  },
        13  : {   7 : 12553,  19 : 13343,  },
        14  : {   8 : 12822,  20 : 13538,  },
        15  : {   9 : 10602,  21 : 11357,  },
        16  : {  10 : 11416,  23 : 11846,  },
        17  : {  11 : 13212,  24 : 13885,  },
        18  : {  12 : 13165,  25 : 13861,  },
        19  : {  13 : 13409,  26 : 14184,  },
        20  : {  14 : 12766,  27 : 13393,  },
        21  : {  15 : 11209,  28 : 11758,  },
        22  : {  30 : 11359, 100 :  8384,  },
        23  : {  16 : 13200,  31 : 13574,  },
        24  : {  17 : 13380,  32 : 13952,  },
        25  : {  18 : 15088,  33 : 15808,  },
        26  : {  19 : 14002,  34 : 14645,  },
        27  : {  20 : 13720,  35 : 14270,  },
        28  : {  21 : 12634,  36 : 13013,  },
        29  : {  37 : 10783, 101 :  7089,  },
        30  : {  22 : 11584,  38 : 11841,  },
        31  : {  23 : 12942,  39 : 13238,  },
        32  : {  24 : 14030,  40 : 14415,  },
        33  : {  25 : 15856,  41 : 16469,  },
        34  : {  26 : 14339,  42 : 14813,  },
        35  : {  27 : 14717,  43 : 15164,  },
        36  : {  28 : 13297,  44 : 13612,  },
        37  : {  29 : 11201,  45 : 11413,  },
        38  : {  30 : 11734,  46 : 11922,  },
        39  : {  31 : 13860,  47 : 13975,  },
        40  : {  32 : 15435,  48 : 15611,  },
        41  : {  33 : 15805,  49 : 16135,  },
        42  : {  34 : 15060,  50 : 15451,  },
        43  : {  35 : 14204,  51 : 14514,  },
        44  : {  36 : 12916,  52 : 13147,  },
        45  : {  37 : 10514,  53 : 10743,  },
        46  : {  38 : 11885,  47 : 12813,  54 : 11863,  },
        47  : {  39 : 12710,  46 : 12035,  48 : 13445,  55 : 12708,  },
        48  : {  40 : 14488,  47 : 13878,  49 : 15167,  56 : 14482,  },
        49  : {  41 : 16136,  48 : 15835,  50 : 16385,  57 : 16196,  },
        50  : {  42 : 16582,  49 : 16855,  51 : 16271,  58 : 16646,  },
        51  : {  43 : 15880,  50 : 16569,  52 : 15302,  59 : 15965,  },
        52  : {  44 : 12647,  51 : 13401,  53 : 11987,  60 : 12655,  },
        53  : {  45 : 11705,  52 : 12803,  61 : 11727,  },
        54  : {  46 : 11804,  62 : 11598,  },
        55  : {  47 : 13174,  63 : 12999,  },
        56  : {  48 : 14596,  64 : 14372,  },
        57  : {  49 : 16677,  65 : 16361,  },
        58  : {  50 : 16724,  66 : 16483,  },
        59  : {  51 : 15199,  67 : 15066,  },
        60  : {  52 : 13714,  68 : 13590,  },
        61  : {  53 : 11602,  69 : 11536,  },
        62  : {  54 : 11983,  70 : 11705,  },
        63  : {  55 : 13003,  71 : 12750,  },
        64  : {  56 : 13923,  72 : 13606,  },
        65  : {  57 : 15375,  73 : 14862,  },
        66  : {  58 : 14886,  74 : 14424,  },
        67  : {  59 : 15655,  75 : 15330,  },
        68  : {  60 : 12978,  76 : 12718,  },
        69  : {  61 : 11254,  77 : 11076,  },
        70  : {  62 : 10102, 102 :  7473,  },
        71  : {  63 : 12260,  78 : 11889,  },
        72  : {  64 : 14899,  79 : 14283,  },
        73  : {  65 : 15402,  80 : 14762,  },
        74  : {  66 : 14353,  81 : 13786,  },
        75  : {  67 : 13972,  82 : 13402,  },
        76  : {  68 : 13032,  83 : 12681,  },
        77  : {  69 : 10995, 103 :  7252,  },
        78  : {  71 : 12418,  84 : 11942,  },
        79  : {  72 : 14209,  85 : 13486,  },
        80  : {  73 : 13255,  86 : 12511,  },
        81  : {  74 : 14797,  87 : 14085,  },
        82  : {  75 : 13069,  88 : 12483,  },
        83  : {  76 : 12725,  89 : 12300,  },
        84  : {  78 : 11721,  90 : 11066,  },
        85  : {  79 : 13383,  91 : 12748,  },
        86  : {  80 : 13901,  92 : 13146,  },
        87  : {  81 : 14168,  93 : 13354,  },
        88  : {  82 : 13304,  94 : 12598,  },
        89  : {  83 : 12437,  95 : 11668,  },
        90  : {  84 : 10769, 108 :  6708,  },
        91  : {  85 : 12462,  96 : 11747,  },
        92  : {  86 : 12219,  97 : 11533,  },
        93  : {  87 : 12605,  98 : 11863,  },
        94  : {  88 : 12676,  99 : 12024,  },
        95  : {  89 : 10655, 109 :  5388,  },
        96  : {  91 : 12435, 110 : 10880,  },
        97  : { None:None,  92 : 12639,  },
        98  : { None:None,  93 : 12007,  },
        99  : {  94 : 11001, 111 :  9428,  },
        100 : { None:None,  22 : 11111,  },
        101 : { None:None,  29 : 10398,  },
        102 : { None:None,  70 : 11093,  },
        103 : { None:None,  77 : 10368,  },
        104 : { None:None,   0 : 10890,  },
        105 : { None:None,   3 : 10284,  },
        106 : { None:None,   4 :  6502,  },
        107 : { None:None,   9 :  5919,  },
        108 : { None:None,  90 :  7188,  },
        109 : { None:None,  95 :  5848,  },
        110 : { None:None,  96 : 11225,  },
        111 : { None:None,  99 :  9114,  },
        },
        'y' : {                         # 904478
        0   : {   5 : 30247, 104 : 27014,  },
        1   : { None:None,   6 : 30060,  },
        2   : { None:None,   7 : 30560,  },
        3   : {   8 : 26110, 105 : 23277,  },
        4   : {  10 : 28468, 106 : 16789,  },
        5   : {   0 : 28949,  11 : 30490,  },
        6   : {   1 : 29439,  12 : 30793,  },
        7   : {   2 : 27719,  13 : 29343,  },
        8   : {   3 : 28707,  14 : 30006,  },
        9   : {  15 : 25237, 107 : 14211,  },
        10  : {   4 : 29156,  16 : 30625,  },
        11  : {   5 : 29431,  17 : 30851,  },
        12  : {   6 : 32789,  18 : 34639,  },
        13  : {   7 : 29340,  19 : 31165,  },
        14  : {   8 : 29069,  20 : 30431,  },
        15  : {   9 : 24170,  21 : 25494,  },
        16  : {  10 : 28194,  23 : 29120,  },
        17  : {  11 : 32296,  24 : 33950,  },
        18  : {  12 : 31462,  25 : 33310,  },
        19  : {  13 : 31575,  26 : 33366,  },
        20  : {  14 : 29027,  27 : 30627,  },
        21  : {  15 : 25358,  28 : 26186,  },
        22  : {  30 : 28641, 100 : 21564,  },
        23  : {  16 : 32168,  31 : 32842,  },
        24  : {  17 : 32541,  32 : 33988,  },
        25  : {  18 : 36201,  33 : 37892,  },
        26  : {  19 : 33085,  34 : 34609,  },
        27  : {  20 : 31167,  35 : 32404,  },
        28  : {  21 : 28273,  36 : 28870,  },
        29  : {  37 : 23315, 101 : 15688,  },
        30  : {  22 : 28810,  38 : 29144,  },
        31  : {  23 : 31651,  39 : 32065,  },
        32  : {  24 : 34535,  40 : 35359,  },
        33  : {  25 : 37836,  41 : 39126,  },
        34  : {  26 : 33911,  42 : 35163,  },
        35  : {  27 : 33026,  43 : 33993,  },
        36  : {  28 : 28859,  44 : 29235,  },
        37  : {  29 : 24205,  45 : 24310,  },
        38  : {  30 : 28761,  46 : 28710,  },
        39  : {  31 : 33775,  47 : 33795,  },
        40  : {  32 : 37555,  48 : 37571,  },
        41  : {  33 : 38256,  49 : 38835,  },
        42  : {  34 : 35504,  50 : 36347,  },
        43  : {  35 : 32312,  51 : 32684,  },
        44  : {  36 : 27938,  52 : 27974,  },
        45  : {  37 : 22638,  53 : 22981,  },
        46  : {  38 : 29165,  47 : 30856,  54 : 28934,  },
        47  : {  39 : 30531,  46 : 28844,  48 : 32302,  55 : 30357,  },
        48  : {  40 : 34772,  47 : 32952,  49 : 36355,  56 : 34480,  },
        49  : {  41 : 38457,  48 : 37802,  50 : 38956,  57 : 38482,  },
        50  : {  42 : 38828,  49 : 39955,  51 : 37117,  58 : 38730,  },
        51  : {  43 : 35035,  50 : 37561,  52 : 32588,  59 : 34848,  },
        52  : {  44 : 27077,  51 : 29219,  53 : 25086,  60 : 26915,  },
        53  : {  45 : 24787,  52 : 26624,  61 : 24637,  },
        54  : {  46 : 28759,  62 : 28372,  },
        55  : {  47 : 31085,  63 : 30306,  },
        56  : {  48 : 34455,  64 : 33565,  },
        57  : {  49 : 39270,  65 : 38056,  },
        58  : {  50 : 38733,  66 : 37494,  },
        59  : {  51 : 32996,  67 : 32265,  },
        60  : {  52 : 28929,  68 : 28340,  },
        61  : {  53 : 24107,  69 : 23473,  },
        62  : {  54 : 29010,  70 : 28190,  },
        63  : {  55 : 30898,  71 : 29948,  },
        64  : {  56 : 32916,  72 : 31714,  },
        65  : {  57 : 36255,  73 : 34238,  },
        66  : {  58 : 34156,  74 : 32245,  },
        67  : {  59 : 33531,  75 : 32184,  },
        68  : {  60 : 26851,  76 : 26098,  },
        69  : {  61 : 23482,  77 : 22834,  },
        70  : {  62 : 24382, 102 : 18071,  },
        71  : {  63 : 28672,  78 : 27582,  },
        72  : {  64 : 33921,  79 : 31905,  },
        73  : {  65 : 34950,  80 : 32824,  },
        74  : {  66 : 31662,  81 : 29687,  },
        75  : {  67 : 29429,  82 : 27666,  },
        76  : {  68 : 26424,  83 : 25325,  },
        77  : {  69 : 22226, 103 : 14430,  },
        78  : {  71 : 28498,  84 : 27151,  },
        79  : {  72 : 32657,  85 : 30472,  },
        80  : {  73 : 29433,  86 : 27108,  },
        81  : {  74 : 31475,  87 : 29280,  },
        82  : {  75 : 27038,  88 : 25424,  },
        83  : {  76 : 25607,  89 : 24477,  },
        84  : {  78 : 26612,  90 : 24968,  },
        85  : {  79 : 29779,  91 : 28130,  },
        86  : {  80 : 30057,  92 : 27931,  },
        87  : {  81 : 29363,  93 : 27291,  },
        88  : {  82 : 27206,  94 : 25398,  },
        89  : {  83 : 24701,  95 : 22856,  },
        90  : {  84 : 24327, 108 : 15231,  },
        91  : {  85 : 27197,  96 : 25492,  },
        92  : {  86 : 25918,  97 : 24180,  },
        93  : {  87 : 25936,  98 : 24014,  },
        94  : {  88 : 25538,  99 : 24071,  },
        95  : {  89 : 21226, 109 : 10376,  },
        96  : {  91 : 26875, 110 : 23465,  },
        97  : { None:None,  92 : 26132,  },
        98  : { None:None,  93 : 24468,  },
        99  : {  94 : 21914, 111 : 18536,  },
        100 : { None:None,  22 : 27800,  },
        101 : { None:None,  29 : 23192,  },
        102 : { None:None,  70 : 26588,  },
        103 : { None:None,  77 : 21075,  },
        104 : { None:None,   0 : 28094,  },
        105 : { None:None,   3 : 24827,  },
        106 : { None:None,   4 : 17005,  },
        107 : { None:None,   9 : 13920,  },
        108 : { None:None,  90 : 16305,  },
        109 : { None:None,  95 : 11135,  },
        110 : { None:None,  96 : 24503,  },
        111 : { None:None,  99 : 17488,  },
        },
          'nb0921' : {                   # from 904746
        0   : {   5 : 47991, 104 : 42753,  },
        1   : { None:None,   6 : 47889,  },
        2   : { None:None,   7 : 48250,  },
        3   : {   8 : 41839, 105 : 36873,  },
        4   : {  10 : 44256, 106 : 26506,  },
        5   : {   0 : 45645,  11 : 47935,  },
        6   : {   1 : 46472,  12 : 48791,  },
        7   : {   2 : 43813,  13 : 46487,  },
        8   : {   3 : 46258,  14 : 48253,  },
        9   : {  15 : 40079, 107 : 22808,  },
        10  : {   4 : 45601,  16 : 47959,  },
        11  : {   5 : 46490,  17 : 49006,  },
        12  : {   6 : 52108,  18 : 56288,  },
        13  : {   7 : 46614,  19 : 50677,  },
        14  : {   8 : 46861,  20 : 49378,  },
        15  : {   9 : 38751,  21 : 41015,  },
        16  : {  10 : 44519,  23 : 45956,  },
        17  : {  11 : 51029,  24 : 54818,  },
        18  : {  12 : 51689,  25 : 55921,  },
        19  : {  13 : 51256,  26 : 55822,  },
        20  : {  14 : 46600,  27 : 49815,  },
        21  : {  15 : 40438,  28 : 41736,  },
        22  : {  30 : 44579, 100 : 33958,  },
        23  : {  16 : 50938,  31 : 52148,  },
        24  : {  17 : 53138,  32 : 56398,  },
        25  : {  18 : 60472,  33 : 63924,  },
        26  : {  19 : 55218,  34 : 58249,  },
        27  : {  20 : 51134,  35 : 53950,  },
        28  : {  21 : 44568,  36 : 45467,  },
        29  : {  37 : 37250, 101 : 25363,  },
        30  : {  22 : 45438,  38 : 46130,  },
        31  : {  23 : 49782,  39 : 50791,  },
        32  : {  24 : 56527,  40 : 58307,  },
        33  : {  25 : 64401,  41 : 66741,  },
        34  : {  26 : 57233,  42 : 59439,  },
        35  : {  27 : 55301,  43 : 57261,  },
        36  : {  28 : 46176,  44 : 46948,  },
        37  : {  29 : 38758,  45 : 38997,  },
        38  : {  30 : 45655,  46 : 45597,  },
        39  : {  31 : 53227,  47 : 53403,  },
        40  : {  32 : 62492,  48 : 62961,  },
        41  : {  33 : 64261,  49 : 65570,  },
        42  : {  34 : 60212,  50 : 61712,  },
        43  : {  35 : 54065,  51 : 54806,  },
        44  : {  36 : 44881,  52 : 45212,  },
        45  : {  37 : 35981,  53 : 36603,  },
        46  : {  38 : 45236,  47 : 47884,  54 : 44913,  },
        47  : {  39 : 48553,  46 : 45143,  48 : 52838,  55 : 48236,  },
        48  : {  40 : 58583,  47 : 54940,  49 : 61582,  56 : 58042,  },
        49  : {  41 : 65420,  48 : 63998,  50 : 66544,  57 : 65288,  },
        50  : {  42 : 66270,  49 : 68388,  51 : 63409,  58 : 65955,  },
        51  : {  43 : 59629,  50 : 64181,  52 : 54761,  59 : 59149,  },
        52  : {  44 : 43322,  51 : 47561,  53 : 39830,  60 : 42964,  },
        53  : {  45 : 39746,  52 : 42603,  61 : 39529,  },
        54  : {  46 : 44708,  62 : 43968,  },
        55  : {  47 : 49944,  63 : 48638,  },
        56  : {  48 : 58210,  64 : 56393,  },
        57  : {  49 : 66814,  65 : 64395,  },
        58  : {  50 : 66058,  66 : 63838,  },
        59  : {  51 : 56086,  67 : 54641,  },
        60  : {  52 : 46571,  68 : 45541,  },
        61  : {  53 : 38990,  69 : 38151,  },
        62  : {  54 : 45481,  70 : 44059,  },
        63  : {  55 : 48521,  71 : 46860,  },
        64  : {  56 : 54566,  72 : 51924,  },
        65  : {  57 : 60779,  73 : 57181,  },
        66  : {  58 : 57761,  74 : 54509,  },
        67  : {  59 : 56726,  75 : 54157,  },
        68  : {  60 : 43232,  76 : 41851,  },
        69  : {  61 : 37282,  77 : 36316,  },
        70  : {  62 : 38226, 102 : 28521,  },
        71  : {  63 : 45132,  78 : 43238,  },
        72  : {  64 : 56396,  79 : 52175,  },
        73  : {  65 : 58550,  80 : 54381,  },
        74  : {  66 : 53979,  81 : 49977,  },
        75  : {  67 : 49318,  82 : 45717,  },
        76  : {  68 : 42865,  83 : 41130,  },
        77  : {  69 : 35935, 103 : 23584,  },
        78  : {  71 : 44979,  84 : 42672,  },
        79  : {  72 : 51845,  85 : 47427,  },
        80  : {  73 : 49062,  86 : 44111,  },
        81  : {  74 : 53177,  87 : 48212,  },
        82  : {  75 : 44498,  88 : 41152,  },
        83  : {  76 : 41361,  89 : 39647,  },
        84  : {  78 : 42108,  90 : 39438,  },
        85  : {  79 : 46685,  91 : 43677,  },
        86  : {  80 : 48571,  92 : 44216,  },
        87  : {  81 : 48060,  93 : 43847,  },
        88  : {  82 : 43627,  94 : 40587,  },
        89  : {  83 : 40099,  95 : 37153,  },
        90  : {  84 : 38331, 108 : 23984,  },
        91  : {  85 : 42755,  96 : 40133,  },
        92  : {  86 : 41005,  97 : 38167,  },
        93  : {  87 : 41594,  98 : 38496,  },
        94  : {  88 : 40788,  99 : 38674,  },
        95  : {  89 : 33980, 109 : 16992,  },
        96  : {  91 : 42407, 110 : 36862,  },
        97  : { None:None,  92 : 41636,  },
        98  : { None:None,  93 : 38866,  },
        99  : {  94 : 35418, 111 : 29854,  },
        100 : { None:None,  22 : 44206,  },
        101 : { None:None,  29 : 37131,  },
        102 : { None:None,  70 : 41433,  },
        103 : { None:None,  77 : 33842,  },
        104 : { None:None,   0 : 44094,  },
        105 : { None:None,   3 : 38960,  },
        106 : { None:None,   4 : 27178,  },
        107 : { None:None,   9 : 22524,  },
        108 : { None:None,  90 : 25627,  },
        109 : { None:None,  95 : 18414,  },
        110 : { None:None,  96 : 38140,  },
        111 : { None:None,  99 : 28364,  },
        },
          }

def getLevel(serial, filter='g', fiddle=False, calculate=False, visit=0, butler=None, canonical=None):
    levels = levels902986

    if not levels:
        avg = 1.0
    else:
        if canonical is None:
            levs = np.array(levels.values())
            avg = np.median(levs[np.where(levs > 0.5*np.median(levs))])
        else:
            avg = levels[canonical]

    filter = filter.lower()

    lev = levels[serial] if levels else avg
    correction = avg/float(lev)

    if calculate:
        assert butler

        cam = butler.get("camera")

        raw = butler.get("raw", visit=visit, ccd=serial)
        ccd = raw.getDetector()
        raw = afwCGUtils.trimRawCallback(raw, convertToFloat=True, correctGain=True)
        raw = raw.getImage()
        raw *= correction
        w, h = raw.getWidth(), raw.getHeight()

        if False:
            ds9.mtv(raw)
            ds9.erase()

        cenMM0 = afwCGUtils.findCcd(cam, serial).getPositionFromPixel(afwGeom.PointD(0.5*w, 0.5*h)).getMm()

        if not edges.has_key(filter):
            edges[filter] = edges['r'].copy()

        neighbors = sorted(edges[filter][serial].keys())

        print "        %-3d : {" % serial,
        for serial1 in neighbors:
            if serial1 is None:
                print "None:None,",
                continue

            ccd = afwCGUtils.findCcd(cam, serial1)
            nQuarter = ccd.getOrientation().getNQuarter()
            w1, h1 = ccd.getAllPixels().getDimensions()

            cenMM = afwCGUtils.findCcd(cam,
                                       serial1).getPositionFromPixel(afwGeom.PointD(0.5*w1, 0.5*h1)).getMm()
            offset = cenMM - cenMM0
            offset = [int(_ + 0.5) if _ > 0 else int(_ - 0.5) for _ in [offset[0]/((w + w1)/2),
                                                                        offset[1]/((h + h1)/2)]]

            delta = 100
            if offset[0] == -1:
                assert offset[1] == 0
                im = raw[30:30+delta, 100:-100]
            elif offset[0] == 0:
                if offset[1] == -1:
                    im = raw[100:-100, 30:30+delta]
                else:
                    im = raw[100:-100, -(30+delta):-30]
            else:
                if offset[1] != 0:
                    print serial, serial1, offset
                assert offset[1] == 0
                im = raw[-(30+delta):-30, 100:-100]

            if True:
                val = afwMath.makeStatistics(im, afwMath.MEANCLIP).getValue()
            else:
                try:
                    val = np.median(im.getImage().getArray())
                except Exception as e:
                    import pdb; pdb.set_trace()
                    pass

            if False:
                ds9Utils.drawBBox(im.getBBox(), borderWidth=0.4)
                print serial, serial1, offset, val

            print "%3d : %5d," % (serial1, val),
        print " },"

        return
    #
    # Apply correction factors
    #
    if fiddle and edges.has_key(filter):
        canonical = 50
        scale = findCanonical(serial, edges[filter], canonical=canonical, verbose=False)
        if scale:
            correction /= scale

    return correction

def findCanonical(serial, edges, seen=None, scale=1.0, canonical=50, verbose=False):
    """Find a path to the "canonical" CCD via the pairs of adjacent CCD edges defined in edges[]"""
    if seen == None:
        seen = []

    if serial == canonical:
        return 1.0

    for s, level in edges.get(serial, {}).items():
        if s in seen:
            continue

        seen.append(serial)
        if s == canonical:
            if verbose:
                print canonical, serial, scale, edges[serial][canonical], edges[canonical][serial]
            return float(edges[serial][canonical])/float(edges[canonical][serial])

        x = findCanonical(s, edges, seen, scale=scale, canonical=canonical, verbose=verbose)
        if x is not None:
            if verbose:
                print s, serial, scale, edges[s][serial], edges[serial][s], \
                    x, float(edges[serial][s])/float(edges[s][serial]), \
                    x*float(edges[serial][s])/float(edges[s][serial])
            return x*float(edges[serial][s])/float(edges[s][serial])

    return None

def getCameraQE(butler, visit, fd=None):
    """Return an estimate of the chips' QE from a visit's zero point"""
    ccdIds = butler.queryMetadata('raw', 'visit', ['ccd'], visit=visit)

    qe = np.empty(len(ccdIds))
    for i, ccd in enumerate(ccdIds):
        try:
            calexp_md = butler.get("calexp_md", visit=visit, ccd=ccd, immediate=True)
        except:
            qe[i] = np.nan
            continue

        calib = afwImage.Calib(calexp_md)
        qe[i] = calib.getMagnitude(1.0)

    good = np.isfinite(qe)
    qe = qe - np.median(qe[good])
    qe[np.logical_not(good)] = 0.0

    qe = 10**(0.4*qe)

    if fd:
        print >> fd, "levels%dZP = {" % visit
        for i, ccd in enumerate(ccdIds):
            print >> fd, "    %-3d : %.4f," % (ccd, qe[i])
        print >> fd, "}"

    return dict(zip(ccdIds, qe))


#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def printLine(line, LaTeX, underline=False):
    if LaTeX:
        line = " & ".join(line) + r" \\"
    else:
        line = " ".join(line)

    print line
    if underline:
        if LaTeX:
            print r"\hline"
        else:
            print "-"*len(line)

    return []

def writeQETable(butler, filters="ugrizy", LaTeX=False, nCol=3, canonical=50):
    """Write a table of relative QEs, based on XXX and edges dicts"""
    _filters = filters
    filters = []
    for f in _filters:
        if f in ['g'] + edges.keys():
            filters.append(f)

    ccds = sorted(butler.queryMetadata("raw", "visit", ["ccd"]))
    QE = {}
    for ccd in ccds:
        QE[ccd] = {}
        for f in filters:
            try:
                QE[ccd][f] = getQE(ccd, f)
            except KeyError:
                break
        if len(QE[ccd]) == 0:
            del QE[ccd]

    if LaTeX:
        llll = "l"*(1 + len(filters))
        print r"\begin{tabular}{*%d{%s@{\qquad}}%s}" % (nCol - 1, llll, llll)

    line = []
    for i in range(nCol):
        line.append("%3s " % ("CCD"))
        for f in filters:
            line.append("%4s" % ("NB921" if f == "NB0921" else f))

    line = printLine(line, LaTeX, underline=True)

    for i, ccd in enumerate(ccds):
        if not QE.has_key(ccd):
            continue

        line.append("%3d " % (ccd))
        for f in filters:
            line.append("%4.2f" % (QE[ccd][f]))
        if (i + 1)%nCol == 0:
            line = printLine(line, LaTeX)

    if line:
        line = printLine(line, LaTeX)

    if LaTeX:
        print r"\end{tabular}"

def writeObsTable(butler, mos, LaTeX=False):
    visits = sorted(mos.keys())
    comments = {
        902634 : "Twilight flat 15000 e$^-$",
        902688 : "Dome flat 62000 e$^-$",
        902982 : "Twilight flat XXX DN",
        902984 : "Twilight flat 13000 ADU",
        902986 : "Twilight flat 65000 e$^-$",
        903036 : "Dome flat; 4x10W 6V 6.33A 32000 e$^-$",
        903420 : "Twilight flat 32000 e$^-$",
        903432 : "Twilight flat 15000 e$^-$",
        903440 : "Dome flat; 4x10W 6V 4.33A",
        903452 : "Dome flat; 4x10W 3V 3.19A",
        904520 : "Dark dome EL=90",
        904534 : "Dark dome EL=90",
        904536 : "Dark dome El=90; vent shutters open",
        904538 : "Dark dome EL=90; vent shutters closes; light leaks sealed",
        904670 : "Dark dome EL=90",
        904672 : "Dark dome EL=60",
        904674 : "Dark dome EL=30",
        904676 : "Dark dome EL=15",
        904678 : "Dark dome EL=45",
        904786 : "Dark dome El=90; Top screen at rear",
        904788 : "Dark dome El=90; Usual screen position",
        904790 : "Dark dome El=90; cal off",
        904792 : "Dark dome El=90; cal off; tertiary cover closed",
        904794 : "Dark dome El=90; cal off; tertiary cover closed; mirror cover power off",
        905420 : "Dome flat",
        905428 : "Dome flat",
        }

    summary = {}
    for v in visits:
        md = butler.get("raw_md", visit=v, ccd=10)

        if False:
            filter = afwImage.Filter(md).getName() # why does this fail?
        else:
            filter = md.get("FILTER01")[-1:].lower()
        if filter == "z" and v in range(904672, 905068+1, 2): # z wasn't available
            filter = "i"                                      # error in header

        summary[v] = dict(filter=filter,
                          exptime="%.0f" % md.get("EXPTIME"),
                          date = md.get("DATE-OBS"),
                          start = md.get("UT")[:8],
                          )
        if not comments.has_key(v):
            comments[v] = md.get("OBJECT")

    fields = ["visit"]
    for k in ["date", "start", "exptime", "filter"]:
        if summary.values()[0].has_key(k):
            fields.append(k)
    fields.append("comment");

    if LaTeX:
        print r"\begin{longtable}{*{%d}{l}}" % (len(fields))

    line = []
    for k in fields:
        line.append(k.capitalize())
    line = printLine(line, LaTeX, underline=True)

    for v in visits:
        line = [str(v)]
        for k in fields:
            if k not in ("visit", "comment"):
                line.append(summary[v][k])

        comment = re.sub(r"([[_&#]])", r"\\\1", comments.get(v, ""))
        line.append(comment)

        line = printLine(line, LaTeX)

    if LaTeX:
        print r"\end{longtable}"

def stackMosaics(mos, visits):
    im = mos[visits[0]]

    sctrl = afwMath.StatisticsControl()
    #sctrl.setAndMask(BAD)

    imgList = afwImage.vectorMaskedImageF() if hasattr(im, "getImage") else afwImage.vectorImageF()
    for v in visits:
        im = mos[v].clone()
        if False:
            im /= afwMath.makeStatistics(im, afwMath.MEANCLIP, sctrl).getValue()
        else:
            im /= np.median(im.getArray())
        imgList.push_back(im)

    out = afwMath.statisticsStack(imgList, afwMath.MEANCLIP, sctrl)
    out.setXY0(im.getXY0())

    return out

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def getVignetting(X, Y):
    """Return the vignetting at the points (X, Y) (measured in focal plane mm) """
    vig = np.array([1.00, 1.00,  0.74, 0.03]) # vignetting
    xz =  np.array([0,    0.115, 0.76, 0.90]) # distance from boresight, deg
    xz /= 0.16865/3600                        # convert to pixels

    r = np.hypot(X, Y)
    vignet = np.where(r < xz[1], vig[0], vig[1] + (r - xz[1])*(vig[2] - vig[1])/(xz[2] - xz[1]))

    vshape = vignet.shape
    if len(vshape) > 1:
        vim = afwImage.ImageF(vignet.shape[1], vignet.shape[0])
    else:
        vim = afwImage.ImageF(vignet.shape[0], 1)

    vim.getArray()[:] = vignet

    return vim

def getPixelArea(camera, X, Y):
    """Return the pixel area for pixels at (X, Y) (measured in focal plane mm)"""

    r = np.hypot(X, Y)

    dist = camera.getDistortion()
    if len(r) == 1:
        aim = afwImage.ImageF(1, 1)
        aim[0,0] = dist.computeQuadrupoleTransform(afwGeom.PointD(0, r[0]), False).computeDeterminant()

        return aim

    br = np.linspace(0, np.max(r), 100) # enough points to allow linear interpolation in numpy
    det = np.empty_like(br)
    for i in range(len(br)):
        det[i] = dist.computeQuadrupoleTransform(afwGeom.PointD(0, br[i]), False).computeDeterminant()

    if False:
        pyplot.clf()
        pyplot.plot(br, det)
        pyplot.show()

    area = np.reshape(np.interp(r.flatten(), br, det), X.shape) # better interpolation's in scipy

    ashape = area.shape
    aim = afwImage.ImageF(ashape[1], ashape[0])

    aim.getArray()[:] = area

    return aim

def correctVignettingCallback(im, ccd=None, butler=None, imageSource=None, filter=None,
                              correctQE=True, medianFilter=False, nx=11, ny=None):
    """A callback function that subtracts the bias and trims a raw image and then
corrects for vignetting/Jacobian"""

    if not ccd:
        ccd = im.getDetector()
    ccdSerial = ccd.getId().getSerial()

    isr = isrCallbackQE if correctQE else isrCallback
    im = isr(im, ccd, butler, imageSource=imageSource, doFlatten=False, correctGain=True)

    parent = ccd.getParent()
    while parent:
        camera = parent
        parent = parent.getParent()

    width, height = im.getDimensions()

    fxy = []
    if False:                           # too slow and memory wasting
        for xy in [(0,         0),
                   (width - 1, height - 1),
                   ]:
            fxy.append(ccd.getPositionFromPixel(afwGeom.PointD(*xy)).getPixels(1.0))

        X, Y = np.meshgrid(fxy[0][0] + (fxy[1][0] - fxy[0][0])/width* np.arange(width),
                           fxy[0][1] + (fxy[1][1] - fxy[0][1])/height*np.arange(height))
        im /= getVignetting(X, Y)
        im /= getPixelArea(camera, X, Y)
    else:
        vim = afwImage.ImageF(2,2)
        for iy, y in enumerate([0.25, 0.75]):
            for ix, x in enumerate([0.25, 0.75]):
                fx, fy = ccd.getPositionFromPixel(afwGeom.PointD(x*width, y*height)).getPixels(1.0)

                vim[ix, iy]  = getVignetting([fx], [fy])
                vim[ix, iy] *= getPixelArea(camera, [fx], [fy])

        backobj = afwMath.BackgroundMI(im.getBBox(), afwImage.makeMaskedImage(vim))
        im[:] /= backobj.getImageF("LINEAR")
    #
    # Estimate useful signal level even for very strongly vignetted chips
    #
    if imageSource and imageSource.verbose:
        print "Correcting vignetting for %s %3d %.2f" %(ccd.getId().getName(), ccdSerial, median)

    if medianFilter:
        im = medianFilterImage(im, nx, ny)

    return im

def makeVignettingImage(im, modelJacobian=True, modelQE=True, filter=None):
    return makeFlatImage(im, filter=filter,
                         modelVignetting=True, modelJacobian=modelJacobian, modelQE=modelQE)

def makeEvenSplineInterpolator(x, y):
    npt = len(x)

    vecs = [x, y]
    for i, v in enumerate(vecs):
        v_o = np.empty(2*npt - 1)
        vecs[i] = v_o

        v_o[0:npt] = v[::-1]
        if i == 0:                      # radius
            v_o[0:npt] *= -1
        v_o[npt:] =   v[1:]

    return scipy.interpolate.interp1d(vecs[0], vecs[1], kind='cubic')

def makeFlatImage(im, filter=None, modelVignetting=True, modelJacobian=True, modelQE=True,
                  filterThroughput=None):
    ccd = im.getDetector()

    width, height = im.getDimensions()

    fxy = []
    for xy in [(0,         0),
               (width - 1, height - 1), 
               ]:
        fxy.append(ccd.getPositionFromPixel(afwGeom.PointD(*xy)).getPixels(1.0))

    X, Y = np.meshgrid(fxy[0][0] + (fxy[1][0] - fxy[0][0])/width* np.arange(width),
                       fxy[0][1] + (fxy[1][1] - fxy[0][1])/height*np.arange(height))

    im.getMaskedImage().getImage()[:] -= im.getMaskedImage().getImage()[:] # 0 or NaN
    im.getMaskedImage().getImage()[:] += getVignetting(X, Y) if modelVignetting else 1

    if filterThroughput:
        filterR, filterThroughput = filterThroughput
        filterThroughput = filterThroughput/filterThroughput[0]
        filterR = filterR/15e-3         # convert to pixels
        #
        # Prepare a spline fit to filterThroughput, forced to be an even function
        # and extended in radius by a factor of 2
        #
        if scipy:
            filterR = list(filterR)
            filterR.append(2*filterR[-1])
            filterR=np.array(filterR)

            filterThroughput = list(filterThroughput)
            filterThroughput.append(filterThroughput[-1])
            filterThroughput = np.array(filterThroughput)

            interpolator = makeEvenSplineInterpolator(filterR, filterThroughput)

        fim = np.empty((height, width)) # n.b. numpy index order
        y0 = 0
        for frac in (0.5, 1.0):
            y1 = int(frac*height)
            y = np.array([y0, y1 - 1])

            fY = np.arange(y1 - y0)/float(y1 - y0 - 1)
            for x in range(width):
                r = [np.hypot(*ccd.getPositionFromPixel(afwGeom.PointD(x, _y)).getPixels(1.0)) for _y in y]
                throughput = interpolator(r) if scipy else \
                    np.interp(r, filterR, filterThroughput) # linear interpolation

                fim[y0:y1, x] = throughput[0] + fY*(throughput[1] - throughput[0])

            y0 = y1

        im.getMaskedImage().getImage().getArray()[:] *= fim

    if modelJacobian:
        dist = ccd.getDistortion()

        jim = np.empty((height, width)) # n.b. numpy index order
        jY = np.arange(height)/float(height - 1)

        jacobian = [0, 0]
        for x in range(width):
            for i, y in enumerate((0, height - 1)):
                jacobian[i] = dist.computeQuadrupoleTransform(
                    ccd.getPositionFromPixel(afwGeom.PointD(x, y)).getPixels(1.0), False).computeDeterminant()

            jim[:, x] = jacobian[0] + jY*(jacobian[1] - jacobian[0])

        im.getMaskedImage().getImage().getArray()[:] *= jim

    if modelQE:
        im.getMaskedImage()[:] *= getQE(ccd.getId().getSerial(), filter)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

filterData = dict(

    g =    dict(radius      = np.array([    0.00,    50.00,   100.00,   150.00,   200.00,   250.00,   270.00]), # mm
                EW          = np.array([  143.35,   143.00,   142.90,   142.73,   142.57,   142.33,   142.22]),
                lambda_bar  = np.array([  473.07,   473.04,   472.94,   472.78,   472.56,   472.10,   471.99]),
                peak        = np.array([   98.18,    97.95,    98.17,    98.09,    98.05,    98.00,    97.95]),
                on50        = np.array([  399.65,   399.65,   399.63,   399.56,   399.40,   399.04,   398.96]),
                off50       = np.array([  546.79,   546.76,   546.57,   546.34,   546.05,   545.51,   545.38]),
                on10        = np.array([  397.02,   397.03,   397.03,   396.97,   396.76,   396.39,   396.30]),
                off10       = np.array([  550.84,   550.81,   550.61,   550.30,   549.92,   549.29,   549.08]),
                on80        = np.array([  401.02,   401.01,   400.98,   400.92,   400.79,   400.47,   400.38]),
                off80       = np.array([  544.43,   544.42,   544.29,   544.15,   543.92,   543.42,   543.30]),
                Tmin        = np.array([   95.98,    95.83,    95.78,    95.81,    95.78,    95.60,    95.65]),
                Tmax        = np.array([   98.18,    97.95,    98.17,    98.09,    98.05,    98.00,    97.95]),
                Tavg        = np.array([   97.14,    96.92,    96.97,    96.96,    96.94,    96.89,    96.85])),
   r =     dict(radius      = np.array([    0.00,    50.00,   100.00,   150.00,   200.00,   250.00,   270.00]), # mm
                EW          = np.array([  144.90,   138.90,   135.08,   134.15,   134.40,   135.38,   133.72]),
                lambda_bar  = np.array([  620.59,   623.80,   621.14,   622.74,   623.21,   621.77,   623.72]),
                peak        = np.array([   96.47,    94.66,    92.39,    91.74,    92.12,    93.19,    91.75]),
                on50        = np.array([  542.38,   547.38,   545.37,   547.02,   547.74,   546.63,   548.32]),
                off50       = np.array([  697.74,   699.44,   696.49,   697.87,   697.97,   695.92,   697.99]),
                on10        = np.array([  537.35,   542.44,   540.43,   542.05,   542.79,   541.69,   543.26]),
                off10       = np.array([  702.70,   704.74,   701.74,   703.18,   703.26,   701.16,   703.56]),
                on80        = np.array([  546.61,   553.00,   550.31,   551.93,   552.58,   551.37,   553.48]),
                off80       = np.array([  694.47,   695.88,   692.84,   694.26,   694.34,   692.39,   694.13]),
                Tmin        = np.array([   88.35,    86.91,    84.96,    83.78,    84.31,    85.60,    85.00]),
                Tmax        = np.array([   96.25,    94.41,    91.96,    91.74,    92.12,    93.19,    91.75]),
                Tavg        = np.array([   93.79,    91.61,    89.39,    89.18,    89.57,    90.67,    89.28])),
     r1 =   dict(radius     = np.array([    0.00,   10.00,   20.00,   30.00,   40.00,   50.00,   60.00,   70.00,
                                            80.00,   90.00,  100.00,  110.00,  120.00,  130.00,  140.00,  150.00,
                                            160.00,  170.00,  180.00,  190.00,  200.00,  210.00,  220.00,  230.00,
                                            240.00,  250.00,  260.00,  267.00]), # mm
                 peak       = np.array([   99.20,   99.68,   99.23,   98.71,   98.92,   98.26,   98.36,   98.23,
                                           97.61,   97.33,   96.51,   96.83,   96.26,   96.18,   96.09,   95.91,
                                           95.87,   95.79,   95.92,   95.73,   96.15,   96.22,   96.67,   96.59,
                                           96.86,   97.06,   96.74,   96.30]),
                 on50       = np.array([  542.48,  543.30,  546.23,  549.77,  550.49,  549.50,  548.16,  547.44,
                                          547.54,  548.06,  548.63,  549.14,  549.78,  550.27,  550.20,  549.69,
                                          549.17,  548.80,  548.46,  548.25,  548.30,  548.55,  549.08,  549.72,
                                          550.35,  550.78,  551.04,  551.36]),
                 off50      = np.array([  680.95,  681.41,  683.75,  686.95,  687.74,  686.96,  685.99,  685.54,
                                          685.65,  686.17,  687.10,  688.20,  689.33,  690.24,  690.55,  690.35,
                                          689.99,  689.54,  689.14,  688.91,  688.99,  689.45,  690.22,  691.00,
                                          691.72,  692.09,  692.29,  692.85]),
                 on10       = np.array([  539.33,  540.09,  541.45,  545.03,  546.90,  545.83,  544.64,  544.22,
                                          544.36,  544.91,  545.41,  546.02,  546.56,  547.15,  547.17,  546.71,
                                          546.26,  545.90,  545.54,  545.36,  545.41,  545.71,  546.20,  546.84,
                                          547.45,  548.00,  548.24,  548.53]),
                 off10      = np.array([  685.20,  685.55,  688.95,  691.78,  692.32,  691.51,  690.37,  689.71,
                                          689.76,  690.21,  691.04,  692.08,  693.28,  694.18,  694.50,  694.23,
                                          693.82,  693.45,  692.90,  692.65,  692.74,  693.22,  693.97,  694.81,
                                          695.57,  695.97,  696.07,  696.62]),
                 on80       = np.array([  551.63,  551.87,  552.05,  553.07,  553.51,  552.33,  550.77,  554.03,
                                          549.70,  550.31,  550.84,  551.36,  551.88,  552.45,  557.52,  557.46,
                                          556.96,  556.63,  556.26,  556.23,  556.59,  557.08,  558.02,  558.74,
                                          559.54,  560.09,  560.37,  560.68]),
                 off80      = np.array([  677.34,  677.87,  679.95,  683.19,  684.12,  683.50,  682.58,  682.22,
                                          682.38,  682.84,  683.61,  684.66,  685.75,  686.75,  687.01,  686.82,
                                          686.43,  686.07,  685.63,  685.38,  685.35,  685.79,  686.32,  687.27,
                                          687.93,  688.17,  688.24,  688.32]),
                 Tmin       = np.array([    0.93,    0.93,    0.91,    0.93,    0.93,    0.92,    0.93,    0.92,
                                            0.92,    0.92,    0.91,    0.90,    0.90,    0.90,    0.90,    0.90,
                                            0.90,    0.90,    0.90,    0.90,    0.90,    0.90,    0.91,    0.91,
                                            0.91,    0.91,    0.90,    0.90]),
                 Tmax       = np.array([    0.99,    1.00,    0.99,    0.99,    0.99,    0.98,    0.98,    0.98,
                                            0.98,    0.97,    0.97,    0.97,    0.96,    0.96,    0.96,    0.96,
                                            0.96,    0.96,    0.96,    0.96,    0.96,    0.96,    0.97,    0.97,
                                            0.97,    0.97,    0.97,    0.96]),
                 Tavg       = np.array([    0.96,    0.97,    0.96,    0.96,    0.96,    0.96,    0.96,    0.95,
                                            0.95,    0.95,    0.94,    0.94,    0.94,    0.94,    0.94,    0.93,
                                            0.93,    0.93,    0.93,    0.93,    0.93,    0.94,    0.94,    0.94,
                                            0.94,    0.94,    0.94,    0.94]),
                 EW         = np.array([  133.29,  133.50,  132.65,  132.12,  131.86,  131.65,  131.74,  131.60,
                                          131.25,  130.73,  130.34,  130.69,  130.90,  131.15,  131.40,  131.33,
                                          131.62,  131.51,  131.38,  131.38,  131.39,  132.07,  132.60,  132.85,
                                          133.07,  133.10,  132.78,  132.35]),
                 lambda_bar = np.array([  614.49,  614.87,  616.00,  618.13,  618.82,  617.91,  616.67,  618.12,
                                          616.04,  616.58,  617.23,  618.01,  618.82,  619.60,  622.26,  622.14,
                                          621.69,  621.35,  620.94,  620.81,  620.97,  621.43,  622.17,  623.00,
                                          623.73,  624.13,  624.31,  624.50]),
                 ),
   i =     dict(radius      = np.array([    0.00,    50.00,   100.00,   150.00,   200.00,   250.00,   270.00]), # mm
                EW          = np.array([  141.50,   142.01,   143.25,   144.59,   146.72,   148.23,   147.72]),
                lambda_bar  = np.array([  771.10,   771.81,   771.80,   770.18,   769.23,   773.33,   774.76]),
                peak        = np.array([   95.38,    95.09,    95.00,    94.81,    94.85,    94.61,    93.66]),
                on50        = np.array([  696.47,   696.74,   696.01,   693.52,   691.35,   694.64,   696.20]),
                off50       = np.array([  844.27,   845.38,   845.84,   845.46,   845.48,   850.22,   852.02]),
                on10        = np.array([  688.47,   688.75,   688.03,   685.46,   683.38,   686.58,   688.15]),
                off10       = np.array([  857.11,   857.91,   858.34,   857.77,   857.75,   864.25,   865.20]),
                on80        = np.array([  701.04,   701.27,   700.50,   697.96,   695.72,   699.06,   700.65]),
                off80       = np.array([  839.78,   840.75,   841.29,   840.85,   841.01,   844.89,   845.35]),
                Tmin        = np.array([   92.99,    92.59,    92.41,    91.99,    91.93,    92.10,    91.07]),
                Tmax        = np.array([   95.33,    94.91,    95.00,    94.56,    94.36,    94.57,    93.66]),
                Tavg        = np.array([   94.25,    94.02,    93.93,    93.68,    93.62,    93.68,    92.69])),
   z =     dict(radius      = np.array([    0.00,    50.00,   100.00,   150.00,   200.00,   250.00,   270.00]), # mm
                EW          = np.array([   80.01,    80.12,    80.14,    79.37,    78.82,    80.41,    80.26]),
                lambda_bar  = np.array([  892.63,   892.33,   892.18,   891.72,   891.99,   891.25,   891.45]),
                peak        = np.array([   98.53,    98.82,    99.17,    99.05,    98.59,    99.26,    99.66]),
                on50        = np.array([  852.97,   852.75,   852.75,   852.66,   852.77,   852.37,   852.36]),
                off50       = np.array([  932.22,   931.92,   931.67,   930.89,   930.99,   930.86,   930.81]),
                on10        = np.array([  844.28,   844.11,   844.25,   843.83,   844.40,   843.20,   843.54]),
                off10       = np.array([  940.81,   940.58,   940.30,   939.53,   939.57,   939.62,   939.50]),
                on80        = np.array([  858.27,   857.95,   858.00,   858.02,   858.10,   857.70,   857.58]),
                off80       = np.array([  926.58,   926.29,   926.08,   925.21,   925.36,   925.12,   925.09]),
                Tmin        = np.array([   97.07,    96.86,    96.91,    97.21,    97.00,    97.57,    97.78]),
                Tmax        = np.array([   98.53,    98.82,    99.17,    99.05,    98.59,    99.26,    99.66]),
                Tavg        = np.array([   97.97,    98.07,    98.32,    98.34,    98.03,    98.63,    98.98])),
    y =    dict(radius      = np.array([    0.00,    50.00,   100.00,   150.00,   200.00,   250.00,   270.00]), # mm
                EW          = np.array([  141.37,   141.34,   139.06,   141.19,   139.50,   143.70,   144.91]),
                lambda_bar  = np.array([ 1002.77,  1002.98,  1004.36,  1003.56,  1003.99,  1003.19,  1001.92]),
                peak        = np.array([   97.92,    98.21,    97.74,    98.36,    97.59,    98.18,    97.86]),
                on50        = np.array([  933.55,   934.12,   935.81,   935.33,   934.85,   932.85,   930.31]),
                off50       = np.array([ 1072.36,  1072.35,  1072.90,  1072.62,  1072.86,  1073.84,  1073.22]),
                on10        = np.array([  923.65,   924.03,   926.13,   925.00,   925.35,   922.85,   920.70]),
                off10       = np.array([ 1082.85,  1082.86,  1083.39,  1083.36,  1083.38,  1084.44,  1083.83]),
                on80        = np.array([  939.26,   939.65,   941.54,   941.23,   941.02,   939.10,   936.69]),
                off80       = np.array([ 1065.99,  1065.89,  1066.40,  1066.11,  1066.32,  1067.11,  1066.53]),
                Tmin        = np.array([   96.25,    96.64,    96.20,    97.17,    96.06,    96.63,    96.31]),
                Tmax        = np.array([   97.92,    98.21,    97.74,    98.36,    97.59,    98.18,    97.86]),
                Tavg        = np.array([   97.13,    97.42,    97.03,    97.84,    96.85,    97.47,    97.25])),
   )

def plotRadialFilterData(what, filters="grizy", doSpline=False):
    options = ["EW", "lambda"]
    if what not in options:
        raise RuntimeError("Please choose one of %s" % " ".join(options))

    filterColor = dict(
        g  = 'green',
        r  = 'red',
        r1 = 'orange',
        i  = 'magenta',
        z  = 'black',
        y  = 'brown',
        )

    plt.clf()

    ymin, ymax = None, None
    for b in filters:
        filterR = filterData[b]["radius"]
        filterR = filterR/15e-3         # convert to pixels

        if what == "EW":
            y = filterData[b]["EW"]
            y = y/y[0]
        elif what == "lambda":
            y = filterData[b]["lambda_bar"]
            n = len(y)
            xvec = np.empty(2*n);  yvec = np.empty_like(xvec)
            xvec[0:n] = filterR;  xvec[n:] = filterR[::-1]

            if ymin is None:
                ymin, ymax = 1e30, -1e30

            for i in (10, 50, 80):
                alpha = 0.1 + 5e-3*i

                yvec[0:n] = filterData[b]["on%d" % i]
                yvec[n:] =  filterData[b]["off%d" % i][::-1]

                p = plt.Polygon(zip(xvec, yvec), color=filterColor[b], alpha=alpha)
                plt.axes().add_artist(p)

                ymin, ymax = (min(ymin, np.min(yvec)), max(ymax, np.max(yvec)))
        else:
            raiseRuntimeError("You can't get here")

        plt.plot(filterR, y, color=filterColor[b], label=b)
        plt.plot(filterR, y, "o", color=filterColor[b])
        if scipy and doSpline:
            if what == "EW":
                interpolator = makeEvenSplineInterpolator(filterR, y)
                r = np.linspace(0, filterR.max(), 100)
                plt.plot(r, interpolator(r), "--", color=filterColor[b])

    plt.xlabel("Radius (pixels)")
    if what == "EW":
        plt.ylabel("Equivalent Width (relative to R==0)")
    elif what == "lambda":
        plt.ylabel(r"$\lambda$ (nm)")
        d = ymax - ymin
        plt.ylim(ymin - 0.1*d, ymax + 0.1*d)
    else:
        raiseRuntimeError("You can't get here")

    legend = plt.legend(loc="best")
    if legend:
        legend.draggable(True)
    plt.show()

def makeNewFlats(butler, calibDir, filter, modelVignetting=True, modelJacobian=True, modelQE=True,
                 modelGain=True, modelFilter=True, visit=None, ccdList=None):
    camera = butler.get("camera")

    if not os.path.exists(calibDir):
        os.makedirs(calibDir)

    filterName = filter
    if filterName in "grizy":
        filterName = "HSC-%s" % filterName
    filterName = filterName.upper()

    if not visit:
        visits = butler.queryMetadata('raw', 'visit', ['visit',], filter=filterName)
        if visits:
            visit = visits[0]
        else:
            raise RuntimeError("I can't find any data taken with the %s filter" % filter)

    if not ccdList:
        ccdList = butler.queryMetadata('raw', 'visit', ['ccd',], visit=visit)

    if modelFilter:
        filterThroughput = (filterData[filter]["radius"], filterData[filter]["EW"])
    else:
        filterThroughput = None

    for ccd in ccdList:
        fileName = butler.get("flat_filename", visit=visit, ccd=ccd)[0]
        flat     = butler.get("flat",          visit=visit, ccd=ccd)

        ccd = flat.getDetector()

        makeFlatImage(flat, filter=filter,
                      modelVignetting=modelVignetting, modelJacobian=modelJacobian,
                      modelQE=modelQE, filterThroughput=filterThroughput)

        if modelGain:
            flatIm = flat.getMaskedImage().getImage()
            for a in ccd:
                flatIm[a.getDataSec(True)] /= a.getElectronicParams().getGain()

        ofileName = os.path.join(calibDir, os.path.split(fileName)[1])
        print ofileName

        md = flat.getMetadata()
        md.add("COMMENT", "Artificial Flat")

        md.set("FILTER", filter)
        md.add("COMMENT", "Which effects were modelled:")

        md.set("FILTER TRANSMISSION", modelFilter)
        md.set("GAINS", modelGain)
        md.set("JACOBIAN", modelJacobian)
        md.set("QE", modelQE)
        md.set("VIGNETTING", modelVignetting)

        flat.writeFits(ofileName)

class DataRef(object):
    """Fake a ButlerDataRef for the isrTask"""
    def __init__(self, dataId, **kwargs):
        self.dataId = dataId
        self._dict = dict(**kwargs)

    def get(self, what, immediate=False):
        if what == "raw_md":
            return self._dict.get("raw").getMetadata()
        else:
            return self._dict.get(what, what)

def isrCallbackQE(im, ccd=None, butler=None, imageSource=None, **kwargs):
    im = isrCallback(im, ccd=ccd, butler=butler, imageSource=imageSource, **kwargs)

    filter = imageSource.filter

    ccdSerial = ccd.getId().getSerial()
    if True:                        # new way, using SDSS photometry to get QE
        correction = 1/getQE(ccdSerial, filter)
    else:                           # old way, using a g frame and assuming continuity in rizy
        correction = getLevel(ccdSerial, filter, fiddle=True)

    im *= correction

    return im

def isrCallback(im, ccd=None, butler=None, imageSource=None, doFlatten=True, correctGain=False,
                doBias=False, doDark=False, doSaturation=True):
    """Trim and linearize a raw CCD image, and maybe convert from DN to e^-, using ip_isr
    """
    from lsst.obs.subaru.isr import SubaruIsrTask as IsrTask

    isrTask = IsrTask()
    isrTask.log.setThreshold(100)

    for k in isrTask.config.qa.keys():
        if re.search(r"^do", k):
            setattr(isrTask.config.qa, k, False)

    if doFlatten:
        correctGain = False

    isrTask.config.doBias = doBias
    isrTask.config.doDark = doDark
    isrTask.config.doFlat = doFlatten
    isrTask.config.doLinearize = True
    isrTask.config.doApplyGains = correctGain
    isrTask.config.doSaturation = doSaturation
    isrTask.config.doDefect = False
    isrTask.config.doGuider = False
    isrTask.config.doMaskNaN = False
    isrTask.config.doFringe = False
    isrTask.config.doWrite = False

    isMI = hasattr(im, 'getImage')
    if not isMI:
        im = afwImage.makeMaskedImage(im)

    ccdId = ccd.getId().getSerial()
    visit = imageSource.kwargs["visit"]
    md = butler.get("raw_md", visit=visit, ccd=ccdId)

    raw = afwImage.makeExposure(im)
    raw.setDetector(ccd)
    raw.setCalib(afwImage.Calib(md))
    raw.setMetadata(md)

    filter = imageSource.filter
    if filter in "grizy":
        filter = "HSC-" + filter
    filter = filter.upper()

    taiObs = md.get("DATE-OBS")
    bias = butler.get("bias", ccd=ccdId, taiObs=taiObs, visit=0) \
        if isrTask.config.doBias else None
    flat = butler.get("flat", ccd=ccdId, filter=filter, taiObs=taiObs, visit=0) \
        if isrTask.config.doFlat else None
    dark = butler.get("dark", ccd=ccdId, taiObs=taiObs, visit=0) \
        if isrTask.config.doDark else None

    if dark and dark.getCalib().getExptime() == 0:
        dark.getCalib().setExptime(1.0)

    if imageSource and imageSource.verbose:
        print "Running ISR for visit %d CCD %3d" % (visit, ccdId)

    result = isrTask.run(raw, bias=bias, dark=dark, flat=flat)

    mi = result.exposure.getMaskedImage()
    return mi if isMI else mi.getImage()

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def foo(data, visits=(902936, 903332, 902476), ccdIds=[], fig0=1, dfig=1, savefig=False, fileNameSuffix = "",
        showCamera=True, showZP=False, correctJacobian=True, pclim=(None, None),
        visit2=None, zplim=(None, None), zp2lim=(None, None), rmax=None,
        **kwargs):
    fig = fig0

    if False:
        for v in visits:
            if True:
                figure = anUtils.plotCalibration(data, selectObjId=anUtils.makeSelectVisit(v,ccds=ccdIds), fig=fig,
                                                 showCamera=showCamera, showZP=showZP,
                                                 correctJacobian=correctJacobian, 
                                                 ymin=pclim[0], ymax=pclim[1], markersize=1.0,
                                                 **kwargs)
                if savefig:
                    fileName = "photoCal%s-%d" % (fileNameSuffix, v)
                    figure.savefig(fileName + ".png")

            fig += dfig

    if True:
        for v in visits:
            for v2 in (None,) if visit2 is None else (None, visit2):
                if v == v2:
                    continue

                figure = plotCcdZP(data.butler, v, ccdIds=ccdIds, visit2=v2, fig=fig,
                                   correctJacobian=False if True else correctJacobian,
                                   rmax=rmax,
                                   zlim=zplim if v2 is None else zp2lim,
                                   **kwargs)

                if savefig:
                    fileName = "ZP%s-%d" % (fileNameSuffix, v)
                    if v2:
                        fileName += "-%d" % v2
                    figure.savefig(fileName + ".png")

                fig += dfig
