"""
SuprimeCam-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
from lsst.obs.subaru.isr import SuprimeCamIsrTask

root.isr.retarget(SuprimeCamIsrTask)  # custom task that adds guider correction
root.isr.doBias = False
root.isr.doDark = False
root.isr.doWrite = False

# crosstalk coefficients for SuprimeCam, as crudely measured by RHL
root.isr.crosstalkCoeffs.values = [
     0.00e+00, -8.93e-05, -1.11e-04, -1.18e-04,
    -8.09e-05,  0.00e+00, -7.15e-06, -1.12e-04,
    -9.90e-05, -2.28e-05,  0.00e+00, -9.64e-05,
    -9.59e-05, -9.85e-05, -8.77e-05,  0.00e+00,
    ]

# nonlinearity for SuprimeCam
root.isr.linearizationCoefficient = 2.5e-7
