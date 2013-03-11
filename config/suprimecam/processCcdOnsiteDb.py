from hsc.pipe.tasks.onsiteDb import SuprimecamOnsiteDbTask
root.onsiteDb.retarget(SuprimecamOnsiteDbTask)

# Load regular processCcd configuration
import os
root.load(os.path.join(os.path.abspath(__file__), 'processCcd.py'))
