# Config override for lsst.ap.verify.Gen3DatasetIngestTask

import os

configDir = os.path.dirname(__file__)

config.visitDefiner.load(os.path.join(configDir, 'defineVisits.py'))
