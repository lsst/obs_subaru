from hsc.pipe.tasks.onsiteDb import SuprimecamOnsiteDbTask
config.onsiteDb.retarget(SuprimecamOnsiteDbTask)

# Load regular processCcd configuration
import os
config.load(os.path.join(os.path.abspath(__file__), 'processCcd.py'))
