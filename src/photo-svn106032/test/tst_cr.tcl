#
# This file contains tests for the cosmic ray finding and removal 
# routines found in CR.c.
#
set soft_param_file [envscan \$PHOTO_DIR]/test/tst_params
inipar $soft_param_file

proc test_find_cr { reg sky noise psf e_per_dn {keep ""}} {
   eval findCR $reg $sky -bkgd $sky $noise $psf $e_per_dn $keep \
       -min_sigma [getparf cr_min_sigma] -min_e [getpari cr_min_e] \
       -cond3 [getparf cr_cond3] -cond32 [getparf cr_cond32]
}
#
# Test findCR
#
set failed 0

set sigma 1
set psf [dgpsfNew -sigmax1 1 -b 0 -sigmax2 1e5]
handleSet $psf.sigma1_2G 1; handleSet $psf.sigma2_2G 1e5

set reg [regNew]
regReadAsFits -keeptype $reg [envscan \$PHOTO_DIR]/test/test_cr.fit
regIntConstAdd $reg 1000;		# add soft_bias
set mask [spanmaskNew]; handleSetFromHandle $reg.mask (MASK*)&$mask

set gain 3;				# gain of amps
set sky 103.8;				# sky level
set skysig 4.8;				# sky sigma
set ncr 6;				# number of CRs

if {[set n [test_find_cr $reg $sky $skysig $psf $gain]] != $ncr } {
   echo "TEST-ERR: findCR failed: found $n not $ncr CR pixels"
   incr failed
}

spanmaskDel $mask
set mask [spanmaskNew]; handleSetFromHandle $reg.mask (MASK*)&$mask
if {[set n [test_find_cr $reg $sky $skysig $psf $gain -keep]] != 0 } {
   echo "TEST-ERR: findCR failed to remove all CRs (missed $n)"
   incr failed
}
set pixels [list \
		{46 50  1125} \
		{47 57  1125} \
		]
foreach p $pixels {
   set r [lindex $p 0]; set c [lindex $p 1]
   if {[regPixGet $reg $r $c] > [lindex $p 2]} {
      echo "TEST-ERR: findCR failed to fix pixel ($r, $c)"
      incr failed
   }
}
   
if {0 && $failed} {			# RHL
   mtv $reg
}

dgpsfDel $psf
spanmaskDel $mask
handleSet (long)$reg.mask 0
regDel $reg

#
# Return status
#
if $failed {
   error "Failed $failed tests"
}

