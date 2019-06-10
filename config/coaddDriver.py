# Load from sub-configurations
import os.path

from lsst.utils import getPackageDir

from lsst.pipe.tasks.assembleCoadd import CompareWarpAssembleCoaddTask
config.assembleCoadd.retarget(CompareWarpAssembleCoaddTask)

# TODO DM-20074: Remove after S19A DRP complete
from lsst.obs.hsc import SubaruMakeCoaddTempExpTask
config.makeCoaddTempExp.retarget(SubaruMakeCoaddTempExpTask)

for sub, filename in (("makeCoaddTempExp", "makeCoaddTempExp"),
                      ("backgroundReference", "backgroundReference"),
                      ("assembleCoadd", "compareWarpAssembleCoadd"),
                      ("detectCoaddSources", "detectCoaddSources")):
    path = os.path.join(getPackageDir("obs_subaru"), "config", filename + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)

config.doBackgroundReference = False

from lsst.pipe.tasks.selectImages import PsfWcsSelectImagesTask
config.select.retarget(PsfWcsSelectImagesTask)

from lsst.pipe.drivers.utils import NullSelectImagesTask
config.assembleCoadd.select.retarget(NullSelectImagesTask)
