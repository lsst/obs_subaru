"""
Subaru-specific overrides for ProcessCcdTask (applied before HSC-specific overrides).
"""
import os.path


for sub in ("isr", "charImage", "calibrate"):
    path = os.path.join(os.path.dirname(__file__), sub + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)
