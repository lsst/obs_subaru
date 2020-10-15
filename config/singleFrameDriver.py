"""
Subaru-specific overrides for SingleFrameDriverTask (applied before HSC-specific overrides).
"""
import os.path


config.processCcd.load(os.path.join(os.path.dirname(__file__), "processCcd.py"))
config.ccdKey = 'ccd'
