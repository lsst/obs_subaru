config.badMaskPlanes = ("BAD", "EDGE", "SAT", "INTRP", "NO_DATA",)
config.subregionSize = (10000, 200)  # 200 rows (since patch width is typically < 10k pixels
config.doMaskBrightObjects = True
config.removeMaskPlanes.append("CROSSTALK")
config.doNImage = True

# 200 rows (since patch width is typically < 10k pixels
config.assembleStaticSkyModel.subregionSize = (10000, 200)
