import math
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.pipette.distortion as pipDist
import hsc.meas.match.distest as distest

class HscDistortion(pipDist.CameraDistortion):
    """ Adapter class for the pipette framework. """
    
    def __init__(self, ccd, config):
        """Constructor

        @param ccd Ccd for distortion (sets position relative to center)
        @param config Configuration for distortion
        """
        self.ccd = ccd
        self.pixelSize = ccd.getPixelSize()
        self.transform = ccd.getGlobalTransform()
        self.inverseTransform = self.transform.invert()
        angle = ccd.getOrientation().getNQuarter() * math.pi/2
        self.cos, self.sin = math.cos(angle), math.sin(angle)

    def _rotate(self, x, y, reverse=False):
        sin = - self.sin if reverse else self.sin
        return self.cos * x + self.sin * y, self.cos * y - self.sin * x

    def _distortPosition(self, x, y, direction=None, elevation=60.0, copy=True):
        """Distort/undistort a position.

        @param x X coordinate to distort. pixels from focal plane center.
        @param y Y coordinate to distort
        @param direction "toIdeal" or "toActual"
        @returns Copy of input source with distorted/undistorted coordinates
        """

        if direction == "toIdeal":
            point = self.transform(afwGeom.PointD(x, y))
            x, y = point.getX() / self.pixelSize, point.getY() / self.pixelSize
            distX, distY = distest.getUndistortedPosition(x, y, elevation)
            return self._rotate(distX, distY, reverse=False)

        if direction == "toActual":
            x, y = self._rotate(x, y, reverse=True)
            undistX, undistY = distest.getDistortedPosition(x, y, elevation)
            point = afwGeom.PointD(undistX * self.pixelSize, undistY * self.pixelSize)
            point = self.inverseTransform(point)
            return point.getX(), point.getY()

        raise RuntimeError("unknown distortion direction: %s" % (direction))
    
    # Need to get elevation in here -- CPL
    def actualToIdeal(self, sources, copy=True):
        return self._distortSources(sources, direction="toIdeal", copy=copy)

    def idealToActual(self, sources, copy=True):
        return self._distortSources(sources, direction="toActual",  copy=copy)
