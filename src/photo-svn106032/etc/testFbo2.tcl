# test out fbo module
source etc/read_dump.tcl
inipar [envscan \$PHOTODATA_DIR/fbo/frames/input/data_processing_plan]
inipar [envscan [getpars software]]

# read image
set myreg [regReadAsFits [regNew -mask] [envscan \$PHOTODATA_DIR/fbo/frames/input/bright_r4.fit]]

# get noise mask

set nreg [regNewFromReg $myreg -type U8  ]
regSubtract $nreg $nreg

# set FRSTAT
set fstat [frstatNew]

# set parameters

# MAKE OBJECT1 LIST

set olist [listNew OBJECT1]

# Make dgpsf
set psf [dgpsfNew]
handleSet $psf.sigmax1  1.05533
handleSet $psf.sigmax2 3.92557
handleSet $psf.sigmay1 1.06176
handleSet $psf.sigmay2 3.838
handleSet $psf.b 0.161413

# do the stuff
echo "Going in ...."
findBrightObjects $myreg $nreg 109. 4.2 $olist $fstat

# set up plotting masks
saoMaskColorSet {red yellow red green blue blue yellow green}
