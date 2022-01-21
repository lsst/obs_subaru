# Don't do photometry for HSC (we use fgcmcal instead).
config.doPhotometry = False

# HSC needs a higher order polynomial to track the steepness of the optical
# distortions along the edge of the field. Emperically, this provides a
# measurably better fit than the default order=5.
config.astrometryVisitOrder = 7

# For the HSC deep fields with hundreds of visits, outlier rejection takes ~two
# weeks of runtime. By stopping outlier rejection when it ceases to have a
# significant effect on the model, we can bring compute time down to a few days.
config.astrometryOutlierRelativeTolerance = 0.002
