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

    if False:
        print 'Calling deblender...'
        objs = deblender.deblend(foots, pks, mi, psf)
        print 'got', objs
        for obj in objs:
            print 'Object:'
            print obj
    else:
        print 'Calling naive deblender...'
        objs = naive_deblender.deblend(foots, pks, mi, psf)
        print 'got', objs
        for obj in objs:
            print 'Object:'
            print obj
        for i,templs in enumerate(objs):
            for j,templ in enumerate(templs):
                templ.writeFits('templ-f%i-t%i.fits' % (i, j))
                H,W = templ.getHeight(), templ.getWidth()
                T = np.zeros((H,W))
                for ii in range(H):
                    for jj in range(W):
                        T[ii,jj] = templ.get(jj,ii)
                plt.clf()
                plt.imshow(T, interpolation='nearest', origin='lower')
                plt.savefig('templ-f%i-t%i.png' % (i,j))


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
        

