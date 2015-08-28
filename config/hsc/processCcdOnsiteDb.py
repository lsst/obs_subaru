from hsc.pipe.tasks.onsiteDb import HscOnsiteDbTask
config.onsiteDb.retarget(HscOnsiteDbTask)

# Load regular processCcd configuration
import os
config.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hsc', 'processCcd.py'))
