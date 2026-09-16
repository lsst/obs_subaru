config.measurement.load("apertures.py")

# We only need the 12 pixel aperture for subsequent measurements.
config.measurement.plugins["base_CircularApertureFlux"].radii = [12.0]

config.measurement.slots.gaussianFlux = None
config.doApplySkyCorr = True
