#!/usr/bin/env python

import os
import sys

import lsst.pex.logging as pexLog
import lsst.obs.hscSim as obsHsc
import lsst.obs.suprimecam as obsSc
import lsst.pipette.config as pipConfig
import lsst.pipette.processCcd as pipCcd
import lsst.pipette.options as pipOptions
import lsst.pipette.catalog as pipCatalog
import lsst.pipette.readwrite as pipReadWrite
import lsst.pipette.phot as pipPhot

import lsst.pipette.ioHacks as pipExtraIO
from lsst.pipette.specific.hscDc2 import CalibrateHscDc2
from lsst.pipette.specific.suprimecam import ProcessCcdSuprimeCam

from IPython.core.debugger import Tracer
debug_here = Tracer()

class DeferredHSCState(object):
    def __init__(self, dataId, io, matchlist, matchMeta, sources, brightSources, exposure):
        self.dataId = dataId
        self.io = io
        self.matchlist = matchlist
        self.matchMeta = matchMeta
        self.sources = sources
        self.brightSources = brightSources
        self.exposure = exposure

    def __str__(self):
        return "DeferredHSCState(id=%s, exposure=%s)" % (self.dataId, self.exposure)
    
def run(rerun,                          # Rerun name
        frame,                          # Frame number
        ccd,                            # CCD number
        config,                         # Configuration
        log = pexLog.Log.getDefaultLog(), # Log object
    ):

    # Make our own mappers for now
    mapperArgs = {'rerun': rerun}       # Arguments for mapper instantiation

    if config.has_key('roots'):
        roots = config['roots']
        for key, value in {'data': 'root',
                           'calib': 'calibRoot',
                           'output': 'outRoot'}.iteritems():
            if roots.has_key(key):
                mapperArgs[value] = roots[key]
    
    camera = config['camera']
    if camera.lower() in ("hsc"):
        mapper = obsHsc.HscSimMapper(**mapperArgs)
        ccdProc = pipCcd.ProcessCcd(config=config, Calibrate=CalibrateHscDc2, log=log)
    elif camera.lower() in ("suprimecam-mit", "sc-mit", "scmit", "suprimecam-old", "sc-old", "scold"):
        mapper = obsSc.SuprimecamMapper(mit=True, **mapperArgs)
        ccdProc = ProcessCcdSuprimeCam(config=config, log=log)
    elif camera.lower() in ("suprimecam", "suprime-cam", "sc"):
        mapper = obsSc.SuprimecamMapper(**mapperArgs)
        ccdProc = ProcessCcdSuprimeCam(config=config, log=log)
        
    io = pipReadWrite.ReadWrite(mapper, ['visit', 'ccd'], config=config)

    oldUmask = os.umask(2)
    if oldUmask != 2:
        io.log.log(io.log.WARN, "pipette umask started as: %s" % (os.umask(2)))

    dataId = { 'visit': frame, 'ccd': ccd }

    # Skip as much ISR as we can if the calexp exists. Otherwise do all of it and
    # build the calexp for next time.
    try:
        raws = io.read('calexp', dataId)
        detrends = []
    except:
        detrends = io.detrends(dataId, config)

    if len([x for x in detrends if x]): # We need to run at least part of the ISR
        raws = io.readRaw(dataId)
    else:
        config['do']['calibrate']['repair']['cosmicray'] = False

        io.fileKeys = ['visit', 'ccd']
        raws = io.read('calexp', dataId)
        detrends = None

    try:
        exposure = raws[0]
        psf = io.read('psf', dataId)[0]
        matches = io.readMatches(dataId)
        brightSources = io.read('icSrc', dataId)[0]
        matchMeta = io.read('icMatch', dataId)[0]
        apcorr = None   # Eventually will need this.
        io.log.log(io.log.WARN, "read calibration products from disk, NO APCORR yet!")
        doHalfRun = True
    except Exception, e:
        io.log.log(io.log.WARN, "failed to read calibration products, will make them..... e:%s" % (e))
        doHalfRun = False
        exposure, psf, apcorr, brightSources, sources, matches, matchMeta = ccdProc.run(raws, detrends)
        io.write(dataId, exposure=None, psf=psf, sources=None)

    if doHalfRun:
        import pipette.processCcd as ourProcessCcd
        ourCcdProc = ourProcessCcd.ProcessCcd(config=config, log=log)
        exposure, psf, apcorr, brightSources, sources, matches, matchMeta = ourCcdProc.runFromData(exposure,
                                                                                                   psf, apcorr,
                                                                                                   brightSources,
                                                                                                   matches,
                                                                                                   matchMeta)

    catPolicy = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "catalog.paf")
    catalog = pipCatalog.Catalog(catPolicy, allowNonfinite=False)

    deferredState = DeferredHSCState(dataId, io, matches, matchMeta, sources, brightSources, exposure)
    return deferredState

def doMergeWcs(deferredState, wcs):
    dataId = deferredState.dataId
    io = deferredState.io
    exposure = deferredState.exposure
    matchlist = deferredState.matchlist
    
    if not wcs:
        wcs = exposure.getWcs()
        io.log.log(deferredState.io.log.WARN, "!!!!! stuffing exposure with its own wcs.!!!")

    exposure.setWcs(wcs)

    # Apply WCS to sources
    # In the matchlist, only_ convert the matchlist.second source, which is our measured source.
    matches = [m.second for m in deferredState.matchlist] if deferredState.matchlist is not None else None
    for sources in (deferredState.sources, deferredState.brightSources, matches):
        if sources is None:
            continue
        for s in sources:
            s.setRaDec(wcs.pixelToSky(s.getXAstrom(), s.getYAstrom()))

    # Write SRC....fits files here, until we can push the scheme into a butler.
    sources = deferredState.sources
    if sources:
        metadata = exposure.getMetadata()
        hdrInfo = dict([(m, metadata.get(m)) for m in metadata.names()])
        filename = io.outButler.get('source_filename', dataId)[0]
        io.log.log(io.log.INFO, "writing sources to: %s" % (filename))
        pipExtraIO.writeSourceSetAsFits(sources, filename, hdrInfo=hdrInfo, clobber=True, log=io.log)

    filename = io.outButler.get('matchFull_filename', dataId)[0]
    io.log.log(io.log.INFO, "writing match debugging info to: %s" % (filename))
    try:
        pipExtraIO.writeMatchListAsFits(matchlist, filename)
    except Exception, e:
        print "failed to write matchlist: %s" % (e)
        
    deferredState.io.write(deferredState.dataId, sources=sources, exposure=exposure,
                           brightSources=deferredState.brightSources, matches=matchlist,
                           matchMeta=deferredState.matchMeta)


def getConfigFromArguments(argv=None):
    default = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "ProcessCcdDictionary.paf")
    parser = pipOptions.OptionParser()
    parser.add_option("-r", "--rerun", default=os.getenv("USER", default="rerun"), dest="rerun",
                      help="rerun name (default=%default)")
    parser.add_option("-f", "--frame", dest="frame",
                      help="visit to run (default=%default)")
    parser.add_option("-c", "--ccd", dest="ccds", action="append",
                      help="CCD to run (default=%default)")

    config, opts, args = parser.parse_args([default], argv=argv)
    if (len(args) > 0 # or opts.instrument is None
        or opts.rerun is None
        or opts.frame is None
        or opts.ccds is None):

        parser.print_help()
        print "argv was: %s" % (argv)
        return None, None, None

    return config, opts, args

    
def getConfig(instrument="hsc", output=None, data=None, calib=None):
    default = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "ProcessCcdDictionary.paf")
    override = os.path.join(os.getenv("PIPETTE_DIR"), "policy", instrument + ".paf")
    if output:
        override['roots.output'] = output
    if data:
        override['roots.data'] = data
    if calib:
        override['roots.calib'] = calib
    return pipConfig.configuration(default, override)


def doLoad(instrument="hsc", output=None, data=None, calib=None, log=pexLog.Log.getDefaultLog()):
    config = getConfig(instrument=instrument, output=output, data=data, calib=calib)
    phot = pipPhot.Photometry(config=config, log=log)
    phot.imports()


def doRun(rerun=None, frameId=None, ccdId=None,
          doMerge=True,
          instrument="hsc",
          output=None,
          data=None,
          calib=None,
          log = pexLog.Log.getDefaultLog() # Log object
          ):
    config = getConfig(instrument=instrument, output=output, data=data, calib=calib)
    state = run(rerun, frameId, ccdId, config, log=log)
    if doMerge:
        doMergeWcs(state, None)
    else:
        return state


def main(argv=None):
    config, opts, args = getConfigFromArguments(argv)
    if not config:
        raise SystemExit("argument parsing error")
    
    ccds = []
    for c in opts.ccds:
        c = c.split(":")
        if len(c) == 1:
            c.append(int(c[0]) + 1)

        for ccd in range(int(c[0]), int(c[1])):
            ccds.append(ccd)

    for ccd in ccds:
        state = run(opts.rerun, int(opts.frame), ccd, config)
        doMergeWcs(state, None)

if __name__ == "__main__":
    main()
