"""
Subaru-specific overrides for ProcessCcdWithFakesTask (applied before SuprimeCam- and HSC-specific overrides).
This is taken from, and needs to be kept updated with, obs_subaru/config/calibrate.py. This is to ensure that
the outputs from both processCcdWithFakes and calibrate are consistent.
"""
import os.path

from lsst.utils import getPackageDir
from lsst.meas.algorithms import ColorLimit

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config")

bgFile = os.path.join(ObsConfigDir, "background.py")

# Cosmic rays and background estimation
config.detection.background.load(bgFile)

# Set to match defaults currently used in HSC production runs (e.g. S15B)
config.catalogCalculation.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95

# Detection
config.detection.isotropicGrow = True

config.measurement.load(os.path.join(ObsConfigDir, "apertures.py"))
config.measurement.load(os.path.join(ObsConfigDir, "kron.py"))
config.measurement.load(os.path.join(ObsConfigDir, "hsm.py"))

# Deblender
config.deblend.maxFootprintSize = 0
config.deblend.maskLimits["NO_DATA"] = 0.25  # Ignore sources that are in the vignetted region
config.deblend.maxFootprintArea = 10000

config.measurement.plugins.names |= ["base_Jacobian", "base_FPPosition"]
