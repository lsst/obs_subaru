import os.path

config_dir = os.path.dirname(__file__)

# Derived from HSC PDR2 reprocessing v24.1.0 (DM-39132)
# These are the median values of quality-selected exposures.

config.fiducialPsfSigma = {
    'u': 2.0,
    'g': 2.07,
    'r': 1.95,
    'i': 1.51,
    'z': 1.75,
    'y': 1.88,
    'N387': 1.89,
    'N816': 1.91,
    'N921': 1.80,
}    
config.fiducialSkyBackground = {
    'u': 1.0,
    'g': 2.86,
    'r': 6.42,
    'i': 13.80,
    'z': 11.78,
    'y': 23.35,
    'N387': 0.07,
    'N816': 0.49,
    'N921': 1.01,
}
config.fiducialZeroPoint = {
    'u': 26.0,
    'g': 27.04,
    'r': 27.09,
    'i': 26.93,
    'z': 26.06,
    'y': 25.63,
    'N387': 22.23,
    'N816': 23.93,
    'N921': 24.10,
}
