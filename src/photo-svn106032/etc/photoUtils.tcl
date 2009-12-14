# This file contains development tools, etc. that are NOT required to run
#  the pipelines.
  
proc just_copy {infile outfile {verbose 0}} {
   if $verbose {
      echo [format "%-40s --> %s" $infile $outfile]
   }
      if [catch { set in [open $infile] } msg] {
         error $msg
      }
      if [catch { set out [open $outfile w] } msg] {
         catch {close $in}
         error $msg
      }

      copyfile $in $out
      close $in; close $out
}

###############################################################################
#
# Run the PS pipeline, using a pretty minimalist psPlan file
#
proc do-psp {args} {
   global cmdlineParam cmdlinePlan data_root

   set compatibility 0;			# read old plan files
   set fakePSF 0
   set pr 0;				# run is a PR number
   set opts [list \
		 [list [info level 0] ""] \
		 [list {[run]} STRING 0 run "The desired run"] \
		 [list {[camCol]} INTEGER 0 camCol "Desired camera column"] \
		 [list {[startField]} STRING "first" startField \
		    "Desired first (or only) field to process, or \"first\""] \
		 [list {[endField]} STRING "" endField \
		      "Desired last field to process, or \"last\" (default: <startField>)"]\
		 [list -filterlist STRING "" filterlist \
		      "Filters to process (may be `all')"] \
		 [list -plan STRING "" \
		      plan "A list of key val pairs to add to cmdlinePlan"] \
		 [list -param STRING "" \
		      param "A list of key val pairs to add to cmdlineParam"] \
	         [list -psPlan STRING "" planfile \
		      "psPlan file to use (default psPlan-camCol.par or psPlan.par)"] \
		 [list -rerun INTEGER -1 rerun "The desired rerun"] \
		 [list -verbose INTEGER 0 verbose "Set frames' verbosity"] \
		 [list -pr CONSTANT 1 pr \
		      "Run the testcase for PR <run> \[camCol <camCol>\]"] \
	         [list -display STRING "none" display \
	       "Display some diagnostics (e.g. -disp raw|wing, or -disp {})
 (Repeat to increase value, e.g. raw|psp|psp)"] \
		 [list -compatibility CONSTANT 1 compatibility \
		      "Read old plan files"] \
		 [list -fakePSF CONSTANT 1 fakePSF \
		      "Ignore PSF determination"] \
		 [list -memTraceFile STRING "" memTraceFile \
		      "Write a memory trace to this file (1 => mem.trace)"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set root $data_root
   
   if [info exists cmdlinePlan] {
      unset cmdlinePlan
   }

   if {$startField != "first" && $startField != "start"} {
      array set cmdlinePlan [list startField $startField]
      if {$endField == ""} {
	 set endField $startField
      }
   }
   if {$endField != "" && $endField != "end" && $endField != "last"} {
      array set cmdlinePlan [list endField $endField]
   }

   if $pr {
      array set cmdlinePlan [list version default]
      if {$planfile == ""} {
	 set planfile $root/prs/$run/photo/psPlan
	 if {$camCol != 0} {
	    set planfile $planfile-$camCol.par
	 } else {
	    if [file exists $planfile.par] {
	       set planfile $planfile.par
	    } else {
	       loop c 1 7 {
		  if [file exists $planfile-$c.par] {
		     set planfile $planfile-$c.par
		     break;
		  }
	       }
	    }
	 }
	 #
	 # OK, look for run/rerun/photo/psPlan.par
	 #
	 set base $root/prs/$run
	 if ![catch { set dirs [glob $base/\[0-9\]*\[0-9\]] }] {
	    if {[llength $dirs] == 1} {
	       set dir [lindex $dirs 0]
	       if ![catch { set dirs [glob $dir/\[0-9\]*] }] {
		  if {[llength $dirs] == 1} {
		     set dir [lindex $dirs 0]
		     set planfile $dir/photo/psPlan.par
		  }
	       }
	    }
	 }

	 if ![file exists $planfile] {
	    error "I cannot find a $planfile"
	 }
      }

      set run 0
   }
   
   if {$memTraceFile != ""} {
      array set cmdlinePlan [list trace_memory $memTraceFile]
   }
   
   if $compatibility {
      array set cmdlinePlan [list \
				 check_planfile 1 \
				 version [photoVersion] \
				 baseDir $root \
				]
   }

   if $fakePSF {
      array set cmdlinePlan [list \
				 ignore_PSF          1 \
				 ignore_calibration  1 \
				 ignore_wing_stars   1 \
				 ignore_wing_stars   1 \
				 ignore_WC           1 \
				 ignore_plots        1 \
				 write_psFang        0 \
                                 fakePSF             1 \
				]
   }

   if {$display != "none" && $display != "off"} {
      global sao
      
      if {![info exists sao(default)]} {
	 set reg [regNew 1 1500]; regClear $reg
	 regPixSetWithDbl $reg 0 0 10; # ds9 doesn't like constant images
	 set sao(default) [saoDisplay $reg "-geometry +1+1 -histeq -lowerleft"]
	 regDel $reg
      }

      if {$display != ""} {
	 foreach what [split $display " |"] {
	    regsub {^display_} $what "" what

	    if ![info exists displays($what)] {
	       set displays($what) 0
	    }
	    incr displays($what)
	 }

	 array set cmdlinePlan [list \
				    display 1 \
				   ]
	 foreach what [array names displays] {
	    array set cmdlinePlan [list \
				       display_$what $displays($what) \
				      ]
	 }
      }
   }

   if {$filterlist == "all"} {
      set filterlist "u g r i z"
   } else {
      if {[llength $filterlist] == 1} {
	 set filterlist [split $filterlist {}]
      }
   }

   array set cmdlinePlan [list \
			      verbose $verbose \
			     ]
   if {$camCol > 0} {
      array set cmdlinePlan [list camCol $camCol]
   }
   if {$filterlist != ""} {
      array set cmdlinePlan [list filterlist $filterlist]
   }
   if {$rerun >= 0} {
      array set cmdlinePlan [list rerun $rerun]
   }
   if {$plan != ""} {
      array set cmdlinePlan $plan
   }

   if [info exists cmdlineParam] {
      unset cmdlineParam
   }
   if {$param != ""} {
      array set foo $param;		# separate keys/values
      foreach el [array names foo] {
	 keylset cmdlineParam $el $foo($el)
      }
   }

   if {$planfile == ""} {
      set dir "$root/$run/$rerun"
      
      if {$rerun == -1} {
	 if ![catch { set dirs [glob $root/$run/\[0-9\]*\[0-9\]] }] {
	    set dir [lindex $dirs 0]
	 }
      }

      set planfile $dir/photo/psPlan
      if {![file exists $planfile.par] &&
	  [file exists $planfile-$camCol.par]} {
	 append planfile "-$camCol"
      }
      append planfile ".par"
   } else {
      if ![regexp {^/} $planfile] {
	 set planfile $root/$run/$rerun/photo/$planfile
      }
   }

   if ![file exists $planfile] {
      error "I cannot find $planfile"
   }

   uplevel #0 [list \
       run_ps_pipeline $planfile \
      ]
}

###############################################################################
#
# Run the frames pipeline, using a pretty minimalist fpPlan file
#
proc do-frames {args} {
   global cmdlineParam cmdlinePlan data_root sao

   set compatibility 0;			# read old plan files
   set make_fpC 0;			# make fpC files from idR
   set make_fpM 0;			# write fpM as well as fpC files
   set make_fpFieldStat 0;		# write fpFieldStat as well as fpC files
   set write_fpAtlas_files 1;		# write fpAtlas files
   set pr 0;				# run is a PR number
   set read_fpC 0;			# read fpC/fpM not idR files
   set reconstruct_fpC 0;		# make fpC files from fpAtlas/fpBIN
   set write_test_info 0
   
   set opts [list \
		 [list [info level 0] ""] \
		 [list {[run]} STRING 0 run "The desired run"] \
		 [list {[camCol]} INTEGER 0 camCol "Desired camera column"] \
		 [list {[startField]} STRING "first" startField \
		    "Desired first (or only) field to process, or \"first\""] \
		 [list {[endField]} STRING "" endField \
		      "Desired last field to process, or \"last\" (default: <startField>)"]\
	         [list -filterlist STRING "" filterlist \
		      "Filters to process (may be `all')"] \
		 [list -plan STRING "" \
		      plan "A list of key val pairs to add to cmdlinePlan"] \
		 [list -param STRING "" \
		      param "A list of key val pairs to add to cmdlineParam"] \
	         [list -fpPlan STRING "" planfile \
		      "fpPlan file to use (default fpPlan.par or fpPlan-camCol.par)"] \
	         [list -display STRING "none" display_list \
		      "Display some diagnostics (e.g. -disp all|smooth|binned, or -disp all)
 (Repeat to increase value, e.g. u|g|r|objs|objs).  -disp nowait is also supported"] \
		 [list -rerun INTEGER -1 rerun "The desired rerun"] \
		 [list -verbose INTEGER 0 verbose "Set frames' verbosity"] \
		 [list -test_info CONSTANT 1 write_test_info \
		      "Write the .TST file"] \
		 [list -pr CONSTANT 1 pr \
		      "Run the testcase for PR <run> \[camCol <camCol>\]"] \
		 [list -read_fpC CONSTANT 1 read_fpC \
		      "Read fpC/M, not idR, files"] \
		 [list -fpC CONSTANT 1 make_fpC \
		      "Just make fpC files (written to fpCDir; consider e.g.
 -plan {fpCDir .}"] \
		 [list -fpM CONSTANT 1 make_fpM \
		      "Make fpM as well as fpC files"] \
		 [list -fpFieldStat CONSTANT 1 make_fpFieldStat \
		      "make fpFieldStat as well as fpC files"] \
		 [list -noFpAtlas CONSTANT 0 write_fpAtlas_files \
		      "Don't make fpAtlas files files"] \
		 [list -reconstruct_fpC CONSTANT 1 reconstruct_fpC \
		      "Just make fpC files from atlas images/fpBIN files"] \
		 [list -compatibility CONSTANT 1 compatibility \
		      "Read old plan files"] \
		 [list -memTraceFile STRING "" memTraceFile \
		      "Write a memory trace to this file (1 => mem.trace)"] \
		 [list -select STRING "" select \
		      "Selection function for objects to include"] \
		 [list -subtract STRING "" subtract \
		      "Selection function for objects to _subtract_ from fpC"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set root $data_root

   if $reconstruct_fpC {
      set make_fpC 1
   }

   if [info exists cmdlinePlan] {
      unset cmdlinePlan
   }

   if {$startField != "first" && $startField != "start"} {
      array set cmdlinePlan [list startField $startField]
      if {$endField == ""} {
	 set endField {$startField}
      }
   }
   if {$endField != "" && $endField != "end" && $endField != "last"} {
      array set cmdlinePlan [list endField $endField]
   }

   if $read_fpC {
      array set cmdlinePlan [list read_fpC_files 1  write_fpC_files 0]
   }
   if !$write_fpAtlas_files {
      array set cmdlinePlan [list write_fpAtlas_files 0]
   }
   
   if $pr {
      array set cmdlinePlan [list version default]
      if {$planfile == ""} {
	 set planfile $root/prs/$run/photo/fpPlan
	 if {$camCol != 0} {
	    set planfile $planfile-$camCol.par
	 } else {
	    if [file exists $planfile.par] {
	       set planfile $planfile.par
	    } else {
	       loop c 1 7 {
		  if [file exists $planfile-$c.par] {
		     set planfile $planfile-$c.par
		     break;
		  }
	       }
	    }
	 }
	 #
	 # OK, look for run/rerun/photo/fpPlan.par
	 #
	 set base $root/prs/$run
	 if ![catch { set dirs [glob $base/\[0-9\]*\[0-9\]] }] {
	    if {[llength $dirs] == 1} {
	       set dir [lindex $dirs 0]

	       if {$rerun != -1} {
		  append dir /$rerun
		  set planfile $dir/photo/fpPlan.par
	       } elseif ![catch { set dirs [glob $dir/\[0-9\]*] }] {
		  set dir [lindex $dirs end]
		  set planfile $dir/photo/fpPlan.par
	       }
	    }
	 }

	 if ![file exists $planfile] {
	    error "I cannot find a $planfile"
	 }
      }

      set run 0
   }

   if {$filterlist == "all"} {
      set filterlist "u g r i z"
   } else {
      if {[llength $filterlist] == 1} {
	 set filterlist [split $filterlist {}]
      }
   }

   set display_filterlist ""
   if {$display_list == "none"} {
      set display 0
   } else {
      set display 1
      
      foreach what [split $display_list " |"] {
	 regsub {^display_} $what "" what

	 if {$what == "wait"} {
	    continue;
	 } elseif {$what == "nowait"} {
	    set display -10
	    continue
	 }
	 
	 if {$what == "all" || [string length $what] == 1} {
	    lappend display_filterlist $what
	 } else {
	    if ![info exists displays($what)] {
	       set displays($what) 0
	    }
	    incr displays($what)
	 }
      }

      foreach what [array names displays] {
	 array set cmdlinePlan [list \
				    display_$what $displays($what) \
				   ]
      }
      
      if {$display_filterlist == "all" || $display_filterlist == ""} {
	 if {$filterlist != ""} {
	    set display_filterlist $filterlist
	 } else {
	    set display_filterlist "u g r i z"
	 }
      }

      foreach f $display_filterlist {
	 set_mtv $f
	 if {$filterlist == "all" || $filterlist == ""} {
	    set _filterlist "u g r i z"
	 } else {
	    set _filterlist [split $filterlist]
	 }
	    
	 set sao([lsearch $_filterlist $f]) $sao($f);	# support integer indexes too 
      }
   }

   array set cmdlinePlan [list \
			      verbose $verbose \
			      write_test_info $write_test_info \
			      display $display \
			      display_filterlist $display_filterlist \
			     ]
   if {$camCol > 0} {
      array set cmdlinePlan [list camCol $camCol]
   }
   if {$filterlist != ""} {
      array set cmdlinePlan [list filterlist $filterlist]
   }

   if {$plan != ""} {
      array set cmdlinePlan $plan
   }

   if [info exists cmdlineParam] {
      unset cmdlineParam
   }
   if {$param != ""} {
      array set foo $param;		# separate keys/values
      foreach el [array names foo] {
	 keylset cmdlineParam $el $foo($el)
      }
   }

   if {$rerun >= 0} {
      array set cmdlinePlan [list rerun $rerun]
   }

   if {$memTraceFile != ""} {
      array set cmdlinePlan [list trace_memory $memTraceFile]
   }
   
   if $compatibility {
      array set cmdlinePlan [list \
				 check_planfile 1 \
				 version [photoVersion] \
				 baseDir $root \
				]
   }

   if {$planfile == ""} {
      set dir "$root/$run"
      if {$rerun != -1} {
	 append dir "/$rerun"
      } else {
	 if ![catch { set dirs [glob $root/$run/{\[0-9\],\[0-9\]*\[0-9\]}] }] {
	    set dir [lindex $dirs 0]
	 }
      }

      set planfile $dir/photo/fpPlan
      if {![file exists $planfile.par] &&
	  [file exists $planfile-$camCol.par]} {
	 append planfile "-$camCol"
      }
      append planfile ".par"
   } else {
      if ![regexp {^/} $planfile] {
	 set planfile $root/$run
	 if {$rerun != -1} {
	    append planfile /$rerun
	 }
	    
	 #append planfile /photo/$planfile
	  append planfile /photo/fpPlan.par
      }
   }

   if ![file exists $planfile] {
      error "I cannot find $planfile"
   }

   if $make_fpC {
      if {[array names cmdlinePlan fpCDir] == ""} {
	 array set cmdlinePlan [list fpCDir \$baseDir/\$run/\$rerun/corr/\$camCol]
      }

      if $reconstruct_fpC {
	 if {$make_fpM || $make_fpFieldStat} {
	    error "You mayn't specify -fpM and -reconstruct_fpC"
	 }
	 uplevel \#0 [list \
	     make_corrected_frames -reconstruct -bkgd "binnedSky" $planfile \
	     -select $select \
	    ]
      } else {
	 if {$select != ""} {
	    error "-select only makes sense with -fpC -reconstruct_fpC"
	 }
	 array set cmdlinePlan "write_fpC_files 1"
	 set cmd [list make_corrected_frames -subtract $subtract]
	 if $make_fpM {
	    lappend cmd [list -fpM]
	 }
	 if $make_fpFieldStat {
	    lappend cmd [list -fpFieldStat]
	 }
	 lappend cmd [list $planfile]
	 uplevel \#0 $cmd
      }
   } else {
      if {$make_fpM || $make_fpFieldStat} {
	 error "You may only specify -fpM if you also choose -fpC"
      }
      if {$select != ""} {
	 error "-select only makes sense with -fpC"
      }
      uplevel \#0 [list \
	  run_frames_pipeline $planfile \
	 ]
   }
}

###############################################################################

proc make_wallpaper {args} {
   global env

   if 1 {				# RHL
      global dsky
   }
   
   set constant_sky 0;			# should sky level be a constant?
   set display 0;			# display final file?
   set no_ppm 0;			# Don't make the PPM files if the exist
   set make_fpC 0;			# make fpC files as needed?
   set make_ppm_file 0;			# Produce the PPM file?
   set make_png_file 0;			# Convert PPM file to PNG?
   set make_tiff_file 0;		# Convert PPM file to TIFF?
   set monochromatic 0;			# Make monochromatic? image 
   set verbose 0

   set firstSky 0;			# determine sky from first field
   set lastSky 0;			# determine sky from last field
   					# N.b. If both are set, use mean
   
   set opts [list \
		 [list [info level 0] "\
 Do all the work to make a mosaic from a specified pair of runs, possible
 including actually running e.g.
        assemblePPM wallpaper/mosaic.in wallpaper/mosaic.ppm

 E.g. make_wallpaper wallpaper/mosaic 1894 3:5 48:66  2570 3:4 201:219 -type asinh:Q8 -stretch m:50

 You can specify an ra/dec and nrow,ncol
      make_wallpaper Perseus 49.8540 41.5102 5x5 -v
 It's more efficient to include the runs:
      make_wallpaper Perseus 3610 49.8540,41.5102 5x5 3628
 "] \
		 [list <name> STRING "" basename \
		      "Input files are named <name>-run-camCol-frame.ppm"] \
		 [list <run1> STRING 0 run1 "First run (or ra)"] \
		 [list <camCol1> STRING "" camCol1 \
		      "Camera columns in run1 in form 1\[:6\] or dec or ra,dec"] \
		 [list <field1> STRING "" field1 \
		      "Camera columns in run1 in form 11\[:106\] or nfield,ncamCol"] \
		 [list {[run2]} STRING "" run2 "Second run"] \
		 [list -outfile STRING "" outfile \
		      "Name of generated input for assemblePPM.
 If you specify -ppm, the output filename will be derived from <outfile>
 by replacing its suffix with \"ppm\""] \
		 [list {[camCol2]} STRING "" camCol2 \
		      "Camera columns in run2 in form 1\[:6\]"] \
		 [list {[field2]} STRING "" field2 \
		      "Camera columns in run2 in form 11\[:106\]"] \
		 [list -rerun STRING {} rerun "Desired rerun"] \
		 [list -rerun1 STRING {} rerun1 "Desired rerun for run one"] \
		 [list -rerun2 STRING {} rerun2 "Desired rerun for run two"] \
		 [list -fpC CONSTANT 1 make_fpC "Make fpC files as needed"] \
		 [list -make_assemblePPM_args STRING "" make_assemblePPM_args \
		      "Extra arguments to make_assemblePPM"] \
		 [list -set_run_args STRING "" set_run_args \
		      "Extra arguments to set_run"] \
		 [list -lut_args STRING "" \
		      lut_args "Arguments to make_ppm_file to define LUT (see also -type and -stretch)"] \
		 [list -stretch STRING "m:50" \
		      lut_stretch "Stretch used to build LUT"] \
		 [list -type STRING "asinh:Q10" \
		      lut_type "Type of stretch used to build LUT"] \
		 [list -bin INTEGER 1 binfac \
		      "How much to bin input files"] \
		 [list -make_ppm_file_args STRING "-fix_with_U16 -use_fpM" \
		      make_ppm_file_args "Extra arguments to make_ppm_file"] \
		 [list -scalebar DOUBLE 0.0 scalebar \
		      "Draw a scalebar this many arcsec long"] \
		 [list -display CONSTANT 1 display "Display file?"] \
		 [list -constantSky CONSTANT 1 constant_sky \
		      "Take the sky from the first field in each camCol"]\
		 [list -bands STRING "i r g" bands \
		      "bands to use; RGB order"] \
		 [list -monochromatic CONSTANT 1 monochromatic "make a monochromatic image"] \
		 [list -nsigma DOUBLE 0.0 nsigma \
		      "Increase sky estimate by this many sigma"] \
		 [list -sky STRING "" _sky \
		      "A a single sky value or a the name of an array specifying the desired sky levels in each filter"] \
		 [list -firstSky CONSTANT 1 firstSky \
		      "Use constant sky per camCol (from first field)"] \
		 [list -lastSky CONSTANT 1 lastSky \
		      "Use constant sky per camCol (from last field)"] \
		 [list -skyFrameOffset INTEGER 0 skyFrameOffset \
		      "If -firstSky and/or -lastSky, offset reference fields this
 much from start/end of run"] \
		 [list -noPPM CONSTANT 1 no_ppm "Don't make the ppm files"] \
		 [list -nrow INTEGER 1489 nrow "Row-size of input U16 files"] \
		 [list -ncol INTEGER 2048 ncol "Col-size of input U16 files"] \
		 [list -show STRING "{}" sel_show \
		      "Show objects for which this proc returns true (see select)"] \
		 [list -ppm CONSTANT 1 make_ppm_file "Produce the PPM file?"]\
		 [list -png CONSTANT 1 make_png_file \
		      "Convert PPM file to PNG?"]\
		 [list -tiff CONSTANT 1 make_tiff_file \
		      "Convert PPM file to TIFF?"]\
		 [list -verbose CONSTANT 1 verbose "Be chatty?"]\
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$rerun != ""} {
      if {$rerun1 != "" && $rerun1 != $rerun} {
	 error "-rerun and -rerun1 must agree"
      }
      set rerun1 $rerun
   }

   if {$_sky != ""} {
      if [regexp {^[0-9]} $_sky] {
	 set sky_value $_sky
      } else {
	 upvar $_sky sky
      }
   }

   if $verbose {
      lappend make_ppm_file_args "-verbose"
   }
   if $make_fpC {
      lappend make_ppm_file_args "-fpC"
   }
   if {$scalebar > 0} {
      lappend make_ppm_file_args "-scalebar"
      lappend make_ppm_file_args $scalebar
   }
   if {$sel_show != "{}"} {
      lappend make_ppm_file_args "-show"
      lappend make_ppm_file_args $sel_show
   }
   #
   # Parse e.g. camCol1, giving camCol11 and camCol12
   #
   # First check if camCol1 is of the form ra,dec, in which case field1
   # specifies the number of camCols wide and fields long the mosaic
   # should be
   #
   if {[regexp {^[0-9]+\.[0-9]+} $run1] &&
       [regexp {^[0-9]+\.[0-9]+} $camCol1]} {# "ra dec" not "run ..."
      set camCol1 "$run1,$camCol1"
      set run1 0
   }

   if [regexp {^([0-9]+\.[0-9]+),([+-]?[0-9]+\.[0-9]+)$} $camCol1 \
	   {} ra dec] {
      if {$run1 > 0} {
	 set findit [where_in_run $run1 $ra $dec]
	 if {$findit == -1} {
	    error "I cannot find ra,dec = $camCol1 in run $run1"
	 }
      } else {
	 set findit [where_in_SDSS -quiet $ra $dec]
	 set findit [lindex $findit end]
	 if {$findit == ""} {
	    error "I don't think that the SDSS has imaged ra,dec = $camCol1"
	 }
      }
      set camCol [lindex $findit 1]
      set field [lindex $findit 2]
      set rowc [lindex $findit 3]
      set colc [lindex $findit 4]

      if {$field1 == ""} {
	 set field1 "1,1"
      }
      if ![regexp {^([0-9]+)[x,]([0-9]+)$} $field1 {} nfield ncamCol] {
	 error "Please specify <nfield>x<ncol>"
      }

      set left 0
      if {$ncamCol != 1} {		# find the matching run
	 global fieldparams table

	 set_run $run1 $camCol $field
	 
	 if {$colc <= 1024} {
	    set left 1;			# look for camCol to left
	    set colc [expr 2048/2 - 2048]
	 } else {
	    set left 0;			# look for camCol to right
	    set colc [expr 2048/2 + 2048]
	 }
	 set band 2; set filter r
	 set gc [transApply *$fieldparams.frame<$band>.toGCC $filter \
		     [expr 1361/2] 0 $colc 0]
   
	 set eq [GCToEq [keylget gc mu] [keylget gc nu] \
		     -node [keylget table node] -incl [keylget table incl]]
	 set ra [keylget eq ra]; set dec [keylget eq dec]

	 if {$run2 != ""} {
	    set findit [where_in_run $run2 $ra $dec]
	 } else {
	    set findit [where_in_SDSS -quiet $ra $dec]
	    set findit [lindex $findit end]
	 }

	 if {$findit == -1 || $findit == ""} {
	    error "I cannot find ra,dec = $ra, $dec"
	 }
	 set run2 [lindex $findit 0]
	 set camCol2 [lindex $findit 1]
	 set field2 [lindex $findit 2]
      }

      set ans [choose_mosaic_columns $camCol $camCol2 $ncamCol $left]

      set i -1
      set camCol11 [lindex $ans [incr i]]
      set camCol12 [lindex $ans [incr i]]
      set camCol21 [lindex $ans [incr i]]
      set camCol22 [lindex $ans [incr i]]

      set camCol1 $camCol11:$camCol12
      set field11 [expr $field - $nfield/2]
      set field12 [expr $field11 + $nfield - 1]

      if {$camCol21 < 0} {
	 set run2 ""; set camCol2 ""; set field2 ""
	 set field21 0; set field22 0;	# only set for incr's sake
      } else {
	 set camCol2 $camCol21:$camCol22

	 set field21 [expr $field2 - $nfield/2]
	 set field22 [expr $field21 + $nfield - 1]

	 set field2 [expr $field21 - 1]:[expr $field22 + 1];# +-1 to overlap run1
      }

      if {$nfield%2 == 0} {
	 if {$rowc > 1489/2} {
	    incr field11  1; incr field21  1
	    incr field12  1; incr field22  1
	 } else {
	    #incr field11 -1; incr field21 -1
	    #incr field12 -1; incr field22 -1
	 }
      }

      set field1 $field11:$field12

      if $verbose {
	 echo \
	     "Run camCol field: $run1 $camCol1 $field1  $run2 $camCol2 $field2"
      }
   }
   #
   # OK, we've processed any ra/decs; parse the requested runs
   #
   set runs $run1
   set ${run1}_rerun \{$rerun1\}
   if {$run2 == ""} {
      if {$camCol2 != "" || $field2 != ""} {
	 error "You cannot specify camCol/field without run2"
      }
   } else {
      lappend runs $run2
      set ${run2}_rerun \{$rerun2\}
   }

   foreach r "1 2" {
      foreach el "camCol field" {
	 if {[set ${el}$r] != ""} {
	    if [regexp {^([0-9]+)$} [set ${el}$r] {} [set run$r]_${el}_1] {
	       set [set run$r]_${el}_2 [set [set run$r]_${el}_1]
	    } elseif {![regexp {^([0-9]+):([0-9]+)$} [set ${el}$r] {} \
			    [set run$r]_${el}_1 [set run$r]_${el}_2]} {
	       error "${el}$r ([set ${el}$r]) is not of form nnn:nnn"
	    }
	 }
      }
   }
   #
   # Be nice to people who write e.g. "gri"
   #
   if [regexp {^([ugriz]) *([ugriz]) *([ugriz])$} $bands {} r g b] {
      regsub -all {[ 	]} $bands "" bnds
      if [regexp {^(ugr|ugi|ugz|gri|grz|riz|uuu|ggg|rrr|iii|zzz)$} $bnds] {
	 set tmp $b; set b $r; set r $tmp
	 set bands "${r} ${g} ${b}"
      }
   } elseif {[regexp {^[ugriz]$} $bands band]} {
      set bands "$band $band $band"
      set monochromatic 1
   }

   if $monochromatic {
      lappend make_ppm_file_args "-monochromatic"
   }
   #
   # Do the work
   #
   set args "-use_calib [expr 0x1 | 0x8] -sky sky -dsky dsky"
   #set args "-use_calib [expr 0x1 | 0x8] -dsky dsky"
   foreach run $runs {
      loop c [set ${run}_camCol_1] [expr [set ${run}_camCol_2] + 1] {
	 eval set_run $set_run_args $run $c -rerun "[set ${run}_rerun]"
	 
	 set f1 [set ${run}_field_1];  set f2 [set ${run}_field_2]; 
	 #
	 # Get sky level for this run/camCol (if not set in $sky())
	 #
	 global fieldparams table

	 set table [openit -noAI $f1]
	 set filterlist [keylget table filters]
	 objfileClose table

	 if {[info exists sky([lindex $bands 0])] || [info exists sky_value]} {
	    foreach f $bands {
	       if [info exists sky($f)] {
		  set sky1($f) $sky($f)
		  set sky2($f) $sky($f)
	       } else {
		  if [info exists sky_value] {
		     set sky($f) $sky_value
		     set sky1($f) $sky_value
		     set sky2($f) $sky_value
		  } else {
		     error \
		    "No sky level for filter $f is specified in sky array $_sky"
		  }
	       }
	    }
	 } elseif {$firstSky || $lastSky} {
	    set skyf1  [expr $f1 - $skyFrameOffset]
	    set skyf2  [expr $f2 + $skyFrameOffset]
	    
	    if {!$firstSky} {
	       set skyf1 $skyf2
	    }
	    if {!$lastSky} {
	       set skyf2 $skyf1
	    }

	    foreach f "1 2" {
	       set table [openit -noAI [set skyf$f]]
	       loop i 0 [llength $filterlist] {
		  set filter [lindex $filterlist $i]
		  
		  if {[lsearch $bands $filter] < 0} {
		     continue;
		  }
		  
		  set sky${f}($filter) \
		      [binregionInterpolate *$fieldparams.frame<$i>.sky 0 0]
	       }
	       objfileClose table
	    }
	 } else {
	    set vfield [vectorExprEval "($f1,$f2)"]
	    
	    loop i 0 [llength $filterlist] {
	       set filter [lindex $filterlist $i]

	       if {[lsearch $bands $filter] < 0} {
		  continue;
	       }

	       set vsky($filter) [vectorExprNew [expr $f2 - $f1 + 1]]
	       
	       loop f $f1 [expr $f2 + 1] {
		  if [catch {
		     set table [openit -noAI $f]
		  }] {
		     handleSet $vsky($filter).vec<[expr $f-$f1]> \
			 [vectorExprGet $vsky($filter)<[expr $f-$f1-1]>]
		     continue
		  }

		  
		  handleSet $vsky($filter).vec<[expr $f-$f1]> \
		      [binregionInterpolate *$fieldparams.frame<$i>.sky 0 0]
		  
		  objfileClose table;
	       }
	       #
	       # Fit straight line
	       #
	       if {$f2 == $f1} {
		  set a [vectorExprGet $vsky($filter)]
		  set b 0
	       } else {
		  lsq $vfield $vsky($filter) a b
	       }
	       
	       set sky1($filter) [expr $a + $b*$f1]
	       set sky2($filter) [expr $a + $b*$f2]

	       if 0 {
		  puts -nonewline \
		      [format "%s %s %s ($f1, $f2) %6.2f %6.2f  %6.2f %6.2f  "\
			   $run $c $filter [expr $a + $b*$f1] $b \
			   $sky1($filter) $sky2($filter)]
		  loop f $f1 [expr $f2 + 1] {
		     puts -nonewline \
			 [format " %5.1f" \
			      [vectorExprGet $vsky($filter)<[expr $f-$f1]>]]
		  }
		  echo ""
	       }
	       
	       vectorExprDel $vsky($filter)
	    }
	    
	    vectorExprDel $vfield
	 }
	 #
	 # Create ppm files
	 #
	 if 1 {
	    foreach f $bands {
	       if {$constant_sky || $f1 == $f2} {
		  set skygrad($f) 0
	       } else {
		  set skygrad($f) \
		      [expr ($sky2($f) - $sky1($f))/($f2 - $f1)]
	       }
	    }
	    #
	    # Create sky gradient regions, if needed.  These are
	    # used to flatten the sky in the scan (==time) direction,
	    # in addition to the per-frame mean values in the sky array
	    #
	    set shift 5;		# how many bits to shift dsky
	    
	    foreach f $bands {
	       if {$skygrad($f) == 0} {
		  if [info exists dsky($f)] {
		     unset dsky($f)
		  }
	       } else {
		  set dsky($f) [regGradSet $skygrad($f) $nrow $ncol $shift]
	       }
	    }
	    #
	    # Actually generate ppm files
	    #
	    if 1 {
	       set df 1;		# every field; normal case
	    } else {
	       set df [expr $f2 - $f1];# debugging
	       if {$df == 0} { incr df }
	       echo Setting df to $df
	    }

	    if {[info commands prepare_regions_callback] != ""} {
	       rename prepare_regions_callback {}
	    }				
	    #
	    # Set a prepare_regions_callback if so desired. An example follows:
	    #
	    if 0 {
	       proc set_prepare_regions_callback {run camCol} {
		  if {$run == 259 && $camCol == 3} {
		     proc prepare_regions_callback {frame f} {
			if {$f == "i"} {
			   uplevel {
			      set fac 1.15
			      echo Fiddling Flux20 by $fac for $f field $frame
			      set flux20s($f) [expr $fac*$flux20s($f)]
			   }
			}
		     }
		  }
	       }
	    }
	    # Call the callback
	    if {[info commands set_prepare_regions_callback] != ""} {
	       set_prepare_regions_callback $run $c
	    }

	    append lut_args " -type $lut_type"
	    append lut_args " -stretch $lut_stretch"

	    loop field $f1 [expr $f2 + 1] $df {
	       foreach f $bands {
		  set sky($f) [expr $sky1($f) + ($field - $f1)*$skygrad($f)]
	       }

	       if 0 {
		  set line "$run $c $field  "
		  foreach f $bands {
		     append line [format " %.1f" $sky($f)]
		  }
		  echo $line
	       }
	       
	       if [regexp {^(-LUT +)?(h[0-9]+)$} $lut_args {} {} lut] {
		  if $verbose {
		     echo "Using $lut as colour map lookup table"
		  }
		  set lut_args ""
	       }
	       if [regexp -- {^(.*)-getLUT +([^ ]+)(.*)$} \
		       $lut_args {} pre lutvar post] {
		  set lut_args "$pre $post"
		  upvar $lutvar lut
	       }

	       if ![info exists lut] {
		  if $verbose {
		     puts -nonewline "LUT: "
		  }

		  eval make_ppm_file $basename-$run-$c $field $field $bands \
		      $args $lut_args -getLUT lut $make_ppm_file_args -bin $binfac
	       }
	       #
	       # Does the file already exist?
	       #
	       set ppmfile "$basename-$run-$c-$field.ppm"
	       if {$no_ppm && [file exists $ppmfile]} {
		  if $verbose {
		     echo $ppmfile already exists
		  }
		  continue;
	       }
	       #
	       # Fiddle sky level to keep us above the noise
	       #
	       if {$nsigma != 0} {
		  foreach f $bands {
		     #puts -nonewline "RHL $f $sky($f) "
		     set gain 3;		# good enough for this fiddle
		     set sky($f) [expr $sky($f) + $nsigma*sqrt($sky($f)/$gain)]
		     #puts -nonewline " --> $sky($f)\n"
		  }
	       }
	       
	       eval make_ppm_file $basename-$run-$c $field $field $bands \
		   $args -LUT $lut $make_ppm_file_args -bin $binfac

	       if {$display && !$make_ppm_file} {
		  display_ppm_file -geom +1+1 $basename-$run-$c-$field.ppm
	       }
	    }
	 } else {
	    array set sky [array get sky2]
	    if [info exists lut] {
	       eval make_ppm_file $basename-$run-$c $f1 $f2 $bands \
		   $args -LUT $lut $make_ppm_file_args
	    } else {
	       eval make_ppm_file $basename-$run-$c $f1 $f2 $bands \
		   $args $lut_args -getLUT lut $make_ppm_file_args
	    }
	 }

	 if {[set k [lsearch $make_ppm_file_args "-scalebar"]] >= 0} {
	    set make_ppm_file_args \
		[lreplace $make_ppm_file_args $k [expr $k+1]];	# only draw one scalebar
	 }
	 
	 foreach f $bands {
	    if [info exists dsky($f)] {
	       binregionDel $dsky($f); unset dsky($f)
	    }
	 }
      }
      unset sky;	# values are only correct for first run
   }

   if {[info commands prepare_regions_callback] != ""} {
      rename prepare_regions_callback {}
   }				

   if [info exists lut] { regDel $lut }
   #
   # Actually call the code to make the assemble.in file
   #
   if {$outfile == ""} {
      set outfile "$basename.in"
   }

   foreach v [list set_run_args lut_args nsigma make_ppm_file_args] {
      lappend hdr [format "%-20s \"%s\"" $v [set $v]]
   }

   set binning "-bin $binfac"
   set row0 40; set col0 40
   if {$binfac != 1} {
      foreach v "nrow ncol row0 col0" {
	 set $v [expr [set $v]/$binfac]
      }
      lappend hdr "Binned ${binfac}x${binfac}"
   }

   set nrow [expr $nrow - 2*$row0]; set ncol [expr $ncol - 2*$col0]

   eval make_assemblePPM $basename $make_assemblePPM_args \
       -rerun1 \"$rerun1\" $run1 $camCol1 $field1 \
       -rerun2 \"$rerun2\" $run2 $camCol2 $field2 \
       -out \"$outfile\" -set_run_args \"$set_run_args\" \
       -hdr {[join $hdr "\n"]} $binning \
       -row0 $row0 -col0 $col0 -nrow $nrow -ncol $ncol
   #
   # Do the assembly?  This execs standalone utilities, and the ability
   # to do it here is just a courtesy
   #
   if $make_ppm_file {
      regsub {\..*$} $outfile ".ppm" ppm_file
      if $verbose {
	 echo Assembling $ppm_file
      }
      exec $env(PHOTO_DIR)/src/assemblePPM $outfile $ppm_file
      
      if {$make_png_file + $make_tiff_file > 0} {
	 if {$make_png_file + $make_tiff_file > 1} {
	    echo "Please choose exactly one extra output format"
	 }

	 if $make_png_file {
	    set fmt "png"
	 } elsif $make_tiff_file {
	    set fmt "tiff"
	 } else {
	    echo "Impossible error converting file type; complain to RHL"
	    set fmt ""
	 }
      
	 if {$fmt != ""} {
	    if $verbose {
	       echo Converting $ppm_file to $fmt
	    }
	    exec mogrify -format $fmt $ppm_file
	 }
      }

      if $display {
	 display_ppm_file -geom +1+1 $ppm_file
      }

      return $ppm_file
   }
}

###############################################################################
#
# Given an assemblePPM.in file, return the input file corresponding to a
# pixel in the final mosaic
#
proc where_in_mosaic {args} {
   set display 0;
   set swap_row_col 0

   set opts [list \
		 [list [info level 0] \
 "Given an assemblePPM.in file, return the input file corresponding to a
 pixel in the final mosaic.  If you know the origin of the displayed portion
 of the image, you probably want to set \[rc\]off to half the window size.
 "] \
		 [list <infile> STRING "" infile "Input file to assemblePPM"] \
		 [list <dbl> DOUBLE 0.0 rc "Desired row"] \
		 [list <dbl> DOUBLE 0.0 cc "Desired column"] \
		 [list -swap CONSTANT 1 swap_row_col \
		      "Assume inputs are col row not row col"] \
		 [list -bin INTEGER 1 binfac "How much mosaic is binned"] \
		 [list -roff INTEGER 0 roff \
		      "Number of rows to offset in displayed image"] \
		 [list -coff INTEGER 0 coff \
		      "Number of columns to offset in displayed image"] \
		 [list -display CONSTANT 1 display "Display file?"] \
	 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if $swap_row_col {
      set tmp $rc; set rc $cc; set cc $tmp
   }

   set rc [expr $binfac*($rc + $roff)]
   set cc [expr $binfac*($cc + $coff)]

   set fd [open $infile "r"]

   while {[gets $fd line] >= 0} {
      regsub {^ *(\#.*)?$} $line {} line
      if {$line == ""} {
	 continue;
      }

      set file [lindex $line 0]

      if {$file == "mosaic"} {
	 continue
      }

      set r0 [lindex $line 1]
      set c0 [lindex $line 2]
      set rsize [lindex $line 3]
      set csize [lindex $line 4]

      if {$rc > $r0 && $rc <= $r0 + $rsize &&
	  $cc > $c0 && $cc <= $c0 + $csize} {
	 close $fd

	 if $display {
	    display_ppm_file -geom +1+1 $file
	 }

	 return $file
      }
   }

   close $fd

   return ""
}

###############################################################################
#
# Solve the surprisingly difficult task of choosing which camera columns
# are needed in a mosaic
#
proc choose_mosaic_columns {camCol1 camCol2 ncamCol {left 0}} {
   #echo RHL choose_mosaic_columns $camCol1 $camCol2 $ncamCol $left
   
   if {$ncamCol <= 1} {
      return "$camCol1 $camCol1 -1 -1"
   }

   set middle [expr 2*$camCol1];	# Work in 2*camCol to handle 2 strips
   set first [expr $middle - ($ncamCol + $left - 1)/2]
   set last [expr $first + $ncamCol - 1]

   set camCols1 ""; set camCols2 ""
   loop c $first [expr $last + 1] {
      if {$c%2 == 0} {
	 lappend camCols1 [expr $c/2]
      } else {
	 lappend camCols2 [expr $c/2]
      }
   }

   set camCol11 [lindex $camCols1 0]
   set camCol12 [lindex $camCols1 end]
   set camCol21 [lindex $camCols2 0]
   set camCol22 [lindex $camCols2 end]

   if {!$left} {
      incr camCol21 [expr $camCol2 - $camCol1]
      incr camCol22 [expr $camCol2 - $camCol1]
   }

   foreach t "11 12 21 22" {
      if {[set camCol$t] < 1} {
	 set camCol$t 1
      } elseif {[set camCol$t] > 6} {
	 set camCol$t 6
      }
   }

   return "$camCol11 $camCol12 $camCol21 $camCol22"
}

###############################################################################
#
# Fit a straight line y = ax + b
#
proc lsq {x y _a _b} {
   upvar $_a a  $_b b

   set mx [vectorExprGet sum($x)/dimen($x)]
   set my [vectorExprGet sum($y)/dimen($y)]
   set Sxx [vectorExprGet "sum(($x - $mx)^2)"]
   set Sxy [vectorExprGet "sum(($x - $mx)*($y - $my))"]
   set b [expr $Sxy/$Sxx]
   set a [expr $my - $b*$mx]
}

###############################################################################
#
# Return a region with a given gradient
#
proc regGradSet {grad nrow ncol shift} {
   set fac [expr 1<<$shift];		# how much to scale dsky

   if {int(0.5*$fac*abs($grad)) < 0} {
      return ""
   }

   set dsky [binregionNew]
   
   set skyreg [regNew -type S32 2 2]
   handleSetFromHandle $dsky.reg &$skyreg
   loop i 0 2 {
      handleSet $skyreg.rows_s32<0><$i> [expr -0.5*int($grad*$fac)]
      handleSet $skyreg.rows_s32<1><$i> [expr  0.5*int($grad*$fac)]
   }
   handleDel $skyreg
   
   handleSet $dsky.bin_row $nrow
   handleSet $dsky.bin_col $ncol
   handleSet $dsky.shift $shift

   return $dsky
}

###############################################################################
#
# A convenient wrapper to make_colored_diagrams
#
proc CM {args} {
   set two_color 0;			# plot a two-colour diagram?

   set opts [list \
		 [list [info level 0] "Create and display a CM.ppm file"] \
		 [list <name> STRING "" name "Name of output file"] \
		 [list <run1> STRING 0 run1 "First run"] \
		 [list <camCol1> STRING "" camCol1 \
		      "Camera columns in run1 in form 1:6"] \
		 [list <field1> STRING "" field1 \
		      "Camera columns in run1 in form 11:106"] \
		 [list {[run2]} STRING "" run2 "Second run"] \
		 [list {[camCol2]} STRING "" camCol2 \
		      "Camera columns in run2 in form 1:6"] \
		 [list {[field2]} STRING "" field2 \
		      "Camera columns in run2 in form 11:106"] \
		 [list -rerun STRING {} rerun "Desired rerun"] \
		 [list -rerun1 STRING {} rerun1 "Desired rerun for run one"] \
		 [list -rerun2 STRING {} rerun2 "Desired rerun for run two"] \
		 [list -set_run_args STRING "" set_run_args \
		      "Extra arguments to set_run"] \
		 [list -select STRING "sel_good&&sel_gri" sel \
		      "Logical expression to select objects"] \
		 [list -two CONSTANT 1 two_color "Plot a two-colour diagram"] \
		 [list -nrow INTEGER 1024 nrow "Number of rows in diagram"]\
		 [list -ncol INTEGER 1024 ncol "Number of columns in diagram"]\
		 [list -lweight INTEGER 1 lweight "Width of axes/ticks"] \
		 [list -maglim DOUBLE 0 maglim "Desired magnitude limit"] \
		 [list -x0 DOUBLE -0.55 x0 "Minimum value on x-axis"] \
		 [list -x1 DOUBLE  2.05 x1 "Maximum value on x-axis"] \
		 [list -y0 DOUBLE 22.75 y0 "Minimum value on y-axis"] \
		 [list -y1 DOUBLE 13.75 y1 "Maximum value on y-axis"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$rerun != ""} {
      if {$rerun1 != "" && $rerun1 != $rerun} {
	 error "-rerun and -rerun1 must agree"
      }
      set rerun1 $rerun
   }
   #
   # Parse e.g. camCol1, giving camCol11 and camCol12
   #
   set runs $run1
   set ${run1}_rerun \{$rerun1\}
   if {$run2 == ""} {
      if {$camCol2 != "" || $field2 != ""} {
	 error "You cannot specify camCol/field without run2"
      }
   } else {
      lappend runs $run2 
      set ${run2}_rerun \{$rerun2\}
   }
   
   foreach r "1 2" {
      foreach el "camCol field" {
	 if {[set ${el}$r] != ""} {
	    if [regexp {^([0-9]+)$} [set ${el}$r] {} [set run$r]_${el}_1] {
	       set [set run$r]_${el}_2 [set [set run$r]_${el}_1]
	    } elseif ![regexp {^([0-9]+):([0-9]+)$} [set ${el}$r] {} \
		     [set run$r]_${el}_1 [set run$r]_${el}_2] {
	       error "${el}$r ([set ${el}$r]) is not of form nnn:nnn"
	    }
	 }
      }
   }
   #
   # Do the work
   #
   global regs

   set first "-erase";			# used on first time through
   set last "";				# used on last time through
   if $two_color {
      if {$maglim == 0} { set maglim 21 }
      set CM "-maglim $maglim -x0 $x0 -x1 $x1"
   } else {
      if {$maglim == 0} { set maglim 999 }
      set CM "-CM g-r:g -maglim $maglim -x0 $x0 -x1 $x1 -y0 $y0 -y1 $y1"
   }
   foreach run $runs {
      loop c [set ${run}_camCol_1] [expr [set ${run}_camCol_2] + 1] {
	 eval set_run $set_run_args $run $c -rerun "[set ${run}_rerun]"
	 
	 set f1 [set ${run}_field_1];  set f2 [set ${run}_field_2]; 
	 
	 if {$run == [lrange $runs end end] && $c == [set ${run}_camCol_2]} {
	    set last "-smooth 1 -ticks -ppm_file $name -lweight $lweight"
	 }

	 set file [eval make_colored_diagrams regs \
		       -verbose -margin 100 \
		       -sel $sel -nrow $nrow -ncol $ncol -smooth 0 \
		       -filters {"g r i"} -field $f1 -field1 $f2 $CM \
		       -ppm_stretch 0:1000 $first $last]

	 set first ""
      }
   }

   foreach n [array names regs] {
      regDel $regs($n); unset regs($n)
   }

   display_ppm_file -geom +1+1 $file
}

###############################################################################
#
# Make Plan files from scratch
#
proc createPlanFiles {args} {
   global data_root openit

   set canon_dirs 0;			# put files in outDir/run/rerun/...
   set constantPSF 0;			# assume a constant PSF?
   set fpPlan 0; set psPlan 0;		# which plans should I make?
   set fsc 0;				# Plan files for the FSC
   set idR 0;				# copy idR files if clone is true?
   set make_directories 0;		# make all required directories?
   set read_fpC 0;			# read fpC files, not idR
   set ignore_wing_stars 1;		# Use wing stars?

   set opts [list \
		 [list [info level 0] "Make plan files from scratch.
 See also makePlanFiles to convert minimal (photo v5_3+) plan files to
 the full per-column ones used by DP"] \
		 [list <run> INTEGER 0 run "Desired run"] \
		 [list <rerun> INTEGER 0 rerun "Desired rerun"] \
		 [list -camCol INTEGER 0 camCol "Camera column, if desired"] \
		 [list -outDir STRING "" outdir "Base of file hierarchy"] \
		 [list -directories CONSTANT 1 make_directories \
		      "Make all required directories"] \
		 [list -canonical CONSTANT 1 canon_dirs \
		      "Put files in canonical places; outdir/run/rerun/..."]\
		 [list -clone INTEGER -1 clone "Clone this rerun"] \
		 [list -idR CONSTANT 1 idR "Copy idR files too if -clone?"] \
		 [list -filterlist STRING "u g r i z" filterlist \
		      "Desired filters"] \
		 [list -startField INTEGER 0 startField "Starting field"] \
		 [list -endField INTEGER 0 endField "Ending field"] \
		 [list -fpPlan CONSTANT 1 fpPlan "Make fpPlan file"] \
		 [list -psPlan CONSTANT 1 psPlan "Make psPlan file"] \
		 [list -fsc CONSTANT 1 fsc \
		      "Setup for FakeStampCollecting pipeline outputs"] \
		 [list -constantPSF CONSTANT 1 constantPSF \
		      "Assume a constant PSF across the field"] \
		 [list -read_fpC CONSTANT 1 read_fpC \
		      "Read fpC not idR files files"] \
		 [list -wingStars CONSTANT 0 ignore_wing_stars \
		      "Look for and use wing stars"] \
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$startField != 0 && $endField == 0} {
      set endField $startField
   }
   #
   # Which plans do they want?
   #
   if {$fpPlan + $psPlan == 0} {
      set fpPlan 1
   }
   lappend plans fpPlan
   lappend plans psPlan
   #
   # Make directories?
   #
   if {$outdir == ""} {
      set outdir $data_root
   }

   if $canon_dirs {
      set basedir $outdir/$run/$rerun
      if {$clone >= 0 || $make_directories} {
	 foreach d "astrom corr fangs fields logs objcs photo psFangs ssc" {
	    if [regexp {^(astrom|corr|fangs|fields|objcs|psFangs)$} $d] {
	       loop c 1 7 {
		  lappend dirs $d/$c
	       }
	    } elseif {$d == "photo"} {
	       lappend dirs $d/calib
	    } else {
	       lappend dirs $d
	    }
	    
	    foreach d $dirs {	 
	       if ![file isdirectory $basedir/$d] {
		  mkdir -path $basedir/$d
	       }
	    }
	 }
      }
   } else {
      set basedir $outdir
   }
   #
   # Clone a pre-existing rerun?
   #
   set opFiles "opConfig opECalib opBC opCamera"

   if {$clone < 0} {
      foreach f $opFiles {
	 set $f $f.par
      }
   } else {
      set clone_root $data_root/$run/$clone
      if ![file isdirectory $clone_root] {
	 error "$clone_root doesn't seem to exist"
      }

      foreach f $opFiles {
	 if [catch {
	    set parfiles [glob $clone_root/logs/$f*.par]
	 }] {
	    echo "I cannot find a $f.par file in $clone_root"
	    set $f $f.par
	 } else {
	    if {[llength $parfiles] > 1} {
	       echo "There is more than one $f file in rerun $clone: $files"
	    }
	    set $f [lindex $parfiles 0]
	    lappend files "logs/[file tail [set $f]]"
	 }
      }

      lappend files astrom/asTrans-[format %06d $run].fit
      if {$startField > 0} {
	 if {$camCol > 0} {
	    set camCols $camCol
	 } else {
	    set camCols "1 2 3 4 5 6"
	 }
	 foreach c $camCols {
	    foreach filt $filterlist {
	       lappend files objcs/$c/[format psFF-%06d-%s%d.fit $run $filt $c]
	    }
	    
	    loop f $startField [expr $endField + 2] {
	       lappend files objcs/$c/[format psField-%06d-%d-%04d.fit \
					   $run $c $f]
	       foreach filt $filterlist {
		  lappend files objcs/$c/[format psBB-%06d-%s%d-%04d.fit \
					      $run $filt $c $f]
		  if $idR {
		     set file [format fields/$c/idR-%06d-%s%d-%04d.fit \
						  $run $filt $c $f]
		     if ![file exists $data_root/$run/$clone/$file] {
			foreach s [compressionTypes] {
			   if [file exists "$data_root/$run/$clone/$file.$s"] {
			      append file ".$s"
			      break;
			   }
			}
		     }
		     lappend files $file
		  }
	       }
	    }
	 }
      }

      foreach f $files {
	 if ![file exists $clone_root/$f] {
	    echo "I cannot find $clone_root/$f to copy"
	 } else {
	    exec cp $clone_root/$f $basedir/$f
	 }
      }
   }
   #
   # Make those plan files
   #
   if $canon_dirs {
      set plandir "$basedir/photo"
   } else {
      set plandir "."
   }
   if ![file isdirectory $plandir] {
      error "$plandir doesn't exist; consider specifying -directories"
   }

   foreach plan $plans {
      set fd [open "$plandir/$plan.par" w]
      puts $fd "#"
      puts $fd \
	"# Generated by createPlanFiles [fmtclock [getclock]] ([photoVersion])"
      puts $fd "#"
      puts $fd "run			$run"
      puts $fd "rerun			$rerun"
      if {$camCol > 0} {
	 puts $fd "camCol			$camCol"
      }
      if {$startField > 0} {
	 puts $fd "startField		$startField"
      }
      if {$endField > 0} {
	 puts $fd "endField		$endField"
      } elseif {$startField > 0} {
	 puts $fd "endField		$startField"
      }
      puts $fd ""
      puts $fd "filterlist		$filterlist"
      puts $fd ""
      if {$outdir == $data_root} {
	 puts $fd "baseDir			\$data_root"
      } else {
	 puts $fd "baseDir			$outdir"
      }
      if [info exists openit(asdir)] {
	 puts $fd "transFileDir		$openit(asdir)"
      } elseif {$fsc && !$canon_dirs} {
	 puts $fd "transFileDir		\$baseDir"
      }
      if [info exists openit(psdir)] {
	 puts $fd "psDir			$openit(psdir)"
      } elseif {$plan == "fpPlan" && $fsc && !$canon_dirs} {
	 puts $fd "psDir		\$baseDir"
      }
      puts $fd ""
      if $fsc {
	 set pp [string range $plan 0 1]Param

	 if {$pp == "psParam"} {
	    lappend params $pp
	    
	    puts $fd "parametersDir		$plandir"
	    puts $fd "parameters		$pp.par"
	    puts $fd "allow_params		1"
	 }
      }
      puts $fd ""
      foreach f $filterlist {
	 puts $fd "ccdrow_$f		[camRowGet $f]"
      }
      puts $fd ""
      if $fsc {
	 puts $fd {asTrans_format	asTrans-%06d-%d.fit $run $camCol}
	 if !$canon_dirs {
	    if {$plan == "psPlan"} {
	       set dirs "bias config fang output"
	    } else {
	       set dirs "bias config image output ps"
	    }
	    foreach d $dirs {
	       puts $fd "${d}Dir		\$baseDir"
	    }
	 }
      }
      puts $fd "ccdConfig		$opConfig"
      puts $fd "ccdECalib		$opECalib"
      puts $fd "ccdBC			$opBC"
      puts $fd "ccdCamera		$opCamera"
      puts $fd ""
      puts $fd "#outputDir		\$baseDir"
      puts $fd ""
      if {$plan == "psPlan"} {
	 puts $fd "psFieldOld		/dev/null"
	 puts $fd ""
	 puts $fd "#"
	 puts $fd "# Things to ignore"
	 puts $fd "#"
	 puts $fd "diagnostics		0"
	 puts $fd "ignore_calibration	1"
	 puts $fd "ignore_WC		$ignore_wing_stars"
	 puts $fd "ignore_FF		1"
	 puts $fd "ignore_wing_stars	$ignore_wing_stars"
	 puts $fd "ignore_plots		1"
	 puts $fd ""
	 puts $fd "constantPSF		$constantPSF"
	 if $fsc {
	    puts $fd "test_SLprofile		0"
	    puts $fd "fix_background		0"
	    puts $fd "write_psFang		0"
	    puts $fd "trace_memory		0"
	 }
      } else {
	 if $read_fpC {
	    puts $fd "write_fpC_files		0"
	    puts $fd "read_fpC_files		1"
	 } 
	 puts $fd "diagnostics		0"
	 puts $fd "plot_summary		0"
	 if $fsc {
	    puts $fd ""
	    puts $fd "no_overlap		1"
	    puts $fd "read_psBB_files		0"
	    puts $fd "no_CR_removal		1"
	    puts $fd "write_fpC_files		0"
	 }
      }
      puts $fd ""
      puts $fd "verbose			0"

      close $fd
   }

   if [info exists params] {
      foreach param $params {
	 set fd [open "$plandir/$param.par" w]
	 puts $fd "#"
	 puts $fd \
	"# Generated by createPlanFiles [fmtclock [getclock]] ([photoVersion])"
	 puts $fd "#"
	 
	 if {$param == "psParam"} {
	    puts $fd "clip_ssc_quartiles		0"
	 }
	 close $fd
      }
   }
}

###############################################################################
#
# Make {fp,ps}Plan-camCol.par from minimal {fp,ps}PLan.par files;
# this is for the use of e.g. dp
#
# See also createPlanFiles
#
proc makePlanFiles {args} {
   global cmdlinePlan data_root

   set psp 0; set frames 0;		# what sort of files to create

   set opts [list \
		 [list [info level 0] {\
 Make expanded planfiles for data processing, who use them to stuff opdb.
 The values come from the run's plan file and the plan file in photo/etc.

   e.g. makePlanFiles -rerun 666 745 1:6 -frames -out {$baseDir/$run/$rerun/photo}

 See also createPlanFiles} ] \
		 [list <run> INTEGER 0 run "The desired run"] \
		 [list <camCols> STRING "1:6" camCols \
		      "Desired camera columns, e.g. 1:6"] \
		 [list {[startField]} STRING "first" startField \
		    "Desired first (or only) field to process, or \"first\""] \
		 [list {[endField]} STRING "" endField \
		      "Desired last field to process, or \"last\" (default: <startField>)"]\
		 [list -rerun INTEGER -1 rerun "The desired rerun"] \
		 [list -outDir STRING "" outDir \
		      "Where to write output plan files"] \
		 [list -frames CONSTANT 1 frames \
		      "Create fpPlan-camCol.par files"] \
		 [list -psp CONSTANT 1 psp "Create psPlan-camCol.par files"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$frames + $psp != 1} {
      error "Please specify exactly one of -frames and -psp"
   }

   if [regexp {^([1-6])$} $camCols {} camCol] {
      set camCol1 $camCol; set camCol2 $camCol
   } elseif [regexp {^([1-6]):([1-6])$} $camCols {} camCol1 camCol2] {
      ;
   } else {
      error "Illegal camCol specification: $camCols"
   }

   if {$startField != "first" && $startField != "start"} {
      array set cmdlinePlan [list startField $startField]
      if {$endField == ""} {
	 set endField {$startField}
      }
   }
   if {$endField != "" && $endField != "end" && $endField != "last"} {
      array set cmdlinePlan [list endField $endField]
   }
   set root $data_root

   if {$rerun >= 0} {
      array set cmdlinePlan [list rerun $rerun]
   }

   if $frames {
      set plan "fpPlan"
   } elseif $psp {
      set plan "psPlan"
   } else {
      error "I don't know what you want me to do"
   }

   set planfile $root/$run/$rerun/photo/$plan.par
   #
   # Read and set all the parameters in the default and _then_ the
   # specified planfile, in local scope, and also set this list pplan.
   #
   # Note that this means that the specified planfile will _override_
   # any parameters set in the default one
   # 
   set defplanfile "[envscan \$PHOTO_DIR]/etc/$plan.par"
   #
   # Loop over each camCol, generating desired output
   #
   loop camCol $camCol1 [expr $camCol2 + 1] {
      array set cmdlinePlan [list camCol $camCol]
      #
      # Process planfiles
      #
      set cmdline cmdlinePlan
      if [info exists pplan] { unset pplan }
      foreach file "defplanfile cmdline planfile cmdline" {
	 process_plan pplan [read_planfile [set $file]]
      }
      #
      # Remove `default' values, and set the plan as variables
      #
      # If the value is "nodefault", complain
      #
      foreach var [keylget pplan] {
	 set val [keylget pplan $var]
	 if {$val == "default"} {
	    keyldel pplan $var
	 } elseif {$val == "nodefault"} {
	    error "** keyword $var has value nodefault! **"
	 }
	 
	 set $var $val
      }
      #
      # Expand camCol-specific names
      #
      set camCol $cmdlinePlan(camCol)
      
      eval set baseDir $baseDir

      keylset pplan camCol $camCol
#
#  Expand all the directory names. If any are omitted, use $defaultDir.
# Note that all names are expanded, so they can contain a variable, e.g.:
# baseDir	/u/rhl/data
# run		745
# defaultDir	$baseDir/$run
#
# Process the second list (bias etc) second so that they may refer to the first
#
      foreach dir [concat \
		       "config image output ps" \
		       "bias ff fpC ko parameters psBB transFile"] {
	 if {$psp && [regexp {^(image|ps|fpC|psBB)$} $dir]} {
	    continue;			# not used by PSP
	 }
	 
	 set dir ${dir}Dir
	 
	 if ![info exists $dir] {
	    if {$dir == "koDir"} {
	       verb_echo 2 "You haven't specified any known objects"
	       continue;
	    }
	    if [info exists defaultDir] {
	       set $dir $defaultDir
	    } else {
	       echo "No defaultDir; unable to set $dir"; continue
	    }
	 }
	 eval set $dir [set $dir]
	 
	 set $dir [envscan [set $dir]]
      }
      #
      # Open output file now that we've processed all the direcory names
      #
      if {$outDir != ""} {
	 eval set outfile "$outDir/$plan-$camCol.par"
	 set fd [open $outfile "w"]
      } else {
	 set fd "stdout"
      }

      puts $fd "\#"
      puts $fd "\# Plan file was machine generated by"
      puts $fd "\#     [info level 0]"
      puts $fd "\# on [fmtclock [getclock]]"
      puts $fd "\#"
      puts $fd "\# Please do NOT edit"
      puts $fd "\#"

      foreach el [keylget pplan] {
	 if [regexp {Dir$} $el] {
	    if {$el == "includeDir"} {
	       continue
	    }
	    
	    set val [set $el];		# expanded above
	 } else {
	    set val [keylget pplan $el]
	    if {[regexp {((_f|F)ormat)$} $el] ||
		[regexp {^(display|truth_)} $el]} {
	       continue;
	    }
	 }

	 if {$el == "mtPatches"} {
	    set vlist ""
	    foreach v $val {
	       set val [eval list $v]
	       lappend vlist [list [lindex $val 0] [lindex $val 1]]
	    }
	    set val $vlist
	 } else {
	    if [regexp {\$} $val] {
	       eval set val $val
	    }
	 }

	 if {$val == "/dev/null"} {
	    continue;
	 }

	 puts $fd [format "%-30s %s" $el $val]
      }

      if {$fd != "stdout"} {
	 close $fd
	 chmod 444 $outfile
      }
   }
}

###############################################################################

proc showCCD {args} {
   global openit
   set showBC 1;			# show bad columns?
   
   set opts [list \
		 [list [info level 0] "Print the properties of a CCD;
 If you don't specify -dir or explicit files, the previous set_run command
 will be used to find the files."] \
		 [list <ccd> STRING "" ccd "Desired CCD, e.g. r3"] \
		 [list -dir STRING "" dir "Directory to search for files"] \
		 [list -opConfig STRING "" opConfig \
		      "Read this opConfig file"] \
		 [list -opECalib STRING "" opECalib \
		      "Read this opECalib file"] \
		 [list -opBC STRING "" opBC \
		      "Read this opBC file"] \
		 [list -match STRING "" match \
		      "Required match for op file names (e.g. 4x4)"] \
		 [list -noBC CONSTANT 0 showBC "Don't print bad columns"]\
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![info exists openit(logdir)] {
      error "Please use set_run and try again"
   }

   if ![regexp {^([ugriz])([1-6])$} $ccd {} filter camCol] {
      error {Please specify a CCD name of the form [ugriz][1-6], e.g. r1}
   }
   keylset plan ccdrow_$filter [camRowGet $filter]

   set files "opConfig opECalib"
   if {1 || $showBC} {			# fetch_ccdpars needs an opBC file
      lappend files opBC
   }

   if {$dir == ""} {
      set notified 0
      foreach f $files {
	 if {[set $f] == ""} {
	    set dir $openit(logdir)
	    if !$notified {
	       set notified 1
	       echo Looking for files in $dir
	    }
	 }
      }
   }

   foreach f $files {
      if {[set $f] != ""} {
	 if [file exists [set $f]] {	# complete file name
	    continue;
	 }
	 
	 if {$dir != ""} {
	    if [file exists $dir/[set $f]] {# exists in $dir
	       set $f "$dir/[set $f]"
	       continue
	    }
	 }
      }

      if {$dir != ""} {
	 set $f [glob $dir/$f*$match.par]
	 if {[llength [set $f]] == 0} {
	    error "I cannot find a $f file in $dir"
	 } elseif {[llength [set $f]] > 1} {
	    echo "There's more than one $f file in $dir: [set $f] (using [lindex [set $f] 0])"
	    set $f [lindex [set $f] 0]
	 }
      }

      if {[set $f] == ""} {
	 error "Please specify an $f file (or a directory with -dir)"
      }
   }

   fetch_ccdpars \
       "" $opConfig $opECalib $opBC ccdpars $filter $camCol $plan
   #
   # Format data
   #
   set namps [exprGet $ccdpars($filter).namps]

   echo [format "$ccd:  cam{Row,Col} = %d,%d   %d amplifiers   CCD nrow/col = %dx%d (using %dx%d)" \
	     [exprGet $ccdpars($filter).iRow] \
	     [exprGet $ccdpars($filter).iCol] \
	     $namps \
	     [exprGet $ccdpars($filter).nrows] \
	     [exprGet $ccdpars($filter).ncols] \
	     [exprGet $ccdpars($filter).nDataRow] \
	     [exprGet $ccdpars($filter).nDataCol] \
	    ]
   loop a 0 $namps {
      if {[exprGet $ccdpars($filter).amp$a] < 0} {
	 continue;
      }

      foreach el "sPrescan nPrescan sPostscan nPostscan" {
	 set $el [exprGet $ccdpars($filter).${el}$a]
      }
      set bias "  Bias:"
      if {$nPrescan > 0} {
	 append bias \
	     [format " %d--%d" $sPrescan [expr $sPrescan + $nPrescan - 1]]
      }
      if {$nPostscan > 0} {
	 append bias \
	     [format " %d--%d" $sPostscan [expr $sPostscan + $nPostscan - 1]]
      }

      echo [format "Amplifier $a:  Readnoise %.1g  Gain %.2g  Fullwell %.0f$bias" \
		[exprGet $ccdpars($filter).readNoise$a] \
		[exprGet $ccdpars($filter).gain$a] \
		[exprGet $ccdpars($filter).fullWell$a]]

      set coeffs ""
      loop i 0 [exprGet $ccdpars($filter).n_linear_coeffs$a] {
	 lappend coeffs [exprGet $ccdpars($filter).linear_coeffs$a<$i>]
      }

      echo [format "              Linearity: %s  %s" \
		[exprGet -enum $ccdpars($filter).linearity_type] $coeffs]

      if {$showBC && $a == [expr $namps - 1]} {
	 set defects [handleBindFromHandle [handleNew] \
			  *$ccdpars($filter).CCDDefect]
	 loop k 0 [chainSize $defects] {
	    set el [chainElementGetByPos $defects $k]
	    puts -nonewline [format "%-4d (%-6s,%s)  " \
				 [exprGet $el.dfcol0] \
				 [exprGet -enum $el.dftype] \
				 [exprGet -enum $el.dfaction]]
	    if {$k%3 == 2} {
	       echo ""
	    }
	 }
	 if {$k%3 != 0} {
	    echo ""
	 }

	 handleDel $defects
      }
   }
   
   ccdparsDel $ccdpars($filter)

   return ""
}

###############################################################################
#
# Create a quartile region from a region
#
proc reg2quartiles {args} {
   set idR 0;				# is reg read from an idR file?
   set opts [list \
		 [list [info level 0] "Return a 3xNCOL region of <reg>'s
 quartiles."] \
		 [list <reg> STRING "" reg "The input "] \
		 [list -idR CONSTANT 1 idR "<reg> is from an idR file"] \
		 [list -bias0 INTEGER 0 bias0 "Number of regions to treat as
 (undersampled) bias at left of <reg>"] \
		 [list -bias1 INTEGER 0 bias1 "Number of regions to treat as
 (undersampled) bias at right of <reg>"] \
		 [list -tshift DOUBLE 64 tshift \
		      "Multiply quartiles by tshift"] \
		 [list -nrow INTEGER 3 nrow_q "Number of rows in output region (must be >= 3)"] \
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {($bias0 != 0 || $bias1 != 0) && $idR} {
      error "You may not specify -bias\[01\] and -idR"
   }
   if $idR {
      assert {[exprGet $reg.ncol] == 2128}
      set bias0 40; set bias1 40
   }

   if {$bias0 < 0 || $bias1 < 0} {
      error "bias\[01\] must be >= 0: $bias0 $bias1"
   }

   if {$nrow_q < 3} {
      error "-nrow must be at least 3"
   }
   #
   # Do we need to treat bias regions specially?
   #
   set dirn 0;				# collapse onto a row
   
   if {$bias0 == 0} {
      loop i 0 3 {
	 lappend b0list [vectorExprNew 0]
      }
   } else {
      set b0list [vectorGetFromRegion $reg 0 -1 0 [expr $bias0-1] \
		      $dirn -nbin 3 -median -quart]
   }
   set qlist [vectorGetFromRegion $reg 0 -1 $bias0 -[expr $bias1+1] \
		  $dirn -median -quart]
   if {$bias1 == 0} {
      loop i 0 3 {
	 lappend b1list [vectorExprNew 0]
      }
   } else {
      set b1list [vectorGetFromRegion $reg 0 -1 -$bias1 -1  \
		      $dirn -nbin 3 -median -quart]
   }
   #
   # Put those vectors together
   #
   loop i 0 3 {
      vectorExprSet [lindex $qlist $i] \
	  "([lindex $b0list $i] concat [lindex $qlist $i]) concat [lindex $b1list $i]"
      if {$tshift != 1.0} {
	 vectorExprSet [lindex $qlist $i] "$tshift*[lindex $qlist $i]"
      }
   }
   #
   # Assemble into a region
   #
   set ncol [exprGet $reg.ncol]
   set q [regNew $nrow_q $ncol]; regClear $q
   hdrInsWithInt $q.hdr TSHIFT $tshift

   loop i 0 3 {
      set sq [subRegNew $q 1 $ncol $i 0]
      vectorAddToRegionRows $sq [lindex $qlist $i]
      regDel $sq
   }
   #
   # Clean up
   #
   foreach v "$b0list $qlist $b1list" {
      vectorExprDel $v
   }

   return $q
}

###############################################################################
#
# Make an fpFieldstat file
#
proc fpFieldstatCreate {args} {
   global FIELD

   set opts [list \
		 [list [info level 0] "Make a dummy fpFieldstat file"] \
		 [list <outDir> STRING "" outputDir "Directory to write to"] \
		 [list <run> INTEGER 0 run "Desired run"] \
		 [list <camCol> INTEGER 0 camCol "Desired camCol"] \
		 [list <field> INTEGER 0 field "Desired field"] \
		 [list -status STRING "MISSING" status "Status to use"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set status [string toupper $status]
   if ![info exists FIELD($status)] {
      error "Invalid status: please use one of [array names FIELD]"
   }
   set status $FIELD($status)

   set fieldstat [fieldstatNew]

   handleSet $fieldstat.status $status

   set file [format $outputDir/fpFieldStat-%06d-$camCol-%04d.fit $run $field]

   set hdr [hdrNew]
   hdrInsWithInt $hdr RUN $run "Imaging Run Number"
   hdrInsWithInt $hdr CAMCOL $camCol "Column in imaging camera"
   hdrInsWithInt $hdr FIELD $field "the field reduced"

   set ch [chainNew FIELDSTAT]
   chainElementAddByPos $ch $fieldstat
   schema2Fits $ch $file -pdu_hdr $hdr

   chainDestroy $ch fieldstatDel
   hdrFreeAll $hdr; hdrDel $hdr;

   append_types_to_file $file FIELD_FLAGS

   crc2Fits $file "a"

   return 1
}

###############################################################################
#
# Make a mosaic of psBB files
#
proc show_psBB {args} {
   global data_root openit

   set read_psBB 0;			# read psBB files?
   set read_psFF 0;			# read scFang files?
   set read_scFang 0;			# read psFF files?
   set scale 0;				# scale result?
   set opts [list \
		 [list [info level 0] "Make a mosaic of PSP output files"] \
		 [list <run> STRING 0 run "Run of interest"] \
		 [list <col> INTEGER 0 col0 "Column of interest"] \
		 [list <filter> STRING "" c "Name of desired filter"] \
		 [list <field0> INTEGER 0 f0 "Desired starting field"] \
		 [list {[field1]} INTEGER 0 f1 "Desired ending field (default: field0)"] \
		 [list -rootdir STRING "$data_root" root_dir \
		      "Root for SDSS data/input directories"] \
		 [list -rerun STRING "" rerun \
		      "Desired reprocessing number (default: youngest)"] \
		 [list -psBB CONSTANT 1 read_psBB \
		      "Mosaic psBB files (default)"] \
		 [list -psFF CONSTANT 1 read_psFF \
		      "Show psFF files"] \
		 [list -scFang CONSTANT 1 read_scFang \
		      "Mosaic scFang files"] \
		 [list -quartile INTEGER 1 which_quartile \
		      "Index of desired quartile in scFang file"] \
		 [list -scale CONSTANT 1 scale \
		      "Scale region to DN (removing soft bias)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$f1 == 0} {
      set f1 $f0
   }

   if {$read_psBB + $read_psFF + $read_scFang == 0} {
      set read_psBB 1
   }

   if $read_psBB {
      set prefix "psBB"
   } elseif $read_psFF {
      set prefix "psFF"
   } elseif $read_scFang {
      set prefix "scFang"
   } else {
      error "You cannot get here"
   }  

   set nfield  [expr $f1 - $f0 + 1]
   if {$nfield < 0} {
      error "Ending field $f1 is < starting field $f0"
   }

   set_run -root $root_dir -rerun $rerun $run $col0

   loop f $f0 [expr $f1 + 1] {
      if $read_psBB {
	 set psBB [format "%s/$prefix-%s-${c}$col0-%04d.fit" \
		       $openit(objdir) $openit(run) $f]
	 set reg [regReadAsFits [regNew] $psBB]
	 set tshift [hdrGetAsInt $reg.hdr TSHIFT]
      } elseif $read_psFF {
	 set psFF [format "%s/$prefix-%s-${c}$col0.fit" \
		       $openit(objdir) $openit(run)]
	 set reg [regReadAsFits [regNew] $psFF]
	 set tshift [hdrGetAsInt $reg.hdr TSHIFT]
      } elseif $read_scFang {
	 set psFang [format "%s/$prefix-%s-$col0-%04d.fit" \
		       $openit(fangdir) $openit(run) $f]
	 set quart [fang2quart $psFang $c]

	 set reg [regNew -type S32 1 [exprGet $quart.nc]]
	 regIntCopy $reg *$quart.data<$which_quartile>
	 set tshift [exprGet $quart.tscal]

	 quartilesDel $quart
      } else {
	 error "You can't get here"
      }
       
      if ![info exists mosaic] {
	 set ncol [exprGet $reg.ncol]
	 set mosaic [regNew -type S32 $nfield $ncol]
	 regClear $mosaic
      }

      set smosaic [subRegNew $mosaic 1 $ncol [expr $f - $f0] 0]
      regIntCopy $smosaic $reg
      regDel $smosaic

      regDel $reg
   }

   if $scale {
      set fmosaic [regNew -type FL32 $nfield $ncol]
      regMultiplyWithDbl $mosaic [expr 1.0/$tshift] -regOut $fmosaic
      regDel $mosaic
      set mosaic $fmosaic

      if !$read_psFF {
	 regAddWithDbl $mosaic -[softBiasGet]
      }
   }

   return $mosaic
}

###############################################################################
#
# Read a set of psField files, estimating the value of cond3_fac2
#
proc check_cond3_fac2 {args} {
   global data_root openit

   set opts [list \
		 [list [info level 0] "\
 Find the maximum usable value of cond3_fac2 (in the CR code) for a set
 of fields."] \
		 [list {[run]} INTEGER 0 run "The desired run"] \
		 [list {[camCol]} INTEGER 0 camCol0 "Desired camera column"] \
		 [list {-camCol1} INTEGER 0 camCol1 "Desired ending camera column"] \
		 [list <field0> INTEGER 0 field0 "Desired starting field"] \
		 [list {[field1]} INTEGER 0 field1 \
		      "Desired ending field (default: <field0>)"] \
		 [list -rerun INTEGER -1 rerun "The desired rerun"] \
		 [list -outfile STRING "stdout" outFile \
		      "Where to write results.  If the name starts with a +, append"] \
		 [list -filterlist STRING "u g r i z" filterlist \
		      "Name of desired filters"] \
		 [list -cond3_fac2 DOUBLE 1.5 cr_cond32 \
		      "Initial value of cond3_fac2"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set root $data_root

   if {$camCol1 == 0} { set camCol1 $camCol0 }
   if {$field1 == 0} { set field1 $field0 }
   #
   # Deal with output
   #
   if [regsub {^\+} $outFile {} outFile] {
      set mode "a"
   } else {
      set mode "w"
   }
   if {$outFile == "stdout"} {
      set fd stdout
   } else {
      set fd [open $outFile $mode]
   }
   puts $fd \
       "\# run $run rerun $rerun camCol $camCol0:$camCol1 \"$filterlist\"\n\#"
   #
   # Create an (empty) "corrected frame"
   #
   set rsize 1489; set csize 2048;	# real size of corrected frame
   set corrimage [regNew 100 100]; regIntSetVal $corrimage [softBiasGet]
   set mask [spanmaskNew -nrow 100 -ncol 100]
   handleSetFromHandle $corrimage.mask (MASK*)&$mask; handleDel $mask

   set cr_cond3 3

   declare_schematrans [llength $filterlist]

   loop col $camCol0 [expr $camCol1 + 1] {
      set_run -root $root -rerun $rerun $run $col
      loop field $field0 [expr $field1 + 1] {
	 set psFieldFile \
	     [get_psField_filename $openit(psdir) $run $camCol $field]
	 
	 set calib [read_calib1byframe $field $psFieldFile $filterlist]
	 read_psfBasis psfBasis $psFieldFile $filterlist
	 
	 puts -nonewline $fd [format "%4d %3d %d %4d " \
				  $run $rerun $openit(col) $field]
	 foreach filter $filterlist {
	    set index [lsearch $filterlist $filter]
	    #
	    # Recon PSF
	    #
	    set psfReg [psfKLReconstruct $psfBasis($filter) \
			    [expr $rsize/2] [expr $csize/2]]
	    set reg [handleBindFromHandle [handleNew] *$psfReg.reg]
	    handleSet $psfReg.reg 0x0
	    psfRegDel $psfReg
	    set psfRegion $reg
	    #
	    # Check CR detection
	    #
	    findCR $corrimage \
		0 -bkgd 0 \
		1 *$calib.calib<$index>->psf 1 -min_sigma 100 -min_e 100 \
		-cond3 $cr_cond3 -cond32 $cr_cond32 -keep \
		-psfRegion $psfRegion -adopted_cond32 c32
	    puts -nonewline $fd \
		[format "  %.4f %.4f" \
		     [exprGet $calib.calib<$index>->psf->width] $c32]
	    #
	    # Clean up
	    #
	    regDel $psfRegion
	    psfBasisDel $psfBasis($filter); unset psfBasis($filter)
	 }
	 puts $fd ""; flush $fd
	 
	 calib1byframeDel $calib
      }
   }

   fitsBinForgetSchemaTrans NULL;
   regDel $corrimage
   
   if {$fd != "stdout"} {
      close $fd
   }
}

###############################################################################
#
# Generate a set of PPM files for every atlas image in an fpAtlas file
#

proc fpAtlas2ppm {args} {
   global objcIo OBJECT1

   set show_family 0
   set opts [list \
		 [list [info level 0] ""] \
		 [list {[n1]} INTEGER 1 n1 "first object to display"] \
		 [list {[n2]} INTEGER -1 n2 "last object to display"] \
		 [list -filters STRING "irg" filters \
		      "3 filters to use in constructing output file"] \
		 [list -binned INTEGER 1 binfac \
		      "How much to bin PPM file"] \
		 [list -stretch STRING "0:100s" method \
		      "stretch to use, e.g. 0:100
 (if it ends in s, e.g. 0:10s, in units of sigma)"] \
		 [list -type STRING "log" type \
		      "type of stretch: asinh:n sqrta:n lin, log, sqrt"] \
		 [list -select STRING "" select "Selection function"] \
		 [list -children CONSTANT 1 show_family \
		      "Show blends' families"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set args ""
   if !$show_family {
      lappend args "-nochildren"
   }

   upvar table table
   if ![info exists table] {
      error "Please use set_run and try again"
   }
   
   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }

   if {"[keylget table fileSchema]" == "OBJC_IO"} {
       set is_fpObjc 1
   } else {
       set is_fpObjc 0
   }
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]

   

   loop i $n1 [expr $n2 + 1] {
      catch {catObjDel $objcIo -deep};	# it may not exist
      set objcIo [objcIoRead table $i]
      
      if {[exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)} {
	 continue
      }

      if {"$select" != "" && ![eval $select $objcIo]} {
	 incr i;
	 continue;
      }

      if [catch {
	 eval mtv_blend_ppm table [exprGet $objcIo.id] -filter $filters \
	     -binned $binfac -stretch $method -type $type -nodisplay $args
      } msg] {
	 echo $msg
      }
   }
}

###############################################################################
#
# Routines to convert fpM files to and from U16 images
#
proc fpM2reg {args} {
   global MASK_TYPE

   set opts [list \
		 [list [info level 0] \
		      "Read an fpM file and return a region containing some or all of its bitplanes"] \
		 [list <fpMfile> STRING "" fpMfile "Name of input fpM file"] \
		 [list -types STRING [array names MASK_TYPE] types \
		      "Names of desired bitplanes"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![file exists $fpMfile] {
      error "File $fpMfile doesn't seem to exist"
   }
   set hdr [hdrReadAsFits [hdrNew] $fpMfile]
   #
   # Clean FITS keywords out of header
   #
   foreach key "SIMPLE BITPIX NAXIS EXTEND END" {
      hdrDelByKeyword $hdr $key
   }
   #
   # Read fpM file
   #
   if [catch { set sm [get_mask $fpMfile] }] {
      declare_schematrans [hdrGetAsInt $hdr "NFILTER"]
      set sm [get_mask $fpMfile]
   }

   set reg [regNew [exprGet $sm.nrow] [exprGet $sm.ncol]]
   regClear $reg
   hdrCopy $hdr $reg.hdr; hdrDel $hdr

   foreach mt [array names MASK_TYPE] {
      if {[lsearch $types $mt] < 0} {
	 continue;
      }
      
      set m [handleBindFromHandle [handleNew] *$sm.masks<$MASK_TYPE($mt)>]
      regSetFromObjmaskChain $m $reg [expr 1<<$MASK_TYPE($mt)]
      handleDel $m
   }

   if 1 {				# debugging; include mask in region
      handleSetFromHandle $reg.mask (MASK*)&$sm
      handleDel $sm
   } else {
      spanmaskDel $sm
   }

   return $reg
}

proc reg2spanmask {args} {
   global MASK_TYPE IMASK_TYPE

   set opts [list \
		 [list [info level 0] ""] \
		 [list <reg> STRING "" reg \
		      "Region to be converted to a SPANMASK which will be returned"] \
		 [list -types STRING [array names MASK_TYPE] types \
		      "Names of desired bitplanes"] \
		 [list -level INTEGER 0 level \
		      "Include all pixels >= level in mask"] \
		 [list -spanMask STRING "" sm \
		      "Input SPANMASK to be augmented. If omitted, one will be created for you"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # Start by creating the SPANMASK
   #
   if {$sm == ""} {
      set sm [spanmaskNew -nrow [exprGet $reg.nrow] -ncol [exprGet $reg.ncol]]
   }
   #
   # Set the planes
   #
   if {$level > 0} {			# look for any pixel >= level
      set val $level
   } else {				# any pixel that survives Logand
      set val 1
   }

   set levels [vectorExprEval "{$val}"];# levels for object finder

   if {$level == 0} {
      loop i 0 [array size MASK_TYPE] {
	 if {[lsearch $types $IMASK_TYPE($i)] < 0} {
	    continue;
	 }
	 #
	 # Run object finder to find regions where certain bits are set.
	 #
	 set ureg [regIntLogand [regIntCopy "" $reg] [expr (1<<$i)]]
	 
	 set objlist [regFinder $ureg $levels]
	 spanmaskSetFromObjectChain $objlist $sm $i 0
	 
	 objectChainDel $objlist
	 regDel $ureg
      }
   } else {
      #
      # Run object finder to find regions where certain bits are set.
      #
      set objlist [regFinder $reg $levels]
      loop i 0 [array size MASK_TYPE] {
	 if {[lsearch $types $IMASK_TYPE($i)] < 0} {
	    continue;
	 }
	 spanmaskSetFromObjectChain $objlist $sm $i 0
      }
	 
      objectChainDel $objlist
   }
   
   vectorExprDel $levels

   return $sm
}

#
# And write it
#
proc spanmask2fpM {args} {
   global MASK_TYPE
   set opts [list \
		 [list [info level 0] ""] \
		 [list <spanmask> STRING "" sm \
		      "spanmask to be written as an fpM file"] \
		 [list <fpMFile> STRING "" fpMfile \
		      "Name of output fpM file"] \
		 [list <hdr> STRING "" hdr "Header to write to fpM file"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   set fd [fitsBinTblOpen $fpMfile w -hdr $hdr]
     
   loop i 0 [array size MASK_TYPE] {
      set ochain [handleBindFromHandle [handleNew] *$sm.masks<$i>]
	
      fitsBinTblHdrWrite $fd "OBJMASK"
      loop j 0 [chainSize $ochain] {
	 set objmask [chainElementGetByPos $ochain $j]
	 fitsBinTblRowWrite $fd $objmask	   
	 handleDel $objmask
      }
      fitsBinTblEnd $fd

      handleDel $ochain
   }
   fitsBinTblClose $fd

   append_types_to_file $fpMfile S_MASKTYPE   

   return $fpMfile
}

#
# Simple wrapper for the preceeding two functions
#
proc reg2fpM {args} {
   global MASK_TYPE
   
   set opts [list \
		 [list [info level 0] \
 "Convert a region to a SPANMASK and write it to an fpM file.  If any
 non-zero pixel corresponds to a bit in a given bitplane, you can say
    reg2fpM \$reg_SATUR -type SATUR -level 1
 If you want to set more than one plane, try
    set sm \[reg2spanmask -types SATUR -level 1 \$reg_SATUR\]
            reg2spanmask -types CR    -level 1 \$reg_CR     -spanMask \$sm
    spanmask2fpM \$sm \$fpMfile $reg_SATUR.hdr
    spanmaskDel \$sm
 "] \
		 [list <reg> STRING "" reg \
		      "Region to be converted to a SPANMASK"] \
		 [list <fpMfile> STRING "" fpMfile "fpM file to write"] \
		 [list -types STRING [array names MASK_TYPE] types \
		      "Names of desired bitplanes"] \
		 [list -level INTEGER 0 level \
		      "Include all pixels >= level in mask; only makes sense with a single -types argument"] \
		 [list -spanMask STRING "" sm \
		      "Input SPANMASK to be augmented. If omitted, one will be created for you"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set sm [reg2spanmask -types $types -level $level $reg]
   spanmask2fpM $sm $fpMfile $reg.hdr

   spanmaskDel $sm

   return ""
}

###############################################################################
#
# Make a PR testcase.
#
# I could have sworn that I wrote this before
#
proc makePR {args} {
   global data_root openit

   set verbose 0

   if [info exists data_root] {
      set pr_root "$data_root/prs"
   } else {
      set pr_root "."
   }

   set astrom_pr 0;			# is this an astrom PR?
   set asTrans 0;			# Use asTrans, not scTrans, file?
   set scTrans 0;			# Use scTrans, not asTrans, file?
   set fix_background 1;		# Ask PSP to "fix_background"?
   set frames_pr 0;			# is this a frames PR?
   set ignore_WC 0;			# ignore wing stars?
   set relocateable 0;			# normalize an FNAL-generated PR?
   set overlap 0;			# provide overlap field too?
   set psp_pr 0;			# is this a PSP PR?
   set save_PS 0;			# Save Pan-STARRS files too? 
   set ssc_pr 0;			# is this an SSC PR?
   set use_fpC 0;			# Use fpC, not idR, files?

   set opts [list \
		 [list [info level 0] \
 "Make a _relocatable_ PR testcase (i.e. one that can be copied to other sites
 without the need to edit plan files) (use the -reloc flag to make a PR test
 case relocatable).

 The usual usage is:
   set_run -rerun <rerun> <run> <camCol> <field>
   makePR XXXX
 but if the field crashed, you may need to say:
   set_run -rerun <rerun> <run> <camCol>
   makePR XXXX -field <field>

 If you need to fix ssc or psp, use the -ssc or -psp flags.
 "] \
		 [list <pr> STRING "" pr "PR number"] \
		 [list -field INTEGER 0 field \
		      "Field containing problem case"] \
		 [list -filters STRING "all" filters \
		      "Filters to include"] \
		 [list -astrom CONSTANT 1 astrom_pr \
 "This is a astrom PR. Create as{Plan,Param}.par files"] \
		 [list -ssc CONSTANT 1 ssc_pr \
 "This is a SSC PR.  Include <nfield> fields starting at <field>"] \
		 [list -psp CONSTANT 1 psp_pr \
 "This is a PSP PR.  Include <nfield> fields starting at <field>"] \
		 [list -frames CONSTANT 1 frames_pr \
		      "This is a Frames PR (default)"] \
		 [list -nfield INTEGER 1 nfield \
		      "Number of fields to include in PR"] \
		 [list -relocateable CONSTANT 1 relocateable \
		      "Normalize directories for an FNAL-generated PR"] \
		 [list -idFFiter INTEGER 2 idFFiter \
		      "Iteration number for flat fields"] \
		 [list -asTrans CONSTANT 1 asTrans \
		      "Use asTrans, not scTrans, file for astrometry"] \
		 [list -scTrans CONSTANT 1 scTrans \
 "Use scTrans, not asTrans, file for astrometry (default for frames PRs)"] \
		 [list -noFix_background CONSTANT 0 fix_background \
 "Don't ask the PSP to \"fix_background\", i.e. find psFF/psBB files"] \
		 [list -ignore_WC CONSTANT 1 ignore_WC "Ignore wing stars"] \
		 [list -dir STRING "$pr_root" pr_root \
		      "Root directory for PRs"] \
		 [list -psphot CONSTANT 1 save_PS "Save psphot files too?"] \
		 [list -fpC CONSTANT 1 use_fpC \
		      "Use fpC not idR files?"] \
		 [list -overlap CONSTANT 1 overlap \
		      "Include next (overlap) field?"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {!$ssc_pr && !$psp_pr && !$frames_pr} {
      set frames_pr 1;			# default
   }
   if {$frames_pr && !$scTrans} {
      set asTrans 1
   }

   if $save_PS {
      if !$frames_pr {
	 error "You can only specify -psphot with frames PRs"
      }
      set use_fpC 1
      set overlap 1
   }

   if {$ssc_pr || $psp_pr} {
      if {$use_fpC || $overlap} {
	 error "-fpC and -overlap make no sense with -psp or -ssc"
      }
   }
   if {$ssc_pr && $asTrans} {
      error "-asTrans makes no sense with -ssc"
   }
   if !$psp_pr {
      if {!$fix_background || $ignore_WC} {
	 error "-noFix_background and -ignore_WC only make sense with -psp"
      }
      
      if $overlap { incr nfield }
   }

   if $relocateable {			# PR exists with hard-coded paths
      set odir $data_root/prs/$pr
      if ![file exists $odir] {
	 error "I cannot find a pre-existing PR $pr"
      }
      if [catch {
	 set planfile [glob $odir/photo/fpPlan-?.par]
      } msg] {
	 error "Failed to find an old planfile: $msg"
      }

      param2Chain $planfile oldplan

      set ndir $odir/[keylget oldplan run]/0
      if [catch {
	 mkdir -path $ndir
      } msg] {
	 error "Cannot make prs/$pr/[keylget oldplan run]/0"
      }
      #
      # move the directories; actually, just make symbolic links
      #
      foreach d [glob $odir/*] {
	 set d [file tail $d]
	 if {"$d" == "$pr"} {
	    continue;			# A circular link to the PR
	 }

	 link -sym ../../$d $ndir/$d
      }
      #
      # And a new plan file
      #
      set fd [open $ndir/photo/fpPlan.par "w"]

      puts $fd "\#"
      puts $fd "\# PR $pr"
      puts $fd "\#"
      puts $fd "\# You should be able to reproduce this PR by saying"
      puts $fd "\#    do-frames -pr $pr"
      puts $fd "\#"
      puts $fd "baseDir 		\$data_root/prs/$pr"
      puts $fd ""
      puts $fd "run     		[keylget oldplan run]"
      puts $fd "rerun   		0"
      puts $fd "camCol  		[keylget oldplan camCol]"
      puts $fd "startField		[keylget oldplan startField]"
      puts $fd "endField		[keylget oldplan endField]"

      if ![keylget oldplan ccdCamera file] {
	 keylset oldplan ccdCamera "/dev/null";	# for the sake of v5_4 
      }

      puts $fd ""
      set parFiles "ccdConfig ccdECalib ccdBC opCamera idCoadds"
      foreach f $parFiles {
	 if [keylget oldplan $f file] {
	    puts $fd "$f    		$file"
	 }
      }
      
      puts $fd ""
      puts $fd "write_fpC_files 	0"

      close $fd

      return
   }

   if ![info exists openit(run)] {
      error "Please use set_run and try again"
   }
   if {$field == 0} {
      if [info exists openit(field)] {
	 set field $openit(field)
      } else {
	 error "Please specify a field, either directly or via set_run"
      }
   }

   set field0 [expr $field];
   set field1 [expr $field0 + $nfield - 1]
      
   if {$filters == "all"} {
      global table
      if [catch {
	 if {![info exists table] || [keylget table field] != $field} {
	    set table [openit $field]
	 }
      } msg] {
	 set filters "u g r i z"
      } else {
	 set filters [keylget table filters]
      }
   }
   #
   # Tell us how to find psField files
   #
   global psField_format
   if ![info exists psField_format] {
      set psField_format {psField-%06d-%d-%04d.fit $run $camCol $field}
   }
   #
   # Create directories
   #
   regsub {^0*} $openit(run) {} run; if {$run == ""} { set run 0 }

   set baseDir $pr_root/$pr
   set root "$baseDir/$run/0"

   set camCol $openit(col)
   set dirs [list objcs fields photo astrom logs]
   if {$ssc_pr || $psp_pr} {
      if $ssc_pr {
	 lappend dirs "gangs"
      }
      lappend dirs "fangs"
      lappend dirs "ssc"
      lappend dirs "psFangs"
   }
   foreach d $dirs {
      if ![regexp {astrom|logs|photo|ssc} $d] {
	 append d "/$camCol"
      }
      if ![file exists $root/$d] {
	 mkdir -path $root/$d
      }
   }
   if $psp_pr {
      if ![file exists $root/photo/calib] {
	 mkdir -path $root/photo/calib
      }
   }
   #
   # Copy files
   #
   # Plan files.  Even for the PSP, generate an fpPlan file too
   #
   if $astrom_pr {
      lappend basenames asPlan
   }
   if {$ssc_pr || $psp_pr} {
      if $ssc_pr {
	 lappend basenames scPlan
      }
      lappend basenames psPlan
   }
   if {$ssc_pr || $psp_pr || $frames_pr} {
      lappend basenames fpPlan
   }

   if {$frames_pr && [file exists $openit(photo)/fpParam.par]} {
      just_copy $openit(photo)/fpParam.par $root/photo/fpParam.par $verbose
      set have_fpParam 1
   }

   set formats {}
   foreach basename $basenames {
      set file "$basename.par"
      if {$basename == "asPlan"} {
	 set indir $openit(asdir)
	 set outdir $root/astrom
      } elseif {$basename == "scPlan"} {
	 set indir $openit(sscdir)
	 set outdir $root/ssc
      } else {
	 set indir $openit(photo)
	 set outdir $root/photo
      }

      if ![file exists $indir/$file] {
	 set file $basename-$camCol.par
	 if ![file exists $indir/$file] {
	    error "I Cannot find a valid $basename file in $indir"
	 }
      }
      param2Chain $indir/$file fpplan

      foreach v [keylget fpplan] {	# save all _format entries
	 if [regexp {_format$} $v] {
	    if ![keylget formats $v foo] {
	       keylset formats $v [keylget fpplan $v]
	    }
	 }
      }

      set parFiles "ccdConfig ccdECalib ccdBC"
      if {$basename == "asPlan"} {
	 lappend parFiles "weatherFile logFile"
      }
      if {$basename == "scPlan"} {
	 lappend parFiles "report"
      }
      if [keylget fpplan idCoadds file] {
	 set have_idCoadds 1
	 lappend parFiles "idCoadds"
      }
      
      foreach f $parFiles {
	 eval set file [keylget fpplan $f]
	 if {$file != "/dev/null"} {
	    eval just_copy $openit(logdir)/$file $root/logs/$file $verbose
	 }
      }
      
      if [keylget fpplan idCoadds file] {
	 set have_idCoadds 1
	 if {$file != "/dev/null"} {
	    eval just_copy $openit(logdir)/$file $root/logs/$file $verbose
	 }
      }
      
      set fd [open $outdir/$basename.par "w"]
      puts $fd "\#"
      puts $fd "\# PR $pr"
      puts $fd "\#"
      puts $fd "\# You should be able to reproduce this PR by saying"
      if {$basename == "asPlan"} {
	 puts $fd "\#    set data_root \$data_root/prs/$pr; do-astrom -rerun 0 $run $camCol"
      } elseif {$basename == "scPlan"} {
	 puts $fd "\#    do-ssc -pr $pr"
      } elseif {$basename == "psPlan"} {
	 puts $fd "\#    do-psp -pr $pr"
      } else {
	 puts $fd "\#    do-frames -pr $pr"
      }
      puts $fd "\#"
      if {$baseDir == "$data_root/prs/$pr"} {
	 puts $fd "baseDir 		\$data_root/prs/$pr"
      } else {
	 puts $fd "baseDir 		$baseDir"
      }
      puts $fd ""
      puts $fd "run     		$run"
      puts $fd "rerun   		0"
      puts $fd "camCol  		$camCol"
      puts $fd "startField		$field0"
      puts $fd "endField		[expr $field1 - ($overlap && !$use_fpC ? 1 : 0)]"
      puts $fd "filterlist  		$filters"

      puts $fd ""
      foreach f $parFiles {
	 puts $fd "$f    		[keylget fpplan $f]"
      }

      if {[keylget fpplan koDir koDir] && $koDir != "/dev/null"} {
	 puts $fd "koDir    		$koDir"
      }
      
      if [regexp {^(fp|ps)Plan$} $basename] {
	 if !$asTrans {
	    puts $fd "transFileDir \t	\$baseDir/\$run/\$rerun/ssc"
	    puts $fd "asTrans_format\t\tscTrans-%06d-%d.fit	\$run \$camCol"
	 }
      }

      foreach v [keylget formats] {
	 puts $fd "$v\t[keylget formats $v]"
      }

      if {$basename == "psPlan"} {
	 puts $fd "fix_background	$fix_background"
	 puts $fd "ignore_WC		$ignore_WC"
	 puts $fd "idFFiter                $idFFiter"
      } elseif {$basename == "asPlan"} {
	 regexp {idReport-([0-9]+)\.par} [keylget fpplan logFile] {} mjd
	 puts $fd "mjd                $mjd"

	 puts $fd {
 configDir		$baseDir/$run/$rerun/logs
 koDir			$baseDir/$run/$rerun/astrom
 logFileDir          	$baseDir/$run/$rerun/logs
 outputDir           	$baseDir/$run/$rerun/astrom
 parametersDir		$baseDir/$run/$rerun/astrom

 logFile		idReport-$mjd.par
 weatherFile		idWeather-$mjd.par

 inputDirs \
	{1 $baseDir/$run/$rerun/psFangs/1} \
	{2 $baseDir/$run/$rerun/psFangs/2} \
	{3 $baseDir/$run/$rerun/psFangs/3} \
	{4 $baseDir/$run/$rerun/psFangs/4} \
	{5 $baseDir/$run/$rerun/psFangs/5} \
	{6 $baseDir/$run/$rerun/psFangs/6} \
	{7 $baseDir/$run/$rerun/psFangs/7} \
	{8 $baseDir/$run/$rerun/psFangs/8} \
	{9 $baseDir/$run/$rerun/psFangs/9} \
	{10 $baseDir/$run/$rerun/psFangs/10} \
	{11 $baseDir/$run/$rerun/psFangs/11}

 parameters		asParam.par
	 }
      } elseif {$basename == "scPlan"} {
	 regexp {idReport-([0-9]+)\.par} [keylget fpplan report] {} mjd
	 puts $fd "mjd                $mjd"
	 #puts $fd "aGangInputDir		/dev/null"
	 #puts $fd "ignoreFrameLog		1"
	 
	 set koCat [keylget fpplan koFileBase]-$openit(run).fit
	 foreach c "$camCol 6 7 8 9 10" {
	    set idFrameLog($c) idFrameLog-$openit(run)-[format %02d $c].par
	 }
      } elseif {$basename == "fpPlan"} {
	 puts $fd ""
	 puts $fd "no_overlap		[expr !$overlap || $use_fpC]"
	 puts $fd "write_fpC_files 	0"
	 if $use_fpC {
	    puts $fd "read_fpC_files 	1"
	 }

	 if [info exists have_fpParam] {
	    puts $fd "allow_params	1"
	 }
      } else {
	 error "Unsupported basename: $basename"
      }
      close $fd
   }
   #
   # Copy needed files
   #
   if $ssc_pr {
      #
      # idGang
      #
      set gangdir $openit(gangdir)

      loop fld $field0 [expr $field1 + 8 + 1] {# "+ 8" to go from r to g frame
	 foreach cr [list c$camCol r6 r7] {
	    set file idGang-$openit(run)-$cr-[format %04d $fld].fit
	    if ![file exists $gangdir/$file] {
	       foreach ct [compressionTypes] {
		  if [file exists $gangdir/$file.$ct] {
		     append file ".$ct"
		  }
	       }
	    }
	    if ![file exists $gangdir/$file] {
	       error "Cannot find $file in $gangdir (even compressed)"
	    }
	    
	    just_copy $gangdir/$file $root/gangs/$file $verbose
	 }
      }

      just_copy $openit(sscdir)/$koCat $root/ssc/$koCat $verbose
      foreach el [array names idFrameLog] {
	 set file $idFrameLog($el)
	 just_copy $openit(logdir)/$file $root/logs/$file $verbose
      }
   }
   
   if $psp_pr {
      #
      # scFang
      #
      set fangdir $openit(fangdir)

      loop fld $field0 [expr $field1 + 1] {
	 set file scFang-$openit(run)-$camCol-[format %04d $fld].fit
	 if ![file exists $fangdir/$file] {
	    foreach ct [compressionTypes] {
	       if [file exists $fangdir/$file.$ct] {
		  append file ".$ct"
	       }
	    }
	 }
	 if ![file exists $fangdir/$file] {
	    error "Cannot find $file in $fangdir (even compressed)"
	 }
	 
	 just_copy $fangdir/$file $root/fangs/$camCol/$file $verbose
      }
      #
      # Maybe scWing stars
      #
      if !$ignore_WC {
	 set file scWing-$openit(run)-$camCol.par
	 if [file exists $fangdir/$file] {
	    just_copy $fangdir/$file $root/fangs/$camCol/$file $verbose
	 } else {
	    echo "I cannot find $file in $fangdir; setting ignore_WC"
	    set ignore_WC 1
	 }

	 loop fld $field0 [expr $field1 + 1] {
	    foreach f $filters {
	       set file scWing-$openit(run)-${f}$camCol-[format %04d $fld].fit
	       if ![file exists $fangdir/$file] {
		  foreach ct [compressionTypes] {
		     if [file exists $fangdir/$file.$ct] {
			append file ".$ct"
		     }
		  }
	       }
	       if [file exists $fangdir/$file] {
		  just_copy $fangdir/$file $root/fangs/$camCol/$file $verbose
	       }
	    }
	 }
      }
      #
      # idFF files.
      #
      set ffpath idFFdir/iteration$idFFiter
      if ![file exists $baseDir/$ffpath] {
	 mkdir -path $baseDir/$ffpath
      }
      set season [getFFseasonNumberFromRun $run]

      foreach f $filters {
	 set file [format "idFF-%03d%03d-${f}$camCol.fit" $idFFiter $season]
	 just_copy $data_root/$ffpath/$file $baseDir/$ffpath/$file $verbose
      }
      #
      # idB files
      #
      global idB_format
      if ![keylget formats "idB_format" idB_format] {
	 set idB_format {idB-%06d-%s%d.fit       $run $filter $camCol}
      }
      foreach f $filters {
	 set file [get_bias_filename "" $openit(run) $f $camCol]
	 just_copy $openit(photo)/calib/$file $root/photo/calib/$file $verbose
      }
      #
      # Trans from ssc outputs
      #
      global asTrans_format
      if $asTrans {
	 if ![keylget formats "asTrans_format" asTrans_format] {
	    set asTrans_format {asTrans-%06d.fit $run $camCol}
	 }

	 set file [get_trans_filename "" $openit(run) $f $camCol]
	 if ![file exists $openit(asdir)/$file] {
	    set file asTrans-$openit(run)-$camCol.fit
	    if ![file exists $openit(asdir)/$file] {
	       error "Cannot find asTrans file in $openit(asdir)"
	    }
	 }
	 just_copy $openit(asdir)/$file $root/astrom/$file $verbose
      } else {
	 if ![keylget formats "asTrans_format" asTrans_format] {
	    set asTrans_format {scTrans-%06d-%d.fit $run $camCol}
	 }

	 set file [get_trans_filename "" $openit(run) $f $camCol]

	 if ![file exists $openit(fangdir)/$file] {
	    error "Cannot find scTrans file $file in $openit(fangdir)"
	 }
	 just_copy $openit(fangdir)/$file $root/ssc/$file $verbose
      }
   }
   if {$ssc_pr || $frames_pr} {
      #
      # idR/fpC files
      #
      if $use_fpC {
	 set idR "fpC"; set fielddir $openit(corrdir)
      } else {
	 set idR "idR"; set fielddir $openit(fielddir)
      }

      foreach f $filters {
	 loop fld $field0 [expr $field1 + 1 + ($overlap ? 1 : 0)] {
	    set file $idR-$openit(run)-${f}$camCol-[format %04d $fld].fit
	    if ![file exists $fielddir/$file] {
	       foreach ct [compressionTypes] {
		  if [file exists $fielddir/$file.$ct] {
		     append file ".$ct"
		  }
	       }
	    }
	    if ![file exists $fielddir/$file] {
	       error "Cannot find $file in $fielddir (even compressed)"
	    }
	    
	    just_copy $fielddir/$file $root/fields/$camCol/$file $verbose
	    
	    if $use_fpC {			# Need fpM files too
	       set file [get_mask_filename "" $run $f $camCol $fld]

	       if ![file exists $openit(dir)/$file] {
		  foreach ct [compressionTypes] {
		     if [file exists $openit(dir)/$file.$ct] {
			append file ".$ct"
		     }
		  }
	       }
	       
	       if [file exists $openit(dir)/$file] {
		  just_copy $openit(dir)/$file $root/fields/$camCol/$file $verbose
	       } else {
		  echo "Warning: unable to find $file"
	       }
	    }
	 }
      }
      if $frames_pr {
	 #
	 # Astrom outputs
	 #
	 set file asTrans-$openit(run).fit
	 if ![file exists $openit(asdir)/$file] {
	    set file asTrans-$openit(run)-$camCol.fit
	    if ![file exists $openit(asdir)/$file] {
	       error "Cannot find asTrans file in $openit(asdir)"
	    }
	 }
	 just_copy $openit(asdir)/$file $root/astrom/$file $verbose
      }

      if $save_PS {
	 if ![file exists $root/corr] {
	    link -sym $root/fields $root/corr
	 }
	 catch {
	    foreach file [glob $root/corr/$camCol/fpM*gz] {
	       exec gunzip $file
	    }
	 }

	 regsub {/logs$} $openit(logdir) "/PS" openit(PSdir)
	 mkdir -path $root/PS/config
	 mkdir -path $root/PS/$camCol
	 
	 set file "ipprc.config"
	 just_copy $openit(PSdir)/$file $root/PS/$file $verbose
	 foreach file [list camera.config  format.sdss  ppImage.config  psphot.config] {
	    just_copy $openit(PSdir)/config/$file $root/PS/config/$file $verbose
	 }
      }
   }
   #
   # PSP outputs
   #
   if {!$ssc_pr && !($psp_pr && $fix_background) &&
       !($frames_pr && $use_fpC)} {
      foreach f $filters {
	 set file psFF-$openit(run)-${f}$camCol.fit
	 just_copy $openit(psdir)/$file $root/objcs/$camCol/$file $verbose
	 
	 loop field $field0 [expr $field1 + 1] {
	    set file psBB-$openit(run)-${f}$camCol-[format %04d $field].fit
	    just_copy $openit(psdir)/$file $root/objcs/$camCol/$file $verbose
	 }
      }
   }

   if $frames_pr {
      loop field $field0 [expr $field1 + 1] {
	 set file [get_psField_filename "" $openit(run) $camCol $field]
	 just_copy $openit(psdir)/$file $root/objcs/$camCol/$file $verbose
      }
   }

   if $frames_pr {
      if ![file exists $root/nfcalib] {
	 mkdir -path $root/nfcalib
      }
      
      set file "calibPhotom-$openit(run)-$camCol.fits"
      if [file exists $openit(nfcalibdir)/$file] {
	 just_copy $openit(nfcalibdir)/$file $root/nfcalib/$file
      }
   }
}

#
# Add a set of biases to a set of idR files
#
proc add_bias_to_idR {args} {
   set opts [list \
		 [list [info level 0] ""] \
		 [list <run_in> INTEGER 0 run_in \
		      "Run with input idR files"] \
		 [list -rerun_in INTEGER -1 rerun_in \
		      "Rerun with input idR files"] \
		 [list <camCol_in> INTEGER 0 camCol_in \
		      "CamCol with input idR files"] \
		 [list <field0_in> INTEGER 0 field0_in \
		      "Starting field number for input idR files"] \
		 [list -nfield INTEGER 1 nfield \
		      "Number of frames to process"] \
		 \
		 [list <run_bias> INTEGER 0 run_bias \
		      "Run with input idR files"] \
		 [list -rerun_bias INTEGER -1 rerun_bias \
		      "Rerun with input idR files"] \
		 [list <camCol_bias> INTEGER 0 camCol_bias \
		      "CamCol with input idR files"] \
		 [list <field0_bias> INTEGER 0 field0_bias \
		      "Starting field number for input idR files"] \
		 [list -bias_dir STRING "" bias_dir \
		      "Directory with idR files to be used as biases"] \
		 [list -outdir STRING "." out_dir \
		      "Directory for output idR files"] \
		 [list -filterlist STRING "u g r i z" filterlist \
		      "Filters to process"] \
		 [list -verbose INTEGER 0 verbose "Chattiness"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![file isdir $out_dir] {
      error "$out_dir is not a directory"
   }
   global openit table

   set_run -rerun $rerun_in $run_in $camCol_in
   set infield_dir $openit(fielddir)

   if {$bias_dir == ""} {
      set_run -rerun $rerun_bias $run_bias $camCol_bias
      set bias_dir $openit(fielddir)
   }

   loop field $field0_in [expr $field0_in + $nfield] {
      foreach f $filterlist {
	 set infile [format "idR-%06d-$f$camCol_in-%04d.fit" \
			 $run_in $field]
	 if {$verbose > 0} {
	    echo $infile
	 }
	 set inreg [regReadAsFits [regNew] "$infield_dir/$infile"]

	 set biasfile [format "idR-%06d-$f$camCol_in-%04d.fit" \
			   $run_bias [expr $field - $field0_in + $field0_bias]]
	 if {$verbose > 0} {
	    echo $biasfile
	 }
	 set biasreg [regReadAsFits [regNew] "$bias_dir/$biasfile"]
	 
	 regIntLincom $inreg $biasreg 0 1 1
	 regWriteAsFits $inreg "$out_dir/$biasfile.Z"

	 regDel $inreg; regDel $biasreg
      }
   }
}
