from lsst.pex.config import Config, ConfigurableField
from lsst.pipe.base import CmdLineTask
from lsst.meas.algorithms import SourceDeblendTask
import lsst.afw.table as afwTable

class MyConfig(Config):
    deblend = ConfigurableField(target=SourceDeblendTask, doc="Deblender")

class MyTask(CmdLineTask):
    _DefaultName = "my"
    ConfigClass = MyConfig

    def _getConfigName(self):
        return None

    def __init__(self, *args, **kwargs):
        super(MyTask, self).__init__(*args, **kwargs)
        # Don't really care about schema because the sources we read already have that,
        # but the SourceDeblendTask constructor requires it.
        self.schema = kwargs.pop("schema", None)
        if self.schema is None:
            self.schema = afwTable.SourceTable.makeMinimalSchema()
        self.makeSubtask("deblend", schema=self.schema)

    def run(self, dataRef):
        calexp = dataRef.get("calexp")
        psf = calexp.getPsf()
        sources = dataRef.get("src")
        print len(sources), 'sources before deblending'

        self.deblend.run(calexp, sources, psf)
        print len(sources), 'sources after deblending'

        sources.setWriteHeavyFootprints(True)
        sources.writeFits('deblended.fits')

        calexp.writeFits('calexp.fits')
        
if __name__ == "__main__":
    MyTask.parseAndRun()
