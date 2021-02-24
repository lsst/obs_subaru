import os.path
# Gen3 mergeDetections will supersede mergeCoaddDetections
# Keep in sync in the meantime
config.load(os.path.join(os.path.dirname(__file__), "mergeDetections.py"))
