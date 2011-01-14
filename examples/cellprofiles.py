if __name__ == '__main__':
    import matplotlib
    matplotlib.use('Agg')

import lsst.meas.deblender.deblenderLib as lib

import pylab as plt
import numpy as np
import matplotlib as mpl
from math import pi

if __name__ == '__main__':
    db = lib.SDSSDeblenderF()
    strs = db.debugProfiles()
    
    for s in strs:
        exec(s)

    print 'cellgeom:', cellgeom

    for j,logr in enumerate([False, True]):
        plt.clf()
        colors = ['r','b','g','m','c','y','0.5']
        for i,cell in enumerate(cellgeom):
            inner = cell['inner']
            outer = cell['outer']
            cw = cell['cw']
            ccw = cell['ccw']

            #if cw > ccw:
            angles = np.linspace(ccw, cw, 100)
            #else:
            #    angles = np.linspace(ccw, cw + 2.*pi, 100)
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
            plt.text(midx, midy, '%i/%i/%i' % (i, cell['ann'], cell['sec']), ha='center')

        plt.axis('tight')
        plt.axis('equal')
        plt.savefig('cellprofs-%i.png' % j)

        if logr:
            R = 0.7
        else:
            R = 10.
        plt.axis([-R,R,-R,R])
        plt.savefig('cellprofs-%ib.png' % j)

