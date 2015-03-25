# Enable HSM shapes (unsetup meas_extensions_shapeHSM to disable)
# 'root' is a SourceMeasurementConfig.
import os
try:
    root.load(os.path.join(os.environ['MEAS_EXTENSIONS_SHAPEHSM_DIR'], 'config', 'enable.py'))
    root.algorithms["shape.hsm.regauss"].deblendNChild = "deblend.nchild"
except Exception as e:
    print "Cannot enable shapeHSM (%s): disabling HSM shape measurements" % (e,)
