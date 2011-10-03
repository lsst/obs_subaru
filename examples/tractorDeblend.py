#!/usr/bin/env python

import os
import sys

import lsst.pipette.options as pipOptions
import lsst.pipette.readwrite as pipReadWrite

from lsst.meas.deblender import deblender

from tractorPreprocess import getMapper, footprintsFromPython


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

    exposureDatatype = 'visitim'
    exposure = butler.get(exposureDatatype, dataId)
    mi = exposure.getMaskedImage()
    print 'MaskedImage:', mi

    psf = butler.get('psf', dataId)
    print 'PSF:', psf

    print 'Calling deblender...'
    objs = deblender.deblend(foots, pks, mi, psf)
    print 'got', objs


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
        

