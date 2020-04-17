"""Subaru-specific overrides for DeblendCoaddSourcesTask"""

import os.path

config.singleBandDeblend.load(os.path.join(os.path.dirname(__file__), "singleBandDeblend.py"))
config.multiBandDeblend.load(os.path.join(os.path.dirname(__file__), "multiBandDeblend.py"))
