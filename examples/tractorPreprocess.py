#!/usr/bin/env python

import os
import sys

import lsst.pipette.options as pipOptions
import lsst.pipette.readwrite as pipReadWrite
import lsst.pipette.isr as pipIsr
import lsst.pipette.processCcd as pipProcCcd

from tractorMapper import *


class NullISR(pipIsr.Isr):
    def __init__(self, **kwargs):
        pass
    def run(exposures, detrends):
        # exposure, defects, background
        return exposures, None, None

def run(visit, rerun, config):
    mapper = TractorMapper()
    dataId = { 'visit': visit, 'rerun': rerun }
    rrdir = mapper.getPath('outdir', dataId)
    if not os.path.exists(rrdir):
        print 'Creating directory for ouputs:', rrdir
        os.makedirs(rrdir)
    io = pipReadWrite.ReadWrite(mapper, ['visit'], config=config)
    ccdProc = pipProcCcd.ProcessCcd(config=config, Isr=NullISR)

    #raws = io.readRaw(dataId)
    #detrends = io.detrends(dataId, config)
    print 'Reading exposure'
    #exposure = io.read('visitim', dataId)
    detrends = []
    raws = io.inButler.get('visitim', dataId)
    print 'ccdProc.run()...'
    exposure, psf, apcorr, brightSources, sources, matches, matchMeta = ccdProc.run(raws, detrends)
    print 'done!'
    #io.write(dataId, exposure=exposure, psf=psf, sources=sources, matches=matches, matchMeta=matchMeta)

if __name__ == "__main__":
    parser = pipOptions.OptionParser()
    parser.add_option("-r", "--rerun", default=0, dest="rerun", type=int, help='rerun number')
    parser.add_option("-v", "--visit", dest="visit", type=int, default=0, help="visit to run (default=%default)")

    default = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "ProcessCcdDictionary.paf")
    overrides = os.path.join(os.getenv("MEAS_DEBLENDER_DIR"), 'examples', 'tractor.paf')
    config, opt, args = parser.parse_args([default, overrides])
    if len(args) > 0 or len(sys.argv) == 1 or opt.rerun is None or opt.visit is None:
        parser.print_help()
        sys.exit(1)
    print 'running visit', opt.visit, 'rerun', opt.rerun
    run(opt.visit, opt.rerun, config)
