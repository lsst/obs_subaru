import os.path

configDir = os.path.dirname(__file__)
bgFile = os.path.join(configDir, "background.py")

config.detection.background.load(bgFile)
config.subtractBackground.load(bgFile)

config.isr.load(os.path.join(configDir, "isr.py"))
config.isr.doBrighterFatter = False
