#!/usr/bin/env python

import os
import sys

import pyfits

import lsst.pipette.options as pipOptions
import lsst.pipette.readwrite as pipReadWrite
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom

import lsst.obs.suprimecam as obsSc
import lsst.pipette.readwrite as pipReadWrite
import lsst.pipette.options as pipOptions

import matplotlib
matplotlib.use('Agg')


def run(visit, ccd, config):
    mapper = getMapper()
    dataId = { 'visit': visit, 'ccd': ccd }
    io = pipReadWrite.ReadWrite(mapper, ['visit'], config=config)
    butler = io.inButler
    testDeblend(butler, dataId)


if __name__ == "__main__":
    parser = pipOptions.OptionParser()
    parser.add_option('-v', '--visit', dest='visit', type=int, help='visit # to run')
    parser.add_option('-c', '--ccd', dest='ccd', type=int, help='CCD # to run')

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
        

