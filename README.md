# obs_superBIT

superBIT-specific configuration and tasks (based on obs_subaru as the backbone) for the LSST Data Management Stack

Version of LSST Science Pipelines: 15.0

how to use this package?
1. In lsst_stack/
	
	source loadLSST.bash

2. Clone a package

	git clone https://github.com/sutieng/obs_superBIT.git
  
	cd obs_superBIT

3. Set up the package

 	setup lsst_obs
  
 	setup -j -r obs_superbit

4. Create a DATA directory

 	mkdir TESTDATA
  
 	echo "lsst.obs.superBIT.SuperBITMapper" >  TESTDATA/_mapper

5. Ingest the raw images into the data repository

 	python ./bin/superbitIngestImages.py TESTDATA/ RAWDATAFILE/raw/*.fits (image in obs_superBIT/superbit_repo/TESTIMAGE1/2017-05-25/WHEELPOS0/) yo, this is not general
