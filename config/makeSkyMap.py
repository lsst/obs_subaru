config.skyMap = "discrete"

# Configuration for HealpixSkyMap
config.skyMap["healpix"].log2NSide = 5
config.skyMap["healpix"].pixelScale = 0.18
config.skyMap["healpix"].patchBorder = 333 # Pixels
config.skyMap["healpix"].tractOverlap = 2.0/60 # Degrees
config.skyMap["healpix"].projection = "TAN"

# Configuration for DiscreteSkyMap
# 0: ACTJ0022M0036
# 1: M31
config.skyMap["discrete"].raList = [5.5, 10.7]
config.skyMap["discrete"].decList = [-0.6, 41.3]
config.skyMap["discrete"].radiusList = [0.5, 0.8]
config.skyMap["discrete"].pixelScale = 0.168
