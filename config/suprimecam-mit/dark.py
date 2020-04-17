import os.path


config.load(os.path.join(os.path.dirname(__file__), "isr.py"))
config.darkTime = "EXP1TIME"
config.isr.doGuider = False

config.combination.clip = 2.5
