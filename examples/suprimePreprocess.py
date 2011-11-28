#!/usr/bin/env python

import os
import sys

import matplotlib
matplotlib.use('Agg')

import lsst.obs.suprimecam as obsSc

import lsst.pex.logging as pexLog

import lsst.pipette.readwrite as pipReadWrite
import lsst.pipette.options as pipOptions
import lsst.pipette.processCcd as pipProcCcd
import lsst.pipette.isr as pipIsr
import lsst.pipette.calibrate as pipCalib
import lsst.pipette.phot as pipPhot

import lsst.meas.algorithms
#import lsst.meas.algorithms.psfSelectionRhl
#import lsst.meas.algorithms.psfAlgorithmRhl
#import lsst.meas.extensions.shapeHSM





if __name__ == '__main__':
    parser = pipOptions.OptionParser()

    parser.add_option('-r', '--rerun', dest='rerun', help='rerun name')
    parser.add_option('-v', '--visit', dest='visit', type=int, help='visit # to run')
    parser.add_option('-c', '--ccd', dest='ccd', type=int, help='CCD # to run')

    #parser.add_option('--data-range', dest='datarange', type=float, nargs=2, help='Image stretch (low, high)')
    #parser.add_option('--plotsize', dest='plotsize', type=int, nargs=2, help='Plot size (w x h) inches')

    parser.add_option('--roi', dest='roi', type=int, nargs=4, help='ROI (x0, x1, y0, y1)')
    parser.add_option('--psf', dest='psf', help='Wrote boost-format PSF to the given output filename')
    parser.add_option('--psfimg', dest='psfimg', help='Render a PSF image at center -- to the given output filename')
    parser.add_option('--image', dest='image', help='Write out the image in FITS format -- to the given filename')
    parser.add_option('--sources', dest='sources', help='Write out the sources in FITS format -- to the given filename')
    parser.add_option('--footprints', dest='footprints', help='Write out the footprints in pickle format -- to the given filename')
    
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
    outbut = io.outButler
    dataId = { 'visit': opt.visit, 'ccd': opt.ccd }

    print 'Reading exposure'
    
    #exposure = io.inButler.get('visitim', dataId)
    exposure = io.inButler.get('calexp', dataId)
    print 'exposure is', exposure
    print 'size', exposure.getWidth(), 'x', exposure.getHeight()
    mi = exposure.getMaskedImage()
    wcs = exposure.getWcs()
    assert wcs

    print 'Calibrate...'
    log = pexLog.getDefaultLog()
    cal = pipCalib.Calibrate(config=config, log=log)#, Photometry=MyPhotometry)

    # can we ignore ISR?
    #exposure, defects, background = self.isr(exposureList, detrendsList)
    defects=None
    background=None
    print 'cal.run()...'
    psf, apcorr, brightSources, matches, matchMeta = cal.run(exposure, defects=defects)

    print 'psf is', psf
    if opt.psf:
        from lsst.daf.persistence import Mapper, ButlerLocation, LogicalLocation

        class HackMapper(Mapper):
            def __init__(self, themap={}):
                self.themap = themap
            def map(self, datasetType, dataId):
                return self.themap.get(datasetType)

        mapper = HackMapper({'psf': ButlerLocation(
            'lsst.afw.detection.Psf', 'Psf', 'BoostStorage', opt.psf, dataId)})
        #outbut.setMapper(mapper)
        outbut.mapper = mapper
        outbut.put(psf, 'psf', dataId)
        print 'Wrote psf to', opt.psf


    print 'Photometry...'
    phot = pipPhot.Photometry(config=config, log=log)#imports=[])
    print 'imports()...'
    phot.imports()
    print 'detect()...'
    footprintSet = phot.detect(exposure, psf)
    #print 'measure()...'
    #sources = phot.measure(exposure, footprintSet, psf, apcorr=apcorr, wcs=wcs)
    #print 'done!'


    # print 'Photometry()...'
    # phot = pipPhot.Photometry(config=config, log=log)
    # sources, footprints = phot.run(exposure, psf)
    # print 'sources:', len(sources)
    # for s in sources:
    #     print '  ', s, s.getXAstrom(), s.getYAstrom(), s.getPsfFlux(), s.getIxx(), s.getIyy(), s.getIxy()
    # 
    # print 'footprints:', footprints
    # # oh yeah, baby!
    # fps = footprints.getFootprints()
    # print len(fps)
    # bb = []
    # for f in fps:
    #     print '  Footprint', f
    #     print '  ', f.getBBox()
    #     bbox = f.getBBox()
    #     bb.append((bbox.getMinX(), bbox.getMinY(), bbox.getMaxX(), bbox.getMaxY()))
    #     print '   # peaks:', len(f.getPeaks())
    #     for p in f.getPeaks():
    #         print '    Peak', p

    # print 'writing output...'
    # io.write(dataId, psf=psf, sources=sources)
    # print 'done!'
    # print 'Writing bounding-boxes...'
    # io.outButler.put(bb, 'bb', dataId)
    # # serialize a python version of footprints & peaks
    # pyfoots = footprintsToPython(fps)
    # print 'Writing py footprints...'
    # io.outButler.put(pyfoots, 'pyfoots', dataId)
    # return bb


    if opt.psfimg:
        import lsst.afw.geom as afwGeom
        import lsst.afw.image as afwImage
        import numpy as np
        import pyfits
        
        #psf = butler.get('psf', dataId)
        print 'psf is', psf
        if opt.roi is None:
            #exposure = butler.get(expdatatype, dataId)
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
        #exposure = butler.get(expdatatype, dataId)

        if opt.roi is None:
            exposure.writeFits(opt.image)
        else:
            import lsst.afw.image as afwImage
            import lsst.afw.geom as afwGeom
            x0,x1,y0,y1 = opt.roi
            subexp = afwImage.ExposureF(exposure, afwGeom.Box2I(afwGeom.Point2I(x0,y0),
                                                                afwGeom.Point2I(x1-1,y1-1)))
            subexp.writeFits(opt.image)
        print 'Wrote', opt.image

        #rawexp = butler.get('raw', dataId)
        #print 'Raw exposure:', rawexp
        #rawexp.writeFits('raw.fits')

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


    if opt.footprints:
        import cPickle
        from testDeblend import footprintsToPython

        if opt.roi is not None:
            import lsst.afw.geom as afwGeom
            x0,x1,y0,y1 = opt.roi
            roibb = afwGeom.Box2I(afwGeom.Point2I(x0,y0),
                                  afwGeom.Point2I(x1-1,y1-1))
        else:
            roibb = None
        pyfoots = footprintsToPython(footprintSet.getFootprints(), keepbb=roibb)
        out = open(opt.footprints, 'wb')
        cPickle.dump(pyfoots, out, -1)
        out.close()
        print 'Wrote footprints to', opt.footprints
        
