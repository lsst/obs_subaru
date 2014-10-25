import os
config.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'apertures.py'))
config.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'kron.py'))
config.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'cmodel.py'))
config.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hsm.py'))

config.measurement.slots.instFlux = None

config.deblend.maxNumberOfPeaks = 20

config.deblend.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "deblend.py"))
