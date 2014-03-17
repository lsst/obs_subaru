import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam-mit', 'isr.py'))
root.isr.doGuider = False
