import os

configDir = os.path.dirname(__file__)
bgFile = os.path.join(configDir, "background.py")

config.detection.background.load(bgFile)
config.subtractBackground.load(bgFile)
