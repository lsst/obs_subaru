# Load from sub-configurations
import os.path

from lsst.pipe.tasks.assembleCoadd import CompareWarpAssembleCoaddTask
config.assembleCoadd.retarget(CompareWarpAssembleCoaddTask)

for sub, filename in (("makeCoaddTempExp", "makeCoaddTempExp"),
                      ("backgroundReference", "backgroundReference"),
                      ("assembleCoadd", "compareWarpAssembleCoadd"),
                      ("detectCoaddSources", "detectCoaddSources")):
    path = os.path.join(os.path.dirname(__file__), filename + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)

config.doBackgroundReference = False

from lsst.pipe.tasks.selectImages import PsfWcsSelectImagesTask
config.select.retarget(PsfWcsSelectImagesTask)

from lsst.pipe.drivers.utils import NullSelectImagesTask
config.assembleCoadd.select.retarget(NullSelectImagesTask)
