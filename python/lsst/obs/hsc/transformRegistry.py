from lsst.afw.geom import xyTransformRegistry
import lsst.afw.geom as afwGeom
from lsst.pex.config import Config, Field
from lsst.obs.subaru.subaruLib import DistEstXYTransform
__all__ = ["xyTransformRegistry"]

class DistEstXYTransformConfig(Config):
    elevation = Field(
        doc = """Elevation in degrees to use in constructing the transform""",
        dtype = float,
        default = 45.,
    )
    plateScale = Field(
       doc = """Platescale in arcsec/mm""",
       dtype = float,
    )
def makeDistEstTransform(config):
    elevation = afwGeom.Angle(config.elevation, afwGeom.degrees)
    return DistEstXYTransform(elevation, config.plateScale)
makeDistEstTransform.ConfigClass = DistEstXYTransformConfig
xyTransformRegistry.register("distEst", makeDistEstTransform)
