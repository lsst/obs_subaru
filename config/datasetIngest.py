# Config override for lsst.ap.verify.DatasetIngestTask
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.ingest import HscIngestTask

configDir = os.path.dirname(__file__)
defectDir = os.path.join(getPackageDir('obs_subaru_data'), 'hsc', 'defects')

config.textDefectPath = defectDir
config.dataIngester.retarget(HscIngestTask)
config.dataIngester.load(os.path.join(configDir, 'ingest.py'))
config.calibIngester.load(os.path.join(configDir, 'ingestCalibs.py'))
config.defectIngester.load(os.path.join(configDir, 'ingestCuratedCalibs.py'))
