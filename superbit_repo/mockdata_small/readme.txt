Folder contains simulations for debugging auxTelBIT

######################
LIST OF FILES
######################

CALIB FILES:
bias_test.fits.fz  : zero-exposure GalSim image, should have same noise properties
bias_test.fits2.fz : ibidem
mock_superBIT_flat.fits : flat field made in Python; displays (1-r^r4) vignetting

DATA FILES:
superBIT_mockdata_power_spectrum_wcs.fits.fz : GalSim output, has WCS & LSS PowerSpectrum shear
superBIT_mockdata_power_spectrum_wcs2.fits.fz : ibid., lower variance
mock_data_powerspectrum_FFconvolved.fits : superBIT_mockdata_power_spectrum_wcs.fits.fz convolved with flat-field
mock_data_powerspectrum_FFconvolved_skybkg : same as above but with 1000 ADU added to simulate constant background
