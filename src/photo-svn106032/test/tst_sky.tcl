#
# Test the sky-measuring routines
#
source [envscan \$PHOTO_DIR]/etc/photoStartup.tcl
set pi 3.1415926536
set nrow 200; set ncol 200
set npix [expr $nrow*$ncol]

# We do NOT want to use a random seed! These tests should be deterministic.
# Note that they are not quite; the gaussian random number generator keeps a
# single value cached so it is possible for these tests to have two outcomes
# if run more than once
foreach rand [handleListFromType RANDOM] {
   phRandomDel $rand
}
if 1 {
   set seed1 865672948
   set seed2 1524949426
   set rand [phRandomNew 200000 -seed $seed1 -type 2]
} else {
   set seed1 [expr [getclock]^([pid]<<10)]
   set rand [phRandomNew 200000 -seed $seed1 -type 2]
   set seed2 [expr [getclock]^[phRandom]]; echo Seeds are $seed1 $seed2
}
phRandomSeedSet $rand $seed2

#
# A simple blank region
#
set descrip(0) "constant level"
proc make_region_0 {sky sig _vals} {
   upvar $_vals vals
   global nrow ncol

   set reg [regNew $nrow $ncol]

   regIntSetVal $reg $sky;

   set vals [list \
		 ]
   lappend vals [list mean $sky 1]
   lappend vals [list median $sky 1]
   lappend vals [list medsky $sky 1]
   lappend vals [list sig 0 1]

   return $reg;
}
#
# A region with Gaussian noise
#
set descrip(1) "Gaussian noise"
proc make_region_1 {sky sig _vals} {
   upvar $_vals vals
   global nrow ncol npix pi rand

   set sky [expr $sky+0.3]

   set reg [regNew $nrow $ncol]

   regClear $reg
   regIntGaussianAdd $reg $rand $sky $sig

   set sig_n [expr $sig/sqrt($npix)]

   set vals [list \
		 ]
   lappend vals [list mean $sky $sig_n]
   lappend vals [list median $sky sqrt($pi/2)*$sig_n]
   lappend vals [list medsky $sky sqrt($pi/2)*$sig_n]
   lappend vals [list sig $sig sqrt($pi/2)*$sig_n/sqrt(2)]
   lappend vals [list mean_median 0 $sig_n]
   lappend vals [list mean_cmean 0 $sig_n]
   lappend vals [list median_cmedian 0 sqrt($pi/2)*$sig_n]
   lappend vals [list sig_csig 0 sqrt($pi/2)*$sig_n/sqrt(2)]

   return $reg;
}
#
# A region with Gaussian noise, then smoothed
#
set descrip(2) "smoothed Gaussian noise"
proc make_region_2 {sky sig _vals} {
   upvar $_vals vals
   global pi rand nrow ncol npix

   set hsize 9
   set size [expr 2*$hsize+1]
   set reg [regNew [expr $nrow+$size+2] [expr $ncol+$size+2]]
   set scr [regNew [expr $nrow+$size+2] [expr $ncol+$size+2]]

   regClear $reg
   regIntGaussianAdd $reg $rand $sky $sig

   set alpha 1
   convolveWithGaussian $reg $reg $scr $size $alpha
   regDel $scr
   if 1 {
      # estimate n_eff for the filter that we actually use
      set sum 0
      loop i 0 $size {
	 set sum [expr $sum+exp(-pow(($i-$hsize)/$alpha,2)/2)]
      }
      set sum2 0
      loop i 0 $size {
	 set sum2 [expr $sum2+pow(exp(-pow(($i-$hsize)/$alpha,2)/2)/$sum,2)]
      }
      
      set ssig [expr $sig/sqrt(4*$pi*pow($alpha,2))]
   } else {
      # continuum value
      set ssig [expr $sig*$sum2]
   }
   set vals [list \
		 ]
   lappend vals [list mean $sky $sig/sqrt($npix)]
   lappend vals [list median $sky $sig/sqrt($npix)]
   lappend vals [list csig $ssig 2*$sig/(2*sqrt($npix))]
   lappend vals [list mean_median 0 $sig/sqrt($npix)]
   lappend vals [list mean_cmean 0 $sig/sqrt($npix)]
   lappend vals [list median_cmedian 0 $sig/sqrt($npix)]

   set sreg [subRegNew $reg $nrow $ncol [expr $hsize+1] [expr $hsize+1]]
   set reg2 [regNew $nrow $ncol]
   regPixCopy $sreg $reg2
   regDel $sreg; regDel $reg
   set reg $reg2

   return $reg;
}
#
# A region with Gaussian noise, and outliers
#
set descrip(3) "Gaussian noise + outliers"
proc make_region_3 {sky sig _vals} {
   upvar $_vals vals
   global pi rand nrow ncol npix

   set reg [regNew $nrow $ncol]

   regClear $reg
   regIntGaussianAdd $reg $rand $sky $sig

   set sreg [subRegNew $reg $nrow [expr int($ncol/10)] 0 0]
   regIntSetVal $sreg [expr $sky+2000]
   regDel $sreg

   set vals [list \
		 ]
   lappend vals [list medsky $sky $sig/sqrt($npix)]

   return $reg;
}
#
# A region with a ramp + Gaussian noise
#
# This region may be saved to disk as "sky4.fts" for future use (or it may
# be deleted at the end of this file)
#
set descrip(4) "ramp + Gaussian noise"
proc make_region_4 {sky sig _vals} {
   upvar $_vals vals
   global pi rand nrow ncol npix

   set sig 3
   set grad 0.5
   
   set file "sky4.fts"

   set reg [regNew $nrow $ncol]
   if [file exists $file] {
      regReadAsFits $reg $file
   } else {
      echo "Creating ramp for sky tests"
      loop i 0 $nrow {
	 loop j 0 $ncol {
	    regPixSetWithDbl $reg $i $j \
		[expr int($i*$grad+0.5*(1+[phRandom].0/(1<<31)))]
	 }
      }
      if 1 {
	 regIntGaussianAdd $reg $rand $sky $sig
      } else {
	 regIntConstAdd $reg $sky
      }

      regWriteAsFits $reg $file
   }

   set sig [expr 1.6*$sig];		# allow for extra dither etc.
   set mean [expr $sky+$grad*($nrow-1)/2.0]
   set vals [list \
		]
   lappend vals [list mean $mean $sig/sqrt($npix)]
   lappend vals [list median $mean $sig/sqrt($npix)]
   lappend vals [list medsky $mean $sig/sqrt($npix)]
      
   return $reg;
}
#
# A constant region with Gaussian noise, which we then subtract
#
set descrip(5) "Gaussian noise - sky"
proc make_region_5 {sky sig _vals} {
   upvar $_vals vals
   global pi rand nrow ncol npix rand

   set nbit 7; set soft_bias 1000

   set reg [regNew $nrow $ncol]

   regClear $reg
   regIntGaussianAdd $reg $rand $sky $sig

   set bsr [medianSky $reg 32 32 -c 1 -shift $nbit]
   regIntConstAdd *$bsr.reg [expr -$soft_bias*(1<<$nbit)];
   skySubtract $reg $reg $bsr $rand
   binregionDel $bsr

   set vals [list \
		]
   lappend vals [list mean $soft_bias $sig/sqrt($npix)]
   lappend vals [list median $soft_bias $sig/sqrt($npix)]
   lappend vals [list medsky $soft_bias $sig/sqrt($npix)]
   
   return $reg;
}
#
# A region with a ramp, which we then subtract
#
set descrip(6) "ramp + Gaussian noise - sky"
proc make_region_6 {sky sig _vals} {
   upvar $_vals vals
   global pi rand nrow ncol npix rand

   set nbit 7; set soft_bias 1000

   set reg [make_region_4 $sky $sig vals]
   set bsr [medianSky $reg 32 32 -c 1 -shift $nbit]
   if 0 {
      mtv *$bsr.reg
      puts -nonewline "RHL sky"; gets stdin
   }
   regIntConstAdd *$bsr.reg [expr -$soft_bias*(1<<$nbit)];
   skySubtract $reg $reg $bsr $rand
   binregionDel $bsr
   #
   # Images with gradients are not correctly subtracted as the ansatz is that
   # images are flat (otherwise we get confused by things like stars at the
   # edge of the frame).  The problem is that the sky subtraction is done in
   # NxN regions made of 2x2 (N/2)x(N/2) patches.  At the edge we can only
   # get the (N/2)xN patch, and have to extrapolate its value to the edge
   # of the frame.
   #
   # So trim the edge of the frame to avoid this problem for this artificial
   # problem created by using a ramp for this test
   #
   set edge 16
   set sreg [subRegNew $reg [expr $nrow-2*$edge] [expr $ncol-2*$edge] $edge $edge]
   set treg [regIntCopy NULL $sreg]
   regDel $sreg; regDel $reg; set reg $treg
   
   set vals [list \
		]
   lappend vals [list mean $soft_bias $sig/sqrt($npix)]
   lappend vals [list median $soft_bias $sig/sqrt($npix)]
   lappend vals [list medsky $soft_bias $sig/sqrt($npix)]
   
   return $reg;
}

#################################################################
# check to see that we got things right
# return 0 if all okay, or 1 if not.
#
proc check_sky { reg vals } {
   global nsigma

   #
   # Measure things 3 ways:
   # 1/
   #
   regStatsFromQuartiles $reg -coarse 1 -median median -cmedian cmedian \
       -mean mean -sigma sig -csigma csig
       
   set mean_median [expr abs($mean-$median)]
   # 2/
   set stats [regStats $reg -clip 2.36]
   set median_rs [lindex [keylget stats quartiles] 1]
   set mean_rs [keylget stats mean]
   set sig_rs [keylget stats sigma]
   # 3/
   set bsr [medianSky $reg [exprGet $reg.ncol] [exprGet $reg.nrow] -c 1 -sh 7]
   set medsky \
       [expr [exprGet $bsr.reg->rows_s32<1><1>].0/(1<<[exprGet $bsr.shift])]
   binregionDel $bsr
   #
   # compare the methods
   #
   set mean_cmean [expr $mean-$mean_rs]
   set median_cmedian [expr $median-$median_rs]
   set sig_csig [expr $csig-$sig_rs]
   #
   # Now check our results
   #
   set errs_mean 0; set errs_generic 0

   foreach elem $vals {
      set name [lindex $elem 0]
      set val [expr [lindex $elem 1]]
      set err [expr [lindex $elem 2]]
      set nsig [keylget nsigma $name]

      set measured [set $name]
      if { [check_val $name $measured $val $err $nsig] != 0 } {
	 if [regexp {mean|median|medsky} $name] {
	    set errs_mean 1
	 } else {
	    set errs_generic 1
	 }
      }
   }

   return [expr $errs_mean + $errs_generic]
}

#
# this is an array of permissible number of sigma in errors for
# measured properties
#
set nsigma [list \
		{mean_median 2} \
		{mean_cmean 2} \
		{median_cmedian 2} \
		{sig_csig 2} \
		{mean 2} \
		{median 2} \
		{medsky 2} \
		{sig 2} \
		{cmedian 2} \
		{csig 2} \
	   ]

################################################################
#
# check that "$measure" is equal to the given "value",
# to within an amount "diff".  
#   return 0 if the test succeeds, or 1 if it fails
#
proc check_val { what measured value sig nsig } {
   global npix

   set diff [expr abs($measured-$value)] 
   if { $diff > [expr $nsig*$sig] } {
      global errmsg
      lappend errmsg "TEST-ERR: should find $what = $value, but is $measured"
      lappend errmsg "                      |delta| = $diff == [expr $diff/$sig]sigma"
      lappend errmsg "                      (tolerance [expr $sig*$nsig] == $nsig sigma)"
      return 1
   }
   return 0
}

#
# Do the real work
#
set sky 1024;				# sky level
set sig 7;				# standard deviation of sky

set nfail 0;				# allow no failures
set retval 0;				# success until proven a failure
catch { unset errmsg };			# may not exist

if {![info exists tests]} {
   set tests [list 0 1 2 3 4 5 6]
   set nfail 1;				# allow one failures
}
if 0 {
   set tests [list 1];
   
   set nfail 0
   set seed1 [expr [getclock]^([pid]<<10)]
   set rand [phRandomNew 200000 -seed $seed1 -type 2]
   set seed2 [expr [getclock]^[phRandom]]; echo Seeds are $seed1 $seed2
}

foreach test $tests {
   echo "Test $test: $descrip($test)"
#
# create data
#
   eval "set reg \[make_region_$test $sky $sig values\]"
   if 0 {
      mtv $reg; puts -nonewline "RHL "; gets stdin
   }
#
# See if we got it right
#
   if {[set nerr [check_sky $reg $values]] > 0} {
      global errmsg
      lappend errmsg "TEST-ERR: failed on sky frame $test"
      set retval [expr $retval+$nerr]
   }

   if {$retval > $nfail} {
      if {[info exists abort_on_error] && $abort_on_error} {
	 echo [join $errmsg "\n"]
	 error "retval = $retval, reg = $reg"
      }
   }

   regDel $reg
}

#
# Clean up
#
phRandomDel $rand
exec rm -f sky4.fts
#exec sh -c {if [ -f sky4.fts ]; then mv sky4.fts sky4.fts.sav; fi}

if {$retval > $nfail} {
   echo [join $errmsg "\n"]
   error "Failed $retval tests ($nfail failures are permitted)"
}
