import os.path

config.measurement.load(os.path.join(os.path.dirname(__file__), "apertures.py"))
config.measurement.load(os.path.join(os.path.dirname(__file__), "kron.py"))

config.measurement.slots.gaussianFlux = None
config.doApplyExternalPhotoCalib = True
config.doApplyExternalSkyWcs = True
config.doApplySkyCorr = True
