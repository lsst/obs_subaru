if __name__ == '__main__':
    import matplotlib
    matplotlib.use('Agg')

import lsst.meas.deblender.sdss as sdss

import pylab as plt
import numpy as np
import matplotlib as mpl
from math import pi

if __name__ == '__main__':
    sdss.phInitProfileExtract()
    cstats = sdss.phProfileGeometry()
    print cstats

    for j,logr in enumerate([False, True]):
        plt.clf()
        colors = ['r','b','g','m','c','y','0.5']

        for i in range(cstats.ncell):
            cg = sdss.cell_stats_get_cellgeom(cstats, i)

            inner = cg.inner
            outer = cg.outer
            cw = cg.cw
            ccw = cg.ccw
            ann = cg.ann
            sec = cg.sec

            angles = np.linspace(ccw, cw, 100)
            rangles = angles[-1::-1]

            theta = np.hstack((angles, rangles, angles[0]))
            r = np.hstack((inner * np.ones_like(angles), outer * np.ones_like(rangles), inner))
            midangle = angles[len(angles)/2]
            midr = (inner + outer)/2.

            if logr:
                r = np.log10(1. + r)
                midr = np.log10(1. + midr)

            x = np.cos(theta) * r
            y = np.sin(theta) * r
            midx = np.cos(midangle) * midr
            midy = np.sin(midangle) * midr

            c = colors[i % len(colors)]
            plt.plot(x, y, c)
            a = plt.gca()
            p = mpl.patches.Polygon(np.vstack((x,y)).T, fill=True, alpha=0.5, fc=c)
            a.add_artist(p)
            plt.text(midx, midy, '%i/%i/%i' % (i, ann, sec), ha='center',
                     clip_on=True)

        plt.axis('tight')
        plt.axis('equal')
        plt.savefig('cellprofs-%i.png' % j)

        if logr:
            R = 0.7
        else:
            R = 10.
        plt.axis([-R,R,-R,R])
        plt.savefig('cellprofs-%ib.png' % j)

