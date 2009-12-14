# This is the main program for the frames pipeline
#
ftclHelpDefine photo run_frames_pipeline\
   "runs the Frames Pipeline on a set of frames; "

proc run_frames_pipeline {planfile } {
   global env photoEnv data_root
   global softpars plan defpars
   global TYPE_PIX MASK_TYPE
   global rawnrows rawncols overlap corrows corcols
   global filterlist nfilter ref_color psfilterlist
   global FIELD fieldstat fieldparams
   global ccdpars coadds ccdrow camCol run rerun runstr field
   global verbose diagnostics logfile
   global crc crc_per_field crc_nbyte_max id_values
   global wrote_files
   global max_brightobj

   # get our own e.g. FRAMES_ID
   if [info exists id_values] {
      unset id_values;
   }

   timerStart

   regsub {^~/} $planfile "$env(HOME)/" planfile
   if {[file isdirectory $planfile] && [file exists $planfile/fpPlan.par]} {
      set planfile "$planfile/fpPlan.par"
   }
   #
   # Read and set all the parameters in the default and _then_ the
   # specified planfile, in local scope, and also set this list fpplan.
   #
   # Note that this means that the specified planfile will _override_
   # any parameters set in the default one
   # 
   set defplanfile "[envscan \$PHOTO_DIR]/etc/fpPlan.par"

   # Get the location of the plan file from photoEnv, if provided.
   # Otherwise use "." as the directory containing the plan file.
   if {![regexp {^/} $planfile] && [info exists photoEnv(FRAMES_PLAN)]} {
      set ffile "$photoEnv(FRAMES_PLAN)/$planfile"
      if [file exists "$ffile"] {
	 set planfile "$ffile"
      }
   }
   #
   # Process planfiles
   #
   set cmdline cmdlinePlan
   if [info exists fpplan] { unset fpplan }
   foreach file "defplanfile cmdline planfile cmdline" {
      process_plan fpplan [read_planfile [set $file]]
   }
   #
   # Remove `default' values, and set the plan as variables in this scope
   # (or global, if they so desired).
   #
   # If the value is "nodefault", complain
   #
   foreach var [keylget fpplan] {
      set val [keylget fpplan $var]
      if {$val == "default"} {
	 keyldel fpplan $var
      } elseif {$val == "nodefault"} {
	 error "** keyword $var has value nodefault! **"
      } else {
	 if {[regexp {^display} $var] ||
	     [regexp {_format$} $var] ||
	     [regexp {^truth_} $var] ||
	     [regexp {^(read|write)_(fpC|fpAtlas)_files} $var] ||
	     [regexp {^(fpC|idR)_(zero|scale)$} $var] ||
	     $var == "filterlist" ||
	     $var == "skipFieldList" ||
	     $var == "read_psBB_files" || $var == "psBBDir" ||
	     $var == "compress_image_files" ||
	     $var == "write_test_info" || $var == "allow_old_psField" ||
	     $var == "write_bright_atlas_images" ||
	     $var == "no_measure_objects"} {
	    global $var
	 }
	 
	 set $var $val
      }
   }
   #
   # Process some plan variables
   #
   if [info exists version] {
      if {$version != [photoVersion]} {
	 error [concat \
		    "Plan file specifies version $version, " \
		    "but you are using version [photoVersion]"]
      }
   }

   if {$action != "frames"} {
      error "Found unknown action $action in fpPlan"
   }

   if {$compress_image_files == "0"} {
      set compress_image_files "";
   } elseif {$compress_image_files == "1"} {
      set compress_image_files "R";		# default
   }
   regsub {^[.]} $compress_image_files "" compress_image_files

   set id_values(allow_mismatch) $allow_id_mismatch
   if {$read_fpC_files && $write_fpC_files} {
      error "You may not read and write fpC files"
   }
   #
   # Parse skipFieldList to replace ranges (e.g. 20-21) by values (e.g. 20 21)
   #
   if ![info exists skipFieldList] {
      global skipFieldList; set skipFieldList ""
   }
   if {$skipFieldList != ""} {
      set skipped [list ]
      foreach v $skipFieldList {
	 if [regexp {^([0-9]+)$} $v {} start] {
	    set end $start
	 } elseif {[regexp {^([0-9]+)-([0-9]+)$} $v {} start end]} {
	    ;
	 } else {
	    error "Malformed element of skipFieldList: $v"
	 }
	    
	 loop i $start [expr $end+1] {
	    lappend skipped $i
	 }

	 set skipFieldList $skipped
      }
   }
   #
   # Calculate CRCs and configure memory management
   #
   if [info exists crc] {		# CRCs for all input files
      unset crc
   }
   if [info exists crc_per_field] {
      unset crc_per_field
   }
   calc_crc $defplanfile
   calc_crc $planfile

   if {[info commands memBlocksizeSet] != ""} {
      if [regexp {([0-9]+)k(by)?} $fp_memBlocksize foo fp_memBlocksize] {
	 set fp_memBlocksize [expr $fp_memBlocksize*1024]
      }
      if [regexp {([0-9]+)M(by)?} $fp_memBlocksize foo fp_memBlocksize] {
	 set fp_memBlocksize [expr $fp_memBlocksize*1024*1024]
      }

      if {$fp_memBlocksize != 0} {
	 if {[keylget fpplan memory_reserve] == "0"} {
	    memBlocksizeSet $fp_memBlocksize
	 } else {
	    verb_echo 1 "You may not set fp_memBlocksize as well as memory_reserve; ignoring former"
	 }
      }
   }

   # Expand baseDir before trace_memory. Otherwise see the notes right below this
   # section
   eval set baseDir $baseDir

   #
   # To disable memory reporting, it is sufficient to not define mem. You
   # can and should leave the write_mem calls intact. Note that you can
   # do this be defining trace_memory to be "" or 0 in your plan file, or
   # omitting it altogether. If $trace_memory is "1", write to "mem.trace"
   #
   if {[info exists trace_memory] && \
	   $trace_memory != "" && $trace_memory != 0} {
      eval set trace_memory $trace_memory
      if {$trace_memory == 1} {
	 set trace_memory "mem.trace"
      } 
      global mem
      set mem [open $trace_memory w]
   }
   write_mem mem "starting photo"
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

      verb_echo 5 [format %-15s $dir] = [set $dir]
   }

   if $read_fpC_files {
      set biasDir ""       
   } elseif $read_psBB_files {
      if ![info exists biasDir] {
	 set biasDir $psDir
      }
   }

   if [regexp {^[0-9]+$} $run] {
      set runstr [format %06d $run]
   } else {
      set runstr $run
   }
   
   # Set up the diagnostics file if we are using it
   if {$diagnostics > 0} {
      set logfile [open $outputDir/$diagFile-$runstr-$camCol.par w]
   }
   
   verb_echo 1 "Using $planfile"
   
   # Read the software and hardware parameters, but don't set them.
   # Let the individual procs do that as necessary.
   # Note: some values may also be set at photo level; these are ones
   # that must be the same in frames and psp
   # Also create a CCDPARS with a defectlist for each filter

   verb_echo 3 "$configDir/$ccdConfig"
   verb_echo 3 "$configDir/$ccdECalib"
   verb_echo 3 "$configDir/$ccdBC"
   param2Chain [envscan \$PHOTO_DIR]/etc/fpParam.par defpars
   calc_crc [envscan \$PHOTO_DIR]/etc/fpParam.par

   getDefaultparams_photo defpars;	# get params common to psp/frames
   
   if {[info exists parametersDir] && [info exists parameters] &&
       [file exists $parametersDir/$parameters]} {
      set vlevel 3
      if [regexp {^/} $parameters] {
	 verb_echo $vlevel "$parameters"
      } else {
	 verb_echo $vlevel "$parametersDir/$parameters"
      }

      param2Chain $parametersDir/$parameters softpars
      calc_crc $parametersDir/$parameters

      foreach el [keylget softpars] {
	 if {![keylget defpars $el foo] ||
	     [keylget softpars $el] != [keylget defpars $el]} {
	    lappend changed $el
	 }
      }
   } else {
      set softpars ""
   }
   
   if [info exists changed] {
      if {!$allow_params || $verbose > 0} {
	 set term ":"
      } else {
	 set term ""
      }
      echo "Warning: some parameters are changed from default values$term"
      if {!$allow_params || $verbose > 0} {
	 echo "    $changed"
      }
      if !$allow_params {
	 error \
	     "No parameters may be set as allow_params is false in the plan file"
      }
   } else {
      if $allow_params {
	 error \
	     "allow_params must be true iff you've set some parameters in fpParam"
      }
   }
   #
   # choose the _last_ occurrence of each keyword, in accordance with
   # behaviour of the plan files
   #
   foreach file "defpars softpars" {
      foreach el [set $file] {
	 set key [lindex $el 0]
	 set val [join [lrange $el 1 end]]
	 keylset $file $key $val
      }
   }
   #
   # merge in cmdlineParam
   #
   global cmdlineParam
   if [info exists cmdlineParam] {
      foreach key [keylget cmdlineParam] {
	 if ![keylget defpars $key oval] {
	    error "Unknown command line param $key"
	 }
	 set val [keylget cmdlineParam $key]
	 
	 if {$val != $oval} {
	    verb_echo 1 \
		"Command line parameter changes value of $key from $oval to $val"
	 }
	 keylset softpars $key $val
      }
   }
   #
   # Set param values from the plan file
   #
   if {[keylget fpplan no_overlap no_overlap] && $no_overlap} {
      if {[keylget softpars scan_overlap scan_overlap] &&
	  $scan_overlap != 0} {
	 echo \
	  "You shouldn't specify no_overlap in fpPlan and scan_overlap in fpParam (ignoring scan_overlap)"
      }
      keylset softpars scan_overlap 0
   }
   if [keylget fpplan no_CR_removal] {
      if {[keylget softpars cr_min_sigma cr_min_sigma] &&
	  $cr_min_sigma != 0} {
	 error \
       "You mayn't specify no_CR_removal in fpPlan and cr_min_sigma in fpParam"
      }
      keylset softpars cr_min_sigma 0
   }
   if [keylget fpplan no_search_binned] {
      keylset softpars ffo_binshifts {}
   }
   if [keylget fpplan no_measure_objects] {
      keylset softpars npeak_per_obj_max 0; # not running deblender
   }

   write_mem mem "fetch_ccdpars"
   fetch_ccdpars $configDir $ccdConfig $ccdECalib $ccdBC \
       ccdpars $filterlist $camCol $fpplan
   verb_echo 2 "Finished reading params"
   verb_echo 2 [cpuTime]
   #
   # There's a bug in dervish by which regInfoGet allocates memory the first
   # time that it's called; we call it here to ensure that this happens
   # outside the main photo loop. saoDisplay has a similar problem
   #
   if {[info exists display_filterlist] && $display_filterlist == "all"} {
      set display_filterlist $filterlist
   }
   if 1 {
      global sao
      set reg [regNew 1 1]; regClear $reg
      regInfoGet $reg
      if {$display != 0} {
	 if {![info exists sao] && $display_filterlist != ""} {
	    echo Creating SAODisplays...
	 }
	 foreach f $display_filterlist {
	    if {![info exists sao($f)]} {
	       puts "Display for $f"
	       display $reg "" $f 0 -1
	    }
	    saoReset $sao($f); saoLabel off $sao($f)
	 }
      }
      regDel $reg
   }
   
######################################################################
# Initialize each of the modules
######################################################################
   set nfilter [llength $filterlist]
   set fieldstat [fieldstatNew]

   set fieldparams [fieldparamsNew $filterlist]
   handleSet $fieldparams.run $run;	# can be useful for diagnostics/gdb
   handleSet $fieldparams.camCol $camCol

   set filter_order [getsoftpar filter_order]
   set rpos [lsearch $filter_order [getsoftpar mo_fiber_color]]
   set i -1
   foreach f $filterlist {
      set pos [lsearch $filter_order $f]
      handleSet $fieldparams.frame<[incr i]>.dframe [expr 2*($pos - $rpos)]
   }
   
   meschachInit

   write_mem mem "init_read_frames"
   init_read_frames $filterlist rawimages
   
   init_correct_frames $biasDir biasvector $ffDir flatvector \
       $rawncols $corrows $corcols \
       cr_keep cr_min_sigma cr_min_e cr_cond3 cr_cond32 fullWell

   set ob [object1New]
   set null [handleBindFromHandle [handleNew] *$ob.region]
   object1Del $ob

   set sky_object_size [getsoftpar sky_object_size]
   set nsky_objects [getsoftpar nsky_objects]
   set nsky_failures [getsoftpar nsky_failures]
   array set max_brightobj [getsoftpar max_brightobj]

   set field [getField $camCol $startField]
   check_calibfile \
       [get_psField_filename $psDir $run $camCol $field] $filterlist

   write_mem mem "init_random"
   init_random rand
   init_find_objects $fieldparams $corrows $corcols \
       ffo_levels ffo_median_size ffo_psf_filt_size \
       ffo_binshifts ffo_binned_thresholds

			write_mem mem "init_merge_colors"
   init_merge_colors $fieldparams

			write_mem mem "init_measure"
   init_measure_objects $fieldparams ref_color $rand

   declare_schematrans [llength $filterlist] 

   # set the global "ccdrow" array, which tells which row each filter is in
   foreach f $filterlist {
      set ccdrow($f) [keylget fpplan ccdrow_$f]
   }

###########################################################################
   verb_echo 1 "Starting Frames Pipeline for run $run, column $camCol"
###########################################################################
   write_mem mem "after init procs"
   upvar startMem startMem; set startMem [memNextSerialNumber];
   verb_echo 2 [cpuTime]
   #
   # setup up tcl-level signal handling
   #
   set signal_list [list SIGABRT SIGUSR1]
   foreach sig $signal_list {
      signal trap $sig {error "SIGNAL: caught %S (field $field)"}
   }

   # Loop over each field
   set ret_code 0;			# return code for the parent process
   eval set endField $endField;		# may be `$startField'
   loop field $startField [expr $endField+1] {
      set field [getField $camCol $field];# handle fieldList if defined
      if {$field == ""} {
	 break
      }

      verb_echo 1 "Entering loop for run $run column $camCol field $field"
      verb_echo 2 "memory serial number: " [memNextSerialNumber]
      write_mem mem "start field $field"
      
      declare_schematrans [llength $filterlist] 
      #
      # Reset the random seed, so each field will be repeatable.
      #
      set seed [set_random_seed $rand "field"]

      if {[catch {process_field} err_msg]} {
	 regsub "\n\$" $err_msg "" err_msg;# chop newline

	 set signal ""
	 if {[regexp {Timer expired.*} $err_msg all] ||
	     [regexp {^Almost out of memory} $err_msg all] ||
	     [regexp {^Skipping field$} $err_msg all] ||
	     [regexp {(SIGNAL: caught (SIG[^ ]*).*)} $err_msg foo all signal]} {
	    if {$signal == ""} {
	       if {[regexp {^Almost out of memory} $err_msg all]} {
		  set signal "TOO_LARGE"
	       } elseif {$err_msg == "Skipping field"} {
		  set signal "SKIPPED"
	       } else {
		  set signal "TOO_LONG"
	       }
	    }
	 } elseif {[regexp {Too many bright objects.*} $err_msg all]} {
	    set signal TOO_MANY_BRIGHT_OBJECTS
	 } else {
	    global errorInfo
	    error "$err_msg" $errorInfo
	 }

	 echo "Error: $all; continuing with next field"

	 if {$signal == "SIGABRT"} {
	    set status $FIELD(ABORTED)
	    write_mem mem "Aborting field"
	 } elseif {$signal == "SKIPPED"} {
	    set status $FIELD(SKIPPED)
	    write_mem mem "Field skipped"
	 } elseif {$signal == "TOO_LARGE"} {
	    set status $FIELD(TOO_LARGE)
	    write_mem mem "Tried to use too much memory"
	 } elseif {$signal == "TOO_LONG"} {
	    set status $FIELD(TOO_LONG)
	    write_mem mem "Timer expired"
	 } elseif {$signal == "TOO_MANY_BRIGHT_OBJECTS"} {
	    set status $FIELD(TOO_MANY_BRIGHT_OBJECTS)
	    write_mem mem "Too many bright objects"
	 } else {
	    error "I don't understand signal: $signal"
	 }

	 write_fieldstat $outputDir $field $status

	 set binned [regNew 1 1]; regClear $binned
	 foreach f $filterlist {
	    if ![info exists wrote_files(fpM,$f)] {
	       write_fpM $f $corrimages($f) $outputDir
	    }
	    if ![info exists wrote_files(fpBIN,$f)] {
	       write_fpBIN $f $binned $outputDir brightStarWings objectModels
	    }
	 }
	 regDel $binned

	 memoryErrorIgnore;		# the cleanup code is fragile!
	 catch {
	    fini_measure_objects_field $filterlist $fieldparams \
		$file_objcIo $fd_objcIo $file_aimage $fd_aimage $fd_test_info
	 }

	 loop_reset [memSerialNumber] biasvector corrimages mergedlist \
	     $kos $calibs $fieldstat brightStarWings objectModels
	 
	 set startMem [error_reset $startMem]
	 memoryErrorIgnore -default

	 incr ret_code

	 continue;
      }

      if {$status != $FIELD(OK) && $status != $FIELD(TOO_LONG) && $status != $FIELD(TOO_LARGE)} {
	 incr ret_code
      }

      # Restore to state before entering loop
      verb_echo 2 "end of loop -- about to blow away all allocated stuff "

      verb_echo 2 [cpuTime]

      loop_reset $startMem biasvector corrimages mergedlist \
	  $kos $calibs $fieldstat brightStarWings objectModels

      verb_echo 2 [cpuTime]
      write_mem mem "End of loop"
   }
   # End loop for each field

   foreach sig $signal_list {
      signal default $sig
   }

   fieldstatDel $fieldstat
   fieldparamsDel $fieldparams; unset fieldparams

   # Fini each module
   fini_read_frames $filterlist rawimages
   if !$read_fpC_files {
       fini_correct_frames ccdpars
   }
   fini_find_bright_objects
   fini_random $rand
   fini_find_objects $ffo_levels
   fini_merge_colors
   fini_measure_objects

   fitsBinForgetSchemaTrans NULL;

   # delete other stuff that's left over
   foreach f $filterlist {
      if [info exists biasvector($f)] {
	 regDel $biasvector($f)
      }
      regDel $flatvector($f)
   }
   overlayDelAll

   defragment_memory

   write_mem mem "end of run"
   if [info exists mem] { close $mem }

   if {[getField $camCol -1] != ""} {	# no frameList
      if {![info exists plot_summary] || $plot_summary} {
	 plot_fpstats /PS $startField $endField $outputDir $run $camCol \
	     -rerun $rerun
      }
      if $display_summary {
	 plot_fpstats /XWINDOW $startField $endField $outputDir $run $camCol \
	     -rerun $rerun
      }
   }
   verb_echo 1 "Completed OK"
   if {[info exists logfile] && $logfile != ""} {
      close $logfile
   }
 
   return $ret_code
}

#
# Actually process a field
#
proc process_field {} {
   uplevel {
      timerSet [keylget fpplan time_limit] -setvar time_left; global time_left

      set memory_limit [keylget fpplan memory_limit]
      set memory_reserve [keylget fpplan memory_reserve]

      if {$memory_limit != "0"} {
	 verb_echo 2 "Limiting memory footprint to $memory_limit"
      }
      if {$memory_reserve != "0"} {
	 verb_echo 2 "Holding back $memory_reserve bytes"
      }
      memoryMaxSet $memory_limit
      strategicMemoryReserveSet $memory_reserve
      
      handleSet $fieldparams.fieldnum $field

      set fieldstr [format %04d $field]
      #
      if [info exists wrote_files] {
	 unset wrote_files
      }
      
      set kos [chainNew KNOWNOBJ]
      if [info exists koFile_format] {
	 if [info exists koDir] {
	    if {$koDir != "/dev/null"} {
	       chainDel $kos; set kos [read_knownobjs $koDir/[eval format $koFile_format]]
	    }
	 } else {
	    error "You must specify koDir if you set koFile_format"
	 }
      }
      #
      # Is this a coadd? If so, read input file giving effective sky/gain 
      #
      eval set idCoadds $idCoadds
      init_coadds coadds $configDir $idCoadds $run $camCol $filterlist
      #
      # Let us easily raise the detection threshold on all the ffo passes (unbinned + binned).
      #
      set ffo_factor 1
       
      set psFieldFile [get_psField_filename $psDir $run $camCol $field]

      set calibs [read_calib1byframe $field $psFieldFile $filterlist]
      read_trans $calibs \
	  [get_trans_filename $transFileDir $run "" $camCol] $camCol $filterlist
      #
      # Read variable PSF data
      #
      read_psfBasis psfBasis $psFieldFile $filterlist
      #
      # loop over each filter, reading from disk, flat fielding,
      # and finding bright objects
      #
      set status $FIELD(OK)
      
      foreach f $filterlist {
	 verb_echo 3 "Filter $f"
	 set ifilter [lsearch $filterlist $f]

	 write_mem mem "Reading $f"

	 # Read in next frame
	 if [catch {
	    read_frames $f $rawimages($f) corrimages($f) biasvector($f) $imageDir
	 } msg] {
 	    if {$field != $startField && $field == $endField &&
 		![regexp {main body} $msg]} {
	       set status $FIELD(MISSING)
	       write_fieldstat $outputDir $field $status
	       foreach ff $filterlist {
		  if [info exists corrimages($ff)] {
		     catch {
			if {[handleType $corrimages($ff)] == "REGION"} {
			   regDel $corrimages($ff)
			}
		     }
		     unset corrimages($ff)
		  }

		  foreach s "skyreg skySigreg" {
		     if [info exists ${s}($ff)] {
			catch {
			   binregionDel [set ${s}($ff)]
			}
			unset ${s}($ff)
		     }
		  }

		  if [info exists psfBasis($ff)] {
		     catch {
			psfBasisDel $psfBasis($ff)
		     }
		     unset psfBasis($ff)
		  }
	       }
	       verb_echo 0 "recovering from missing frame"
	       write_mem mem "recovering from missing frame"
	       timerSet 0 -unsetvar time_left;	# stop timer
	       return
	    } else {
	       error $msg
	    }
	 }
	 verb_echo 3 [cpuTime]

	 write_mem mem "flatfielding $f"

	 if {[info exists display_raw] && $display_raw && !$read_fpC_files} {
	    display $rawimages($f) "raw frame" $f 1 1 "plot_ccdpars ccdpars $f"
	 }

	 # Flatfield the frame; reset the saturation level first
	 if ![info exists fpCDir] {
	    set fpCDir $outputDir
	 }

	 handleSet $fieldparams.frame<$ifilter>.fullWell<0> $fullWell(0,$f)
	 handleSet $fieldparams.frame<$ifilter>.fullWell<1> $fullWell(1,$f)
	 correct_frames $f $rawimages($f) $corrimages($f) $biasvector($f) \
		 $flatvector($f) $calibs skyTypical skySigma skyErr $fpCDir \
		 $cr_keep $cr_min_sigma $cr_min_e $cr_cond3 $cr_cond32 \
	         $psfBasis($f)
	 verb_echo 3 [cpuTime]
	 assert {[exprGet (int)$corrimages($f).type]==$TYPE_PIX}

	 if {[info exists display_raw] && $display_raw} {
	    catch { global sao; saoReset $sao($f) }
	    display $corrimages($f) "corrected frame" $f 1
	 }
	 if 0 {
	    #
	    # Subtract a constant sky
	    #
	    set isky [expr int($skyTypical($f))]
	    regIntConstAdd $corrimages($f) -$isky
	    
	    set dsky [expr $skyTypical($f) - $isky]
	    handleSet $fieldparams.frame<$ifilter>.bkgd $isky
	    set skyreg($f) [binregionNewFromConst $dsky]
	 } else {
	    set skyreg($f) [binregionNewFromConst $skyTypical($f)]
	 }
	 set skySigreg($f) [binregionNewFromConst $skyErr($f)]

	 skyEstimateDebias -clipped $skyreg($f) \
	     [exprGet $fieldparams.frame<$ifilter>.gain0] \
	     [exprGet $fieldparams.frame<$ifilter>.gain1]
      }
      #
      # Now that we have the corrected images, initialise measure objects
      # for this field
      #
      # make header for output objc table
      set bintable_hdr [make_table_header $seed]

      write_mem mem "init_measure_objects_field"
      init_measure_objects_field obj_id corrimages skyreg skySigreg \
	  psfBasis $calibs $fieldparams $fieldstat \
	  $outputDir $runstr-$camCol-$fieldstr $bintable_hdr \
	  file_objcIo fd_objcIo file_aimage fd_aimage fd_test_info
      hdrFreeAll $bintable_hdr; hdrDel $bintable_hdr
      #
      # Do we want to skip this field?
      #
      if [skip_field $field] {
	 error "Skipping field"
      }
      #
      # Find Bright Objects
      #
      foreach f $filterlist {
	 set ifilter [lsearch $filterlist $f]
	       
	 write_mem mem "bright objects $f"
	 set brightlists($f) [find_bright_objects $f $corrimages($f) \
				  $skySigma($f)]
	 handleSet $fieldstat.nbrightobj<$ifilter> [chainSize $brightlists($f)]
	 verb_echo 3 [cpuTime]
	 #
	 display $corrimages($f) "bright objects" $f 1 0 "show_object_list $f $brightlists($f)"
	 
	 if {$time_left < 0} {
	    error "Timer expired: after find_bright_objects $f"
	 }
	 if [strategicMemoryReserveIsEmpty] {
	    error "Almost out of memory: after find_bright_objects $f"
	 }
      }

      #
      # Avoid reducing fields which have too many stars to expect to reduce well.
      # This mechanism is not expected to be the final one.
      foreach f $filterlist {
	 set thresh $max_brightobj($f)
	 if {$thresh > 0 && [chainSize $brightlists($f)] >= $thresh} {
	     echo "Many bright objects, scaling ffo_levels from $ffo_levels"

	     set ffo_factor 50
	 }
      }

      #
      # Merge the bright object list
      #
      write_mem mem "merging bright objects"

      merge_colors brightlists $fieldparams mergedlist $rand -cut_atlas

      foreach f $filterlist {
	 chainDel $brightlists($f)
      }
      unset brightlists
      #
      # Peak up the astrometry between the different bands
      #
      find_astrom_offsets $mergedlist $nfilter $fieldparams $fieldstat
      #
      # Measure at least some of the bright objects
      #
      if !$no_measure_objects {
	 write_mem mem "measuring bright objects"
      }

      if {$display && $display_measure_bright} {
	 foreach f $filterlist {
	    display $corrimages($f) "Measuring bright objects" $f 1 0
	 }
      }

      if [getsoftpar mo_fit_models] {
	 set model_fit ""
      } else {
	 set model_fit "-no_model"
      }
      set subtract_cts [getsoftpar subtract_counts]; # subtract objects with
      ;						     # at least this many counts
      set subtract_npixel_min [getsoftpar subtract_npixel_min]; # subtract objects with
      ;						     # at least this many pixels
      set subtract_stars [getsoftpar subtract_stars]; # subtract bright stars (i.e. don't reinstate them)
      #
      # Make a copy of mergedlist with initial pixels --- n.b. DEBLENDED_AT_EDGE
      # can modify the atlas images
      #
      set mergedlist0 [chainNew OBJC]
      loop i 0 [chainSize $mergedlist] {
	 set objc_c [objcNewFromObjc [chainElementGetByPos $mergedlist $i] -deep -copy_ai]
	 chainElementAddByPos $mergedlist0 $objc_c TAIL
      }
      
      set subtract_chain \
	  [measure_bright_objects obj_id $mergedlist $subtract_cts $subtract_npixel_min\
	       $fd_objcIo $fd_aimage $fd_test_info \
	       $fieldparams $fieldstat skyTypical $model_fit]
      #
      # Reinstate all of the objects
      #
      loop i 0 [chainSize $mergedlist0] {
	 set objc [chainElementGetByPos $mergedlist0 $i]
	 insertAtlasImage $objc $fieldparams
	 handleDel $objc
      }
      objcChainDel $mergedlist0

      if 0 {				# XXX
	 foreach f $filterlist {
	    display $corrimages($f) "After restoring objcs after measure bright objects" $f 1 0
	 }
      }
      #
      if 0 {
	 subtract_star_wings starWings corrimages $mergedlist $fieldparams $rand
	 
	 foreach f $filterlist {
	    display $corrimages($f) "After subtracting star wings" $f 1 0
	 }
      }
      #
      # Destroy the initial estimates of the sky and sky sigma. We'll
      # find a better value in find faint objects
      #
      foreach f $filterlist {
	 set ifilter [lsearch $filterlist $f]

	 binregionDel *$fieldparams.frame<$ifilter>.sky
	 handleSet $fieldparams.frame<$ifilter>.sky 0x0

	 binregionDel *$fieldparams.frame<$ifilter>.skyErr
	 handleSet $fieldparams.frame<$ifilter>.skyErr 0x0
      }
      #
      # Time to find the faint objects
      #
      verb_echo 2 "Finding objects in unbinned data"

      foreach f $filterlist {
	 set ifilter [lsearch $filterlist $f]
	 # Find Faint Objects
	 # bind scratch buffers
	 overlayGet reg_scr $corrows $corcols
	 overlayGet smoothed $corrows $corcols
      
	 write_mem mem "faint objects $f"
      
	 set psfsigma [exprGet $calibs.calib<$ifilter>->psf->sigma1_2G]
      
	 set tlevels [vectorExprEval "$ffo_levels * $ffo_factor"]
	 set faintlists($f) \
	     [find_objects $f $smoothed $corrimages($f) \
		  $reg_scr $rand $tlevels \
		  $ffo_median_size $ffo_psf_filt_size $psfsigma \
		  -subtract $subtract_chain -objectModels objectModels -subtract_stars $subtract_stars]
	 vectorExprDel $tlevels
      	 #
	 # release find-objects scratch buffers
	 #
	 overlayRelease reg_scr; overlayRelease smoothed
	 handleSet $fieldstat.nfaintobj<$ifilter> [chainSize $faintlists($f)]
	 verb_echo 3 [cpuTime]
      
	 display $corrimages($f) "faint objects" $f 1 0 "show_object_list $f $faintlists($f)"

	 if {$time_left < 0} {
	    error "Timer expired: after find_objects $f"
	 }
	 if [strategicMemoryReserveIsEmpty] {
	    error "Almost out of memory: after find_objects $f"
	 }
      }
      chainDel $subtract_chain; unset subtract_chain
      
      # Merge the faint objects with the mergedlist of bright objects

      write_mem mem "merge"

      merge_colors faintlists $fieldparams mergedlist $rand -cut_atlas -no_restore_ai
      foreach f $filterlist {
	 chainDel $faintlists($f)
      }
      unset faintlists
      
      #
      # Bin the image, and look for faint objects again
      #
      set binshift_max [lrange [lsort -real $ffo_binshifts] end end]
      foreach binshift $ffo_binshifts {	# lg_2(binning factor)
	 set binfac [expr 1<<$binshift]
	 verb_echo 2 "Finding objects in data binned ${binfac}x${binfac}"

	 set breg "NULL"
	 foreach f $filterlist {
	    set ifilter [lsearch $filterlist $f]
	    write_mem mem "faint objects binned ${binfac}x${binfac} $f"
	    
	    set dcoord [offsetDo $fieldparams \
			    [expr $corrows/2] [expr $corrows/2] \
			    [exprGet $fieldparams.ref_band_index] $ifilter]
	    foreach rc "row col" {
	       if {$rc == "row"} { set i 0 } else { set i 2 }
	       set dd [lindex $dcoord $i]
	       set dd [expr "$dd<0 ? -int(-$dd+0.5) : int($dd+0.5)"]
	       
	       set ${rc}0 $dd
	    }
	    verb_echo 3 Binning $f ${binfac}x$binfac: row0,col0 = $row0, $col0
	    set offmax 100
	    if {abs($row0) > $offmax || abs($col0) > $offmax} {
	       foreach ad "a d" {
		  set ii -1
		  foreach if "$ifilter [exprGet $fieldparams.ref_band_index]" {
		     set $ad[incr ii] \
			 [exprGet $fieldparams.frame<$if>.toGCC->$ad]
		  }
	       
		  set d$ad [expr [set ${ad}1] - [set ${ad}0]]
	       }

	       error [format "Apparent abs(offset) between colors is > $offmax; probable astrom problem.\n   Trans: delta(a) = %.3f, delta(d) = %.3f (%.3f, %.3f pixels)" \
			  $da $dd [expr 3600*$da/0.400] [expr 3600*$dd/0.400]]
	    }

	    set soft_bias [softBiasGet]
	    set foo [regIntBin $breg $corrimages($f) $binfac $binfac \
			  -rshift $binshift -bkgd $soft_bias \
			  -addcon [expr -($binfac-1)*$soft_bias] \
			  -row0 $row0 -col0 $col0]
	    if {$breg == "NULL"} {
	       set breg $foo
	    }
	    if {![info exists binned]} {
	       set binned [handleBindFromHandle [handleNew] *$breg.reg]
	       #
	       # N.b. NOTCHECKED isn't set correctly; cf. PR 3094
	       #
	       set mask [spanmaskNew \
			     -nrow [exprGet $binned.nrow] \
			     -ncol [exprGet $binned.ncol]]
	       handleSetFromHandle $binned.mask (MASK*)&$mask; handleDel $mask
	    }
	    hdrFree $binned.hdr
	    #
	    # Now run the object finder on the binned image
	    #
	    overlayGet reg_scr [exprGet $binned.nrow] [exprGet $binned.ncol]
	    overlayGet smoothed [exprGet $binned.nrow] [exprGet $binned.ncol]
	    
	    set psfsigma [exprGet $calibs.calib<$ifilter>->psf->sigma1_2G]
	    set binnedlists($f) \
		[find_objects $f $smoothed $breg $reg_scr $rand \
		     [expr [set ffo_binned_thresholds_$binshift] * $ffo_factor]\
		     0 $ffo_psf_filt_size $psfsigma]
	    spanmaskSetFromObject1Chain $binnedlists($f) \
		*$corrimages($f).mask $MASK_TYPE(BINOBJECT)

	    if {[info exists display_binned] && $display_binned} {
	       display $corrimages($f) \
		   "binned objects ${binfac}x${binfac}" $f -1
	    }

	    overlayRelease reg_scr; overlayRelease smoothed
	    #
	    # write to disk the corrected frame, its associated defect mask,
	    # and the binned image the _last_ time through
	    #
	    if {$binshift == $binshift_max} {
	       hdrCopy $corrimages($f).hdr $binned.hdr
	       hdrInsWithInt $binned.hdr "BINFAC" $binfac \
		   "Amount image is binned by"
	       hdrInsWithInt $binned.hdr "SHIFTED" $binshift \
		   "Number of bits image is scaled up"
	       hdrInsWithInt $binned.hdr "ROW0" $row0 \
		   "Row origin of binned image"
	       hdrInsWithInt $binned.hdr "COL0" $col0 \
		   "Column origin of binned image"
	       hdrInsWithInt $binned.hdr "CSIZE" \
		   [exprGet $corrimages($f).ncol] "ncol for original image"
	       hdrInsWithInt $binned.hdr "RSIZE" \
		   [exprGet $corrimages($f).nrow] "nrow for original image"
	       hdrInsWithInt $binned.hdr "SOFTBIAS" [softBiasGet] \
		   "software \"bias\" added to all DN"

	       write_fpBIN $f $binned $outputDir brightStarWings objectModels
	    }
	 }

	 set mask [handleBindFromHandle [handleNew] *$binned.mask]
	 spanmaskDel $mask
	 handleSet $binned.mask 0x0
	 handleDel $binned; unset binned
	 binregionDel $breg
	 
	 # Merge the binned objects with the mergedlist 
	 merge_colors binnedlists $fieldparams mergedlist $rand -cut_atlas
	 foreach f $filterlist {
	    chainDel $binnedlists($f)
	 }
	 unset binnedlists
      }
      #
      # Maybe merge in an extra list of interesting positions,
      # as if we'd detected them. Useful for running the deblender
      # for doing science (shhhh)
      #
      merge_extra_peaks $extra_peaks_file $fieldparams mergedlist $rand
	 
      verb_echo 3 [cpuTime]

      if {$display && $display_merged} {
	 foreach f $display_filterlist {
	    show_object_list $f $mergedlist
	 }
      }

      write_mem mem "known objects"
      # Make OBJCs for KNOWNOBJs so we can measure them
      set kobjcs [cutKnownObjects $kos $fieldparams]

      set filter [lindex $filterlist [exprGet $fieldparams.ref_band_index]]
      if {[info exists sao($filter)] && [info exists display_ko] && $display_ko} {
	 set curs [chainCursorNew $kobjcs]
	 while {[set objc [chainWalk $kobjcs $curs]] != ""} {
	    draw_objs $objc $filter
	    handleDel $objc
	 }
	 chainCursorDel $kobjcs $curs
      }
      
      merge_known_objects $mergedlist $kobjcs $rand
      #
      # Write out masks
      #
      foreach f $filterlist {
	 write_fpM $f $corrimages($f) $outputDir
      }      	    
      #
      # Make OBJCs for skyobjects too
      #
      skyObjectsFind $nsky_objects $mergedlist $nfilter \
       [exprGet $fieldparams.ref_band_index] $corrows $corcols \
	  -nfail $nsky_failures -size $sky_object_size
      #
      # Maybe re-estimate the sky now that the objects are masked
      #
      if [getsoftpar refit_sky_post_objects] {
	 foreach f $filterlist {
	    set i [lsearch $filterlist $f]
	    regStatsFromQuartiles $corrimages($f) \
		-coarse 2 -mean mean -median med -iqr iqr
	    set omean [expr $mean-$soft_bias]
	    set omed [expr $med-$soft_bias]
	    set osigma [expr 0.7413*$iqr]

	    handleSet $fieldstat.sky_frames_sub<$i> \
		[expr [exprGet $fieldstat.sky_frames_sub<$i>] + $med]
	    
	    set nbit 10;		# number of bits to shift sky
	    set filt_c $ffo_median_size
	    set filt_r $ffo_median_size
	    
	    set skySigreg($f) [binregionNew]
	    set bsm [medianSky $corrimages($f) $filt_c $filt_r -shift $nbit\
			 -skysig $skySigreg($f)];
	    if 0 {
	       skyEstimateDebias -clipped $bsm \
		   [exprGet $fieldparams.frame<$i>.gain0] \
		   [exprGet $fieldparams.frame<$i>.gain1]
	    }
	    regIntConstAdd *$bsm.reg [expr -$soft_bias*(1<<$nbit)];
	    skySubtract $corrimages($f) $corrimages($f) $bsm $rand
	    
	    set skyreg($f) $bsm
	    
	    regStatsFromQuartiles $corrimages($f) \
		-coarse 2 -mean mean -median med -iqr iqr
	    set mean [expr $mean-$soft_bias]
	    set med [expr $med-$soft_bias]
	    set sigma [expr 0.741301*$iqr]
	    
	    verb_echo 2 \
		[format [concat "$f: med = %5.2f mean = %5.2f sig = %.2f" \
			     "  nmed = %5.2f nmean = %5.2f nsig = %.2f"] \
		     $omed $omean $osigma $med $mean $sigma]
	    
	    binregionDel $skyreg($f); binregionDel $skySigreg($f);
	 }

	 if 0 {
	    display $corrimages($f) "After second sky-subtraction" $f 1
	 }
      }
      #
      # Time to measure.
      #
      if 0 {				# atlas images are removed and masked
	 foreach f $filterlist {
	    display $corrimages($f) "masked objects" $f 1
	 }
      }

      if !$no_measure_objects {
	 write_mem mem \
	     "measuring [exprGet $mergedlist.nElements] parent objects"
      }
      if [getsoftpar mo_fit_models] {
	 set model_fit ""
      } else {
	 set model_fit "-no_model"
      }
      
      measure_objects obj_id $mergedlist $fd_objcIo $fd_aimage $fd_test_info \
	  $fieldparams $fieldstat $model_fit
      write_mem mem "total nobj: $obj_id"

      if 0 {				# frames after finishing processing
	 loop i 0 $nfilter {
	    display *$fieldparams.frame<$i>.data \
		"after measure objects" [lindex $filterlist $i] 1
	 }
      }

      if {$display < 0} {		# take a look at measured objects
	 puts -nonewline [format "%-50s" "Finished frame $field    "]
  	 puts -nonewline "hit return to continue  " ;
	 if {[gets stdin] == "q"} {
	    if {$field < $endField} {
	       echo "quitting"
	       set endField $field
	    }
	 }
      }

      fini_measure_objects_field $filterlist $fieldparams \
	  $file_objcIo $fd_objcIo $file_aimage $fd_aimage $fd_test_info

      verb_echo 3 [cpuTime]

      # Write out remaining results
      write_fieldstat $outputDir $field $status
      write_mem mem "restoring"
   }
   timerSet 0 -unsetvar time_left;	# stop timer

   return
}
