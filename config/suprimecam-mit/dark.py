import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam-mit', 'isr.py'))
root.darkTime = "EXP1TIME"
root.isr.doGuider = False

root.combination.clip = 2.5

