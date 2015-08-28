import os
config.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam-mit', 'isr.py'))
config.isr.doGuider = False
