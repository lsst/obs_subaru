import os.path

from lsst.utils import getPackageDir

config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "apertures.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "kron.py"))
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "cmodel.py"))

config.measurement.slots.instFlux = None

config.measurement.plugins['base_PixelFlags'].masksFpAnywhere.append('CLIPPED')
config.measurement.plugins['base_PixelFlags'].masksFpCenter.append('BRIGHT_OBJECT')
config.measurement.plugins['base_PixelFlags'].masksFpAnywhere.append('BRIGHT_OBJECT')

config.catalogCalculation.plugins.names = ["base_ClassificationExtendedness"]
config.measurement.slots.psfFlux = "base_PsfFlux"
