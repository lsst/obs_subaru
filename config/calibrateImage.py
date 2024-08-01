import math
import os.path

from lsst.meas.algorithms import ColorLimit

config_dir = os.path.dirname(__file__)

# Maintain characterizeImage defaults while we work to understand differences
# between this new Task and older runs.
config.install_simple_psf.fwhm = 1.5*2.0*math.sqrt(2.0*math.log(2.0))
config.psf_detection.thresholdType = "stdev"

# Overrides to improved astrometry matching.
config.astrometry.doFiducialZeroPointCull = True
config.astrometry.load(os.path.join(config_dir, "fiducialZeroPoint.py"))

# Use PS1 for both astrometry and photometry.
config.connections.astrometry_ref_cat = "ps1_pv3_3pi_20170110"
config.astrometry_ref_loader.load(os.path.join(config_dir, "filterMap.py"))
# Use the filterMap instead of the "any" filter (as is used for Gaia).
config.astrometry_ref_loader.anyFilterMapsToThis = None
config.photometry_ref_loader.load(os.path.join(config_dir, "filterMap.py"))

# Use colorterms for photometric calibration, with color limits on the refcat.
config.photometry.applyColorTerms = True
config.photometry.photoCatName = "ps1_pv3_3pi_20170110"
colors = config.photometry.match.referenceSelection.colorLimits
colors["g-r"] = ColorLimit(primary="g_flux", secondary="r_flux", minimum=0.0)
colors["r-i"] = ColorLimit(primary="r_flux", secondary="i_flux", maximum=0.5)
config.photometry.colorterms.load(os.path.join(config_dir, "colorterms.py"))

# Exposure summary stats
config.compute_summary_stats.load(os.path.join(config_dir, "computeExposureSummaryStats.py"))
