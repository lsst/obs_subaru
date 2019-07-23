#
# LSST Data Management System
# Copyright 2008-2016 AURA/LSST.
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
"""
Determine and apply crosstalk corrections

N.b. This code was written and tested for the 4-amplifier Hamamatsu chips used in (Hyper)?SuprimeCam,
and will need to be generalised to handle other amplifier layouts.  I don't want to do this until we
have an example.

N.b. To estimate crosstalk from the SuprimeCam data, the commands are e.g.:
import crosstalk
coeffs, coeffsErr = crosstalk.estimateCoeffs(butler, range(131634, 131642), range(10), threshold=1e5,
                                             plot=True, title="CCD0..9", fig=1)
crosstalk.fixCcd(butler, 131634, 0, coeffs)
"""
import sys
import math
import time

import numpy as np

import lsst.afw.detection as afwDetect
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.geom as geom
import lsst.pex.config as pexConfig
import lsst.afw.display as afwDisplay
from functools import reduce
from lsst.ip.isr.crosstalk import CrosstalkTask


class CrosstalkCoeffsConfig(pexConfig.Config):
    """Specify crosstalk coefficients for a CCD"""

    values = pexConfig.ListField(
        dtype=float,
        doc="Crosstalk coefficients",
        default=[0, 0, 0, 0,
                 0, 0, 0, 0,
                 0, 0, 0, 0,
                 0, 0, 0, 0],
    )
    shape = pexConfig.ListField(
        dtype=int,
        doc="Shape of coeffs array",
        default=[4, 4],
        minLength=1,                  # really 2, but there's a bug in pex_config
        maxLength=2,
    )

    def getCoeffs(self):
        """Return a 2-D numpy array of crosstalk coefficients of the proper shape"""
        return np.array(self.values).reshape(self.shape)


class SubaruCrosstalkConfig(CrosstalkTask.ConfigClass):
    minPixelToMask = pexConfig.Field(dtype=float, default=45000,
                                     doc="Set crosstalk mask plane for pixels over this value")
    crosstalkMaskPlane = pexConfig.Field(dtype=str, default="CROSSTALK", doc="Name for crosstalk mask plane")
    coeffs = pexConfig.ConfigField(dtype=CrosstalkCoeffsConfig, doc="Crosstalk coefficients")


class SubaruCrosstalkTask(CrosstalkTask):
    ConfigClass = SubaruCrosstalkConfig

    def run(self, exp, **kwargs):
        self.log.info("Applying crosstalk correction/Subaru")
        subtractXTalk(exp.getMaskedImage(), self.config.coeffs.getCoeffs(), self.config.minPixelToMask,
                      self.config.crosstalkMaskPlane)


nAmp = 4


def getXPos(width, hwidth, x):
    """Return the amp that x is in, and the positions of its image in each amplifier"""
    amp = x//(hwidth//2)                # which amp am I in?  Assumes nAmp == 4
    assert nAmp == 4
    assert amp in range(nAmp)

    if amp == 0:
        xa = x                          # distance to amp
        xs = hwidth - x - 1             # symmetrical position within this half of the chip
        xx = (x, xs, hwidth + xa, hwidth + xs)
    elif amp == 1:
        xa = hwidth - x - 1             # distance to amp
        xs = hwidth - x                 # symmetrical position within this half of the chip
        xx = (xs - 1, x, hwidth + xs - 1, hwidth + x)
    elif amp == 2:
        xa = x - hwidth                 # distance to amp
        xs = width - x                  # symmetrical position within this half of the chip
        xx = (xa, width - x - 1, x, width - xa - 1)
    elif amp == 3:
        xa = x - hwidth                 # distance to amp
        xs = width - x                  # symmetrical position within this half of the chip
        xx = (width - x - 1, xa, width - xa - 1, x)

    return amp, xx


def getAmplitudeRatios(mi, threshold=45000, bkgd=None, rats=None):
    img = mi.getImage()
    msk = mi.getMask()
    width = mi.getWidth()
    hwidth = width//2

    if rats is None:
        rats = []
        for i in range(nAmp):
            rats.append([])
            for j in range(nAmp):
                rats[i].append([])
            rats[i][i].append(0)

    fs = afwDetect.FootprintSet(mi, afwDetect.Threshold(threshold), "DETECTED")

    if bkgd is None:
        sctrl = afwMath.StatisticsControl()
        sctrl.setAndMask(msk.getPlaneBitMask("DETECTED"))

        bkgd = afwMath.makeStatistics(mi, afwMath.MEDIAN, sctrl).getValue()

    badMask = msk.getPlaneBitMask(["BAD", "EDGE", "SAT", "INTRP"])

    for foot in fs.getFootprints():
        for s in foot.getSpans():
            y, x0, x1 = s.getY(), s.getX0(), s.getX1()
            for x in range(x0, x1):
                val = img.get(x, y)
                amp, xx = getXPos(width, hwidth, x)
                for a, _x in enumerate(xx):
                    if a != amp:
                        if msk.get(_x, y) & badMask:
                            continue
                        if False:
                            foo = (img.get(_x, y) - bkgd)/val
                            if np.abs(foo) < 1e-5:
                                print(img.get(_x, y) - bkgd, val)
                                mi.getMask().set(_x, y, 0x100)

                        rats[amp][a].append((img.get(_x, y) - bkgd)/val)

    return rats


def calculateCoeffs(rats, nsigma, plot=False, fig=None, title=None):
    """Calculate cross-talk coefficients"""
    coeffs = np.empty((nAmp, nAmp))
    coeffsErr = np.empty_like(coeffs)

    if plot:
        if fig is None:
            fig = int(title[-1]) + 1 if title else 1

        fig = getMpFigure(fig, clear=True)
        subplots = makeSubplots(fig, nAmp, nAmp)

        rMin = 2e-3
        bins = np.arange(-rMin, rMin, 0.05*rMin)

        xMajorLocator = ticker.MaxNLocator(nbins=3)  # steps=(-rMin/2, 0, rMin/2))

    for ain in range(nAmp):
        for aout in range(nAmp):
            tmp = np.array(rats[ain][aout])
            tmp.sort()
            for i in range(3):
                n = len(tmp)
                med = tmp[int(0.5*n)]
                sigma = 0.741*(tmp[int(0.75*n)] - tmp[int(0.25*n)])
                w = np.where(abs(tmp - med) < nsigma*sigma)
                if not np.any(w):
                    break
                tmp = tmp[w]

            coeffs[ain][aout] = tmp[len(tmp)//2]

            err = 0.741*(tmp[3*len(tmp)//4] - tmp[len(tmp)//4])  # estimate s.d. from IQR
            err /= math.sqrt(len(tmp))                           # standard error of mean
            err *= math.sqrt(math.pi/2)                          # standard error of median
            coeffsErr[ain][aout] = err

            if plot:
                axes = next(subplots)
                axes.xaxis.set_major_locator(xMajorLocator)

                if ain != aout:
                    hist = np.histogram(rats[ain][aout], bins)[0]
                    axes.bar(bins[0:-1], hist, width=bins[1]-bins[0], color="red", linewidth=0, alpha=0.8)

                axes.axvline(0, linestyle="--", color="green")
                axes.axvline(coeffs[ain][aout], linestyle="-", color="blue")
                for i in (-1, 1):
                    axes.axvline(coeffs[ain][aout] + i*coeffsErr[ain][aout], linestyle=":", color="cyan")
                axes.text(-0.9*rMin, 0.8*axes.get_ylim()[1], r"%.1e" % coeffs[ain][aout], fontsize="smaller")
                axes.set_xlim(-1.05*rMin, 1.05*rMin)

    if plot:
        if title:
            fig.suptitle(title)
        fig.show()

    return coeffs, coeffsErr


def subtractXTalk(mi, coeffs, minPixelToMask=45000, crosstalkStr="CROSSTALK"):
    """Subtract the crosstalk from MaskedImage mi given a set of coefficients

    The pixels affected by signal over minPixelToMask have the crosstalkStr bit set
    """
    sctrl = afwMath.StatisticsControl()
    sctrl.setAndMask(mi.getMask().getPlaneBitMask("BAD"))
    bkgd = afwMath.makeStatistics(mi, afwMath.MEDIAN, sctrl).getValue()
    #
    # These are the pixels that are bright enough to cause crosstalk (more precisely,
    # the ones that we label as causing crosstalk; in reality all pixels cause crosstalk)
    #
    tempStr = "TEMP"                    # mask plane used to record the bright pixels that we need to mask
    msk = mi.getMask()
    msk.addMaskPlane(tempStr)
    try:
        fs = afwDetect.FootprintSet(mi, afwDetect.Threshold(minPixelToMask), tempStr)

        mi.getMask().addMaskPlane(crosstalkStr)
        afwDisplay.getDisplay().setMaskPlaneColor(crosstalkStr, afwDisplay.MAGENTA)
        # the crosstalkStr bit will now be set whenever we subtract crosstalk
        fs.setMask(mi.getMask(), crosstalkStr)
        crosstalk = mi.getMask().getPlaneBitMask(crosstalkStr)

        width, height = mi.getDimensions()
        for i in range(nAmp):
            bbox = geom.BoxI(geom.PointI(i*(width//nAmp), 0), geom.ExtentI(width//nAmp, height))
            ampI = mi.Factory(mi, bbox)
            for j in range(nAmp):
                if i == j:
                    continue

                bbox = geom.BoxI(geom.PointI(j*(width//nAmp), 0), geom.ExtentI(width//nAmp, height))
                if (i + j)%2 == 1:
                    ampJ = afwMath.flipImage(mi.Factory(mi, bbox), True, False)  # no need for a deep copy
                else:
                    ampJ = mi.Factory(mi, bbox, afwImage.LOCAL, True)

                msk = ampJ.getMask()
                if np.all(msk.getArray() & msk.getPlaneBitMask("BAD")):
                    # Bad amplifier; ignore it completely --- its effect will come out in the bias
                    continue
                msk &= crosstalk

                ampJ -= bkgd
                ampJ *= coeffs[j][i]

                ampI -= ampJ
        #
        # Clear the crosstalkStr bit in the original bright pixels, where tempStr is set
        #
        msk = mi.getMask()
        temp = msk.getPlaneBitMask(tempStr)
        xtalk_temp = crosstalk | temp
        np_msk = msk.getArray()
        mask_indicies = np.where(np.bitwise_and(np_msk, xtalk_temp) == xtalk_temp)
        np_msk[mask_indicies] &= getattr(np, np_msk.dtype.name)(~crosstalk)

    finally:
        msk.removeAndClearMaskPlane(tempStr, True)  # added in afw #1853


def printCoeffs(coeffs, coeffsErr=None, LaTeX=False, ppm=False):
    """Print cross-talk coefficients"""

    if LaTeX:
        print(r"""\begin{tabular}{l|*{4}{l}}
ampIn    &    \multicolumn{4}{c}{ampOut} \\
     &    0              &    1              &     2             &    3           \\
\hline""")
        for ain in range(nAmp):
            msg = "%-4d " % ain
            for aout in range(nAmp):
                if ppm:
                    msg += "&         " if ain == aout else "&   %5.0f " % (1e6*coeffs[ain][aout])
                else:
                    msg += "&   %9.2e " % (coeffs[ain][aout])
                if coeffsErr is not None:
                    if ppm:
                        if ain != aout:
                            val = int(1e6*coeffsErr[ain][aout] + 0.5)
                            msg += r"$\pm$ %s%d " % (r"$\phantom{0}$" if val < 10 else "", val)
                    else:
                        msg += r"$\pm$ %7.1e " % (coeffsErr[ain][aout])
            print(msg + r" \\")
        print(r"\end{tabular}")

        return

    print("ampIn                   ", end=' ')
    if coeffsErr is not None:
        print("                         ", end=' ')
    print("ampOut")

    msg = "%-4s " % ""
    for aout in range(nAmp):
        msg += "       %d    " % aout
        if coeffsErr is not None:
            msg += "%11s" % ""
    print(msg)
    for ain in range(nAmp):
        msg = "%-4d " % ain
        for aout in range(nAmp):
            msg += "   %9.2e" % coeffs[ain][aout]
            if coeffsErr is not None:
                msg += " +- %7.1e" % coeffsErr[ain][aout]

        print(msg)


#
# Code to simulate crosstalk
#
xTalkAmplitudes = np.array([(0, -1.0e-4, -2.0e-4, -3.0e-4),  # cross talk from amp0 to amp1, 2, 3
                            (-1.5e-4, 0, -2.5e-4, -2.9e-4),
                            (-2.2e-4, -3.1e-4, 0, -0.9e-4),
                            (-2.7e-4, -3.3e-4, -1.9e-4, 0)])  # ... from amp 3


def addTrail(mi, val, x0, y0, pix, addCrosstalk=True):
    width = mi.getWidth()
    hwidth = width//2

    if addCrosstalk:
        xtalk = mi.Factory(mi.getDimensions())

    SAT = reduce(lambda x, y: x | afwImage.Mask.getPlaneBitMask(y), ["SAT", "INTRP"], 0x0) if False else 0

    for _y, _x12 in enumerate(pix):
        for _x in range(*_x12):
            x, y = x0 + _x, y0 + _y
            mi.set(x, y, (val, SAT,))

            if addCrosstalk:
                amp, xx = getXPos(width, hwidth, x)
                for i, x in enumerate(xx):
                    xtalk.set(x, y, (xTalkAmplitudes[amp][i]*val, ))

    mi += xtalk


def addSaturated(mi, addCrosstalk=True):
    trail1 = 6*[(0, 2)] + 4*[(-1, 3)] + 4*[(-2, 4)] + 3*[(-1, 3)] + 4*[(0, 2)]
    trail2 = 12*[(0, 2)] + 8*[(-1, 3)] + 4*[(-2, 4)] + 4*[(-3, 6)] + 3*[(-2, 5)] + 3*[(-1, 3)] + 10*[(0, 2)]

    addTrail(mi, 48000, 300, 350, trail1, addCrosstalk)
    addTrail(mi, 50000, 100, 450, trail1, addCrosstalk)
    addTrail(mi, 60000, 50, 550, trail1, addCrosstalk)
    addTrail(mi, 52000, 450, 650, trail1, addCrosstalk)

    addTrail(mi, 60000, 100, 300, trail2, addCrosstalk)
    addTrail(mi, 50000, 200, 400, trail2, addCrosstalk)
    addTrail(mi, 46000, 300, 500, trail2, addCrosstalk)
    addTrail(mi, 48000, 400, 600, trail2, addCrosstalk)


def makeImage(width=500, height=1000):
    mi = afwImage.MaskedImageF(width, height)
    var = 50
    mi.set(1000, 0x0, var)

    addSaturated(mi, addCrosstalk=True)

    ralg, rseed = "MT19937", int(time.time()) if True else 1234

    noise = afwImage.ImageF(width, height)
    afwMath.randomGaussianImage(noise, afwMath.Random(ralg, rseed))
    noise *= math.sqrt(var)
    mi += noise

    return mi


def readImage(butler, **kwargs):
    try:
        return butler.get("calexp", **kwargs).getMaskedImage()
    except Exception as e:
        print(e)
        import pdb
        pdb.set_trace()


def makeList(x):
    try:
        x[0]
        return x
    except TypeError:
        return [x]


def estimateCoeffs(butler, visitList, ccdList, threshold=45000, nSample=1, plot=False, fig=None, title=None):
    rats = None
    for v in visitList:
        for ccd in ccdList:
            if ccd == "simulated":
                mi = makeImage()
            else:
                mi = readImage(butler, visit=v, ccd=ccd)

            rats = getAmplitudeRatios(mi, threshold, rats=rats)

    return calculateCoeffs(rats, nsigma=2, plot=plot, title=title, fig=fig)


def main(butler, visit=131634, ccd=None, threshold=45000, nSample=1, showCoeffs=True, fixXTalk=True,
         plot=False, title=None):
    if ccd is None:
        visitList = list(range(nSample))
        ccdList = ["simulated", ]
    else:
        ccdList = makeList(ccd)
        visitList = makeList(visit)

    coeffs, coeffsErr = estimateCoeffs(butler, visitList, ccdList, threshold=45000, plot=plot, title=title)

    if showCoeffs:
        printCoeffs(coeffs, coeffsErr)

    mi = readImage(butler, visit=visitList[0], ccd=ccdList[0])
    if fixXTalk:
        subtractXTalk(mi, coeffs, threshold)

    return mi, coeffs


try:
    import matplotlib.ticker as ticker
    import matplotlib.pyplot as pyplot
except ImportError:
    pyplot = None
try:
    mpFigures
except NameError:
    mpFigures = {0: None}              # matplotlib (actually pyplot) figures


def makeSubplots(figure, nx=2, ny=2):
    """Return a generator of a set of subplots"""
    for window in range(nx*ny):
        yield figure.add_subplot(nx, ny, window + 1)  # 1-indexed


def getMpFigure(fig=None, clear=True):
    """Return a pyplot figure(); if fig is supplied save it and make it the default
    fig may also be a bool (make a new figure) or an int (return or make a figure (1-indexed;
    python-list style -n supported)
    """

    if not pyplot:
        raise RuntimeError("I am unable to plot as I failed to import matplotlib")

    if isinstance(fig, bool):       # we want a new one
        fig = len(mpFigures) + 1    # matplotlib is 1-indexed

    if isinstance(fig, int):
        i = fig
        if i == 0:
            raise RuntimeError("I'm sorry, but matplotlib uses 1-indexed figures")
        if i < 0:
            try:
                i = sorted(mpFigures.keys())[i]  # simulate list's [-n] syntax
            except IndexError:
                if mpFigures:
                    print("Illegal index: %d" % i, file=sys.stderr)
                i = 1

        def lift(fig):
            fig.canvas._tkcanvas._root().lift()  # == Tk's raise, but raise is a python reserved word

        if i in mpFigures:
            try:
                lift(mpFigures[i])
            except Exception:
                del mpFigures[i]

        if i not in mpFigures:
            for j in range(1, i):
                getMpFigure(j)

            mpFigures[i] = pyplot.figure()
            #
            # Modify pyplot.figure().show() to make it raise the plot too
            #

            def show(self, _show=mpFigures[i].show):
                _show(self)
                try:
                    lift(self)
                except Exception:
                    pass
            # create a bound method
            import types
            mpFigures[i].show = types.MethodType(show, mpFigures[i], mpFigures[i].__class__)

        fig = mpFigures[i]

    if not fig:
        i = sorted(mpFigures.keys())[0]
        if i > 0:
            fig = mpFigures[i[-1]]
        else:
            fig = getMpFigure(1)

    if clear:
        fig.clf()

    pyplot.figure(fig.number)           # make it active

    return fig


def fixCcd(butler, visit, ccd, coeffs, display=True):
    """Apply cross-talk correction to a CCD, given the cross-talk coefficients"""
    mi = readImage(butler, visit=visit, ccd=ccd)
    if display:
        afwDisplay.getDisplay(frame=1).mtv(mi.getImage(), title="CCD %d" % ccd)

    subtractXTalk(mi, coeffs)

    if display:
        afwDisplay.getDisplay(frame=2).mtv(mi, title="corrected %d" % ccd)


if __name__ == "__main__":
    main()
