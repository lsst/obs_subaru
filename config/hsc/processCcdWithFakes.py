"""
HSC-specific overrides for ProcessCcdWithFakesTask
(applied after Subaru overrides in ../processCcdWithFakes.py).
Should be kept up to date with config/hsc/calibrate.py so that
processCcd and processCcdWithFakes produce the same output.
"""
import os.path


ObsConfigDir = os.path.dirname(__file__)

config.measurement.plugins["base_Jacobian"].pixelScale = 0.168
