import os.path

# Load configs from base assembleCoadd
config.load(os.path.join(os.path.dirname(__file__), "assembleCoadd.py"))

# Load configs for the subtasks
config.assembleMeanCoadd.load(os.path.join(os.path.dirname(__file__), "assembleCoadd.py"))
config.assembleMeanCoadd.statistic = 'MEAN'
config.assembleMeanClipCoadd.load(os.path.join(os.path.dirname(__file__), "assembleCoadd.py"))
config.assembleMeanClipCoadd.statistic = 'MEANCLIP'
