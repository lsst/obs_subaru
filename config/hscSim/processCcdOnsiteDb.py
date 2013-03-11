from hsc.pipe.tasks.onsiteDb import HscOnsiteDbTask
root.onsiteDb.retarget(HscOnsiteDbTask)

# Load regular processCcd configuration
import os
root.load(os.path.join(os.path.abspath(__file__), 'processCcd.py'))
