import math
import os
import re
import sys
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt;  pyplot = plt
import numpy as np

import lsst.daf.base as dafBase
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
        ccd = afwCG.cast_Ccd(im.getDetector())

    for attr in ["getMaskedImage", "convertF"]:
        if hasattr(im, attr):
            im = getattr(im, attr)()

    tim = im[ccd.getAllPixels(True)].clone()

    for ia, a in enumerate(ccd):
        sim = im[a.getDiskDataSec()]
        sim -= afwMath.makeStatistics(im[a.getDiskBiasSec()], afwMath.MEANCLIP).getValue()
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
            ccd = afwCG.cast_Ccd(exp.getDetector())
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
        ccd = afwCG.cast_Ccd(im.getDetector())
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

        ccd = afwCG.cast_Ccd(calexp.getDetector())
        ccdImage = calexp.getMaskedImage().getImage()
        for amp in ccd:
            bias = afwMath.makeStatistics(ccdImage[amp.getBiasSec()], afwMath.MEANCLIP).getValue()
            data = afwMath.makeStatistics(ccdImage[amp.getDataSec()], afwMath.MEANCLIP).getValue()
            print did, amp.getId().getSerial(), "%7.2f, %7.2f" % (bias, data)

        
def showElectronics(camera, maxCorr=1.8e5):
    print "%-12s" % (camera.getId())
    for raft in [afwCG.cast_Raft(det) for det in camera]:
        print "   %-12s" % (raft.getId())
        for ccd in [afwCG.cast_Ccd(det) for det in raft]:
            print "      %-12s" % (ccd.getId())
            for amp in ccd:
                ep = amp.getElectronicParams()
                print "         %s %.2f %.1f %.1f" % (amp.getId(),
                                                   ep.getGain(), ep.getSaturationLevel(), maxCorr/ep.getGain())

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

        ccd = afwCG.cast_Ccd(exp.getDetector())
        if False:
            im = afwCGUtils.trimRawCallback(exp.getMaskedImage().convertF(), ccd, correctGain=False)
        else:
            im = isrCallback(exp.getMaskedImage(), ccd, correctGain=False)
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
            
            ccd = afwCG.cast_Ccd(exp.getDetector())
            im = isrCallback(exp.getMaskedImage(), ccd)
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
        dumpCcdParams(afwCG.cast_Ccd(ccd), nIndent + dIndent, dIndent, fd=fd)

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
        dumpCcdElectronicParams(afwCG.cast_Ccd(ccd), nIndent + dIndent, dIndent, fd=fd)

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
    import lsst.analysis.utils as anUtils

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
            ccd = afwCG.cast_Ccd(raw.getDetector())
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

def plotCcdTemperatures(butler, visit):
    camera = butler.get("camera")
    ccdIds = butler.queryMetadata('raw', 'visit', ['ccd',], visit=visit)

    temp = np.empty_like(ccdIds)
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
    ccd = afwCG.cast_Ccd(exp.getDetector())

    if hasattr(exp, "convertF"):
        exp = exp.convertF()

    im = exp.getMaskedImage()

    for a in ccd:
        im[a.getDiskAllPixels()] -= afwMath.makeStatistics(im[a.getDiskBiasSec()], afwMath.MEANCLIP).getValue()

    return exp

def grads(exp):
    ccd = afwCG.cast_Ccd(exp.getDetector())
    im = exp.getMaskedImage().getImage()

    plt.clf()
    for a in ccd:
        ima = im[a.getDiskDataSec()].getArray()
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

levels = {                          # Set from g-band sky 902986 (~ 65k e/pixel)
    0 : 43403.91,
    2 : 43475.00,
    6 : 45646.65,
    8 : 43301.74,
    10 : 43978.67,
    12 : 44293.16,
    4 : 43734.02,
    14 : 44390.88,
    1 : 44305.02,
    16 : 46750.84,
    7 : 47042.65,
    13 : 46904.64,
    18 : 47514.49,
    15 : 45602.04,
    11 : 46206.36,
    3 : 46105.14,
    20 : 46220.73,
    5 : 45130.94,
    9 : 43843.98,
    17 : 45602.81,
    24 : 46964.68,
    26 : 47609.61,
    19 : 46901.52,
    21 : 46797.34,
    28 : 44727.69,
    22 : 43524.61,
    34 : 48517.57,
    32 : 46906.66,
    30 : 43167.49,
    25 : 45742.11,
    38 : 43484.00,
    36 : 44505.52,
    27 : 45897.85,
    40 : 45357.87,
    23 : 43763.25,
    42 : 48451.24,
    35 : 45633.72,
    33 : 45385.44,
    39 : 44324.83,
    31 : 45565.83,
    29 : 44373.68,
    44 : 45896.54,
    46 : 43871.89,
    48 : 47345.57,
    41 : 47269.23,
    37 : 44076.87,
    50 : 46363.21,
    43 : 47208.25,
    54 : 44049.64,
    52 : 47038.79,
    56 : 47352.31,
    47 : 47213.46,
    49 : 47184.77,
    60 : 44624.29,
    58 : 46172.26,
    45 : 46746.83,
    51 : 44757.63,
    57 : 46203.75,
    55 : 45615.36,
    64 : 47884.43,
    62 : 43105.61,
    66 : 49160.41,
    68 : 46512.36,
    53 : 43994.41,
    59 : 46128.25,
    72 : 45134.80,
    61 : 44380.55,
    65 : 47685.35,
    74 : 48786.36,
    76 : 45419.64,
    63 : 45568.49,
    70 : 46890.92,
    67 : 44784.35,
    78 : 45410.07,
    80 : 49517.01,
    82 : 47927.74,
    84 : 45733.28,
    69 : 44967.30,
    73 : 46785.52,
    86 : 46039.84,
    75 : 47307.23,
    71 : 47169.37,
    88 : 45553.18,
    79 : 45274.47,
    81 : 46178.20,
    77 : 45155.86,
    83 : 45173.39,
    92 : 48012.97,
    85 : 45101.29,
    94 : 45261.70,
    87 : 45407.47,
    98 : 46693.97,
    90 : 45250.52,
    96 : 43334.68,
    93 : 46665.44,
    89 : 43998.86,
    100 : 43482.74,
    91 : 45547.59,
    95 : 45804.45,
    104 : 38319.73,
    97 : 44616.59,
    106 : 38008.73,
    99 : 47609.93,
    102 : 43768.43,
    108 : 39169.61,
    110 : 40072.82,
    101 : 44730.29,
    107 : 38863.93,
    105 : 38908.18,
    109 : 37313.50,
    103 : 46055.49,
    111 : 39301.70,
    }
#
# These are the levels at the edges of the CCDs, so edges['r'][69][77] is the level (in r)
# on CCD69 adjacent to CCD77
#
# They were calculated with e.g.
#  for serial in range(112):
#     ccdTesting.correctQE(serial, filter='y', calculate=True, visit=904352, butler=butler)
#
edges = { 'r' :
              {
        103 : {None: None,  77 : 29705 },
        77 : { 103 : 31180, 69 : 31390 },
        69 : {  77 : 31313, 61 : 31337 },
        61 : {  69 : 31918, 53 : 32065 },
        53 : {  61 : 32346, 45 : 32460, 52 : 35019 },
        45 : {  53 : 29950, 37 : 29999 },
        37 : {  45 : 32477, 29 : 32679 },
        29 : {  37 : 31832, 101: 31860 },
        101 : { 29 : 31085, None : 1 },

        109: {None : None,  95 : 36073 },
        95 : { 109 : 29851, 89 : 30867 },
        89 : {  95 : 33070, 83 : 32784 },
        83 : {  89 : 31664, 76 : 31402 },
        76 : {  83 : 31294, 68 : 31270 },
        68 : {  76 : 30291, 60 : 30194 },
        60 : {  68 : 31448, 52 : 31588 },
        52 : {  60 : 29425, 44 : 29657, 51 : 28935, 53 : 32085 },
        44 : {  52 : 30783, 36 : 32193 },
        36 : {  44 : 33929, 28 : 34284 },
        28 : {  36 : 33905, 21 : 32489 },
        21 : {  28 : 30529, 15 : 30607 },
        15 : {  21 : 31092,  9 : 31372 },
        9  : {  15 : 32620, 107: 31999 },
        107: {   9 : 35251, None:None },

        111 : {None : None,  99 : 35635 - (32061 - 30229) },
        99 : {111  : 27951, 94 : 30343 },
        94 : { 99  : 32764, 88 : 32143 },
        88 : { 94  : 32140, 82 : 31733 },
        82 : { 88  : 29710, 75 : 29682 },
        75 : { 82  : 30159, 67 : 30139 },
        67 : { 75  : 32531, 59 : 32306 },
        59 : { 67  : 31120, 51 : 31263 },
        51 : { 59  : 32490, 43 : 32920,  50 : 33234, 52 : 31854 },
        43 : { 51  : 30507, 35 : 30977 },
        35 : { 43  : 32920, 27 : 33451 },
        27 : { 35  : 32797, 20 : 33144 },
        20 : { 27  : 32654, 14 : 32940 },
        14 : { 20  : 34740,  8 : 34147 },
        8 : { 14  : 35087,  3 : 34719 },
        3 : {  8  : 31816, 105: 29300 },
        105 : {  3  : 33512, None:None },

        98 : {None : None,  93 : 32180 },
        93 : {98 : 31682,   87 : 31316 },
        87 : {93 : 32867,   81 : 32867 },
        81 : {87 : 32340,   74 : 32355 },
        74 : {81 : 30014,   66 : 29795 },
        66 : {74 : 29709,   58 : 30014 },
        58 : {66 : 32677,   50 : 33774 },
        50 : {58 : 33847,   42 : 34579, 49 : 34261, 51 : 32423, },
        42 : {50 : 32267,   34 : 31960 },
        34 : {42 : 31609,   26 : 30980 },
        26 : {34 : 31785,   19 : 31814 },
        19 : {26 : 32530,   13 : 32355 },
        13 : {19 : 32165,    7 : 31872 },
        7 : {13 : 32109,    2 : 31595 },
        2 : { 7 : 34769, None : None  },

        97 : { None:None,   92 : 34138 },
        92 : {  97 : 31032, 86 : 30847 },
        86 : {  92 : 32626, 80 : 32852 },
        80 : {  86 : 29573, 73 : 29574 },
        73 : {  80 : 32420, 65 : 32222 },
        65 : {  73 : 31050, 57 : 31700 },
        57 : {  65 : 33333, 49 : 34078 },
        49 : {  57 : 33160, 41 : 33104, 50 : 33514, 48 : 32000 },
        41 : {  49 : 32866, 33 : 32500 },
        33 : {  41 : 33790, 25 : 33393 },
        25 : {  33 : 33392, 18 : 33555 },
        18 : {  25 : 31429, 12 : 31300 },
        12 : {  18 : 34582,  6 : 34168 },
        6 : {  12 : 32780,  1 : 32690 },
        1 : {   6 : 33969,  None:None },

        110 : { None:None,   96 : 34200 },
        96 : { 110 : 32600, 91 : 33620 },
        91 : {  96 : 31915, 85 : 31940 },
        85 : {  91 : 32291, 79 : 34677 },
        79 : {  85 : 34840, 72 : 34740 },
        72 : {  79 : 34851, 64 : 34857 },
        64 : {  72 : 31630, 56 : 31556 },
        56 : {  64 : 32163, 48 : 32400 },
        48 : {  56 : 32155, 40 : 32124, 49 : 31500, 47 : 32046},
        40 : {  48 : 33900, 32 : 32197 },
        32 : {  40 : 30540, 24 : 32414 },
        24 : {  32 : 32370, 17 : 32580 },
        17 : {  24 : 33897, 11 : 33583 },
        11 : {  17 : 32506,  5 : 32430 },
        5 : {  11 : 33890,  0 : 33894 },
        0 : {   5 : 35492, 104: 34916 },
        104 : {   0 : 38014, None:None},

        108 : { None: None,  90 : 37900 },
        90  : { 108 : 31893, 84 : 33660 },
        84  : {  90 : 33940, 78 : 33865 },
        78  : {  84 : 34310, 71 : 33934 },
        71  : {  78 : 32561, 63 : 32368 },
        63  : {  71 : 33624, 55 : 33194 },
        55  : {  63 : 33124, 47 : 32881 },
        47  : {  55 : 31722, 39 : 31578, 46 : 32341, 48 : 31740 },
        39  : {  47 : 34346, 31 : 34201 },
        31  : {  39 : 33005, 23 : 33070 },
        23  : {  31 : 34688, 16 : 34745 },
        16  : {  23 : 31625, 10 : 31753 },
        10  : {  16 : 34849,  4 : 34995 },
        4   : {  10 : 34525, 106: 33833 },
        106 : {  4  : 38815, None:None },

        102 : { None:None,   70 : 33492 },
        70  : { 102 : 30747, 62 : 30736 },
        62  : {  70 : 34957, 54 : 34695 },
        54  : {  62 : 33636, 46 : 33466 },
        46  : {  54 : 33447, 38 : 33412, 47 : 34625 },
        38  : {  46 : 33682, 30 : 33546 },
        30  : {  38 : 33976, 22 : 34038 },
        22  : {  30 : 33630, 100: 33614 },
        100 : {  22 : 32750, None:None },
    },
    'i' : {
        0   : {   5 : 11565, 104 : 10050,  },
        1   : { None:None,   6 : 11812,  },
        2   : { None:None,   7 : 12124,  },
        3   : {   8 : 10498, 105 :  9144,  },
        4   : {  10 : 10583, 106 :  6096,  },
        5   : {   0 : 11069,  11 : 11730,  },
        6   : {   1 : 11428,  12 : 12190,  },
        7   : {   2 : 10985,  13 : 11768,  },
        8   : {   3 : 11654,  14 : 12372,  },
        9   : {  15 : 10304, 107 :  5727,  },
        10  : {   4 : 10933,  16 : 11609,  },
        11  : {   5 : 11312,  17 : 11938,  },
        12  : {   6 : 12885,  18 : 13624,  },
        13  : {   7 : 11879,  19 : 12477,  },
        14  : {   8 : 12088,  20 : 12722,  },
        15  : {   9 : 10018,  21 : 10705,  },
        16  : {  10 : 10754,  23 : 11145,  },
        17  : {  11 : 12462,  24 : 13023,  },
        18  : {  12 : 12260,  25 : 13005,  },
        19  : {  13 : 12632,  26 : 13284,  },
        20  : {  14 : 12000,  27 : 12562,  },
        21  : {  15 : 10547,  28 : 11014,  },
        22  : {  30 : 10788, 100 :  7968,  },
        23  : {  16 : 12398,  31 : 12762,  },
        24  : {  17 : 12527,  32 : 13040,  },
        25  : {  18 : 14062,  33 : 14704,  },
        26  : {  19 : 13080,  34 : 13688,  },
        27  : {  20 : 12868,  35 : 13369,  },
        28  : {  21 : 11853,  36 : 12253,  },
        29  : {  37 : 10214, 101 :  6695,  },
        30  : {  22 : 10947,  38 : 11160,  },
        31  : {  23 : 12194,  39 : 12461,  },
        32  : {  24 : 13098,  40 : 13521,  },
        33  : {  25 : 14817,  41 : 15111,  },
        34  : {  26 : 13353,  42 : 13869,  },
        35  : {  27 : 13744,  43 : 14175,  },
        36  : {  28 : 12485,  44 : 12800,  },
        37  : {  29 : 10573,  45 : 10816,  },
        38  : {  30 : 11053,  46 : 11261,  },
        39  : {  31 : 12964,  47 : 13094,  },
        40  : {  32 : 14363,  48 : 14528,  },
        41  : {  33 : 14729,  49 : 15030,  },
        42  : {  34 : 14081,  50 : 14391,  },
        43  : {  35 : 13319,  51 : 13543,  },
        44  : {  36 : 12152,  52 : 12375,  },
        45  : {  37 :  9942,  53 : 10071,  },
        46  : {  38 : 11200,  47 : 12097,  54 : 11190,  },
        47  : {  39 : 11966,  46 : 11340,  48 : 12682,  55 : 11981,  },
        48  : {  40 : 13619,  47 : 13025,  49 : 14233,  56 : 13593,  },
        49  : {  41 : 15108,  48 : 14779,  50 : 15293,  57 : 15113,  },
        50  : {  42 : 15517,  49 : 15745,  51 : 15207,  58 : 15550,  },
        51  : {  43 : 14832,  50 : 15475,  52 : 14292,  59 : 14885,  },
        52  : {  44 : 11932,  51 : 12658,  53 : 11318,  60 : 11946,  },
        53  : {  45 : 11059,  52 : 12107,  61 : 11124,  },
        54  : {  46 : 11126,  62 : 10953,  },
        55  : {  47 : 12418,  63 : 12243,  },
        56  : {  48 : 13652,  64 : 13478,  },
        57  : {  49 : 15612,  65 : 15236,  },
        58  : {  50 : 15659,  66 : 15340,  },
        59  : {  51 : 14243,  67 : 14064,  },
        60  : {  52 : 12891,  68 : 12758,  },
        61  : {  53 : 10974,  69 : 10857,  },
        62  : {  54 : 11291,  70 : 11073,  },
        63  : {  55 : 12252,  71 : 12051,  },
        64  : {  56 : 13052,  72 : 12727,  },
        65  : {  57 : 14350,  73 : 13880,  },
        66  : {  58 : 13963,  74 : 13486,  },
        67  : {  59 : 14664,  75 : 14314,  },
        68  : {  60 : 12203,  76 : 11993,  },
        69  : {  61 : 10663,  77 : 10459,  },
        70  : {  62 :  9605, 102 :  7119,  },
        71  : {  63 : 11538,  78 : 11215,  },
        72  : {  64 : 13965,  79 : 13449,  },
        73  : {  65 : 14411,  80 : 13822,  },
        74  : {  66 : 13486,  81 : 12941,  },
        75  : {  67 : 13059,  82 : 12609,  },
        76  : {  68 : 12371,  83 : 12002,  },
        77  : {  69 : 10437, 103 :  6855,  },
        78  : {  71 : 11603,  84 : 11256,  },
        79  : {  72 : 13322,  85 : 12779,  },
        80  : {  73 : 12445,  86 : 11828,  },
        81  : {  74 : 13933,  87 : 13222,  },
        82  : {  75 : 12288,  88 : 11765,  },
        83  : {  76 : 12057,  89 : 11633,  },
        84  : {  78 : 11046,  90 : 10456,  },
        85  : {  79 : 12650,  91 : 12055,  },
        86  : {  80 : 13086,  92 : 12380,  },
        87  : {  81 : 13375,  93 : 12681,  },
        88  : {  82 : 12530,  94 : 11953,  },
        89  : {  83 : 11783,  95 : 11066,  },
        90  : {  84 : 10202, 108 :  6424,  },
        91  : {  85 : 11811,  96 : 11128,  },
        92  : {  86 : 11543,  97 : 10895,  },
        93  : {  87 : 11918,  98 : 11287,  },
        94  : {  88 : 11978,  99 : 11406,  },
        95  : {  89 : 10149, 109 :  5122,  },
        96  : {  91 : 11724, 110 : 10304,  },
        97  : { None:None,  92 : 11992,  },
        98  : { None:None,  93 : 11375,  },
        99  : {  94 : 10415, 111 :  8951,  },
        100 : { None:None,  22 : 10590,  },
        101 : { None:None,  29 :  9862,  },
        102 : { None:None,  70 : 10602,  },
        103 : { None:None,  77 :  9874,  },
        104 : { None:None,   0 : 10333,  },
        105 : { None:None,   3 :  9785,  },
        106 : { None:None,   4 :  6250,  },
        107 : { None:None,   9 :  5666,  },
        108 : { None:None,  90 :  6856,  },
        109 : { None:None,  95 :  5590,  },
        110 : { None:None,  96 : 10650,  },
        111 : { None:None,  99 :  8688,  },
        },
        'y' : {
        0   : {   5 :  3316, 104 :  2922,  },
        1   : { None:None,   6 :  3375,  },
        2   : { None:None,   7 :  3505,  },
        3   : {   8 :  3061, 105 :  2738,  },
        4   : {  10 :  3127, 106 :  1834,  },
        5   : {   0 :  3196,  11 :  3384,  },
        6   : {   1 :  3295,  12 :  3502,  },
        7   : {   2 :  3196,  13 :  3419,  },
        8   : {   3 :  3366,  14 :  3548,  },
        9   : {  15 :  3148, 107 :  1758,  },
        10  : {   4 :  3211,  16 :  3387,  },
        11  : {   5 :  3263,  17 :  3451,  },
        12  : {   6 :  3701,  18 :  3948,  },
        13  : {   7 :  3443,  19 :  3636,  },
        14  : {   8 :  3450,  20 :  3629,  },
        15  : {   9 :  3010,  21 :  3159,  },
        16  : {  10 :  3116,  23 :  3228,  },
        17  : {  11 :  3628,  24 :  3810,  },
        18  : {  12 :  3551,  25 :  3821,  },
        19  : {  13 :  3702,  26 :  3911,  },
        20  : {  14 :  3467,  27 :  3667,  },
        21  : {  15 :  3143,  28 :  3235,  },
        22  : {  30 :  3201, 100 :  2388,  },
        23  : {  16 :  3571,  31 :  3677,  },
        24  : {  17 :  3643,  32 :  3828,  },
        25  : {  18 :  4147,  33 :  4369,  },
        26  : {  19 :  3882,  34 :  4100,  },
        27  : {  20 :  3750,  35 :  3937,  },
        28  : {  21 :  3516,  36 :  3624,  },
        29  : {  37 :  3073, 101 :  2050,  },
        30  : {  22 :  3202,  38 :  3263,  },
        31  : {  23 :  3561,  39 :  3642,  },
        32  : {  24 :  3903,  40 :  4044,  },
        33  : {  25 :  4376,  41 :  4498,  },
        34  : {  26 :  4014,  42 :  4222,  },
        35  : {  27 :  4007,  43 :  4175,  },
        36  : {  28 :  3621,  44 :  3702,  },
        37  : {  29 :  3175,  45 :  3224,  },
        38  : {  30 :  3220,  46 :  3262,  },
        39  : {  31 :  3818,  47 :  3861,  },
        40  : {  32 :  4281,  48 :  4324,  },
        41  : {  33 :  4482,  49 :  4589,  },
        42  : {  34 :  4260,  50 :  4374,  },
        43  : {  35 :  3994,  51 :  4081,  },
        44  : {  36 :  3539,  52 :  3583,  },
        45  : {  37 :  3004,  53 :  3029,  },
        46  : {  38 :  3343,  47 :  3557,  54 :  3326,  },
        47  : {  39 :  3516,  46 :  3312,  48 :  3767,  55 :  3514,  },
        48  : {  40 :  4054,  47 :  3824,  49 :  4277,  56 :  4032,  },
        49  : {  41 :  4568,  48 :  4434,  50 :  4673,  57 :  4580,  },
        50  : {  42 :  4686,  49 :  4790,  51 :  4528,  58 :  4687,  },
        51  : {  43 :  4361,  50 :  4595,  52 :  4143,  59 :  4365,  },
        52  : {  44 :  3493,  51 :  3749,  53 :  3283,  60 :  3496,  },
        53  : {  45 :  3289,  52 :  3498,  61 :  3289,  },
        54  : {  46 :  3303,  62 :  3270,  },
        55  : {  47 :  3591,  63 :  3505,  },
        56  : {  48 :  4018,  64 :  3943,  },
        57  : {  49 :  4693,  65 :  4553,  },
        58  : {  50 :  4693,  66 :  4552,  },
        59  : {  51 :  4157,  67 :  4049,  },
        60  : {  52 :  3736,  68 :  3675,  },
        61  : {  53 :  3210,  69 :  3123,  },
        62  : {  54 :  3328,  70 :  3272,  },
        63  : {  55 :  3586,  71 :  3508,  },
        64  : {  56 :  3881,  72 :  3765,  },
        65  : {  57 :  4342,  73 :  4135,  },
        66  : {  58 :  4193,  74 :  3972,  },
        67  : {  59 :  4224,  75 :  4052,  },
        68  : {  60 :  3505,  76 :  3414,  },
        69  : {  61 :  3159,  77 :  3073,  },
        70  : {  62 :  2849, 102 :  2131,  },
        71  : {  63 :  3346,  78 :  3253,  },
        72  : {  64 :  4016,  79 :  3818,  },
        73  : {  65 :  4245,  80 :  4019,  },
        74  : {  66 :  3915,  81 :  3705,  },
        75  : {  67 :  3719,  82 :  3533,  },
        76  : {  68 :  3471,  83 :  3340,  },
        77  : {  69 :  3010, 103 :  1979,  },
        78  : {  71 :  3330,  84 :  3252,  },
        79  : {  72 :  3917,  85 :  3732,  },
        80  : {  73 :  3611,  86 :  3386,  },
        81  : {  74 :  3943,  87 :  3701,  },
        82  : {  75 :  3457,  88 :  3283,  },
        83  : {  76 :  3381,  89 :  3259,  },
        84  : {  78 :  3180,  90 :  3029,  },
        85  : {  79 :  3637,  91 :  3476,  },
        86  : {  80 :  3742,  92 :  3510,  },
        87  : {  81 :  3743,  93 :  3520,  },
        88  : {  82 :  3521,  94 :  3352,  },
        89  : {  83 :  3299,  95 :  3101,  },
        90  : {  84 :  2964, 108 :  1896,  },
        91  : {  85 :  3359,  96 :  3186,  },
        92  : {  86 :  3268,  97 :  3084,  },
        93  : {  87 :  3331,  98 :  3139,  },
        94  : {  88 :  3350,  99 :  3207,  },
        95  : {  89 :  2899, 109 :  1461,  },
        96  : {  91 :  3348, 110 :  2972,  },
        97  : { None:None,  92 :  3354,  },
        98  : { None:None,  93 :  3200,  },
        99  : {  94 :  2915, 111 :  2502,  },
        100 : { None:None,  22 :  3103,  },
        101 : { None:None,  29 :  3010,  },
        102 : { None:None,  70 :  3165,  },
        103 : { None:None,  77 :  2879,  },
        104 : { None:None,   0 :  3017,  },
        105 : { None:None,   3 :  2861,  },
        106 : { None:None,   4 :  1862,  },
        107 : { None:None,   9 :  1719,  },
        108 : { None:None,  90 :  2023,  },
        109 : { None:None,  95 :  1583,  },
        110 : { None:None,  96 :  3111,  },
        111 : { None:None,  99 :  2435,  },
        },
          'NB0921' : {                   # from 904744
        0   : {   5 : 44175, 104 : 39568,  },
        1   : { None:None,   6 : 44176,  },
        2   : { None:None,   7 : 44435,  },
        3   : {   8 : 38523, 105 : 34166,  },
        4   : {  10 : 41107, 106 : 24782,  },
        5   : {   0 : 42054,  11 : 44168,  },
        6   : {   1 : 42468,  12 : 44961,  },
        7   : {   2 : 40421,  13 : 43001,  },
        8   : {   3 : 42604,  14 : 44456,  },
        9   : {  15 : 37050, 107 : 21248,  },
        10  : {   4 : 42261,  16 : 44322,  },
        11  : {   5 : 42867,  17 : 45287,  },
        12  : {   6 : 47791,  18 : 51816,  },
        13  : {   7 : 43250,  19 : 46474,  },
        14  : {   8 : 43337,  20 : 45579,  },
        15  : {   9 : 35970,  21 : 37997,  },
        16  : {  10 : 41161,  23 : 42462,  },
        17  : {  11 : 47239,  24 : 50503,  },
        18  : {  12 : 47165,  25 : 51506,  },
        19  : {  13 : 47366,  26 : 51330,  },
        20  : {  14 : 43026,  27 : 45937,  },
        21  : {  15 : 37398,  28 : 38467,  },
        22  : {  30 : 41603, 100 : 31698,  },
        23  : {  16 : 46994,  31 : 48182,  },
        24  : {  17 : 48867,  32 : 51809,  },
        25  : {  18 : 55329,  33 : 58436,  },
        26  : {  19 : 50657,  34 : 53525,  },
        27  : {  20 : 47151,  35 : 49727,  },
        28  : {  21 : 41162,  36 : 42060,  },
        29  : {  37 : 34650, 101 : 23522,  },
        30  : {  22 : 42216,  38 : 42793,  },
        31  : {  23 : 46105,  39 : 47027,  },
        32  : {  24 : 51882,  40 : 53554,  },
        33  : {  25 : 59153,  41 : 60267,  },
        34  : {  26 : 52432,  42 : 54717,  },
        35  : {  27 : 50803,  43 : 52617,  },
        36  : {  28 : 42618,  44 : 43368,  },
        37  : {  29 : 35930,  45 : 36312,  },
        38  : {  30 : 42346,  46 : 42473,  },
        39  : {  31 : 48993,  47 : 49285,  },
        40  : {  32 : 57189,  48 : 57663,  },
        41  : {  33 : 58935,  49 : 60081,  },
        42  : {  34 : 55351,  50 : 56513,  },
        43  : {  35 : 49825,  51 : 50277,  },
        44  : {  36 : 41480,  52 : 41831,  },
        45  : {  37 : 33440,  53 : 33749,  },
        46  : {  38 : 42128,  47 : 44565,  54 : 41764,  },
        47  : {  39 : 45029,  46 : 41919,  48 : 49062,  55 : 44768,  },
        48  : {  40 : 54200,  47 : 50737,  49 : 56865,  56 : 53612,  },
        49  : {  41 : 60240,  48 : 58766,  50 : 61100,  57 : 59962,  },
        50  : {  42 : 60989,  49 : 62860,  51 : 58281,  58 : 60613,  },
        51  : {  43 : 54759,  50 : 58944,  52 : 50291,  59 : 54244,  },
        52  : {  44 : 40221,  51 : 44179,  53 : 36999,  60 : 39894,  },
        53  : {  45 : 36946,  52 : 39642,  61 : 36904,  },
        54  : {  46 : 41538,  62 : 40911,  },
        55  : {  47 : 46347,  63 : 45105,  },
        56  : {  48 : 53583,  64 : 52093,  },
        57  : {  49 : 61563,  65 : 59054,  },
        58  : {  50 : 60850,  66 : 58484,  },
        59  : {  51 : 51706,  67 : 50218,  },
        60  : {  52 : 43052,  68 : 42093,  },
        61  : {  53 : 36323,  69 : 35353,  },
        62  : {  54 : 42264,  70 : 41095,  },
        63  : {  55 : 45030,  71 : 43663,  },
        64  : {  56 : 50373,  72 : 47886,  },
        65  : {  57 : 55846,  73 : 52604,  },
        66  : {  58 : 53329,  74 : 50195,  },
        67  : {  59 : 52315,  75 : 49828,  },
        68  : {  60 : 40029,  76 : 38833,  },
        69  : {  61 : 34790,  77 : 33752,  },
        70  : {  62 : 35842, 102 : 26796,  },
        71  : {  63 : 41872,  78 : 40244,  },
        72  : {  64 : 52113,  79 : 48453,  },
        73  : {  65 : 53972,  80 : 50184,  },
        74  : {  66 : 49949,  81 : 46239,  },
        75  : {  67 : 45559,  82 : 42406,  },
        76  : {  68 : 40028,  83 : 38342,  },
        77  : {  69 : 33579, 103 : 21940,  },
        78  : {  71 : 41453,  84 : 39694,  },
        79  : {  72 : 47969,  85 : 44351,  },
        80  : {  73 : 45403,  86 : 41106,  },
        81  : {  74 : 49361,  87 : 44675,  },
        82  : {  75 : 41263,  88 : 38258,  },
        83  : {  76 : 38596,  89 : 36936,  },
        84  : {  78 : 39170,  90 : 36792,  },
        85  : {  79 : 43551,  91 : 40758,  },
        86  : {  80 : 45083,  92 : 41078,  },
        87  : {  81 : 44784,  93 : 41043,  },
        88  : {  82 : 40476,  94 : 37976,  },
        89  : {  83 : 37419,  95 : 34714,  },
        90  : {  84 : 35870, 108 : 22674,  },
        91  : {  85 : 39985,  96 : 37534,  },
        92  : {  86 : 38224,  97 : 35598,  },
        93  : {  87 : 38777,  98 : 36127,  },
        94  : {  88 : 38010,  99 : 36200,  },
        95  : {  89 : 31889, 109 : 15929,  },
        96  : {  91 : 39479, 110 : 34518,  },
        97  : { None:None,  92 : 39009,  },
        98  : { None:None,  93 : 36328,  },
        99  : {  94 : 33089, 111 : 27992,  },
        100 : { None:None,  22 : 41392,  },
        101 : { None:None,  29 : 34599,  },
        102 : { None:None,  70 : 39045,  },
        103 : { None:None,  77 : 31737,  },
        104 : { None:None,   0 : 41356,  },
        105 : { None:None,   3 : 36258,  },
        106 : { None:None,   4 : 25607,  },
        107 : { None:None,   9 : 21133,  },
        108 : { None:None,  90 : 24121,  },
        109 : { None:None,  95 : 17348,  },
        110 : { None:None,  96 : 35760,  },
        111 : { None:None,  99 : 26673,  },
        },
          }

def correctQE(serial, filter='g', calculate=False, visit=0, butler=None, canonical=None):
    if canonical is None:
        levs = np.array(levels.values())
        avg = np.median(levs[np.where(levs > 0.5*np.median(levs))])
    else:
        avg = levels[canonical]

    lev = levels[serial]
    correction = avg/float(lev)
    if calculate:
        assert butler

        cam = butler.get("camera")

        raw = butler.get("raw", visit=visit, ccd=serial)
        ccd = afwCG.cast_Ccd(raw.getDetector())
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
                ds9Utils.drawBBox(im.getBBox(afwImage.PARENT), borderWidth=0.4)
                print serial, serial1, offset, val

            print "%3d : %5d," % (serial1, val),
        print " },"

        return
    #
    # Apply correction factors
    #
    fiddle = True
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
                QE[ccd][f] = correctQE(ccd, f, canonical=canonical)
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
            line.append("%4s" % (f))

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
        904520 : "Dark dome",
        904534 : "Dark dome",
        904536 : "Dark dome",
        904538 : "Dark dome",
        904670 : "Dark dome",
        904672 : "Dark dome",
        904674 : "Dark dome",
        904676 : "Dark dome",
        904678 : "Dark dome",
        904786 : "Dark dome",
        904788 : "Dark dome",
        904790 : "Dark dome",
        904792 : "Dark dome",
        904794 : "Dark dome",
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
        
def isrCallback(im, ccd=None, butler=None, imageSource=None, correctGain=True):
    """Trim and linearize a raw CCD image, and maybe convert from DN to e^-, using ip_isr
    """
    from lsst.obs.subaru.isr import SubaruIsrTask as IsrTask

    isrTask = IsrTask()
    isrTask.log.setThreshold(100)

    for k in isrTask.config.qa.keys():
        if re.search(r"^do", k):
            setattr(isrTask.config.qa, k, False)

    isrTask.config.doBias = False
    isrTask.config.doDark = False
    isrTask.config.doFlat = False
    isrTask.config.doLinearize = True
    isrTask.config.doApplyGains = correctGain
    isrTask.config.doGuider = False
    isrTask.config.doWrite = False
    
    isMI = hasattr(im, 'getImage')
    if not isMI:
        im = afwImage.makeMaskedImage(im)

    raw = afwImage.makeExposure(im)
    raw.setDetector(ccd)
    raw.setMetadata(dafBase.PropertySet())
    dataRef = DataRef(ccd.getId().getSerial(), raw=raw)

    result = isrTask.run(dataRef)

    mi = result.exposure.getMaskedImage()
    return mi if isMI else mi.getImage()
