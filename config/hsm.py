# Enable HSM shapes (unsetup meas_extensions_shapeHSM to disable)
# 'config' is a SourceMeasurementConfig.
import os
try:
    config.load(os.path.join(os.environ['MEAS_EXTENSIONS_SHAPEHSM_DIR'], 'config', 'enable.py'))
    config.algorithms["shape.hsm.regauss"].deblendNChild = "deblend.nchild"
    config.slots.shape = "shape.hsm.moments"
except Exception as e:
    print "Cannot enable shapeHSM (%s): disabling HSM shape measurements" % (e,)
