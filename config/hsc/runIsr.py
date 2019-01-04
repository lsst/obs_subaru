"""
HSC-specific overrides for RunIsrTask
"""
import os.path

from lsst.utils import getPackageDir

obsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")

# Load the HSC config/isr.py configuration, but DO NOT retarget the ISR task.
config.isr.load(os.path.join(obsConfigDir, "isr.py"))
