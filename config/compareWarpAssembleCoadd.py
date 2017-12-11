config.badMaskPlanes = ("BAD", "EDGE", "SAT", "INTRP", "NO_DATA",)
config.subregionSize = (10000, 200)  # 200 rows (since patch width is typically < 10k pixels
config.doMaskBrightObjects = True
config.doNImage = True

config.assembleStaticSkyModel.badMaskPlanes = ("BAD", "EDGE", "SAT", "INTRP", "NO_DATA",)
# 200 rows (since patch width is typically < 10k pixels
config.assembleStaticSkyModel.subregionSize = (10000, 200)
