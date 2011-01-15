if __name__ == '__main__':
    import matplotlib
    matplotlib.use('Agg')

import sys

import lsst.meas.deblender.sdss as sdss

import pylab as plt
import numpy as np
import matplotlib as mpl
from math import pi

import pyfits
    
def main():
    sdss.phInitProfileExtract()

    P = pyfits.open('tests/test1.fits')
    # a galaxy image
    img = P[0].data
    img *= 1000.
    mn,mx = img.min(), img.max()
    print 'image range', mn, mx

    # make int list
    (H,W) = img.shape
    ilist = [int(p) for p in img.ravel()]
    cx,cy = 50,50

    # punch it   (..., maxrad, sky, skysig)
    cstats = sdss.extractProfile(ilist, W, H, cx+0.5, cy+0.5, 0, 0, 0)

    cmap = mpl.cm.hot

    plt.clf()
    plt.imshow(img, interpolation='nearest', origin='lower', cmap=cmap,
               extent=[-cx-0.5, -cx+W-0.5, -cy-0.5, -cy+H-0.5])
    ax = plt.axis()
    #plt.axhline(0, color='b')
    plt.axis(ax)
    plt.savefig('img.png')

    for kind in ['mean', 'median']:
        plt.clf()
        for i in range(cstats.ncell):
            geom = sdss.cell_stats_get_cellgeom(cstats, i)
            pstats = sdss.cell_stats_get_cell(cstats, i)
            if kind == 'mean':
                v = pstats.mean
                #print 'mean val', v
            else:
                v = sdss.pstats_get_median(pstats)
                #print 'median val', v
            c = cmap( (v - mn) / (mx - mn) )
            plot_cell(geom, color=c, falpha=1., ealpha=0.1, ec='0.5', elw=0.5,
                  ##
                  label_cell=False,
                  label='%.1f' % pstats.mean,
                  labelkwargs={'fontsize':10, 'color':'1.0'})
        plt.gca().set_axis_bgcolor('k')
        plt.axis('scaled')
        plt.axis(ax)
        plt.savefig('profile-%s.png' % kind)

    median = 1
    #    (median, sky, gain, darkvar, psfwidth, poserr, use diff, sky noise only)
    cprof = sdss.getCellProfile(cstats, median, 0, 1., 1., 2., 0.1, 0, 0)

    print 'cprof', cprof
    print 'ncell', cprof.ncell
    print 'median?', cprof.is_median
    print 'data:', [sdss.cellprof_get_data(cprof,i) for i in range(cprof.ncell)]

    sys.exit(0)

    ncell = 169
    plt.clf()
    x0,x1, y0,y1 = -20,20, -20,20
    #nsteps = 200
    nsteps = 161
    XX = np.linspace(x0, x1, nsteps)
    YY = np.linspace(y0, y1, nsteps)
    cidimg = np.zeros((len(YY),len(XX)))
    for i,y in enumerate(YY):
        for j,x in enumerate(XX):
            (cid,ri,si) = sdss.phFakeGetCellId(x, y, ncell)
            print (x,y), '-->', (cid,ri,si)
            cidimg[i,j] = cid
    print 'unique cellids:', np.unique(cidimg)
    plt.imshow(cidimg, interpolation='nearest', origin='lower',
               extent=[x0,x1,y0,y1])
    plt.colorbar()
    plt.savefig('cellids-fake.png')

    #sys.exit(0)

    # phGetCellid() isn't very useful
    plt.clf()
    x0,y0,W,H = -20, -20, 41, 41
    cidimg = np.zeros((H,W))
    for r in range(H):
        for c in range(W):
            cid = sdss.phGetCellid(r + y0, c + x0)
            cidimg[r,c] = cid
    print 'unique cellids:', np.unique(cidimg)
    plt.imshow(cidimg, interpolation='nearest', origin='lower')
    plt.colorbar()
    plt.savefig('cellids.png')

    cstats = sdss.phProfileGeometry()
    print cstats
    for j,logr in enumerate([False, True]):
        plt.clf()
        colors = ['r','b','g','m','c','y','0.5']
        for i in range(cstats.ncell):
            cg = sdss.cell_stats_get_cellgeom(cstats, i)
            c = colors[i % len(colors)]
            plot_cell(cg, color=c, logr=logr, label_rad=True, celli=i)
        plt.savefig('cellprofs-%i.png' % j)
        if logr:
            R = 0.7
        else:
            R = 10.
        plt.axis([-R,R,-R,R])
        plt.savefig('cellprofs-%ib.png' % j)

def plot_cell(cg, logr=False, color='r', label_cell=True, label_rad=False, label=None, celli=-1, falpha=0.5, ec='k', ealpha=0.5,
              elw=1., labelkwargs={}):
    inner = cg.inner
    outer = cg.outer
    cw = cg.cw
    ccw = cg.ccw
    ann = cg.ann
    sec = cg.sec

    if logr:
        def mapr(r):
            return np.log10(1.+r)
    else:
        def mapr(r):
            return r

    def getx(r, theta):
        # theta from -ve x axis
        return -np.cos(theta) * r
    def gety(r, theta):
        return np.sin(theta) * r
    def getxy(r, theta):
        return getx(r,theta), gety(r,theta)


    angles = np.linspace(ccw, cw, 100)
    rangles = angles[-1::-1]

    theta = np.hstack((angles, rangles, angles[0]))
    r = np.hstack((inner * np.ones_like(angles), outer * np.ones_like(rangles), inner))
    midangle = angles[len(angles)/2]
    midr = (inner + outer)/2.

    r = mapr(r)
    midr = mapr(midr)

    x,y = getxy(r, theta)
    midx,midy = getxy(midr, midangle)

    a = plt.gca()
    p = mpl.patches.Polygon(np.vstack((x,y)).T, fill=True, alpha=falpha, fc=color)
    a.add_artist(p)
    plt.plot(x, y, color=ec, alpha=ealpha, lw=elw)
    if label_cell:
        if label is None:
            label = '%i/%i/%i' % (celli, ann, sec)
        plt.text(midx, midy, label, ha='center', clip_on=True, **labelkwargs)
        if sec == 0 and label_rad:
            rr = outer
            rr = mapr(rr)
            xx,yy = getxy(rr, cw)
            plt.text(xx, yy, '%.1f' % outer, ha='center', clip_on=True, bbox=dict(facecolor='w'))
    plt.axis('tight')
    plt.axis('equal')



if __name__ == '__main__':
    main()
