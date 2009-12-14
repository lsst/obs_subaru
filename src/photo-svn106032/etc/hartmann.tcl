#
# Analyse Hartman Screen data
#
# Typical usage:
#   setParamFile /home/rhl/hartmann
#   init_hartmann
#   process_hartmann -asc spots.dat -fpId foo \
#			    /data/optics/R87F70G1.FIT -rowc 208 -colc 1543
#   fini_hartmann
#
#   (look at file spots.dat)
#
#   set_run -root outDir 1 1;
#   set table [openit my_id_str]; mtv_objc_list table "" mtv_blend
#
###############################################################################
#
# Read a Hartmann image
#
proc get_hartmann_reg {args} {
   set opts [list \
		 [list get_hartmann_reg \
		      "Open a Hartmann image, returning a REGION"] \
		 [list <file> STRING "" file \
		      "Name of Hartmann image file"] \
		 [list -rowc INTEGER -1 rowc \
		      "Row position of star image"] \
		 [list -colc INTEGER -1 colc \
		      "Column position of star image"] \
		 [list -size INTEGER 200 size \
		      "Size of region to analyse, centred at (rowc, colc)"] \
		 [list -bias STRING "" biasReg \
		      "Bias (or dark) region to subtract"] \
		 ]
   if {[shTclParseArg $args $opts open_mytables] == 0} {
      return 
   }

   set reg [regReadAsFits [regNew] $file]
   #
   # Bias subtract
   #
   if {$biasReg != ""} {
      regIntLincom $reg $biasReg 1000 1 -1
   }
   #
   # Trim image if rowc/colc is specified
   #
   if {$rowc >= 0} {
      if {$colc < 0} {
	 error "Please specify both -rowc and -colc"
      }
      set treg [regNew $size $size];	# trimmed region
      set sreg [subRegNew $reg $size $size \
		    [expr int($rowc-$size/2)] [expr int($colc-$size/2)]]
      regIntCopy $treg $sreg
      
      regDel $sreg; regDel $reg
      set reg $treg
   }
   #
   # Add mask
   #
   set mask [spanmaskNew -nrow [exprGet $reg.nrow] -ncol [exprGet $reg.ncol]]
   handleSetFromHandle $reg.mask &$mask
   handleDel $mask
   #
   # Return answer
   #
   return $reg
}

################################################################
#
# given a region, and a sky value, run the object finder to find all
# pixels above sky, and use this to generate an OBJC, which is then returned
#
proc find_objc {reg sky psf_filt_size psfsigma} {
   global MASK_TYPE fieldparams OBJECT1 filterlist verbose
   
   set psfsigma 1;			# this works better than the argument!
   if 0 {
      set psfsigma -1;			# don't smooth
   }

   set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<0>]

   set filter [lindex $filterlist 0]
   set hpsf_filt_size [expr int($psf_filt_size/2) + 1]
   
   set nrow [exprGet $reg.nrow]
   set ncol [exprGet $reg.ncol]

   if {$psfsigma > 0} {			# smooth image
      set sreg [regNew $nrow $ncol]; set scr [regNew $nrow $ncol]
      convolveWithGaussian $sreg $reg $scr $psf_filt_size $psfsigma
      regDel $scr
   
      handleSetFromHandle $sreg.mask $reg.mask; # set bits in $reg.mask
   } else {
      set sreg $reg
   }

   set lev [vectorExprEval [softBiasGet]+20]
   set objects [regFinder $reg $lev $fparams \
		    -row0  $hpsf_filt_size -col0  $hpsf_filt_size \
		    -row1 -$hpsf_filt_size -col1 -$hpsf_filt_size];

   #
   # grow the found objects, and cull the resulting peak lists
   #
   objectChainGrow $objects 0 $reg 6

   if {$psfsigma <= 0} {		# no smoothing was applied
      set neff 1
   } else {
      set neff [expr 4*3.14*pow($psfsigma,2)];# neff of smoothing filter
   }
   set nsigma 3;			# XXX
   objectChainPeaksCull $reg $objects -$nsigma -fp $fparams -neff $neff

   if {$verbose > 1} {
      echo "Found [chainSize $objects] objects"
   }
      
   if {$sreg != $reg} {
      handleSet (long)$sreg.mask 0;
      regDel $sreg
   }
   vectorExprDel $lev
   #
   # convert OBJECT chain to OBJECT1 chain
   #
   set objs($filter) [objectToObject1ChainConvert $objects 0 $reg $sky]
   spanmaskSetFromObject1Chain $objs($filter) *$reg.mask $MASK_TYPE(OBJECT)
   object1ChainFlagsSet $objs($filter) $OBJECT1(BINNED1)
   chainDel $objects
   #
   # Merge the bright object list
   #
   if [info exists objects] { unset objects }   
   merge_colors objs $fieldparams objects
   chainDel $objs($filter); unset objs
   #
   # get our OBJC from the OBJC chain; choose the brightest if there's
   # more than one
   #
   set our_objc [objcNew [llength $filterlist]]
   
   set curs [chainCursorNew $objects]
   while {[chainWalk $objects $curs] != ""} {
      set objc [chainElementRemByPos $objects HEAD]
      if {[exprGet (int)$our_objc.peaks] == 0 || \
	      ([exprGet $objc.peaks->peaks<0>->peak] > \
		   [exprGet $our_objc.peaks->peaks<0>->peak])} {
	 objcDel $our_objc; set our_objc $objc
      } else {
	 objcDel $objc
      }
   }
   chainCursorDel $objects $curs
   chainDel $objects
   
   set objc $our_objc; unset our_objc

   handleDel $fparams

   return $objc
}

###############################################################################
#
# Show all the peaks in an object
#
proc show_child_peaks {objc} {
   handleDel [objcDescendentNext $objc];# parent
   while {[set child [objcDescendentNext NULL]] != ""} {
      draw_cross + [exprGet $child.rowc] [exprGet $child.colc]
   }
}
###############################################################################
#
# Print all child peaks
#
proc print_child_peaks {args} {
   global spot_rowc spot_colc verbose

   set opts [list \
		 [list print_child_peaks \
		      "Print the peaks detected in a Hartmann mask image"] \
		 [list <hartmannFile> STRING "" inputFile \
		      "Name of the image being reduced"] \
		 [list <fd> STRING "" fd \
		      "Open TCL file descriptor to write to"] \
		 [list <objc> STRING "" objc \
		      "The OBJC to print"] \
		 [list -petroRad DOUBLE 10 petroRad \
		      "Maximum value of Petrosian radius to accept"] \
		 [list -nmin INTEGER 2 nmin \
		      "Minimum number of points to define a ring of spots"] \
		 [list -nmin INTEGER 3 nring \
		      "Number of rings of holes in Hartmann mask"] \
		 [list -nmin INTEGER 16 nhole \
		      "Number of holes per ring"] \
		 [list -pixel_scale DOUBLE 0.0 pixel_scale \
		   "Pixel size (in microns); if provided positions are in mm"] \
		 ]
   if {[shTclParseArg $args $opts print_child_peaks] == 0} {
      return 
   }

   if [info exists spot_rowc] { unset spot_rowc }
   if [info exists spot_colc] { unset spot_colc }

   set i 0
   handleDel [objcDescendentNext $objc];# parent
   while {[set child [objcDescendentNext NULL]] != ""} {
      #echo [format %.1f [exprGet $child.color<0>->petroRad]]

      if {[exprGet $child.color<0>->petroRad] > $petroRad} {
	 handleDel $child
	 continue
      }
      
      set spot_rowc($i) [exprGet $child.rowc]
      set spot_colc($i) [exprGet $child.colc]

      lappend list_rowc $spot_rowc($i);	# for sorting
      lappend list_colc $spot_colc($i)

      set rowc_sum $spot_rowc($i)
      set colc_sum $spot_colc($i)

      handleDel $child
      incr i
   }
   set n $i

   if {$n == 0} {
      error [format \
		 "Couldn't find any spots: Object is at (r,c) = (%.1f, %.1f)" \
		 [exprGet $objc.rowc] [exprGet $objc.colc]]
   }
   #
   # Find and subtract centre of pattern.
   #
   # The first estimate is the mean of the 2nd and 2nd-last row/column values
   #
   foreach v "rowc colc" {
      set sl [lsort -real [set list_$v]]
      set pattern_$v [expr 0.5*([lindex $sl 1] + \
				    [lindex $sl [expr [llength $sl]-2]])]
   }

   loop i 0 $n {
      set spot_rowc($i) [expr $spot_rowc($i) - $pattern_rowc]
      set spot_colc($i) [expr $spot_colc($i) - $pattern_colc]
   }
   #
   # Now make radial histograms to try and identify the rings of sopts
   #
   loop i 0 $n {
      set r [format %.3f [expr sqrt(pow($spot_rowc($i),2) + pow($spot_colc($i),2))]]
      set ir [expr int($r + 0.5)]

      lappend radii($ir) $r
      lappend iradii($ir) $i
   }
   
   set irs [lsort -integer [array names radii]]
   loop ir [lindex $irs 0] [expr [lindex $irs end]+1] {
      if [info exists radii($ir)] {
	 set radii($ir) "0 $iradii($ir)"
      } else {
	 set radii($ir) 0
      }
   }

   if 0 {				# print histogram
      foreach ir [lsort -integer [array names radii]] {
	 echo [format "%3d %d %s" $ir [expr [llength $radii($ir)]-1] \
		   [lrange $radii($ir) 1 end]]
      }
   }
   #
   # Classify spots into ring1, ring2, and ring3
   #
   set n 1
   foreach ir [lsort -integer [array names radii]] {
      if {[llength $radii($ir)] == 1} {	# i.e. only "0"
	 if {[info exists radii([expr $ir-1])] &&
	     [llength $radii([expr $ir-1])] > 1} {
	    incr n
	 }
      } else {
	 append ring$n " [join [lrange $radii($ir) 1 end]]"
      }
   }
   #
   # Reject rings with less than nmin members
   #
   foreach ring [info vars {ring[0-9]*}] {
      if {[llength [set $ring]] < $nmin} {
	 unset $ring
      }
   }
   #
   # Renumber if needs be
   #
   set renumber 0
   loop i 1 [expr $nring + 1] {
      if ![info exists ring$i] {
	 incr renumber
	 break
      }
   }
   if $renumber {
      foreach ring [info vars {ring[0-9]*}] {
	 set s$ring [set $ring]
	 unset $ring
      }
      
      set i 0
      foreach sring [info vars {sring[0-9]*}] {
	 set ring[incr i] [set $sring]
	 unset $sring
      }
   }

   if $verbose {
      foreach ring [lsort -command sort_rings [info vars {ring[0-9]*}]] {
	 echo [format "%s %-2d %s" $ring [llength [set $ring]] [set $ring]]
      }
   }
   #
   # Check that we are making sense
   #
   set nring_seen [llength [info vars {ring[0-9]}]]
   if {$nring != $nring_seen} {
      error "More or fewer than $nring rings are detected: $nring_seen"
   }
   #
   # Improve centre of object
   #
   set ir 0
   if {[llength [set ring$nring]] == $nhole} {
      set ir $nring
   } else if {[llength $ring1] == $nhole} {
      set ir 1
   }

   if {$ir == 0} {
      echo "Neither first or last ring is complete; not recentroiding"
   } else {
      set pattern_rowc 0; set pattern_colc 0; set n 0
      foreach i [set ring$ir] {
	 incr n
	 set pattern_rowc [expr $pattern_rowc + $spot_rowc($i)]
	 set pattern_colc [expr $pattern_colc + $spot_colc($i)]
      }

      set pattern_rowc [expr $pattern_rowc/$n]
      set pattern_colc [expr $pattern_colc/$n]

      loop ir 1 [expr $nring + 1] {
	 foreach i [set ring$ir] {
	    set spot_rowc($i) [expr $spot_rowc($i) - $pattern_rowc]
	    set spot_colc($i) [expr $spot_colc($i) - $pattern_colc]
	 }
      }
   }
   #
   # For each ring of spots, sort in order of increasing angle
   #
   loop ir 1 [expr $nring + 1] {
      set ring$ir [lsort -command sort_in_angle [set ring$ir]]
   }
   #
   # Actually write data, ring by ring
   #
   puts $fd "#Input file: $inputFile"

   if {$pixel_scale == 0} {
      set convert_to_mm 0
      puts $fd "#Units: Pixels"
   } else {
      set scale [expr 1e-3*$pixel_scale]
      set convert_to_mm 1
      puts $fd "#Units: mm (scale: $pixel_scale microns/pixel)"
   }

   puts $fd [format "%-5s %-10s %-10s" "#\n#Ring" "row" "column"]

   loop ir 1 [expr $nring + 1] {
      set j [expr $nhole*($ir-1) + 1]
      foreach i [set ring$ir] {
	 set rowc $spot_rowc($i)
	 set colc $spot_colc($i)

	 if $convert_to_mm {
	    set rowc [expr $rowc*$scale]
	    set colc [expr $colc*$scale]
	 }
	 puts $fd [format "%-5d %-10.3f %-10.3f" $ir $rowc $colc]
	 incr j
      }
   }
}

proc sort_rings {r1 r2} {
   regexp {ring([0-9]+)} $r1 foo n1
   regexp {ring([0-9]+)} $r2 foo n2

   return [expr $n1 - $n2]
}

proc sort_in_angle {i1 i2} {
   global spot_rowc spot_colc

   set a1 [expr atan2($spot_rowc($i1), $spot_colc($i1))]
   set a2 [expr atan2($spot_rowc($i2), $spot_colc($i2))]

   if {$a1 < $a2} {
      return -1
   } else {
      return 1
   }
}

###############################################################################
#
# Set the param file
#
proc setParamFile {dir {file fpParam.par}} {
   global parametersDir parameters

   set parametersDir $dir
   set parameters $file
}

################################################################################
#
# the main routine -- create a new region, create all auxiliary
#   data, then call measureObj on the region
#
if ![info exists verbose] {
   set verbose 1
}

proc init_hartmann {} {
   global env verbose init_hartmann
   global calibs corrows corcols fieldparams fieldstat filterlist nfilter rand 
   global defpars softpars parametersDir parameters

   if [info exists init_hartmann] {
      error "You have already called init_hartmann"
   }
   set init_hartmann 1

   param2Chain [envscan \$PHOTO_DIR]/etc/fpParam.par defpars

   if {[info exists parametersDir] && [info exists parameters]} {
      param2Chain $parametersDir/$parameters softpars
      calc_crc $parametersDir/$parameters
      if {[llength $softpars] > 0} {
	 if {$verbose > 0} { set term ":" } else { set term "" }
	 echo "Warning: some parameters are changed from default values$term"
	 if {$verbose > 0} {
	    echo "    [keylget softpars]"
	 }
      }
   } else {
      keylset softpars nchild_max 100;	# max no. of children to run deblender
      keylset softpars npeak_per_obj_max -1;# max. no. of peaks per object;
      					#-1 => unlimited
      keylset softpars mo_fit_models 0;	# run model fitting code?
   }
   #
   # choose the _last_ occurrence of each keyword, in accordance with
   # behaviour of the plan files
   #
   set corrows 1500; set corcols 2048
   set flux20 1000
   
   set filterlist [list r]
   set nfilter [llength $filterlist]
   
   keylset softpars gain 1
   keylset softpars dark_variance 0
   
   foreach file "defpars softpars" {
      foreach el [set $file] {
	 set key [lindex $el 0]
	 set val [join [lrange $el 1 end]]
	 keylset $file $key $val
      }
   }
   
   
   #####
   # create CALIB1BYFRAME structure
   
   set calibs [calib1byframeNew $filterlist]
   loop i 0 $nfilter {
      set cal1 [calib1New [lindex $filterlist $i]]
      handleSet $cal1.flux20 $flux20
      handleSet $cal1.node 0.0
      handleSet $cal1.incl 0.0
      handleSet $cal1.gain [getsoftpar gain]
      handleSet $cal1.dark_variance [getsoftpar dark_variance]
      
      if 0 {
	 set psf [dgpsfNew -sigmax1 0.94 -sigmax2 2.70 -sigmay1 0.84 \
		      -sigmay2 2.90 -b 0.11]
      } else {
	 set psf [dgpsfNew -sigmax1 1 -sigmax2 2 -b 0.1]
      }
      handleSet $psf.sigma1_2G \
	  [expr 0.5*([exprGet $psf.sigmax1]+[exprGet $psf.sigmay1])]
      handleSet $psf.sigma2_2G \
	  [expr 0.5*([exprGet $psf.sigmax2]+[exprGet $psf.sigmay2])]
      handleSet $psf.b_2G [exprGet $psf.b]
      handleSet $psf.width [expr 1.2*[exprGet $psf.sigma1_2G]]
      handleSetFromHandle $cal1.psf &$psf; handleDel $psf
      
      handleSetFromHandle $calibs.calib<$i> &$cal1
      handleDel $cal1
   }
   
   #####
   # create needed structures and initialise
   #
   set fieldstat [fieldstatNew]
   
   set fieldparams [fieldparamsNew $filterlist]
   
   set rand [phRandomNew 200000 -seed [getsoftpar fp_random_seed] -type 2]
   
   initProfileExtract

   global ffo_median_size ffo_psf_filt_size ffo_levels
   init_find_objects $fieldparams $corrows $corcols \
       ffo_levels ffo_sigma_max ffo_median_size ffo_psf_filt_size \
       ffo_binshifts ffo_binned_thresholds
   init_merge_colors $fieldparams
   init_measure_objects $fieldparams ref_color $rand
   
   #
   # Get scratch space for the deblender
   #
   if {[overlayN] == 0} {
      loop n 0 1 {
	 overlayAdd [regNew -name scr$n $corrows $corcols]
      }
   }
   #
   # call the Init Measure Obj module
   #
   set catalog "$env(PHOTO_DIR)/lib/rgalprof.dat"
   set cellcat "$env(PHOTO_DIR)/lib/cellprof.dat"
   
   set failed 0
   foreach f "catalog cellcat" {
      set file [set $f]
      if {![file readable $file]} {
	 echo "TEST-ERR: you must make $file before running tests"
	 incr failed
      }
   }
   if $failed {
      error "required input files are missing (run \"make datafiles\")"
   }
   
   initFitobj $catalog
   initCellFitobj $cellcat 1
   
   declare_schematrans $nfilter
}


proc fini_hartmann {} {
   global init_hartmann fieldparams fieldstat calibs

   if ![info exists init_hartmann] {
      return
   }

   finiProfileExtract
   finiMeasureObj
   finiCellFitobj
   
   calib1byframeDel $calibs
   fieldstatDel $fieldstat
   fieldparamsDel $fieldparams; unset fieldparams

   unset init_hartmann
}

proc process_hartmann {args} {
   global ffo_psf_filt_size corrows corcols
   global calibs fieldparams fieldstat filterlist rand
   global display init_hartmann verbose

   set opts [list \
		 [list process_hartmann \
		      "Process a Hartmann image file"] \
		 [list <file> STRING "" file \
		      "The Hartmann image to process"] \
		 [list -fpId STRING "" fpId_str \
	      "ID string for fpObjc tables; if omitted tables aren't written"] \
		 [list -asciiId STRING "" asciiId_str \
	      "ID string for ascii output; if omitted summary isn't written"] \
		 [list -outDir STRING "" outputDir \
		      "Directory to write fpObjc tables (if desired)"] \
		 [list -rowc INTEGER -1 rowc \
		      "Row position of star image"] \
		 [list -colc INTEGER -1 colc \
		      "Column position of star image"] \
		 [list -size INTEGER 200 size \
		      "Size of region to analyse, centred at (rowc, colc)"] \
		 [list -bias STRING "" biasReg \
		      "Bias (or dark) region to subtract"] \
		 [list -pixel_scale DOUBLE 0.0 pixel_scale \
	     "Pixel size (in microns); if positions in ascii table are in mm"] \
		 ]
   if {[shTclParseArg $args $opts open_mytables] == 0} {
      return 
   }

   if ![info exists init_hartmann] {
      error "You must call init_hartmann before processing an image"
   }

   set soft_bias [softBiasGet]

   set index 0;				# which filter we're processing
   set filter [lindex $filterlist $index]

   #
   # Read data
   #
   set reg [get_hartmann_reg $file -rowc $rowc -colc $colc -size $size \
		-bias $biasReg]
   if 0 {
      display $reg "image" default 1
   }
   #
   # Deal with CRs
   #
   regStatsFromQuartiles $reg -coarse 2 -cmedian cmedian -csigma csigma
   set skyTypical [expr $cmedian - $soft_bias]
   set skySigma $csigma

   foreach var "gain cr_min_sigma cr_min_e cr_cond3 cr_cond32 cr_keep" {
      set $var [getsoftpar $var]
   }

   set ncr [eval findCR $reg $skyTypical -bkgd $skyTypical \
		$skySigma *$calibs.calib<$index>->psf \
		$gain -min_sigma $cr_min_sigma -min_e $cr_min_e \
		-cond3 $cr_cond3 -cond32 $cr_cond32 \
		[lindex {-keep {}} [expr $cr_keep?0:1]]]
   if {$verbose > 1} {
      echo Found $ncr CRs
   }
   #
   # Find sky level and subtract it
   #
   global ffo_median_size; set median_size $ffo_median_size
   set nbit 10

   set filt_c $median_size;
   set filt_r $median_size;
   set skysigreg [binregionNew]
   set skyreg [medianSky $reg $filt_c $filt_r -shift $nbit \
		   -skysig $skysigreg];
   regIntConstAdd *$skyreg.reg [expr -$soft_bias*(1<<$nbit)];
   skySubtract $reg $reg $skyreg $rand

   if 0 {
      display $reg "sky subtracted" default 1
   }
   #
   # update fieldparams
   #
   handleSetFromHandle $fieldparams.frame<$index>.sky &$skyreg
   handleSetFromHandle $fieldparams.frame<$index>.skyErr &$skysigreg
   handleSet $fieldparams.frame<$index>.bkgd 0;# it's been subtracted
   #
   # Now we have a region, set colors for measure objects; note that one
   # of the arguments is the data region
   #
   set prof [compositeProfileNew]; handleSet $prof.psfCounts 1e4
   handleSetFromHandle $calibs.calib<$index>->prof &$prof
   handleDel $prof
   
   set trans [transNew]
   handleSetFromHandle $calibs.calib<$index>->toGCC &$trans
   handleDel $trans
   
   foreach v "reg skyreg skysigreg" {
      set ${v}_arr($filter) [set $v]
   }
   set psfBasis($filter) ""
   set hdr [hdrNew]
   hdrInsWithInt $hdr "RUN" -1
   hdrInsWithInt $hdr "CAMCOL" 10 "Engineering Camera"
   hdrInsWithInt $hdr "FIELD" -1
   hdrInsWithAscii $hdr "VERSION" [photoVersion]
   hdrInsWithInt $hdr "REF_BAND" 0
   hdrInsWithAscii $hdr "FILTERS" $filterlist

   hdrInsWithAscii $hdr "FILE" $file
   hdrInsWithDbl $hdr "ROWC" $rowc
   hdrInsWithDbl $hdr "COLC" $colc

   init_measure_objects_field obj_id reg_arr skyreg_arr skysigreg_arr \
       psfBasis $calibs $fieldparams $fieldstat \
       $outputDir $fpId_str $hdr \
       file_objcIo fd_objcIo file_aimage fd_aimage fd_test_info
   hdrDel $hdr
   handleDel $skyreg; handleDel $skysigreg
   #
   # Create an OBJC from the data
   #   
   set objc [find_objc $reg *$fieldparams.frame<$index>.sky $ffo_psf_filt_size \
		 [exprGet $fieldparams.frame<$index>.psf->width]]

   display $reg "after object finding" default 1
   #
   # Setup the deblender's scratch space
   #
   overlayGet reg_scr $corrows $corcols
   set regs [overlaySplit reg_scr 2 2]
   set scr0 [lindex $regs 0]
   set scr1 [lindex $regs 1]
   set scr2 [lindex $regs 2]
   set scr3 [lindex $regs 3]
   unset regs

   deblendSet $scr0 $scr1 $scr2 $scr3
   #
   # OK! Measure
   #
   measureObjc $objc $fieldparams $fieldstat

   if {[info exists display] && $display} {
      if 1 {
	 show_child_peaks $objc
      } else {
	 set blend [mtv_objc_blend $objc]
	 display $blend "deblended object" default 1
	 regDel $blend
      }
   }

   set return_obj 1
   if {$fpId_str != ""} {
      set return_obj 0
      write_objc $objc obj_id $fd_objcIo $fd_aimage $fd_test_info
   }
   if {$asciiId_str != ""} {
      set return_obj 0
      
      set ascii_file $asciiId_str      
      if ![regexp {\.} $ascii_file] {
	 append ascii_file ".dat"
      }

      if {$outputDir != ""} {
	 set ascii_file $outputDir/$ascii_file
      }
      
      if [catch {
	 set fd [open $ascii_file w]
	 print_child_peaks $file -pixel_scale $pixel_scale $fd $objc
	 close $fd
      } msg] {
	 echo "Error writing $ascii_file: $msg"
      }
   }
#
# cleanup
#
   measureObjUnset $fieldparams

   if !$return_obj {
      objcDel $objc
   }

   spanmaskDel *$reg.mask
   handleSet (long)$reg.mask 0
   regDel $reg

   fini_measure_objects_field $filterlist $fieldparams \
       $file_objcIo $fd_objcIo $file_aimage $fd_aimage $fd_test_info

   deblendUnset; overlayRelease reg_scr
   profileExtract NULL 0 0 0 0 0 -id -1;# invalidate the cached ID

   if $return_obj {
      return $objc
   } else {
      return ""
   }
}
