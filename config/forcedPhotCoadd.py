import os

root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'apertures.py'))
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'kron.py'))
# Turn off cmodel until latest fixes (large blends, footprint merging, etc.) are in
# root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'cmodel.py'))

# Disable Gaussian mags (which aren't really forced)
root.measurement.algorithms.names -= ["base_GaussianFlux"]
root.measurement.algorithms.names |= ["base_SdssShape"]
root.measurement.slots.instFlux = None
