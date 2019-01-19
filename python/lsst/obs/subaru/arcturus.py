"""Process the scattered light data from Arcturus"""

import numpy as np
import multiprocessing
try:
    pyplot
except NameError:
    import matplotlib.pyplot as pyplot
    pyplot.interactive(1)

import lsst.afw.display as afwDisplay
import lsst.analysis.utils as utils

position = {                            # position of Arcturus on 16x16 binned "showCamera" images
    906142: (8, 8),
    906144: (8, 116),
    906146: (6, 228),
    906148: (5, 341),
    906150: (5, 455),
    906152: (8, 566),
    906154: (9, 682),
    906156: (8, 798),
    906158: (8, 917),
    906160: (8, 1039),
    906162: (8, 1038 + 108),
    906164: (8, 1038 + 2*108),
    906186: (8, 1038 + 3*108),
    906188: (8, 1038 + 4*108),
    906190: (8, 1038 + 5*108),
}


def showFrames(mos, frame0=1, R=23, subtractSky=True):
    visits = sorted(position.keys())

    frame = frame0 - 1
    for v in visits:
        frame += 1

        xc, yc = position[v]
        xc -= mos[v].getX0()
        yc -= mos[v].getY0()

        im = mos[v]
        if subtractSky:
            im = im.clone()
            im[2121:2230, 590:830] = np.nan  # QE for this chip is bad

            ima = im.getArray()
            im[:] -= np.percentile(ima, 25)

        disp = afwDisplay.Display(frame=frame)
        disp.mtv(im, title=v)
        disp.dot("o", xc, yc, size=R,
                 ctype=afwDisplay.GREEN if yc < mos[v].getHeight() else afwDisplay.RED)


class MedianFilterImageWorker(object):

    def __init__(self, mos, medianN):
        self.mos = mos
        self.medianN = medianN

    def __call__(self, v):
        print("Median filtering visit %d" % v)
        return v, utils.medianFilterImage(self.mos[-v], self.medianN)


def prepareFrames(mos, frame0=1, R=100, medianN=23, onlyVisits=[], nJob=1, force=False):
    """Prepare frames to have their radial profiles measured.
Subtract the first quartile
Set a radius R circle about the centre of Arcturus to NaN
Set a chip with a bad QE value to NaN
Median filter with a medianN x medianN filter

The results are set as mos[-visit]

If onlyVisits is specified, only process those chips [n.b. frame0 is still obeyed]; otherwise
process every visit in the positions dict
    """
    visits = sorted(position.keys())

    visits0 = visits[:]                 # needed to keep frame numbers aligned
    if not force:
        visits = [v for v in visits if -v not in mos]  # don't process ones we've already seen
    if onlyVisits:
        visits = [v for v in visits if v in onlyVisits]

    width, height = mos[visits[0]].getDimensions()
    X, Y = np.meshgrid(np.arange(width), np.arange(height))

    for v in visits:
        im = mos[v].clone()
        mos[-v] = im
        im[2121:2230, 590:830] = np.nan  # QE for this chip is bad

        ima = im.getArray()
        im[:] -= np.percentile(ima, 25)

        xc, yc = position[v]
        xc -= im.getX0()
        yc -= im.getY0()

        ima[np.hypot(X - xc, Y - yc) < R] = np.nan
    #
    # multiprocessing
    #
    worker = MedianFilterImageWorker(mos, medianN)
    if nJob > 1:
        pool = multiprocessing.Pool(max([len(visits), nJob]))

        # We use map_async(...).get(9999) instead of map(...) to workaround a python bug
        # in handling ^C in subprocesses (http://bugs.python.org/issue8296)
        for v, im in pool.map_async(worker, visits).get(9999):
            mos[-v] = im

        pool.close()
        pool.join()
    else:
        for v in visits:
            mos[-v] = worker(v)
    #
    # Display
    #
    frame = frame0 - 1
    for v in visits0:
        frame += 1

        if v not in visits:
            continue

        im = mos[-v]

        disp = afwDisplay.Display(frame=frame)
        if True:
            disp.mtv(im, title=v)
        else:
            disp.erase()

        xc, yc = position[v]
        xc -= im.getX0()
        yc -= im.getY0()

        disp.dot("o", xc, yc, size=R, ctype=afwDisplay.GREEN if yc < im.getHeight() else afwDisplay.RED)


def OLDprepareFrames(mos, frame0=1, R=23, medianN=23, onlyVisits=[], force=False):
    """Prepare frames to have their radial profiles measured.
Subtract the first quartile
Set a radius R circle about the centre of Arcturus to NaN
Set a chip with a bad QE value to NaN
Median filter with a medianN x medianN filter

The result is set as mos[-visit]

If onlyVisits is specified, only process those chips [n.b. frame0 is still obeyed]
    """

    visits = sorted(position.keys())

    width, height = mos[visits[0]].getDimensions()

    X, Y = np.meshgrid(np.arange(width), np.arange(height))

    frame = frame0 - 1
    for v in visits:
        frame += 1
        if onlyVisits and v not in onlyVisits:
            continue

        print("Processing %d" % v)

        im = mos[v].clone()
        im[2121:2230, 590:830] = np.nan  # QE for this chip is bad

        ima = im.getArray()
        im[:] -= np.percentile(ima, 25)

        xc, yc = position[v]
        xc -= im.getX0()
        yc -= im.getY0()

        ima[np.hypot(X - xc, Y - yc) < R] = np.nan

        if force or -v not in mos:
            im = utils.medianFilterImage(im, medianN)
            mos[-v] = im
        else:
            im = mos[-v]

        disp = afwDisplay.Display(frame=frame)
        if True:
            disp.mtv(im, title=v)
        else:
            disp.erase()
        disp.dot("o", xc, yc, size=R, ctype=afwDisplay.GREEN if yc < im.getHeight() else afwDisplay.RED)


def annularAverage(im, nBin=100, rmin=None, rmax=None, median=False):
    """Return image im's radial profile (more accurately the tuple (rbar, prof), binned into nBin bins

r is measured from (0, 0) in the image, accounting properly for XY0

If rmax is provided it's the maximum value of r to consider
    """

    width, height = im.getDimensions()

    if not rmax:
        rmax = 0.5*min(width, height)

    bins = np.linspace(0, rmax, nBin)
    binWidth = bins[1] - bins[0]

    rbar = np.empty_like(bins)
    prof = np.empty_like(bins)

    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    R = np.hypot(X + im.getX0(), Y + im.getY0()).flatten()
    imProf = im.getArray().flatten()

    if rmin:
        imProf = imProf[R > rmin]
        R = R[R > rmin]

    for i in range(len(bins)):
        inBin = np.abs(R - bins[i]) < 0.5*binWidth
        rbar[i] = np.mean(R[inBin])
        if np.sum(inBin):
            ivals = imProf[inBin]
            if median:
                val = np.median(ivals)
            else:
                val = np.mean(ivals[np.isfinite(ivals)])
        else:
            val = 0

        prof[i] = val

    return rbar, prof


def plotAnnularAverages(mos, visits, bin=16, useSmoothed=True, weightByR=True, showSum=True, **kwargs):
    pyplot.clf()
    #
    # Find the spacing between exposures of Arcturus
    #
    allVisits = sorted(position.keys())
    spacing = np.hypot(*position[allVisits[1]]) - np.hypot(*position[allVisits[0]])

    sumProf = None
    for v in visits:
        rbar, prof = annularAverage(mos[-v if useSmoothed else v], **kwargs)
        rbar *= bin
        prof *= spacing*bin             # scale up as if we had exposures of Arcturus with 1 pixel steps
        #
        # Allow for the extra phase space further away from the boresight
        if weightByR:
            r = bin*(np.hypot(*position[v]) + 0.3*spacing)  # really a geometric mean?
            prof *= 2*np.pi*r

        print("%d %07.2g" % (v, np.sum(prof)))

        if showSum:
            if sumProf is None:
                sumProf = np.zeros_like(prof)

            sumProf += prof

        pyplot.plot(rbar, prof, label=str(v))

    if sumProf is not None:
        pyplot.plot(rbar, sumProf, label="sum")

    legend = pyplot.legend(loc="best", ncol=2,
                           borderpad=0.1, labelspacing=0, handletextpad=0, handlelength=2, borderaxespad=0)

    if legend:
        legend.draggable(True)

    pyplot.show()
