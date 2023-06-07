import os.path

config.measurement.load(os.path.join(os.path.dirname(__file__), "apertures.py"))

# We only need the 12 pixel aperture for subsequent measurements.
config.measurement.plugins["base_CircularApertureFlux"].radii = [12.0]

config.measurement.slots.gaussianFlux = None
config.doApplyExternalPhotoCalib = True
config.doApplyExternalSkyWcs = True
config.doApplySkyCorr = True
