from lsst.pex.config import Config, ConfigurableField
from lsst.pipe.base import Task
from . import measSeeingQa

class QaConfig(Config):
    seeing = ConfigurableField(target=measSeeingQa.MeasureSeeingConfig, doc="Measure seeing")

class QaTask(Task):
    ConfigClass = QaConfig
    def __init__(self, *args, **kwargs):
        super(QaTask, self).__init__(*args, **kwargs)
        self.makeSubtask("seeing")

    def run(self, dataRef, exposure, sources):
        self.seeing.run(dataRef, sources, exposure)

        metadata = exposure.getMetadata()

        # = flags
        # this part should be done by calculating merit functions somewhere else in a polite manner.
        metadata.set('FLAG_AUTO', 0)
        metadata.set('FLAG_USR', 0)
        metadata.set('FLAG_TAG', 1)

        # = info for data management
        # = rerun; assuming directory tree has the fixed form where 'rerun/' is just followed by '$rerun_name/'
        rerunName = self.getRerunName(dataRef)
        metadata.set('RERUN', rerunName)

    def getRerunName(self, sensorRef):
        # rerun: assuming directory tree has the fixed form where 'rerun/' is just followed by '$rerun_name/'
        corrPath = sensorRef.get('calexp_filename')[0]
        return corrPath[corrPath.find('rerun'):].split('/')[1]
