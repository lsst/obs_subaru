import os.path

configDir = os.path.dirname(__file__)

config.isr.load(os.path.join(configDir, "isr.py"))
config.isr.doBrighterFatter = False
