import os.path

config_dir = os.path.dirname(__file__)

# Derived from HSC PDR2 reprocessing v24.1.0 (DM-39132)
# These are the median values of quality-selected exposures.

config.fiducialPsfSigma = {
    'u': 2.0,
    'g': 2.04,
    'r': 1.86,
    'i': 1.51,
    'z': 1.75,
    'y': 1.88,
    'N387': 1.84,
    'N816': 1.82,
    'N921': 1.70,
}    
config.fiducialSkyBackground = {
    'u': 1.0,
    'g': 2.84,
    'r': 6.26,
    'i': 13.03,
    'z': 11.44,
    'y': 22.07,
    'N387': 0.07,
    'N816': 0.50,
    'N921': 1.01,
}
config.fiducialZeroPoint = {
    'u': 26.0,
    'g': 27.05,
    'r': 27.10,
    'i': 26.94,
    'z': 26.07,
    'y': 25.63,
    'N387': 22.24,
    'N816': 23.94,
    'N921': 24.09,
}
