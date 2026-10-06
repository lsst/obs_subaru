import lsst.obs.subaru.filterFraction  # noqa F401: required for FilterFraction below

config.measurement.load("apertures.py")
config.measurement.load("kron.py")
config.measurement.load("convolvedFluxes.py")
config.measurement.load("hsm.py")
config.load("cmodel.py")

# This isn't good!  There appears to be no way to configure the
# base_PixelFlags measurement algorithm based on a configuration parameter;
# see DM-4159 for a discussion.  The name BRIGHT_MASK must match
# `assembleCoaddConfig.brightObjectMaskName`.
#
config.measurement.plugins["base_PixelFlags"].masksFpCenter.append("BRIGHT_OBJECT")
config.measurement.plugins["base_PixelFlags"].masksFpAnywhere.append("BRIGHT_OBJECT")

config.measurement.plugins.names |= ["base_InputCount"]
config.measurement.plugins.names.add("subaru_FilterFraction")
