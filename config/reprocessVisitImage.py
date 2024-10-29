#!/usr/bin/env python
import os.path

ObsConfigDir = os.path.dirname(__file__)

config.compute_summary_stats.load(os.path.join(ObsConfigDir, "computeExposureSummaryStats.py"))
