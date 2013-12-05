from hsc.pipe.tasks.background import XyTaperWeightImage
bgRef = root.backgroundReference
bgRef.construct.taper.retarget(XyTaperWeightImage)
bgRef.construct.taper.xStart = 3000
bgRef.construct.taper.xStop = 4750
bgRef.construct.taper.yStart = 2500
bgRef.construct.taper.yStop = 3750
