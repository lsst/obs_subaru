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

    import plotSources
    import pylab as plt
    bb = []
    if opt.plotsize:
        plt.figure(figsize=opt.plotsize)

    plotSources.plotSources(butler=butler, dataId=dataId,
                            fn='suprime-v%06i-c%02i.png' % (opt.visit,opt.ccd),
                            exposureDatatype='calexp',
                            datarange=opt.datarange,
                            roi=opt.roi,
                            bboxes=bb)
    
