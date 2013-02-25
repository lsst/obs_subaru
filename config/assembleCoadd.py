from lsst.obs.subaru.selectImages import ButlerSelectImagesTask
root.select.retarget(ButlerSelectImagesTask)

root.maxMatchResidualRatio = 10.0**9
root.matchBackgrounds.binSize = 1024
root.matchBackgrounds.gridStatistic = "MEDIAN"
