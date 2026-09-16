# Load configs from base assembleCoadd
config.load("assembleCoadd.py")

# 200 rows (since patch width is typically < 10k pixels
config.assembleStaticSkyModel.subregionSize = (10000, 200)
config.doFilterMorphological = True
