# List of filters to put into the look-up table (LUT)
config.physicalFilters = ['HSC-G', 'HSC-R', 'HSC-R2', 'HSC-I', 'HSC-I2',
                          'HSC-Z', 'HSC-Y', 'NB0387', 'NB0816', 'NB0921',
                          'NB1010']

# Override mapping from physical filter labels to 'standard' physical
# filter labels. The 'standard' physical filter defines the transmission
# curve that the FGCM standard bandpass will be based on.
# Any filter not listed here will be mapped to
# itself (e.g. HSC-G->HSC-G).  These overrides specify that HSC-R should
# be mapped onto the HSC-R2 system and HSC-I should be mapped onto
# the HSC-I2 system.
config.stdPhysicalFilterOverrideMap = {'HSC-R': 'HSC-R2',
                                       'HSC-I': 'HSC-I2'}

# FGCM name or filename of precomputed atmospheres
config.atmosphereTableName = 'fgcm_atm_subaru3'

