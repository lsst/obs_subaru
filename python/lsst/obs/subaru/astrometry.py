#!/usr/bin/env python

# NOTE: THIS MODULE SHOULD NOT BE IMPORTED BY __init__.py, AS IT IMPORTS THE OPTIONAL hscAstrom PACKAGE.

import lsst.pex.config as pexConfig
import lsst.daf.base as dafBase
import lsst.pipe.base as pipeBase
import lsst.meas.astrom as measAstrom
import lsst.pipe.tasks.astrometry as ptAstrometry
import hsc.meas.astrom as hscAstrom

class SubaruAstrometryConfig(ptAstrometry.AstrometryConfig):
    solver = pexConfig.ConfigField(
        dtype=hscAstrom.TaburAstrometryConfig,
        doc = "Configuration for the Tabur astrometry solver"
        )
    failover = pexConfig.Field(dtype=bool, doc="Fail over from hscAstrom to meas_astrom?", default=False)

    allowFailedAstrometry = pexConfig.Field(dtype=bool, doc="Proceed even if astrometry fails?", default=False)

# Use hsc.meas.astrom, failing over to lsst.meas.astrom
class SubaruAstrometryTask(ptAstrometry.AstrometryTask):
    ConfigClass = SubaruAstrometryConfig
    @pipeBase.timeMethod
    def astrometry(self, exposure, sources, llc=(0,0), size=None):
        """Solve astrometry to produce WCS

        @param exposure Exposure to process
        @param sources Sources
        @param llc Lower left corner (minimum x,y)
        @param size Size of exposure
        @return Star matches, match metadata
        """
        assert exposure, "No exposure provided"

        self.log.log(self.log.INFO, "Solving astrometry")

        wcs = exposure.getWcs()
        if wcs is None:
            self.log.log(self.log.WARN, "Unable to use hsc.meas.astrom; reverting to lsst.meas.astrom")
            return ptAstrometry.AstrometryTask.astrometry(exposure, sources, llc=llc, size=size)

        if size is None:
            size = (exposure.getWidth(), exposure.getHeight())

        astrom = None
        try:
            if not self.astrometer or not isinstance(self.astrometer, hscAstrom.TaburAstrometry):
                self.astrometer = hscAstrom.TaburAstrometry(self.config.solver, log=self.log)
            astrom = self.astrometer.determineWcs(sources, exposure)
            if astrom is None:
                raise RuntimeError("hsc.meas.astrom failed to determine the WCS")
        except Exception, e:
            self.log.log(self.log.WARN, "hsc.meas.astrom failed (%s)" % e)
            if self.config.failover:
                self.log.info("Failing over to lsst.meas.astrom....")
                # N.b. this will replace the previous astrometer with a meas_astrom one
                self.astrometer = measAstrom.Astrometry(self.config.solver, log=self.log)
                astrom = self.astrometer.determineWcs(sources, exposure)

        if astrom is None and self.config.allowFailedAstrometry:
            matches = []
            matchMeta = dafBase.PropertySet()
        else:
            if astrom is None:
                raise RuntimeError("Unable to solve astrometry for %s", exposure.getDetector().getId())

            wcs = astrom.getWcs()
            matches = astrom.getMatches()
            matchMeta = astrom.getMatchMetadata()
            if matches is None or len(matches) == 0:
                raise RuntimeError("No astrometric matches for %s", exposure.getDetector().getId())
            self.log.log(self.log.INFO, "%d astrometric matches for %s" % \
                         (len(matches), exposure.getDetector().getId()))
            exposure.setWcs(wcs)

        # Apply WCS to sources
        for source in sources:
            distorted = source.get(self.centroidKey)
            sky = wcs.pixelToSky(distorted.getX(), distorted.getY())
            source.setCoord(sky) 

        self.display('astrometry', exposure=exposure, sources=sources, matches=matches)

        metadata = exposure.getMetadata()
        for key in self.metadata.names():
            val = self.metadata.get(key)
            if isinstance(val, tuple):
                self.log.log(self.log.DEBUG, "Value of %s is a tuple: %s" % (key, val))
                val = val[-1]

            try:
                if isinstance(val, int) and val > 0x8fffff:
                    metadata.setLong(key, val)
                else:
                    metadata.set(key, val)
            except Exception, e:
                self.log.log(self.log.WARN, "Value of %s == %s is invalid; %s" % (key, val, e))

        metadata.set('NOBJ_BRIGHT', len(sources))
        metadata.set('NOBJ_MATCHED', len(matches))
        metadata.set('WCS_NOBJ', len(matches))

        return matches, matchMeta

    def refitWcs(self, exposure, sources, matches):
        sip = ptAstrometry.AstrometryTask.refitWcs(self, exposure, sources, matches)
        order = self.config.solver.sipOrder if self.config.solver.calculateSip else 0
        rms = sip.getScatterOnSky().asArcseconds() if sip else -1
        metadata = exposure.getMetadata()
        metadata.set('WCS_SIPORDER', order)
        metadata.set('WCS_RMS', rms)
        return sip
