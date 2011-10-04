#!/usr/bin/env python

import os
import sys

import matplotlib
matplotlib.use('Agg')
import lsst.obs.suprimecam as obsSc

import lsst.pipette.readwrite as pipReadWrite
import lsst.pipette.options as pipOptions


if __name__ == '__main__':
    parser = pipOptions.OptionParser()
    parser.add_option('-r', '--rerun', dest='rerun', help='rerun name')
    parser.add_option('-v', '--visit', dest='visit', type=int, help='visit # to run')
    parser.add_option('-c', '--ccd', dest='ccd', type=int, help='CCD # to run')
    parser.add_option('--data-range', dest='datarange', type=float, nargs=2, help='Image stretch (low, high)')
    parser.add_option('--roi', dest='roi', type=int, nargs=4, help='ROI (x0, x1, y0, y1)')
    parser.add_option('--plotsize', dest='plotsize', type=int, nargs=2, help='Plot size (w x h) inches')
    parser.add_option('--psf', dest='psf', help='Render a PSF image at center -- to the given output filename')
    parser.add_option('--image', dest='image', help='Write out the image in FITS format -- to the given filename')
    parser.add_option('--sources', dest='sources', help='Write out the sources in FITS format -- to the given filename')
    
    default = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "ProcessCcdDictionary.paf")
    overrides = os.path.join(os.getenv("MEAS_DEBLENDER_DIR"), 'examples', 'suprime.paf')
    config, opt, args = parser.parse_args([default, overrides])
    if 'roots' in config:
        roots = config['roots']
    else:
        roots = {}
    # ugh, Config objects don't have a get(k,d) method...
    dataroot = None
    if 'data' in roots:
        dataroot = roots['data']
    calibroot = None
    if 'calib' in roots:
        calibroot = roots['calib']
    elif dataroot is not None:
        calibroot = os.path.join(dataroot, 'CALIB')
        
    mapper = obsSc.SuprimecamMapper(root=dataroot, calibRoot=calibroot, rerun=opt.rerun)

    io = pipReadWrite.ReadWrite(mapper, ['visit'], config=config)
    butler = io.inButler

    dataId = { 'visit': opt.visit, 'ccd': opt.ccd }
    #print 'data range', opt.datarange
    #print 'roi', opt.roi
    #print 'plotsize', opt.plotsize

    expdatatype = 'calexp'

    if opt.psf:
        import lsst.afw.geom as afwGeom
        import lsst.afw.image as afwImage
        import numpy as np
        import pyfits
        
        psf = butler.get('psf', dataId)
        print 'psf is', psf
        if opt.roi is None:
            exposure = butler.get(expdatatype, dataId)
            w,h = exposure.getWidth(), exposure.getHeight()
            x,y = w/2., h/2.
        else:
            x,y = (opt.roi[0] + opt.roi[1])/2., (opt.roi[2] + opt.roi[3])/2.
        img = psf.computeImage(afwGeom.Point2D(x,y), False)
        print 'img', img
        w,h = img.getWidth(), img.getHeight()
        npimg = np.empty((h,w))
        for y in range(h):
            for x in range(w):
                npimg[y,x] = img.get(x,y)
        pyfits.writeto(opt.psf, npimg, clobber=True)
        print 'Wrote PSF image to', opt.psf

    if opt.image:
        exposure = butler.get(expdatatype, dataId)

        if opt.roi is None:
            exposure.writeFits(opt.image)
        else:
            x0,x1,y0,y1 = opt.roi
            subexp = afwImage.ExposureF(exposure, afwGeom.Box2I(afwGeom.Point2I(x0,y0),
                                                                afwGeom.Point2I(x1-1,y1-1)))
            subexp.writeFits(opt.image)
        print 'Wrote', opt.image

        #rawexp = butler.get('raw', dataId)
        #print 'Raw exposure:', rawexp
        #rawexp.writeFits('raw.fits')

        

    import plotSources
    import pylab as plt
    bb = []
    if opt.plotsize:
        plt.figure(figsize=opt.plotsize)

    srcs = plotSources.plotSources(butler=butler, dataId=dataId,
                                   fn='suprime-v%06i-c%02i.png' % (opt.visit,opt.ccd),
                                   exposureDatatype=expdatatype,
                                   datarange=opt.datarange,
                                   roi=opt.roi,
                                   bboxes=bb)

    if opt.sources:
        import pyfits
        import numpy as np

        data = dict([(c, np.array([getattr(s,c) for s in srcs]))
                     for c in ['x','y','ra','dec','a','b','theta','flux']])
        if opt.roi is not None:
            x0,x1,y0,y1 = opt.roi
            data['x'] -= x0
            data['y'] -= y0

        tab = pyfits.new_table([pyfits.Column(name=c, format='D', array=d)
                                for c,d in data.items()])
        tab.writeto(opt.sources, clobber=True)
        print 'Wrote sources to', opt.sources
