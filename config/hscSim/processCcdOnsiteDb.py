from hsc.pipe.tasks.onsiteDb import HscOnsiteDbTask
root.onsiteDb.retarget(HscOnsiteDbTask)

# Load regular processCcd configuration
import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hscSim', 'processCcd.py'))
