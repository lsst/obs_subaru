#
# Make simulated fields
#
proc make_sim {args} {
   set noise 1
   set opts [list \
		 [list [info level 0] "Make a simulated field"] \
		 [list {<nstar>} INTEGER 1 nstar "Desired number of stars"] \
		 [list {-npsf} INTEGER 1 npsf "Desired number of PSF stars"] \
		 [list {-I0} DOUBLE 100 I0 "Central intensity of stars"] \
		 [list {-nrow} INTEGER 0 nrow "Number of rows (default: 100)"] \
		 [list {-ncol} INTEGER 0 ncol "Number of columns (default: 200)"] \
		 [list {-region} STRING "" reg "Region to overwrite with simulation"] \
		 [list {-type} STRING "U16" type "Desired type of region"] \
		 [list {-sky} DOUBLE 100.0 sky "Sky level"] \
		 [list {-gain} DOUBLE 1.0 gain "Gain of amplifier"] \
		 [list {-dark_variance} DOUBLE 0.0 dark_variance "Variance in the absence of signal"] \
		 [list {-nonoise} CONSTANT 0 noise "Don't add any noise"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$reg == ""} {
      if {$nrow == 0} { set nrow 100 }
      if {$ncol == 0} { set ncol 200 }

      set reg [regNew $nrow $ncol]
   } else {
      if {$nrow > 0 || $ncol > 0} {
	 error "Don't be greedy; choose either -reg or -n{row,col}"
      }

      set nrow [exprGet $reg.nrow]; set ncol [exprGet $reg.ncol]
   }
   regIntSetVal $reg [expr [softBiasGet] + $sky]

   set rand [lindex [handleListFromType RANDOM] 0]
   if {[llength $rand] == 0} {
      echo "Making new rand"
      set rand [phRandomNew 100000:3 -seed 12345]
   } else {
      set rand [lindex $rand 0]
   }
   #
   # Add stars
   #
   set psf [dgpsfNew -sigmax1 1 -sigmax2 2 -b 0.1]
   set outer 30;	# maximum radius to calculate profile

   set fd [open foo "w"]
   loop i 0 $nstar {
      set colc [expr [phRandomUniform]*$ncol]
      set rowc [expr [phRandomUniform]*$nrow]

      set rowc [expr int($rowc) + 0.5];
      set colc [expr int($colc) + 0.5];

      #puts -nonewline [format "%-3d (%6.1f, %6.1f) %g\r" $i $rowc $colc $I0]; flush stdout
      puts $fd [format "%-3d (%6.1f, %6.1f) %g" $i $rowc $colc $I0]
      dgpsfAddToReg $reg $psf $rowc $colc -a [expr $i < $npsf ? 10000 : $I0] -outer $outer
   }
   close $fd
   dgpsfDel $psf

   #return $reg
   #
   # Add desired noise
   #
   if $noise {
      set oreg $reg
      set reg [regIntConvert $oreg -type FL32]
      hdrCopy $oreg.hdr $reg.hdr
      regDel $oreg
      
      regIntNoiseAdd $reg $rand $dark_variance $gain -poisson
   }

   if {$type != "FL32"} {
      set oreg $reg
      set reg [regIntConvert $oreg -type $type]
      hdrCopy $oreg.hdr $reg.hdr
      regDel $oreg
   }

   return $reg
}

#
# Process Galaxy simulations as made by Rachel Mandelbaum/Michael Blanton/Richard Cool/Fergal Mullally
#
proc analyzeSims {args} {
   global data_root openit objcIo table

   set mtv_list 0
   set opts [list \
		 [list [info level 0] ""] \
		 [list <run> STRING "" run "Run number"] \
		 [list <col> INTEGER "" camCol "Column number"] \
		 [list {[startField]} STRING "start" startField "Starting field number"] \
		 [list {[endField]} STRING "" endField "Ending field number"] \
		 [list -rerun STRING "" rerun \
		      "Desired reprocessing number (default: youngest)"] \
	         [list -filterlist STRING "all" filterlist \
		      "Filters to process (may be `all')"] \
		 [list {-outfile} STRING "-" outfile "Where to write the outputs"] \
		 [list {-root} STRING $data_root root "The data_root for the sims"] \
		 [list {-mtv_list} CONSTANT 1 mtv_list "Display objects using mtv_list"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set file $root/$run/info.txt
   if [catch {
      set sims [param2Chain $file ""]
   }] {
      error "Failed to read $file"
   }
   if {$sims == ""} {
      error "Failed to read any CHAINs from $file"
   }

   set_run -root $root -rerun $rerun $run $camCol
   param2Chain $openit(photo)/fpPlan.par vals

   if {$startField == "first" || $startField == "start"} {
      set startField [keylget vals startField]
      if {$endField == ""} {
	 set endField "end"
      }
   }
   if {$endField == ""} {
      set endField $startField
   } elseif {$endField == "end" || $endField == "last"} {
      set endField [keylget vals endField]
   }

   set table [openit $startField]
   
   set fpObjc_filterlist [keylget table "filters"]
   if {$filterlist == "all"} {
      set filterlist $fpObjc_filterlist
   } else {
      if {[llength $filterlist] == 1} {
	 set filterlist [split $filterlist {}]
      }
   }
   #
   # Read truth table, and arrange it to be in increasing order of field, with bands ordered with r first
   #
   set truth [list]
   loop i 0 [chainSize $sims] {
      set s [chainElementGetByPos $sims $i]

      set field [exprGet $s.field]
      if {$field < $startField || $field > $endField} {
	 continue
      }

      set f [exprGet $s.band]
      if {[lsearch $filterlist $f] < 0} {
	 continue;
      }
      #
      # Unpack $s
      #
      set elem [list]

      keylset elem field $field
      keylset elem filter $f
      keylset elem rowc [exprGet $s.ypos]
      keylset elem colc [exprGet $s.xpos]
      foreach name [list magnitude sersicflux sersicr50 sersicn axisratio orient] {
	 keylset elem $name [exprGet $s.$name]
      }
      handleDel $s

      lappend truth $elem
   }
   chainDel $sims; unset sims
   #
   # A proc to sort by field, and put the r band detection first within
   # each field.  This is needed as find_objc uses r-band positions, and
   # the sims put out per-band positions
   #
   proc compar {a b} {
      set field_a [keylget a "field"]
      set field_b [keylget b "field"]
      if {$field_a < $field_b} {
	 return -1;
      } elseif {$field_a == $field_b} {
	 set f_a [keylget a "filter"]
	 set f_b [keylget b "filter"]

	 if {$f_a == "r"} {
	    return -1
	 } else {
	    return 1
	 }
      } else {
	 return 1;
      }
   }

   set truth [lsort -command compar $truth]
   #
   # Open output file
   #
   if {$outfile == "-"} {
      set fd stdout
   } else {
      set fd [open $outfile "w"]
   }
   #
   # Read fieldparams for field without simulated galaxy
   #
   set field_nosim 11
   if [catch {
      global fieldparams
      set table [openit $field_nosim]
   } msg] {
      error "Failed to read $field_nosim: $msg"
   }

   loop b 0 [llength $fpObjc_filterlist] {
      set skyBinregion($b) [handleBindFromHandle [handleNew] *$fieldparams.frame<$b>.sky]
      handleSet $fieldparams.frame<$b>.sky 0x0
   }
   #
   # Read photo outputs
   #
   set oldField -1

   set i -1
   foreach elem $truth {
      incr i
      set field [keylget elem "field"]
      set filter [keylget elem "filter"]
      set b [lsearch $fpObjc_filterlist $filter];	# index in $objcIo

      set colc [keylget elem colc]
      set rowc [keylget elem rowc]

      if {$field == $oldField} {
	 ;
      } else {
	 assert {[string compare $filter r] == 0};	# so coordinate system is right
	 if [catch {
	    set table [openit $field]
	    set id [find_objc table $rowc $colc !bright&(!BLENDED|NODEBLEND)]

	    set oldField $field
	 } msg] {
	    echo $msg
	    break
	 }
      }

      foreach name [list magnitude sersicflux sersicr50 sersicn axisratio orient] {
	 set $name [keylget elem $name]
      }

      foreach name [list petro model cmodel exp deV ap10 ap13] {
	 set ${name}Mag [get_mag_from_counts $objcIo $b $name ""]
      }
      foreach name [list r_exp r_deV ab_exp ab_deV sky petroRad petroR50] {
	 set $name [exprGet $objcIo.$name<$b>]
      }
      set fracDeV [exprGet $objcIo.fracPSF<$b>]

      set sky_nosim [binregionInterpolate $skyBinregion($b) [expr int($rowc)] [expr int($colc)]]

      set line [format "%s %d %d %s %d  " $run $rerun $camCol $filter $field]
      append line [format " %.2f %5.2f %5.2f %5.1f  " $sersicn $sersicr50 $axisratio $orient]
      append line [format " %6.2f %6.2f %6.2f %5.1f %5.1f " $fracDeV $r_exp $r_deV $ab_exp $ab_deV]
      append line [format " %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f " $magnitude $petroMag $modelMag $cmodelMag $expMag $deVMag \
		       $ap10Mag $ap13Mag]
      append line [format " %7.3f %6.2f %6.2f" [expr $sky-$sky_nosim] $petroRad $petroR50]
      puts $fd $line

      if {$mtv_list && $f == [lindex $filterlist 0]} {
	 mtv_list $id

	 if {$field != [expr $endField - 1]} {
	    puts -nonewline {Quit? [n] }
	    set ans [gets stdin]
	    if {$ans == "q" || $ans == "y"} {
	       break;
	    }
	 }
      }
   }

   loop b 0 [llength $fpObjc_filterlist] {
      binregionDel $skyBinregion($b)
   }
   
   if {$fd != "stdout"} {
      close $fd
   }
}

#
# Run all the galaxies in an input file as PRs
#
proc runGalaxiesAsPRs {args} {
   global data_root

   set create 0
   set frames 0
   set analyze 0

   set force 0
   set opts [list \
		 [list [info level 0] ""] \
		 [list <infile> STRING "" infile "Input file"] \
		 [list -outfile STRING "" outfile "Output file (- for stdout)"] \
		 [list <prefix> STRING "XXX" prefix "Prefix to use in PR name"] \
		 [list -line0 INTEGER 0 line0 "Starting line to process"] \
		 [list -line1 INTEGER 0 line1 "Last line to process if > 0"] \
		 [list -create CONSTANT 1 create "Create the PRs"] \
		 [list -frames CONSTANT 1 frames "Run frames on the PRs"] \
		 [list -analyze CONSTANT 1 analyze "Analyse the frames outputs"] \
		 [list -rerun INTEGER -1 rerun "Rerun to find data in"] \
		 [list -force CONSTANT 1 force "Force the re-creation of PRs"] \
		 [list -verbose INTEGER 0 verbose "Be chatty"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$create + $frames + $analyze == 0} {
      if $verbose {
	 echo Running create/frames/analyze steps 
      }
      set create 1; set frames 1; set analyze 1
   }

   set ifd [open $infile]

   if {$outfile == "-" || $outfile == ""} {
      set ofd stdout
   } else {
      set ofd [open $outfile "w"]
   }

   set printHeader 1;		# Should I print a header?
   set lineno 0
   while {[gets $ifd line] >= 0} {
      incr lineno
      if [regexp {^ *\#} $line] {
	 continue
      }

      if {$lineno < $line0} {
	 continue
      } elseif {$line1 > 0 && $lineno > $line1} {
	 break
      }

      set i -1
      set run    [lindex $line [incr i]]
      set camCol [lindex $line [incr i]]
      set field  [lindex $line [incr i]]
      set rowc   [lindex $line [incr i]]
      set colc   [lindex $line [incr i]]
      set mag_r  [lindex $line [incr i]]
      set re_r   [lindex $line [incr i]]
      set eyeflag [lindex $line [incr i]]
      set flag   [lindex $line [incr i]]
      set good   [lindex $line [incr i]]

      set prName $prefix-$lineno

      if $create {
	 set createThisPR 1
	 if [file exists $data_root/prs/$prName] {
	    if {$verbose > 1} {
	       echo PR $prName already exists
	    }
	    if !$force {
	       set createThisPR 0
	    }
	 }

	 if $createThisPR {
	    if $verbose {
	       echo Creating PR $prName from $run-$camCol-$field
	    }
	    
	    if [catch {
	       set_run -rerun $rerun $run $camCol $field
	       makePR $prName -overlap
	    } msg] {
	       echo "Failed to create PR: $msg"
	       continue
	    }
	 }
      }

      if ![file exists $data_root/prs/$prName] {
	 if {($frames || $analyze) && $verbose > 1} {
	    echo PR $prName wasn't created
	 }
	 continue
      }

      if $frames {
	 set runThisPR 1
	 if ![catch {glob $data_root/prs/$prName/$run/0/objcs/$camCol/fpObjc*}] {
	    if {$verbose > 1} {
	       echo PR $prName has already been processed
	    }
	    if !$force {
	       set runThisPR 0
	    }
	 }

	 if $runThisPR {
	    if $verbose {
	       echo do-frames -pr $prName -verbose $verbose
	    }

	    if [catch {
	       do-frames -pr $prName -verbose $verbose
	    } msg] {
	       echo "Failed to process $prName: $msg"
	       continue
	    }
	 }
      }

      if [catch {glob $data_root/prs/$prName/$run/0/objcs/$camCol/fpObjc*} msg] {
	 if {$analyze && $verbose > 1} {
	    echo PR $prName wasn't processed successfully: $msg
	 }
	 continue
      }
      
      if $analyze {
	 set b 2;		# r-band

	 if [catch {
	    # Old results
	    set_run -rerun $rerun $run $camCol $field
	    set vals [get_photo_results $b $rowc $colc]

	    set j -1
	    set opetroMag  [lindex $vals [incr j]]
	    set ocmodelMag [lindex $vals [incr j]]
	    set ocre       [lindex $vals [incr j]]
	    # New results
	    set_run -pr $prName
	    set vals [get_photo_results $b $rowc $colc]

	    set j -1
	    set petroMag  [lindex $vals [incr j]]
	    set cmodelMag [lindex $vals [incr j]]
	    set cre       [lindex $vals [incr j]]

	    if $printHeader {
	       puts $ofd "\#PR         run c field rowc   colc      mag_r    re_r  old: petroMag cmodelMag r_e*sqrt(ab) new: petroMag cmodelMag r_e*sqrt(ab) eyeflag flag good"
	       
	       set printHeader 0;	# we did.
	    }

	    puts $ofd [format "%-10s %-4d %d %4d %6.1f %6.1f  %8.3f %7.2f     %8.3f %8.3f %7.2f            %8.3f %8.3f %7.2f %d %d %d" \
			   $prName \
			   $run $camCol $field $rowc $colc $mag_r $re_r \
			   $opetroMag $ocmodelMag $ocre \
			   $petroMag $cmodelMag $cre $eyeflag $flag $good]
	    flush $ofd
	 } msg] {
	    echo "Analysis failed for $prName: $msg"
	 }
      }
   }

   close $ifd

   if {$ofd != "stdout"} {
      close $ofd
   }
}

proc get_photo_results {b rowc colc} {
   global table objcIo
   
   set id [find_objc table $rowc $colc !bright&(!BLENDED|NODEBLEND)]
	    
   foreach name [list petro model cmodel exp deV ap10 ap13] {
      set ${name}Mag [get_mag_from_counts $objcIo $b $name ""]
   }
   foreach name [list r_exp r_deV ab_exp ab_deV sky petroRad petroR50] {
      set $name [exprGet $objcIo.$name<$b>]
   }
   set fracDeV [exprGet $objcIo.fracPSF<$b>]
   
   set cre [expr ($r_deV*$fracDeV*$deVMag + $r_exp*(1 - $fracDeV)*$expMag)/$cmodelMag]
   set cab [expr ($ab_deV*$fracDeV*$deVMag + $ab_exp*(1 - $fracDeV)*$expMag)/$cmodelMag]
   set cre [expr 0.396*$cre*sqrt($cab)]

   return [list $petroMag $cmodelMag $cre]
}
