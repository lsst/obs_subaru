"""
Suprimecam-specific overrides for RunIsrTask
(applied after Subaru overrides in ../processCcd.py)
"""
import os.path


obsConfigDir = os.path.join(os.path.dirname(__file__))

config.isr.load(os.path.join(obsConfigDir, "isr.py"))
