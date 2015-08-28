import os
config.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam', 'isr.py'))
config.isr.doGuider = False

config.combination.clip = 2.5
