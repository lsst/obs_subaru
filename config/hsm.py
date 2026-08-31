# Enable HSM shapes (unsetup meas_extensions_shapeHSM to disable)
# 'config' is a SourceMeasurementConfig.
try:
    config.load("eups://meas_extensions_shapeHSM/config/enable.py")
    config.plugins["ext_shapeHSM_HsmShapeRegauss"].deblendNChild = "deblend_nChild"
    # Enable debiased moments
    config.plugins.names |= ["ext_shapeHSM_HsmPsfMomentsDebiased"]
except LookupError as e:
    print("Cannot enable shapeHSM (%s): disabling HSM shape measurements" % (e,))
