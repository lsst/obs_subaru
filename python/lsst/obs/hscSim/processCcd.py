from lsst.pex.config import ListField
from hsc.pipe.tasks.processCcd import SubaruProcessCcdConfig, SubaruProcessCcdTask

class HscProcessCcdConfig(SubaruProcessCcdConfig):
    ignoreCcdList = ListField(dtype=int, default=range(104, 112), doc="List of CCD numbers to ignore")

class HscProcessCcdTask(SubaruProcessCcdTask):
    ConfigClass = HscProcessCcdConfig

    def run(self, sensorRef):
        ccdNum = sensorRef.dataId['ccd']
        if ccdNum in self.config.ignoreCcdList:
            raise RuntimeError("Refusing to process CCD %d: in ignoreCcdList" % ccdNum)
        return super(HscProcessCcdTask, self).run(sensorRef)
