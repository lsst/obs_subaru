import os
root.calibrate.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'apertures.py'))
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'apertures.py'))
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'kron.py'))
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'cmodel.py'))

root.measurement.slots.instFlux = None

# Enable HSM shapes (unsetup meas_extensions_shapeHSM to disable)
try:
    root.measurement.load(os.path.join(os.environ['MEAS_EXTENSIONS_SHAPEHSM_DIR'], 'config', 'enable.py'))
    root.measurement.algorithms["shape.hsm.regauss"].deblendNChild = "deblend.nchild"
except:
    print "Cannot enable shapeHSM: disabling shear measurement"

root.deblend.maxNumberOfPeaks = 20
