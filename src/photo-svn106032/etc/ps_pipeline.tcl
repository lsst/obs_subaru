ftclHelpDefine photo run_ps_pipeline \
    "    runs the Postage-Stamp Pipeline on a set of frames

    Usage: run_ps_pipline planfile
    "
   #############################################################   
   ## This is the main program for the postage stamp pipeline ##
   #############################################################

proc run_ps_pipeline {planfile} {
   upvar startMem startMem 
   global env photoEnv data_root
   global PSP_STATUS STAR1
   global softpars hardpars psplan ccdpars defpars QApars defQApars
   global camCol run rerun frame outputDir startRun endRun startField endField
   global verbose diagnostics logfile stampsize
   global crc crc_nbyte_max
   global id_values Npatches
   global filterlist nfilter 
   global schemaOK
   global psfKLbasis_nframe psfKLcoeffs_nframe
   global nrowperframe ncolperframe

# ======================================================================
# ===================  0) Preliminary stuff  ===========================
# ======================================================================
   regsub {^~/} $planfile "$env(HOME)/" planfile

   # for measuring execution time
   timerStart

   # for  memory reporting
   set startMem [memNextSerialNumber]

   # random numbers
   set seed  819396102   
   if ![info exists rand] {
      set rand [phRandomNew 100000 -seed $seed -type 2]
   }

   # get our own e.g. PS_ID
   if [info exists id_values] {
      unset id_values;
   }

   meschachInit

# ======================================================================
# ===================  1) Read input data ==============================
# ======================================================================

   #
   # Read and set all the parameters in the default and _then_ the
   # specified planfile, in local scope, and also set this list fpplan.
   #
   # Note that this means that the specified planfile will _override_
   # any parameters set in the default one
   # 

   set defplanfile "[envscan \$PHOTO_DIR]/etc/psPlan.par"

   if ![info exists photoEnv(PS_PLAN)] {
       set photoEnv(PS_PLAN) ""
   }
   if {$photoEnv(PS_PLAN) != ""} {
      set planfile $photoEnv(PS_PLAN)/$planfile
   }

   foreach v "startRun endRun" {
      if [info exists $v] { unset $v }
   }

   if [info exists psplan] { unset psplan }
   set cmdline cmdlinePlan
   foreach file "defplanfile planfile cmdline" {
      verb_echo 3 "   processing input file [set $file]"
      process_plan psplan [read_planfile [set $file]]
   }
   #
   # Remove `default' values, and set the plan as variables in this scope
   # (or global, if so declared above)
   #
   foreach var [keylget psplan] {
      set val [keylget psplan $var]
      if {$val == "default"} {
	 keyldel psplan $var
      } elseif {$val == "nodefault"} {
	 error "** keyword $var has value nodefault! **"
      } else {
	 if [regexp {^(display.*|.*_format)$} $var] {
	    global $var
	 }

	 set $var $val
      }
   }

   if {[info exists startRun] || [info exists endRun]} {
      ### we no longer support running in chunks
      echo "  *** startRun and endRun are obsolete ***"
      echo "   setting startRun to startField=$startField"
      echo "   setting endRun to endField=$endField"
   }
   set startRun $startField
   set endRun $endField
 
   # sanity checks on run limits
   if {$startField < $startRun} {
      error "startField (=$startField) < startRun (=$startRun) !?"            
   }  
   if {$endField > $endRun} {
      error "endField (=$endField) > endRun (=$endRun) !?"      
   }  
   if {$startField > $endField} {
      error "startField (=$startField) > $endField (=$endField) !?"            
   }   

   if {$ignore_PSF && $write_psFang} {
      error "You may not specify write_psFang if you've set ignore_PSF"
   }
   if {$fakePSF && $write_psFang} {
      error "You may not specify write_psFang if you've set ignore_PSF"
   }

   #
   # Calculate CRCs and configure memory management
   #
   if [info exists crc] {		# CRCs for all input files
      unset crc
   }
   calc_crc $defplanfile
   calc_crc $planfile

   # control avoid memory fragmentation
   if {$psp_memBlocksize != "0" && $psp_memBlocksize != ""} {
      if [regexp {([0-9]+)k(by)?} $psp_memBlocksize {} psp_memBlocksize] {
	 set psp_memBlocksize [expr $psp_memBlocksize*1024]
      }
      if [regexp {([0-9]+)M(by)?} $psp_memBlocksize {} psp_memBlocksize] {
	 set psp_memBlocksize [expr $psp_memBlocksize*1024*1024]
      }

      memBlocksizeSet $psp_memBlocksize
   }
#  - bug in dervish
   # There's a bug in dervish by which regInfoGet allocates memory the first
   # time that it's called; we call it here to ensure that this happens
   # outside the main photo loop. saoDisplay has a similar problem
   #
   if 1 {
      global sao
      set reg [regNew 1 1]; regClear $reg
      regInfoGet $reg
      if {$display != 0} {
	 if {![info exists sao(default)]} {
 	    echo " Creating SAODisplay..."
 	    display $reg "" default 0 -1
	 }
      }
      regDel $reg
   }
#
#  Expand all the directory names. If any are omitted, use $defaultDir.
# Note that all names are expanded, so they can contain a variable, e.g.:
# baseDir	/u/rhl/data
# run		745
# defaultDir	$baseDir/$run
#
   eval set baseDir $baseDir
   foreach dir "bias config fang ff idFF mtPatch output outputFang parameters photomFile transFile trueflat" {
      set dir ${dir}Dir

      if ![info exists $dir] {
	 set $dir $defaultDir
      }
      eval set $dir [set $dir]
      
      set $dir [envscan [set $dir]]
   }

   set transFile [get_trans_filename $transFileDir $run "" $camCol]

   foreach f "psFieldOld" {		# expand plan file entries
      eval set $f [set $f]
   }
   #
   # To disable memory reporting, it is sufficient to not define mem. You
   # can and should leave the write_mem calls intact. Note that you can
   # do this be defining trace_memory to be "" or 0 in your plan file, or
   # omitting it altogether. If $trace_memory is "1", write to "mem.trace"
   #
   if {[info exists trace_memory] && \
	   $trace_memory != "" && $trace_memory != 0} {
      if {$trace_memory == 1} {
         set runstr [format %06d $run]
         set trace_memory $outputDir/mem.$runstr-col$camCol.trace
         set mem [open $trace_memory w]
      }
   }
   write_mem mem "starting PSP"
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

   if {$action != "ps"} {
      error "Found unknown action $action in psPlan"
   }
   
   set softpars ""
   set QApars ""

   #  set up the diagnostics file if we are using it
   set runstr [format %06d $run]
   if {[info exists diagnostics] && $diagnostics > 0} {
      set logfile [open $outputDir/$diagFile-$runstr-$camCol.par w]
   } else {
      set logfile ""
   }
   verb_echo 1 "   PHOTO VERSION: [photoVersion]"
   verb_echo 1 "   Plan file: $planfile"

   set softpars ""; set QApars ""
   if {[info exists parametersDir]} {
      if {[info exists parameters] &&
	  [file exists $parametersDir/$parameters]} {
	 param2Chain $parametersDir/$parameters softpars
	 calc_crc $parametersDir/$parameters
      }
      
      if {[info exists QAparameters] &&
	  [file exists$parametersDir/$QAparameters]} {
	 param2Chain $parametersDir/$QAparameters QApars
	 calc_crc $parametersDir/$QAparameters
      } 
   }
 
   # Read the software and hardware parameters, but don't set them.
   # Let the individual procs do that as necessary.
   # Note: some values may also be set at photo level; these are ones
   # that must be the same in frames and psp
   # Also create a CCDPARS with a defectlist for each filter

   param2Chain [envscan \$PHOTO_DIR]/etc/psParam.par defpars
   param2Chain [envscan \$PHOTO_DIR]/etc/psQApar.par defQApars
   calc_crc [envscan \$PHOTO_DIR]/etc/psParam.par
   calc_crc [envscan \$PHOTO_DIR]/etc/psQApar.par

   getDefaultparams_photo defpars;	# get params common to psp/frames

   verb_echo 3 "   Default software parameters are from:"
   verb_echo 3 "   [envscan \$PHOTO_DIR]/etc/psParam.par"    

   # choose the _last_ occurrence of each keyword, in accordance with
   # behaviour of the plan files
   foreach file "defpars softpars" {
      foreach el [set $file] {
	 set key [lindex $el 0]
	 set val [join [lrange $el 1 end]]
	 keylset $file $key $val
      }
   }
   foreach file "defQApars QApars" {
      foreach el [set $file] {
	 set key [lindex $el 0]
	 set val [join [lrange $el 1 end]]
	 keylset $file $key $val
      }
   }
   #
   # See if any software parameters have been changed
   #
   foreach el [keylget softpars] {
      if {![keylget defpars $el foo] ||
	  [keylget softpars $el] != [keylget defpars $el]} {
	 lappend changed $el
      }
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
	  "allow_params must be true iff you've set some parameters in psParam"
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
		"Command line parameter $key is changed from $val to $oval"
	 }
	 keylset softpars $key $val
      }
   }
   # some psParam entries can be set from the psPlan file
   if [keylget psplan constantPSF] {
      if {[keylget softpars ncomp ncomp] &&
	  $ncomp != 1} {
	 error \
       "You mayn't specify constantPSF in psPlan and ncomp in psParam"
      }
      keylset softpars ncomp 1
      keylset softpars nrow_b 1
      keylset softpars ncol_b 1
   }

   set scan_overlap [keylget defpars scan_overlap]

   write_mem mem "config files"
   # read config files
   fetch_ccdpars $configDir $ccdConfig $ccdECalib $ccdBC \
       ccdpars $filterlist $camCol $psplan

   # number of rows per frame
   set nrowperframe [exprGet $ccdpars([lindex $filterlist 0]).nDataRow]     
   set ncolperframe [exprGet $ccdpars([lindex $filterlist 0]).nDataCol]     

   set stampsize [getsoftpar ps_stampsize]

   foreach filter $filterlist { 
       assert {$nrowperframe == [exprGet $ccdpars($filter).nDataRow]}  
       assert {$ncolperframe == [exprGet $ccdpars($filter).nDataCol]}  
   }
   #
   # Read aperture correction param file
   #
   if [getsoftpar use_calculated_ap_corr_run] {
      set apCorrFile [envscan \$PHOTO_DIR]/etc/[keylget psplan apCorrFile]

      global apCorrCoeffs
      read_apCorrCoeffs apCorrCoeffs $apCorrFile
   }

   # take care of the display flags
   if {![info exists display]} {set display 0}

   # FSAO displays
   foreach q {raw psf PSFstars wing symmetry flat KLpsf} {
      if {![info exists display_$q] || $display == 0} {
           set display_$q 0
      }
   }
   # PGPLOT displays
   foreach q {bias clipping rejection profiles summary} {
      if {![info exists display_$q]} {
           set display_$q 0
      }
   }

   # are we ignoring PSF
   if {[info exists fakePSF] && $fakePSF} {
      verb_echo 1 "   ### faking PSF!!! ###"
   }
   # are we ignoring flatfields
   if {$ignore_FF} {
       verb_echo 1 "   ** ignoring flat field determination **"
   }
   # are we ignoring photometric calibration
   if {$ignore_calibration} {
       verb_echo 1 "   ** ignoring PT calibration **"
   }
   # are we ignoring PSF determination
   if {$ignore_PSF} {
       verb_echo 1 "   ** ignoring PSF determination **"
   }
   # are we ignoring wing stars
   if {$ignore_wing_stars} {
       verb_echo 1 "   ** ignoring wing stars **"
   }
   # are we ignoring WC
   if {$ignore_wing_stars} {set ignore_WC 1}
   if {$ignore_WC} {
       verb_echo 1 "   ** ignoring wing correction (WC) **"
   }
   # are we ignoring plots
   if {$ignore_plots} {
       verb_echo 1 "   ** ignoring all plots **"
   }
   # are we ignoring PSF determination by forcing the deafault
   # from psPlan
   if {$forceDefaultPsf} {
       verb_echo 1 "   ** Forcing Default PSFs **"
   }



   ### fixing the scattered light problem
   if {$fix_background} {
       verb_echo 1 "   ** requested to fix the scattered light **"
       ### depending on idFFiter, use idFF files or input psFF files
       set idFFiter [envscan $idFFiter]
       if {$idFFiter > 0} {
	  assert { $trueflatDir == {} }
          ## new (post-Feb '03) flats from the given iteration of idFF files
          set ffDir $idFFDir
          if {[check_file $ffDir "Directory with idFF files"]} {
             return -1
          }
          verb_echo 1 "    idFF files from $ffDir "
          foreach filter $filterlist {
              if {$runFlatfield == "defaultFFseasons"} {
                  set season [getFFseasonNumberFromRun $run] 
              } else {
                  # forced to assume other season
                  set season $runFlatfield
              }
              set iterSeason [format "%03d" $idFFiter][format "%03d" $season]
              set trueflat($filter) [get_idFF_filename $ffDir $iterSeason $filter $camCol]
              if {[check_file $trueflat($filter) "idFF*fit file for $filter band"]} {
                 return -1
              }
          }           
       } else {
          ## old (pre-Feb '03) flats from external psFF files
          # do we have default seasons, if so get the right run number
          if {$runFlatfield == "defaultFFseasons"} {
             set runFlatfield [getFFseasonRunNo $run]
          }
          set trueflatDir [envscan $trueflatDir]
          set ffDir $trueflatDir 
          if {[check_file $ffDir "Directory with true flatfield vectors"]} {
             return -1
          }
          verb_echo 1 "    true flats from $ffDir "
          foreach filter $filterlist {
              set trueflat($filter) [get_ff_filename $ffDir $runFlatfield $filter $camCol]
              if {[check_file $trueflat($filter) "True flatfield vector for $filter band"]} {
                 return -1
              }
          } 
       }     
       set SLsmoothNfield [getsoftpar SLsmoothNfield]
       # since we are fixing background, makes no sense to use uncorrected bias
       set read_psBB_files 1
   }

   # testing the scattered profile
   if {$test_SLprofile} {
       verb_echo 3 "   ** requested to test the scattered light profile **"
       set SLDir [envscan $SLDir]
       if {[check_file $SLDir "Directory with scattered light profiles"]} {
          return -1
       }
       verb_echo 3 "    scattered light profiles from $SLDir "
       set file "$SLDir/"
       foreach filter $filterlist {
           set SLtestfile($filter) $file 
           append SLtestfile($filter) [eval format $psSL_format]
           if {[check_file $SLtestfile($filter) "Standard scattered light profile for $filter band"]} {
              return -1
           }
       }     
   }
 
   # for clipping quartiles 
   set badQfracMax [getsoftpar badQfracMax]  
   # this takes care of short runs (PR 4238)
   set nfields [expr $endRun - $startRun + 1]
   set fracBadMax [expr $badQfracMax + 10.0/(20.0+$nfields)]

   write_mem mem "declare_schematrans"
   ### I/O declaration for PSF_KL_COMP
   if ![info exists schemaOK] {
       verb_echo 9 "    declaring I/O structures"
       declare_schematrans [llength $filterlist]
       set schemaOK 1
   }

   # initialize measure objects
   set nfilter [llength $filterlist]
   set fieldparams [fieldparamsNew $filterlist]
   # get radial limit for aperture and wing corrections (in each band)
   get_nann_ap $filterlist nann_ap_frame nann_ap_run

   loop i 0 $nfilter {
      handleSet $fieldparams.frame<$i>.nann_ap_frame [lindex $nann_ap_frame $i]
      handleSet $fieldparams.frame<$i>.ap_corr_run 1
   }

   # fake some stuff in measureObj
   handleSet $fieldparams.pixscale 1.0
   handleSet $fieldparams.fiber_rad 1.5
   set sblist ""
   foreach f $filterlist {
      set sb -100.0
      lappend sblist $sb
   }
   initMeasureObj $fieldparams $nfilter $sblist $rand
   fieldparamsDel $fieldparams; unset fieldparams


   # print out
   verb_echo 3 " Input parameters are from the following files:"
   if {$softpars != ""} {
      verb_echo 3 "  $parametersDir/$parameters"
   }
   if {$QApars != ""} {
      verb_echo 3 "  $parametersDir/$QAparameters"
   }
   verb_echo 3 "  $configDir/$ccdConfig"
   verb_echo 3 "  $configDir/$ccdECalib"
   verb_echo 3 "  $configDir/$ccdBC"
   verb_echo 3 "  $transFile"
   verb_echo 2 "  Input parameters successfully read in" 
   verb_echo 2 "  [cpuTime]"  
 
########################################################################
   verb_echo 1 " "
   verb_echo 1 "===================================="
   verb_echo 1 "Starting PSP for Run $run, Column $camCol"
   verb_echo 1 "===================================="
########################################################################

   # Read the astrometric transformations
   set Npatches 0 
   if {(!$ignore_PSF || !$ignore_calibration) || $fakePSF} {
       astrom_read $startRun $endRun $camCol \
          astroms $filterlist $transFile astrom_node astrom_incl   
   } else {
       set id_values(ASTRO_ID) "not available"
       set astrom_node dummy
       set astrom_incl dummy
   }

   # exPhotom file has to be read here because the keywords
   # are needed in psFF file (ZI, Feb, 99)

   if {0} {
      if $write_psFang {
         set scparambs_arg scparambs;	# name of SCPARAMBS chain array
      } else {
         set scparambs_arg "";		# i.e. don't read SCPARABS
      }
      write_mem mem "making frameinfos"
      set frameinfos [make_frameinfos $fangDir $run $filterlist $camCol \
		       $startRun $endRun $scparambs_arg]
      write_mem mem "made frameinfos"
   } elseif {!$ignore_calibration || !$ignore_FF} {
      write_mem mem "making frameinfos"
      set frameinfos [make_frameinfos $fangDir $run $filterlist $camCol \
		       $startRun $endRun ""]
      write_mem mem "made frameinfos"
   }

   set PTfields ""
   if {!$ignore_calibration} {
      # make a list of lists {field RA Dec} where (RA,Dec) is the position
      # of the field center
      set band0 [lindex $filterlist 0]
      set fieldRADec [get_fieldRADec \
                        $astroms($band0) $astrom_node $astrom_incl $startRun] 
      write_mem mem "reading PT patches"
      set extinctions [extinction_read $photomFileDir $photomFile $filterlist]
      set patches [chainNew CHAIN]
      set mtPatchList [get_mtPatchList $mtPatchDir $mtPatchFormat $mtPatches]
      set ipatch 0
      set ipatchAll 0
      foreach patchfile $mtPatchList {
	 set patch_data [mtpatch_read $patchfile $fieldRADec $PTfields $astrom_node $astrom_incl]
	 if {$patch_data == ""} {	# failed to read patch
	    continue
	 }
	 
	 set patch [lindex $patch_data 0]
	 set PTfields "$PTfields [lindex $patch_data 1]" 
         if {$patch != ""} {
            chainElementAddByPos $patches $patch TAIL AFTER
            incr ipatch
            set PatchFileName($ipatch) $patchfile
            set this_patch [lindex $mtPatches $ipatchAll]
            set PatchFilePrep($ipatch) [lindex $this_patch 0]
            verb_echo 3 " There are [exprGet $patch.nElements] stars in patch"
            verb_echo 3 "   $patchfile"
            handleDel $patch
         } else {
            echo "*** bad data in $patchfile ?!? ***"  
         }
         incr ipatchAll
      }
      write_mem mem "done with PT patches"
      # set KALxx_ID keywords
      set Npatches [chainSize $patches] 
      add_KAL_keywords $filterlist $startRun $Npatches PatchFileName PatchFilePrep

      if {$PTfields != ""} {
         verb_echo 4 " PTfields = $PTfields"
      } else {
         verb_echo 4 " There are no overlapping fields with PT patches"
      }
   } else {
         verb_echo 4 " There are no PT fields since ignore_calibration = 1"
   } 



   # old calibration
   # as of spring 99 old calibration comes from opCamera file, instead of psCT
   # file (see PR 227)
   if {(![info exists ccdCamera] && [set no_file 1] && 
       [set msg "No opCamera file (old calibration) is specified"] != "") ||
       [catch {set oldcal [opCam_read $configDir $ccdCamera $camCol $filterlist]} msg]} {
       if [info exists no_file] {
           unset no_file
           echo " psPlan does not specify opCamera file needed for old zero points!?"
       } else {
           echo " problem with getting zero points from opCamera file:"
           echo $msg
       }
       if {[info exists oldcal]} {calib1bytimeDel $oldcal}
       set oldcal [calib1bytimeNew $filterlist]
       # failed, set fall-back values 
       echo " WARNING: setting fall-back values for photometric zero points"
       handleSet $oldcal.mjd 0	
       loop i 0 $nfilter {
          handleSet $oldcal.calib<$i>->flux20 1000
          handleSet $oldcal.calib<$i>->flux20Err 1000
       }
   }

   loop i 0 $nfilter {
      set f [lindex $filterlist $i]
      verb_echo 6 "      f20($f) = [exprGet  $oldcal.calib<$i>->flux20]"
      verb_echo 6 "   f20Err($f) = [exprGet  $oldcal.calib<$i>->flux20Err]"
   }
   

   write_mem mem "before initialization"
# ======================================================================
# ===================  2) Initialization  ==============================
# ======================================================================

   # print out
   verb_echo 1 " "
   verb_echo 1 "  Initialization" 

#  - initialize modules 
   set smooth_length [getsoftpar ps_smooth_length]
   set wingsize [getsoftpar ps_wingsize]
   initCorrectFrames
   initCellFitobj [envscan [getsoftpar cellprofile_file]] $nfilter

   assert {$smooth_length == 0}
   initProfileExtract
   set fieldparams [fieldparamsNew $filterlist]
   set_ccds_in_fieldparams ccdpars $fieldparams $filterlist

#  - make the calib1byframe list and set the astrometric parameters
   verb_echo 8 "  Making CALIB1BYFRAME for each frame"
   set calib1byframelist [chainNew CALIB1BYFRAME]

 
   loop field $startRun [expr $endRun+1] {
      set position [expr $field-$startRun]
      set cframe [calib1byframeNew $filterlist]
      handleSet $cframe.field $field
      handleSet $cframe.psp_status $PSP_STATUS(OK)
      handleSet $fieldparams.fieldnum $field
      # add astrometry
       if {(!$ignore_PSF || !$ignore_calibration) || $fakePSF} {
         foreach filter $filterlist {
	    set ifilter [lsearch $filterlist $filter]
            handleSet $cframe.calib<$ifilter>->status $PSP_STATUS(OK)
	    # Put the astrom transformation for each color into the CALIB1BYFRAME
            handleSet $cframe.calib<$ifilter>->incl $astrom_incl
  	    handleSet $cframe.calib<$ifilter>->node $astrom_node
            set astro [chainElementGetByPos $astroms($filter) $position]
            handleSetFromHandle $cframe.calib<$ifilter>->toGCC &$astro 
            handleDel $astro  
         }  
      }
      chainElementAddByPos $calib1byframelist $cframe TAIL AFTER
      handleDel $cframe
   }

   # delete the astroms
   if {(!$ignore_PSF || !$ignore_calibration) || $fakePSF} {
      foreach filter $filterlist {
         chainDel $astroms($filter) 
      }
   }


   # get SSC_ID keyword needed for psFF header
   set fangfile [get_scfang_filename $fangDir $run "" $camCol $startRun]
   if [catch {set hdr [hdrReadAsFits [hdrNew] $fangfile]}] {
        error "Cannot read file $fangfile"
   }
   if [catch {set id_values(SSC_ID) [hdrGetAsAscii $hdr SSC_ID]}] {
        set id_values(SSC_ID) "not available"
   }   
   hdrDel $hdr
 

   #for checking that astrometry makes sense
   #print_astro $filterlist $calib1byframelist 

   verb_echo 2 "  [cpuTime]"  

# ======================================================================
# ===================  3) Flatfields  ==================================
# ======================================================================
   # print out
   verb_echo 1 " "
   if {!$ignore_FF} {
      verb_echo 1 "  Calculating Flatfields" 
   } else {
      verb_echo 1 "  Ignoring Flatfields" 
   }


   write_mem mem "calculating flatfields"

   set nFieldDefrag [getsoftpar nFieldDefrag]
   set ffninterp [getsoftpar  ff_ninterp]
   set ffsigrej [getsoftpar ff_sigrej]
   set ffbeta [getsoftpar ff_beta]
   set left_buffer [getsoftpar left_buffer]
   set right_buffer [getsoftpar right_buffer]

   ### loop over filters ###
   foreach filter $filterlist {

      set ifilter [lsearch $filterlist $filter]
      verb_echo 3 " Starting filter $filter"
      set runstr [format "%06d" $run]
      set ccdrow [exprGet $ccdpars($filter).iRow]

      # Get the bias vector for current filter
      set biasfile [get_bias_filename $biasDir $run $filter $camCol]
      set bias [regReadAsFits [regNew] $biasfile]
      calc_crc $biasfile
      set bscale [hdrGetAsInt $bias.hdr TSHIFT]
      # get BIAS_ID keyword 
      if {![info exists id_values(BIAS_ID)]} { 
         if [catch {set hdr [hdrReadAsFits [hdrNew] $biasfile]}] {
             error "Cannot read file $file"
         }
         if [catch {set id_values(BIAS_ID) [hdrGetAsAscii $hdr BIAS_ID]}] {
             set id_values(BIAS_ID) "not available"
         }   
         hdrDel $hdr
      }

      # ensure that all filters use same bias_scale (since we only save
      # one value in psField file).
      if { ![info exists bias_scale] } {
	 set bias_scale $bscale 
      } else {
	 assert {$bscale == $bias_scale}
      }


      # the above part was needed for bias_scale, now we can quit
      # in case we are just stuffing psField files w/o calibration
      if {$ignore_FF && $ignore_PSF && $ignore_calibration} {
          regDel $bias
          continue
      }
	      
      if {[exprGet $ccdpars($filter).namps] == 1} {
          set nSplit [expr [exprGet $ccdpars($filter).nData0]/2]
      } else {
          set nSplit [exprGet $ccdpars($filter).nData0]
      }
      set nData [exprGet $ccdpars($filter).nDataCol]
      set ColOffset [expr [exprGet $ccdpars($filter).nPrescan0]+[exprGet $ccdpars($filter).nPreBias0]]  

      # read the quartiles structure for all the fields through which we'll 
      # loop in this filter
      set rawquartiles [quartiles_read $fangDir $run $ccdrow $camCol \
			$ccdpars($filter) $startRun $endRun $filter tshift]
      set quartiles [quartiles_read $fangDir $run $ccdrow $camCol \
			$ccdpars($filter) $startRun $endRun $filter tshift]
      set qTscale [exprGet $quartiles.tscal]

      ### unfixing quartiles
      set clip_ssc_quartiles [getsoftpar clip_ssc_quartiles]
      if {$clip_ssc_quartiles} {

         set NsigmaSSCQuart [getsoftpar NsigmaSSCQuart]  
         set BCchain *$ccdpars($filter).CCDDefect

         # PR 4128
         quartiles_doBC $quartiles $BCchain $ColOffset 0 1 2
         reset_quartiles $quartiles 0 0 1 2     

         # SSC is too good, reset its quartiles to -9 for fields
	 # with bright stars.  This mimics astroline's behaviour, giving up
	 # and setting an error code.  The PSP knows how to handle such codes.

	 set colStart 0; set colEnd [expr $ColOffset-1]
         set stats [clip_ssc_quartiles $quartiles $colStart $colEnd \
			[expr $ColOffset/2] [expr 1+$NsigmaSSCQuart] $qTscale]
	 if {[lindex $stats 2] > $fracBadMax} {
             echo "clip_ssc_quartiles $quartiles $qTscale [expr 40+$nSplit] $NsigmaSSCQuart" 
             echo "For filter $filter there are too many ([lindex $stats 2]) quartiles-outliers"
             error "clip_ssc_quartiles fails for cols = $colStart - $colEnd"
	 }
    
	 set colStart $ColOffset; set colEnd [expr $ColOffset + $nData - 1]
         set stats [clip_ssc_quartiles $quartiles $colStart $colEnd \
			[expr $ColOffset+$nSplit] $NsigmaSSCQuart $qTscale]    
	 if {[lindex $stats 2] > $fracBadMax} {
             echo "clip_ssc_quartiles $quartiles $qTscale [expr 40+$nSplit] $NsigmaSSCQuart" 
             echo "For filter $filter there are too many ([lindex $stats 2]) quartiles-outliers"
             error "clip_ssc_quartiles fails for cols = $colStart - $colEnd"
	 } 
     
	 set colStart [expr $ColOffset + $nData]; set colEnd [expr 2*$ColOffset + $nData -1] 
         set stats [clip_ssc_quartiles $quartiles $colStart $colEnd \
			 [expr $ColOffset + $nData + $ColOffset/2] [expr 1+$NsigmaSSCQuart] $qTscale]
	 if {[lindex $stats 2] > $fracBadMax} {
             echo "clip_ssc_quartiles $quartiles $qTscale [expr 40+$nSplit] $NsigmaSSCQuart" 
             echo "For filter $filter there are too many ([lindex $stats 2]) quartiles-outliers"
             error "clip_ssc_quartiles fails for cols = $colStart - $colEnd"
	 }


         # PR 4128
         # set all BC to -9
         quartiles_doBC $quartiles $BCchain $ColOffset 
         reset_quartiles $quartiles 1

      }

      if {0} {
	 #
         # Calculate quartiles directly from images
	 #
	 error "This is a purely debugging code path. Do not call it"
         set fieldsDir $data_root/$run/$rerun/fields/$camCol
         quartiles_overwrite $quartiles $fieldsDir $run $filter \
	     $camCol $startRun $endRun $tshift       
      }

      if {$display && [info exists display_quartiles] && $display_quartiles} {
	 set scaled_tmp [regNew -type FL32 \
			     [exprGet $quartiles.data<0>->nrow] \
			     [exprGet $quartiles.data<0>->ncol]]

	 set disp_list quartiles
	 if {$display_quartiles > 0} {
	    set disp_list [linsert $disp_list 0 rawquartiles]
	 }

	 foreach qq $disp_list {
	    set ind -1;			# index into quartiles array
	    foreach type "Q25 Median Q75 Mode" {
	       set tmp [handleBindFromHandle [handleNew] *[set $qq].data<[incr ind]>]
	       regMultiplyWithDbl $tmp [expr 1./$tshift] -regOut $scaled_tmp
	       handleDel $tmp
	       
	       display $scaled_tmp "$filter $qq $type image"
	    }
	 }
	 
	 regDel $scaled_tmp; unset scaled_tmp
      }
      
      set flatlist [flatfieldsCalc $quartiles $bias $bias_scale \
			 $left_buffer $right_buffer $ccdpars($filter)]
      set biases($filter) $bias
      set skylevels [keylget flatlist skylevel1d]
      # bias drift for all chip columns
      set 1dbiasdrift [keylget flatlist biasdrift1d]
      # bias drift for odd chip columns
      set O1dbiasdrift [keylget flatlist Obiasdrift1d]
      # bias drift for even chip columns
      set E1dbiasdrift [keylget flatlist Ebiasdrift1d]
      set flatregion [keylget flatlist flat2d]
      set flatfieldnc [exprGet $flatregion.ncol]

      set median [handleBindFromHandle [handleNew] *$quartiles.data<1>]
      set fmedian [regNew -type FL32 [exprGet $median.nrow] [exprGet $median.ncol]]
      regMultiplyWithDbl $median [expr 1./$tshift] -regOut $fmedian
      handleDel $median
      ### dump quartile region for QA purposes
      set outfile $outputDir/psQuartileRegion-$runstr-$filter$camCol.fit
      regWriteAsFits $fmedian $outfile
      ### dump raw quartiles for QA purposes
      # q25
      set median [handleBindFromHandle [handleNew] *$rawquartiles.data<0>]
      regMultiplyWithDbl $median [expr 1./$tshift] -regOut $fmedian
      handleDel $median
      set outfile $outputDir/psRawQuartile25Region-$runstr-$filter$camCol.fit
      regWriteAsFits $fmedian $outfile
      # q50      
      set median [handleBindFromHandle [handleNew] *$rawquartiles.data<1>]
      regMultiplyWithDbl $median [expr 1./$tshift] -regOut $fmedian
      handleDel $median
      set outfile $outputDir/psRawQuartile50Region-$runstr-$filter$camCol.fit
      regWriteAsFits $fmedian $outfile
      # q75
      set median [handleBindFromHandle [handleNew] *$rawquartiles.data<2>]
      regMultiplyWithDbl $median [expr 1./$tshift] -regOut $fmedian
      handleDel $median
      set outfile $outputDir/psRawQuartile75Region-$runstr-$filter$camCol.fit
      regWriteAsFits $fmedian $outfile

      ### dump flatfield region for QA purposes
      set outfile $outputDir/psFFRegion-$runstr-$filter$camCol.fit
      regWriteAsFits $flatregion $outfile
 
      if {$display && [info exists display_flat] && $display_flat} {
	 display $fmedian "      $filter median quartile image"
	 display $flatregion "      2D $filter flat region"
      }
	 
      regDel $fmedian
 
      verb_echo 6 "------------------------------"
      verb_echo 6 "field    raw sky  interp. sky  skyerr  skysig  lbias  rbias  lOEdiff  rOEdiff"

      ######################################################################

      # interpolate sky and determine bias drift 
      loop row 0 [exprGet $flatregion.nrow] {
	  
	 set cframe [chainElementGetByPos $calib1byframelist $row]
	 set calib [handleBindFromHandle [handleNew] *$cframe.calib<$ifilter>]
	 skybiasInterp $skylevels $1dbiasdrift $O1dbiasdrift $E1dbiasdrift $row $ffninterp \
                       $ffsigrej $ffbeta $ccdpars($filter) $calib
         # n.b. the interpolated value is calib.sky, the raw value is calib.skyraw
         # make sure that all fields have usable sky value
         if {[exprGet $calib.sky] <= 0.0 || [exprGet $calib.skyerr] <= 0.0} {
             if {1 && $row > 0} {
                set oldcframe [chainElementGetByPos $calib1byframelist [expr $row-1]]
   	        set oldcalib [handleBindFromHandle [handleNew] *$oldcframe.calib<$ifilter>]
                handleSet $calib.sky [exprGet $oldcalib.sky]
                handleSet $calib.skyerr [exprGet $oldcalib.skyerr]
	        handleDel $oldcframe
	        handleDel $oldcalib
	     } else {
                verb_echo 2 "ERROR bad sky: sky interpolation failed (ff_ninterp = $ffninterp)"
                verb_echo 2 "      setting sky to raw value for field [expr $startRun+$row]"
                handleSet $calib.sky [exprGet $calib.skyraw]
                handleSet $calib.skyerr [exprGet $calib.skyraw]
             }
         } 

         # print out (calib.skyraw holds raw sky values, calib.sky is interp.)
	 set field [expr $startRun+$row]
	 verb_echo 6 "  $field     [exprGet $calib.skyraw]     [exprGet $calib.sky]    \
                 [exprGet $calib.skyerr]    [exprGet $calib.skysig]   \
                 [exprGet $calib.lbias]    [exprGet $calib.rbias]   \
                 [exprGet $calib.lbiasOEdiff]    [exprGet $calib.rbiasOEdiff] "

	 handleDel $cframe
	 handleDel $calib
      }
 
      # PR 3938
      set maxDrift 10000
      # signal binned data
      if {$ncolperframe < 2000} {set maxDrift 32000}

      smooth_bias_drift $calib1byframelist $ifilter $maxDrift

      set failedFF 0
      #### the rest depends on whether we are fixing the scattered light ###
      if {$fix_background} {
         ### the scattered light fix is requested

         # bad columns 
         set bclist [param2Chain $configDir/$ccdBC bcpars]
         calc_crc $configDir/$ccdBC
         set camRow [keylget psplan ccdrow_$filter]
         set badCols [find_defects $bclist $camRow $camCol]
         set BCmaskAux [vectorExprNew [exprGet $flatregion.ncol]]
         loop kk 0 [chainSize $badCols] {
            set el [chainElementGetByPos $badCols $kk]
            set BCpos [exprGet $el.dfcol0]
            handleSet $BCmaskAux.vec<$BCpos> 1
         }
         set BCmask [vectorExprEval "!$BCmaskAux"]
         vectorExprDel $BCmaskAux
         chainDestroy $bclist genericDel
         chainDestroy $badCols genericDel


         # determine the smoothing window width
         set nfields [expr $endRun - $startRun + 1]
         if {$SLsmoothNfield < 0} {set SLsmoothNfield $nfields}
         if {$SLsmoothNfield == 0} {set SLsmoothNfield 1}
         if {$SLsmoothNfield > $nfields} {set SLsmoothNfield $nfields}
         if {![expr $SLsmoothNfield%2]} {
             incr SLsmoothNfield
             if {$SLsmoothNfield > $nfields} {
                set SLsmoothNfield [expr $SLsmoothNfield - 2]
             }
         }
         # extent of the window to each side
         set nHalfWin [expr ($SLsmoothNfield - 1) / 2]
         if {$nHalfWin < 0} {set nHalfWin 0}
         verb_echo 3 "    Smoothing scattered light over $SLsmoothNfield fields" 

         # do we want to temper with provided flatfields?
         set correct_trueFF [getsoftpar correct_trueFF]
	 if {$correct_trueFF > -1} {
         # before we can do anything else we need to correct possible offset
         # between the left and right half of flatfield vector:
         # holder for region with Bfunc = trueFF / localFF
         set BfuncRegion [regNew -type FL32 [exprGet $flatregion.nrow] [exprGet $flatregion.ncol]]
         # read true flat field vector
         set trueflatvector [regNew]
         regReadAsFits $trueflatvector $trueflat($filter)
         # needed for later       
	 set FFshift [hdrGetAsDbl $trueflatvector.hdr TSHIFT]

         # get Bfunc for each field by medianing flatregion every SLsmoothNfield fields
         loop field $startRun [expr $endRun+1] {    
	     set ifield [expr $field - $startRun]
             verb_echo 5 "   computing Bfunc for field $field (SLsmoothNfield=$SLsmoothNfield)"   
             if {$SLsmoothNfield > 1} {
                set rowStart [expr $ifield - $nHalfWin] 
                if {[expr $rowStart+$SLsmoothNfield] >= [exprGet $flatregion.nrow]} {
                   set rowStart [expr [exprGet $flatregion.nrow]-$SLsmoothNfield]
                }
                if {$rowStart < 0} {
                   set rowStart 0
                }
                # get subregion for medianing
                set SUBflatregion [subRegNew $flatregion $SLsmoothNfield \
				       [exprGet $flatregion.ncol] $rowStart 0]
                # get median "flatfield" in this window
                set flatfield [flatfieldsInterp $SUBflatregion] 
                regDel $SUBflatregion   
	     } else {
		 # no smoothing requested, simply copy the relevant row
                 verb_echo 6 "    BYPASSING THE SMOOTHING OF THE SCATTERED LIGHT PROFILE"
                 set flatfield [regNew -type FL32 1 $flatfieldnc]
                 regClear $flatfield
                 set aux [subRegNew $flatregion 1 $flatfieldnc $ifield 0]   
                 regAdd $flatfield $aux
                 regDel $aux 
             } 

             # convert the local "flat field" to a FL32 region
             set FF32 [regNew -type FL32 1 $flatfieldnc]
             regClear $FF32
             set flatfieldRow0 [subRegNew $flatfield 1 $flatfieldnc 0 0]
             regAdd $FF32 $flatfieldRow0
             regDel $flatfieldRow0
             # divide the true (inverted) flatfield vector by the local one:
             # Bfunc = trueFF / localFF
             set Bfunc [regNew -type FL32 1 $flatfieldnc]
             set stats [regStatsFind $FF32]
             if {[keylget stats low] > 0} {
                 regDivByRow $trueflatvector $FF32 -regOut $Bfunc
             } else {
                 # try to fix minor cases of missing quartiles
                 set NbadColumns [interpolateFF $FF32 0] 
                 if {$NbadColumns > 40} {
		    set msg "ERROR: Too many ($NbadColumns) bad columns: inspect quartiles!!!"
		    echo $msg; error $msg
                 }
                 set stats [regStatsFind $FF32]
                 if {[keylget stats low] > 0} {
                    regDivByRow $trueflatvector $FF32 -regOut $Bfunc
                 } else {
                    set stats [regStatsFind $FF32]                 
                    regClear $Bfunc
                 }
             }
  
             # add the background profile for this field to the output region
             set BfuncThisRow [subRegNew $BfuncRegion 1 [exprGet $BfuncRegion.ncol] $ifield 0]
             regClear $BfuncThisRow
             regAdd $BfuncThisRow $Bfunc

             # clean
             regDel $BfuncThisRow
             regDel $Bfunc   
             regDel $flatfield
             regDel $FF32
 
         }

         # now we have the properly scaled and smoothed background in $BfuncRegion;
         # estimate the jump at centre of chip
         #set Lbuffer 200
         #set Rbuffer 200
	 #Modify these to handle binned data
	 set binning_factor [expr 2048/$ncolperframe]
         set Lbuffer [round 200/$binning_factor] 
         set Rbuffer [round 200/$binning_factor]
	 # get vectors with jumps across $nSplit
         set vJump [get_jump $BfuncRegion $nSplit $Lbuffer $Rbuffer 4]
         set Jmedian [format "%8.6f" [vMedian $vJump]]
         # will be needed to correct for gain ratio variations
         set vJumpNormMed [vectorExprEval $vJump/$Jmedian]
	 set lim 0.01
         if {[expr abs($Jmedian-1)] > $lim} {
	    if {$Jmedian > 1} {
	       set op ">"; set lim [format "%g" [expr 1 + $lim]]
	    } else {
	       set op "<"; set lim [format "%g" [expr 1 - $lim]]
	    }
             verb_echo 1 "The median gain adjustment for filter $filter is $Jmedian ($op $lim)"

	    set lim 0.1
	    if {[expr abs($Jmedian-1)] > $lim} {
	       if {$Jmedian > 1} {
		  set op ">"; set lim [format "%g" [expr 1 + $lim]]
	       } else {
		  set op "<"; set lim [format "%g" [expr 1 - $lim]]
	       }
	       
                set Npoints [exprGet $vJump.dimen]
                set meanClipped [vSigmaClip $vJump -sigmaClip 1.5 -nIter 2]
                set mean  [format "%6.4f" [lindex [lindex $meanClipped 0] 1]]
                set sigma [format "%6.4f" [lindex [lindex $meanClipped 1] 1]]
                echo " WARNING: The median gain adjustment for filter $filter is $Jmedian ($op $lim)"
                echo " Npoints = $Npoints, median = $Jmedian, mean = $mean, sigma = $sigma"
                echo " *** THE PROVIDED FLATFIELD MAY BE BADLY OFF ***" 
		if {[expr abs($Jmedian-1)] > 0.5} { 
                     set failedFF 1
                     echo "ERROR: The median gain adjustment is too large ($Jmedian): inspect flatfields!!!"
                }
             }
         }

         if {1} {
            ### for testing fast gain variations
            set aux [vIndex $nfields]
            set aux2 [vectorExprEval $aux+$startField]
            set nameout $outputDir/psGainVariation-$runstr-$filter$camCol.dat
	    set title "left-to-right gain variation, chip ${filter}$camCol (field vs. jump)"
            vectorsWriteToFile [list $aux2 $vJump] $nameout $title
            vectorExprDel $aux; vectorExprDel $aux2
         }

         vectorExprDel $vJump
         regDel $trueflatvector
         regDel $BfuncRegion
         # calculate fractional corrections to flatfield vector
         set FFcorrectionL [format "%6.4f" [expr 2.0*$Jmedian/(1+$Jmedian)]]
         set FFcorrectionR [format "%6.4f" [expr 2.0/(1+$Jmedian)]]
         verb_echo 3 "Flatfield Corrections: $FFcorrectionL $FFcorrectionR"
         }

         #####################################################################
         # now copy corrected true flatfield vectors as new flatfield vectors
         if $ignore_FF {
	    set hdr [hdrReadAsFits [hdrNew] $trueflat($filter)]
            if [catch {set FF_ID [hdrGetAsAscii $hdr PS_ID]}] {
	       error "Failed to read old PS_ID from $trueflat($filter) header"
            }
	    hdrDel $hdr
	 } else {
            set trueflatvector [regNew]
            regReadAsFits $trueflatvector $trueflat($filter)
	    set FFshift [hdrGetAsDbl $trueflatvector.hdr TSHIFT]
            if {[info exists display_flat] && $display_flat} {
                 dervishPlot $trueflatvector -row 0
                 echo "         True flat-field vector for filter $filter as read from"
                 echo "         $trueflat($filter)"
            }
            calc_crc $trueflat($filter)

  	    if {$correct_trueFF > 0} {
                set leftFF [subRegNew $trueflatvector 1 $nSplit 0 0]
                regIntLincom $leftFF "" 0 $FFcorrectionL 0
                # regMultiplyWithDbl $leftFF $FFcorrectionL 
                regDel $leftFF
                set length [expr [exprGet $trueflatvector.ncol] - $nSplit]  
                set rightFF [subRegNew $trueflatvector 1 $length 0 $nSplit] 
                regIntLincom $rightFF "" 0 $FFcorrectionR 0
                #regMultiplyWithDbl $rightFF $FFcorrectionR 
                regDel $rightFF
            }

            # renormalize such that 1/ff = 1 
            renormalizeFF $trueflatvector $FFshift

            # sanity check: make sure that all elements are > 0
            set stats [regStatsFind $trueflatvector]
            set min [keylget stats low]
            if {$min <= 0} {
               error " ERROR: true flat-field vector (filter $filter) has elements <= 0!!!"
            }

            # PS_ID from provided flatfields is later stored as FF_ID
            if [catch {set FF_ID [hdrGetAsAscii $trueflatvector.hdr PS_ID]}] {
                error "Flatfield vectors must have PS_ID keyword but file $trueflat($filter) doesn't!?!"
            }
	    # fix the flat-field header 
            set kwords "RUN CAMCOL FILTER FIELD VERSION DERV_VER ASTR_VER PS_ID"
            set kwords "$kwords BIAS_ID SSC_ID ASTRO_ID EXCAL_ID"
            foreach kword $kwords {
                catch {hdrDelByKeyword $trueflatvector.hdr $kword}
            }
            # remove KALxx_ID keywords, if any (PR 2280) 
            loop xx 0 99 {
                set keyword KAL[format %02d $xx]_ID 
                catch {hdrDelByKeyword $trueflatvector.hdr $keyword}
            }
            # add the keywords relevant for this run
	    hdrInsWithInt $trueflatvector.hdr RUN $run "Imaging run number."
	    hdrInsWithInt $trueflatvector.hdr CAMCOL $camCol \
	                            "   Column in the imaging camera."
	    hdrInsWithAscii $trueflatvector.hdr FILTER $filter \
	                            "   Filter (u, g, r, i, z, l, or t)."
 	    hdrInsWithInt $trueflatvector.hdr FIELD $field "starting field for this run"
            catch {hdrDelByKeyword $trueflatvector.hdr FF_ID}
 	    hdrInsWithAscii $trueflatvector.hdr FF_ID $FF_ID "a copy of PS_ID from supplied flatfield" 
            insert_id_values -psp $trueflatvector.hdr 
            # write flatfield file
	    set FFoutfile $outputDir/psFF-$runstr-$filter$camCol.fit
            regWriteAsFits $trueflatvector $FFoutfile
            if {[info exists display_flat] && $display_flat} {
                 dervishPlot $trueflatvector -row 0
                 echo "         True flat-field vector for filter $filter after correction"
                 echo "         and written to"
                 echo "         $FFoutfile"
            }
            regDel $trueflatvector
	 }

         ## determine flat-field vector by medianing quartiles, each bin NSLsmooth fields long
         # region for dumping SLfunc profiles
         set Bout [regNew -type FL32 $nfields $flatfieldnc]
         regClear $Bout
         insert_id_values -psp $Bout.hdr

         # read true flat field vector
	 set flatfile $outputDir/psFF-$runstr-$filter$camCol.fit
	 set flatvector($filter) [regNew -type FL32 1 $flatfieldnc]
         regReadAsFits -keep $flatvector($filter) $flatfile 

         # use interpolated sky (can be anything as long as it is consistent)
         set skyVector [vFromChain $calib1byframelist calib<$ifilter>->sky]
         set sData0 [exprGet $ccdpars($filter).sData0]
         # for dumping "better" bias vectors
         set BB [regNew [exprGet $biases($filter).nrow] \
			         [exprGet $biases($filter).ncol]]
         set auxB [regNew -type FL32 [exprGet $biases($filter).nrow] \
			         [exprGet $biases($filter).ncol]]

         if {[info exists display_flat] && $display_flat} {
             dervishPlot $biases($filter) -row 0
             echo "         Raw Bias vector "
             dervishPlot $flatvector($filter) -row 0
             echo "         True flat-field vector "
         }

         # get Bfunc, SL and better bias for each field
         loop field $startRun [expr $endRun+1] {    
             verb_echo 3 "   computing scattered light for field $field"   
	     set ifield [expr $field - $startRun]
             set fieldstr [format "%04d" $field]  
	     set BBoutfile $outputDir/psBB-$runstr-$filter$camCol-$fieldstr.fit
             # calib for this field so that we can overwrite the sky
             set calib [chainElementGetByPos $calib1byframelist $ifield] 
             if {$SLsmoothNfield > 1} {
                set rowStart [expr $ifield - $nHalfWin] 
                if {[expr $rowStart+$SLsmoothNfield] >= [exprGet $flatregion.nrow]} {
                   set rowStart [expr [exprGet $flatregion.nrow]-$SLsmoothNfield]
                }
                if {$rowStart < 0} {
                   set rowStart 0
                }
                # get subregion for medianing
                verb_echo 5 "       smoothing from row $rowStart for $SLsmoothNfield rows"   
                set SUBflatregion [subRegNew $flatregion $SLsmoothNfield \
				       [exprGet $flatregion.ncol] $rowStart 0]
                # get median "flatfield" in this window
                set flatfield [flatfieldsInterp $SUBflatregion] 
                regDel $SUBflatregion   
	     } else {
		 # no smoothing requested, simply copy the relevant row
                 # echo "    BYPASSING SMOOTHING OF SCATTERED LIGHT"
                 set flatfield [regNew -type FL32 1 $flatfieldnc]
                 regClear $flatfield
                 set aux [subRegNew $flatregion 1 $flatfieldnc $ifield 0]   
                 regAdd $flatfield $aux
                 regDel $aux 
             } 

             # convert the local "flat field" to a FL32 region
             set FF32 [regNew -type FL32 1 $flatfieldnc]
             regClear $FF32; regSetWithDbl $FF32 0.001
             set flatfieldRow0 [subRegNew $flatfield 1 $flatfieldnc 0 0]
             regAdd $FF32 $flatfieldRow0
             regDel $flatfieldRow0
             # divide the true (inverted) flatfield vector by the local one:
             # Bfunc = trueFF / localFF
             set Bfunc [regNew -type FL32 1 $flatfieldnc]
             set stats [regStatsFind $FF32]
             if {[keylget stats low] > 0} {
                 regDivByRow $flatvector($filter) $FF32 -regOut $Bfunc
             } else {
                 echo " WARNING: local FF for field $field, filter $filter has 0 value:"
                 if {[info exists display_flat] && $display_flat } {
                     dervishPlot $FF32 -row 0
                 }     
                 # interpolate the missing values by taking the SL shape from the previous field
                 if {$field > $startRun} {
                     verb_echo 3 " WARNING: interpolating local FF ($FF32)..."
                     if {$display_flat > 1} {
                        echo "before interpolation"
                        dervishPlot $FF32 -row 0
                        echo "interpolation source ($OldFlatfield)"
                        dervishPlot $OldFlatfield -row 0
                     }
                     # fix bad values
                     set NbadColumns [interpolateFF $FF32 0] 
                     if {$NbadColumns > 20} {
                        echo "ERROR: Too many ($Nbadcol) bad columns: inspect quartiles!!!"
                        error "ERROR: Too many ($Nbadcol) bad columns: inspect quartiles!!!" 
                     }
                     loop col 1 [exprGet $FF32.ncol] {
                        if {[exprGet $FF32.rows_fl32<0><$col>] <= 0} {
                           set oldvalue [exprGet $FF32.rows_fl32<0><$col>] 
                           set delta [expr [exprGet $OldFlatfield.rows_fl32<0><$col>] - \
                                        [exprGet $OldFlatfield.rows_fl32<0><[expr $col-1]>]] 
                           set value [expr [exprGet $FF32.rows_fl32<0><[expr $col-1]>] + $delta] 
                           handleSet $FF32.rows_fl32<0><$col> $value
                           echo "($field,$col): replaced $oldvalue by $value (delta=$delta)" 
                           echo "               previous field value = [exprGet $OldFlatfield.rows_fl32<0><$col>]"
			}
                     }
                     if {[exprGet $FF32.rows_fl32<0><0>] <= 0} {
                         handleSet $FF32.rows_fl32<0><0> [exprGet $FF32.rows_fl32<0><1>] 
                     }
                     set stats [regStatsFind $FF32]
                     if {$display_flat > 1} {
                        echo "after interpolation ($FF32)"
                        dervishPlot $FF32 -row 0
                     }
                     if {[keylget stats low] > 0} {
                         regDivByRow $flatvector($filter) $FF32 -regOut $Bfunc
                     } else {
                         echo  "       the first pixel: [exprGet $FF32.rows_fl32<0><0>]"
                         error "ERROR: tried to interpolate quartiles for field $field and failed miserably."
                     }
                 } else {
                    # well, this is the first field, the best we can do is interpolation in the 
                    # column direction
                    if {[exprGet $FF32.rows_fl32<0><0>] <= 0} {
                         error "Not a good way to start a day: the First Quartile <= 0" 
                    }
                    loop col 1 [exprGet $FF32.ncol] {
                        if {[exprGet $FF32.rows_fl32<0><$col>] <= 0} {
                           set left [exprGet $FF32.rows_fl32<0><[expr $col-1]>] 
                           handleSet $FF32.rows_fl32<0><$col> $left
                           echo " WARNING: faking quartiles for the very first field ($field)"
			}
                    }
                    if {$display_flat > 1} {
                        echo "after interpolation of the FIRST field"
                        dervishPlot $FF32 -row 0
                    }
                    if {[keylget stats low] > 0} {
                        regDivByRow $flatvector($filter) $FF32 -regOut $Bfunc
                    } else {
                        error "ERROR: tried to fake quartiles for the first field and failed miserably."
                    }
                 }
              }
               
              # add the background profile for this field to the output region
              set BfuncThisRow [subRegNew $BfuncRegion 1 [exprGet $BfuncRegion.ncol] $ifield 0]
              regClear $BfuncThisRow
              regAdd $BfuncThisRow $Bfunc

              # clean
              regDel $BfuncThisRow
              if {$field > $startRun} {
                 if [info exist OldFlatfield] {
                     regDel $OldFlatfield
                 } else {
                     error "What is going on: $field > $startRun and OldFlatfield !exist??"
                 }
              }
              # for the case of bad quartiles
              set OldFlatfield $FF32
             

             # find the fractional scattered light contribution:
             # SL = sky/trueFF * [Bfunc - Bo] = sky * SLfunc
             # here Bo can be either min(Bfunc) or median(Bfunc); the first choice
             # satisfies SL>=0 everywhere, and the second one preserves the sky value
             # somewhat counterintuitively, it was found for run 1336 that the 
             # calibrated sky agrees much better across the camera columns for 
             # the second choice, than for the first one; so
             set positiveSL 0
             ## first find the reliable minimum of the background
             set Bvector [vectorGetFromRegion $Bfunc 0 0 0 [expr $flatfieldnc-1] 0]
             ### do not trust opBC file             
             set goodBC 0             
             if {$goodBC} {
                 # the bad columns info is reliable 
                 set Bmin [vExtreme $Bvector -vMask $BCmask min]  
                 set Bmax [vExtreme $Bvector -vMask $BCmask max] 
                 set Bmed [vMedian $Bvector -vMask $BCmask] 
             } else {
                 # the BC info is not reliable, find and mask all >10% spikes 
                 set spikemask [get_spike_mask $Bvector 0.1 $BCmask]  
                 set Bmin [vExtreme $Bvector -vMask $spikemask min]  
                 set Bmax [vExtreme $Bvector -vMask $spikemask max]  
                 set Bmed [vMedian $Bvector -vMask $spikemask] 
                 vectorExprDel $spikemask
             }
             vectorExprDel $Bvector

             if {$positiveSL} {
                 set Bo $Bmin
             } else { 
                 set Bo $Bmed		   
             }

             ### for proper sky normalization
             set Bo 1.0
             verb_echo 5 "Bmin=$Bmin, Bmax=$Bmax, Bmed=$Bmed, Bo=$Bo (positiveSL=$positiveSL)"

             # multiplicative sky correction due to the part folded into bias
             if {$Bmed > 0} {
                 set Bcorr [expr 1.0*$Bo/$Bmed]
             } else {
                 error "Bmed < 0 ($Bmed)???"
             }
             
             # set up regions needed to evaluate background correction
             set aux [regNew -type FL32 1 $flatfieldnc]
             set SLfunc [regNew -type FL32 1 $flatfieldnc]
             # SLfunc = [Bfunc - Bo]/trueFF * FFshift
             regClear $aux; regAdd $aux $Bfunc
             regAddWithDbl $aux -$Bo
             if {[info exists display_flat] && $display_flat > 3} {
                 regAddWithDbl $aux 1.0
                 dervishPlot $aux -row 0
                 echo "         (1 + SL profile) for field $field"
                 regAddWithDbl $aux -1.0
             }          
             # before dividing by true flatfield
             # add [Bfunc - Bo] prof for this field to the output region
             set BoutThisRow [subRegNew $Bout 1 [exprGet $Bout.ncol] $ifield 0]        
             regClear $BoutThisRow
             regAdd $BoutThisRow $aux
             regDel $BoutThisRow
             # now divide by true flat-field because bias gets multiplied by FF in frames  
             regDivByRow $aux $flatvector($filter) -regOut $SLfunc

             if {[info exists display_flat] && $display_flat > 1} {
                 dervishPlot $SLfunc -row 0
                 echo "         SL profile before multiplying by $FFshift" 
             }          
   
             regMultiplyWithDbl $SLfunc $FFshift 

             if {[info exists display_flat] && $display_flat > 1} {
                 dervishPlot $SLfunc -row 0 -ymin -0.3 -ymax 0.3
                 echo "         SL profile after multiplying by $FFshift" 
             }          

             regDel $aux

             # display the results
             if {[info exists display_flat] && $display_flat} {
                if {$display_flat > 2} {
                   dervishPlot $flatfield -row 0
                   echo "         Local flat-field vector for field $field"
                   if {$SLsmoothNfield > 1} {
                      dervishPlot $flatfield -row 1
                      echo "         Fractional error in local FF (multiplied by 10^6)"
                      dervishPlot $flatfield -row 2
                      echo "         Difference between 2 halfs of local FF (m. by 10^6)"
                   }
                   dervishPlot $Bfunc -row 0
                   echo "      B function (true/local): min = $Bmin, max = $Bmax, med=$Bmed" 
                }
                if {$display_flat > 1} {
                   regAddWithDbl $SLfunc 1.0
                   dervishPlot $SLfunc -row 0 -ymin 0.7 -ymax 1.3
                   echo "         (1 + SL profile) divided by FF for field $field"
                   regAddWithDbl $SLfunc -1.0
                }
             }
 
             ### write out better bias
             set auxFF [regNew -type FL32 1 $flatfieldnc]
             # copy header and add FF_ID keyword
             hdrCopy $biases($filter).hdr $BB.hdr
 	     hdrInsWithAscii $BB.hdr FF_ID $FF_ID "a copy of PS_ID from supplied flatfield" 
             insert_id_values -psp $BB.hdr
             # evaluate bkgd correction as bkgd = sky * (aux1 - aux2)
             # sky (smoothed) for this field
	     set sky [exprGet $skyVector.vec<$ifield>]

             # correct the sky in calib for the part which goes into bias vector (PR 2279) 
             # bkgd correction = sky*SLfunc = sky*(1/localFF - Bo/trueFF)
             regMultiplyWithDbl $SLfunc $sky -regOut $auxFF            
             set bkgd [vectorGetFromRegion $auxFF 0 0 0 [expr $flatfieldnc-1] 0]
             set medianBkgd [vMedian $bkgd]; vectorExprDel $bkgd
             verb_echo 5 " The background contribution hidden into bias is $medianBkgd counts (sky=$sky)"
             if {$medianBkgd >= $sky} {
                 echo " == FIELD=$field, filter=$filter =="
                 echo " sky = $sky, medianBkgd = $medianBkgd (SLfunc=$SLfunc)"
                 error " median background cannot be larger than the sky!?!"
             } 

             # for proper sky normalization
             set medianBkgd 0.0
             handleSet $calib.calib<$ifilter>->sky [expr $sky-$medianBkgd]
             verb_echo 5 " Old sky = $sky, new sky = [exprGet $calib.calib<$ifilter>->sky]" 
             set skyraw [exprGet $calib.calib<$ifilter>->skyraw]
             handleSet $calib.calib<$ifilter>->skyraw [expr $skyraw-$medianBkgd]

             # bkgd correction to bias vector must include bias_scale
             set scale [expr $sky * $bias_scale]
             regMultiplyWithDbl $SLfunc $scale -regOut $auxFF 
             if {[info exists display_flat] && $display_flat} {
                 dervishPlot $SLfunc -row 0
                 echo "         SLfunc"
                 dervishPlot $auxFF -row 0 -ymin [expr -0.5*$scale] -ymax [expr 0.5*$scale]
                 echo "         and bkgd contribution (auxFF)"
             }
                   
             # n.b. sData0: auxFF is 2048 long, auxB is 2128 long
             regClear $auxB
	     set auxFF2B [subRegNew $auxB 1 [exprGet $auxFF.ncol] 0 $sData0]  
	     regAdd $auxFF2B $auxFF; regDel $auxFF2B
             # adding scattered light component to the original bias 
             if {[info exists display_flat] && $display_flat} {
                 dervishPlot $biases($filter) -row 0
                 echo "         Raw bias vector again..."
                 dervishPlot $auxB -row 0 -ymin -100 -ymax 100
                 echo "         and bkgd contribution (auxB):"
             }
             ### this is where the scattered light is hidden into bias vector
             regAdd $biases($filter) $auxB -regOut $BB 
             ### 
             if {[info exists display_flat] && $display_flat} {
                 dervishPlot $BB -row 0 
                 echo "         Bias vector after bkgd addition (and before OE corr.)"
             }
             # correct bias vector for odd/even bias drift (and QA) 
             do_bias_drift $BB [expr $nSplit+$sData0] $ifilter $calib  
             # correct bias vector for gain ratio variation
             set gainRatio [exprGet $vJumpNormMed.vec<$ifield>]
             if {[correct_gain $BB [expr $nSplit+$sData0] $gainRatio $bias_scale $sky]} {
                verb_echo 7 "correct_gain $BB [expr $nSplit+$sData0] $gainRatio $bias_scale $sky"
                verb_echo 7 "do_bias_drift $BB [expr $nSplit+$sData0] $ifilter $calib"
                # error "failed in correct_gain"
             }   
 
             ### for in situ testing
             if {0} {
                set ff_scale 2048    
                ### make pseudo bias
                set BBp [regNew -type FL32 1 2048] 
                set BFscale [expr $bias_scale * $ff_scale]
                set BBs [subRegNew $BB 1 2048 0 40] 
                regMultWithRow $BBs $flatvector($filter) -scale $BFscale \
                         -regOut $BBp 
                # read quartiles
                set file [eval format "scFang-%06d-%d-%04d.fit" $run $camCol $startField]
                set file $fangDir/$file      
                set hdu [get_hdu $file $filter quarts]
                set quartiles_nc 2128
                set quartiles [quartilesNew 1 $quartiles_nc]
                set fld $field
                set file [eval format "scFang-%06d-%d-%04d.fit" $run $camCol $fld]
                set file $fangDir/$file      
                set tblcol [handleNewFromType TBLCOL]
                fitsRead $tblcol $file -binary -hdu $hdu
                if [catch {
	        tblcolToQuartiles $tblcol $quartiles [expr $fld-$startField]} msg] {
	           error "file $file $filter: $msg"
                }
                set tshift [hdrGetAsInt $tblcol.hdr TSHIFT]
                echo TSHIFT = $tshift
                handleDelFromType $tblcol
                # make pseudo raw
                set rawQuartilesScaled [handleBindFromHandle [handleNew] *$quartiles.data<1>]
                set rawQuartilesTrimmed [subRegNew $rawQuartilesScaled 1 2048 0 40]  

                # quartile region on the same scale as idR files
                set rawQ [regNew -type FL32 1 2048]
                regMultiplyWithDbl $rawQuartilesTrimmed [expr 1.0/$tshift] -regOut $rawQ
                # output region with corrected quartiles
                set corrQ [regNew -type FL32 1 2048]  
                # pseudo-raw
                set pRaw [regNew -type FL32 1 2048] 
                regMultWithRow $rawQ $flatvector($filter) -scale $ff_scale -regOut $pRaw
                # subtract pseudo-bias
                regSubtract $pRaw $BBp -regOut $corrQ
                echo "corrected quartiles for field $fld"
                dervishPlot $corrQ -row 0 -xmin 0 -xmax 2048 -ymin 108 -ymax 114 
             }

             # write out better bias
             if {[info exists display_flat] && $display_flat} {
                 dervishPlot $BB -row 0
                 echo "         and a Better Bias vector!"

             }
             regWriteAsFits $BB $BBoutfile
	     calc_crc -field $BBoutfile

             # clean
             regDel $flatfield
             regDel $Bfunc; regDel $SLfunc; regDel $auxFF

         }
  
         if [info exist OldFlatfield] {
             regDel $OldFlatfield
         } else {
             error "What is going on: finished field loop but OldFlatfield !exist??"
         }
   
         # dump SL profile region
	 set outfile $outputDir/psSLprof-$runstr-$filter$camCol.fit
         regWriteAsFits $Bout $outfile

         if {$test_SLprofile} {
            # test SL region
            if {$filter == "u"} {
               set maxMeanOffset 0.003          
               set maxMedianOffset 0.02          
               set maxSigma  0.01
            } else {
               set maxMeanOffset 0.001         
               set maxMedianOffset 0.01         
               set maxSigma  0.005
            }
            if {[testSLregion $SLtestfile($filter) $outfile $maxMeanOffset $maxMedianOffset $maxSigma]} {
               echo "Scattered light profile for filter $filter changed too much, please inspect these files:"
               echo "$SLtestfile($filter)"
               echo "$outfile"
               echo "called: testSLregion file1 file2 $maxMeanOffset $maxMedianOffset $maxSigma"
               set failedFF 1 
            } 
         }
          
         # clean
         vListDel [list $skyVector $BCmask $vJumpNormMed]
         regDel $BB; regDel $auxB
         regDel $flatvector($filter)
         regDel $Bout

      } else {

         ### the scattered light fix is not requested

         # determine flat-field vector by medianing quartiles
         # for the _entire_ run 
         set flatfield [flatfieldsInterp $flatregion]

         # display flat_field
         if {[info exists display_flat] && $display_flat > 1} {
               dervishPlot $flatfield -row 0
               echo "         Flat-field vector "
               dervishPlot $flatfield -row 1
               echo "         Fractional error in flat-field (multiplied by 10^6)"
               dervishPlot $flatfield -row 2
               echo "         Difference between 2 halfs of flat-field (m. by 10^6)"
         }
         # and bias/sky
         if {[info exists display_bias] && $display_bias} {
            dervishPlot $biases($filter) -row 0
            echo "      Bias vector"
            dervishPlot $skylevels -col 0
            echo "      1d sky vector (before interpolation)"
            dervishPlot $skylevels -col 2  
            echo "      1d sky vector (after interpolation)"
            dervishPlot $1dbiasdrift -col 0
            echo "      1d bias drift vectors (left)"
            if {[exprGet $ccdpars($filter).namps] > 1} {
	       echo "      1d bias drift vectors (right)" 
	       dervishPlot $1dbiasdrift -col 1
            }
         }
         # write flat-field vector 
         if {!$ignore_FF} {   
	    # (fix the header first)
	    hdrInsWithInt $flatfield.hdr RUN $run "Imaging run number."
	    hdrInsWithInt $flatfield.hdr CAMCOL $camCol \
	       "   Column in the imaging camera."
	    hdrInsWithAscii $flatfield.hdr FILTER $filter \
	       "   Filter (u, g, r, i, z, l, or t)."
            catch {hdrDelByKeyword $flatfield.hdr FIELD}
 	    hdrInsWithInt $flatfield.hdr FIELD $field "starting field for this run"
            insert_id_values -psp $flatfield.hdr 
            # flatfield file
	    set outfile $outputDir/psFF-$runstr-$filter$camCol.fit
            # $flatfield has 3 rows, we need to store only the first one
            set auxreg [subRegNew $flatfield 1 $flatfieldnc 0 0]
            # copy header
            hdrCopy $flatfield.hdr $auxreg.hdr
            regWriteAsFits $auxreg $outfile
            # make sure that all elements are > 0
            set stats [regStatsFind $auxreg]
            set min [keylget stats low]
            if {$min <= 0} {
               echo " **** WARNING: flat-field vector (filter $filter) has \
                           elements <= 0!!! ****"
            }
            regDel $auxreg
	 }
         regDel $flatfield
      }

      verb_echo 6 "------------------------------"
      quartilesDel $quartiles
      quartilesDel $rawquartiles
      regDel $skylevels
      regDel $1dbiasdrift
      regDel $O1dbiasdrift
      regDel $E1dbiasdrift
      regDel $flatregion
      regDel $biases($filter) 
   } 
   # end of loop over filters


   if {!$ignore_FF || !$ignore_PSF || !$ignore_calibration} {

      # calculate sky slope (based on interpolated values)
      get_skyslope $calib1byframelist $filterlist $nrowperframe

      # for investigating sky behavior 
      if {[info exists dump_Sky] && $dump_Sky} {
         foreach filter $filterlist {
            set ifilter [lsearch $filterlist $filter]
            set skyfile $outputDir/Sky-$run-$camCol-$filter.dat  
            dump_sky $calib1byframelist $skyfile $ifilter $startRun
         }
      }
   }

   verb_echo 2 "  [cpuTime]"   

   if $defragment_memory {
      defragment_memory
   } else {
      verb_echo 3 Skipping memory defrag
   }

# ======================================================================
# ===================  4) Making starlists  ============================
# ======================================================================
  
   # print out
   verb_echo 1 " "
   verb_echo 1 "  Making Starlists" 
   verb_echo 3 "    initializing..." 
   write_mem mem "initializing starlists"  

   ### initializing ### 
   if {!$ignore_PSF} {
       set wingstar1clist [chainNew STAR1C]
   }
   foreach filter $filterlist {
      set flatvector($filter) [regNew -type U16]
      if {!$ignore_PSF} {
	 set winglist($filter) [chainNew STAR1]
         set wing_rejects($filter) [chainNew STAR1]
         set allwingmasks($filter) [chainNew OBJMASK]
         set wingmasks($filter) [chainNew OBJMASK]
         set rej_wingmasks($filter) [chainNew OBJMASK]
      }
      set frameinfolist($filter) [chainNew FRAME_INFORMATION]
      if {!$ignore_calibration} {
         set MTstarlist($filter) [chainNew STAR1PC]
      }
   }
	  
   # display wing stars?
   if {$display && [info exists display_wing] && $display_wing} {
       set make_wing_regions "-region"
   } else {
       set make_wing_regions ""
   }
         
   # display symmetry?  
   if {$display && [info exists display_symmetry] \
                && $display_symmetry} {	
       set keep_symmetry "-keep_symmetry"
   } else {
       set keep_symmetry ""
   }

   # read  parameters relevant to creating PSFs and stars
   set star1_aperature [getsoftpar star1_aperature]  
   set psf_def_rad [getsoftpar psf_def_rad] 
   set wing_def_rad [getsoftpar wing_def_rad] 
   set psfKLbasis_nframe  [getsoftpar psfKLbasis_nframe]
   set psfKLbasis_Nmax [getsoftpar psfKLbasis_Nmax] 
   set psfKLcoeffs_nframeReal  [getsoftpar psfKLcoeffs_nframe]
   set psfKLcoeffs_nframe [expr int($psfKLcoeffs_nframeReal + 0.9999)]
   set nsigma_sigma [getsoftpar psf_nsigma_sigma]
   set nsigma_QU [getsoftpar psf_nsigma_QU]
   set nsigma_RMS_rad [getsoftpar psf_nsigma_RMS_rad]
   set niter [getsoftpar psf_niter]
   set critical_amplitude [getsoftpar psf_critical_amplitude]
   set min_starnum [getsoftpar min_starnum]
   set psf_threshold [int [getsoftpar psf_threshold]]	 
   set cr_min_s [getsoftpar cr_min_sigma]           
   set cr_min_e [getsoftpar cr_min_e]            
   set cr_cond3 [getsoftpar cr_cond3]
   set cr_cond32 [getsoftpar cr_cond32]
   set sigma_multi [getsoftpar sigma_multi]        
   set sigma_badsky [getsoftpar sigma_badsky]         
   set second_peak [getsoftpar second_peak]   
   set nowing_option [getsoftpar nowing_option]   
   set err_min  [getsoftpar soft_err_min]   
   set err_max  [getsoftpar soft_err_max]  
   set ncellclip [getsoftpar ncellclip] 
   set nbadcell [getsoftpar nbadcell] 
   set maxdev [getsoftpar maxdev]  
   # determine the window width for PSF determination (make sure 
   # that it has odd number of frames)
   set nfields [expr $endRun - $startRun + 1]
   if {$psfKLbasis_nframe > $nfields} {set psfKLbasis_nframe $nfields}
   if {$psfKLcoeffs_nframe > $nfields} {set psfKLcoeffs_nframe $nfields} 
   if {![expr $psfKLbasis_nframe%2]} {incr psfKLbasis_nframe}
   if {![expr $psfKLcoeffs_nframe%2]} {incr psfKLcoeffs_nframe}
   # check again that <= $nfields
   if {$psfKLbasis_nframe > $nfields} {
       set psfKLbasis_nframe [expr $psfKLbasis_nframe - 2]
   }
   if {$psfKLcoeffs_nframe > $nfields} {
       set psfKLcoeffs_nframe [expr $psfKLcoeffs_nframe - 2]
   }

   # extent of the window for KL basis functions to each side
   set nHalfWin [expr ($psfKLbasis_nframe - 1) / 2]
   if {$nHalfWin < 0} {set nHalfWin 0}
   # extent of the window for KL coefficients to each side
   set nHalfWinCoeff [expr ($psfKLcoeffs_nframe - 1) / 2]
   if {$nHalfWinCoeff > $nHalfWin} {set nHalfWinCoeff $nHalfWin} 
   set delWin [expr $nHalfWin - $nHalfWinCoeff]

   if {$ignore_PSF} {
       # doing only PT calibration, set all windows to 1 to simplify coding
       set nHalfWin 0
       set nHalfWinCoeff 0
       set delWin 0
   }
   # limits on master_row for coeff stars
   if {$psfKLcoeffs_nframeReal < 1.0} {set psfKLcoeffs_nframeReal 1.0}
   set master_row_max0 [expr $nrowperframe*(0.5+$psfKLcoeffs_nframeReal/2.0)+$scan_overlap]
   set master_row_min0 [expr $nrowperframe*(0.5-$psfKLcoeffs_nframeReal/2.0)]
   # clipping parameters for phQAforPsfStars
   set clip_apCrat [getsoftpar clip_apCrat]
   set apCrat_qmin [getsoftpar apCrat_qmin]  
   set apCrat_qmax [getsoftpar apCrat_qmax] 
   # Flags to control selecting PSF stars:
   # (N.b. *_star1_ignore_mask is automatically ORed into *_star1c_ignore_mask)
   set PSF_star1c_require_mask 0x0
   set PSF_star1c_ignore_mask 0x0
   set PSF_star1_require_mask 0x0
   set PSF_star1_ignore_mask [expr $STAR1(CR) | $STAR1(ASYMMETRIC)]
   # Flags to control selecting wing stars:
   set wings_star1c_require_mask [expr $STAR1(SATURATED)]
   set wings_star1c_ignore_mask [expr $wings_star1c_require_mask]
   set wings_star1_require_mask 0x0
   set wings_star1_ignore_mask [expr $STAR1(SATURATED) | $STAR1(CR) | \
				     $STAR1(MULTIPLE) | $STAR1(BADSKY)]
   # Flags to control selecting psFang stars:
   set psFangs_star1c_require_mask 0x0
   set psFangs_star1c_ignore_mask [expr $STAR1(SATURATED) | $STAR1(NOSTOKES) |\
				        $STAR1(BADMULTIPLE) | \
				        $STAR1(BADCENTER) | \
				        $STAR1(NOPROFILE) | $STAR1(NOAPMAG)]
   set psFangs_star1_require_mask 0x0
   set psFangs_star1_ignore_mask [expr $STAR1(CR) | \
				       $STAR1(MULTIPLE) | $STAR1(BADSKY)]

   # get flatfield and biases for each filter ### 
   foreach filter $filterlist {
      set file $outputDir/psFF-$runstr-$filter$camCol.fit
      regReadAsFits -keep $flatvector($filter) $file
      calc_crc $file
      # sanity check flatfield vector
      # assert { [is_good_flat $file] } 
 
      # sanity check for bias files
      set field $startRun
      if {$read_psBB_files} {
          set BBfile [get_bias_filename $outputDir $run $filter $camCol $field]
      } else {
	  set BBfile [get_bias_filename $biasDir $run $filter $camCol]
      }
      set BBvector($filter,$field) [regNew]
      regReadAsFits $BBvector($filter,$field) $BBfile
      calc_crc $BBfile
      # if "better bias" is used, then it must have the same FF_ID
      # as the flatfield files
      if {$read_psBB_files} {
          set FF_IDflat [hdrGetAsAscii $flatvector($filter).hdr FF_ID]
          set FF_IDbias [hdrGetAsAscii $BBvector($filter,$field).hdr FF_ID]
	  assert { $FF_IDflat == $FF_IDbias }             	 
      } 
      regDel $BBvector($filter,$field)
    
   }



   ### WING STARS ###
   write_mem mem "wing stars"

   # first initialize wing_exists for all needed fields and filters
   loop field $startRun [expr $endRun+1] { 
      foreach filter $filterlist { 
	 set wing_exists($filter,$field) 0 
	 set wing_col0($filter,$field) 0 
	 set wing_row0($filter,$field) 0 
      }
   }

   # read info on wing stars
   if {!$ignore_wing_stars} {
        verb_echo 3 "    wing stars..." 
        read_wingpar wing_exists wing_col0 wing_row0 $fangDir \
                     $run $camCol $startRun $endRun $filterlist
   } else {
        verb_echo 3 "    ignoring wing stars" 
   }

   # loop over each filter and field creating wing lists
   foreach filter $filterlist {
      if {$ignore_PSF} {      # no need for wing stars if bypassing PSF
          continue
      }

      set ifilter [lsearch $filterlist $filter]
      set ccdrow  [exprGet $ccdpars($filter).iRow]
      set wid0 0
     
      # loop over each frame # 
      loop field $startRun [expr $endRun+1] {

         # read bias file for this field
         if {$read_psBB_files} {
	    set BBfile [get_bias_filename $outputDir $run $filter $camCol $field]
         } else {
	    set BBfile [get_bias_filename $biasDir $run $filter $camCol]
         }
	 set wingBBvector($filter,$field) [regNew]
         regReadAsFits $wingBBvector($filter,$field) $BBfile

	 handleSet $fieldparams.fieldnum $field
	 # get the field parameters
	 set cframe [chainElementGetByPos $calib1byframelist \
			 [expr $field-$startRun]]
	 set calib [handleBindFromHandle [handleNew] *$cframe.calib<$ifilter>]

	 set gainreg [binregionNewFromConst [exprGet $calib.gain] -shift $PHOTO_CONSTS(MAX_U16)]
	 handleSetFromHandle $fieldparams.frame<$ifilter>.gain &$gainreg
	 set dark_variancereg [binregionNewFromConst [exprGet $calib.dark_variance] -shift $PHOTO_CONSTS(MAX_U16)]
	 handleSetFromHandle $fieldparams.frame<$ifilter>.dark_variance $calib.dark_variancereg

         # if there is a wing star for this field and filter, read it
         # and put it on a chain of regions (there is only 1 element; this
         # is to agree with makeStar1clist)
         if {$wing_exists($filter,$field)} {
             if {$wing_exists($filter,$field) == 1} {  
               # wing_exists == 1 means there is really a wing star         
               verb_echo 6 "There is a wing star for field $field"
	       set fieldstr [format %04d $field]
	       set file $fangDir/scWing-$runstr-$filter$camCol-$fieldstr.fit
               set rawwing [regNew $wingsize $wingsize]  
               regReadAsFits $rawwing $file
	       if {$field == $startRun} {calc_crc $file}
               # get lower left corner from the frame center
               handleSet $rawwing.col0 [expr $wing_col0($filter,$field) - $wingsize/2]
               handleSet $rawwing.row0 [expr $wing_row0($filter,$field) - $wingsize/2]
             } else {
               # wing_exists == 2 means there is at least 1 wing star for this field
               # but not in this filter; thus fake an empty region
               verb_echo 6 "Faking a wing star for field $field"  
               set rawwing [regNew $wingsize $wingsize]  
               regClear $rawwing
               regIntSetVal $rawwing 1000
               handleSet $rawwing.name "fake wing star"   
               handleSet $rawwing.col0 0
               handleSet $rawwing.row0 0
             }
           
             set wings [chainNew REGION]
             chainElementAddByPos $wings $rawwing TAIL AFTER
             handleDel $rawwing
             # correct the wings 
             set wing1_aperature 10
             set biases($filter) $wingBBvector($filter,$field)  
	     eval makeStar1clist $wingstar1clist $allwingmasks($filter) $wings \
	        $ifilter $nfilter $wid0 $field $flatvector($filter) $biases($filter) \
    	        $ccdpars($filter) $bias_scale $calib $wingsize $sigma_multi $sigma_badsky \
                $second_peak 1 $wing1_aperature $wing_def_rad $cr_min_s $cr_min_e \
                $cr_cond3 $cr_cond32 $fieldparams.frame<$ifilter> $psf_threshold \
                $make_wing_regions  

	     incr wid0 [exprGet $wings.nElements]
	     chainDestroy $wings regDel

             if {$verbose > 6} {	
                set wingstar1c [chainElementGetByPos $wingstar1clist [expr $wid0-1]] 
                print_star1c_flags $wingstar1c $ifilter
             }
         } 

	 handleDel $calib
	 handleDel $cframe

         # free bias that was needed to correct wing stars from this field
         regDel $wingBBvector($filter,$field)

      }
   }
   #
   # Now trim those lists of wing stars
   #
   foreach filter $filterlist {
      if {$ignore_PSF} {      # no need for wing stars if bypassing PSF
          continue
      }

      set ifilter [lsearch $filterlist $filter]

      makeStarlist $wingstar1clist $winglist($filter) $wing_rejects($filter) \
	  $ifilter 0 [expr [chainSize $wingstar1clist] - 1] \
	  $wings_star1c_require_mask $wings_star1c_ignore_mask \
	  $wings_star1_require_mask $wings_star1_ignore_mask
      # select the masks for good wing stars  
      get_wing_masks wingstar1clist winglist allwingmasks wingmasks rej_wingmasks $filter $ifilter 
      # prints out table with sx1, sx2, width, ...
      verb_echo 9 "WINGS:"
      log_starlist $winglist($filter) $filter
      verb_echo 9 "WINGS REJECTS:"
      log_starlist $wing_rejects($filter) $filter


      if {[info exists display_wing] && $display_wing} {            
           set mosaic [star1_mosaic $winglist($filter) $wingsize ids]
	   if {[saoGetRegion] != ""} {		# an saoImage exists
	      saoReset;
	      foreach id $ids { 
		 # print id and frame
		 saoDrawText [lindex $id 0] [lindex $id 1] \
		     "[lindex [lindex $id 2] 0]:[lindex [lindex $id 2] 2]"
		 draw_frame [lindex $id 0] [lindex $id 1] $wingsize 1 -i  
	      }
	   }
           display $mosaic " Wing stars in filter $filter" "" 1
           if {0} {
               echo "XXX dumping wing mosaic"
               regWriteAsFits $mosaic wingmosaic_filter$filter_[pid].fit
           }
           regDel $mosaic;
           if {[chainSize $wing_rejects($filter)] > 0} {
              set mosaic [star1_mosaic $wing_rejects($filter) $wingsize ids]
	      if {[saoGetRegion] != ""} {		# an saoImage exists
		 saoReset;
		 foreach id $ids { 
		    # print id and frame
		    saoDrawText [lindex $id 0] [lindex $id 1] \
		        "[lindex [lindex $id 2] 0]:[lindex [lindex $id 2] 2]:[format 0x%x [lindex [lindex $id 2] 1]]"
		    draw_frame2 [lindex $id 0] [lindex $id 1] $wingsize 1 
		 }
	      }	     
              display $mosaic " Rejected wing stars in filter $filter" "" 1
	      regDel $mosaic
	   }
      }
   }; ### end of WING loop over filters ###


   ################################
   ######## POSTAGE STAMPS ######## 
   write_mem mem "starting PSF part"
   verb_echo 3 "    postage stamps..." 



   if {[info exists psFieldOld]} {
       set psFieldOldFile $psFieldOld
   } else {
       echo " ** You must specify an old psField file in your psPlan.par **"
       echo " **               by using keyword psFieldOld               **"
       return
   }

   catch {unset psPSFfile}
   set ncomp [getsoftpar ncomp] 
   set nrow_b [getsoftpar nrow_b]
   set ncol_b [getsoftpar ncol_b]
   set maxNSR [getsoftpar maxNSR]
   set border [getsoftpar border]
   # parameters for checking whether KL PSF went wild
   set apCorr_min [getsoftpar apCorr_min]
   set apCorr_max [getsoftpar apCorr_max] 
   # for memory reporting 
   set Nmem [expr ($endRun - $startRun) / 5]
   if {$Nmem < 1} {set Nmem 1}

   foreach filter $filterlist {
      set PSFfile($filter) 0
      set dummy_psf($filter) 0
      set id0($filter) 0   
   }


   set LastRead -1
   set LastFreed -1

   #################################
   ### main loop over each field ###
   ################################# 
   loop field $startRun [expr $endRun+1] {
      verb_echo 9 "     ------------------------------"
      verb_echo 3 "       field $field" 
      set field0  [expr $field-$startRun]
      verb_echo 7 "WORKING ON FIELD $field"
      write_mem mem "starting FIELD $field"

      ## get bias vector
      foreach filter $filterlist {  
          # read bias file for this field and filter
          if {$read_psBB_files} {
              set BBfile [get_bias_filename $outputDir $run $filter $camCol $field]
          } else {
	      set BBfile [get_bias_filename $biasDir $run $filter $camCol]
          }
          set BBvector($filter,$field) [regNew]
          regReadAsFits $BBvector($filter,$field) $BBfile
      }

      if {[info exists psPSFfile]} {
          set psFieldOldFile $psPSFfile
      }

      # do we need this field for PSF?
      if {$ignore_PSF || $field < $startField || $field > $endField} {
          set PSFfield 0
      } else {
          set PSFfield 1

      }   

      if {$fakePSF} {set PSFfield 1}

      # is this field overlapping a PT patch?
      # n.b. if ignoring PT calibration PTfields is ""
      if {[lsearch $PTfields $field] < 0} {
         set PTfield 0
      } else {
         set PTfield 1
      }

      verb_echo 7 "  PSFfield = $PSFfield, PTfield = $PTfield"

      # do we need this field?
      if {!$PSFfield && !$PTfield} {
          foreach filter $filterlist {           
             set dummy_frame_info [frameinformationNew 0]
             handleSet $dummy_frame_info.frame $field
             handleSet $dummy_frame_info.filter $filter
             chainElementAddByPos $frameinfolist($filter) $dummy_frame_info TAIL AFTER
          }
         continue
      }


      # set up the KL basis window boundaries
      set wLeft  [expr $field - $nHalfWin]  
      set wRight [expr $field + $nHalfWin]
      # take care of edge effects 
      if {$wLeft < $startRun} {
          set wLeft $startRun
          set wRight [expr $wLeft + 2*$nHalfWin]
      } 
      if {$wRight > $endRun} {
          set wRight $endRun
          set wLeft [expr $wRight - 2*$nHalfWin]
      } 
      if {$wLeft < $startRun} {
          set wLeft $startRun
      } 
      # set up the PSF coeffs window boundaries
      set cLeft  [expr $field - $nHalfWinCoeff]  
      set cRight [expr $field + $nHalfWinCoeff]
      set master_row_max $master_row_max0
      set master_row_min $master_row_min0
      # take care of edge effects 
      if {$cLeft < $startRun} {
          set cLeft $startRun
          set cRight [expr $cLeft + 2*$nHalfWinCoeff]
          set master_row_min 0
          set master_row_max $nrowperframe
      } 
      if {$cRight > $endRun} {
          set cRight $endRun
          set cLeft [expr $cRight - 2*$nHalfWinCoeff]
          set master_row_min 0
          set master_row_max $nrowperframe
      } 
      if {$cLeft < $startRun} {
          set cLeft $startRun
      } 

      # XXX
      set master_row_max $master_row_max0
      set master_row_min $master_row_min0
    
      # sanity checks
      assert { $wLeft >= $startRun } 
      assert { $wRight <= $endRun } 
      assert { [expr $wLeft + 2*$nHalfWin] >= $wRight } 
      assert { [expr $cLeft + 2*$nHalfWinCoeff] >= $cRight } 
      assert { $cLeft >= $wLeft }
      assert { $cRight <= $wRight }

      if {!$PSFfield && $PTfield} {
          set wLeft $field
          set wRight $field
      }

      ## if needed read new data, clean old data... 
      loop fieldRead $wLeft [expr $wRight + 1] {

         #do we need to read this field ?     
         if {$fieldRead <= $LastRead} {
             # this field has already been read
             continue 
         } else {
             set LastRead $fieldRead
             set readOK($fieldRead) 1
	 }

         verb_echo 8 "       READING for field = $field, fieldRead = $fieldRead"

         handleSet $fieldparams.fieldnum $fieldRead
         set star1clist($fieldRead) [chainNew STAR1C] 
	 set fieldRead0 [expr $fieldRead - $startRun]         

         # loop over filters
         foreach filter $filterlist {  
            set ifilter [lsearch $filterlist $filter]
            set ccdrow  [exprGet $ccdpars($filter).iRow] 

            # if fieldRead != field, then we need bias vectors 
            if {$fieldRead != $field} {
                ## read bias file for this field and filter
                if {$read_psBB_files} {
                   set BBfile [get_bias_filename $outputDir $run $filter $camCol $fieldRead]
                } else {
	           set BBfile [get_bias_filename $biasDir $run $filter $camCol]
                }
                set BBvector($filter,$fieldRead) [regNew]
                regReadAsFits $BBvector($filter,$fieldRead) $BBfile
	    }
     
	    # get the field parameters for this filter
            set cframe [chainElementGetByPos $calib1byframelist $fieldRead0]
	    set calib [handleBindFromHandle [handleNew] *$cframe.calib<$ifilter>]
            set sky [exprGet $calib.sky]
            set skyerr [exprGet $calib.skyerr]
            set gain [binregionInterpolate *$fieldparams.frame<$ifilter>.gain 0 0]
            set dark_var [binregionInterpolate $fieldparams.frame<$ifilter>.dark_variance 0 0]

            # allocate new chains
            set starlist($fieldRead,$filter) [chainNew STAR1]
            set ps_rejects($fieldRead,$filter) [chainNew STAR1]
            set allstarmasks($fieldRead,$filter) [chainNew OBJMASK]
            set starmasks($fieldRead,$filter) [chainNew OBJMASK]
            set rej_masks($fieldRead,$filter) [chainNew OBJMASK]
            # read PSF regions
	    set pstamps($fieldRead,$filter) \
                [ps_read $fangDir $run $camCol $fieldRead $filter]
            # display raw postage stamps
	    if {$display && $display_raw} { 
	       if [info exists ids] { unset ids }
	       loop i 0 [chainSize $pstamps($fieldRead,$filter)] {
		  lappend ids $i
	       }
	       set mosaic \
		   [ps_mosaic $pstamps($fieldRead,$filter) $stampsize 0 ids]

	       catch {
		  saoReset;
		  foreach id $ids { 
		     # print star number
		     saoDrawText [lindex $id 0] [lindex $id 1] \
			 "[lindex [lindex $id 2] 0]"
		     draw_frame [lindex $id 0] [lindex $id 1] $stampsize 1 -i  
		  }
	       }
	       
	       display $mosaic \
		   " raw postage stamps for field $fieldRead filter $filter"
	       regDel $mosaic
	    }
	    set nstamps($fieldRead,$filter) [exprGet $pstamps($fieldRead,$filter).nElements]
            if {$filter == [lindex $filterlist 0]} {
                verb_echo 4 \
                "        $nstamps($fieldRead,$filter) postage stamps read for field $fieldRead"
            }

            if {$fakePSF} {continue}

            ### correct stamps and extract profiles -> make star1clist   
            set id0($filter) 0
	    if {$ifilter == 0} {write_mem mem "making star1clist"}
            set biases($filter) $BBvector($filter,$fieldRead)  

	    eval makeStar1clist $star1clist($fieldRead) \
                        $allstarmasks($fieldRead,$filter) $pstamps($fieldRead,$filter) \
                        $ifilter $nfilter $id0($filter) $fieldRead $flatvector($filter)  \
	                $biases($filter) $ccdpars($filter) $bias_scale $calib $stampsize \
                        $sigma_multi $sigma_badsky $second_peak 0 $star1_aperature \
                        $psf_def_rad $cr_min_s $cr_min_e $cr_cond3 $cr_cond32 \
                        $fieldparams.frame<$ifilter> $psf_threshold "-region" $keep_symmetry
	    #
	    # time to display corrected frames now that we have them
	    #
	    if {$display && $display_corrected} {# show corrected stamps.
	       set mosaic \
		 [star1_mosaic $star1clist($fieldRead) $stampsize ids $ifilter]
	       catch {
		  saoReset;
		  foreach id $ids { 
		     # print id frame, and flags
		     saoDrawText [lindex $id 0] [lindex $id 1] \
			 "[lindex [lindex $id 2] 0]:[lindex [lindex $id 2] 2]:[format 0x%x [lindex [lindex $id 2] 1]]"
		     draw_frame [lindex $id 0] [lindex $id 1] $stampsize 1 -i  
		  }
	       }	     
	       display $mosaic \
		   " corrected postage stamps field $fieldRead filter $filter"\
		   "" 1
	       regDel $mosaic
	    }

            # we don't need raw stamps any more from this point on
            chainDestroy $pstamps($fieldRead,$filter) regDel 

            if {$filter == [lindex $filterlist 0]} {              
                set Nstar1clist($fieldRead) [chainSize $star1clist($fieldRead)]
                verb_echo 9 "       Nstar1clist($fieldRead) = $Nstar1clist($fieldRead)" 
            } else {
                assert { $Nstar1clist($fieldRead) == [chainSize $star1clist($fieldRead)]}
            }
            set id0($filter) [expr $id0($filter) + $Nstar1clist($fieldRead)]         


            # range of current stars
            set CS1 0 
	    set CS2 [expr $Nstar1clist($fieldRead) -1]
	    #
            # split star1clist into starlist & ps_rejects
	    # (i.e. reject obviously bad stamps)
	    #
	    # The second branch of this test is used to read files with
	    # known status from another version of PSP.
	    #
	    # It is ONLY to be used for debugging
	    #
	    if 1 {
	       makeStarlist $star1clist($fieldRead) \
		   $starlist($fieldRead,$filter) \
		   $ps_rejects($fieldRead,$filter) $ifilter $CS1 $CS2 \
		   $PSF_star1c_require_mask $PSF_star1c_ignore_mask \
		   $PSF_star1_require_mask $PSF_star1_ignore_mask
	     } else {
		 error "You shouldn't be here"
		 set read_makeStarlist 1
		 set file "makeStarlist-$fieldRead.dat"
		 echo "Reading good/bad stars from $file"
		 if [info exists star_status] { unset star_status }
		 set fd [open $file "r"]
		 while {[gets $fd line] > 0} {
		     set i -1
		     set _id [lindex $line [incr i]]
		     set _frame [lindex $line [incr i]]
		     set _starid [lindex $line [incr i]]
		     set _flags [lindex $line [incr i]]
		     set _good [lindex $line [incr i]]
		     
		     set star_status($_frame:$_starid) $_good		     
		 }
		 close $fd

		 set ch $star1clist($fieldRead)
		 loop i 0 [chainSize $ch] {
		     set el [chainElementGetByPos $ch $i]
		     set star \
			 [handleBindFromHandle [handleNew] *$el.star1<$ifilter>]
		     set ident [exprGet $star.frame]:[exprGet $star.starid]

		     if ![info exists star_status($ident)] {
			 error "$el is not in $file"
		     }

		     if {$star_status($ident) && [exprGet (int)$star.cprof] == 0} {
			 echo "Star $ident is good in v5_2, no cprof in v5_3 [exprGet -flat -unfold $el]"			 
			 if 1 {		# Give up now? Otherwise add to rejects
			     error $el
			 }
			 lappend bbad $star;
			 unset star_status($ident)
			 chainElementAddByPos $ps_rejects($fieldRead,$filter) \
			     $star TAIL AFTER
			 continue
		     }


		     if $star_status($ident) {
			 chainElementAddByPos $starlist($fieldRead,$filter) \
			     $star TAIL AFTER
		     } else {
			 chainElementAddByPos $ps_rejects($fieldRead,$filter) \
			     $star TAIL AFTER
		     }
		     unset star_status($ident)

		     handleDel $star; handleDel $el
		 }
		 assert {[llength [array names star_status]] == 0}

		 if [info exists bbad] {
		     foreach b $bbad {
			 mtv *$b.object->region
			 puts -nonewline "continue  "
			 if {[gets stdin] == "q"} {
			     error $b
			 }
			 handleDel $b
		     }
		     unset bbad
		 }
	     }

            # select the masks for good PS stars
            get_masks star1clist starlist allstarmasks starmasks \
		rej_masks $filter $ifilter $fieldRead $CS1 $CS2 $CS1 $CS2

            ## generate PS list for photometric calibration
            set flags [expr $STAR1(CR) | $STAR1(BADPSF) | \
			   $STAR1(BADSKY) | $STAR1(ASYMMETRIC)]
	    
            if {!$ignore_calibration && $PTfield} {
                write_mem mem "processing PT stars"
                set MTstarlist_aux [chainNew STAR1PC]            
                set MTrejects [chainNew STAR1]
                makeStarPClist $star1clist($fieldRead) $MTstarlist_aux $MTrejects \
                               $ifilter $CS1 $CS2 $flags
                chainDel $MTrejects 
                loop k 0 [chainSize $MTstarlist_aux] {
                    set star1pc [chainElementGetByPos $MTstarlist_aux $k]
                    chainElementAddByPos $MTstarlist($filter) $star1pc
                }
                chainDel $MTstarlist_aux
            }
            # prints out table with sx1, sx2, width, ...
            verb_echo 9 "GOOD STAMPS:"
            log_starlist $starlist($fieldRead,$filter) $filter
	    if {$display && $display_psf} {
		set mosaic [star1_mosaic $starlist($fieldRead,$filter) $stampsize ids]
	       if [catch {
		  saoReset
		  foreach id $ids { 
		     # print id frame, and flags
		     saoDrawText [lindex $id 0] [lindex $id 1] \
			 "[lindex [lindex $id 2] 0]:[lindex [lindex $id 2] 2]:[format 0x%x [lindex [lindex $id 2] 1]]"
		     draw_frame [lindex $id 0] [lindex $id 1] $stampsize 1 -i
		  }
	       } msg] { echo $msg }	     

	       if {[exprGet $mosaic.nrow]*[exprGet $mosaic.ncol] > 1} {
		  display $mosaic " Good stars in field $fieldRead filter $filter (starid:frame:flags)" "" 1
	       }
	       regDel $mosaic
	    }

            verb_echo 10 "REJECTED STAMPS:"
            log_starlist $ps_rejects($fieldRead,$filter) $filter

	    if {$display && $display_psf > 1} {
	       set mosaic [star1_mosaic $ps_rejects($fieldRead,$filter) $stampsize ids]
	       if [catch {
		  saoReset
		  foreach id $ids { 
		     # print id frame, and flags
		     saoDrawText [lindex $id 0] [lindex $id 1] \
			 "[lindex [lindex $id 2] 0]:[lindex [lindex $id 2] 2]:[format 0x%x [lindex [lindex $id 2] 1]]"
		     draw_frame [lindex $id 0] [lindex $id 1] $stampsize 1 -i
		  }
	       } msg] {
		  echo $msg
	       }

	       if {[exprGet $mosaic.nrow]*[exprGet $mosaic.ncol] > 1} {
		  display $mosaic " Rejected stars in field $fieldRead filter $filter" "" 1
	       }
	       regDel $mosaic
	    }

            # clean
	    handleDel $calib
	    handleDel $cframe

            # if fieldRead != field, then we need bias vectors 
            if {$fieldRead != $field} {
               regDel $BBvector($filter,$fieldRead)
	    }

         }; # end of loop over filters


      }; # end of loop over fieldRead

      # needed for output
      set cframe [chainElementGetByPos $calib1byframelist $field0]    
      assert { [exprGet $cframe.field] == $field }
 
      ### all set for this field in all filters, now determine 
      #   KL PSF etc. for each filter
      if {!$ignore_PSF && $PSFfield} {
         write_mem mem "determining PSF"
      }

      foreach filter $filterlist {

        # skip when ignoring PSF
         if {$ignore_PSF || !$PSFfield} {
             set dummy_frame_info [frameinformationNew 0]
             handleSet $dummy_frame_info.frame $field
             handleSet $dummy_frame_info.filter $filter
             chainElementAddByPos $frameinfolist($filter) $dummy_frame_info TAIL AFTER
	     set psfBasis($filter) ""
             continue
	 } 

         if {$fakePSF || $ignore_PSF || !$PSFfield} {continue} 
   
         set ifilter [lsearch $filterlist $filter]
         verb_echo 7 "  determining PSF for field $field, filter $filter"

         # for counting OE transients
         set nOEtransients($ifilter) 0


         ## collect all PSF stars from the specified window
         set star_list_aux [chainNew STAR1] 
         # loop over all fields in this window
         loop fieldW $wLeft [expr $wRight + 1] {
            # is this the left edge of the coeff window?
	    if {$fieldW  == $cLeft} {
                set c_f1 [chainSize $star_list_aux]
	    }
            # are we within coeff window?
	    if {$fieldW >= $cLeft && $fieldW <= $cRight} {
                set coeffWindow 1
	    } else {
                set coeffWindow 0
            } 
            # add all stars from this frame
            loop k 0 [chainSize $starlist($fieldW,$filter)] {
	       set starlet [chainElementGetByPos $starlist($fieldW,$filter) $k]
	       if {$coeffWindow} {
                   handleSet $starlet.used4coeffs 0
               } else {
                   handleSet $starlet.used4coeffs -1
               }
               chainElementAddByPos $star_list_aux $starlet TAIL AFTER
            }
            # is this the right edge of the coeff window?
	    if {$fieldW == $cRight} {
                set c_f2 [expr [chainSize $star_list_aux] - 1]
	    }
         }
 
         set n1 0; set n2 [expr [chainSize $star_list_aux] - 1] 
         verb_echo 4 "  initial N stars for basis = [chainSize $star_list_aux]"
	 verb_echo 4 "  initial N stars for coeffs = [expr $c_f2 - $c_f1 + 1]"
         verb_echo 4 "  $Nstar1clist($field) stars are from this ($field) frame"

         # select acceptable PSF stars 
         if {$n2 < $n1} {
	    set dummy_frame_info [frameinformationNew 0]
	    handleSet $dummy_frame_info.frame $field
	    handleSet $dummy_frame_info.filter $filter
	    chainElementAddByPos \
		$frameinfolist($filter) $dummy_frame_info TAIL AFTER
	 } else {
	     eval selectPsfStars $star_list_aux $calib1byframelist frameinfolist \
		 \"$filterlist\" $ifilter $field0 $n1 $n2 $ncellclip \
		 $nbadcell $maxdev \
		 $nsigma_sigma $nsigma_QU $nsigma_RMS_rad $niter $min_starnum 
	     #
	     # This overrides selectPsfStars' decisions.  For debugging ONLY
	     #
	     if 0 {
		 error "Don't call me"
		 set file "selectPsfStars-$field.dat"
		 echo "Reading good/bad status for stars from $file"
		 if [info exists star_status] { unset star_status }
		 set fd [open $file "r"]
		 while {[gets $fd line] > 0} {
		     set i -1
		     set _id [lindex $line [incr i]]
		     set _frame [lindex $line [incr i]]
		     set _starid [lindex $line [incr i]]
		     set _flags [lindex $line [incr i]]
		     set _bad [lindex $line [incr i]]
		     
		     set star_status($_frame:$_starid) $_bad
		 }
		 close $fd

		 set Ngood 0
		 set frame_info \
		     [chainElementGetByPos $frameinfolist($filter) $field0]  
		 loop i $n1 [expr $n2 + 1] {
		     set star [chainElementGetByPos $star_list_aux $i]
		     set ident [exprGet $star.frame]:[exprGet $star.starid]

		     if ![info exists star_status($ident)] {
			 if [info exists read_makeStarlist] {
			     error "$ident ($star) is not in $file"
			 }
			 echo "$ident ($star) is not in $file"
			 set star_status($ident) 1
		     }

		     handleSet $star.badstar $star_status($ident)
		     handleSet $frame_info.starflags<$i> $star_status($ident)

		     if ![exprGet $star.badstar ] {
			 incr Ngood
		     }
		     unset star_status($ident)
		     handleDel $star
		 }
		 if [info exists read_makeStarlist] {
		     assert {[llength [array names star_status]] == 0}
		 }

		 handleSet $frame_info.NgoodBasis $Ngood
		 handleDel $frame_info
	     }
         }
         # we have a frame info now, store the number of raw stamps
         set frame_info [chainElementGetByPos $frameinfolist($filter) $field0]  
         handleSet $frame_info.Nallstars [chainSize $star_list_aux]
         handleSet $frame_info.Nframestars $Nstar1clist($field)
         verb_echo 5 "  done with selectPsfStars for field $field, filter $filter "
         verb_echo 5 "  after selectPsfStars Nstars for basis = \
                        [exprGet $frame_info.NgoodBasis]"

         # get KL representation for this set of stars
	 verb_echo 8 "calling starlist2KLpsf"
         # write_mem mem "determining KL PSF"

	 set scale 0

	 if {$forceDefaultPsf} then {
	     verb_echo 2 "Forcing Default PSF, field $field - filter $filter"
	     set psfBasis($filter) [set_default_PSF $filter $ifilter $psFieldOldFile $cframe]
	 } else {
	     set psfBasis($filter) [starlist2KLpsf $star_list_aux $n1 $n2 $c_f1 $c_f2 $psfKLbasis_Nmax \
					$field $scale $master_row_min $master_row_max $border    \
					$maxNSR $ncomp $nrow_b $ncol_b $nrowperframe $ncolperframe \
					$scan_overlap $filter $apCorr_min $apCorr_max $psFieldOldFile \
					$cframe $ifilter $frame_info $display_KLpsf $display_KLresiduals]
	 }
	     

	 if {[exprGet $frame_info.NgoodBasis] > $psfKLbasis_Nmax} {
              handleSet $frame_info.NgoodBasis $psfKLbasis_Nmax
         }
         verb_echo 5 "  done with starlist2KLpsf for field $field, filter $filter "
         if {$psfBasis($filter) == ""} {
             echo "returned from starlist2KLpsf w/o PSF_BASIS"
             error "what to do now? temporarily will die..."
         }
 
         # display mosaic of PSFs 
         if {$display && $display_KLpsf} {
	    if [catch {
	       set mosaic [psf_grid $psfBasis($filter) 10 15]
	    } msg] {
	       echo "$msg"
	    } else {
	       catch {saoReset}
	       display $mosaic "KL PSF for frame $field, filter $filter"
	       regDel $mosaic
	    }
         }


         if {$clip_apCrat != 0} {
             # check that reconstructed PSF actually resembles the stars
             verb_echo 8 "  calling QAforPsfStars (clip_apCrat) for field $field, filter $filter "
             eval QAforPsfStars $star_list_aux $psfBasis($filter) $calib1byframelist frameinfolist \
                            \"$filterlist\" $ifilter $field0 $nrowperframe $ncolperframe \
                            $gain $dark_var $sky $skyerr $n1 $n2 $apCrat_qmin $apCrat_qmax
             verb_echo 5 "  done with QAforPsfStars for field $field, filter $filter "
             verb_echo 5 " after clipping QAforPsfStars Nstars for basis = \
                            [exprGet $frame_info.NgoodBasis]"

             # free the first PSF_BASIS
             psfBasisDel $psfBasis($filter)

             # determine KL basis again

	     if {$forceDefaultPsf} then {
		 verb_echo 2 "Forcing Default PSF (2nd time), field $field - filter $filter"
		 set psfBasis($filter) [set_default_PSF $filter $ifilter $psFieldOldFile $cframe]
	     } else {
		 set psfBasis($filter) [starlist2KLpsf $star_list_aux $n1 $n2 $c_f1 $c_f2 $psfKLbasis_Nmax \
					    $field 0 $master_row_min $master_row_max $border    \
					    $maxNSR $ncomp $nrow_b $ncol_b $nrowperframe $ncolperframe \
					    $scan_overlap $filter $apCorr_min $apCorr_max $psFieldOldFile \
					    $cframe $ifilter $frame_info $display_KLpsf]
	     }
	     if {[exprGet $frame_info.NgoodBasis] > $psfKLbasis_Nmax} {
                 handleSet $frame_info.NgoodBasis $psfKLbasis_Nmax
             }
             verb_echo 5 "  done with second starlist2KLpsf for field $field, filter $filter "
             if {$psfBasis($filter) == ""} {
                echo "returned from second starlist2KLpsf w/o PSF_BASIS"
                error "what to do now? temporarily will die..."
             }

             # display mosaic of PSFs 
             if {$display && $display_KLpsf} {
                 if [catch {
                     set mosaic [psf_grid $psfBasis($filter) 10 15]
                     display $mosaic "frame $field, filter $filter"
                     regDel $mosaic
                 } msg] {
	             echo "$msg"
                 }
             }

         }
         # check that reconstructed PSF actually resembles the stars
         verb_echo 8 "  calling QAforPsfStars for field $field, filter $filter "
         # write_mem mem "QA for PSF stars"

         eval QAforPsfStars $star_list_aux $psfBasis($filter) $calib1byframelist frameinfolist \
                            \"$filterlist\" $ifilter $field0 $nrowperframe $ncolperframe \
                            $gain $dark_var $sky $skyerr $n1 $n2 0.0 1.0 
         verb_echo 5 "  done with final QAforPsfStars for field $field, filter $filter "
         verb_echo 4 " " 
         verb_echo 4 "   after final QAforPsfStar N stars from this frame = \
                        [exprGet $frame_info.Nframestars]"  
         verb_echo 4 "   after final QAforPsfStar N good basis stars = \
                        [exprGet $frame_info.NgoodBasis]"
         verb_echo 4 "   after final QAforPsfStar N stars for coeffs = \
                        [exprGet $frame_info.NgoodCoeffs]"  
         verb_echo 4 "  "  
 
         # fit analytic functions to the center PSF
	 if $ignore_wing_stars {
	    set winglistArr "NULL"
	 } else {
	    set winglistArr winglist
	 }
         eval measurePsf $psfBasis($filter) $winglistArr wingmasks \
	     $calib1byframelist frameinfolist $gain $dark_var \"$filterlist\" \
	     $ifilter $sky $skyerr $err_min $err_max $field0 $nrowperframe \
	     $ncolperframe $nowing_option

         # test for odd/even transients, and if found increment PSP_STATUS
         set maxOEdiffChangeCounts 0.5
         if {$field > $startRun} {
             set cframeOld [chainElementGetByPos $calib1byframelist \
				[expr $field - $startRun -1]]
             incr nOEtransients($ifilter) \
		 [testOEtransients $cframe $cframeOld $bias_scale $ifilter \
		      $maxOEdiffChangeCounts]
         }
         # write PSF_BASIS to output file
         set psPSFfile [write_psPSF $filter $field $psfBasis($filter) $cframe \
			    $bias_scale $astrom_node $astrom_incl] 

         # don't need this one any more
         handleDel $frame_info

         # if requested dump detailed information on PSF stars
         if {[info exists dump_PSFstars] && $dump_PSFstars} {
             set open_f 0; set close_f 0
             if {$field == $startField} {set open_f 1}
             if {$field == $endField} {set close_f 1}
	     verb_echo 5 "dumping [chainSize $star_list_aux] stars for $field $filter ($n1-$n2)"
	     set PSFfile($filter) \
		 [print_PSFlist $open_f $close_f $PSFfile($filter) \
		      $star_list_aux $n1 $n2 $run $camCol $outputDir $filter \
		      $startRun $nrowperframe $master_row_min $master_row_max \
		      $field]
             verb_echo 9 "    dumped PSF stars" 
         }  

         # display clipping statistics
         show_clipping $field0 $display $display_PSFstars $display_clipping \
	     $display_profiles $star_list_aux $stampsize \
	     $frameinfolist($filter) $run $camCol $filter $outputDir \
	     $nsigma_sigma $nsigma_QU $nsigma_RMS_rad

         # done with these stars...
         chainDel $star_list_aux

         # test whether there are too many odd/even transients
         set NmaxOEtransients 2
         if {$nOEtransients($ifilter) > $NmaxOEtransients} {
             echo "  There are already $nOEtransients($ifilter) for filter $filter"
             echo "  at field = $field (run=$run, camCol=$camCol)."
             echo "  PSP strongly suspects that Zeljko has a bug; or the camera went wild..."
             echo "  (though there is a non-zero chance that dp played with odd/even pipeline)"
             error 
         }

      }; # end of loop over filters for this field's PSF determination 

         
      #
      # Generate psFang files with positions corrected using the KL PSF
      #
      if $write_psFang {
	 # write_mem mem "making scparambs"
	 set dummy_frameinfos [make_frameinfos $fangDir $run $filterlist $camCol \
				   $field $field scparambs]
	 chainDestroy $dummy_frameinfos frameinfoDel
	 
	 if {[info exists dump_psFangQA] && $dump_psFangQA} {
             set QAoutfile $outputDir/psFangCorr-$run-$camCol.dat
	     if {$field == $startRun} { 
                set line "# f  field  psfWidth rowc  colc  rowErr \
                             colErr  rowBias colBias  counts "
                set outf [open $QAoutfile w]
                puts $outf $line
                close $outf
             }
         } else {
              set QAoutfile ""
         }
	 #
	 # The badstar values aren't set for all stars, only those that
	 # pass the PSF selection.  We need them for _all_ stars in order
	 # to decide which stars belong in the psFang files
	 #
	 setBadstarFlagFromChain $star1clist($field) frameinfolist $filterlist\
	     -nsigma_sigma $nsigma_sigma -nsigma_QU $nsigma_QU \
	     -nsigma_RMS_rad $nsigma_RMS_rad \
	     -ncellclip $ncellclip -nbadcell $nbadcell -maxdev $maxdev

         set minCounts [getsoftpar psFang_minCounts]
	 set ignore_ssc_statusFit [getsoftpar ignore_ssc_statusFit]

	 write_psFang $outputFangDir star1clist scparambs psfBasis \
	     $cframe $run $camCol $filterlist $field $ignore_ssc_statusFit \
	     $psFangs_star1c_require_mask $psFangs_star1c_ignore_mask \
	     $psFangs_star1_require_mask $psFangs_star1_ignore_mask \
	     $minCounts $QAoutfile
      }

      # free SCPARAMBS chains and headers for this field
      if [info exists scparambs] {
	 foreach el [array names scparambs $field,*] {
	    if [regexp {hdr} $el] {
	       hdrDel $scparambs($el)
	    } else {
	       chainDestroy $scparambs($el) genericDel
	    }
	 }
      }


      #
      # this block is executed only when stuffing fake PSFs
      #
      if {$fakePSF} {
          foreach filter $filterlist {
   	     set ifilter [lsearch $filterlist $filter] 
             set psfBasisFake($filter) [set_default_PSF $filter $ifilter $psFieldOld $cframe]
             set psfFake($filter) [KLpsf2dgpsf $psfBasisFake($filter) 0 0 0 0 1 0]
             handleSetFromHandle $cframe.calib<$ifilter>->psf &$psfFake($filter)
             set psPSFfile [write_psPSF $filter $field $psfBasisFake($filter) $cframe $bias_scale \
                                    $astrom_node $astrom_incl]
          }
          foreach filter $filterlist {
   	     set ifilter [lsearch $filterlist $filter] 
             handleSet $cframe.calib<$ifilter>->psf 0
             dgpsfDel $psfFake($filter)
	     psfBasisDel $psfBasisFake($filter)
          }
      }


      # free PSF_BASISs for this field
      foreach filter $filterlist { 
	 if {!$fakePSF} {psfBasisDel $psfBasis($filter)}
      }

      # free old chains, if they exist
      set fFree1 [expr $LastFreed + 1]
      set fFree2 $wLeft
      if {$field == $endField || (!$PSFfield && $PTfield)} {
         set fFree2 [expr $wRight + 1]
      }


      loop fieldFree $fFree1 $fFree2 {

          if {[info exists readOK($fieldFree)] && $readOK($fieldFree)} {
               set free_it 1
               unset readOK($fieldFree) 
               set LastFreed $fieldFree
          } else {
               set free_it 0
          }
  
          if $free_it {
             verb_echo 8 "       CLEANING for field = $field, fieldFree = $fieldFree"
             write_mem mem "cleaning for field $fieldFree"
             chainDestroy $star1clist($fieldFree) star1cDel
             foreach filter $filterlist {
                # chains with STAR1s
                chainDestroy $starlist($fieldFree,$filter) star1Del 
                chainDestroy $ps_rejects($fieldFree,$filter) star1Del
                # chains with masks
                chainDel $starmasks($fieldFree,$filter) 
                chainDel $rej_masks($fieldFree,$filter) 
                chainDestroy $allstarmasks($fieldFree,$filter) objmaskDel
             }
             write_mem mem "cleaning done"
          }
      }; # end of freeing part    

      # free output for this frame
      handleDel $cframe
 
      # free biases that was needed to correct stars from this field
      foreach filter $filterlist { 
         regDel $BBvector($filter,$field)
      }

      if $defragment_memory {
	 if {![expr $field%$nFieldDefrag]} {
	    defragment_memory
            write_mem mem "defragment_memory"
	 } else {
	    verb_echo 3 Skipping memory defrag
	 }
      }
   }; ### end of main loop over each field ### 
   ############################################


   # free flatfield vectors
   foreach filter $filterlist {
       regDel $flatvector($filter)       
   }
   verb_echo 5 "[cpuTime]"  

   # debugging 
   if {!$ignore_PSF && [info exists dump_ApCorr] && $dump_ApCorr} {
      foreach filter $filterlist {
	 print_ApCorr $frameinfolist($filter) $run $camCol $outputDir $filter
      } 
   }  


# ======================================================================
# ===================  6) Calculate Photometric Parameters =============
# ======================================================================

   # print out
   verb_echo 1 " "
   verb_echo 1 "  Finding Photometric Parameters" 

   write_mem mem "photometric parameters"

   # make list to hold calibration for each patch
   set ctimes [chainNew CALIB1BYTIME]
   
   if $ignore_calibration {
    loop field $startRun [expr $endRun+1] {
       handleSet $fieldparams.fieldnum $field
       # get the field parameters
       loop ifilter 0 $nfilter {
	  set cframe [chainElementGetByPos $calib1byframelist \
			  [expr $field-$startRun]]
	  set calib [handleBindFromHandle [handleNew] *$cframe.calib<$ifilter>]
	  handleSetFromHandle $calib.flux20 \
	      $oldcal.calib<$ifilter>->flux20
	  handleSetFromHandle $calib.flux20Err \
	      $oldcal.calib<$ifilter>->flux20Err
	  handleDel $cframe; handleDel $calib;
       }
    }
 } else {
   # get the color-index reference filters for each filter in 'filterlist'
    get_color_index_and_slope $filterlist ref_filterlist color_term \
	color_term_errors
    set starsep [getsoftpar fpp_starsep]
    set sys_ph_err [getsoftpar fpp_sys_ph_err]
    set max_err [getsoftpar fpp_max_err]
    set n_max [getsoftpar fpp_n_max]
    loop i 0 $nfilter { 
       set flux20_m($i) 0
       set flux20_c($i) 0
       set flux20_chi2($i) 0
    }
    
    # print out
    verb_echo 2 "  Done with pre-fpp"
    verb_echo 2 "  [cpuTime]"

   # run findPhotoParams
   set outstr [findPhotoParams $filterlist $ref_filterlist $color_term \
		   $color_term_errors MTstarlist $patches $extinctions \
		   $frameinfos $oldcal $starsep $sys_ph_err $max_err $n_max \
                   flux20_m flux20_c flux20_chi2 $ctimes $calib1byframelist]  

   # stuff frameinfolist with flux20 info from MT patches
   if {!$ignore_PSF} {
       add_calib $filterlist $startRun $ctimes frameinfolist
   }
  
   # print out
   verb_echo 2 "  Done with fpp"
   verb_echo 2 "  [cpuTime]"

   set patchcal [keylget outstr patchcal]
   set framecal [keylget outstr framecal]
   if {$patchcal == ""} {
      verb_echo 1 " "
      verb_echo 1 "  ** Did not calibrate any MT patches **"
   } else {
      if {$patchcal != $filterlist} {
	 verb_echo 1 "  Calibrated MT patches only in the these bands: $patchcal"
      }
   }
   if {$framecal == ""} {
      verb_echo 1 \
	  "  ** Used out of date patchcal. Do not have current calib in any band. **"
      } else  {
      if {$framecal != $filterlist} {
	 verb_echo 1 \
	     "  Have current photo calibration only in these bands: $framecal"
      }
   }

   verb_echo 7  "--------------------------"
   log_patchcal $ctimes PatchFileName 1  
   log_photoparams $calib1byframelist
   verb_echo 6  "--------------------------"

   verb_echo 7 "  [cpuTime]"
}; # end of possibly bypassed part

# ======================================================================
# ===================  7) Writing results ==============================
# ======================================================================

   # print out
   verb_echo 1 " "
   verb_echo 1 "  Computing Aperture Corrections" 
   write_mem mem "aper. corr."

   ### calculate wing correction from the composite profiles
   if !$ignore_PSF {
      # bias and error up to the 5th (nann_ap_frame) annulus
      if [catch {
          write_mem mem "doAperCorr"
	  set medApCorr [doAperCorr $filterlist $calib1byframelist $nann_ap_frame $nann_ap_run]} msg] {
          echo "ERROR: problems with calculating aperture correction in doAperCorr; setting to 0"
          echo "       $msg"
	  foreach f $filterlist {lappend medApCorr 0.0}
          #set medApCorr 0.0;
	  #This is a bug due to the line 
	  #below involving [lindex $medApCorr $ifilter]
      }
      verb_echo 1 "  Median aperture correction bias:" 
      foreach f $filterlist {
         set ifilter [lsearch $filterlist $f]
         set mac [format "%6.3f" [lindex $medApCorr $ifilter]]
         verb_echo 1 "    $f: $mac" 
      }   
      # bias from the 5th (nann_ap_frame) to the 7th (nann_ap_run) annulus
      if {1} {
         # using composite profile
         write_mem mem "stuffWC"
         if {[info exist ignore_comp_prof_test] && $ignore_comp_prof_test} {
               set fracBadCompProf 1.0
         } else {
	     set fracBadCompProf [expr 0.3 + 7.0/($nfields+10)]
         }
         if {[stuffWC $filterlist $calib1byframelist frameinfolist $nann_ap_frame \
                      $nann_ap_run $ignore_WC $fracBadCompProf] < 0} {
	    echo "ERROR: problems with calculating wing correction"
         }
      } else {
         # using median ratio flux7/flux5 for individual stars
         doWingCorr $filterlist $calib1byframelist
      }
   }
     
   # writing results to psField files
   set cframe [chainElementGetByPos $calib1byframelist 0]
   loop i 0 [llength $filterlist] {
      handleSet $cframe.calib<$i>->status $PSP_STATUS(OE_TRANSIENT)
   }

   ##### attach the aperture correction info and rename psPSF to psField files ###
   verb_echo 1 "  Writing Results" 
   write_mem mem "writing results"
   write_ps_results $calib1byframelist $ctimes $keep_psPSF_files
   write_mem mem "wrote ps_results" 
  
   # vector lists needed for QA plots
   set lbiasOEdiff ""; set rbiasOEdiff ""
   loop i 0 [llength $filterlist] {
      lappend lbiasOEdiff [vFromChain $calib1byframelist calib<$i>->lbiasOEdiff]
      lappend rbiasOEdiff [vFromChain $calib1byframelist calib<$i>->rbiasOEdiff]
   } 


# ======================================================================
# ===================  Cleaning up  =================================
# ======================================================================

   # print out
   verb_echo 1 " "
   verb_echo 1 "  Cleaning up" 
   write_mem mem "cleaning up"
   chainDestroy $calib1byframelist calib1byframeDel
   chainDestroy $ctimes calib1bytimeDel
   calib1bytimeDel $oldcal

   measureObjUnset $fieldparams
   fieldparamsDel $fieldparams; unset fieldparams
   
   if [info exists frameinfos] {
      chainDestroy $frameinfos frameinfoDel
   }
   if {!$ignore_PSF} {
       chainDestroy $wingstar1clist star1cDel
   }
   if {!$ignore_calibration} {
        patchesDestroy $patches
        chainDestroy $extinctions extinctionDel
   }
   foreach filter $filterlist {
      if {!$ignore_PSF} {
          chainDel $wingmasks($filter)  
          chainDel $rej_wingmasks($filter) 
          chainDestroy $winglist($filter) star1Del
          chainDestroy $wing_rejects($filter) star1Del
          chainDestroy $allwingmasks($filter) objmaskDel  
      } 
      if {!$ignore_calibration} {
          chainDestroy $MTstarlist($filter) star1pcDel 
      }
      ccdparsDel $ccdpars($filter) 
   }
   


# ================================================================
# ===================  QA plots  =================================
# ================================================================

   write_mem mem "plotting results" 
   # the following plots get data by reading psField files, as well as
   # from frameinfolist 
   if {!$ignore_PSF && !$ignore_plots} { 
      verb_echo 1 "  Plotting Results" 
      # by default only PS summary plots are produced
      # display XWINDOW summary plots when display_summary set to >0
      if {$display_summary > 0} {
          verb_echo 9 "plotting plot_pspstats (XWINDOW)" 
          plot_pspstats /XWINDOW $startField $endField $outputDir $run \
	      $camCol $filterlist frameinfolist flux20_m flux20_c flux20_chi2 \
                $lbiasOEdiff $rbiasOEdiff -rerun $rerun
      }
      # produce PS files with summary plots (unless display_summary set to <0)
      if {$display_summary > -1} {
          verb_echo 9 "plotting plot_pspstats (PS)"
          plot_pspstats /CPS $startField $endField $outputDir $run $camCol \
	         $filterlist frameinfolist flux20_m flux20_c flux20_chi2 \
                $lbiasOEdiff $rbiasOEdiff -rerun $rerun
      }
      # display rejection plots
      if {$display_rejection > 0} {
         verb_echo 9 "plotting plot_rejection (XWINDOW)"
         plot_rejection /XWINDOW $startField $endField $outputDir $run  \
	                        $camCol $filterlist frameinfolist $rerun
         if {$display_rejection > -1} {
             verb_echo 9 "plotting plot_rejection (PS)"
             plot_rejection /CPS $startField $endField $outputDir $run  \
	                      $camCol $filterlist frameinfolist $rerun
         }  
      }
   }   

   # when redoing calibration produce psPlots4 files
   if {$ignore_PSF && !$ignore_calibration && !$ignore_plots} { 
       verb_echo 1 "  Plotting PT calibration" 
       if {$display_summary > 0} {
           verb_echo 9 "plotting PT calibration (XWINDOW)"
           plot_PTcalib /XWINDOW $startField $endField $outputDir $run \
	                 $camCol $filterlist flux20_m flux20_c flux20_chi2
       }
       if {$display_summary > -1} {
           verb_echo 9 "plotting PT calibration (PS)"
           plot_PTcalib /CPS $startField $endField $outputDir $run \
	                 $camCol $filterlist flux20_m flux20_c flux20_chi2
       }
   }

   write_mem mem "doing QA"

   if {!$ignore_PSF} {
      # QA processor 
      set QAquantities [list sky drift sigma1 sigma2 b sigmap p0 beta  \
                          PTchi2 flux20]
      foreach quant $QAquantities {
         set w1($quant) [getQApar qa_$quant\_w1]
         set w2($quant) [getQApar qa_$quant\_w2]
         set f1($quant) [getQApar qa_$quant\_f1]
         set f2($quant) [getQApar qa_$quant\_f2]
      }
      set QAflag [doQA $logfile $startField $endField $outputDir $run $camCol \
	            $filterlist $QAquantities w1 w2 f1 f2 flux20_chi2]
   } else {
      set QAflag 0
   }

   # leftover cleaning (because of QA plots)
   vListDel $lbiasOEdiff
   vListDel $rbiasOEdiff
   foreach filter $filterlist {
       catch {chainDestroy $frameinfolist($filter) frameinformationDel}
   }
   
  

# ================================================================
# =================== Finish unsetup  ============================
# ================================================================



   write_mem mem "end of PSP"

   finiObjectFinder
   finiMeasureObj
   finiCorrectFrames
   finiProfileExtract
   finiCellFitobj
   fini_measure_objects

   verb_echo 2 "  [cpuTime]"
   #verb_echo 5 "  [handleList]"
   phRandomDel $rand; unset rand

   # free I/O structures
   if {[info exists schemaOK] && $schemaOK} {
      verb_echo 9 "    freeing I/O structures" 
      fitsBinForgetSchemaTrans NULL
      unset schemaOK 
   } 

   # check for memory corruption, if available in dervish
   if {[info commands memCheck] != ""} {
       assert {[memCheck] == ""}
   }

   meschachFini

   # check for memory leaks
   if {!$fakePSF} {
       assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}
   } else {
       if {[memBlocksGetRange $startMem [memSerialNumber]] != {}} {
           echo "leaking memory!"
       }
   }

   if {$logfile != ""} {
      close $logfile; unset logfile
   }
   write_mem mem "end of PSP"

   if {[info exists trace_memory] && \
	   $trace_memory != "" && $trace_memory != 0} {
      if {$trace_memory == 1} {
	 exec cp mem.trace mem.trace.$run.$camCol
      }
   }

   # if everything is OK, QAflag must be 0
   # for each triggered warning flag QAflag increases by 1
   # if any fatal flag is triggered QAflag = -1
   # details can be found in proc doQA (parameters set in psQApar.par)

   # from Jan 19 2001, v5_2_14 the return code is 0 even when there
   # are some warnings (i.e. QAflag>1) and 1 for fatal failures (QAflag<0)
   if {$QAflag < 0 || $failedFF} {
       set return_code 1
   } else {
       set return_code 0
   }
   return $return_code
}

##############################################################################

 
