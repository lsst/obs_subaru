import os.path


config.load(os.path.join(os.path.dirname(__file__), "isr.py"))
config.isr.doGuider = False
