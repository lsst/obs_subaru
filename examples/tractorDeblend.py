#!/usr/bin/env python

import os
import sys

import pyfits

import lsst.pipette.options as pipOptions
import lsst.pipette.readwrite as pipReadWrite
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom

from lsst.meas.deblender import deblender

from lsst.meas.deblender import naive as naive_deblender

from tractorPreprocess import getMapper, footprintsFromPython

import matplotlib
matplotlib.use('Agg')
import pylab as plt
import numpy as np

def afwimgtonumpy(I, x0=0, y0=0, W=None,H=None):
    if W is None:
        W = I.getWidth()
    if H is None:
        H = I.getHeight()
    img = np.zeros((H,W))
    for ii in range(H):
        for jj in range(W):
            img[ii, jj] = I.get(jj+x0, ii+y0)
    return img


def run(visit, rerun, config):
    mapper = getMapper()
    dataId = { 'visit': visit, 'rerun': rerun }
    #rrdir = mapper.getPath('outdir', dataId)
    #if not os.path.exists(rrdir):
    #    print 'Creating directory for ouputs:', rrdir
    #    os.makedirs(rrdir)
    io = pipReadWrite.ReadWrite(mapper, ['visit'], config=config)
    butler = io.inButler

    bb = butler.get('bb', dataId)
    print 'Bounding-boxes:', bb
    print len(bb)

    pyfoots = butler.get('pyfoots', dataId)
    foots,pks = footprintsFromPython(pyfoots)
    print 'Footprints:'
    print foots
    print 'Peaks:'
    print pks

    # HACK peaks
    fn = mapper.getPath('truesrc', dataId)
    srcs = pyfits.open(fn)[1].data
    x = srcs.field('x').astype(float)
    y = srcs.field('y').astype(float)
    print x, y
    pks = []
    for foot in foots:
        thispks = []
        bbox = foot.getBBox()
        bb = (bbox.getMinX(), bbox.getMinY(), bbox.getMaxX(), bbox.getMaxY())
        print 'Looking for sources for footprint with bbox', bb
        for xi,yi in zip(x,y):
            if foot.contains(afwGeom.Point2I(int(round(xi)),int(round(yi)))):
                thispks.append(afwDet.Peak(xi,yi))
                print '  Source at', (xi,yi), 'is inside footprint with bbox', bb
        pks.append(thispks)
        print 'Got', len(thispks), 'sources for this footprint'
    print 'OVERRODE peaks', pks

    exposureDatatype = 'visitim'
    exposure = butler.get(exposureDatatype, dataId)
    mi = exposure.getMaskedImage()
    print 'MaskedImage:', mi

    psf = butler.get('psf', dataId)
    print 'PSF:', psf

    import lsst.meas.algorithms as measAlg
    bb = foots[0].getBBox()
    xc = int((bb.getMinX() + bb.getMaxX()) / 2.)
    yc = int((bb.getMinY() + bb.getMaxY()) / 2.)
    pa = measAlg.PsfAttributes(psf, xc, yc)
    psfw = pa.computeGaussianWidth(measAlg.PsfAttributes.ADAPTIVE_MOMENT)
    print 'PSF width:', psfw
    psf_fwhm = 2.35 * psfw

    if False:
        print 'Calling deblender...'
        objs = deblender.deblend(foots, pks, mi, psf)
        print 'got', objs
        for obj in objs:
            print 'Object:'
            print obj

    if True:
        print 'Calling naive deblender...'
        results = naive_deblender.deblend(foots, pks, mi, psf, psf_fwhm)

        for i,(foot,fpres) in enumerate(zip(foots,results)):
            #(foot,templs,ports,bgs,mods,mods2) in enumerate(zip(foots,allt,allp,allbg,allmod,allmod2)):
            sumP = None

            W,H = foot.getBBox().getWidth(), foot.getBBox().getHeight()
            x0,y0 = foot.getBBox().getMinX(), foot.getBBox().getMinY()
            I = afwimgtonumpy(mi.getImage(), x0, y0, W, H)
            mn,mx = I.min(), I.max()
            ima = dict(interpolation='nearest', origin='lower', vmin=mn, vmax=mx)

            for j,pkres in enumerate(fpres.peaks):
                #(templ,port,bg,mod,mod2) in enumerate(zip(templs,ports,bgs,mods,mods2)):
                timg = pkres.timg
                #templ.writeFits('templ-f%i-t%i.fits' % (i, j))

                T = afwimgtonumpy(pkres.timg)
                P = afwimgtonumpy(pkres.portion)
                S = afwimgtonumpy(pkres.stamp)
                M = afwimgtonumpy(pkres.model)
                M2 = afwimgtonumpy(pkres.model2)

                NR,NC = 2,3
                plt.clf()

                plt.subplot(NR,NC,1)
                plt.imshow(I, **ima)
                plt.title('Image')
                plt.colorbar()

                plt.subplot(NR,NC,2)
                plt.imshow(T, **ima)
                plt.title('Template')

                plt.subplot(NR,NC,3)
                plt.imshow(P, **ima)
                plt.title('Flux portion')

                #plt.subplot(NR,NC,4)
                #plt.imshow(B, **ima)
                #plt.title('Background-subtracted')

                #backgr = T - B
                #psfbg = backgr + M

                plt.subplot(NR,NC,4)
                plt.imshow(S, **ima)
                plt.title('near-peak cutout')
                #plt.imshow(psfbg, **ima)
                #plt.title('PSF+bg model')

                plt.subplot(NR,NC,5)
                plt.imshow(M, **ima)
                plt.title('near-peak model')

                plt.subplot(NR,NC,6)
                plt.imshow(M2, **ima)
                plt.title('near-peak model (2)')

                #plt.subplot(NR,NC,5)
                #res = B-M
                #mx = np.abs(res).max()
                #plt.imshow(res, interpolation='nearest', origin='lower', vmin=-mx, vmax=mx)
                #plt.colorbar()
                #plt.title('Residuals')

                # plt.subplot(NR,NC,6)
                # py = pks[i][j].getIy()
                # plt.plot(T[py-y0,:], 'r-')
                # #plt.plot(T[py-y0+1,:], 'b-')
                # #plt.plot(T[py-y0-1,:], 'g-')
                # plt.plot(psfbg[py-y0,:], 'b-')
                # plt.title('Template slices')

                plt.savefig('templ-f%i-t%i.png' % (i,j))

                if sumP is None:
                    sumP = P
                else:
                    sumP += P
            if sumP is not None:
                plt.clf()
                NR,NC = 1,2
                plt.subplot(NR,NC,1)
                plt.imshow(I, **ima)
                plt.subplot(NR,NC,2)
                plt.imshow(sumP, **ima)
                plt.savefig('sump-f%i.png' % i)

if __name__ == "__main__":
    parser = pipOptions.OptionParser()
    parser.add_option("-r", "--rerun", default=0, dest="rerun", type=int, help='rerun number')
    parser.add_option("-v", "--visit", dest="visit", type=int, default=0, help="visit to run (default=%default)")
    #parser.add_option("-p", "--plots", dest="plots", default=False, action='store_true', help='Make plots?')

    default = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "ProcessCcdDictionary.paf")
    overrides = os.path.join(os.getenv("MEAS_DEBLENDER_DIR"), 'examples', 'tractor.paf')
    config, opt, args = parser.parse_args([default, overrides])
    if len(args) > 0 or len(sys.argv) == 1 or opt.rerun is None or opt.visit is None:
        parser.print_help()
        sys.exit(1)
    print 'running visit', opt.visit, 'rerun', opt.rerun
    run(opt.visit, opt.rerun, config)

    #if opt.plots:
    #    plots(opt.visit, opt.rerun, config, bb)
        

