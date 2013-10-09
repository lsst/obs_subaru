import math
import os
import re
import sys
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
import numpy as np

import lsst.afw.cameraGeom as afwCG
import lsst.afw.cameraGeom.utils as afwCGUtils
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.display.ds9 as ds9
import lsst.afw.display.utils as ds9Utils

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
                print "         %s %.1f %.1f %.1f" % (amp.getId(),
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
    sctrl.setNumSigmaClip(5)
    sctrl.setAndMask(BAD)

    ccds, ampData = {}, {}
    for v in visits:
        exp = butler.get("raw", visit=v, ccd=ccdNo)

        ccd = afwCG.cast_Ccd(exp.getDetector())
        im = afwCGUtils.trimRawCallback(exp.getMaskedImage().convertF(), ccd)
        im.getMask()[:] |= flatMask
        
        ampData[ccd] = []
        for a in ccd:
            aim = im[a.getAllPixels()]
            aim /= a.getElectronicParams().getGain() # we corrected for the nominal gain

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
        for v in visits[0:1]:
            exp = butler.get("raw", visit=v, ccd=ccdNo)
            
            ccd = afwCG.cast_Ccd(exp.getDetector())
            im = afwCGUtils.trimRawCallback(exp.getMaskedImage().convertF(), ccd)

            ds9.mtv(im, frame=frame, title=ccdNo)

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
    """Write a Raft's ElectronicParams to the Electronics section of a camera.paf file"""
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
    has radius r2.  The separation of the two centres in d

    If analytic, use the exact formula otherwise simulate "rays"

    If D is non-None it's the distance to an edge that occults the primary, making an angle
    phi with the line joining the primary and the centre of the occulting disk
    """
    if analytic:
        theta1 = np.arccos((r1**2 + d**2 - r2**2)/(2*r1*d)) # 1/2 angle subtended by chord where
        theta2 = np.arccos((r2**2 + d**2 - r1**2)/(2*r2*d)) #    pupil and vignetting disk cross
        
        missing = 0.5*(r1**2*(2*theta1 - np.sin(2*theta1)) + r2**2*(2*theta2 - np.sin(2*theta2)))
        return np.where(d <= r2 - r1, 1.0, missing/np.pi*r1**2)
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

def removeCRs(mi, background, maskBit=None, crThreshold=8, crNpixMax=100, crNGrow=1):
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
        
    removeCRs(mi, background, maskBit=CR)
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


