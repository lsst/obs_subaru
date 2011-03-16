#!/usr/bin/env python

import os
import sys

import lsst.pipette.options as pipOptions
import lsst.pipette.readwrite as pipReadWrite
import lsst.pipette.isr as pipIsr

from tractorMapper import *


class NullISR(pipIsr.Isr):
    def __init__(self, **kwargs):
        pass
    def run(exposures, detrends):
        # exposure, defects, background
        return exposures, None, None

def run(visit, rerun, config):
    io = pipReadWrite.ReadWrite(TractorMapper, ['visit'], config=config)
    dataId = { 'visit': visit, 'rerun': rerun }
    rrdir = io.outMapper.getPath('outdir', dataId)
    if not os.path.exists(rrdir):
        os.mkdir(rrdir)

    ccdProc = pipProcCcd.ProcessCcd(config=config, Isr=NullISR)

    raws = io.readRaw(dataId)
    detrends = io.detrends(dataId, config)
    exposure, psf, apcorr, brightSources, sources, matches, matchMeta = ccdProc.run(raws, detrends)
    io.write(dataId, exposure=exposure, psf=psf, sources=sources, matches=matches, matchMeta=matchMeta)

    catPolicy = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "catalog.paf")
    catalog = pipCatalog.Catalog(catPolicy, allowNonfinite=False)
    if sources is not None:
        catalog.writeSources(basename + '.sources', sources, 'sources')
    if matches is not None:
        catalog.writeMatches(basename + '.matches', matches, 'sources')
    return



if __name__ == "__main__":
    parser = pipOptions.OptionParser()
    parser.add_option("-r", "--rerun", default=0, dest="rerun", type=int, help='rerun number')
    parser.add_option("-v", "--visit", dest="visit", type=int, default=0, help="visit to run (default=%default)")

    default = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "ProcessCcdDictionary.paf")
    #overrides = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "suprimecam.paf")
    #config, opts, args = parser.parse_args([default, overrides])
    config, opts, args = parser.parse_args([default])
    if len(args) > 0 or len(sys.argv) == 1 or opts.rerun is None or opts.frame is None or opts.ccd is None:
        parser.print_help()
        sys.exit(1)

    run(opts.visit, opts.rerun, config)
