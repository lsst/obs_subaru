#
# Code to handle coadds, either from Jim Annis et al. at FNAL, or
# David Schlegel's in Princeton
#
proc make_coadd_psFields {args} {
   global data_root openit

   set schlegel 0;			# Schlgel coadds
   set display_mosaic 0
   set warn_missing_psField 0;		# Warn, not die, if a psField is missing
   set opts [list \
		 [list [info level 0] "Retrieve the input files needed to make psField files for coadds,
 then average the input KL PSFs to generate PSFs for the coadds "] \
		 [list <run> INTEGER 0 coadd_run "Desired (coadded) run"] \
		 [list <camCol> INTEGER 0 coadd_camCol "Desired (coadded) camCol"] \
		 [list <startField> INTEGER 0 coadd_startField "Desired first (coadded) field"] \
		 [list {[endField]} INTEGER -1 coadd_endField \
		      "Desired last (coadded) field (default: startField)"] \
		 [list -rerun INTEGER -1 coadd_rerun "Desired (coadded) rerun"] \
		 [list -input_rerun INTEGER -1 input_rerun_arg "Input rerun (overrides idCoadd file)"] \
		 [list -filter STRING "r" filter "Desired filter"] \
		 [list -host STRING "" ssh_host "Use ssh to retreive needed files from this host"]\
		 [list -inputRoot STRING $data_root input_root "Input data root"] \
		 [list -filterlist STRING "u g r i z" filters "Filters to process"] \
		 [list -outDir STRING "" outDir \
		      "Directory to write psField files to (default: openit(psdir)"] \
		 [list -nrow INTEGER 7 npsf_r "Number of rows of PSFs when building KL basis"] \
		 [list -ncol INTEGER 9 npsf_c "Number of columns of PSFs when building KL basis"] \
		 [list -border INTEGER 10 border "include border pixels around image"] \
		 [list -ncomp INTEGER 4 ncomp "number of KL components"] \
		 [list -nrow_b INTEGER 3 nrow_b "Max order of spatial variation"] \
		 [list "-showBasis" INTEGER -1 fsao_basis \
		      "Display basis images on this saoimage"] \
		 [list "-display" CONSTANT 1 display_mosaic \
		      "Display reconstructed images"] \
		 [list "-idCoadd" STRING "" idCoadd \
		      "idCoadd file for (Annis et al.) coadds"] \
		 [list "-schlegel" CONSTANT 1 schlegel \
		      "Assume book-keeping for Schlegel's coadds (default)"] \
		 [list "-allowMissingPsField" CONSTANT 1 warn_missing_psField \
		      "Warn about missing psFields, but don't die"] \
		 [list -verbose INTEGER 1 verbose "Be chatty?"]\
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set fnal [expr [string compare $idCoadd ""] != 0]
   switch [expr $fnal + $schlegel] {
      0 {
	 set schlegel 1
      }
      1 {
	 ;				# OK
      }
      default {
	 error "Please choose a style of coadd (e.g. -idCoadd or -schlegel)"
      }
   }

   if {$coadd_endField < 0} {
      set coadd_endField $coadd_startField
   }
   #
   # Things that usually come from fpPlan files
   #
   global asTrans_format psField_format

   if ![info exists asTrans_format] {
      set asTrans_format {asTrans-%06d.fit	$run}
   }
   if ![info exists psField_format] {
      set psField_format {psField-%06d-%d-%04d.fit $run $camCol $field}
   }
   #
   # Fake measureObj inputs well enough to run KLpsf2dgpsf
   #
   set nann_ap_frame  5
   set nann_ap_run 7
   set ap_corr_run  0
   set ap_corr_runErr 0
   set fieldparams [fake_initMeasureObj $filters $nann_ap_frame $ap_corr_run]

   #Only cover this fraction of the image
   #So as to stay a few pixels away from the edge
   #which avoids a problem with getting the psf wrong at edge
   #due to discontinuous coverage
    
   #These are the numbers that I get from looking at the coverage maps
   #including run 94, Now using all camsols both N S strips - Dave Johnston 08/11/05
   #cols 50 to 1900 were covered by all runs, I added 5 pixels to be safer
   set buf_begin 50 
   set buf_end 148 
   set imfrac [expr 1.0-($buf_begin+$buf_end)/2048.0]
   #write these to a file so that we have a record
   set_run -rerun $coadd_rerun $coadd_run $coadd_camCol
   set blogfile "$openit(logdir)/edgeBuffers.dat"
   set blog [open $blogfile a]
   puts $blog "$buf_begin $buf_end $imfrac"
   close $blog 

   #
   # To work.
   #


   loop field $coadd_startField [expr $coadd_endField + 1] {
      set_run -rerun $coadd_rerun $coadd_run $coadd_camCol

      if {$outDir == ""} {
	 set outDir $openit(psdir)
	 if ![regexp "/objcs/$openit(camCol)" $outDir] {
	    error "You don't seem to have created a proper objcs directory; openit(psdir) == $openit(psdir)"
	 }
      }
      #
      # Get input asTrans file
      #
      astrom_read $field $field $openit(camCol) astroms $filters \
	  [format $openit(asdir)/asTrans-%s.fit $openit(run)] coadd_node coadd_incl

      foreach f $filters {
	 set coadd_trans($f) [chainElementRemByPos $astroms($f) 0]
	 
	 assert {[chainSize $astroms($f)] == 0}
	 chainDel $astroms($f)
      }

      set inputs {}
      if $fnal {
	 if ![file exists $idCoadd] {
	    error "I cannot find $idCoadd; sorry"
	 }
	 
	 set coadds [param2Chain $idCoadd ""]
	 if [info exists nrow_f] {
	    unset nrow_f
	 }

	 if [info exists inputs_array] {
	    unset inputs_array
	 }
	 loop i 0 [chainSize $coadds] {
	    set coadd [chainElementGetByPos $coadds $i]
	    if {[handleType $coadd] != "IDCOADD"} {
	       if $verbose {
		  echo Skipping [handleType $coadd]
		  continue
	       }
	    }

	    if {[exprGet $coadd.run] != $coadd_run ||
		[exprGet $coadd.camcol] != $coadd_camCol ||
		[exprGet $coadd.field] != $field} {
	       continue
	    }
	    
	    if ![info exists nrow_f] {
	       set dir "$data_root/corr/$coadd_camCol"
	       set input_file \
		   [get_corr_filename $dir $coadd_run [exprGet $coadd.filter] $coadd_camCol $field]

	       if [file exists $input_file] {
		  set reg [regReadAsFits [regNew] $input_file]
		  set nrow_f [exprGet $reg.nrow]
		  set ncol_f [exprGet $reg.ncol]
		  regDel $reg
	       } else {
		  set nrow_f 1489
		  set ncol_f 2048
	       }
	       
	       set coadd_rnoise [expr sqrt ([exprGet $coadd.dark_varEff])]; # Effective read noise
	       set coadd_gain  [exprGet $coadd.gainEff]; # Poisson var = (FLUX+SKYVAL)/GAIN
	       set coadd_skyval [exprGet $coadd.skyEff]; # Effective sky value
	    }

	    if {$input_rerun_arg >= 0} {
	       set input_rerun $input_rerun_arg
	    } else {
	       set input_rerun [exprGet $coadd.rerunIn]
	    }
	    #
	    # Include range of input fields +- 1; the +- 1 allows
	    # a +- 1/2 field overlap when solving for the spatial
	    # variation of the PSFs within a frame
	    #
	    loop j -1 [expr [exprGet $coadd.nFieldsIn] + 1] {
	       set key "[exprGet $coadd.rerunIn]:[exprGet $coadd.runIn]:[exprGet $coadd.camcolIn]:[expr [exprGet $coadd.field0In]+$j]:[exprGet $coadd.filter]"
	       set inputs_array($key) [exprGet $coadd.weight]
	    }
	 }

	 chainDestroy $coadds genericDel
	 #
	 # Restructure inputs array to be a list of fields, with all
	 # the weights from the different filters included
	 #
	 foreach key [array names inputs_array] {
	    set vals [split $key ":"]
	    set i -1
	    set input_rerun  [lindex $vals [incr i]]
	    set input_run    [lindex $vals [incr i]]
	    set input_camCol [lindex $vals [incr i]]
	    set input_field  [lindex $vals [incr i]]
	    set f            [lindex $vals [incr i]]

	    if {$f != $filter} {	# the primary filter
	       continue;
	    }

	    set weights {}
	    foreach f $filters {
	       lappend weights $inputs_array($input_rerun:$input_run:$input_camCol:$input_field:$f)
	    }

	    lappend inputs [list $input_rerun \
				$input_run $input_camCol $input_field $weights]
	 }
      } elseif $schlegel {
	 set file [format $openit(fielddir)/idR-%06d-${filter}$coadd_camCol-%04d.fits $coadd_run $field]
	 set reg [regReadAsFits [regNew] $file]
	 set nrow_f [exprGet $reg.nrow]; set ncol_f [exprGet $reg.ncol]
	 
	 set input_rerun [string trimright [hdrGetAsAscii $reg.hdr "RERUN"]]
	 
	 set coadd_rnoise [hdrGetAsDbl $reg.hdr "RNOISE"]; # Effective read noise [nMgy]
	 set coadd_gain  [hdrGetAsDbl $reg.hdr "GAIN"]; # Poisson var = (FLUX+SKYVAL)/GAIN [nMgy^2]
	 set coadd_skyval [hdrGetAsDbl $reg.hdr "SKYVAL"]; # Effective sky value [nMgy]
	 #
	 # Get a list of the input idR files
	 #
	 set weights {}
	 foreach f $filters {
	    lappend weights 1.0
	 }

	 set i 0
	 while 1 {
	    incr i
	    if [catch {
	       set input_file [hdrGetAsAscii $reg.hdr [format "FILE%03d" $i]]
	    } msg] {
	       break
	    }
	    regexp {^idR-0*([0-9]*)-.([1-6])-0*([0-9]*).fit} $input_file \
		{} input_run input_camCol input_field
	    
	    lappend inputs [list $input_rerun \
				$input_run $input_camCol $input_field $weights]

	    if {$verbose > 1} {
	       echo $input_file
	    }
	 }
	 #
	 # Cleanup input (coadd) data
	 #
	 regDel $reg
      }

      set coadd_dark_variance \
	  [expr sqrt(pow($coadd_rnoise, 2) + $coadd_skyval/$coadd_gain)]
      #
      # Figure out where to generate the coadded PSF information
      #
      if {$npsf_r == 1} {
	 set row0 [expr 0.5*$nrow_f]
	 set drow 1
      } else {
	 set row0 0
	 set drow [expr $nrow_f/($npsf_r - 1.0)]
      }
      if {$npsf_c == 1} {
	 set col0 [expr 0.5*$ncol_f]
	 set dcol 1
      } else {
	 set col0 0
	 set dcol [expr $ncol_f/($npsf_c - 1.0)]
      }

      set n -1
      loop i [expr -($npsf_r/2)] [expr $npsf_r + $npsf_r/2] {
	 loop j 0 $npsf_c {
	    incr n
	    set rowc_list($n) [expr int($row0 + $i*$drow + 0.5) + 0.5]
	    set colc_list($n) [expr int($col0 + $j*$dcol + 0.5)*$imfrac +$buf_begin + 0.5]
	 }
      }
      #
      # Start by setting the globals that write_psPSF/write_ps_results use (#!*^*Q&%@)
      #
      global run camCol startRun startField endField filterlist outputDir

      set run $coadd_run
      set camCol $coadd_camCol
      set startField $field
      set endField $field
      set startRun $startField
      set filterlist $filters
      set outputDir $outDir

      if {$verbose > 1} {
	 echo outDir = $outDir
      }
      #
      # Fake the calibration for the psField
      #
      global id_values
      set id_values(SSC_ID) "(not run)"

      global PSP_STATUS
      set calib1byframelist [chainNew CALIB1BYFRAME]
      
      loop field $startField [expr $endField+1] {
	 if [info exists psf_sum] {
	    unset npsf_sum; unset psf_sum
	    unset nap_corr_run_sum; unset ap_corr_run_sum; unset ap_corr_runVar_sum
	 }
	 #
	 # Add together the input PSFs at the points (rowc_list, colc_list)
	 # in the coadd field
	 #
	 foreach input $inputs {
	    set i -1
	    set input_rerun  [lindex $input [incr i]]
	    set input_run    [lindex $input [incr i]]
	    set input_camCol [lindex $input [incr i]]
	    set input_field  [lindex $input [incr i]]
	    set weights      [lindex $input [incr i]]
	    assert {[lindex $input [incr i]] == ""}
	    #
	    # Get the input psField file
	    #
	    set input_psField [get_psField_filename "" $input_run $input_camCol $input_field]
	    
	    set input_dir "$input_root/$input_run/$input_rerun/objcs/$input_camCol"
	    if ![file exists $input_dir/$input_psField] {
	       if $warn_missing_psField {
		  echo "$input_psField doesn't seem to exist; skipping"
		  continue
	       }
	       if ![file exists $input_dir] {
		  mkdir -path $input_dir
	       }
	       if {$ssh_host == ""} {
		  error "Can't find $input_dir/$input_psField: Please specify -host if you want me to scp files for you"
	       }
	       exec scp $ssh_host:$input_dir/$input_psField $input_dir/$input_psField
	    }
	    #
	    # and the input asTrans file
	    #
	    set input_asTrans [format "asTrans-%06d.fit" $input_run]
	    
	    set input_asdir "$input_root/$input_run/$input_rerun/astrom"
	    if $verbose {
	       if {$verbose > 1 || ![file exists $input_asdir/$input_asTrans]} {
		  echo "                       : $input_asdir/$input_asTrans"
	       }
	    }
	    
	    if ![file exists $input_asdir/$input_asTrans] {
	       if ![file exists $input_asdir] {
		  mkdir -path $input_asdir
	       }
	       if {$ssh_host == ""} {
		  error "Please specify -host if you want me to scp files for you"
	       }
	       exec scp $ssh_host:$input_asdir/$input_asTrans $input_asdir/$input_asTrans
	    }
	    #
	    # Find the data (locally)
	    #
	    set_run $input_run -rerun $input_rerun $input_camCol
	    #
	    # Read the psField file
	    #
	    set calibs [read_calib1byframe $field $input_dir/$input_psField $filters]
	    read_trans $calibs \
		[get_trans_filename $input_asdir $input_run "" $input_camCol] $input_camCol $filters
	    #
	    # Reconstruct the input PSFs
	    #
	    get_psfBasis psfBasis $input_field $filters

	    loop n 0 [array size rowc_list] {
	       set rowc $rowc_list($n); set colc $colc_list($n)
	       #
	       # Convert to original file's coordinate system
	       #
	       set ifilter -1
	       foreach filter $filters {
		  set ifilter [incr ifilter]
		  set input_cal "($calibs.calib<$ifilter>)"
		  set input_node [exprGet $input_cal->node]
		  set input_incl [exprGet $input_cal->incl]

		  set gc [transApply $coadd_trans($filter) $filter $rowc 0.0 $colc 0.0]
		  set eq [GCToEq [keylget gc mu] [keylget gc nu] -node $coadd_node -incl $coadd_incl]
		  
		  set gc [eqToGC [keylget eq ra] [keylget eq dec] -node $input_node -incl $input_incl]
		  set rc [transInverseApply *$input_cal->toGCC \
			      $filter [keylget gc mu] 0 [keylget gc nu] 0]
		  set input_rowc [expr int([keylget rc row]) + 0.5];	# centered in a pixel
		  set input_colc [expr int([keylget rc col]) + 0.5]

		  if {$input_rowc < 0 || $input_rowc > $nrow_f || $input_colc < 0 || $input_colc > $ncol_f} {
		     continue
		  }
		  
		  #echo "RHL reconstructing $input_run $input_camCol $input_field  $n  $rowc $colc   $input_rowc $input_colc"
		  set psf [reconstruct_psf $psfBasis($filter) $input_rowc $input_colc]

		  #want to normalize PSF so change weight to divide by total psf counts
		  set psfstats [regStatsFind $psf]
                  set psfnpix [expr [exprGet $psf.nrow] * [exprGet $psf.ncol]]
                  set psftot [expr ([keylget psfstats mean] - [softBiasGet]) * $psfnpix]
                  set fluxweight [expr 500000.0/$psftot]


		  if ![info exists psf_sum($n,$filter)] {
		     set psf_sum($n,$filter) [regIntConvert -type FL32 $psf]
		     regClear $psf_sum($n,$filter)
		     set npsf_sum($n,$filter) 0
		     set nap_corr_run_sum($filter) 0
		     set ap_corr_run_sum($filter) 0
		     set ap_corr_runVar_sum($filter) 0
		  }

		  set weight [lindex $weights $ifilter]
		  #include normalization weighting
		  set weight [expr $weight*$fluxweight]
		  #echo RHL $input_run $input_camCol $input_field $rowc $colc $n $filter $weight
		  regIntLincom $psf_sum($n,$filter) $psf 0 1 $weight
		  set npsf_sum($n,$filter) \
		      [expr $npsf_sum($n,$filter) + $weight]
		  #mtv $psf; puts -nonewline "RHL $filter ${n}th $input_rowc $input_colc PSF "; gets stdin
		  #mtv $psf_sum($n,$filter); puts -nonewline "RHL ${n}th $filter Summed PSF "; flush stdout; gets stdin
		  
		  regDel $psf
		  #
		  # Average the ap_corr_run values from the input runs
		  #
		  assert {[exprGet $input_cal->nann_ap_frame] == $nann_ap_frame}
		  assert {[exprGet $input_cal->nann_ap_run] == $nann_ap_run}

		  set nap_corr_run_sum($filter) \
		      [expr $nap_corr_run_sum($filter) +  $weight]
		  
		  set ap_corr_run_sum($filter) \
		      [expr $ap_corr_run_sum($filter) + $weight*[exprGet $input_cal->ap_corr_run]]
		  set ap_corr_runVar_sum($filter) \
		      [expr $ap_corr_runVar_sum($filter) + $weight*pow([exprGet $input_cal->ap_corr_runErr],2)]
	       }
	    }
	    #
	    # Clean up
	    #
	    free_psfBasis psfBasis
	    calib1byframeDel $calibs
	 }
	 #
	 # Find the required PSF_KERNEL
	 #
	 # N.b. this filename must match the expectations of the PSP's write_ps_results routine
	 #
	 set psPSF [format "$outDir/psPSF-%06d-$coadd_camCol-%04d.fit" $coadd_run $field]
	 if {$verbose} {
	    echo Writing $psPSF
	 }
	 
	 foreach filter $filters {
	    set regs [chainNew PSF_REG]
	    
	    loop n 0 [array size rowc_list] {
	       if ![info exist psf_sum($n,$filter)] {
		  continue
	       }
	       #mtv $psf_sum($n,$filter); puts -nonewline "RHL ${n}th $rowc_list($n) $colc_list($n)  $filter Summed PSF "; flush stdout; #gets stdin	       
	       regMultiplyWithDbl $psf_sum($n,$filter) [expr 1.0/$npsf_sum($n,$filter)]
	       
	       set counts 1;			# normalisation is unimportant
	       set psfReg [psfRegNew $psf_sum($n,$filter) $rowc_list($n) $colc_list($n) $counts]
	       handleDel $psf_sum($n,$filter)
	       
	       chainElementAddByPos $regs $psfReg TAIL
	       handleDel $psfReg
	    }

	    if {[chainSize $regs] == 0} {
	       error "No PSF stars are available for field $field"
	    }
	    
	    set coaddBasis($filter) [psfKLDecomp -border $border $regs $ncomp $nrow_b]
	    psfKernelSet $regs $coaddBasis($filter) *$coaddBasis($filter).kern
	    
	    psfRegChainDestroy $regs
	    #
	    # Show us how things are going?
	    #	 
	    if {$fsao_basis > 0} {
	       regDel [mtv [mosaic_basis $coaddBasis($filter)] -s $fsao_basis]
	    }
	    
	    if $display_mosaic {
	       if [catch {
		  set mosaic [reconstruct_psf $coaddBasis($filter) -nrow $npsf_r -ncol $npsf_c -trim 10]
	       } msg] {
		  echo "$msg"
	       } else {
		  catch {saoReset}
		  global display; set display 1
		  display $mosaic "KL PSF for frame $field, filter $filter"
		  regDel $mosaic
	       }
	    }
	 }
	 #
	 # Fake calib1byframe for PSP
	 #
	 set cframe [calib1byframeNew $filters]
	 
	 handleSet $cframe.field $field
	 handleSet $cframe.psp_status $PSP_STATUS(OK)
	 if $schlegel {
	    set flux20 10;			# 1 DN is a nMgy, so a 20th mag star == 1e-8Mgy == 10 nMgy
	 } else {
	    set flux20 1000
	 }
	 
	 foreach filter $filters {
	    set cal1 [handleBindFromHandle [handleNew] *$cframe.calib<[lsearch $filters $filter]>]

	    handleSet $cal1.flux20 $flux20
	    handleSet $cal1.node $coadd_node
	    handleSet $cal1.incl $coadd_incl
	    handleSet $cal1.gain $coadd_gain
	    handleSet $cal1.dark_variance $coadd_dark_variance

	    handleSet $cal1.nann_ap_frame  $nann_ap_frame
	    handleSet $cal1.nann_ap_run    $nann_ap_run
	    handleSet $cal1.ap_corr_run    \
		[expr $ap_corr_run_sum($filter)/$nap_corr_run_sum($filter)]
	    if 0 {			# this comes from the input KL errors, and is irrelevant here
	       handleSet $cal1.ap_corr_runErr \
		   [expr sqrt($ap_corr_runVar_sum($filter))/$nap_corr_run_sum($filter)]
	    } else {
	       handleSet $cal1.ap_corr_runErr 0
	    }

	    handleSet $cal1.prof->psfCounts 1
	    #
	    # Reconstruct the PSF at the centre of the frame
	    # and measure its properties for the psPSF file
	    #
	    set psf [KLpsf2dgpsf $coaddBasis($filter) [expr $nrow_f/2] [expr $ncol_f/2] \
			 0 0 1 0]
	    handleSetFromHandle $cal1.psf &$psf; handleDel $psf

	    handleSetFromHandle $cal1.toGCC &$coadd_trans($filter); handleDel $coadd_trans($filter)
	    	    
	    handleDel $cal1
	 }
	 chainElementAddByPos $calib1byframelist $cframe TAIL AFTER
	 #
	 # Intermediate output for the PSP I/O routines
	 #
	 set bias_scale 32;		# XXX
	 global psfKLbasis_nframe psfKLcoeffs_nframe Npatches; # for write_psPSF
	 set psfKLbasis_nframe 1; set psfKLcoeffs_nframe 1; set Npatches -1
	 
	 foreach filter $filters {
	    set psPSFfile [write_psPSF $filter $field $coaddBasis($filter) $cframe \
			       $bias_scale $coadd_node $coadd_incl]
	 }	 
	 #
	 # Clean up for this field
	 #
	 handleDel $cframe
	 free_psfBasis coaddBasis
      }
      #
      # Convert that psPSF file to a psField file
      #
      write_ps_results $calib1byframelist
      #
      # Clean up
      #
      chainDestroy $calib1byframelist calib1byframeDel
   }

   finiCellFitobj
   fini_measure_objects_field $filters $fieldparams
   fini_measure_objects
   fieldparamsDel $fieldparams   
}

#
# Fake setting up measure objects; meeded by KLpsf2dgpsf
#
proc fake_initMeasureObj {filters nann_ap_frame ap_corr_run} {
   set cellprofile_file [envscan {$PHOTO_DIR/lib/cellprof.dat}]
   initCellFitobj $cellprofile_file [llength $filters]
   initProfileExtract

   set nfilter [llength $filters]
   set fieldparams [fieldparamsNew $filters]

   handleSet $fieldparams.pixscale 1.0
   handleSet $fieldparams.fiber_rad 1.5; # 3" fibre; this is asserted on

   loop i 0 $nfilter {
      lappend sblist -100

      handleSet $fieldparams.frame<$i>.nann_ap_frame $nann_ap_frame
      handleSet $fieldparams.frame<$i>.ap_corr_run $ap_corr_run
   }

   set rand [phRandomNew 100000 -seed 819396102 -type 2]

   initMeasureObj $fieldparams $nfilter $sblist $rand

   phRandomDel $rand

   return $fieldparams
}

###############################################################################
#
# Prepare a psField file from the output of a photo run
#
proc framesToPsField {args} {
   global calibs fieldparams openit table
   global run camCol startRun startField endField filterlist outputDir

   set opts [list \
		 [list [info level 0] ""] \
		 [list <ncomp> INTEGER 4 ncomp \
		      "Number of KL components to keep"] \
		 [list <n_b> INTEGER 3 nrow_b \
		      "Degree of polynomial for spatial variation"] \
		 [list -mag1 INTEGER 14 mag1 "Brightest star to include in PSF"] \
		 [list -mag2 INTEGER 18 mag2 "Faintest star to include in PSF"] \
		 [list -select STRING "" select \
		      "Proc to select desired objects; see \"help select\""] \
		 [list -outDir STRING "." outputDir "Output directory"] \
		 [list -filterlist STRING "" filterlist "Filters to process (default: all)"] \
		 [list -ncol_b INTEGER -1 ncol_b "order for col spatial fit"] \
		 [list -border INTEGER 10 border "include border pixels around image"] \
		 [list -delta INTEGER 5 delta "extra space around star"] \
		 [list -nrowPSF INTEGER 35 nrowPSF "Size of image cut out around stars"] \
		 [list "-scale_psfs" CONSTANT 1 scale_psfs \
		      "Fix all stars to have the same amplitude"] \
		 [list -nsigma_clip DOUBLE 0 nsigma_clip \
		      "Clip stars with residuals more than nsigma_clip above the noise"] \
		 [list -n1 INTEGER 1 n1 "Start with this row in the table"] \
		 [list -n2 INTEGER 0 n2 "End with this row in the table (default: last)"] \
		 [list -verbose INTEGER 0 verbose "Be chatty?"] \
		 [list "-KL" INTEGER -1 fsao_KL \
		      "Display KL component images on this saoimage"] \
		 [list "-residuals" INTEGER -1 fsao_residuals \
		      "Display residuals for input objects on this saoimage"] \
		 [list "-mosaic" INTEGER -1 fsao_mosaic \
		      "Display reconstructed images on this saoimage"] \
		 [list -run STRING -1 run "Run of interest"] \
		 [list -col INTEGER 0 col "Column of interest"] \
		 [list -field0 INTEGER 0 field0 "Initial field"]\
		 [list -field1 INTEGER 0 field1 "Final field"]\
		 [list -rootdir STRING "$data_root" root_dir \
		      "Root for SDSS data/input directories"] \
		 [list -rerun STRING "" rerun \
		      "Desired reprocessing number (default: youngest)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
      
   global sel_maglim
   set sel_maglim(type) "psf"; set sel_maglim(mag1) $mag1; set sel_maglim(mag2) $mag2
   #
   # Things that usually come from fpPlan files
   #
   global asTrans_format psField_format

   if ![info exists asTrans_format] {
      set asTrans_format {asTrans-%06d.fit	$run}
   }
   if ![info exists psField_format] {
      set psField_format {psField-%06d-%d-%04d.fit $run $camCol $field}
   }
   #
   # Unpack things and read input files
   #
   set run $openit(run)
   set camCol $openit(camCol)
   set field $openit(field)
   set filters [keylget table filters]

   if {$filterlist == "" || $filterlist == "all"} {
      set filterlist $filters
   }

   if 1 {
      calib1byframeDel $calibs

      set i -1
      foreach f [split [exprGet $fieldparams.filters] {}] {
	 incr i
	 dgpsfDel [handleBindFromHandle [handleNew] *$fieldparams.frame<$i>.psf]
	 handleSet $fieldparams.frame<$i>.psf 0x0
      }

      set psFieldFile [get_psField_filename $openit(psdir) $run $camCol $field]
      
      set calibs [read_calib1byframe $field $psFieldFile $filterlist]
      read_trans $calibs \
	  [get_trans_filename $openit(asdir) $run "" $camCol] $camCol $filterlist
   }
   #
   # Unpack some things from the fieldparams/calibs
   #
   set i -1
   foreach f $filterlist {
      incr i

      set sky($f)           [exprGet $fieldparams.frame<$i>.global_sky]
      set gain($f)          [exprGet $calibs.calib<$i>->gain]
      set dark_variance($f) [exprGet $calibs.calib<$i>->dark_variance]
   }
   #
   # Actually do some work of our own
   #
   proc sel_3bands {o {b 2}} {
      global OBJECT1
      set nband 0
      set ncolor [exprGet $o.ncolor]
      loop i 0 $ncolor {
	 if {[exprGet $o.flags<$i>] & $OBJECT1(BINNED1)} {
	    incr nband
	 }
      }

      set nband_min [expr ($ncolor < 3) ? $ncolor : 3]

      return [expr $nband >= $nband_min ? 1 : 0]
   }

   find_psfBasis_from_table psfBasis $ncomp $nrow_b \
       -n1 $n1 -n2 $n2 -sel !bright&&good&&3bands&&star&&maglim&&!DEBLENDED_AS_PSF \
       -filter $filterlist \
       -scale -sky sky -gain gain -darkVariance dark_variance \
       -nsig $nsigma_clip -verb $verbose\
       -resid $fsao_residuals -KL $fsao_KL -mosaic $fsao_mosaic
   #
   # Prepare to write a psField file
   #
   # Intermediate output for the PSP I/O routines
   #
   global PSP_STATUS sdssFilterName id_values
   set id_values(SSC_ID) "(not run)"

   global psfKLbasis_nframe psfKLcoeffs_nframe Npatches; # for write_psPSF
   set psfKLbasis_nframe 1; set psfKLcoeffs_nframe 1; set Npatches -1
   set bias_scale 32;		# XXX
   set startField $field; set endField $field
   set startRun $startField; 
   
   foreach filter $filterlist {
      set psPSFfile [write_psPSF $filter $field $psfBasis($filter) $calibs \
			 $bias_scale [keylget table node] [keylget table incl]]
   }	 
   #
   # Convert psPSF file to a psField file
   #
   set calib1byframelist [chainNew CALIB1BYFRAME]
   chainElementAddByPos $calib1byframelist $calibs TAIL AFTER

   write_ps_results $calib1byframelist
   set calibs [chainElementRemByPos $calib1byframelist 0]
   
   chainDel $calib1byframelist
   #
   # Clean up
   #
   foreach f $filterlist {
      psfBasisDel $psfBasis($f)
   }
   meschachFini
   fitsBinForgetSchemaTrans NULL
}

###############################################################################

proc find_psfBasis_from_table {args} {
   global objcIo openit table
   set scale_psfs 0;			# Fix all stars to have the same amplitude?
   
   set opts [list \
		 [list [info level 0] "Given a table, set an array of PSF_BASIS structs"] \
		 [list <psfBasis> STRING "" _psfBasis "Array of PSF_BASIS to return"] \
		 [list <ncomp> INTEGER 0 ncomp \
		      "Number of KL components to keep"] \
		 [list <n_b> INTEGER 0 nrow_b \
		      "Degree of polynomial for spatial variation"] \
		 [list -select STRING "" select \
		      "Proc to select desired objects; see \"help select\""] \
		 [list -filterlist STRING "" filterlist "Filters to process (default: all)"] \
		 [list -ncol_b INTEGER -1 ncol_b "order for col spatial fit"] \
		 [list -border INTEGER 10 border "include border pixels around image"] \
		 [list -delta INTEGER 5 delta "extra space around star"] \
		 [list -nrowPSF INTEGER 35 nrowPSF "Size of image cut out around stars"] \
		 [list -sky STRING "" _sky \
		      "Array of subtracted background levels (indexed by filter name)"] \
		 [list -gain STRING "" _gain \
		      "Array of effective gains of detectors (indexed by filter name)"] \
		 [list -darkVariance STRING "" _darkVariance \
		      "Array of dark variances of detectors (indexed by filter name)"] \
		 [list "-scale_psfs" CONSTANT 1 scale_psfs \
		      "Fix all stars to have the same amplitude"] \
		 [list -nsigma_clip DOUBLE 0 nsigma_clip \
		      "Clip stars with residuals more than nsigma_clip above the noise"] \
		 [list -n1 INTEGER 1 n1 "Start with this row in the table"] \
		 [list -n2 INTEGER 0 n2 "End with this row in the table (default: last)"] \
		 [list -verbose INTEGER 0 verbose "Be chatty?"] \
		 [list "-KL" INTEGER -1 fsao_KL \
		      "Display KL component images on this saoimage"] \
		 [list "-residuals" INTEGER -1 fsao_residuals \
		      "Display residuals for input objects on this saoimage"] \
		 [list "-mosaic" INTEGER -1 fsao_mosaic \
		      "Display reconstructed images on this saoimage"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   upvar $_psfBasis psfBasis

   if {$ncol_b < 0} { set ncol_b $nrow_b }

   if ![info exists table] {
      error "Please run proc openit before executing this command"
   }

   if {$filterlist == "" || $filterlist == "all"} {
      set filterlist [keylget table filters]
   }
   set field $openit(field)
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]
   #
   # Set sky/gain/darkVariance if not provided
   #
   foreach v "gain sky darkVariance" {
      if {[set _$v] == ""} {
	 foreach f $filterlist {
	    set ${v}($f) 0
	 }
      } else {
	 upvar [set _$v] $v
      }
   }
   #
   # Setup randomness if required
   #
   if {$_sky != "" || $_darkVariance != ""} {
      foreach f $filterlist {
	 assert {$sky($f) >= 0 && $darkVariance($f) >= 0}
	 assert {$gain($f) > 0}
      }
   
      set seed 50258
      set rand [phRandomNew 100000 -seed $seed -type 2]
   } else {
      set rand "NULL"
   }
   #
   # Estimate PSF
   #
   if {$nrowPSF%2 == 0} {
      incr nrowPSF;
   }
   set ncolPSF $nrowPSF

   foreach f $filterlist {
      set regs($f) [chainNew PSF_REG]
   }
   
   set n1 1
   if {$n2 <= 0} {
      set n2 [keylget table OBJnrow]
   }
   loop id $n1 [expr $n2 + 1] {
      read_objc table $id

      set band -1
      foreach f $filterlist {
	 incr band
	 # Do we want this object in this band??
	 if {"$select" != "" && ![eval $select $objcIo $band]} {
	    continue;
	 }

	 if {[set vals [get_star $objcIo $band -border $border -size $nrowPSF \
			    -sigma [expr sqrt($darkVariance($f) + $sky($f)/$gain($f))] \
			    -rand $rand]] == ""} {
	    continue
	 }

	 set psfReg [eval psfRegNew $vals -field $field]
	 handleDel [lindex $vals 0];		# the region

	 if ![info exists nrow] {
	    set nrow [exprGet $psfReg.reg->nrow];
	    set ncol [exprGet $psfReg.reg->ncol]
	 }
	 
	 if 0 {
	    mtv *$psfReg.reg;
	    puts -nonewline [format "$f (%.1f, %.1f) Next: " [exprGet $psfReg.rowc] [exprGet $psfReg.colc]]
	    if {[gets stdin] == "q"} {
	       error $psfReg
	    }
	 }
	 
	 chainElementAddByPos $regs($f) $psfReg TAIL
	 handleDel $psfReg
      }
   }
   #
   # We have the PSF_REGs, now proceed to the PSF_BASIS
   #
   foreach f $filterlist {
      set done($f) 0;			# have we finished clipping this band?
   }

   set iter 0
   while 1 {
      incr iter
      set nclip(any) 0;			# no stars are clipped (yet)
      
      foreach f $filterlist {
	 if $done($f) {
	    continue;
	 }

	 if {$fsao_KL > 0 || $fsao_residuals > 0 || $fsao_mosaic > 0} {
	    puts -nonewline "$f"
	 }
	 
	 if {$iter > 1} {
	    psfBasisDel $psfBasis($f)
	 }
	 set psfBasis($f) [get_psfBasis_from_regs $regs($f) \
			       $ncomp $nrow_b -ncol_b $ncol_b -border $border -scale_psfs $scale_psfs \
			       -KL $fsao_KL -residuals $fsao_residuals -mosaic $fsao_mosaic]
      #
      # Do we want to clip any stars?
      #
	 set nclip($f) 0
	 if {$nsigma_clip > 0} {
	    
	    if {$gain($f) > 0} {
	       set curs [chainCursorNew $regs($f)]
	       while {[set psf [chainWalk $regs($f) $curs NEXT]] != ""} {
		  set rowc [exprGet $psf.rowc]
		  set colc [exprGet $psf.colc]
		  
		  set peak [exprGet $psf.peak_residual]
		  set rpeak [exprGet $psf.peak_residual_rpeak]
		  set cpeak [exprGet $psf.peak_residual_cpeak]

		  set dataVal [regPixGet *$psf.reg $rpeak $cpeak]
		  
		  set nsigma [expr $peak/sqrt($darkVariance($f) + ($sky($f) + $dataVal)/$gain($f))]
		  if {$nsigma > $nsigma_clip} {
		     if {$verbose > 1} {
			echo [format "$f: %3d %6.1f %6.1f  %5.0f %4.0f  %6.2f" \
				  $i $rowc $colc $peak $dataVal $nsigma]
		     }

		     set psf [chainElementRemByCursor $regs($f) $curs]
		     if [info exists j] {
			incr j -1
		     }
		     psfRegDel $psf

		     incr nclip($f)
		     incr nclip(any)
		  }
	       }
	       chainCursorDel $regs($f) $curs
	    }
	 }

	 if {$nsigma_clip > 0 && $verbose} {
	    echo "Clipped $nclip($f) candidates in $f (iteration $iter)"
	 }

	 if {$nsigma_clip <= 0 || $nclip($f) == 0} {
	    set done($f) 1
	 }
      }

      if {$nclip(any) == 0} {
	 break;
      }
   }
   #
   # Clean up
   #
   foreach f $filterlist {
      psfRegChainDestroy $regs($f)
   }

   if {$rand != "NULL"} {
      phRandomDel $rand
   }
}

#
# Take a set of PSF stars and find the KL estimate of the PSF at the position of the star.
# Find the residuals and examine them.
#
# Optionally generate a mosaic of the original, KL-PSF, and residual images
#
proc find_psfRegChain_residuals {args} {
   set opts [list \
		 [list [info level 0] ""] \
		 [list <psfRegChain> STRING "" psfRegChain "CHAIN of PSF_REGS"] \
		 [list <psfBasis> STRING "" psfBasis "PSF_BASIS"] \
		 [list -border INTEGER 10 border "Border around the psfs"] \
		 [list -mosaic STRING "" _mosaic \
		      "Name of desired image of the residuals"] \
		 ]
   

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$_mosaic == ""} {
      set make_mosaic 0
   } else {
      upvar $_mosaic mosaic
      set make_mosaic 1
   }

   set Nstar [chainSize $psfRegChain]

   if $make_mosaic {
      set npanel 3
      # each panel counts as a unit area  
      set Nrow [expr int(sqrt($npanel*$Nstar))]
      if {$Nrow <= 0} { set Nrow 1 }
      # but npanels must be adjacent
      while {$Nrow/$npanel*$npanel != $Nrow} { incr Nrow }
      # how many columns?     
      set Ncol [expr int($npanel*$Nstar/$Nrow)]
      if {$Ncol <= 0} { set Ncol 1 }
      # do we have enough?
      while {$Ncol*$Nrow < $npanel*$Nstar} { incr Ncol }   
      set i 0; set j 0;			# used for making mosaic
   }
   #
   # Process the stars
   #   
   loop istar 0 $Nstar {
      set psfReg [chainElementGetByPos $psfRegChain $istar]
      set rowc [exprGet $psfReg.rowc]; set colc [exprGet $psfReg.colc]

      # do we know the reg's size?
      if ![info exists nrow] {
	 set nRegrow [exprGet $psfReg.reg->nrow]
	 set nRegcol [exprGet $psfReg.reg->ncol]
	 # account for the dummy border
	 set b_fac [expr int(0.9*$border)]
	 set nrow [expr int($nRegrow - 2*$b_fac)]
	 set ncol [expr int($nRegcol - 2*$b_fac)]
      }
      
      # do we need to set up mosaic?
      if {$make_mosaic && ![info exists mosaic]} {
	 set mosaic [regNew -type FL32 [expr $nrow*$Nrow] [expr int(1.1*$ncol*$Ncol)]]
	 regClear $mosaic
      } 
      # estimate PSF at reg's position
      set treg [psfKLReconstruct $psfBasis $rowc $colc]
      
      ### make mosaic
      
      # first measure peak and flux needed for scaling below
      if 0 {
	 set dataC [exprGet $psfReg.counts]
      } else {
	 set datareg [subRegNew *$psfReg.reg $nrow $ncol $b_fac $b_fac] 

	 set datastats [regStatsFind $datareg]
	 set dataC [expr [keylget datastats high] - [softBiasGet]]

	 regDel $datareg
      }
      
      if 0 {
	 set modelC [exprGet $treg.counts]
      } else {
	 set datastats [regStatsFind *$treg.reg]
	 set modelC [expr [keylget datastats high] - [softBiasGet]]
      }

      # extract central portion of data region (with only a bit of border)
      set datareg [subRegNew *$psfReg.reg $nrow $ncol $b_fac $b_fac] 
      # extract central portion of the reconstructed PSF
      set modelreg [subRegNew *$treg.reg $nrow $ncol $b_fac $b_fac] 
      
      # find scale
      if {$modelC != 0} {
	 set scale [expr 1.0*$dataC/$modelC]
      } else {
	 error "PSF model counts = $modelC"
      }

      if $make_mosaic {
	 ## make 3 panels in mosaic region
	 # data panel
	 set sreg [subRegNew $mosaic $nrow $ncol $j [expr int(1.1*$i*$ncol)]]
	 regIntCopy $sreg $datareg
	 regIntLincom $sreg "" [softBiasGet] 1 0; regDel $sreg
	 # model panel
	 set sreg [subRegNew $mosaic $nrow $ncol \
		       [expr $j + int(0.95*$nrow)] [expr int(1.1*$i*$ncol)]]
	 regIntCopy $sreg $modelreg 
	 regIntLincom $sreg "" [softBiasGet] $scale 0; regDel $sreg
	 # allocate residual; we'll set it in a moment
	 set residual [subRegNew $mosaic \
			   $nrow $ncol [expr $j + int(1.9*$nrow)] [expr int(1.1*$i*$ncol)]]

	 if {[incr i] == $Ncol} {
	    incr j [expr $npanel*$nrow]
	    set i 0
	 }
      } else {
	 set residual [regNew -type FL32 $nrow $ncol]
      }
      #
      # Calculate residual (data-scale*model)
      # (both data and model do NOT have soft bias included)
      #
      regIntCopy   $residual $datareg;
      regIntLincom $residual $modelreg [softBiasGet] 1 -$scale
      #
      # Save some information about residuals
      #
      set cen [regIntMaxPixelFind $residual]
      set rpeak [lindex $cen 0]; set cpeak [lindex $cen 1]
      set peak [expr [regPixGet $residual $rpeak $cpeak] - [softBiasGet]]

      handleSet $psfReg.peak_residual $peak
      handleSet $psfReg.peak_residual_rpeak $rpeak
      handleSet $psfReg.peak_residual_cpeak $cpeak
      #
      # Clean up
      #
      regDel $residual
      regDel $datareg; regDel $modelreg  
      psfRegDel $treg
   }
}


###############################################################################
#
# Do the work of extracting (and displaying) a PSF_BASIS given a chain of PSF_REGS
#
proc get_psfBasis_from_regs {args} {
   set const 0;

   set opts [list \
		 [list [info level 0] ""] \
		 [list <regs> STRING "" regs "CHAIN of PSF_REGS to process"] \
		 [list <ncomp> INTEGER 0 ncomp \
		      "Number of KL components to keep"] \
		 [list <n_b> INTEGER 0 nrow_b \
		      "Degree of polynomial for spatial variation"] \
		 [list -ncol_b INTEGER -1 ncol_b "order for col spatial fit"] \
		 [list -border INTEGER 10 border "include border pixels around image"] \
		 [list "-KL" INTEGER -1 fsao_KL \
		      "Display KL component images on this saoimage"] \
		 [list "-residuals" INTEGER -1 fsao_residuals \
		      "Display residuals for input objects on this saoimage"] \
		 [list "-mosaic" INTEGER -1 fsao_mosaic \
		      "Display reconstructed images on this saoimage"] \
		 [list "-scale_psfs" INTEGER 1 scale_psfs \
		      "Fix all stars to have the same amplitude"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if $scale_psfs {
      set constantW -constant
   } else {
      set constantW ""
   }
   set psfBasis [eval psfKLDecomp -border $border $regs $ncomp $nrow_b $constantW]
   psfKernelSet $regs $psfBasis *$psfBasis.kern
   #
   # How well did we do?
   #
   if {$fsao_residuals <= 0} {
      find_psfRegChain_residuals $regs $psfBasis
   } else {
      find_psfRegChain_residuals $regs $psfBasis -mosaic residualMosaic
      mtv -s $fsao_residuals $residualMosaic
      
      puts -nonewline ": "
      if {[gets stdin] == "q"} {
	 error $residualMosaic
      }
      
      regDel $residualMosaic
   }
   
   if {$fsao_KL > 0} {
      set treg [mosaic_basis $psfBasis]
      
      mtv -s $fsao_KL $treg
      puts -nonewline ": "
      if {[gets stdin] == "q"} {
	 error $treg
      }
      regDel $treg
   }
   
   if {$fsao_mosaic > 0} {
      set treg [reconstruct_psf $psfBasis -nrow 10 -ncol 14 -gutter 1 -trim 10]
      
      mtv -s $fsao_mosaic $treg
      puts -nonewline " : "
      if {[gets stdin] == "q"} {
	 error $treg
      }
      regDel $treg
   }
   
   return $psfBasis
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

proc framesRunToPsField {args} {
   set opts [list \
		 [list [info level 0] ""] \
		 [list <run> STRING 0 run "The desired run"] \
		 [list -rerun INTEGER -1 rerun "The desired rerun"] \
		 [list <camCol> INTEGER 0 camCol "Desired camera column"] \
		 [list <startField> STRING "first" startField \
		    "Desired first (or only) field to process, or \"first\""] \
		 [list {[endField]} STRING "" endField \
		      "Desired last field to process, or \"last\" (default: <startField>)"]\
	         [list -filterlist STRING "" filterlist \
		      "Filters to process (may be `all')"] \
		 [list -ncomp INTEGER 4 ncomp \
		      "Number of KL components to keep"] \
		 [list -n_b INTEGER 3 nrow_b \
		      "Degree of polynomial for spatial variation"] \
		 [list -mag1 INTEGER 14 mag1 "Brightest star to include in PSF"] \
		 [list -mag2 INTEGER 18 mag2 "Faintest star to include in PSF"] \
		 [list -select STRING "" select \
		      "Proc to select desired objects; see \"help select\""] \
		 [list -outDir STRING "." outDir "Output directory"] \
		 [list -ncol_b INTEGER -1 ncol_b "order for col spatial fit"] \
		 [list -nrowPSF INTEGER 35 nrowPSF "Size of image cut out around stars"] \
		 [list -nsigma_clip DOUBLE 0 nsigma_clip \
		      "Clip stars with residuals more than nsigma_clip above the noise"] \
		 [list -verbose INTEGER 0 verbose "Be chatty?"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$endField == ""} {
      set endField {$startField}
   }

   loop field $startField [expr $endField + 1] {
      if $verbose {
	 echo "Field $field"
      }
      set_run -rerun $rerun $run $camCol $field

      framesToPsField $ncomp $nrow_b -ncol_b $ncol_b -select $select -filterlist $filterlist \
	  -mag1 $mag1 -mag2 $mag2 -nrowPSF $nrowPSF -nsigma_clip $nsigma_clip -verbose $verbose \
	  -outDir $outDir
   }
}

proc make_fpM_for_coadds {args} {
   global openit
   global run camCol nfilter field MASK_TYPE wrote_files

   set opts [list \
		 [list [info level 0] "Convert an fpMI file (an image
 of the number of saturated pixels contributing to an output pixel)
 to an fpM file"] \
		 [list <run> INTEGER 0 run "Desired run"] \
		 [list <camCol> INTEGER 0 camCol "Desired camCol"] \
		 [list <startField> INTEGER 0 startField "Desired first field"] \
		 [list {[endField]} INTEGER -1 endField \
		      "Desired last field (default: startField)"] \
		 [list -rerun INTEGER -1 rerun "Desired rerun"] \
		 [list -filterlist STRING "u g r i z" filters "Filters to process"] \
		 [list -outDir STRING "" outDir \
		      "Directory to write psField files to (default: openit(corrdir)"] \
		 [list -verbose INTEGER 1 verbose "Be chatty?"]\
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$endField <= 0} {
      set endField $startField
   }
   set nfilter [llength $filters]

   set_run -rerun $rerun $run $camCol

   if {$outDir == ""} {
      set outDir $openit(corrdir)
   }
   #
   # Set (well, fake) IDs for header
   #
   global id_values
   foreach k "ASTRO_ID PS_ID" {
      set id_values($k) "unknown"
   }
   declare_schematrans $nfilter
   
   loop field $startField [expr $endField + 1] {
      if $verbose {
	 echo $run-$camCol-$field
      }
      foreach filter $filters {
	 set file [get_mask_filename $openit(corrdir) $run $filter $camCol $field]
	 regsub {fpM-} $file {fpMI-} file
	 
	 set reg [regReadAsFits [regNew] -keep $file]
	 set sm [reg2spanmask $reg -types SATUR -level 1]    
	 handleSetFromHandle $reg.mask (MASK*)&$sm
	 handleDel $sm

	 if 0 {
	    mtv $reg
	 }

	 write_fpM $filter $reg $outDir

	 regDel $reg
      }
   }
}

