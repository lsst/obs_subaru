# Extinction coefficients for HSC filters for conversion from E(B-V) to
# extinction, A_filter.
# Numbers provided by Masayuki Tanaka (NAOJ).
#
# Band, A_filter/E(B-V)

# These default extinction coefficients are not optimal for the FGCM standard
# bandpasses.
# TODO DM-34061: Dynamically compute extinction coeffs from the FGCM lookup
# tables which store the standard bandpasses in a calib-like object.
config.extinctionCoeffs = {
    "g": 3.240,
    "r": 2.276,
    "i": 1.633,
    "z": 1.263,
    "y": 1.075,
    "N387": 4.007,
    "N816": 1.458,
    "N921": 1.187,
}
