from lsst.obs.subaru.ingest import HscParseTask
config.parse.retarget(HscParseTask)

# Additional columns, not available for Suprime-Cam
config.register.columns['autoguider'] = 'int'
config.parse.translation['autoguider'] = 'T_AG'
config.register.columns['config'] = 'text'
config.parse.translation['config'] = 'T_CFGFIL'
