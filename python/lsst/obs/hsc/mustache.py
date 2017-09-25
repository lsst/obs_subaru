from __future__ import absolute_import, division, print_function

import numpy as np

import lsst.afw.geom
import lsst.afw.cameraGeom

from lsst.pex.config import Config, Field, ListField, ConfigDictField, ConfigurableField
from lsst.pipe.base import Task, Struct, CmdLineTask, ButlerInitializedTaskRunner
from lsst.afw.image import abMagFromFlux
from lsst.meas.algorithms import LoadIndexedReferenceObjectsTask


class MustacheParameters(Config):
    filterFactor = Field(dtype=float, doc="Filter dependency of ghost location")
    magLimit = Field(dtype=float, doc="Limiting magnitude to calculate ghost prediction")
    arc = ListField(dtype=float, doc="Parameters describing center and radius of arc. "
                                     "The first three parameters are the constant, linear and quadratic "
                                     "polynomial terms for the center. The second three parameters are "
                                     "the constant, linear and quadratic polynomial terms for the radius.")

    def validate(self):
        Config.validate(self)
        if len(self.arc) != 6:
            raise RuntimeError("Wrong size for arc: %d instead of 6" % len(self.arc))

    @classmethod
    def fromValues(cls, filterFactor, magLimit, arc):
        self = cls()
        self.filterFactor = filterFactor
        self.magLimit = magLimit
        self.arc = arc
        return self


gArc = [-4467.9759580157, 10022.1277915657, -5156.3556611527,
        -4375.9715493905, 9717.6993803237, -5203.2424778605]
rArc = [-4354.2575893763, 9762.9766653532, -5011.1243975288,
        -4263.2230039887, 9457.4516871365, -5057.2762518308]
iArc = [-4243.1837126908, 9517.3512135240, -4876.3237965836,
        -4154.5095389959, 9215.4140278639, -4924.2923196315]
zArc = [-3966.2315539829, 8920.7199277389, -4555.9567436784,
        -3878.4529585585, 8619.7241050446, -4604.3603386145]
yArc = [-3723.5919271089, 8403.7641770854, -4281.0159171765,
        -3637.0352789862, 8104.8245491120, -4330.4583627761]

parameters = {
    'g': MustacheParameters.fromValues(1.002, 13.0, gArc),
    'r': MustacheParameters.fromValues(1.0015, 13.5, rArc),
    'i': MustacheParameters.fromValues(1.001, 13.5, iArc),
    'z': MustacheParameters.fromValues(0.9995, 13.0, zArc),
    'y': MustacheParameters.fromValues(1.000, 13.0, yArc),
    'N921': MustacheParameters.fromValues(1.0005, 12.0, yArc),
    # No official filterFactor values for the below; assuming unity
    'N656': MustacheParameters.fromValues(1.0, 13.5, rArc),
    'N816': MustacheParameters.fromValues(1.0, 13.5, zArc),
    'N387': MustacheParameters.fromValues(1.0, 13.0, gArc),
}


class HscMustacheConfig(Config):
    """Config class for predicting mustache ghosts"""
    xOffset = Field(dtype=float, default=12.78, 
                    doc="Offset of mother-plate coord x to HSC FOV center")
    yOffset = Field(dtype=float, default=57.74, 
                    doc="Offset of mother-plate coord y to HSC FOV center")
    samplingStep = Field(dtype=float, default=0.25, doc="Sampling step to cover each ghost arc")
    sampleSize = Field(dtype=int, default=100, doc="Size of mustache sample Footprint")
    minRadius = Field(dtype=float, default=0.8,
                      doc="Minimum radius to extract reference sources as ghosts origin")
    maxRadius = Field(dtype=float, default=1.5, 
                      doc="Maximum radius to extract reference sources as ghosts origin")
    minMustacheArea = Field(dtype=float, default=10000.0,
                            doc="Minimum area for a detection to be masked as a mustache")
    maskName = Field(dtype=str, default="GHOST", doc="Name of mask plane to give mustaches")
    parameters = ConfigDictField(keytype=str, itemtype=MustacheParameters, doc="Parameters by filter",
                                 default=parameters)


class HscMustacheTask(Task):
    """Task to identify 'mustache' ghosts.

    'Mustache' ghosts are reflections of bright stars just off the field
    of view. The result is something like a pencil mustache
    (https://en.wikipedia.org/wiki/Pencil_moustache ; like that worn by Errol
    Flynn): two short arcs either side of an empty space. They are only seen
    around the edge of the field of view.

    The original code for calculating the ghost locations was provided by 
    Dr. Satoshi Kawanomoto, Subaru Telescope/HSC project, NAOJ.
    An initial implementation of this Task was made by Hisanori Furusawa (NAOJ).
    """

    _DefaultName = 'mustache'
    ConfigClass = HscMustacheConfig

    _pixelScaleMm = 0.015  # Pixel scale: mm/pixel
    _pixelScaleArcsec = 0.168  # Pixel scale: arcsec/pixel

    def outerEdgeAngle(self, radius):
        """Outer angle of the arc

        The parameters here come from Kawanomoto-san.

        Parameters
        ----------
        radius : `float`
            Radius of star from boresight, degrees.

        Returns
        -------
        outer : `float`
            Angle of the outer edge of the arc, degrees.
        """
        if radius < 0.8831:
            return 0.0
        if radius < 0.9368:
            return 46.2985*np.sqrt(radius - 0.8831)
        return -2.6788646406597 + 14.311535418553*radius

    def innerEdgeAngle(self, radius):
        """Inner angle of the arc

        The parameters here come from Kawanomoto-san.

        Parameters
        ----------
        radius : `float`
            Radius of star from boresight, degrees.

        Returns
        -------
        inner : `float`
            Angle of the inner edge of the arc, degrees.
        """
        if radius < 0.96464:
            return -243.86606061927*(radius - 0.93238966579931)**2.0 + 2.4636017943593
        return 66.2061*np.sqrt(radius - 0.963531)

    def arcParameters(self, radius, filterName):
        """Parameters for the arc

        Parameters
        ----------
        radius : `float`
            Radius of star from boresight, degrees.
        filterName : `str`
            Name of the filter.

        Returns
        -------
        arc : `lsst.pipe.base.Struct`
            Has members 'center' and 'radius' (both `float`).
        """
        parameters = self.config.parameters[filterName].arc
        assert len(parameters) == 6
        return Struct(center=parameters[0] + parameters[1]*radius + parameters[2]*radius**2,
                      radius=parameters[3] + parameters[4]*radius + parameters[5]*radius**2)

    def mustacheForStar(self, star, boresight, camera, detector, filterName, rotAngle):
        """Return mustache positions for a star

        The prescription for obtaining the focal plane coordiantes of
        mustaches comes from Kawanomoto-san by way of Furusawa-san.

        Parameters
        ----------
        star : `lsst.afw.coord.IcrsCoord`
            Coordinates of the star of interest.
        boresight : `lsst.afw.coord.IcrsCoord`
            Coordinates of the boresight.
        camera : `lsst.afw.cameraGeom.Camera`
            HSC camera description.
        detector : `lsst.afw.cameraGeom.Detector`
            CCD for which to report positions.
        filterName : `str`
            Name of the filter.
        rotAngle : `lsst.afw.geom.Angle`
            Rotator angle (from "INR-STR" in the header).

        Returns
        -------
        ccdCoords : `list` of `lsst.afw.geom.Point2D`
            Coordinates on the CCD of the mustache.
        """
        theta, radius = star.getOffsetFrom(boresight)
        inner = self.innerEdgeAngle(radius.asDegrees())
        outer = self.outerEdgeAngle(radius.asDegrees())
        if inner >= outer:
            return []
        arc = self.arcParameters(radius.asDegrees(), filterName)

#        import pdb;pdb.set_trace()

        # XXX
        if False:
            samples = np.deg2rad(np.arange(inner, outer, 0.25))
        else:
            samples = 0.0
        x = arc.center - arc.radius*np.cos(samples)
        y = arc.radius*np.sin(samples)
        if False:
            rot = np.deg2rad(90.0 + theta.asDegrees() + rotAngle.asDegrees())
        else:
            rot = np.arange(0, 2*np.pi, 0.05)
        xRightArc = x*np.cos(rot) - y*np.sin(rot)
        yRightArc = y*np.cos(rot) + x*np.sin(rot)
        xLeftArc = x*np.cos(rot) + y*np.sin(rot)
        yLeftArc = -y*np.cos(rot) + x*np.sin(rot)
        xArc = np.concatenate((xRightArc, xLeftArc))
        yArc = np.concatenate((yRightArc, yLeftArc))
        adjustment = self.config.parameters[filterName].filterFactor
        xFocalPlane = (xArc*np.cos(-np.pi/2.0) + yArc*np.sin(-np.pi/2.0))*adjustment/self._pixelScaleMm
        yFocalPlane = (-xArc*np.sin(-np.pi/2.0) + yArc*np.cos(-np.pi/2.0))*adjustment/self._pixelScaleMm
        xFocalPlane += self.config.xOffset
        yFocalPlane += self.config.yOffset

        # The minus signs here are necessary for things to work on ccd=69 with new cameraGeom.
        # Not yet sure if they should be a function of the detector or not.
        fpCoords = [lsst.afw.geom.Point2D(-x, -y) for x, y in zip(xFocalPlane, yFocalPlane)]
        transform = detector.getTransform(lsst.afw.cameraGeom.FOCAL_PLANE,
                                          detector.makeCameraSys(lsst.afw.cameraGeom.PIXELS))
        ccdCoords = transform.applyForward(fpCoords)


        ### XXX correct mustache coordinates using the difference between where
        ### boresight+focal plane model says the center of the CCD should be and
        ### where the Wcs says it is.
        ### Probably only need to correct the star position at the beginning.
        # ccdCenter = 0.5*ccdExtent
        # centerCoord = wcs.pixelToSky(ccdCenter)
        # centerFp = transform.applyReverse(ccdCenter)
        # expected = boresight.offset(self._pixelScaleArcsec*somethingFrom(centerFp))
        # offset = centerCoord.getOffsetFrom(expected)
        # star.offset(offset)

        return ccdCoords

    def getStars(self, catalog, fluxField, boresight, filterName, offset=None):
        """Return a Catalog containing only stars of interest

        Those are stars that are bright enough and in the right place.

        The output catalog shares memory (the underlying Table) with the
        input catalog.

        Parameters
        ----------
        catalog : `lsst.afw.table.BaseCatalog`
            Catalog of stars.
        boresight : `lsst.afw.coord.IcrsCoord`
            Coordinates of the boresight.
        filterName : `str`
            Name of the filter.

        Returns
        -------
        output : `lsst.afw.table.BaseCatalog`
            Catalog of stars of interest.
        """

        # XXX
        suspect = lsst.afw.coord.IcrsCoord(149.984689*lsst.afw.geom.degrees,
                                           3.160113*lsst.afw.geom.degrees)

        output = type(catalog)(catalog.getTable())
        fluxKey = catalog.schema[fluxField].asKey()
        minRadius = self.config.minRadius*lsst.afw.geom.degrees
        maxRadius = self.config.maxRadius*lsst.afw.geom.degrees
        for row in catalog:
            mag = abMagFromFlux(row[fluxKey])
            if not np.isfinite(mag) or mag > self.config.parameters[filterName].magLimit:
                continue
            radius = row.getCoord().angularSeparation(boresight)
            if radius < minRadius or radius > maxRadius:
                continue

            # XXX
            if row.getCoord().angularSeparation(suspect) > 10*lsst.afw.geom.arcseconds:
                continue

            if offset is not None:
                print("Before: %s" % row.getCoord())
                coord = row.getCoord()
                coord.offset(*offset)
                row.setCoord(coord)
                print("After: %s" % row.getCoord())

            output.append(row)
        return output

    def maskMustaches(self, mask, mustaches):
        """Mask sources that overlap a mustache

        Pixels that are already DETECTED and overlap a mustache will be
        masked with the `HscMustacheConfig.maskName`.

        Parameters
        ----------
        mask : `lsst.afw.image.Mask`
            Mask in which to label mustaches.
        mustaches : `list` of `list` of `lsst.afw.geom.Point2D`
            List of mustache sample positions for each source star.
        """
        detected = mask.getPlaneBitMask("DETECTED")
        mask.addMaskPlane(self.config.maskName)
        maskVal = mask.getPlaneBitMask(self.config.maskName)
        baseSpans = lsst.afw.geom.SpanSet.fromShape(self.config.sampleSize)
        footprints = lsst.afw.geom.SpanSet.fromMask(mask, detected).split()
        footprints = [fp for fp in footprints if fp.getArea() > self.config.minMustacheArea]
        for mm in mustaches:
            for point in mm:
                spans = baseSpans.shiftedBy(int(point.getX()), int(point.getY()))
                for fp in footprints:
                    if fp.overlaps(spans):
                        fp.setMask(mask, maskVal)

    def isAffected(self, detector):
        """Return whether the detector is potentially affected by mustaches

        Checks whether the detector is on the edge of the field of view.

        This list errs on the side of caution by including some CCDs which may
        be too distant from the edge of the field of view to have mustaches.

        Parameters
        ----------
        detector : `lsst.afw.cameraGeom.Detector`
            Detector of interest.

        Returns
        -------
        affected : `bool`
            Whether the detector is potentially affected by mustaches.
        """
        return detector.getId() in (0, 1, 2, 3, 4, 5, 8, 9, 10, 15, 16, 21, 22, 23, 28, 29, 30, 37, 38, 45,
                                    46, 53, 54, 61, 62, 69, 70, 71, 76, 77, 78, 83, 84, 89, 90, 91, 94, 95,
                                    96, 97, 98, 99, 100, 101, 102, 103)

    def run(self, exposure, catalog, fluxField, camera):
        """Identify and mask mustaches

        Parameters
        ----------
        exposure : `lsst.afw.image.Exposure`
            Exposure to examine.
        catalog : `lsst.afw.table.BaseCatalog`
            Catalog of stars.
        fluxField : `str`
            Name of catalog field containing the relevant flux.
        camera : `lsst.afw.cameraGeom.Camera`
            HSC camera description.
        """
        boresight = exposure.getInfo().getVisitInfo().getBoresightRaDec()
        rotAngle = exposure.getMetadata().get("INR-STR")*lsst.afw.geom.degrees
        filterName = exposure.getFilter().getName()
        detector = exposure.getDetector()



        # XXX Tweak coordinates to compensate for difference between claimed boresight and actual boresight
        ccdCenter = lsst.afw.geom.Box2D(detector.getBBox()).getCenter()
        wcsCoord = exposure.getWcs().pixelToSky(ccdCenter)
        transform = detector.getTransform(lsst.afw.cameraGeom.FOCAL_PLANE,
                                          detector.makeCameraSys(lsst.afw.cameraGeom.PIXELS))
        fpCenter = transform.applyInverse(ccdCenter)
        angle = np.arctan2(fpCenter.getY(), fpCenter.getX())*lsst.afw.geom.radians
        angle -= exposure.getInfo().getVisitInfo().getBoresightRotAngle()
        distance = np.hypot(*fpCenter)
        reckonCoord = boresight.clone()
        reckonCoord.offset(-angle, distance*self._pixelScaleArcsec*lsst.afw.geom.arcseconds)
        offset = wcsCoord.getOffsetFrom(reckonCoord)
        print(offset[0].asDegrees(), offset[1].asDegrees())
#        offset = (offset[0] + 180*lsst.afw.geom.degrees, offset[1])
        reckonCoord.offset(*offset)
        print(wcsCoord)
        print(reckonCoord)
        offset = None
#        import pdb;pdb.set_trace()


        if not self.isAffected(detector):
            self.log.info("Detector %d is not affected by mustache ghosts", detector.getId())
            return
        stars = self.getStars(catalog, fluxField, boresight, filterName, offset)
        if len(stars) == 0:
            self.log.info("No stars producing mustache ghosts")
            return
        mustaches = [self.mustacheForStar(ss.getCoord(), boresight, camera, detector, filterName, rotAngle)
                     for ss in stars]
        self.maskMustaches(exposure.getMaskedImage().getMask(), mustaches)
        self.log.info("Masked %d mustache ghosts", len(mustaches))
        return mustaches

    def plot(self, display, exposure, mustaches):
        """Plot the results

        Parameters
        ----------
        display : `lsst.afw.display.Display`
            Display on which to plot.
        exposure : `lsst.afw.image.Exposure`
            Exposure with masked mustaches.
        mustaches : `list` of `list` of `lsst.afw.geom.Point2D`
            List of mustache sample positions for each source star.
        """        
        display.mtv(exposure)
        for mm in mustaches:
            for point in mm:
                display.dot("o", point.getX(), point.getY(), size=self.config.sampleSize)


class TestMustacheConfig(Config):
    mustaches = ConfigurableField(target=HscMustacheTask, doc="Task to find mustaches")
    refObjLoader = ConfigurableField(target=LoadIndexedReferenceObjectsTask, doc="Reference object loader")
    display = Field(dtype=str, default="ds9", doc="Display backend to use")

    def setDefaults(self):
        Config.setDefaults(self)
        import os
        from lsst.utils import getPackageDir
        self.refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))
        self.refObjLoader.ref_dataset_name = "ps1_pv3_3pi_20170110"

class TestMustacheTask(CmdLineTask):
    """Command-line driven test code for HscMustacheTask

    Loads the reference catalog, finds and masks mustaches
    and then plots the results.

    Expects to find an existing 'calexp'.
    """
    ConfigClass = TestMustacheConfig
    RunnerClass = ButlerInitializedTaskRunner
    _DefaultName = "mustache"

    def __init__(self, butler, **kwargs):
        CmdLineTask.__init__(self, **kwargs)
        self.makeSubtask("mustaches")
        self.makeSubtask("refObjLoader", butler=butler)

    def loadReferenceSources(self, exposure):
        """Load reference catalog

        Parameters
        ----------
        exposure : `lsst.afw.image.Exposure`
            Exposure containing metadata for loading sources.

        Returns
        -------
        catalog : `lsst.afw.image.BaseCatalog`
            Source catalog.
        """
#        center = exposure.getInfo().getVisitInfo().getBoresightRaDec()
#        radius = self.config.mustaches.maxRadius*lsst.afw.geom.degrees

        detector = exposure.getDetector()
        ccdCenter = lsst.afw.geom.Box2D(detector.getBBox()).getCenter()
        center = exposure.getWcs().pixelToSky(ccdCenter)
        radius = 10*lsst.afw.geom.Extent2D(detector.getBBox().getDimensions()).computeNorm()*self.mustaches._pixelScaleArcsec*lsst.afw.geom.arcseconds  # ROUGH!

        filterName = exposure.getFilter().getName()
        return self.refObjLoader.loadSkyCircle(center, radius, filterName)

    def run(self, dataRef):
        """Mask mustaches and inspect results interactively

        Entry point for `CmdLineTask`.

        Parameters
        ----------
        dataRef : `lsst.daf.persistence.ButlerDataRef`
            Data reference for calexp.
        """
        exposure = dataRef.get("calexp")
        loadResult = self.loadReferenceSources(exposure)
        camera = dataRef.get("camera")

        loadResult.refCat.writeFits("catalog.fits")

        mustaches = self.mustaches.run(exposure, loadResult.refCat, loadResult.fluxField, camera)

        import lsst.afw.display
        display = lsst.afw.display.getDisplay(frame=1, backend=self.config.display)
        self.mustaches.plot(display, exposure, mustaches)

    def _getConfigName(self):
        """Disable config persistence"""
        return None

    def _getMetadataName(self):
        """Disable metadata persistence"""
        return None


if __name__ == "__main__":
    TestMustacheTask.parseAndRun()
