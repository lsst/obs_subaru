
from lsst.obs.subaru.ingest import SuprimecamParseTask
config.parse.retarget(SuprimecamParseTask)

config.parse.defaults = {'ccdTemp': '0',
                         'config': 'configuration',
                         'autoguider': '0'
                         }

config.parse.translation = {'proposal': 'PROP-ID',
                            'dataType': 'DATA-TYP',
                            'expTime': 'EXPTIME',
                            'ccd': 'DET-ID',
                            'pa': 'INR-STR',
                            'autoguider': 'T_AG',
                            'ccdTemp': 'T_CCDTV',
                            'config': 'T_CFGFIL',
                            'frameId': 'FRAMEID',
                            'expId': 'EXP-ID',
                            'dateObs': 'DATE-OBS',
                            'taiObs': 'DATE-OBS',
                            }
