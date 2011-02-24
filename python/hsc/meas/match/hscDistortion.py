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

        # Taken from pipette.distortion.RadialDistortion, but not checked
        # for rotations, etc.

        position = ccd.getCenter()        # Centre of CCD on focal plane
        center = ccd.getSize() / ccd.getPixelSize() / 2.0 # Central pixel

        self.ccd = ccd

        # Pixels from focal plane center to CCD corner
        self.x0 = position.getX() - center.getX()
        self.y0 = position.getY() - center.getY()

    def _distortPosition(self, x, y, direction=None, elevation=30.0, copy=True):
        """Distort/undistort a position.

        @param x X coordinate to distort. pixels from focal plane center.
        @param y Y coordinate to distort
        @param direction "toIdeal" or "toActual"
        @returns Copy of input source with distorted/undistorted coordinates
        """

        if False:
            point = afwGeom.makePointD(x, y)
            point = self.ccd.getPositionFromPixel(point)
            pixelSize = self.ccd.getPixelSize()
            x, y = point.getX() / pixelSize, point.getY() / pixelSize
        else:
            x += self.x0
            y += self.y0

        if direction == "toIdeal":
            distX, distY = distest.getUndistortedPosition(x, y, elevation)
        elif direction == "toActual":
            distX, distY = distest.getDistortedPosition(x, y, elevation)
        else:
            raise RuntimeError("unknown distortion direction: %s" % (direction))

        if False:
            point.setX(distX * pixelSize)
            point.setY(distY * pixelSize)
            point = self.ccd.getPixelFromPosition(point)
            return point.getX(), point.getY()
        else:
            return distX - self.x0, distY - self.y0
    
    # Need to get elevation in here -- CPL
    def actualToIdeal(self, sources, copy=True):
        return self._distortSources(sources, direction="toIdeal", copy=copy)

    def idealToActual(self, sources, copy=True):
        return self._distortSources(sources, direction="toActual",  copy=copy)
