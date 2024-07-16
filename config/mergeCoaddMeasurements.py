import os.path

# Gen3 mergeMeasurements will supersede mergeCoaddMeasurements
# Keep in sync in the meantime
config.load(os.path.join(os.path.dirname(__file__), "mergeMeasurements.py"))
