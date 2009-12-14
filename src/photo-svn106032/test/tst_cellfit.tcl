#
# Test of cell-based model fitter
#
source [envscan \$PHOTO_DIR]/test/tst_procs.tcl

# We do NOT want to use a random seed! These tests should be deterministic.
# Note that they are not quite; the gaussian random number generator keeps a
# single value cached so it is possible for these tests to have two outcomes
# if run more than once
set seed 843080201
#set seed [getclock]; echo Seed is $seed
set rand [phRandomNew 200000 -seed $seed -type 2]

###############################################################################
#
# Make a model, fit it, and evaluate the results
#
if {![info exists show_failures]} {
   set show_failures 0;			# if true, display failures
}
proc testFitCellModel {type counts size aratio pa fiparams {noise 0}} {
   global gain rand show_failures
   set ret 0

   set test_it 1;			#  test results?

   set fparams [handleBindFromHandle [handleNew] $fiparams.frame<0>]
   set psf [handleBindFromHandle [handleNew] *$fparams.psf]

   set skysig 1
   if { $type == "star" } {
      set reg [regNew 100 100];
      regIntSetVal $reg [softBiasGet]
      set sig1 [exprGet $psf.sigmax1]; set sig2 [exprGet $psf.sigmax2];
      set b [exprGet $psf.b]
      set rowc [expr 50+$pa/180.]; set colc [expr 50+$aratio]
#      set rowc 50.0; set colc 50.0
#      echo PSF is at ($rowc, $colc)
      dgpsfAddToReg $reg $psf $rowc $colc \
	  -a [expr $counts/(2*acos(-1)*(pow($sig1,2)+$b*pow($sig2,2)))]

      set maxpix [regPixGet $reg 50 50]
      set mask [spanmaskNew -nrow 100 -ncol 100]
      handleSetFromHandle $reg.mask (MASK*)&$mask; handleDel $mask
   } elseif { $type == "deV" } {
      set reg [makeDevModel -exact \
		   $counts $size $aratio $pa *$fparams.psf -max maxpix]
   } elseif { $type == "exp" } {
      set reg [makeExpModel -exact \
		   $counts $size $aratio $pa *$fparams.psf -max maxpix]
   } else {
      print_msgs
      error "Unknown model type: $type"
   }
   set nrow [exprGet $reg.nrow]; set ncol [exprGet $reg.ncol]

   set sky [handleBindFromHandle [handleNew] *$fparams.sky]
   set skylev \
       [expr [exprGet $sky.reg->rows_s32<0><0>]/(1<<[exprGet $sky.shift])]
   regIntConstAdd $reg [expr $skylev]
   handleSet $fparams.bkgd $skylev

   set levels [vectorExprEval $skylev+[softBiasGet]+10]
   
   if 0 {				# read or write all regions to disk
      set read_region 0;		# read rather than write region?
      set badfile "bad.fts"
   }
   if {[info exists read_region] && $read_region} {
      set breg [regReadAsFits [regNew] $badfile];
      handleSetFromHandle $breg.mask $reg.mask
      handleSet (long)$reg.mask 0; regDel $reg; set reg $breg
      
      set nrow [exprGet $reg.nrow]; set ncol [exprGet $reg.ncol]
      
      set tmp [hdrGetAsAscii $reg.hdr "TYPE"]
      regsub { *$} $tmp "" type;	# strip trailing spaces
      set pa [format "%g" [hdrGetAsDbl $reg.hdr "PA"]]
      set counts [format "%g" [hdrGetAsDbl $reg.hdr "FLUX"]]
      set size [format "%g" [hdrGetAsDbl $reg.hdr "SIZE"]]
      set aratio [format "%g" [hdrGetAsDbl $reg.hdr "ARATIO"]]
      echo "reading data from disk " \
	  "(size = $size, counts = $counts, aratio = $aratio, pa = $pa)"

      set noise 0
   } else {
      if $noise {
	 regIntNoiseAdd $reg $rand 1000 $gain
      }
      
      if {[info exists read_region] && !$read_region} {
	 hdrInsWithAscii $reg.hdr "TYPE" $type
	 hdrInsWithDbl $reg.hdr "PA" $pa
	 hdrInsWithDbl $reg.hdr "FLUX" $counts
	 hdrInsWithDbl $reg.hdr "SIZE" $size
	 hdrInsWithDbl $reg.hdr "ARATIO" $aratio
	 
	 regWriteAsFits $reg $badfile;
      }
   }

   if $noise {
      set pi [expr acos(-1)]
      set S [expr $counts/2*$gain];	# counts within r_e
      set var [expr ($aratio*$pi*pow($size,2)*$skylev + \
			 $counts/2)*$gain];# variance within r_e
      set SN [expr $S/sqrt($var)]
      
      set psf_filt_size 11; set psfsigma 1
      set hpsf_filt_size [expr int($psf_filt_size/2)]
      
      set scr [regNew $nrow $ncol]
      set sreg [regNew $nrow $ncol]
      
      convolveWithGaussian $sreg $reg $scr $psf_filt_size $psfsigma
      regDel $scr
      
      handleSetFromHandle $sreg.mask $reg.mask; # set bits in $reg.mask
   
      set objects [regFinder $sreg $levels $fparams \
		       -row0 $hpsf_filt_size  -col0 $hpsf_filt_size \
		       -row1 -$hpsf_filt_size -col1 -$hpsf_filt_size];
      
      handleSet (long)$sreg.mask 0;
      regDel $sreg
   } else {
      set SN 1e10
      
      set objects [regFinder $reg $levels $fparams]
   }

   if 0 {
      mtv $reg;
      puts -nonewline "$type $counts $size $aratio $pa: "
      if {[gets stdin] == "q"} { error "" }
   }

   set objs [objectToObject1ChainConvert $objects 0 $reg $sky]
   #
   # Find the highest peak
   #
   set obj [object1New]
   handleSet $obj.skyErr 0   
   
   set curs [chainCursorNew $objs]
   while {[chainWalk $objs $curs] != ""} {
      set tmp [chainElementRemByPos $objs HEAD]
      if {[exprGet (int)$obj.peaks] == 0 ||
	  [exprGet $tmp.peaks->peaks<0>->peak] > 
					[exprGet $obj.peaks->peaks<0>->peak]} {
	 object1Del $obj; set obj $tmp
      } else {
	 object1Del $tmp
      }
   }
   chainCursorDel $objs $curs
   set objc [objcNew 1]; handleSetFromHandle $objc.color<0> &$obj

   set npix 0
   if {[exprGet $obj.mask] != 0x0} {
      loop i 0 [exprGet $obj.mask->nspan] {
	 set npix \
	     [expr $npix+[exprGet $obj.mask->s<$i>.x2]-[exprGet $obj.mask->s<$i>.x1]+1]
      }
   }
   if {$npix == 0} {
      echo "Object is not detected"
      #maybe_display $reg $obj $size $aratio $pa
      
      set test_it 0
   }

   if {$test_it && $maxpix > 65000} {
      echo "Object is saturated"
      if 0 {
	 maybe_display $reg $obj $size $aratio $pa
      }
      set test_it 0
   }

   if {$test_it} {
      handleSetFromHandle $fparams.data &$reg
      if [catch {fitCellPsfModel $objc 0 $fiparams} msg] {
	 echo "$msg"
	 global msgs; lappend msgs "$msg"
	 incr ret

	 set test_it 0
      }
   }
   
   if {$test_it && $npix < 5} {
      echo "Object is too small to classify "
      handleSet $obj.deV_L 0
      handleSet $obj.exp_L 0

      set test_it 0
   }

   if $test_it {
      if [catch {
	 fitCellDevModel $objc 0 $fiparams
	 fitCellExpModel $objc 0 $fiparams
      } msg] {
	 echo Error: $msg
      }
#
# Display image of chi^2 surface?  Debugging only, of course
#
      if 0 {
	 regDel [mtv [fitCellChisq $objc 0 $fiparams 10000 0 1 20 0 2 20 -sky]]
	 error "fitCellChisq $objc 0 $fiparams 10000 0 1 20 0 2 20 -sky"
      }
#
# relax some tolerances for difficult cases
#
      set ab_tol "0.05|5%"
      set L_tol 1.00;
      set re_tol "0.3|5%"
      set counts_tol "4%"

      if {$counts <= 1e3} {
	 set test_it 0
      } elseif {$counts <= 1e4} {
	 set re_tol 15%
	 if {$aratio < 0.2} {
	    set re_tol 16.5%
	 }
	 if {$size < 1} {
	    set ab_tol 1e10
	    set re_tol 10;		# n.b. the total counts test
	 } elseif {$size < 2.5} {
	    set ab_tol 0.05|6%|2s
	 } elseif {$size < 5} {
	    set ab_tol 0.05|7%|1.5s
	    set counts_tol 6%
	 } else {
	    set test_it 0
	 }
      } elseif {$counts <= 1e5} {
	 if {$size < 0.3} {
	    if {$aratio < 0.5} {
	       set ab_tol 1e10
	    } else {
	       set ab_tol 0.2
	    } 
	 } elseif {$size < 0.5} {
	    set ab_tol 0.1
	 } elseif {$size < 1} {
	    if {$aratio < 0.2} {
	       set ab_tol 0.1
	    } elseif {$aratio < 0.4} {
	       set ab_tol 0.06
	    }
	 }
      }
      #
      # check the likelihood values
      #
      set star_L [exprGet $obj.star_L];
      set deV_L [exprGet $obj.deV_L]
      set exp_L [exprGet $obj.exp_L]
      set max [expr {$star_L > $exp_L ? ($star_L > $deV_L ? "star" : "deV") : \
			 ($exp_L > $deV_L ? "exp" : "deV")}]
      set max_L [set ${max}_L]

      if $test_it {
	 if { $type != "star" } {
	    #
	    # check the fit parameters
	    #
	    if {[check_it (&$obj) counts_$type $counts $counts_tol] != 0} {
	       incr ret
	    }

	    if { [check_it (&$obj) r_$type $size $re_tol] != 0} {
	       incr ret
	    }
	    
	    if {[check_it (&$obj)  ab_$type $aratio $ab_tol] != 0} {
	       incr ret
	    }
	    
	    if 0 {
	       if { [check_it (&$obj)  phi_$type $pa 5] != 0} {
		  incr ret
	       }
	    }
	 }
	 if { [check_it (&$obj) ${type}_L 1 $L_tol] != 0} {
	    incr ret
	 }

	 if {[set ${max}_L] != [set ${type}_L]} {
	    if {[set ${max}_L] - [set ${type}_L] > 0.1 && \
		    $size > 0.3 && $max_L > 0.1 && $type != "star"} {
	       set msg "Maximum likelihood is $max not $type ($max_L not [set ${type}_L])"

	       echo "$msg";
	       global msgs; lappend msgs "$msg"

	       incr ret
	    }
	 }
      }
      #
      # Maybe write to output file
      #
      catch {
	 global out;

	 if {$type == "star"} {
	    puts $out "$type   \
	  [exprGet $obj.star_L] [exprGet $obj.deV_L] [exprGet $obj.exp_L]  \
		       [exprGet $obj.r_deV] [exprGet $obj.r_exp] \
		       [exprGet $obj.counts_deV] [exprGet $obj.counts_exp] \
		       [exprGet $obj.ab_deV] [exprGet $obj.ab_exp] \
		       [exprGet $obj.phi_deV] [exprGet $obj.phi_exp] \
                       [exprGet $psf.sigmax1] [exprGet $psf.sigmax2] [exprGet $psf.b] \
                      $SN"
	 } else {
	    puts $out "$type   \
	  [exprGet $obj.star_L] [exprGet $obj.deV_L] [exprGet $obj.exp_L]  \
		      $size [exprGet $obj.r_$type]   \
		      $counts [exprGet $obj.counts_$type]   \
		      $aratio [exprGet $obj.ab_$type]   \
		      $pa [exprGet $obj.phi_$type] \
                      [exprGet $psf.sigmax1] [exprGet $psf.sigmax2] [exprGet $psf.b] \
                      $SN"
	 }
	 flush $out
      }
   }

   if {$ret || $show_failures > 1} {
      maybe_display $reg $obj $size $aratio $pa
   }
   #
   # clean up
   #
   set mask [handleBindFromHandle [handleNew] *$reg.mask];
   handleSet $reg.mask 0x0;
   
   handleDel $psf
   handleDel $obj
   objcDel $objc
   spanmaskDel $mask;
   regDel $reg; handleSet $fparams.data 0
   objectChainDel $objects; chainDel $objs;
   handleDel $sky; vectorExprDel $levels
   handleDel $fparams
   
   return $ret
}

#
# This is a debugging proc to return a region containing a chisq surface
#
proc chisqRegGet {type counts size aratio pa fiparams {noise 0}} {
   global gain rand
   
   set fparams [handleBindFromHandle [handleNew] $fiparams.frame<0>]
   
   set skysig 1
   if { $type == "star" } {
      set psf [handleBindFromHandle [handleNew] *$fparams.psf]
      set reg [regNew 100 100]; regClear $reg
      dgpsfAddToReg $reg $psf 50.5 50.5 -a $counts
      set mask [spanmaskNew -nrow 100 -ncol 100]
      handleSetFromHandle $reg.mask (MASK*)&$mask; handleDel $mask
      handleDel $psf
   } elseif { $type == "deV" } {
      set reg [makeDevModel $counts $size $aratio $pa *$fparams.psf]
   } elseif { $type == "exp" } {
      set reg [makeExpModel $counts $size $aratio $pa *$fparams.psf]
   } else {
      global msgs; lappend msgs "Unknown model type: $type"
      print_msgs 1
   }

   if 1 {				# make the model larger
      set minsize 300;
      set ncol [exprGet $reg.ncol]; set nrow [exprGet $reg.nrow]
      if {$ncol < $minsize} {
	 set tmp $reg
	 set reg [regNew $minsize $minsize];
	 regIntSetVal $reg [softBiasGet]
	 set mask [spanmaskNew -nrow $minsize -ncol $minsize]
	 handleSetFromHandle $reg.mask (MASK*)&$mask; handleDel $mask
	 set off [expr ($minsize-$ncol)/2]
	 set sreg [subRegNew $reg $nrow $ncol $off $off]
	 regIntCopy $sreg $tmp
	 regDel $sreg; regDel $tmp
	 set nrow $minsize;  set ncol $minsize
      }
   }
   
   set sky [handleBindFromHandle [handleNew] *$fparams.sky]
   set skylev \
       [expr [exprGet $sky.reg->rows_s32<0><0>]/(1<<[exprGet $sky.shift])]
   regIntConstAdd $reg [expr $skylev]
   set levels [vectorExprEval $skylev+[softBiasGet]+10]
   set objects [regFinder $reg $levels $fparams]
   regStatsFromQuartiles $reg -coarse 1 -counts tot
   set tot [expr $tot-1000*[exprGet $reg.nrow]*[exprGet $reg.ncol]]
   if $noise {
      regIntNoiseAdd $reg $rand 1000 $gain;	# _after_ finding the peak
      set SN [expr sqrt($tot/$gain)]
   } else {
      set SN 1e10
   }

   set objs [objectToObject1ChainConvert $objects 0 $reg $sky]
   #
   # Find the highest peak
   #
   set obj [object1New]
   handleSet $obj.peaks->peaks<0>->peak 0
   
   set curs [chainCursorNew $objs]
   while {[chainWalk $objs $curs] != ""} {
      set tmp [chainElementRemByPos $objs HEAD]
      if {[exprGet $tmp.peaks->peaks<0>->peak] > \
	      [exprGet $obj.peaks->peaks<0>->peak]} {
	 object1Del $obj; set obj $tmp
      } else {
	 object1Del $tmp
      }
   }
   chainCursorDel $objs $curs

   handleSetFromHandle $fparams.data &$reg
#
# ahhhh, at last. We're ready to go
#
   if { $type == "deV" } {
      set flag -deV
   } else {
      set flag ""
   }
   set creg [eval fitCellChisq $obj 0 $fiparams $flag \
		 $pa $counts 0.01 1 100 1 40 50]
   #
   # clean up
   #
   set mask [handleBindFromHandle [handleNew] *$reg.mask];
   handleSet $reg.mask 0x0;
   
   mtv $creg

   object1Del $obj;
   spanmaskDel $mask; regDel $reg; handleSet $fparams.data 0
   objectChainDel $objects; chainDel $objs;
   handleDel $sky; vectorExprDel $levels
   handleDel $fparams

   return $creg
}

#
# Display the current region if show_failures is true; optionally abort
#
proc maybe_display {reg obj size aratio pa} {
   global fiparams show_failures;

   if $show_failures {
      flush stderr
      catch { mtv $reg; saoZoom 4 }
      while {1} {
	 puts -nonewline \
	     "(size = $size, a/b = $aratio pa = $pa)  continue? \[ cdeDEfhqs\] ";
	 set reply [gets stdin]
	 if {$reply == ""} {
	    break
	 } elseif {$reply == "c"} {
	    set show_failures 0
	    break;
	 } elseif {[regexp {^[dDeE]$} $reply]} {
	    if {$reply == "d" || $reply == "D"} {
	       set type "deV"
	    } else {
	       set type "exp"
	    }

	    set re [exprGet ($obj).r_$type]
	    if {$re < $size} { set re $size }
	    set rmax [expr int(2*(int($re)/2 + 1.5))]
	    set nsize [expr 10*$rmax]
	    set naratio 10

	    if {$reply == "D" || $reply == "E"} {
	       set nsize [expr 10*$nsize]
	       set naratio [expr 10*$naratio]
	       set zoom 1
	    } else {
	       set zoom 8
	    }

	    regDel [mtv -log -s 2 [fitCellChisq -$type $obj 0 $fiparams 10000 \
				    0 1 $naratio 0 $rmax $nsize -sky -per_dof]]

	    saoReset 2
	    draw_cross + [expr $naratio*$aratio] [expr $naratio*$size] "-s 2"

	    if {$zoom != 1} {
	       saoZoom -s 2 $zoom
	    }
	 } elseif {$reply == "f"} {
	    set sky [exprGet $fiparams.frame<0>.global_sky]
	    regStatsFromQuartiles $reg -coarse 1 -counts tot
	    echo \
		[format "Total flux: %.1f (sky: %.2f)" \
		     [expr $tot - ([softBiasGet] + $sky)*[exprGet $reg.nrow]*[exprGet $reg.ncol]]\
		     $sky]
	 } elseif {$reply == "h"} {
	    echo \
		{Options: c[ontinue] d[eV chi^2] e[xp chi^2] ([DE]: larger) f[lux] h[elp] q[uit] [show]}
	 } elseif {$reply == "q"} {
	    uplevel #0 "set objc \"$obj\""
	    uplevel #0 "set reg \"$reg\""
	    error "Set \$reg and \$objc at global scope"
	    break;
	 } elseif {$reply == "s"} {
	    echo " type       L       counts    r_e    a/b   phi"
   
	    foreach type [list star deV exp] {
	       if {$type == "star"} {
		  set counts [exprGet ($obj).psfCounts]
	       } else {
		  set counts [exprGet ($obj).counts_$type]
	       }
	       
	       puts -nonewline [format " %-5s %6.6f %10.0f" \
				    $type [exprGet ($obj).${type}_L] $counts]
	       
	       if {$type != "star"} {
		  set counts [exprGet ($obj).counts_$type]
		  set re [exprGet ($obj).r_$type]
		  set ab [exprGet ($obj).ab_$type]
		  puts -nonewline [format " %6.2f %6.2f %5.1f" \
				     $re $ab [exprGet ($obj).phi_$type]]
	       }
	       echo ""
	    }
	 } else {
	    echo Unknown reply $reply
	 }
      }
   }
}

###############################################################################
#
# Find an object in a region
#
proc find_objc {reg rpos cpos lev skylev psf fiparams cal1} {
   global fieldstat
   set fparams [handleBindFromHandle [handleNew] $fiparams.frame<0>]
   handleSet $fparams.bkgd $skylev
   set sky [binregionNewFromConst 0]
   handleSetFromHandle $fparams.sky &$sky

   set mask [spanmaskNew -nrow [exprGet $reg.nrow] -ncol [exprGet $reg.ncol]]
   handleSetFromHandle $reg.mask (MASK*)&$mask; handleDel $mask
   #
   # set up measure objects
   #
   handleSetFromHandle $fparams.data &$reg
   handleSetFromHandle $cal1.psf &$psf
   set prof [compositeProfileNew]
   handleSet $prof.psfCounts 1e4
   handleSetFromHandle $cal1.prof &$prof; handleDel $prof
   
   measureObjColorSet \
       $fiparams $fieldstat 0 $cal1 $reg $sky *$fparams.skyErr ""
   handleSet $fiparams.frame<0>.global_sky $skylev

   tst_set_psf *$cal1.psf 0
   
   handleSet $cal1.psf 0
   #
   # Find the objects
   #
   set levels [vectorExprEval $lev]
   set objects [regFinder $reg $levels $fparams]
   #
   # Remove all but the brightest OBJECT on the chain (we may not have
   # smoothed, remember).  Actually, remove them all then put the brightest
   # back
   #
   # As an alternative, it is possible to specify rpos and cpos in the
   # inputs file to specify the approximate centre of the desired object (i.e.
   # the position of the highest pixel above the detection threshold)
   #
   set d2min 100000
   set curs [chainCursorNew $objects]
   while {[chainWalk $objects $curs] != ""} {
      set obj [chainElementRemByPos $objects HEAD]
	 
      set d2 [expr pow([exprGet $obj.rowc] - $rpos, 2) + \
		   pow([exprGet $obj.colc] - $cpos, 2)]
      if 0 {				# useful for setting rpos, cpos
	 echo [exprGet $obj.rowc] [exprGet $obj.colc]
	 draw_cross + [exprGet $obj.rowc] [exprGet $obj.colc]
      }

      if {![info exists our_obj]} {
	 set our_obj $obj
	 set d2min $d2
	 continue
      }
      
      set use_this 0;			# is this the desired object?
      if {$rpos >= 0} {			# there's a requested position
	 if {$d2 < $d2min} {
	    set d2min $d2
	    set use_this 1
	 }
      } else {				# choose the brightest
	 if {([exprGet $obj.peaks->peaks<0>->peak] > \
		  [exprGet $our_obj.peaks->peaks<0>->peak])} {
	    set use_this 1
	 }
      }

      if $use_this {
	 objectDel $our_obj; set our_obj $obj
      } else {
	 objectDel $obj
      }
   }
   chainCursorDel $objects $curs

   chainElementAddByPos $objects $our_obj HEAD
   handleDel $our_obj
   #
   # get our OBJC from the OBJECT chain
   #
   set objs [objectToObject1ChainConvert $objects 0 $reg $sky]
   set obj [chainElementRemByPos $objs HEAD]
   set objc [objcNew 1]; handleSetFromHandle $objc.color<0> &$obj
   handleDel $obj
   #
   # clean up
   #
   spanmaskDel [handleBindFromHandle [handleNew] *$reg.mask];
   handleSet $reg.mask 0x0;

   handleDel $fparams

   objectChainDel $objects; chainDel $objs;
   binregionDel $sky; vectorExprDel $levels

   return $objc
}

###############################################################################
#
# Test the code that identifies an object superimposed on something,
# a constant or a galaxy
#
proc test_obj_on_something {test reg psf L0 lev rpos cpos fiparams cal1} {
   global show_failures

   set ret 0
   set skylev 100
#
# Find the object in question
#
   set objc [find_objc $reg $lev $rpos $cpos $skylev $psf $fiparams $cal1]
#
# Measure things
#
   if [catch \
	  {set val [fitCellPsfModel $objc 0 $fiparams -fit_sky -nann 3]} msg] {
      echo $msg
      global msgs; lappend msgs "$msg"
      incr ret
   }
   set L [exprGet $objc.color<0>->star_L]
   global verbose; if { [info exists verbose] && $verbose } {
      echo "                      Measured Stellar Likelihood $L (L0 = $L0)"
   }
   if {$L0 < 0} {			# |$L0| is maximum allowed
      set L0 [expr -$L0]
      if {$L > $L0} {
	 set msg "Test \"$test\": Measured Stellar Likelihood $L > $L0"
	 echo "$msg";
	 global msgs; lappend msgs "$msg"
	 incr ret
      }
   } else {
      if {$L < $L0} {
	 set msg "Test \"$test\": Measured Stellar Likelihood $L < $L0"
	 echo "$msg";
	 global msgs; lappend msgs "$msg"
	 incr ret
      }
   }

   if {$ret || $show_failures > 1} {
      maybe_display $reg (*$objc.color<0>) $test "" ""
   }
#
# clean up
#
   handleSet $fiparams.frame<0>.sky 0
   handleSet $fiparams.frame<0>.data 0

   measureObjUnset $fiparams

   global abort_on_error
   if {$ret && [info exists abort_on_error] && $abort_on_error} {
      echo "OBJC is $objc"
   } else {
      objcDel $objc
   }

   return $ret
}

#
# Here are the procs that make regions of galaxies + stars/galaxies
#
# A star on a constant background
#
proc make_region_star1 {psf skylev fiparams {noise 0}} {
   set reg [regNew 200 200];
   regIntSetVal $reg [expr [softBiasGet] + $skylev]

   regIntConstAdd $reg 123

   set rowc 95.25; set colc 105.25

   set sig1 [exprGet $psf.sigmax1]; set sig2 [exprGet $psf.sigmax2];
   set b [exprGet $psf.b]
   dgpsfAddToReg $reg $psf $rowc $colc -a 8000

   if $noise {
      global gain rand
      regIntConstAdd $reg $skylev
      regIntNoiseAdd $reg $rand 1000 $gain
      regIntConstAdd $reg -$skylev
   }

   set lev 5000
   set L0 0.90;				# minimum permissible Likelihood
   return "$reg $rowc $colc $lev $L0"
}

#
# A star on an elliptical
#
proc make_region_star2 {psf skylev fiparams {noise 0}} {
   set gal [makeExpModel 5000000 10 0.5 40 $psf]

   set size 200
   set nrow [exprGet $gal.nrow]; set ncol [exprGet $gal.ncol]
   if {$size < $nrow} { set size $nrow }
   if {$size < $ncol} { set size $ncol }
   
   set reg [regNew $size $size];
   regIntSetVal $reg [expr [softBiasGet] + $skylev]

   set sreg [subRegNew $reg [exprGet $gal.nrow] [exprGet $gal.ncol] \
		 [expr int(0.5*([exprGet $reg.nrow]-[exprGet $gal.nrow]))] \
		 [expr int(0.5*([exprGet $reg.ncol]-[exprGet $gal.ncol]))]]
   regIntConstAdd $gal [expr $skylev]
   regIntCopy $sreg $gal
   
   spanmaskDel [handleBindFromHandle [handleNew] *$gal.mask]
   handleSet $gal.mask 0; regDel $gal;
   regDel $sreg

   set rowc [expr $size/2 - 4.75]; set colc [expr $size/2 + 4.25]
   set sig1 [exprGet $psf.sigmax1]; set sig2 [exprGet $psf.sigmax2];
   set b [exprGet $psf.b]
   dgpsfAddToReg $reg $psf $rowc $colc -a 8000

   if $noise {
      global gain rand
      regIntConstAdd $reg $skylev
      regIntNoiseAdd $reg $rand 1000 $gain
      regIntConstAdd $reg -$skylev
   }

   set lev 13500
   set L0 0.95;				# minimum Likelihood
   return "$reg $rowc $colc $lev $L0"
}

#
# An elliptical (re 1.2, a/b 0.25) on a constant background
#
proc make_region_galaxy1 {psf skylev fiparams {noise 0}} {
   set reg [regNew 200 200];
   regIntSetVal $reg [expr [softBiasGet] + $skylev]

   regIntConstAdd $reg 123

   set rowc 94.25; set colc 105.25

   set gal [makeExpModel 90000 1.2 0.25 -50 $psf]
   set sreg [subRegNew $reg [exprGet $gal.nrow] [exprGet $gal.ncol] \
		 [expr int($rowc-0.5*[exprGet $gal.nrow])] \
		 [expr int($colc-0.5*[exprGet $gal.ncol])]]
   regAdd $sreg $gal
   regIntConstAdd $sreg -1000
   
   spanmaskDel [handleBindFromHandle [handleNew] *$gal.mask]
   handleSet $gal.mask 0; regDel $gal;
   regDel $sreg

   if $noise {
      global gain rand
      regIntConstAdd $reg $skylev
      regIntNoiseAdd $reg $rand 1000 $gain
      regIntConstAdd $reg -$skylev
   }

   set lev 5000
   set L0 -.01;				# -maximum permissible Likelihood
   return "$reg $rowc $colc $lev $L0"
}

#
# An elliptical (re 1.2, a/b 0.25) on an elliptical; axes perpendicular
#
proc make_region_galaxy2 {psf skylev fiparams {noise 0}} {
   set gal [makeExpModel 5000000 10 0.5 40 $psf]

   set size 200
   set nrow [exprGet $gal.nrow]; set ncol [exprGet $gal.ncol]
   if {$size < $nrow} { set size $nrow }
   if {$size < $ncol} { set size $ncol }
   
   set reg [regNew $size $size]
   regIntSetVal $reg [expr [softBiasGet] + $skylev]

   set sreg [subRegNew $reg [exprGet $gal.nrow] [exprGet $gal.ncol] \
		 [expr int(0.5*([exprGet $reg.nrow]-[exprGet $gal.nrow]))] \
		 [expr int(0.5*([exprGet $reg.ncol]-[exprGet $gal.ncol]))]]
   regIntConstAdd $gal [expr $skylev]
   regIntCopy $sreg $gal
   
   spanmaskDel [handleBindFromHandle [handleNew] *$gal.mask]
   handleSet $gal.mask 0; regDel $gal;
   regDel $sreg

   set rowc 94.25; set colc 105.25
   set gal [makeExpModel 90000 1.2 0.25 -50 $psf]
   set sreg [subRegNew $reg [exprGet $gal.nrow] [exprGet $gal.ncol] \
		 [expr int($rowc-0.5*[exprGet $gal.nrow])] \
		 [expr int($colc-0.5*[exprGet $gal.ncol])]]
   regAdd $sreg $gal
   regIntConstAdd $sreg -1000
   
   spanmaskDel [handleBindFromHandle [handleNew] *$gal.mask]
   handleSet $gal.mask 0; regDel $gal;
   regDel $sreg

   if $noise {
      global gain rand
      regIntConstAdd $reg $skylev
      regIntNoiseAdd $reg $rand 1000 $gain
      regIntConstAdd $reg -$skylev
   }

   set lev 13300
   set L0 -.025;			# -maximum permissible Likelihood
   return "$reg $rowc $colc $lev $L0"
}

#
# An elliptical (re 1.2, a/b 0.25) on an elliptical; axes parallel
#
proc make_region_galaxy3 {psf skylev fiparams {noise 0}} {
   set gal [makeExpModel 5000000 10 0.5 40 $psf]

   set size 200
   set nrow [exprGet $gal.nrow]; set ncol [exprGet $gal.ncol]
   if {$size < $nrow} { set size $nrow }
   if {$size < $ncol} { set size $ncol }
   
   set reg [regNew $size $size]
   regIntSetVal $reg [expr [softBiasGet] + $skylev]
   
   set sreg [subRegNew $reg [exprGet $gal.nrow] [exprGet $gal.ncol] \
		 [expr int(0.5*([exprGet $reg.nrow]-[exprGet $gal.nrow]))] \
		 [expr int(0.5*([exprGet $reg.ncol]-[exprGet $gal.ncol]))]]
   regIntConstAdd $gal [expr $skylev]
   regIntCopy $sreg $gal
   
   spanmaskDel [handleBindFromHandle [handleNew] *$gal.mask]
   handleSet $gal.mask 0; regDel $gal;
   regDel $sreg

   set rowc 95.25; set colc 105.25
   set gal [makeExpModel 90000 1.5 0.25 40 $psf]
   set sreg [subRegNew $reg [exprGet $gal.nrow] [exprGet $gal.ncol] \
		 [expr int($rowc-0.5*[exprGet $gal.nrow])] \
		 [expr int($colc-0.5*[exprGet $gal.ncol])]]
   regAdd $sreg $gal
   regIntConstAdd $sreg -1000
   
   spanmaskDel [handleBindFromHandle [handleNew] *$gal.mask]
   handleSet $gal.mask 0; regDel $gal;
   regDel $sreg

   if $noise {
      global gain rand
      regIntConstAdd $reg $skylev
      regIntNoiseAdd $reg $rand 1000 $gain
      regIntConstAdd $reg -$skylev
   }

   set lev 13300
   set L0 -.025;			# -maximum permissible Likelihood
   return "$reg $rowc $colc $lev $L0"
}

###############################################################################
#
# Run all the tests
#
if ![info exists catalog] {
   set catalog "$env(PHOTO_DIR)/lib/rgalprof.dat"
}
if ![info exists cellcat] {
   set cellcat "$env(PHOTO_DIR)/lib/cellprof.dat"
}

set failed 0
set msgs ""

foreach f "catalog cellcat" {
   set file [set $f]
   if {![file readable $file]} {
      lappend msgs "you must make $file before running tests"

      incr failed
   }
}
if $failed {
   lappend msgs "required input files are missing (run \"make datafiles\")"

   print_msgs 1
}

initProfileExtract
initFitobj $catalog
initCellFitobj $cellcat 1
#
# Set the FIELDPARAMS for this field
#
set gain 6; set dark_variance 0;
set sky 200

set filterlist "r"
set cal1 [calib1New $filterlist]
set fiparams [fieldparamsNew $filterlist]
set fparams [handleBindFromHandle [handleNew] $fiparams.frame<0>]
set fieldstat [fieldstatNew]
handleSet $cal1.gain $gain
handleSet $cal1.dark_variance $dark_variance

#
# Models to build; this proc sets the canonical set
#
proc set_canonical {_countses _psfs _res _aratios _pas} {
   upvar $_countses countses  $_psfs psfs  $_res res  $_aratios aratios  $_pas pas
   
   set psfs [list \
		 "1 3 0" \
		 "1 3 0.0500" \
		 "1 3 0.1111" \
		]

   set countses [list 5e2 1e3 1e4 1e5]
   set pas [list 0 30 60 90]
   
   set res(deV) [list 5.36548 2.77546 0.74266 0.247553]
   set res(exp) [list 5.55093 2.30499 0.768329 0.25611]
   set res(star) [list 1 1 1 1]
   
   set aratios(deV) [list 0.95 0.438691 0.333333 0.19245]
   set aratios(exp) [list 0.95 0.438691 0.333333 0.14623]
   set aratios(star) [list 0   0.125     0.25    0.50]

   return 1;				# allowed number of failures
}

if 1 {
   set out [open "errors.out" "w"]
}
#
# Misc sets of parameters for debugging the model fit code.
#
set nfail 0
if [info exists aratios] { unset aratios }

set countses [list 1e4]
set pas [list 0 30 60 90]
set psfs [list \
	      "1 3 0" \
	      "1 3 0.0500" \
	      "1 3 0.1111" \
	      "1 3 0.1500" \
	     ]
set types [list deV exp star]

if 0 {
   set res(deV) [list 74.9375 60.1554 48.2893 38.7638 31.1173 24.9792 20.0518\
		     16.0964 12.9213 10.3724 8.32639 6.68394 5.36548 4.30709\
		     3.45748 2.77546 2.22798 1.78849 1.4357 1.15249 0.925154\
		     0.74266 0.596164 0.478566 0.384164 0.308385 0.247553]
   set res(exp) [list 149.875 120.311 96.5786 77.5276 62.2346 49.9583 40.1036\
		     32.1929 25.8425 20.7449 16.6528 13.3679 10.731 8.61418\
		     6.91496 5.55093 4.45596 3.57698 2.87139 2.30499 1.85031\
		     1.48532 1.19233 0.957131 0.768329 0.61677 0.495107 \
		     0.397443 0.319044 0.25611]
   set res(star) [list 1]
   set aratios(deV) [list 1 0.759836 0.57735 0.438691 0.333333 0.253279 \
			 0.19245]
   set aratios(exp) [list 1 0.759836 0.57735 0.438691 0.333333 0.253279 \
			 0.19245 0.14623 0.111111]
   set aratios(star) [list 1 1 1 1  1 1 1 1  1]
} elseif 0 {
   set res(deV) [list 16.0964 10.3724 5.36548 2.77546 1.4357 0.925154 \
		     0.74266 0.247553]
   set res(exp) [list 16.6528 10.731 5.55093 2.30499 1.48532 0.957131 \
		     0.768329 0.25611]
   set res(star) [list 1]
   
   set aratios(deV) [list 1 0.438691 0.333333 0.19245]
   set aratios(exp) [list 1 0.438691 0.333333 0.14623]
   set aratios(star) [list 0   0.125     0.25    0.50]
}

if 0 {
   set psfs [list \
		 "0.8 2.4 0.0000" \
		 "0.8 2.4 0.0500" \
		 "0.8 2.4 0.1111" \
		 "0.8 2.4 0.1500" \
		 "0.8 2.0 0.1111" \
		 "0.8 2.2 0.1111" \
		 "0.8 2.4 0.1111" \
		 "0.8 2.6 0.1111" \
		 "1 2 0.0000" \
		 "1 2 0.0500" \
		 "1 2 0.1111" \
		 "1 2 0.1500" \
		 "1 3 0.0000" \
		 "1 3 0.0500" \
		 "1 3 0.1111" \
		 "1 3 0.1500" \
		 "1 4 0.0000" \
		 "1 4 0.0500" \
		 "1 4 0.1111" \
		 "1 4 0.1500" \
		 "1.5 4.5 0.0000" \
		 "1.5 4.5 0.0500" \
		 "1.5 4.5 0.1111" \
		 "1.5 4.5 0.1500" \
		]
} elseif 0 {
   set psfs [list "1 3 0.1111"]
   set countses [list 1e3]
   set types [list "star"]
} elseif 0 {
   set res(deV) [list 2.77546]
   set res(exp) [list 1.48532]

   set aratios(deV) [list 1]
   set aratios(exp) [list 0.14623]

   set pas [list 0 30 90]
   set psfs [list "1 3 0.1111"]
   set types [list exp deV]

   set countses [list 1e4]   
} elseif 0 {
   set pas [list 0]
   set pas [list 0 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 89.999]
   
   set res(exp) [list 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093]
   set res(star) [list 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093 5.55093]

#   set pas [list 60]
#   set res(star) [list 5.55093]
   set res(deV) [list 5.55093]
   
   set aratios(deV) [list 0.14623]
   set aratios(star) [list 1 1 1]

   set psfs [list "1 3 0.1111"]
   set types [list star]

   set countses [list 5e5]
}
#
# Set the canonical parameters for tests that photo must pass
#
if {![info exists aratios]} {
   set nfail [set_canonical countses psfs res aratios pas]
   set countses [list 1e4]
}
set failed 0

#
# First test some parts of the whole. Specifically, superimposing
# objects on other objects, and asking if they look like PSFs
#
set psf [dgpsfNew -sigmax1 1 -sigmax2 3 -b 0.1]

handleSet $psf.sigma1_2G [exprGet $psf.sigmax1]
handleSet $psf.sigma2_2G [exprGet $psf.sigmax2]
handleSet $psf.b_2G [exprGet $psf.b]
handleSet $psf.width [expr 1.2*[exprGet $psf.sigma1_2G]]
handleSetFromHandle $cal1.psf &$psf
set sky 200

if {![info exists tests]} {
   set tests [list star1 star2 galaxy1 galaxy2 galaxy3 models]
}

foreach test $tests {
   if {$test == "models"} {		# special case
      continue;
   }
   echo Testing region \"$test\"

   #phRandomSeedSet $rand $seed; echo Resetting rand; # RHL
   set vals [make_region_$test $psf $sky $fiparams 1]
   set reg [lindex $vals 0]
   set rpos [lindex $vals 1]; set cpos [lindex $vals 2]
   set lev [lindex $vals 3]; set L0 [lindex $vals 4]
   #regWriteAsFits $reg foo.fts;		# RHL

   if {0 && $show_failures} {
      catch {saoReset}; mtv $reg; puts -nonewline "? "; gets stdin;# RHL
   }

   incr failed [test_obj_on_something $test $reg $psf $L0 \
		    $rpos $cpos $lev $fiparams $cal1]
   if $failed {
      if {[info exists abort_on_error] && \
	      $abort_on_error} {
	 print_msgs
	 error "...aborting"
      }
   }
   
   regDel $reg
}
dgpsfDel $psf
handleSet $cal1.psf 0

#
# Now go ahead and fit a variety of model galaxies, if test "models" is desired
#
if {[lsearch -exact $tests "models"] == -1} {
   set psfs ""
}

foreach p $psfs {
   set sigma1 [lindex $p 0]
   set sigma2 [lindex $p 1]
   set b [lindex $p 2]

   echo PSF: N(0,$sigma1^2) + $b*N(0,$sigma2^2)

   set psf [dgpsfNew -sigmax1 $sigma1 -sigmax2 $sigma2 -b $b]
   handleSet $psf.sigma1_2G [exprGet $psf.sigmax1]
   handleSet $psf.sigma2_2G [exprGet $psf.sigmax2]
   handleSet $psf.b_2G [exprGet $psf.b]
   handleSet $psf.width [expr 1.2*[exprGet $psf.sigma1_2G]]
   handleSetFromHandle $cal1.psf &$psf
 
   set reg [handleBind [handleNew] 0x0 REGION]
   set skyreg [binregionNewFromConst $sky]
   
   set prof [compositeProfileNew]
   handleSet $prof.psfCounts 1e4
   handleSetFromHandle $cal1.prof &$prof; handleDel $prof
   
   measureObjColorSet $fiparams $fieldstat \
       0 $cal1 $reg $skyreg *$fparams.skyErr ""
   tst_set_psf $psf 0
   
   handleSet $fiparams.frame<0>.global_sky $sky
   handleDel $reg
   
   foreach type $types {
      foreach counts $countses {
	 set counts0 $counts;		# i.e. with no noise added
	 foreach re $res($type) {
	    foreach aratio $aratios($type) {
	       foreach pa $pas {
		  if 0 {
		     set counts [random $counts0]
		  }
		  
		  set add_noise 1
		  if {[set ret \
			   [testFitCellModel $type $counts \
				$re $aratio $pa $fiparams $add_noise]] != 0} {
		     set msg "Failed fitting $type (s1 = $sigma1, s2=$sigma2, b = $b, counts = $counts, r_e = $re, a/b = $aratio, pa = $pa)"

		     echo "$msg";
		     global msgs; lappend msgs "$msg"

		     incr failed $ret
		     if {[info exists abort_on_error] && \
			     $abort_on_error} {
			print_msgs
			error "...aborting"
		     }
		  }
	       }
	    }
	 }
      }
   }
   dgpsfDel $psf; handleDel $skyreg
   measureObjUnset $fiparams
}

catch { close $out }

if {![info exists interactive]} {
   echo Cleaning up...
   handleSet $cal1.psf 0; calib1Del $cal1
   handleSet $fparams.sky 0
   handleDel $fparams; fieldparamsDel $fiparams
   finiProfileExtract
   finiFitobj
   finiCellFitobj
   phRandomDel $rand
}

if $failed {
   if {![info exists nfail] || $failed > $nfail} {
      lappend msgs "Failed $failed tests"
      print_msgs 1
   }
   echo Failed $failed test ($nfail failures are permitted)
}
