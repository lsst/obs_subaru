"""
HSC-specific overrides for RunIsrTask
"""
import os.path


obsConfigDir = os.path.dirname(__file__)

# Load the HSC config/isr.py configuration, but DO NOT retarget the ISR task.
config.isr.load(os.path.join(obsConfigDir, "isr.py"))
