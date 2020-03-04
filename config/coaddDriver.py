# Load from sub-configurations
import os.path

from lsst.utils import getPackageDir

from lsst.pipe.tasks.assembleCoadd import AssembleCoaddTask
config.assembleCoadd.retarget(AssembleCoaddTask)

from lsst.obs.hsc import PureNoiseMakeCoaddTempExpTask
config.makeCoaddTempExp.retarget(PureNoiseMakeCoaddTempExpTask)

for sub, filename in (("makeCoaddTempExp", "makeCoaddTempExp"),
                      ("backgroundReference", "backgroundReference"),
                      ("assembleCoadd", "assembleCoadd"),
                      ("detectCoaddSources", "detectCoaddSources")):
    path = os.path.join(getPackageDir("obs_subaru"), "config", filename + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)

config.doBackgroundReference = False

from lsst.pipe.tasks.selectImages import PsfWcsSelectImagesTask
config.select.retarget(PsfWcsSelectImagesTask)

from lsst.pipe.drivers.utils import NullSelectImagesTask
config.assembleCoadd.select.retarget(NullSelectImagesTask)
