import os

root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'apertures.py'))
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'kron.py'))
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'cmodel.py'))

# Disable Gaussian mags (which aren't really forced)
root.measurement.algorithms.names -= ["base_GaussianFlux"]
root.measurement.slots.instFlux = None
