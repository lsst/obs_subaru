# Load from sub-configurations
import os.path


for sub, filename in (("makeCoaddTempExp", "makeCoaddTempExp"),
                      ("backgroundReference", "backgroundReference"),
                      ("assembleCoadd", "compareWarpAssembleCoadd"),
                      ("detectCoaddSources", "detectCoaddSources")):
    path = os.path.join(os.path.dirname(__file__), filename + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)
