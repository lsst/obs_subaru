import os
root.colorterms.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hsc", "colorterms.py"))
root.astrometry.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hsc", "filterMap.py"))
