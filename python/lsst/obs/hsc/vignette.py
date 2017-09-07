from __future__ import absolute_import, division, print_function

from lsst.pex.config import Config, Field


class VignetteConfig(Config):
    """
    Settings to define vignetting pattern
    """
    xCenter = Field(dtype=float, default=0, doc="Center of vignetting pattern, in x (focal plane coords)")
    yCenter = Field(dtype=float, default=0, doc="Center of vignetting pattern, in y (focal plane coords)")
    radius = Field(dtype=float, default=18300, doc="Radius of vignetting pattern, in focal plane coords",
                   check=lambda x: x >= 0)
