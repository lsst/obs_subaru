# test out fbo module
source etc/read_dump.tcl
inipar [envscan \$PHOTODATA_DIR/frames_4/input/fp_data_processing_plan]
inipar [envscan [getpars software]]

# read image
set myreg [regReadAsFits [regNew -mask] [envscan \$PHOTODATA_DIR/frames_4/input/R-0-1-g-0.fit]]


# get noise mask
set nreg [regNewFromReg $myreg ]
phSqrt $nreg 0 6. 0.08333333

# set FRSTAT
set fstat [frstatNew]

# Make dgpsf
set psf [dgpsfNew]
handleSet $psf.sigmax1  1.05533
handleSet $psf.sigmax2 3.92557
handleSet $psf.sigmay1 1.06176
handleSet $psf.sigmay2 3.838
handleSet $psf.b 0.161413

# set parameters

# MAKE OBJECT1 LIST

set olist [listNew OBJECT1]

# do the stuff
echo "Going in .... FBO"
findBrightObjects $myreg $nreg 67.99 3.3 $olist $fstat

# set up plotting masks
saoMaskColorSet {red yellow red green blue blue yellow green}

# doing brightObjectsMeasure
echo "Going in .... BOM"
brightObjectsMeasure $myreg $nreg 67.99 3.3 0. 1. $olist $psf $fstat

iterInit $olist

# doing brightObjectsClassify
echo "Going in .... BOC"
brightObjectsClassify $olist $fstat

# doing cleanFrames
echo "Going in .... CF"
cleanFrames $myreg $nreg $olist $fstat
