"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
root.isr.doBias = False
root.isr.doDark = False
root.isr.doWrite = False
