#!/usr/bin/env python

import os
import sys

import matplotlib
matplotlib.use('Agg')

import lsst.pex.logging as pexLog
import lsst.pipette.options as pipOptions
import lsst.pipette.readwrite as pipReadWrite
import lsst.pipette.processCcd as pipProcCcd
import lsst.pipette.isr as pipIsr
import lsst.pipette.calibrate as pipCalib
import lsst.pipette.phot as pipPhot

from tractorMapper import *

# Required for "dynamic" measurements
import lsst.meas.algorithms
#import lsst.meas.algorithms.psfSelectionRhl
#import lsst.meas.algorithms.psfAlgorithmRhl
#import lsst.meas.extensions.shapeHSM


class NullISR(pipIsr.Isr):
    def __init__(self, **kwargs):
        pass
    def run(self, exposures, detrends):
        # exposure, defects, background
        return exposures[0], None, None

class MyPhotometry(pipPhot.Photometry):
    def run(self, exposure, psf, apcorr=None, wcs=None):
        #print 'my config:', self.config
        print "policy = config['detect'] = ", self.config['detect']
        print 'self._threshold:', self._threshold
        print 'policy["thresholdValue"] = ', (self.config['detect'])['thresholdValue']
        return pipPhot.Photometry.run(self, exposure, psf, apcorr=apcorr, wcs=wcs)

class MyCalibrate(pipCalib.Calibrate):
    def run2(self, exposure, defects=None, background=None):
        assert exposure is not None, "No exposure provided"
        print 'creating fake PSF...'
        psf, wcs = self.fakePsf(exposure)
        print 'wcs is', wcs

        if False:
            import lsst.meas.utils.sourceDetection as muDetection
            import lsst.meas.utils.sourceMeasurement as muMeasurement
            # phot.py : detect()
            print 'Simulating detect()...'
            policy = self.config['detect']
            posSources, negSources = muDetection.detectSources(exposure, psf, policy.getPolicy())
            numPos = len(posSources.getFootprints()) if posSources is not None else 0
            numNeg = len(negSources.getFootprints()) if negSources is not None else 0
            print 'found', numPos, 'pos and', numNeg, 'neg footprints'
            footprintSet = posSources
            # phot.py : measure()
            print 'Simulating measure()...'
            policy = self.config['measure'].getPolicy()
            footprints = []
            num = len(footprintSet.getFootprints())
            self.log.log(self.log.INFO, "Measuring %d positive footprints" % num)
            footprints.append([footprintSet.getFootprints(), False])
            sources = muMeasurement.sourceMeasurement(exposure, psf, footprints, policy)
            print 'sources:', sources
            for s in sources:
                print '  ', s, s.getXAstrom(), s.getYAstrom(), s.getPsfFlux(), s.getIxx(), s.getIyy(), s.getIxy()
            # sourceMeasurement():
            import lsst.meas.algorithms   as measAlg
            import lsst.afw.detection     as afwDetection
            print 'Simulating sourceMeasurement()...'
            pexLog.Trace_setVerbosity("meas.algorithms.measure", True)
            exposure.setPsf(psf)
            measureSources = measAlg.makeMeasureSources(exposure, policy)
            #print 'ms policy', str(measureSources.getPolicy())
            #print 'ms astrom:', measureSources.getMeasureAstrom()
            #print 'ms photom:', measureSources.getMeasurePhotom()
            #print 'ms shape:', measureSources.getMeasureShape()
            F = footprints[0][0]
            for i in range(len(F)):
                # create a new source, and add it to the list, initialize ...
                s = afwDetection.Source()
                f = F[i]
                print 'measuring footprint', f
                measureSources.apply(s, f)
                print 'got', s
                print '  ', s, s.getXAstrom(), s.getYAstrom(), s.getPsfFlux(), s.getIxx(), s.getIyy(), s.getIxy()


        print 'initial photometry...'
        sources, footprints = self.phot(exposure, psf)
        #print 'got sources', str(sources)
        #print 'got footprints', str(footprints)
        #print 'sources:', sources
        print 'got sources:', len(sources)
        for s in sources:
            print '  ', s, s.getXAstrom(), s.getYAstrom(), s.getPsfFlux(), s.getIxx(), s.getIyy(), s.getIxy()
        print 're-photometry...'
        sources = self.rephot(exposure, footprints, psf)
        #print 'got sources', str(sources)
        print 'sources:', len(sources)
        for s in sources:
            print '  ', s, s.getXAstrom(), s.getYAstrom(), s.getPsfFlux(), s.getIxx(), s.getIyy(), s.getIxy()
        return psf, sources, footprints

    #def repair(self, *args, **kwargs):
    #    print 'repair: doing nothing'


def getMapper():
    # haha, base directory for data
    database = os.path.join(os.getenv("MEAS_DEBLENDER_DIR"), 'examples', 'data')
    mapper = TractorMapper(basedir=database)
    return mapper

def run(visit, rerun, config):
    mapper = getMapper()
    dataId = { 'visit': visit, 'rerun': rerun }
    rrdir = mapper.getPath('outdir', dataId)
    if not os.path.exists(rrdir):
        print 'Creating directory for ouputs:', rrdir
        os.makedirs(rrdir)
    else:
        print 'Output directory:', rrdir
    io = pipReadWrite.ReadWrite(mapper, ['visit'], config=config)
    #ccdProc = pipProcCcd.ProcessCcd(config=config, Isr=NullISR, Calibrate=MyCalibrate)
    #raws = io.readRaw(dataId)
    #detrends = io.detrends(dataId, config)
    print 'Reading exposure'
    #exposure = io.read('visitim', dataId)
    detrends = []
    exposure = io.inButler.get('visitim', dataId)
    print 'exposure is', exposure
    print 'size', exposure.getWidth(), 'x', exposure.getHeight()
    # debug
    mi = exposure.getMaskedImage()
    #img = mi.getImage()
    #var = mi.getVariance()
    #print 'var at 90,100 is', var.get(90,100)
    #print 'img at 90,100 is', img.get(90,100)
    #print 'wcs is', exposure.getWcs()
    wcs = exposure.getWcs()
    assert wcs
    #print 'ccdProc.run()...'
    # raws = [exposure]
    #exposure, psf, apcorr, brightSources, sources, matches, matchMeta = ccdProc.run(raws, detrends)
    print 'Calibrate()...'
    log = pexLog.getDefaultLog()
    cal = MyCalibrate(config=config, log=log, Photometry=MyPhotometry)
    psf,sources,footprints = cal.run2(exposure)

    print 'Photometry()...'
    phot = pipPhot.Photometry(config=config, log=log)
    sources, footprints = phot.run(exposure, psf)
    print 'sources:', len(sources)
    for s in sources:
        print '  ', s, s.getXAstrom(), s.getYAstrom(), s.getPsfFlux(), s.getIxx(), s.getIyy(), s.getIxy()

    print 'footprints:', footprints
    # oh yeah, baby!
    fps = footprints.getFootprints()
    print len(fps)
    bb = []
    for f in fps:
        print '  Footprint', f
        print '  ', f.getBBox()
        bbox = f.getBBox()
        bb.append((bbox.getMinX(), bbox.getMinY(), bbox.getMaxX(), bbox.getMaxY()))
        print '   # peaks:', len(f.getPeaks())
        for p in f.getPeaks():
            print '    Peak', p
    #print 'psf', psf
    #print 'sources', sources
    #print 'footprints', footprints
    #psf, apcorr, brightSources, matches, matchMeta = self.calibrate(exposure, defects=defects)
    #if self.config['do']['phot']:
    #    sources, footprints = self.phot(exposure, psf, apcorr, wcs=exposure.getWcs())
    #psf, wcs = self.fakePsf(exposure)
    #sources, footprints = self.phot(exposure, psf)
    #sources = self.rephot(exposure, footprints, psf, apcorr=apcorr)
    #model = calibrate['model']
    #fwhm = calibrate['fwhm'] / wcs.pixelScale()
    #size = calibrate['size']
    # psf = afwDet.createPsf(model, size, size, fwhm/(2*math.sqrt(2*math.log(2))))
    #print 'done!'
    print 'writing output...'
    io.write(dataId, psf=psf, sources=sources)
    print 'done!'
    print 'Writing bounding-boxes...'
    io.outButler.put(bb, 'bb', dataId)

    #print 'Writing footprints...'
    #io.outButler.put(fps, 'footprints', dataId)

    # serialize a python version of footprints & peaks
    pyfoots = footprintsToPython(fps)
    print 'Writing py footprints...'
    io.outButler.put(pyfoots, 'pyfoots', dataId)

    return bb




def plots(visit, rerun, config, bb=[]):
    mapper = getMapper()
    dataId = { 'visit': visit, 'rerun': rerun }
    rrdir = mapper.getPath('outdir', dataId)
    if not os.path.exists(rrdir):
        raise RuntimeError('Rerun dir not found: "%s"' % rrdir)
    io = pipReadWrite.ReadWrite(mapper, ['visit'], config=config)
    butler = io.inButler

    import plotSources

    plotSources.plotSources(butler=butler, dataId=dataId,
                            fn='src-v%04i-rr%04i.png' % (visit,rerun),
                            bboxes=bb)


if __name__ == "__main__":
    parser = pipOptions.OptionParser()
    parser.add_option("-r", "--rerun", default=0, dest="rerun", type=int, help='rerun number')
    parser.add_option("-v", "--visit", dest="visit", type=int, default=0, help="visit to run (default=%default)")
    parser.add_option("-p", "--plots", dest="plots", default=False, action='store_true', help='Make plots?')

    default = os.path.join(os.getenv("PIPETTE_DIR"), "policy", "ProcessCcdDictionary.paf")
    overrides = os.path.join(os.getenv("MEAS_DEBLENDER_DIR"), 'examples', 'tractor.paf')
    config, opt, args = parser.parse_args([default, overrides])
    if len(args) > 0 or len(sys.argv) == 1 or opt.rerun is None or opt.visit is None:
        parser.print_help()
        sys.exit(1)
    print 'running visit', opt.visit, 'rerun', opt.rerun
    bb = run(opt.visit, opt.rerun, config)

    if opt.plots:
        plots(opt.visit, opt.rerun, config, bb)
        

