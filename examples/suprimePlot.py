#!/usr/bin/env python

import os
import sys

import matplotlib
matplotlib.use('Agg')
import lsst.obs.suprimecam as obsSc

import lsst.pipette.readwrite as pipReadWrite
import lsst.pipette.options as pipOptions


if __name__ == '__main__':
    import optparse

    #parser = optparse.OptionParser()
    parser = pipOptions.OptionParser()
    parser.add_option('-R', '--root', dest='root', help='Data root')
    parser.add_option('-C', '--calibroot', dest='calibroot', help='Calibration data root')
    parser.add_option('-r', '--rerun', dest='rerun', help='rerun name')
    parser.add_option('-v', '--visit', dest='visit', type=int, help='visit # to run')
    parser.add_option('-c', '--ccd', dest='ccd', type=int, help='CCD # to run')
    #opt,args = parser.parse_args()

    default = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "ProcessCcdDictionary.paf")
    overrides = os.path.join(os.getenv("MEAS_DEBLENDER_DIR"), 'examples', 'suprime.paf')
    config, opt, args = parser.parse_args([default, overrides])

    #rerun=, outRoot=, root=, mit=?
    mapper = obsSc.SuprimecamMapper(root=opt.root, calibRoot=opt.calibroot, rerun=opt.rerun)

    io = pipReadWrite.ReadWrite(mapper, ['visit'], config=config)
    butler = io.inButler

    dataId = { 'visit': opt.visit, 'ccd': opt.ccd }
    exposure = butler.get('calexp', dataId)
    print 'exposure:', exposure

    import plotSources
    bb = []
    plotSources.plotSources(butler=butler, dataId=dataId,
                            fn='suprime-v%06i-c%02i.png' % (opt.visit,opt.ccd),
                            exposureDatatype='calexp',
                            bboxes=bb)
    
