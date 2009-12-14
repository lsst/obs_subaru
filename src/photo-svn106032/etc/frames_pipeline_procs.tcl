# FRAMES pipeline procedures
#

set frames_pipeline_procs_tcl 1
#
# Get some numbers from enums
#
set_array_from_define FIELD_FLAGS FIELD_ FIELD
set_array_from_define OBJECT1_FLAGS OBJECT1_ OBJECT1
set_array_from_define OBJECT2_FLAGS OBJECT2_ OBJECT2
set_array_from_define PHOTO_CONSTS
set_array_from_define MODEL_TYPES
catch {					# type may not exist
   set_array_from_define OBJECT3_FLAGS OBJECT3_ OBJECT3

   set OBJECT3(USR1) $OBJECT3(MEASURED)
   set OBJECT3(USR2) $OBJECT3(GROWN_MERGED)
   set OBJECT3(USR3) $OBJECT3(HAS_CENTER)
   set OBJECT3(USR4) $OBJECT3(MEASURE_BRIGHT)
}
set_array_from_define S_MASKTYPE S_MASK_ MASK_TYPE; \
    unset MASK_TYPE(S_NMASK_TYPES)
if ![info exists IMASK_TYPE] {
   foreach el [array names MASK_TYPE] {
      set IMASK_TYPE($MASK_TYPE($el)) $el
   }
}
set_array_from_define STAR1_FLAGS STAR1_ STAR1
set_array_from_define OBJ_TYPE OBJ_
set_array_from_define DFACTION
set_value_from_define N_LINEAR_COEFFS

set PI 3.1415926535897931

#
# Set line buffering for stdout
#
if {[fcntl stdout LINEBUF] == 0} {
   flush stdout; fcntl stdout LINEBUF 1
}
#
# Because of the way that the header code works (allocating a new header
# every time that we use regReadAsFits), we need to free all headers so
# that we'll be able to use memFreeBlocks. It's easiest to do this as we
# read the files
#
# hdrFreeAll is in photo/src/utils.c as the dervish hdrFree doesn't free enough
#

#
# this procedure sets the number of rows and columns in the
# corrected images (includes overlap area)
#
proc init_read_frames {filterlist _rawimages} {
   upvar $_rawimages rawimages
   global softpars ccdpars
   global rawnrows rawncols overlap corrows corcols

   set filter [lindex $filterlist 0]

   # Get size for frame with overlap
   set overlap [getsoftpar scan_overlap]
   set rawnrows [exprGet $ccdpars($filter).nDataRow]
   set rawncols [exprGet $ccdpars($filter).ncols]
   set corrows [expr [exprGet $ccdpars($filter).nDataRow] + $overlap]
   set corcols [exprGet $ccdpars($filter).nDataCol]
   #
   # Allocate raw image buffers
   #
   foreach f $filterlist {
      set rawimages($f) [regNew -name raw_$f $corrows $rawncols]
   }
   #
   # Now for scratch buffers. These will be managed via the overlay mechanism
   # Make big regions, with overscan regions intact. We could get away with
   # some large and some trimmed, but it would make life more complicated.
   #
   loop n 0 4 {
      overlayAdd [regNew -name scr$n $corrows $rawncols]
   }
   #
   # Convert fpC_saturLevel_band to an array
   #
   global fpC_saturLevel
   set fpC_saturLevel(default) [getsoftpar fpC_saturLevel_default]

   foreach f $filterlist {
      if {![getsoftpar fpC_saturLevel_$f level] || $level == "default"} {
	 set level $fpC_saturLevel(default)
      }
      set fpC_saturLevel($f) $level
   }
}

proc fini_read_frames {filterlist _rawimages} {
   upvar $_rawimages rawimages

   foreach f $filterlist {
      regDel $rawimages($f)
   }
}

proc read_frames {filter rawimage _corrimage _biasvector imagedir} {
   upvar $_biasvector biasvector $_corrimage corrimage
   global psBBDir id_values
   global run ccdrow camCol field
   global ccdpars fieldstat corrows corcols rawnrows rawncols overlap
   global read_fpC_files read_psBB_files idR_zero
   global ref_color TYPE_PIX
   
   verb_echo 3 "In read_frames"

   assert {[exprGet (int)$rawimage.type]==$TYPE_PIX}
   
   # Read in the current frame off the disk
   if !$read_fpC_files {
      set rawsub1 [subRegNew $rawimage $rawnrows $rawncols 0 0]
      # Construct the filename
      set rawfile [get_raw_filename $imagedir $run $filter $camCol $field]
      
      verb_echo 4 "Reading in file $rawfile"
      if [catch { regReadAsFits -keeptype $rawsub1 $rawfile } msg] {
	 regDel $rawsub1
	 error "Reading main body of field: $msg"
      }
      calc_crc -field $rawfile
      
      # Copy the header read from tape to the raw region
      hdrCopy $rawsub1.hdr $rawimage.hdr; hdrFreeAll $rawsub1.hdr
      regDel $rawsub1
      
      #
      # Fix EQUINOX keyword to be Double, not Ascii. PR 5186
      #
      set line [hdrGetLine $rawimage.hdr EQUINOX]
      if [regexp {EQUINOX = '} $line] {	# written as a string
	 set equinox [hdrGetAsAscii $rawimage.hdr EQUINOX]
	 hdrDelByKeyword $rawimage.hdr EQUINOX
	 hdrInsWithDbl $rawimage.hdr EQUINOX $equinox
      }
      
      if {$overlap > 0} {
	 # Read in the next frame to build the overlap region
	 overlayGet tapeimage $rawnrows $rawncols
	 
	 # Construct the filename
	 set rawfile \
	     [get_raw_filename $imagedir $run $filter $camCol [expr $field+1]]
	 verb_echo 4 "Reading in file $rawfile for overlap"
	 
	 if [catch {
	    regReadAsFits -keeptype $tapeimage $rawfile
	 } msg] {
	    hdrFreeAll $rawimage.hdr
	    hdrFreeAll $tapeimage.hdr
	    overlayRelease tapeimage
	    error $msg
	 }
	 
	 calc_crc -field $rawfile
	 
	 # Join the overlap to the end of the raw region
	 set rawsub2 [subRegNew $rawimage $overlap $rawncols $rawnrows 0]
	 set tapesub [subRegNew $tapeimage $overlap $rawncols 0 0]
	 regPixCopy $tapesub $rawsub2
	 regDel $tapesub; regDel $rawsub2
	 
	 hdrFreeAll $tapeimage.hdr
	 overlayRelease tapeimage

	 if {[info exists idR_zero] && $idR_zero > 0} {
	    regIntConstAdd $rawimage $idR_zero
	 }
      }

      if $read_psBB_files {
	 set biasvector [regNew -name bias 1 $rawncols -type U16]
	 read_biasfile $biasvector \
	     [get_bias_filename $psBBDir $run $filter $camCol $field] $filter
	 set id_values(BIAS_ID) "PS"
      }
   }
   #
   # Create the corrected image as a subimage of the rawimage
   #
   set datacol0 [exprGet $ccdpars($ref_color).sData0]
   set corrimage [subRegNew $rawimage $corrows $corcols 0 $datacol0]
   set mask [spanmaskNew -nrow $corrows -ncol $corcols]
   handleSetFromHandle $corrimage.mask (MASK*)&$mask; handleDel $mask
   handleSet $corrimage.col0 0
}

proc init_correct_frames {biasDir _biasvector ffDir _flatvector \
		        rawncols rcor ccor \
			_cr_keep _cr_min_sigma _cr_min_e _cr_cond3 _cr_cond32 \
			      _fullWell } {
   upvar $_biasvector biasvector  $_flatvector flatvector  
   upvar $_cr_keep cr_keep  $_cr_min_sigma cr_min_sigma \
       $_cr_min_e cr_min_e   $_cr_cond3 cr_cond3   $_cr_cond32 cr_cond32 \
       $_fullWell fullWell
   global ccdpars hardpars softpars read_fpC_files read_psBB_files
   global filterlist nfilter
   global fieldstat fieldparams
   global run camCol
   global id_values
   
   initCorrectFrames

   #
   # Some software parameters, and the smoothing bias values for DSC
   #
   set cr_keep      [getsoftpar cr_keep]
   set cr_min_sigma [getsoftpar cr_min_sigma]
   set cr_min_e     [getsoftpar cr_min_e]
   set cr_cond3     [getsoftpar cr_cond3]
   set cr_cond32    [getsoftpar cr_cond32]

   handleSet $fieldparams.scan_overlap [getsoftpar scan_overlap]
   handleSet $fieldparams.run_overlap [getsoftpar run_overlap]

   set soft_bias [softBiasGet]
   set_ccds_in_fieldparams ccdpars $fieldparams $filterlist fullWell
   #
   # as we are not actually correcting frames, we don't need idB etc. files
   #
   if $read_fpC_files {
      set id_values(PS_ID) "(unknown)"
      set id_values(PS_ID_file) "(unknown)"
       foreach f $filterlist {
	   set biasvector($f) [regNew]
	   set flatvector($f) [regNew]
       }
      return
   }
   #
   # Read the flat field and bias vectors
   #
   foreach f $filterlist {
      if { $rcor > 0 } {
	 set flatvector($f) [regNew -name flat 3 $ccor -type U16]
      }

      # Get the appropriate flatfield and bias vectors
   
      set flatfile [get_ff_filename $ffDir $run $f $camCol]
      verb_echo 3 flatfile = $flatfile
      regReadAsFits $flatvector($f) $flatfile
      calc_crc $flatfile
      set id_values(PS_ID) [hdrGetAsAscii $flatvector($f).hdr PS_ID]
      set id_values(PS_ID_file) $flatfile
      if [catch {				# keyword may not be present
	 set id_values(FF_ID,$f) [hdrGetAsAscii $flatvector($f).hdr FF_ID]
      }] {
	 set id_values(FF_ID,$f) "(none)"
      }

      if $read_psBB_files {
	 ;				# different files for each frame
      } else {
	 set biasvector($f) [regNew -name bias 1 $rawncols -type U16]
	 read_biasfile $biasvector($f) \
	     [get_bias_filename $biasDir $run $f $camCol] $f
      }
   }
}

proc read_biasfile {biasvector biasfile f} {
   global id_values

   regReadAsFits $biasvector $biasfile;
   if [regexp {idB} $biasfile] {
      calc_crc $biasfile
   } else {
      calc_crc -field $biasfile
   }
   
   if [catch {
      set bias_id [hdrGetAsAscii $biasvector.hdr BIAS_ID]
   }] {
      global bias_warnings
      if ![info exists bias_warnings($biasfile)] {
	 set bias_warnings($biasfile) 1
	 echo "$biasfile has no BIAS_ID keyword"
      }
      set bias_id "unknown";	# XXX
   }
   
   if {[info exists id_values(BIAS_ID)] &&
       $id_values(BIAS_ID) != $bias_id} {
      # error "BIAS_ID == \"$bias_id\" read from $biasfile doesn't match \$id_values(BIAS_ID) == \"$id_values(BIAS_ID)\""
   }

   set id_values(BIAS_ID) $bias_id
   set id_values(BIAS_ID_file) $biasfile
   #
   # Deal with FF_ID keywords. They must be absent in both flatfield and bias,
   # or the same in both files.
   #
   # The most likely reason for trouble is failing to set "read_psBB_files 1"
   # in the fpPlan file
   #
   if [catch {				# keyword may not be present
      set FF_ID [hdrGetAsAscii $biasvector.hdr FF_ID]
   }] {
      set FF_ID "(none)"
   }

   if {$id_values(FF_ID,$f) != $FF_ID} {
      if {$id_values(FF_ID,$f) == "(none)"} {
	 error "File $id_values(PS_ID_file) has no FF_ID keyword; $biasfile has FF_ID == $FF_ID"
      } elseif {$FF_ID == "(none)"} {
	 error "File $id_values(PS_ID_file) has FF_ID == $id_values(FF_ID,$f); $biasfile has no FF_ID keyword"
      } else {
	 error "File $id_values(PS_ID_file) has FF_ID == $id_values(FF_ID,$f); $biasfile has $FF_ID"
      }
   }
}
  
proc correct_frames {filter rawimage corrimage biasvector \
			 flatvector calib _skyTypical _skySigma _skyErr \
			 outdir \
                         cr_keep cr_min_sigma cr_min_e cr_cond3 cr_cond32 \
			 {psfBasis ""}} {
   verb_echo 3 "In correct_frames"
   upvar $_skyTypical skyTypical $_skySigma skySigma
   if {$_skyErr != ""} {
      upvar $_skyErr skyErr
   }
   global PHOTO_CONSTS
   global corrows corcols
   global filterlist coadds ccdpars
   global runstr run camCol ccdrow field
   global fieldstat fieldparams
   global read_fpC_files fpC_zero fpC_scale fpC_saturLevel
   global write_fpC_files compress_image_files id_values
   global fix_bias
   
   set fieldstr [format %04d $field]

   set gain0 [exprGet $ccdpars($filter).gain0]
   set dark_variance0 [exprGet $ccdpars($filter).readNoise0]
   if {[exprGet $ccdpars($filter).namps] == 1} {
      set gain1 $gain0
      set dark_variance1 $dark_variance0
   } else {
      set gain1 [exprGet $ccdpars($filter).gain1]
      set dark_variance1 [exprGet $ccdpars($filter).readNoise1]
   }
   set gain [expr ($gain0 + $gain1)/2.0]
   set dark_variance [expr ($dark_variance0 + $dark_variance1)/2.0]

   # Check that we grabbed the right calib
   set index [lsearch $filterlist $filter]
   assert {[exprGet $calib.calib<$index>->filter<0>] == $filter}

   set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<$index>]
   
   set row $ccdrow($filter)
   					# sky level/error from PSP
   set skyTypical($filter) [exprGet $calib.calib<$index>->sky]
   set skySigma($filter) [exprGet $calib.calib<$index>->skysig]
   #
   # Are these coadds?  If so, we need to set the effective sky level and gain
   # from the idCoadds file
   #
   if [info exists coadds] {
      if ![info exists coadds($field,$filter)] {
	 error "Coadds array exists, but there is no entry for $field,$filter"
      }
      set sky_eff [exprGet $coadds($field,$filter).skyEff]
      set gain [exprGet $coadds($field,$filter).gainEff]
      set rnoise [expr sqrt([exprGet $coadds($field,$filter).dark_varIn])]
      #
      # If we scaled, fix gain/dark variance too
      #
      if {$fpC_scale != 1.0} {
	 set sky_eff [expr $sky_eff*$fpC_scale]
	 set rnoise  [expr $rnoise*$fpC_scale]
	 set gain    [expr $gain/$fpC_scale]
      }

      verb_echo 2 [format \
   "$filter: sky_eff = %.2f gain: %.3f Dark_var = %.3f" \
		       $sky_eff $gain  [expr $rnoise*$rnoise]]

      set dark_variance [expr $rnoise*$rnoise]
      set skyTypical($filter) $sky_eff
   }
   #
   # Set the dark_variance/gain now that we've thought about coadds
   #
   handleSet $fparams.dark_variance $dark_variance
   foreach g "gain0 gain1" {
      handleSet $fparams.$g [set $g]
   }

   set gainreg 0x0;  set dark_variancereg 0x0
   if $read_fpC_files {
      if [catch {
	 upvar imageDir imageDir;		# we know it's there...
	 set infile [get_corr_filename $imageDir $run $filter $camCol $field]
	 
	 set dark_variancereg [binregionFromRegion [regReadAsFits [regNew] $infile -hdu 1]]
	 set gainreg  [binregionFromRegion [regReadAsFits [regNew] $infile -hdu 2]]
      } msg] {
	 echo "RHL" $msg
      }
   }

   if {$gainreg == 0x0} {
      set gainreg [binregionNewFromConst $gain \
		       -nrow $corrows -bin_row $corrows \
		       -ncol $corcols -bin_col $corcols -shift $PHOTO_CONSTS(MAX_U16)]
   }

   if {$dark_variancereg == 0x0} {
      set dark_variancereg [binregionNewFromConst $dark_variance \
				-nrow $corrows -bin_row $corrows \
				-ncol $corcols -bin_col $corcols -shift $PHOTO_CONSTS(MAX_U16)]
   }

   handleSetFromHandle $fparams.gain &$gainreg
   handleSetFromHandle $fparams.dark_variance &$dark_variancereg
   #
   if $read_fpC_files {
      upvar imageDir imageDir;		# we know it's there...
      set infile [get_corr_filename $imageDir $run $filter $camCol $field]
      calc_crc -field $infile
      
      verb_echo 4 "Reading file $infile"
      if [info exists sky_eff] {
	 verb_echo 5 "sky_eff($filter) = $sky_eff"
      }
      if {$fpC_zero == 0 && $fpC_scale == 1 && ![info exists sky_eff]} {
	 regReadAsFits -keeptype $corrimage $infile
	 if [info exists sky_eff] {	# effective sky level for a coadd
	    regIntConstAdd $corrimage $sky_eff
	 }
      } else {
	 set zero $fpC_zero
	 
	 if [info exists sky_eff] {	# effective sky level for a coadd
	    set zero [expr $zero + $sky_eff]
	 }
	 set scr [regReadAsFits [regNew] $infile]
	 regIntLincom $scr "" $zero $fpC_scale 0
	 regIntCopy $corrimage $scr
	 hdrCopy $scr.hdr $corrimage.hdr; hdrFreeAll $scr.hdr
	 if 0 {			 # RHL read DLS header vars that should be in idCoadds*.par
	    set rnoise [hdrGetAsDbl $corrimage.hdr "RNOISE"]
	    set gain [hdrGetAsDbl $corrimage.hdr "GAIN"]
	    set meanwts [hdrGetAsDbl $corrimage.hdr "MEANWTS"]
	    set skyval [hdrGetAsDbl $corrimage.hdr "SKYVAL"]
	    echo [format "COADD 1 137 $run $camCol $field $filter %7.4f %8.6f %8.2f %10.8f" $meanwts $skyval $gain $rnoise]
	 }
	 regDel $scr
      }

      set maskFile [get_mask_filename $imageDir $run $filter $camCol $field]

      if {![file exists $maskFile] && $fpC_saturLevel($filter) == 0} {
	 verb_echo 1 "Failed to find fpM file for $filter"
      } elseif {[catch {
	 get_mask $maskFile *$corrimage.mask CR INTERP SATUR NOTCHECKED
      } msg]} {
	 verb_echo 0 "Failed to find fpM file for $filter: $msg"
      }

      if {$fpC_saturLevel($filter) > 0} {# (also?) set SATUR from the image
	 set sm [handleBindFromHandle [handleNew] *(SPANMASK*)$corrimage.mask]
	 reg2spanmask $corrimage -spanMask $sm \
	     -types "INTERP SATUR" -level $fpC_saturLevel($filter)
	 handleDel $sm
      }

      global display display_raw
      if {[info exists display] && $display &&
	  [info exists display_raw] && $display_raw} {
	 display $corrimage "as-read corrected frame" $filter 1 1
      }
   } else {
       hdrCopy $rawimage.hdr $corrimage.hdr; hdrFreeAll $rawimage.hdr
       
       verb_echo 4 "Flatfielding the $filter filter"
       # Flatfield the image
       set fix_bias [getsoftpar fix_bias]
       if {[info exists fix_bias] && $fix_bias} {
	   set fix_bias_flg "-fix_bias"
       } else {
	   set fix_bias_flg ""
       }
       
       set bias_scale [ hdrGetAsInt $biasvector.hdr TSHIFT]
       # need psf width for horizontal trail correction  
       set psf_temp [dgpsfNew]  
       handleSetFromHandle $fparams.psf &$psf_temp
       handleSet $fparams.psf->width [exprGet $calib.calib<$index>->psf->width]
      set minval [expr [exprGet $calib.calib<$index>->sky] - \
		      2*[exprGet $calib.calib<$index>->skysig]]
      #
      # Set the fparams.sky from the PSP in case there are any FILCOL defects
      #
      set skyreg [binregionNewFromConst $skyTypical($filter)]
      handleSetFromHandle $fparams.sky &$skyreg
      handleSet $fparams.global_sky $skyTypical($filter)

      eval correctFrames $rawimage $biasvector $flatvector $ccdpars($filter) \
	   $fparams $bias_scale -regout $corrimage \
	   -leftbias [exprGet $calib.calib<$index>->lbias] \
	   -rightbias [exprGet $calib.calib<$index>->rbias] \
	   -minval $minval $fix_bias_flg

      dgpsfDel $psf_temp
      handleSet $fparams.sky 0x0; binregionDel $skyreg; unset skyreg
   }
   #
   # Find frames' estimate of sky
   #
   regStatsFromQuartiles $corrimage -coarse 2 \
       -cmedian cmedian -csigma csigma
   set soft_bias [softBiasGet]
   set cmedian [expr $cmedian-$soft_bias]

   handleSet $fieldstat.sky_frames<$index> $cmedian
   handleSet $fieldstat.sky_psp<$index> [exprGet $calib.calib<$index>->sky]
   
   handleSet $fieldstat.sigpix<$index> $csigma
   
   if 1 {				# use frames' sky/error not PSP's
      set skyTypical($filter) $cmedian
      set skySigma($filter) $csigma
   }
   global frames_skyval
   if [info exists frames_skyval] {	# XXX Debugging only
      echo Setting sky to $frames_skyval;
      set skyTypical($filter) $frames_skyval
   }

   if {$_skyErr != ""} {
      #
      # Estimate sky uncertainty from the 4 quadrants of the image
      #
      # The s.d. of these 4 measurements is a measure of the sky noise,
      # and an extra factor of sqrt(4) gives the sky uncertainty
      #
      set snrow [expr [exprGet $corrimage.nrow]/2];# n.b. rounded down so e.g.
      set sncol [expr [exprGet $corrimage.ncol]/2];# 2*$snrow <= $nrow

      set sum 0
      loop r 0 2 {
	 loop c 0 2 {
	    set sreg [subRegNew $corrimage $snrow $sncol \
			  [expr $r*$snrow] [expr $c*$sncol]]

	    regStatsFromQuartiles $sreg -coarse 2 -cmedian cmedian
	    set cmedian [expr $cmedian - $soft_bias]
	    regDel $sreg

	    set cm($r:$c) $cmedian
	    set sum [expr $sum + $cmedian]
	 }
      }
      set mean [expr $sum/4.0]

      set sum 0
      foreach el [array names cm] {
	 set sum [expr $sum + pow($cm($el) - $mean, 2)]
      }
      
      set skyErr($filter) [expr sqrt($sum/3.0)/sqrt(4)]

      set cmedian $mean
   }
   
   if 1 {
      echo [format \
		[concat "Field %d-%d-%d $filter sky +- err. " \
		     "PSP: %6.2f +- %5.2f Frames: %6.2f +- %5.2f"] \
		[exprGet $fieldparams.run] \
		[exprGet $fieldparams.camCol] \
		[exprGet $fieldparams.fieldnum] \
		[exprGet $fieldstat.sky_psp<$index>] \
		[exprGet $calib.calib<$index>->skysig] \
		$skyTypical($filter) $skySigma($filter)]
   }
   if {$cmedian <= $csigma} {
      global idR_zero
      
      if {$idR_zero < 0} {
	 set fiddle_sky [expr -$idR_zero]
	 echo {$cmedian <= $csigma} : $cmedian <= $csigma (adding $fiddle_sky)
	 regIntConstAdd $corrimage $fiddle_sky
	 set cmedian [expr $cmedian + $fiddle_sky]
	 set skyTypical($filter) $cmedian
      }

      if {$cmedian <= $csigma} {
	 display $corrimage "Corrected frame (pre-CR removal)" $filter 1 1
	 assert {[skip_field [exprGet $fieldparams.fieldnum]] || $cmedian > $csigma}
      }
   }

   handleSet $fparams.bkgd $skyTypical($filter)
   handleSet $fieldstat.saturation_level<$index> \
       [expr 0.5*([exprGet $fparams.fullWell<0>] + \
		      [exprGet $fparams.fullWell<1>])]

   verb_echo 4 [format "Sky Level is %.1f +- %.1f DN " $skyTypical($filter) $skySigma($filter)]
   if {!$read_fpC_files && $cr_min_sigma > 0} {
       #
       # Deal with CRs
       #
       if {$psfBasis == "" || $cr_cond32 > 0} {
	  set psfBasisCR "NULL"
       } else {
	  set psfBasisCR $psfBasis
	  set cr_cond32 [expr abs($cr_cond32)]
       }
       set ncr [eval findCR $corrimage \
		    $skyTypical($filter) -bkgd $skyTypical($filter) \
		    $skySigma($filter) *$calib.calib<$index>->psf $gain \
		    -min_sigma $cr_min_sigma -min_e $cr_min_e \
		    -cond3 $cr_cond3 -cond32 $cr_cond32 \
		    [lindex {-keep {}} [expr $cr_keep?0:1]] \
		    -psfBasis $psfBasisCR -adopted_cond32 c32]

       handleSet $fieldstat.adopted_cond3_fac2<$index> $c32
       handleSet $fieldstat.nCR<$index> $ncr
   }
       
   # Write out the flatfielded image, if so directed
   if {![info exists write_fpC_files] || $write_fpC_files} {
      set outfile $outdir/fpC-$runstr-$filter$camCol-$fieldstr.fit
      if {$compress_image_files != ""} {
	 append outfile ".$compress_image_files"
      }
      hdrInsWithInt $corrimage.hdr "SOFTBIAS" [softBiasGet] \
	  "software \"bias\" added to all DN"
      hdrInsWithDbl $corrimage.hdr "FLUX20" \
	  [exprGet $calib.calib<$index>->flux20] \
	  "Number of DN in 20th mag object"
      hdrInsWithDbl $corrimage.hdr "FLUX0" -9999\
	  "Number of DN in 0th mag object"
      hdrInsWithAscii $corrimage.hdr "FCALI_ID" "None"
      hdrInsWithDbl $corrimage.hdr "SKY" $skyTypical($filter) \
	  "Approx. sky level (DN)"
      hdrInsWithAscii $corrimage.hdr "BUNIT" "DNs"
      hdrInsWithAscii $corrimage.hdr "FILTER" $filter "filter used"
      hdrInsWithInt $corrimage.hdr "CAMCOL" $camCol \
	  "column in the imaging camera"
      insert_id_values -frames $corrimage.hdr
      #
      # Add WCS information to header
      #
      set trans [handleBindFromHandle [handleNew] *$calib.calib<$index>->toGCC]
      atWcsAddToHdr $trans $corrimage.hdr \
	  [exprGet $calib.calib<$index>->node] \
	  [exprGet $calib.calib<$index>->incl]
      handleDel $trans
      
      cleanCorrectedFrameHeader $corrimage.hdr;	# remove blanks etc.
      
      regWriteAsFits $corrimage $outfile
      calc_crc -field $outfile
   }

   handleDel $fparams
}

#
# Deal with information about coadds;
# the type COADD comes from an idCoadd.par file
#
proc coaddNew {} {
   return [genericNew COADD]
}

proc coaddDel {coadd} {
   genericDel $coadd
}

proc init_coadds {_coadds dir file run camCol filterlist} {
   upvar $_coadds coadds

   if {$file == "/dev/null"} {
      if [info exists coadds] {
	 fini_coadds coadds
      }
      return -1
   }

   if ![regexp {^/} $file] {
      set file "$dir/$file"
   }
   calc_crc -field $file

   verb_echo 3 "$file"

   set ch [param2Chain $file hdr]
   #
   # Unpack desired elements
   #
   set cfilterlist {};		# filters present in idCoadd file

   set curs [chainCursorNew $ch]
   while {[set coadd [chainWalk $ch $curs]] != ""} {
      assert {[exprGet $coadd.run] == $run}
      if {[exprGet $coadd.camcol] == $camCol} {
	 set field [exprGet $coadd.field]
	 set filter [exprGet $coadd.filter]

	 if {[lsearch $cfilterlist $filter] < 0} {
	    lappend cfilterlist $filter
	 }

	 if {[lsearch $filterlist $filter] >= 0} {
	    if [info exists coadds($field,$filter)] {
	       ; # check that the values we want are the same in both
	    } else {
	       set coadds($field,$filter) $coadd
	       chainElementRemByCursor $ch $curs
	       continue
	    }
	 }
      }
      handleDel $coadd
   }
   chainCursorDel $ch $curs

   chainDestroy $ch coaddDel
   #
   # Check that desired filters were all present
   #
   foreach f $filterlist {
      if {[lsearch $cfilterlist $f] < 0} {
	 error "Failed to find filter $f in file $file ($cfilterlist)"
      }
   }

   return [llength [array names coadds]]
}

proc fini_coadds {_coadds} {
   upvar $_coadds coadds

   if [info exists coadds] {
      foreach c [array names coadds] {
	 coaddDel $coadds($c)
      }
      
      unset coadds
   }
}

#
# Check the primary header in the psCB file verifying that all filters on 
# filterlist are present
#
proc check_calibfile {file filterlist} {
   global id_values
   set hdr [hdrReadAsFits [hdrNew] $file]
   calc_crc -field $file
   set psfilterlist [string trim [hdrGetAsAscii $hdr FILTERS]]
   if [catch {
      set ps_id [hdrGetAsAscii $hdr PS_ID]
   } msg] {
      echo Reading $file: $msg
      set ps_id "devel"
   }
   if {$id_values(PS_ID) != $ps_id} {
      set msg [concat \
	  "PS_ID == \"$ps_id\" read from $file doesn't match \$id_values(PS_ID) == \n" \
	  "         \"$id_values(PS_ID)\" read from $id_values(PS_ID_file)"]
      if $id_values(allow_mismatch) {
	 echo "Error: $msg (continuing)"
      } else {
	 #error $msg
      }
   }
   hdrDel $hdr

   foreach filter $filterlist {
      if {[lsearch $psfilterlist $filter] == -1} {
	 echo "$file does not contain info for filter $filter"
      }
   }
}

#
# read in the CALIB1 structure for this frame (in all colors),
# which include the DGPSFs
#
proc read_calib1byframe {field psFieldFile filterlist} {
   set calib1byframe [get_calib1byframe_from_psField $psFieldFile $filterlist]

   loop i 0 [llength $filterlist] {
      if [catch {expr [exprGet $calib1byframe.calib<$i>->psf->b] >= 0} msg] {
	 echo "$msg; setting to 0"
	 handleSet $calib1byframe.calib<$i>->psf->b 0;# XXX
      }
      if {[exprGet $calib1byframe.calib<$i>->psf->psfCounts] == 0} {
	 if 0 {
	    echo "XXX Setting psf->psfCounts to 1";	# this one isn't really used
	 }
	 handleSet $calib1byframe.calib<$i>->psf->psfCounts 1.0
      }
      if {[exprGet $calib1byframe.calib<$i>->prof->psfCounts] == 0} {
	 if 0 {
	    echo "XXX Setting prof->psfCounts to 1";	# this one isn't really used
	 }
	 handleSet $calib1byframe.calib<$i>->prof->psfCounts 1.0
      }

      assert {[exprGet $calib1byframe.calib<$i>->psf->b] >= 0}
   }
   return $calib1byframe
}

proc read_trans {calibs asTransfile column filterlist} {
   set field [exprGet $calibs.field]

   astrom_read $field $field $column \
       astroms $filterlist $asTransfile node incl
   
   loop i 0 [llength $filterlist] {
      set f [lindex $filterlist $i]

      set trans [chainElementRemByPos $astroms($f) 0]
      handleSetFromHandle $calibs.calib<$i>->toGCC &$trans

      assert {[chainSize $astroms($f)] == 0}
      handleDel $trans; chainDel $astroms($f)
      
      handleSet $calibs.calib<$i>->node $node
      handleSet $calibs.calib<$i>->incl $incl
   }

   return $calibs
}

# read in the KNOWNOBJ structure for this field

proc read_knownobjs {kofile} {
   global run camCol field id_values

   verb_echo 2 "Reading known objects from $kofile"

   if [catch {
      set kos [fits2Schema $kofile KNOWNOBJ]
      
      set hdr [hdrReadAsFits [hdrNew] $kofile]
      calc_crc -field $kofile
      if [catch {set id_values(KO_VER) [hdrGetAsAscii $hdr KO_VER]}] {
	 set id_values(KO_VER) "unknown"
      }
      hdrDel $hdr
   } msg] {
      echo Failed to read known objects: $kofile ($msg)
      set kos [chainNew KNOWNOBJ]
   }
   
   verb_echo 3 "Read [chainSize $kos] known objects"

   return $kos
}


proc fini_correct_frames {_ccdpars} {
   upvar $_ccdpars ccdpars
   global filterlist
   finiCorrectFrames;

   foreach f $filterlist {
      ccdparsDel $ccdpars($f)
   }
   unset ccdpars
}

#
# Given an OBJECT1, extract a profile and set the mask accordingly
#
proc set_mask_from_obj1 { reg val obj1 thresh {clip -1}} {
   set st [profileExtract $reg \
	       [exprGet $obj1.rowc] [exprGet $obj1.colc] 100 1000 3]
   if {$clip < 0} {
      maskSetFromProfile *$reg.mask $val $st $thresh -nsec 2
   } else {
      maskSetFromProfile *$reg.mask $val $st $thresh -clip $clip
   }
   handleDel $st
}

proc find_bright_objects {filter corrimage skyErr} {
   global filterlist fieldparams MASK_TYPE OBJECT1 OBJECT2 OBJECT3

   verb_echo 3 "In find_bright_objects"

   set index [lsearch $filterlist $filter]

   set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<$index>]
   set skyTyp [binregionInterpolate *$fparams.sky 0 0]
   
   set soft_bias [softBiasGet]
   set nsig [getsoftpar fbo_thresholds]
   set npixel_min [getsoftpar fbo_npixel_min]

   verb_echo 4 [format "%s%s" \
	      "fbo($filter): [expr $soft_bias+$skyTyp]+-$skyErr; threshold=" \
		    [expr int($soft_bias+$skyTyp+[lindex $nsig 0]*$skyErr)]]
   set lev [vectorExprEval "int($soft_bias+$skyTyp+{$nsig}*$skyErr)"]
   vectorExprSet $lev "($lev >= 65535) ? 65535 : $lev"

   handleSet $fieldparams.frame<$index>.ffo_threshold \
       [expr ([exprGet $lev.vec<0>] - $soft_bias)]
   handleSet $fieldparams.frame<$index>.smooth_sigma 0
   
   set objects [regFinder $corrimage $lev $fparams \
		    -npixel_min $npixel_min -npixel_min_level [getsoftpar fbo_level_npixel_min]]
   vectorExprDel $lev

   objectChainGrow $objects 0 $corrimage [getsoftpar fbo_ngrow]
   
   set nsigma 40;			# we're going to do a crude deblend
   set nsigma 3;			# XXX for step.  40 deletes peaks in BRIGHT that we want
   objectChainPeaksCull $corrimage $objects -$nsigma -fp $fparams -neff 1
   
   objectChainFlags3Set $objects [expr [llength $nsig] - 1] \
       $OBJECT3(MEASURE_BRIGHT)

   set brightlist [objectToObject1ChainConvert $objects 0 \
				$corrimage *$fparams.sky]

   object1ChainFlagsSet $brightlist [expr $OBJECT1(BRIGHT) | $OBJECT1(BINNED1)]
   
   spanmaskSetFromObject1Chain $brightlist *$corrimage.mask \
       $MASK_TYPE(BRIGHTOBJECT)

   saturSetInObject1Chain $corrimage $brightlist

   verb_echo 4 "Found [exprGet $brightlist.nElements] bright objects"

   objectChainDel $objects
   handleDel $fparams

   return $brightlist
}

proc fini_find_bright_objects {} {
   #;
}

proc init_comp_decomp {} {
  initObjcCompDecomp
}

proc objc_comp_decomp {merge} {
  upvar $merge mergedlist 
  global filterlist

  verb_echo 3 "In objc_comp_decomp"
  
  objcCompDecomp $mergedlist $filterlist 1.
}

proc init_random {_rand} {
   upvar $_rand rand
   global softpars
   
   set seed [getsoftpar fp_random_seed]
   set rand [phRandomNew 100000 -seed $seed -type 2]
}

proc fini_random {rand} {
   phRandomDel $rand
}

proc init_find_objects {fieldparams nrow ncol \
			    _levels _median_size _psf_filt_size \
			    _binshifts _binned_thresholds
			 } {
   upvar $_levels levels  $_binshifts binshifts
   upvar $_median_size median_size $_psf_filt_size psf_filt_size
   global nfilter softpars

   initObjectFinder
   initProfileExtract
   
   set median_size [getsoftpar ffo_median_size]
   set psf_filt_size [getsoftpar ffo_psf_filt_size]
   set thresholds [getsoftpar ffo_thresholds]
   set binshifts [getsoftpar ffo_binshifts]
   if {$binshifts == 0} {
      set binshifts ""
   }
   foreach b $binshifts {
      set var ${_binned_thresholds}_$b
      upvar $var $var
      set $var [getsoftpar $var]
   }
   #
   # The final binned image is the one that's saved to disk; we need to
   # know what its pixel size is
   #
   if {$binshifts == ""} {
      set binfac 1
   } else {
      set binfac [expr 1<<[lrange $binshifts end end]]
   } 
   handleSet $fieldparams.binfac $binfac

   loop i 0 $nfilter {
      handleSet $fieldparams.frame<$i>.peak_smooth_thresh \
	  [getsoftpar peak_smooth_thresh]
   }
   
   set levels [eval vectorExprEval "{{$thresholds}}"]
}

proc find_objects {args} {
   global coadds fieldparams fieldstat filterlist mem MASK_TYPE MODEL_TYPES OBJECT1 OBJECT2 sao verbose

   set opts [list \
		 [list [info level 0] "Find faint objects"] \
		 [list <filter> STRING "" filter "filter"] \
		 [list <out> STRING "" out "Smoothed image"] \
		 [list <bin> STRING "" bin "Unsmoothed image"] \
		 [list <scr> STRING "" scr "Scratch REGION"] \
		 [list <rand> STRING "" rand "A phRandom for sky subtraction"] \
		 [list <levels> STRING "" levels "Thresholds"] \
		 [list <median_size> STRING "" median_size "median_size for sky estimation"] \
		 [list <psf_filt_size> STRING "" psf_filt_size "Size of smoothing filter"] \
		 [list <psfsigma> STRING "" psfsigma "RMS size of seeing disk"] \
		 [list -subtract STRING "" subtract_chain \
		      "Chain of objects that should be subtracted before sky subtraction"] \
		 [list -objectModels STRING "" _objectModels \
		      "Array of CHAINs of OBJECT_MODELs representing subtract_chain"] \
		 [list -subtract_stars INTEGER 0 subtract_stars \
		      "Don't reinstate bright stars after sky subtraction"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$_objectModels != ""} {
      upvar $_objectModels objectModels
   }

   verb_echo 3 "In find_objects ($filter)"
   #
   # If region is binned, unpack the relevant parameters
   #
   if {[handleType $bin] == "BINREGION"} {
      set in [handleBindFromHandle [handleNew] *$bin.reg]
      set rbin [exprGet $bin.bin_row]; set cbin [exprGet $bin.bin_col]
      set binshift [exprGet $bin.shift]
   } else {
      set in $bin
      set rbin 1; set cbin 1
      set binshift 0
   }
   set binfac [expr 1<<$binshift]

   set index [lsearch $filterlist $filter]
   
   #
   # Find desired parameters for frame
   #
   set nbit 10;			# number of bits to shift sky
   #
   # Unpack variables
   #
   set nrow [exprGet $in.nrow]
   set ncol [exprGet $in.ncol]
   set soft_bias [softBiasGet]
   #
   # Median smooth image, subtract sky (but remember to add back the
   # the software bias). If median_size <= 0, assume that the sky
   # has already been subtracted
   #
   if {$median_size <= 0} {
      set skyreg [handleBindFromHandle [handleNew] \
		      *$fieldparams.frame<$index>.sky]
   } else {
      set galaxiesModel $scr;		# n.b. we don't own this REGION
      set starsModel [regIntCopy NULL $galaxiesModel]

      verb_echo 3 "maybe subtract [chainSize $subtract_chain] bright objects"
      write_mem mem "maybe sub [chainSize $subtract_chain] bright objects"
      subtract_bright_objects objectModels $galaxiesModel $starsModel $subtract_chain $filter $fieldparams

      regIntLincom $in $galaxiesModel 0 1 -1
      regIntLincom $in $starsModel    0 1 -1
      
      global display_sky_model
      if {[info exists display_sky_model] && $display_sky_model} {
	 set skyModel [regIntCopy NULL $galaxiesModel]
	 regIntLincom $skyModel $starsModel 0 1 1

	 display $skyModel "Model of bright objects to subtract" $filter 1

	 if {$verbose > 1} {
	    display $starsModel    "    stars" $filter 1
	    display $galaxiesModel "    galaxies" $filter 1
	 }

	 regDel $skyModel
      }
      #
      # Estimate the sky level
      #
      set filt_c $median_size;
      set filt_r $median_size;
      set skysigreg [binregionNew]
      set skyreg [medianSky $in $filt_c $filt_r -shift $nbit \
		      -skysig $skysigreg];
      if 0 {
	 skyEstimateDebias -clipped $skyreg \
	     [exprGet $fieldparams.frame<$index>.gain0] \
	     [exprGet $fieldparams.frame<$index>.gain1]
      }
      regIntConstAdd *$skyreg.reg [expr -$soft_bias*(1<<$nbit)];

      global frames_skyval
      if [info exists frames_skyval] {	# XXX Debugging only
	 binregionDel $skyreg	 
	 set skyreg [binregionNewFromConst -shift $nbit $frames_skyval]
      }
      #
      # Reset the random seed, so each field will be repeatable.
      #
      set seed [set_random_seed $rand "sky" $filter]
      skySubtract $in $in $skyreg $rand
      global display_sky_subtracted
      if $display_sky_subtracted {
	 display $in "sky subtracted" $filter
      }
      #
      # update fieldparams
      #
      handleSetFromHandle $fieldparams.frame<$index>.sky &$skyreg
      handleSetFromHandle $fieldparams.frame<$index>.skyErr &$skysigreg
      handleSet $fieldparams.frame<$index>.bkgd 0;# it's been subtracted
      #
      # and fieldstat. We may update this sky estimate after removing
      # all objects
      #
      set stats [regStatsFind *$skyreg.reg]; set mean [keylget stats mean]
      handleSet $fieldstat.sky_frames_sub<$index> [expr 1.0*$mean/(1<<$nbit)]
      #
      # Put (some of) the subtracted models back now
      #
      regIntLincom $in $galaxiesModel 0 1 +1
      if {$subtract_stars} {		# set bits where the star wing aren't
	 # Set bits in flags
	 loop i 0 [chainSize $objectModels($filter)] {
	    set om [chainElementGetByPos $objectModels($filter) $i]
	    # Get OBJECT1* back from $om.id (hack hack)
	    set obj1 [objectModelPtrGetFromId $om]

	    if {[exprGet $om.type<0>] == $MODEL_TYPES(PSF_MODEL)} {
	       # Object had its wings subracted; record that fact
	       handleSet $obj1.flags2 [expr [exprGet $obj1.flags2] | $OBJECT2(SUBTRACTED_WINGS)]
	    }
	    # Replace that pointer by an honest ID number.  N.b. this isn't the one in the output
	    # tables --- we don't have access to that here. Should we null it instead?
	    handleSet $om.id<0> [exprGet $obj1.id]
	    handleSet $om.id<1> 0
	    handleDel $obj1
	 }
	 # Set bits in fpM
	 regStatsFromQuartiles $in -coarse 5 -cmedian med -csigma sigma
	 set subtractedThreshold [expr 1.0*$sigma]
	 set notcheckedThreshold [expr 5.0*$sigma]

	 set_subtracted_mask $in $starsModel -SUBTRACTED $subtractedThreshold -NOTCHECKED $notcheckedThreshold
      } else {				# put the stars back
	 regIntLincom $in $starsModel 0 1 +1
      }
      regDel $starsModel
      
      if {[info exists display_sky_model] && $display_sky_model} {
	 if $subtract_stars {
	    set objects "galaxies"
	 } else {
	    set objects "objects"
	 }
	 display $in "Model of bright $objects put back" $filter 1
      }
   }
   #
   # Smooth with PSF and look for objects. We scale up the resultant image
   # by a factor of $scale, so as to avoid losing information in the rounding.
   #
   # Note that this changes the effective gain of the chip, and we have
   # to allow for this in the FRAMEPARAMS.
   #
   # If the region was binned, we must allow for this too
   #
   set hpsf_filt_size [expr int($psf_filt_size/2 + 1)]
   if {$psfsigma <= 0} {
      set lshift 0; set norm 1
      regIntCopy $out $in
      set med [softBiasGet]; set sigma 1
   } else {
      set lshift 2; set norm [expr (1<<$lshift)]

      convolveWithGaussian $out $in $scr $psf_filt_size \
	  $psfsigma -lshift $lshift
      regIntConstAdd $out [expr -$soft_bias*($norm - 1)];

      regStatsFromQuartiles $out -coarse 2 -cmedian med -csigma sigma
   }

   set fparams [frameparamsNew $filter]
   foreach p "astrom_floor npeak_per_obj_max" {
      handleSet $fparams.$p [exprGet $fieldparams.frame<$index>.$p]
   }
   handleSet $fparams.data_is_smoothed 1
   handleSet $fparams.smooth_sigma $psfsigma
   loop i 0 2 {
      handleSet $fparams.fullWell<$i> \
	  [exprGet $fieldparams.frame<$index>.fullWell<$i>]
   }

   set fac [expr 1/(1.0*$norm*$binfac)]
   foreach gain "gain0 gain1" {
      handleSet $fparams.$gain \
	  [expr [exprGet $fieldparams.frame<$index>.$gain]*$fac]
   }
   # the gain itself is a BINREGION
   set gain [handleBindFromHandle [handleNew] \
		 [regIntConvert *$fieldparams.frame<$index>.gain->reg -type FL32]]
   regIntConstMult $gain $fac
   handleDel $gain

   # The dark_variance is a BINREGION too
   set dark_variance [handleBindFromHandle [handleNew] \
			  [regIntConvert *$fieldparams.frame<$index>.dark_variance->reg -type FL32]]
   regIntConstMult $dark_variance [expr pow($norm*$binfac,2)]
   handleDel $dark_variance

   set binsky [binregionNewFromBinregion *$fieldparams.frame<$index>.sky]
   handleSet $binsky.shift [expr [exprGet $binsky.shift]-$lshift-$binshift]
   handleSetFromHandle $fparams.sky &$binsky
   handleDel $binsky
   handleSetFromHandle $fparams.psf $fieldparams.frame<$index>.psf

   set save_mask [exprGet $out.mask]
   handleSetFromHandle $out.mask $in.mask; # set object bits in $in.mask
   
   set lev [vectorExprEval $med+$levels*$sigma]

   handleSet $fieldparams.frame<$index>.ffo_threshold \
       [expr ([exprGet $lev.vec<0>] - $soft_bias)/$norm]
   handleSet $fieldparams.frame<$index>.smooth_sigma $psfsigma

   verb_echo 2 \
       [format "ffo(%s): median = %.3f+-%.3f, threshold = %.3f, filter N(0,%.3f^2)" \
	    $filter $med $sigma \
	    [exprGet $fieldparams.frame<$index>.ffo_threshold] \
	    [exprGet $fieldparams.frame<$index>.smooth_sigma]]

   set objects [regFinder $out $lev $fparams \
		    -row0  $hpsf_filt_size -col0  $hpsf_filt_size \
		    -row1 -$hpsf_filt_size -col1 -$hpsf_filt_size \
		    -npixel_min [getsoftpar ffo_npixel_min] \
		   ];
   vectorExprDel $lev
   unset hpsf_filt_size
   #
   # All those peak values are a factor of $norm*$binfac too high; fix them
   #
   set curs [chainCursorNew $objects]
   while {[set obj [chainWalk $objects $curs]] != ""} {
      loop i 0 [exprGet $obj.peaks->npeak] {
	 handleSet $obj.peaks->peaks<$i>->peak \
	     [expr [exprGet $obj.peaks->peaks<$i>->peak]/(1.0*$norm*$binfac)]
      }
      handleDel $obj
   }
   chainCursorDel $objects $curs
   #
   # grow the found objects, and cull the resulting peak lists
   #
   objectChainGrow $objects 0 $in [getsoftpar ffo_ngrow]

   if {$psfsigma <= 0} {		# no smoothing was applied
      set neff 1
   } else {
      set neff [expr 4*3.14*pow($psfsigma,2)];# neff of smoothing filter
   }
   set nsigma 3;			# XXX
   objectChainPeaksCull $out $objects -$nsigma -fp $fparams -neff $neff
   
   verb_echo 4 "Found [exprGet $objects.nElements] objects"
   #
   # Correct binned objects for the effects of the binning, putting
   # them back on the original coordinate system
   #
   if {[handleType $bin] == "REGION"} {
      set mask_type $MASK_TYPE(OBJECT)
   } else {
      set mask_type $MASK_TYPE(BINOBJECT)
      set brow0 [exprGet $bin.row0]
      set bcol0 [exprGet $bin.col0]

      objectChainUnbin $objects $rbin $cbin $brow0 $bcol0 \
	  -nrow [exprGet $bin.rsize] -ncol [exprGet $bin.csize]
   }
   #
   # convert from OBJECTs to OBJECT1s
   #
   set objs [objectToObject1ChainConvert $objects 0 $in $skyreg]

   chainDel $objects
   handleSet $fparams.psf 0
   frameparamsDel $fparams
   #
   # Set the MASK_OBJECT bits where appropriate
   #
   spanmaskSetFromObject1Chain $objs *$in.mask $mask_type

   if {$mask_type == "$MASK_TYPE(OBJECT)"} {# not binned
      object1ChainFlagsSet $objs $OBJECT1(BINNED1)
   } else {
      ;					# BINNED2 was set by objectChainUnbin
   }

   global display display_smooth
   if {[info exists display] && $display &&
       [info exists display_smooth] && $display_smooth} {

      if {[handleType $bin] == "BINREGION"} {
	 handleSetFromHandle $bin.reg &$out
	 set dreg [binregionUnbin $bin $rand \
		       -addcon [expr ($binfac-1)*$soft_bias] -bkgd $soft_bias]
	 handleSetFromHandle $bin.reg &$in
      } else {
	 set dreg $out
      }
      
      handleSetFromHandle $dreg.mask $in.mask
      global display_peaks
      if {[info exists sao($filter)] &&
	  [info exists display_peaks] && $display_peaks} {
	 set curs [chainCursorNew $objs]
	 while {[set obj1 [chainWalk $objs $curs]] != ""} {
	    draw_peaks $obj1 $filter
	    handleDel $obj1
	 }
	 chainCursorDel $objs $curs
      }

      set title ""
      if {$binfac != 1} {
	 set title "$binfac*$binfac binned and "
      }
      set title "${title}PSF smoothed"
      display $dreg $title $filter 1

      if {$dreg != $out} {
	 handleSet $dreg.mask 0x0
	 regDel $dreg
      }
   }

   if {$in != $bin} {
      handleDel $in
   }
   handleSet (long)$out.mask $save_mask;	# restore the saved pointer
   handleDel $skyreg
   catch { handleDel $skysigreg };	# may not exist
   
   verb_echo 3 "Leaving find_objects"
   
   return $objs
}
   
proc fini_find_objects {levels} {
   vectorExprDel $levels
   finiProfileExtract
   finiObjectFinder
}

proc init_merge_colors {fieldparams} {
   global nfilter softpars

   initMergeColors $nfilter

   loop i 0 $nfilter {
      handleSet $fieldparams.frame<$i>.peak_min_sep [getsoftpar peak_min_sep]
   }
}

proc merge_colors {_objectlist fieldparams _mergedlist {rand ""} {cut_atlas {}} {no_restore_ai {}}} {
   upvar $_objectlist objectlist  $_mergedlist mergedlist
   global filterlist

   verb_echo 3 "In merge_colors"
   if [info exists mergedlist] {
      verb_echo 4 "[chainSize $mergedlist] input merged objects"
   }
   
   if {$cut_atlas != ""} {
      assert {$rand != ""}
      
      set cut_atlas "$cut_atlas -bkgd -sig 5 -rand $rand"
   }

   if [catch {
      if {[info exists mergedlist]} {
	 eval mergeColors \"$filterlist\" objectlist \
	     $fieldparams -merge $mergedlist $cut_atlas $no_restore_ai
      } else {
	 set mergedlist [eval mergeColors \"$filterlist\" objectlist \
			     $fieldparams $cut_atlas]
      }
   } msg] {
      global time_left
      if {$msg == "" && $time_left < 0} {
	 error "Timer expired: merging objects"
      } else {
	 return -code error -errorinfo $msg
      }
   }

   verb_echo 4 "[chainSize $mergedlist] output merged objects"
}

proc fini_merge_colors {} {
   finiMergeColors
}

#
# Create an extra set of peaks and merge them into the list as if we'd
# detected them. Useful for running the deblender for doing science (shhhh)
#
proc merge_extra_peaks {extra_peaks_file fieldparams _mergedlist rand} {
   upvar $_mergedlist mergedlist
   global filterlist OBJECT1

   if {$extra_peaks_file == "/dev/null"} {
      return
   }

   foreach f $filterlist {
      set extralists($f) [chainNew "OBJECT1"]
   }

   set fd [open $extra_peaks_file]
   while {[gets $fd line] >= 0} {
      regsub "\[ \t\]*#.*" $line "" lline
      regsub "\[ \t\]*$" $lline "" lline
      if {$lline == ""} {
	 continue;
      }
      set lline [split $lline]

      if {[llength $lline] == 2} {
	 lappend lline "r"
      }

      if {[llength $lline] != 3} {
	 error \
	     "Please Specify row column \[filter\]: $lline ($extra_peaks_file)"
      }

      set rowc [expr int([lindex $lline 0] + 0.5)]
      set colc [expr int([lindex $lline 1] + 0.5)]
      set filter [lindex $lline 2]

      if {[lsearch $filterlist $filter] < 0} {
	 echo "Filter $filter is not being processed; ignoring peak at $line"
	 continue;
      }
      #
      # Create an OBJECT1
      #
      set obj1 [object1New]
      handleSet $obj1.rowc $rowc
      handleSet $obj1.colc $colc
      handleSet $obj1.flags $OBJECT1(BINNED1)
      #
      # Give it one peak at (rowc, colc)
      #
      set peaks [peaksNew 1]
      handleSet $peaks.npeak 1
      handleSet $peaks.peaks<0>->peak  1000
      handleSet $peaks.peaks<0>->rpeak $rowc
      handleSet $peaks.peaks<0>->cpeak $colc
      handleSet $peaks.peaks<0>->rowc [expr $rowc + 0.5]
      handleSet $peaks.peaks<0>->colc [expr $colc + 0.5]
      handleSet $peaks.peaks<0>->rowcErr 0
      handleSet $peaks.peaks<0>->colcErr 0

      handleSetFromHandle $obj1.peaks &$peaks

      handleDel $peaks
      #
      # Set its mask to a single pixel at (rowc, colc)
      #
      set mask [objmaskNew -nspan 1]
      handleSet $mask.nspan 1
      handleSet $mask.rmin $rowc
      handleSet $mask.rmax $rowc
      handleSet $mask.cmin $colc
      handleSet $mask.cmax $colc

      handleSet $mask.s<0>.y $rowc
      handleSet $mask.s<0>.x1 $colc
      handleSet $mask.s<0>.x2 $colc

      handleSetFromHandle $obj1.mask &$mask

      handleDel $mask
      #
      # Add it to object1 chain
      #
      chainElementAddByPos $extralists($filter) $obj1
      handleDel $obj1
   }
   close $fd
   #
   # Actually tell photo about the peaks
   #
   merge_colors extralists $fieldparams mergedlist $rand -cut_atlas

   foreach f $filterlist {
      chainDel $extralists($f)
   }
}

proc merge_known_objects {mergedlist kobjcs rand} {
   global fieldparams filterlist nfilter
   
   if 0 {
      # Add the kobjcs to mergedlist. N.b. this won't work if the kobjcs overlap the merged objects
      chainJoin $mergedlist $kobjcs

      return
   } else {
      # Merge the known objects with the mergedlist 
      foreach f $filterlist {
	 set kolists($f) [chainNew "OBJECT1"]
      }

      loop j 0 [chainSize $kobjcs] {
	 set objc [chainElementGetByPos $kobjcs $j]
	 loop i 0 $nfilter {
	    if {[exprGet (int)$objc.color<$i>] != 0x0} {
	       set obj1 [handleBindFromHandle [handleNew] *$objc.color<$i>]
	       handleSet $objc.color<$i> 0x0
	       break;
	    }
	 }
	 #
	 # Add it to object1 chain
	 #
	 chainElementAddByPos $kolists([lindex $filterlist $i]) $obj1
	 handleDel $obj1; unset obj1
	 handleDel $objc
      }
      chainDestroy $kobjcs objcDel

      merge_colors kolists $fieldparams mergedlist $rand -cut_atlas

      foreach f $filterlist {
	 chainDel $kolists($f)
      }
   }
}

# Write a binned version of the image to disk, in the "outdir" directory,
# followed by the sky and skyerr images
proc write_fpBIN { filter binimage outdir _brightStarWings _objectModels} {
   upvar $_brightStarWings brightStarWings  $_objectModels objectModels
   global fieldparams
   global runstr camCol field
   global compress_image_files
   global wrote_files

   set ifilter [string first $filter [exprGet $fieldparams.filters]]
   set fieldstr [format %04d $field]

   # write binned version of corrected frame, with objects removed
   set outfile $outdir/fpBIN-$runstr-$filter$camCol-$fieldstr.fit
   if {$compress_image_files != "0" && $compress_image_files != ""} {
      if {$compress_image_files != "R"} {
	 verb_echo 2 \
	     ".$compress_image_files compression doesn't work for multiple HDUs in fpBIN files"
      } else {
	 append outfile ".$compress_image_files"
      }
   }
   insert_id_values -frames $binimage.hdr
   hdrInsWithLogical $binimage.hdr "EXTEND" "T"
   #
   # Add WCS information to header
   #
   set trans [handleBindFromHandle [handleNew] \
		  *$fieldparams.frame<$ifilter>.toGCC]
   atWcsAddToHdr $trans $binimage.hdr \
       [exprGet $fieldparams.frame<$ifilter>.gc_node] \
       [exprGet $fieldparams.frame<$ifilter>.gc_incl]
   handleDel $trans

   cleanBinnedFrameHeader $binimage.hdr;# remove blanks etc.

   regWriteAsFits $binimage $outfile

   foreach t "sky skyErr" {
      if {[exprGet (int)$fieldparams.frame<$ifilter>.$t] == 0} {
	 echo "Not writing $t to $outfile"
	 continue;
      }
      
      set reg [handleBindFromHandle [handleNew] \
		   *$fieldparams.frame<$ifilter>.$t->reg]
      #
      # The binned sky/skyErr contain an extra row to the top/right appended
      # by medianSky.  Don't write it to disk.
      #
      set sreg [subRegNew $reg \
		    [expr [exprGet $reg.nrow]-1] \
		    [expr [exprGet $reg.ncol]-1] 0 0]

      foreach val "bin_row bin_col shift" {
	 hdrInsWithInt $sreg.hdr [string toupper $val] \
	     [exprGet $fieldparams.frame<$ifilter>.$t->$val]
      }

      regWriteAsFits -image $sreg $outfile

      regDel $sreg; handleDel $reg
   }
   #
   # And a bintable for the wing regions
   #
   if ![info exists brightStarWings($filter)] {
      set brightStarWings($filter) [chainNew WINGOBJECT]
   }

   schema2Fits -append $brightStarWings($filter) $outfile
   #
   # And a bintable for the subtracted models
   #
   if ![info exists objectModels($filter)] {
      set objectModels($filter) [chainNew OBJECT_MODEL]
   }

   schema2Fits -append $objectModels($filter) $outfile
   #
   # All done
   #
   set wrote_files(fpBIN,$filter) 1
   
   calc_crc -field $outfile
}

# Write the corrected image's associated defect mask to $outdir

proc write_fpM { filter corrimage outdir} {
   global run camCol nfilter field MASK_TYPE wrote_files
   
   set fieldstr [format %04d $field]
   set nrow [exprGet $corrimage.nrow]
   set ncol [exprGet $corrimage.ncol]

  # Write out the bit mask in each color
   set sm [spanmaskNew];
   set SPAN_COOKIE [exprGet $sm.cookie]; spanmaskDel $sm

   set sm [handleBindFromHandle [handleNew] *((SPANMASK*)$corrimage.mask)]
   assert {[exprGet $sm.cookie] == $SPAN_COOKIE}

   set hdr [hdrNew]
   hdrInsWithInt $hdr "RUN" $run "Imaging run number"
   hdrInsWithInt $hdr "CAMCOL" $camCol "Column in the imaging camera"
   hdrInsWithInt $hdr "FIELD" $field "Field sequence number"
   hdrInsWithInt $hdr "NFILTER" $nfilter "number of colours processed"
   hdrInsWithInt $hdr "MASKROWS" $nrow "number of rows in the mask"
   hdrInsWithInt $hdr "MASKCOLS" $ncol "number of columns in the mask"
   hdrInsWithInt $hdr "NPLANE" [array size MASK_TYPE] "number of types of mask"
   insert_id_values -frames $hdr
     
   set outfile [get_mask_filename $outdir $run $filter $camCol $field]
   set fd [fitsBinTblOpen $outfile w -hdr $hdr]
   hdrFreeAll $hdr; hdrDel $hdr
     
   loop i 0 [array size MASK_TYPE] {
      set ochain [handleBindFromHandle [handleNew] *$sm.masks<$i>]
	
      fitsBinTblHdrWrite $fd "OBJMASK"
      set curs [chainCursorNew $ochain]
      while {[set objmask [chainWalk $ochain $curs]] != ""} {
	 fitsBinTblRowWrite $fd $objmask	   
	 handleDel $objmask
      }
      fitsBinTblEnd $fd
      chainCursorDel $ochain $curs
      handleDel $ochain
   }
   fitsBinTblClose $fd; handleDel $sm

   append_types_to_file $outfile S_MASKTYPE   
   set wrote_files(fpM,$filter) 1

   calc_crc -field $outfile
}

proc init_measure_objects {fieldparams _refcolor rand} {
   upvar $_refcolor refcolor
   global nfilter filterlist softpars

   set pixscale [getsoftpar fp_pixscale]
   set sblist ""
   foreach f $filterlist {
      if ![getsoftpar mo_sbellip_$f sb] {
	 set sb [getsoftpar mo_sbellip_default]
      }
      lappend sblist $sb
   }
   set fiber_rad [expr 0.5*([getsoftpar mo_fiber_dia]/$pixscale)]
   set refcolor [getsoftpar mo_fiber_color]

   set ref_index [lsearch $filterlist $refcolor]
   if { $ref_index == -1 } { 
      puts -nonewline \
	  "init_measure_objects: cannot find refcolor $refcolor in list $filterlist"
      set ref_index 0
      set refcolor [lindex $filterlist $ref_index]
      echo "; using $refcolor"
   }
   handleSet $fieldparams.pixscale $pixscale
   handleSet $fieldparams.smooth_profs [getsoftpar mo_smooth_profs]
   handleSet $fieldparams.fiber_rad $fiber_rad
   handleSet $fieldparams.ref_band_index $ref_index

   foreach par [list \
		    petro_f1 \
		    petro_f2 \
		    petro_f4 \
		    petro_f5 \
		    nchild_max \
		    deblend_min_peak_spacing \
		    deblend_psf_nann \
		    deblend_psf_Lmin \
		    deblend_psf_rad_max \
		    deblend_npix_max \
		    smooth_petro_radii \
		    petro_gcv_errors \
		    deblend_min_detect \
		    deblend_allowed_unassigned \
		    astrom_tweak_n_min \
		    use_galaxy_ap_correction \
		    deblend_inner_max \
		    psf_constraint_nmodel \
		    deblend_bright_objects \
		   ] {
      handleSet $fieldparams.$par [expr [getsoftpar $par]]
   }

   loop i 0 3 {
      handleSet $fieldparams.sg_classifier<$i> [getsoftpar sg_classifier$i]
   }

   loop i 0 $nfilter {
      set f [lindex $filterlist $i]

      handleSet $fieldparams.frame<$i>.min_peak_statistical_errors \
	  [getsoftpar min_peak_statistical_errors]
      handleSet $fieldparams.frame<$i>.npeak_per_obj_max \
	  [getsoftpar npeak_per_obj_max]
      handleSet $fieldparams.frame<$i>.extra_bad_satur_columns \
	  [getsoftpar extra_bad_satur_columns]
      handleSet $fieldparams.frame<$i>.astrom_floor [getsoftpar astrom_floor]

      if ![getsoftpar fiberCounts_seeing_$f fiber_seeing] {
	 set fiber_seeing [getsoftpar fiberCounts_seeing_default]
      }
      handleSet $fieldparams.frame<$i>.fiberCounts_seeing $fiber_seeing
   }
   
   initMeasureObj $fieldparams $nfilter $sblist $rand
   set rawfile [envscan [getsoftpar rawprofile_file]]
   if ![file exists $rawfile] {
      verb_echo 1 "$rawfile does not exist; you won't be able to make models"
   } else {
      initFitobj $rawfile;
      #calc_crc $rawfile;		# no need to CRC as not used
   }

   set cellfile [envscan [getsoftpar cellprofile_file]]
   initCellFitobj $cellfile $nfilter; calc_crc $cellfile
}

proc fini_measure_objects {} {
  finiMeasureObj
  finiFitobj
  finiCellFitobj
}

#
# A utility function to write an OBJC to the output files
#
proc write_objc { objc _obj_id field \
		      fd_objcIo fd_aimage fd_test_info {display 0}} {
   upvar $_obj_id obj_id
   catch {
      upvar 2 display_objs display_objs;# upvar n is a software disaster!
   }
   global run rerun camCol
   global OBJECT1 sao fieldparams
   global write_bright_atlas_images
   global objcIo
   #
   # if $compress_ai is true we:
   #    subtract all children's atlas images from parent, and
   #    don't save children's master_masks (which are equal to the parent's)
   # to save space on disk. The total saving is c. 25%, and makes
   # reconstructing atlas images significantly more complicated
   #
   set compress_ai 0
   
   if {$compress_ai && [exprGet $objc.nchild] > 0} {
      children_aimages_arithmetic $objc - -[softBiasGet]
   }

   set iter [handleBindFromHandle [handleNew] $objc]
   while 1 {
      set objcIo [objcIoNewFromObjc $iter]

      if {[exprGet $objcIo] == ""} {
	 handleDel $objcIo; unset objcIo
	 break;				# no more OBJCs to write
      }

      incr obj_id
      if {$obj_id%100 == 0} {
	 upvar 2 mem mem; write_mem mem "measured $obj_id objects"
      }

      handleSet $objcIo.id $obj_id
      handleSet $objcIo.aimage->id $obj_id
      handleSet $objcIo.test->id $obj_id

      foreach el "run rerun camCol field" {
	 if ![regexp {^[0-9]+$} [set $el]] {# not an int
	    set $el -1
	 }
	 handleSet $objcIo.aimage->$el [set $el]
      }

      if {$iter == "NULL"} {		# we've been around this loop before
	 handleSet $objcIo.parent $parent_id
	 handleSet $objcIo.aimage->parent $parent_id
	 if {[exprGet $objcIo.nchild] > 0} {
	    set parent_id $obj_id
	 }
      } else {
	 set parent_id $obj_id
	 handleDel $iter
	 set iter "NULL"
      }
      
      if {$display && [info exists display_objs] && $display_objs} {
	 loop i 0 [exprGet $objcIo.ncolor] {
	    draw_cross o [exprGet $objcIo.rowc<$i>] [exprGet $objcIo.colc<$i>] "-s $sao($i)"
	 }
      }

      global display_children
      if {$display && $display_children} {
	 mtv_objc_debug $objcIo -fsao 1
      }

      fitsBinTblRowWrite $fd_objcIo $objcIo

      if {([exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)) &&
	  !$write_bright_atlas_images} {
	 set ai [atlasImageNew]
	 if {$fd_aimage != ""} {
	    fitsBinTblRowWrite $fd_aimage $ai
	 }
	 atlasImageDel $ai
      } else {
	 if {$compress_ai && [exprGet $objcIo.parent] >= 0} {
	    set mm [handleBindFromHandle [handleNew] \
			*$objcIo.aimage->master_mask]	       
	    set dummy [objmaskNew]
	    handleSetFromHandle $objcIo.aimage->master_mask &$dummy
	    handleDel $dummy
	    
	    if {$fd_aimage != ""} {
	       fitsBinTblRowWrite $fd_aimage *$objcIo.aimage
	    }
	    
	    objmaskDel *$objcIo.aimage->master_mask
	    handleSetFromHandle $objcIo.aimage->master_mask &$mm
	    handleDel $mm
	 } else {
	    if {$fd_aimage != ""} {
	       fitsBinTblRowWrite $fd_aimage *$objcIo.aimage
	    }
	 }
      }

      if {"$fd_test_info" != ""} {
	 if [catch {
	    infofileMatch \
		[exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc] 5
	 } info] {
	    if {$info != "phInfofileMatch returns NULL"} {
	       echo [string trim $info]
	    }
	    handleSet $objcIo.test->true_id 0
	    handleSet $objcIo.test->true_type 0
	 } else {
	    handleSet $objcIo.test->true_id [exprGet $info.props<0>]
	    handleSet $objcIo.test->true_type [exprGet $info.props<1>]
	    handleDel $info
	 }
	 fitsBinTblRowWrite $fd_test_info *$objcIo.test
      }
      
      objcIoDel $objcIo
   }
}

#
# Add or subtract all children's atlas images to/from parent
#
proc children_aimages_arithmetic {objc {op +} {bias 0}} {
   global OBJECT1

   if {$op == "+"} {
      set op "Plus"
   } elseif {$op == "-"} {
      set op "Minus"
   }

   if {$op != "Plus" && $op != "Minus"} {
      error "The only allowed operators are +, Plus, -, and Minus"
   }

   objcIoDel [objcIoNewFromObjc $objc];	# the original objc
   if {[exprGet $objc.flags] & $OBJECT1(BRIGHT)} {
      objcIoDel [objcIoNewFromObjc "NULL"]; # skip to blended sibling
   }
   
   while 1 {
      set objcIo [objcIoNewFromObjc NULL]

      if {[exprGet $objcIo] == ""} {
	 handleDel $objcIo
	 return;			# no more children to subtract
      }
      
      assert { [exprGet $objcIo.nchild] == 0 }
      atlasImages${op}Equals *$objc.aimage *$objcIo.aimage -bias $bias
      
      objcIoDel $objcIo
   }
}
#
# Draw an object list
#
proc show_object_list {f objlist} {
   global display_peaks sao
   if {[info exists sao($f)] &&
       [info exists display_peaks] && $display_peaks} {
      if {$display_peaks > 1} {
	 set label_peaks $display_peaks
      } else {
	 set label_peaks 0
      }
      
      saoReset $sao($f)

      set curs [chainCursorNew $objlist]
      while {[set obj1 [chainWalk $objlist $curs]] != ""} {
	 draw_peaks $obj1 $f 0 0 -e 0 $label_peaks
	 handleDel $obj1
      }
      chainCursorDel $objlist $curs
   }
}

proc draw_objs {objc id {display_objs 1}} {
   global filterlist sao display_filterlist

   set ai [handleBindFromHandle [handleNew] *$objc.aimage]
   
   set r0 [expr [exprGet $ai.master_mask->row0] +\
	       [exprGet $ai.master_mask->rmin]]
   set r1 [expr [exprGet $ai.master_mask->row0] +\
	       [exprGet $ai.master_mask->rmax]]
   set c0 [expr [exprGet $ai.master_mask->col0] +\
	       [exprGet $ai.master_mask->cmin]]
   set c1 [expr [exprGet $ai.master_mask->col0] +\
	       [exprGet $ai.master_mask->cmax]]

   saoDrawCache
   foreach f $filterlist {
      if {[info exists display_filterlist] &&
	  [lsearch $display_filterlist $f] == -1} {
	 continue
      }
      
      set ifilter [lsearch $filterlist $f]
      set obj1 [handleBindFromHandle [handleNew] *$objc.color<$ifilter>]
      
      global OBJECT1
      if {[info exists sao($f)] &&
	   (1 || (![expr [exprGet $obj1.flags]&$OBJECT1(SUBTRACTED)] && \
		      [exprGet (int)$obj1.region] != 0))} {
	 set colc [exprGet $obj1.colc]
	 set rowc [exprGet $obj1.rowc]
	    
	 set drow [exprGet $ai.drow<$ifilter>]
	 set dcol [exprGet $ai.dcol<$ifilter>]
	 
	 set rs [expr $r0+$drow]
	 set re [expr $r1+$drow+1]
	 set cs [expr $c0+$dcol]
	 set ce [expr $c1+$dcol+1]
	 
	 draw_cross "o" $rowc $colc " -s $sao($f)"

	 if {$display_objs > 1} {
	    saoDrawPolygon -s $sao($f) \
		$rs $cs $rs $ce $re $ce $re $cs $rs $cs
	 
	    saoDrawText -s $sao($f) $rs [expr $ce+1] $id
	 }

	 global display_ko
	 if {$display_ko > 1} {
	    saoDrawText -s $sao($f) $rs [expr $ce+1] [exprGet $objc.catID]
	    draw_cross "+" $rowc $colc " -s $sao($f)"
	 }
	 
	 global display_peaks display_peaks
	 if {[info exists display_peaks] && $display_peaks} {
	    draw_peaks $obj1 $f
	    draw_peaks $objc $f $drow $dcol -i
	 }
	 if {0 && $id == 24 && $f == [lindex $filterlist end]} { error $objc }
	 
	 global display_petroRad display_petroRad;
	 if {[info exists display_petroRad] && $display_petroRad} {
	    if {[exprGet $obj1.petroRad] > 0} {
	       saoDrawCircle -s $sao($f) \
		   $rowc $colc [exprGet $obj1.petroRad]
	    }
	 }

	 global display_petroRadUQ display_petroRadUQ;
	 if {[info exists display_petroRadUQ] && $display_petroRadUQ} {
	    set colc [exprGet $obj1.colc]
	    set rowc [exprGet $obj1.rowc]
	    
	    set rP [exprGet $obj1.petroRad]
	    if {$rP > 0} {
	       set Q [exprGet $obj1.QErr]
	       set U [exprGet $obj1.UErr]
	       set P [expr sqrt($U*$U + $Q*$Q)]
	       if {$P == 0} {
		  set phi 0;		# doesn't matter
	       } else {
		  if [catch {
		     set phi [expr 180.0/3.141592654*(atan2($U, $Q)/2.0)]
		  } msg] {
		     echo 180.0/3.141592654*(atan2($U, $Q)/2.0): $msg
		     set phi 0
		  }
	       }

	       set ab [expr (1 - $P)/(1 + $P)]
	       if {$ab == 0} {
		  set ab 1e-5
	       }
		  
	       saoDrawEllipse -e -s $sao($f) \
		   $rowc $colc [expr $ab*$rP] [expr $rP/$ab] $phi


	       set Q [exprGet $obj1.Q]
	       set U [exprGet $obj1.U]
	       set P [expr sqrt($U*$U + $Q*$Q)]
	       if {$P == 0} {
		  set phi 0;		# doesn't matter
	       } else {
		  if [catch {
		     set phi [expr 180.0/3.141592654*(atan2($U, $Q)/2.0)]
		  } msg] {
		     echo 180.0/3.141592654*(atan2($U, $Q)/2.0): $msg
		     set phi 0
		  }
	       }

	       set ab [expr (1 - $P)/(1 + $P)]
	       if {$ab == 0} {
		  set ab 1e-5
	       }
		  
	       saoDrawEllipse -i -s $sao($f) \
		   $rowc $colc [expr $ab*$rP] [expr $rP/$ab] $phi
	    }
	 }

	 global display_isoEllipse display_isoEllipse;
	 if {[info exists display_isoEllipse] && $display_isoEllipse} {
	    set rowc [exprGet $obj1.iso_rowc]
	    set colc [exprGet $obj1.iso_colc]

	    if {$rowc >= 0} {
	       draw_cross + $rowc $colc "-s $sao($f)"
	       saoDrawEllipse -s $sao($f) $rowc $colc \
		   [exprGet $obj1.iso_a] [exprGet $obj1.iso_b] \
		   [exprGet $obj1.iso_phi]
	    }
	 }
      }
      handleDel $obj1
   }
   saoDrawCache -off
   handleDel $ai
}

#
# Set up measure objects for this particular field
#
proc init_measure_objects_field { _obj_id _corrimage _skyreg _skysigreg \
				      _psfBasis calibs fieldparams fieldstat \
				      outputDir file_str bintable_hdr \
				      _file_objcIo _fd_objcIo \
				      _file_aimage _fd_aimage \
				      _fd_test_info \
				   } \
{
   upvar $_obj_id obj_id
   upvar $_corrimage corrimage $_skyreg skyreg $_skysigreg skysigreg
   upvar $_psfBasis psfBasis
   upvar $_fd_objcIo fd_objcIo  $_fd_aimage fd_aimage
   upvar $_file_objcIo file_objcIo  $_file_aimage file_aimage
   upvar $_fd_test_info fd_test_info
   global filterlist nfilter corrows corcols verbose write_fpAtlas_files
#
# initialise the parameters for this frame in each colour; we need to
# process the canonical band _last_
#
   set cc [exprGet $fieldparams.ref_band_index]
   loop i 0 $nfilter {
      if {$i != $cc} {
	 lappend ordered_indices $i
      }
   }
   lappend ordered_indices $cc
   
   foreach if $ordered_indices {
      set f [lindex $filterlist $if]
      measureObjColorSet $fieldparams $fieldstat $if \
	  *$calibs.calib<$if> $corrimage($f) \
	  $skyreg($f) $skysigreg($f) $psfBasis($f)

      handleDel $skyreg($f); unset skyreg($f)
      handleDel $skysigreg($f); unset skysigreg($f)

      if {$psfBasis($f) != ""} {
	 handleDel $psfBasis($f)
      }
      unset psfBasis($f)
   }
   
   set obj_id 0;			# unique object ID in outputs
   #
   # Put the gains/variances into the header
   #
   loop c 0 $nfilter {
      lappend dvs [exprGet $fieldparams.frame<$c>.dark_variance]
      lappend gains [exprGet $fieldparams.frame<$c>.gain]
   }
   hdrInsWithAscii $bintable_hdr "DARK_VAR" [join $dvs " "] \
       "Per-pixel variances at zero DN; DN^2"
   hdrInsWithAscii $bintable_hdr "GAIN" [join $gains " "] \
       "Mean gains of amplifiers; e/DN"
   #
   # Binned chips have astrometric offsets that need to be _added_
   # to the measured column positions; include the appropriate
   # corrections for the left and right side of the chips
   #
   hdrInsWithInt $bintable_hdr "SPLIT_LR" \
       [expr [exprGet $fieldparams.frame<0>.data->ncol]/2] \
       "Number of columns in left 1/2 of CCD "

   foreach f $filterlist {
      set i [lsearch $filterlist $f]
      
      hdrInsWithDbl $bintable_hdr "DCOL_L_[string toupper $f]" \
	  [exprGet $fieldparams.frame<$i>.astrom_dcol_left] \
	  "offset to ADD to colc (CCD left 1/2)"
      hdrInsWithDbl $bintable_hdr "DCOL_R_[string toupper $f]" \
	  [exprGet $fieldparams.frame<$i>.astrom_dcol_right] \
	  "offset to ADD to colc (CCD right 1/2)"
   }
   #
   # open output Fits Binary Tables for measured objects
   #
   global write_test_info
   if ![info exists write_test_info] {
      set write_test_info 0
   }

   open_photo_tables $outputDir $file_str \
       $bintable_hdr $corrows $corcols [softBiasGet] \
       file_objcIo fd_objcIo file_aimage fd_aimage \
       $write_test_info fd_test_info 1 $write_fpAtlas_files

   if $write_test_info {
      global truth_file truth_format truth_cols truth_dist run camCol
      if [info exists truth_file] {
	 set file [eval format $truth_file [exprGet $fieldparams.fieldnum]]
	 if [catch {
	    infofileRead $file $truth_format $truth_cols $truth_dist} msg] {
	    echo [string trim $msg]
	    infofileFini
	 }
      }
   }
}

proc open_photo_tables {outputDir file_str \
			    bintable_hdr corrows corcols soft_bias \
			    _file_objcIo _fd_objcIo \
			    _file_aimage _fd_aimage \
			    write_test_info _fd_test_info \
			    {is_fpObjc 1} {write_aimage 1}} \
{
   upvar $_fd_objcIo fd_objcIo  $_fd_aimage fd_aimage
   upvar $_file_objcIo file_objcIo  $_file_aimage file_aimage
   upvar $_fd_test_info fd_test_info

   if {$file_str == ""} {
      set file_objcIo ""; set fd_objcIo ""
      set file_aimage ""; set fd_aimage ""
      set fd_test_info ""
      return
   }

   if $is_fpObjc {
       set file_objcIo $outputDir/fpObjc-$file_str.fit
       set fd_objcIo [fitsBinTblOpen $file_objcIo w -hdr $bintable_hdr]
       fitsBinTblHdrWrite $fd_objcIo "OBJC_IO"
   } else {
       set file_objcIo $outputDir/tsObj-$file_str.fit
       set fd_objcIo [fitsBinTblOpen $file_objcIo w -hdr $bintable_hdr]
       fitsBinTblHdrWrite $fd_objcIo "TSOBJ"
   }
   verb_echo 3 "opening FITS file $file_objcIo"
   
   hdrInsWithInt $bintable_hdr "NROW" $corrows
   hdrInsWithInt $bintable_hdr "NCOL" $corcols
   hdrInsWithInt $bintable_hdr "SOFTBIAS" $soft_bias \
       "software \"bias\" added to all DN"

   if $write_aimage {
       set file_aimage $outputDir/fpAtlas-$file_str.fit
       verb_echo 3 "opening FITS file $file_aimage"
       set fd_aimage [fitsBinTblOpen $file_aimage w -hdr $bintable_hdr]
       fitsBinTblHdrWrite $fd_aimage "ATLAS_IMAGE"
   } else {
       set file_aimage ""
       set fd_aimage ""
   }

   set file_test $outputDir/fpObjc-$file_str-TST.fit
   if $write_test_info {
      verb_echo 3 "opening FITS file $file_test"
      set fd_test_info [fitsBinTblOpen $file_test w -hdr $bintable_hdr]
      fitsBinTblHdrWrite $fd_test_info "TEST_INFO"
   } else {
      if [file exists $file_test] {
	 catch { unlink $file_test }
      }
      set fd_test_info ""
   }
}

proc fini_measure_objects_field {filterlist fieldparams \
				     {file_objcIo ""} {fd_objcIo ""} \
				     {file_aimage ""} {fd_aimage ""} \
				     {fd_test_info ""} \
				  } {
   measureObjUnset $fieldparams
   
   if {$fd_objcIo != ""} {
      fitsBinTblClose $fd_objcIo;
   }
   if {$fd_aimage != ""} {
      fitsBinTblClose $fd_aimage;
   }
   if {$fd_test_info != ""} {
      fitsBinTblClose $fd_test_info;
      infofileFini
   }

   if {$file_objcIo != ""} {
      append_types_to_file $file_objcIo OBJECT1_FLAGS OBJECT2_FLAGS OBJ_TYPE
      calc_crc -field $file_objcIo
   }
   if {$file_aimage != ""} {
      calc_crc -field $file_aimage
   }
}

#
# Here're the routines to actually measure merged object lists
#
proc measure_objects { _obj_id merged fd_objcIo fd_aimage fd_test_info \
			   fieldparams fieldstat {model_fit ""}} \
{
   global corrows corcols display time_left no_measure_objects
   global filterlist nfilter
   upvar $_obj_id obj_id

   verb_echo 3 "In measure_objects; [chainSize $merged] OBJCs"
#
# Get scratch space for the deblender
#
   loop i 0 4 {
      overlayGet scr$i $corrows $corcols
   }

   deblendSet $scr0 $scr1 $scr2 $scr3
#
# measure the objects, emptying the chain as we go
#
   set soft_bias [softBiasGet]
   set field [exprGet $fieldparams.fieldnum]
   set curs [chainCursorNew $merged]

   #
   # Prepare to save the union of all per-band flags
   #
   set flags [list flags flags2]
   foreach f $filterlist {
       foreach flag $flags {
	   set union_flag($f:$flag) 0
       }
   }

   while {[set objc [chainWalk $merged $curs]] != ""} {
      if !$no_measure_objects {
	 eval measureObjc $objc $fieldparams $fieldstat -remove $model_fit
      
	 write_objc \
	     $objc obj_id $field $fd_objcIo $fd_aimage $fd_test_info $display

	 if {0 && [exprGet $objc.id] == 40} {
	    set rand [lindex [handleListFromType RANDOM] 0]
	    set c 0
	    regSetFromAtlasImage *$objc.aimage $c (*$fieldparams.frame<$c>.data)
	    display (*$fieldparams.frame<$c>.data) "Object [exprGet $objc.id]" \
		[lindex [exprGet $fieldparams.filters] $c] 1
	    regSetValFromAtlasImage (*$fieldparams.frame<$c>.data) $c *$objc.aimage \
		[softBiasGet] -rand $rand -sigma 5
	 }

	 loop i 0 $nfilter {
	     set f [lindex $filterlist $i]
	     foreach flag $flags {
		 set union_flag($f:$flag) \
		     [expr $union_flag($f:$flag) | [exprGet $objc.color<$i>->$flag]]
	     }
	 }
      }


      chainElementRemByCursor $merged $curs
      objcDel $objc

      if {$time_left < 0 || [strategicMemoryReserveIsEmpty]} {
	 break
      }
   }

   # Save the unioned objc flags into the fieldstat struct.
   loop i 0 $nfilter {
       set f [lindex $filterlist $i]

       foreach flag $flags {
	   handleSet $fieldstat.objc_$flag<$i> $union_flag($f:$flag)
       }
   }
    
   deblendUnset
   
   if [info exists reg_scr] {
      overlayRelease reg_scr
   } else {
      loop i 0 4 {
	 overlayRelease scr$i
      }
   }

   chainCursorDel $merged $curs
   profileExtract NULL 0 0 0 0 0 -id -1;# invalidate the cached ID

   if {$time_left < 0} {
      error "Timer expired: measuring objects"
   }
   if [strategicMemoryReserveIsEmpty] {
      error "Almost out of memory: measuring objects"
   }
}

#
# Given a chain of bright objects, measure some. As we measure them we
# accumulate a chain describing which objects should be subtracted, and
# return it
#
proc measure_bright_objects {_obj_id merged subtract_cts subtract_npixel_min \
				 fd_objcIo fd_aimage fd_test_info \
				 fieldparams fieldstat _skyTypical \
				 {model_fit ""} } \
{
   upvar $_obj_id obj_id  $_skyTypical skyTypical
   global corrows corcols display filterlist nfilter time_left OBJECT1 OBJECT2 OBJECT3
   global no_measure_objects

   verb_echo 3 "In measure_bright_objects; [chainSize $merged] OBJCs"

   set sub_chain [chainNew OBJC]
#
# Get scratch space for the deblender
#
   loop i 0 4 {
      overlayGet scr$i $corrows $corcols
   }

   deblendSet $scr0 $scr1 $scr2 $scr3
#
# Go through the chain, measuring the properties of OBJECT3(MEASURE_BRIGHT)
# objects, and moving objects with peak counts greater than $sub_peak onto
# $sub_chain
#
   set id 0;				# counter for memory log file
   set curs [chainCursorNew $merged]
   while {[set objc [chainWalk $merged $curs]] != ""} {
      if {[exprGet $objc.flags3] & $OBJECT3(MEASURE_BRIGHT)} {
	 if !$no_measure_objects {
	    eval measureObjc $objc $fieldparams $fieldstat -bright $model_fit -remove
	    
	    if {[incr id]%100 == 0} {
	       upvar 2 mem mem; write_mem mem "measured $id bright objects"
	    }
	 }
      }

      if {$time_left < 0 || [strategicMemoryReserveIsEmpty]} {
	 handleDel $objc
	 break;
      }
      #
      # Go through family deciding whether to put objc onto sub_chain to be subtracted
      #
      set obj [objcDescendentNext $objc]; handleDel $objc
      while {$obj != ""} {
	 global display_children
	 if {$display && $display_children} {
	    mtv_objc_debug $obj -fsao 1
	 }

	 loop i 0 $nfilter {
	    if {[exprGet (int)$obj.color<$i>] != 0 &&
		($subtract_cts > 0 || $subtract_npixel_min > 0) &&
		(
		 ($subtract_cts <= 0    || [exprGet $obj.color<$i>->counts_model] > $subtract_cts) &&
		 ($subtract_npixel_min <= 0 || [exprGet $obj.color<$i>->mask->npix]   > $subtract_npixel_min)
		 )} {
	       break;
	    }
	 }
	 
	 if {$i == $nfilter} {		# too faint to subtract
	    ;
	 } elseif {[exprGet $obj.nchild] > 1} {
	    ;				# i.e. skip parent
	 } elseif {0 && ([exprGet $obj.flags2] & $OBJECT2(SATUR_CENTER))} {
	    ;				# i.e. skip saturated stars
	 } else {
	    verb_echo 3 [format "Deciding to subtract object %3d at (r,c) = %.1f,%.1f) (%7.3eDN, %dpix)" \
			     [exprGet $obj.id] \
			     [exprGet $obj.color<0>->rowc] [exprGet $obj.color<0>->colc] \
			     [exprGet $obj.color<0>->counts_model] [exprGet $obj.color<$i>->mask->npix]]
	    
	    global display_sky_model
	    if {0 &&
		[info exists display] && $display &&
		[info exists display_sky_model] && $display_sky_model} {
	       global sao
	       
	       foreach filter $filterlist {
		  if ![info exists sao($filter)] {
		     continue;
		  }
		  
		  set i [lsearch $filterlist $filter]
		  set obj1 [handleBindFromHandle [handleNew] *$obj.color<$i>]
		  
		  if {[exprGet (int)$obj1.peaks] != 0 && [exprGet $obj1.peaks->npeak] > 0} {
		     set rowc [exprGet $obj1.peaks->peaks<0>->rowc]
		     set colc [exprGet $obj1.peaks->peaks<0>->colc]
		     saoDrawPolygon -s $sao($filter) \
			 [expr $rowc-4] $colc $rowc [expr $colc+4] \
			 [expr $rowc+4] $colc $rowc [expr $colc-4] \
			 [expr $rowc-4] $colc
		  }
		  handleDel $obj1
	       }
	    }
	    
	    chainElementAddByPos $sub_chain $obj
	 }
	 
	 handleDel $obj; set obj [objcDescendentNext NULL]
      }

      if {$time_left < 0 || [strategicMemoryReserveIsEmpty]} {
	 break;
      }
   }
   chainCursorDel $merged $curs
   #
   # Clean up
   #
   deblendUnset

   if [info exists reg_scr] {
      overlayRelease reg_scr
   } else {
      loop i 0 4 {
	 overlayRelease scr$i
      }
   }
   profileExtract NULL 0 0 0 0 0 -id -1;# invalidate the cached ID

   if {$time_left < 0 || [strategicMemoryReserveIsEmpty]} {
      chainDel $sub_chain
      if {$time_left < 0} {
	 error "Timer expired: measuring bright objects"
      } else {
	 error "Almost out of memory: measuring objects"
      }
   }
   
   return $sub_chain
}

#
# Given a chain of measured OBJCs (presumably brightish stars, including
# saturated ones), determine the characteristics of the PSF
#
proc subtract_star_wings {_starWings _corrimages objects fieldparams rand} {
   upvar $_starWings starWings  $_corrimages corrimages
   global filterlist nfilter OBJECT1 OBJECT2

   loop i 0 $nfilter {
      set f [lindex $filterlist $i]
      set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<$i>]

      set cprof [handleBindFromHandle [handleNew] *$fparams.cprof]
      #
      # Unpack PSF
      #
      set beta [exprGet $fparams.psf->beta]
      set sigmap [exprGet $fparams.psf->sigmap]
      set frac [expr [exprGet $fparams.psf->a]*[exprGet $fparams.psf->p0]/\
		    [exprGet $fparams.psf->psfCounts]]
      #
      # Subtract wings of saturated objects
      #
      set corrimage $corrimages($f)
      set binfac 8
      set wingreg [binregionNewFromConst 0 \
		       -nrow [expr [exprGet $corrimage.nrow]/$binfac+1] \
		       -ncol [expr [exprGet $corrimage.ncol]/$binfac+1] \
		       -bin_row $binfac -bin_col $binfac -shift 5]
      set starWings($f) [chainNew WINGOBJECT]

      loop j 0 [chainSize $objects] {
	 set objc [chainElementGetByPos $objects $j]
	 set obj1 [handleBindFromHandle [handleNew] *$objc.color<$i>]

	 if {([exprGet $obj1.flags] & $OBJECT1(BINNED1)) &&
	     ([exprGet $obj1.flags2] & $OBJECT2(SATUR_CENTER))} {
	    set frac 1; echo XXX Faking \$frac = $frac 
	    set sw [brightStarWings $obj1 $cprof $fparams $wingreg \
			 $beta $sigmap $frac \
			 -rtrans 10 -rmax 20 -med -nsigma 7.5]

	    if {$sw != ""} {
	       chainElementAddByPos $starWings($f) $sw TAIL; handleDel $sw
	    }
	 }
	 handleDel $objc; handleDel $obj1
      }

      if {1} {
	 display (*$wingreg.reg) "skySubtract $diff $corrimage $wingreg $rand" $f 1
      }
      skySubtract $corrimage $corrimage $wingreg $rand

      binregionDel $wingreg
      handleDel $cprof
      handleDel $fparams
   }
}


#
# Given a chain of measured OBJCs, return a REGION that contains our
# models of each object.
#

proc subtract_bright_objects {args} {
   global filterlist MASK_TYPE OBJECT1 OBJECT2 MODEL_TYPES

   set opts [list \
		 [list [info level 0] ""] \
		 [list <objectModels> STRING "" _objectModels "Array of CHAINs of OBJECT_MODELs"] \
		 [list <galaxiesModel> STRING "" galaxiesModel "Output region containing galaxies"] \
		 [list <starsModel> STRING "" starsModel "Output region containing stars"] \
		 [list <subtract_chain> STRING "" subtract_chain "subtract_chain"] \
		 [list <filter> STRING "" filter "filter"] \
		 [list <fieldparams> STRING "" fieldparams "fieldparams"] \
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_objectModels objectModels
   set nrow [exprGet $galaxiesModel.nrow]; set ncol [exprGet $galaxiesModel.ncol]   

   set ifilter [lsearch $filterlist $filter]
   set psf [handleBindFromHandle [handleNew] *$fieldparams.frame<$ifilter>.psf]

   assert {![info exists objectModels($filter)]}
   set objectModels($filter) [chainNew OBJECT_MODEL]
   
   regClear $galaxiesModel; regClear $starsModel
   loop j 0 [chainSize $subtract_chain] {
      set objc [chainElementGetByPos $subtract_chain $j]
      set obj1 [handleBindFromHandle [handleNew] *$objc.color<$ifilter>]

      if {([exprGet $obj1.flags2] & $OBJECT2(SATUR_CENTER))} {
	 set names "PSF"
	 set types "PSF"
	 set weights 1
      } elseif {[exprGet -enum $obj1.type] == "OBJ_GALAXY"} {
	 set fracDeV [exprGet $obj1.fracPSF]
	 if {$fracDeV == 0.0} {
	    set types "exp"
	    set weights "1"
	 } elseif {$fracDeV == 1.0} {
	    set types "deV"
	    set weights "1"
	 } else {
	    set types "deV exp"
	    set weights "$fracDeV [expr 1 - $fracDeV]"
	 }

	 set names $types
      } else {
	 set types "psf"
	 set weights 1
	 set names "psf"
	 
	 handleDel $objc;			# we don't do PSFs
	 handleDel $obj1
	 
	 continue
      }
      
      #draw_cross o [exprGet $obj1.rowc] [exprGet $obj1.colc] "-color red"
      
      set om [objectModelNew]
      objectModelIdSetFromPtr $om $obj1

      handleSet $om.ncomp [llength $types]

      loop i 0 [exprGet $om.ncomp] {
	 set type [lindex $types $i]
	 handleSet $om.type<$i> $MODEL_TYPES([string toupper $type]_MODEL)	    

	 handleSet $om.weight<$i> [lindex $weights $i]
	 handleSetFromHandle $om.rowc<$i> $obj1.rowc
	 handleSetFromHandle $om.colc<$i> $obj1.colc
	 assert {[string compare $type psf] != 0}

	 if {$type == "PSF"} {
	    if {$filter == "i"} {
	       set beta 2.4;		# slope of power law
	    } else {
	       set beta 3.25;		# slope of power law
	       #set bets 4
	    }
	    set ipminFit 10;	# minimum index to use in finding power law amplitude
	    set ipminRemove 10;	# minimum index to use in removing wing from frame
	    set frac 1.0;	# fraction of estimated amplitude to remove

	    handleSet $om.spare1<$i> $beta
	    handleSet $om.spare2<$i> [lindex [concat 0 [profileRadii]] $ipminRemove]
	    handleSet $om.counts<$i> [expr $frac*[powerLawSetFromObject1 $obj1 -iprofMin $ipminFit -beta $beta -useMed]]
	 } else {
	    handleSetFromHandle $om.counts<$i> $obj1.counts_$type
	    handleSetFromHandle $om.r_e<$i> $obj1.r_$type
	    handleSetFromHandle $om.ab<$i> $obj1.ab_$type
	    handleSetFromHandle $om.phi<$i> $obj1.phi_$type
	 }
      }

      set nrow [exprGet $fieldparams.frame<$ifilter>.data->nrow]
      set ncol [exprGet $fieldparams.frame<$ifilter>.data->ncol]
      set model [objectModel_to_region $om $psf -id [exprGet $objc.id] -nrow $nrow -ncol $ncol]

      if {$model == ""} {
	 objectModelDel $om
	 handleDel $objc
	 handleDel $obj1

	 continue
      }

      chainElementAddByPos $objectModels($filter) $om; handleDel $om
      #
      # Save that model in the OBJECT1
      #
      verb_echo 3 [format "Subtracting object %3d (%-7s) at r,c= (%.1f,%.1f) ($filter)" \
		       [exprGet $objc.id] $names [exprGet $obj1.rowc] [exprGet $obj1.colc]]
      if 0 {
	 display $model "Model $types" $filter 1 1
      }
      #
      # and add that model into ${galaxies,stars}Model
      #
      if {$names == "psf" || $names == "PSF"} {
	 set modelReg $starsModel
      } else {
	 set modelReg $galaxiesModel
      }
      regIntLincom $modelReg $model 0 1 1 -intersect

      regDel $model
      handleDel $objc; handleDel $obj1
   }

   verb_echo 3 "Subtracted [chainSize $objectModels($filter)] bright objects"

   handleDel $psf
}

proc set_subtracted_mask {args} {
   global MASK_TYPE

   set opts [list \
		 [list [info level 0] ""] \
		 [list <dataReg> STRING "" dataReg "Region to set mask in"] \
		 [list <modelReg> STRING "" modelReg "Model that's being put back into dataReg"] \
		 [list -NOTCHECKED DOUBLE 25 notcheckedThreshold "Level above which to label pixels NOTCHECKED"] \
		 [list -SUBTRACTED DOUBLE 5 subtractedThreshold "Level above which to label pixels SUBTRACTED"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   #
   # Run object finder to find regions where certain bits are set.
   #
   set levels    [list $subtractedThreshold $notcheckedThreshold];
   set maskNames [list SUBTRACTED NOTCHECKED]
   set npixMin 10
   set ngrow 3

   set levs [vectorExprEval "{$levels}"];	# levels for object finder
   set objlist [regFinder $modelReg $levs]
   vectorExprDel $levs

   loop i 0 [llength $maskNames] {
      objectChainTrim $objlist $i $npixMin
      objectChainGrow $objlist $i $modelReg $ngrow
      
      spanmaskSetFromObjectChain $objlist *$dataReg.mask $MASK_TYPE([lindex $maskNames $i]) $i
   }
   
   objectChainDel $objlist
}

###############################################################################
#
# Return a region given a objectModel
#
proc objectModel_to_region {args} {
   global MASK_TYPE MODEL_TYPES

   set opts [list \
		 [list [info level 0] "Return a region given a objectModel"] \
		 [list <om> STRING "" om "an OBJECT_MODEL"] \
		 [list <psf> STRING "" psf "a DGPSF"] \
		 [list -id INTEGER 0 id "An ID number for this object"] \
		 [list -threshold INTEGER 1 threshold "Threshold for region to return"] \
		 [list -npixel_min INTEGER 100 npixel_min "Minimum size of object to return"] \
		 [list -ngrow INTEGER 0 ngrow "Number of pixels to grow detected object"] \
		 [list -nrow INTEGER 0 nrow "Number of rows in corrected frame"] \
		 [list -ncol INTEGER 0 ncol "Number of columns in corrected frame"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set names {}
   loop i 0 [exprGet $om.ncomp] {
      switch [exprGet $om.type<$i>] \
	  [list \
	       $MODEL_TYPES(DEV_MODEL) { set type "deV" }\
	       $MODEL_TYPES(EXP_MODEL) { set type "exp" }\
	       $MODEL_TYPES(PSF_MODEL) { set type "psf" }\
	       default { error "Unknown type of objectModel: [exprGet $om.type<$i>]" }\
	       ]
      lappend names $type;		# used for diagnostics
	 
      set weight [exprGet $om.weight<$i>]
      set rowc [exprGet $om.rowc<$i>]
      set colc [exprGet $om.colc<$i>]
      set counts [exprGet $om.counts<$i>]
      set re [exprGet $om.r_e<$i>]
      set ab [exprGet $om.ab<$i>]
      set phi [exprGet $om.phi<$i>]
      set spare1 [exprGet $om.spare1<$i>]
      set spare2 [exprGet $om.spare2<$i>]

      if {$type == "deV"} {
	 set comp [makeDevModel $counts $re $ab $phi $psf]
      } elseif {$type == "exp"} {
	 set comp [makeExpModel $counts $re $ab $phi $psf]
      } elseif {$type == "psf"} {
	 set comp [makePowerLawModel $counts $spare1 $spare2 \
		       -nrow $nrow -ncol $ncol -rowc $rowc -colc $colc]
      } else {
	 error "You can't get here"
      }

      set cpeak [hdrGetAsDbl $comp.hdr "CPEAK"]
      set rpeak [hdrGetAsDbl $comp.hdr "RPEAK"]
      handleSet $comp.row0 [expr int($rowc - $rpeak + 0.5)]
      handleSet $comp.col0 [expr int($colc - $cpeak + 0.5)]
      regIntConstAdd $comp -[softBiasGet]

      if {$i == 0} {
	 set model $comp
	 if {$weight != 1.0} {
	    regIntLincom $model "" 0 $weight 0
	 }
      } else {
	 set model [regIntLincom $model $comp 0 1 $weight -union]
	 
	 regDel $comp
      }
   }
   #
   # Run an object detector on that model, and only keep pixels
   # above a threshold
   #
   if [catch {
      set trimmed [regTrimToThreshold $model 1 \
		       -npixel_min $npixel_min -ngrow $ngrow -maskType $MASK_TYPE(OBJECT)]
   }] {
      verb_echo 2 [format "Object %3d (%-7s) at r,c= (%.1f,%.1f) has < %d pixels ($om [format 0x%x [exprGet (int)&$om]]); skipping" \
		       $id $names $rowc $colc $npixel_min]
      #mtv $model
      regDel $model
      return ""
   }

   regDel $model; set model $trimmed
   
   return $model
}

#
# Given a chain of objects, detected in all bands, find the astrometric
# offsets between the various colours
#
proc find_astrom_offsets {mergedlist nfilter fieldparams fieldstat} {
   astromOffsetsFind $mergedlist $nfilter $fieldparams $fieldstat
}

#
# Finish setting the fieldstat, and write it to disk
#
proc write_fieldstat {outputDir field status} {
   global fieldstat filterlist ref_color run rerun camCol

   verb_echo 3 "Writing Framestat"

   handleSet $fieldstat.status $status
   fieldstatSetFromMO $fieldstat;	# set median colour fields

   if {0} {
      print_fieldstat $fieldstat
   } 

   set file [format $outputDir/fpFieldStat-%06d-$camCol-%04d.fit $run $field]

   set hdr [hdrNew]
   hdrInsWithInt $hdr RUN $run "Imaging Run Number"
   hdrInsWithInt $hdr RERUN $rerun "Processing Rerun Number"
   hdrInsWithInt $hdr CAMCOL $camCol "Column in imaging camera"
   hdrInsWithInt $hdr FIELD $field "the field reduced"
   hdrInsWithAscii $hdr FILTERS $filterlist "Filters processed"
   hdrInsWithAscii $hdr REFCOLOR $ref_color "Reference Filter"
   insert_id_values -frames $hdr

   set ch [chainNew FIELDSTAT]
   chainElementAddByPos $ch $fieldstat
   schema2Fits $ch $file -pdu_hdr $hdr

   chainDel $ch;			# don't delete the fieldstat
   hdrFreeAll $hdr; hdrDel $hdr;

   append_types_to_file $file FIELD_FLAGS

   crc2Fits $file "a"

   return 1
}

#
# Read a FIELDSTAT. If the first argument is "-hdr", simply read the file's
# header, but always ensure that the type _FIELDSTAT is defined when we
# return
#
# Note that we cannot call it FIELDSTAT due to lack of case sensitivity in
# struct element names in fits2Schema
#
proc read_fieldstat {args} {
   if {[lindex $args 0] == "-hdr"} {
      set read_hdr 1
      set args [lreplace $args 0 0]
   }

   if [regexp -- {^-h(elp)?} [lindex $args 0]] {
      error {read_fieldstat [-hdr] <dir> <run> <camCol> <field> or read_fieldstat [-hdr] table <field>}
   }

   set i -1
   set dir [lindex $args [incr i]]
   set run [lindex $args [incr i]]
   set camCol [lindex $args [incr i]]
   set field [lindex $args [incr i]]

   if {$dir == "table"} {
      global openit
      if ![info exists openit] {
	 error \
	     "Please use set_run before specifying \"table\" to read_fieldstat"
      }
      set dir $openit(objdir)
      set field $run
      set run $openit(run)
      set camCol $openit(camCol)

      if {$field == ""} {
	 global table
	 set field [keylget table field]
      }
   }

   regexp {^0*(.+)$} $run foo run;	# make sure it isn't treated as octal
   set file [format $dir/fpFieldStat-%06d-$camCol-%04d.fit $run $field]
   
   if {![file exists $file]} {
      error "Cannot open FIELDSTAT file $file"
   }

   if [info exists read_hdr] {
      set hdr [hdrReadAsFits [hdrNew] $file]

      if ![catch {schemaGetFromType _FIELDSTAT}] {# we've seen the type
	 return $hdr
      }
   }


   set ch [fits2Schema $file _FIELDSTAT]
   set fieldstat [chainElementRemByPos $ch 0]
   chainDel $ch

   if [info exists read_hdr] {		# we now have a type _FIELDSTAT
      genericDel $fieldstat; return $hdr
   }

   return $fieldstat
}

proc loop_reset {startMem _biasvectors _corrimages _mergedlist kos calibs fieldstat \
		     {_brightStarWings ""} {_objectModels ""}} {
   upvar $_biasvectors biasvectors $_corrimages corrimages
   upvar $_mergedlist mergedlist
   if {$_brightStarWings != ""} {
      upvar $_brightStarWings brightStarWings
   }
   if {$_objectModels != ""} {
      upvar $_objectModels objectModels
   }
   
   global filterlist read_psBB_files read_fpC_files coadds

   strategicMemoryReserveSet 0

   fini_coadds coadds
   calib1byframeDel $calibs

   foreach f $filterlist {
      if [info exists corrimages($f)] {
	 set mask [handleBindFromHandle [handleNew] *$corrimages($f).mask]
	 spanmaskDel $mask
	 handleSet $corrimages($f).mask 0x0
	 regDel $corrimages($f)
	 unset corrimages($f)
      }
      if {!$read_fpC_files && $read_psBB_files && [info exists biasvectors($f)]} {
	 regDel $biasvectors($f)
	 unset biasvectors($f)
      }

      if [info exists brightStarWings($f)] {
	 chainDestroy $brightStarWings($f) wingobjectDel
	 unset brightStarWings($f)
      }
      
      if [info exists objectModels($f)] {
	 chainDestroy $objectModels($f) objectModelDel
	 unset objectModels($f)
      }
   }
   overlayReleaseAll;			# scratch buffers were overlays

   if [info exists mergedlist] {
      objcChainDel $mergedlist; unset mergedlist
   }
   chainDestroy $kos knownobjDel

   # reset fieldstat structure's fields to zero (since we use the
   #   same structure over and over for all frames)
   fieldstatClear $fieldstat
   #
   # Free any allocated convolution LUTs
   #
   finiConvolveLUT
   #
   # Free any allocated PEAKs
   #
   finiObjectFinder
   #
   # Free any Meschach objects (MATs, VECs)
   #
   meschachFini
   #
   # Free any schema definitions
   #
   fitsBinForgetSchemaTrans NULL
   
   # check for memory corruption, if available in dervish
   if {[info commands memCheck] != ""} {
      assert {[memCheck] == ""}
   }

   #
   # unset the CRCs that only apply to a particular field
   #
   crc_reset_field

   uplevel {write_mem mem "freeing memory"}
   verb_echo 2 "Freeing memory blocks"

   set nleak 30;			# show this many leaked pointers
   set leakage [memBlocksGet $startMem [memSerialNumber]]
   if {[llength $leakage] > $nleak} {
      set leakage [lreplace $leakage $nleak end "..."]
   }
   assert {$leakage == {}}
   
   defragment_memory
}

#
# Clean up after trapping an error
#
proc error_reset {startMem} {
   set memSerialNumber [memSerialNumber]
   set nextSerialNumber [memNextSerialNumber]
   overlayReleaseAll

   memFreeHandles $startMem $memSerialNumber 1
   memFreeBlocks $startMem $memSerialNumber

   foreach v [list binned corrimages mergedlist] {
      uplevel [list catch [list unset $v]]
   }

   return $nextSerialNumber
}

#
# Convert a field number that may be an index into fieldList to a real field
#
proc getField {camCol field} {
   upvar fieldList fieldList
   if {[info exists fieldList] && $fieldList != "default"} {
      set tfield [lindex $fieldList $field]
      if {$field == 0 && $tfield == ""} {
	 error "No frames are specified in fieldList" 
      }
      return $tfield
   }
   
   upvar fieldList$camCol fieldList$camCol
   if {[info exists fieldList$camCol] && [set fieldList$camCol] != "default"} {
      set tfield [lindex [set fieldList$camCol] $field]
      if {$field == 0 && $tfield == ""} {
	 error "No frames are specified for camCol $camCol in fieldList$camCol"
      }
      return $tfield
   }

   return $field
}

proc memFreeHandles { n1 n2 {deep 0}} {
    if {[string match $n2 "0"]} {
      set n2 [memSerialNumber]
   }
   set mem [memBlocksGet $n1 $n2]

   #
   # Delete objects with memid's in [n1,n2] that have handles (so we
   # can call their destructors). Because masks and regions with children
   # cannot be deleted, we repeatedly try to destroy all handles until
   # we make no progress (so the children can be destroyed, and then
   # we get another chance to delete their parents)
   #
   while 1 {
      set handle_list [handleList]
      if {[llength $handle_list] == 0} {
	 break;
      }
      
      set ndel 0;			# how many handles have we deleted?

      foreach h $handle_list {
	 if {![catch {keylget mem [lindex $h 2]} id_val]} {
	    set handle [lindex $h 0]
	    if $deep {
		if {[catch {handleDelFromType $handle} err_msg]} {
		    if {![regexp {Schema for .* doesn't specify a destructor}\
			      $err_msg]} {
			echo $err_msg
		    }
		    set type [string tolower [lindex $h 1]]
		    
		    if {$type == "binregion"} {
			if {[exprGet (int)$handle.reg] != 0} {
			    if {[exprGet (int)$handle.reg->prvt] == 0} {
				# already deleted via some other path
				handleSet $handle.reg 0x0
			    }
			}
		    } elseif {$type == "mask"} {
			;
		    } elseif {$type == "region"} {
			if {[exprGet (int)$handle.prvt] == 0} {
			    continue;	# already deleted via some other path
			}
			
			
			if {[exprGet $handle.mask] != 0x0 && 
			    [exprGet ((SPANMASK*)$handle.mask)->cookie] == 0xc00cee} {
			    handleSet $handle.mask 0x0
			}
			
			set type reg
		    } elseif {$type == "phfitsfile"} {
			fitsBinTblClose $handle
			continue;
		    } else {
			# free handle but _not_ its contents
			handleBind $handle 0x0 UNKNOWN
			handleDel $handle
			continue;
		    }
		    
		    if {[catch { eval ${type}Del $handle } msg]} {
			echo $msg
		    } else {
			incr ndel
		    }
		}
	    } else {
		# free handle but _not_ its contents
		handleBind $handle 0x0 UNKNOWN
		handleDel $handle
		incr ndel
	    }
	 }
      }
      if {$ndel == 0} {			# we made no progress
	 foreach h [handleList] {
	    if {![catch {keylget mem [lindex $h 2]} id_val]} {
		set h [lindex $h 0]
		handleBind $h 0x0 UNKNOWN;# DON'T free handle's contents
		handleDel $h
	    }
	 }
	 break;
      }
   }
}

#
# Manage overlays of regions. To add a region to the overlay list, add
# it to the array $overlays, as a two-element list
#	set overlays(0) [list "" $region]
# It's easier to do this as
#	overlayAdd $region
# To delete all overlays, say
#	overlayDelAll
#
# To actually use them, you can bind a region to an overlay with
#	overlayGet name nrow ncol
# and free it again with
#	overlayRelease name
# you can release all overlays with
#	overlayReleaseAll
#
# An overlay (once acquired with overlayGet) may be split into smaller
# regions with overlaySplit -- e.g.
#	overlayGet name 100 100
#	set regs [overlaySplit name 2 2]
# after which $regs is a list of 4 50x50 regions; they are all released
# when the parent overlay is released (i.e. with overlayRelease name)
#
# There are overlayN overlays; overlayNFree are available. All current
# overlays may be listed with overlayList
#
proc overlayN {} {
   global overlays

   if {![info exists overlays]} {
      return 0;
   }
   return [array size overlays]
}

proc overlayNFree {} {
   global overlays

   if {![info exists overlays]} {
      return 0;
   }

   set n [array size overlays]
   set nfree $n
   loop i 0 $n {
      if {[lindex $overlays($i) 0] != ""} {
	 incr nfree -1
      }
   }
   return $nfree
}

proc overlayAdd {reg} {
   global overlays
   if {![info exists overlays]} {
      set i 0
   } else {
      set i [array size overlays]
   }
   set overlays($i) [list "" $reg]
}


proc overlayDelAll {} {
   global overlays

   if {[info exists overlays]} {
      loop i 0 [array size overlays] {
	 if {[lindex $overlays($i) 0] != ""} {
	    error "overlay $i is bound to [lindex $overlays($i) 0]"
	 }
	 regDel [lindex $overlays($i) 1]
      }
      unset overlays
   }
}

proc overlayList {} {
   global overlays

   if {![info exists overlays]} {
      echo "No overlays are allocated"
   }
   loop i 0 [array size overlays] {
      set reg [lindex $overlays($i) 1]
      set size [format "%dx%d" [exprGet $reg.nrow] [exprGet $reg.ncol]]
      set oreg [lindex $overlays($i) 2]
      if {$oreg == ""} {
	 set osize ""
      } else {
	 set osize [format "%dx%d" [exprGet $oreg.nrow] [exprGet $oreg.ncol]]
      }
      echo [format "%-7s %-15s (%s) %s" $size \
		[lindex $overlays($i) 0] $osize "[lindex $overlays($i) 3]"]
   }
}

proc overlayGet {_name nrow ncol} {
   upvar $_name name
   global overlays

   if {![info exists overlays]} {
      error "No overlays are allocated"
   }
   loop i 0 [array size overlays] {
      if {[lindex $overlays($i) 0] != ""} {  # overlay is already in use
	 continue;
      }
      set reg [lindex $overlays($i) 1];
      if {[exprGet $reg.nrow] >= $nrow && [exprGet $reg.ncol] >= $ncol} {
	 set o [subRegNew $reg $nrow $ncol 0 0 -mask]

	 set overlays($i) [list $_name $reg $o]
	 set name $o
	 return $name;
      }
   }
   error "Cannot bind $_name; no free overlays, or none large enough"
}

proc overlaySplit {name nr nc} {
   global overlays

   if {![info exists overlays]} {
      error "No overlays are allocated"
   }
   loop i 0 [array size overlays] {
      if {$name == [lindex $overlays($i) 0]} {# got it!
	 set oreg [lindex $overlays($i) 2] 
	 if {[lindex $overlays($i) 3] != ""} {
	    error "Overlay $name is already split"
	 }

	 set rs [expr [exprGet $oreg.nrow]/$nr]
	 set cs [expr [exprGet $oreg.ncol]/$nc]
	 loop r 0 $nr {
	    loop c 0 $nc {
	       lappend regs \
		   [subRegNew $oreg $rs $cs [expr $r*$rs] [expr $c*$cs]]
	    }
	 }
	 set overlays($i) "$overlays($i) [list $regs]"
	 return $regs
      }
   }
   error "$name isn't bound to an overlay"
}

proc overlayRelease {name} {
   global overlays

   if {![info exists overlays]} {
      error "No overlays are allocated"
   }
   loop i 0 [array size overlays] {
      if {$name == [lindex $overlays($i) 0]} {
	 foreach sreg [lindex $overlays($i) 3] {
	    regDel $sreg
	 }
	 regDel [lindex $overlays($i) 2] 
	 set overlays($i) [list "" [lindex $overlays($i) 1]]
	 return $name;
      }
   }
   error "$name isn't bound to an overlay"
}

proc overlayReleaseAll {} {
   global overlays

   if {![info exists overlays]} {
      error "No overlays are allocated"
   }
   loop i 0 [array size overlays] {
      if {[lindex $overlays($i) 0] != ""} {
	 foreach sreg [lindex $overlays($i) 3] {
	    regDel $sreg
	 }
	 regDel [lindex $overlays($i) 2]
	 set overlays($i) [list "" [lindex $overlays($i) 1]]
      }
   }
}

#
ftclHelpDefine photo schemaTransNewAll \
    "Add all elements in a struct to a SCHEMATRANS, and return it. Any \
elements listed in the optional parameter \$except are _not_ added. E.g. \
      schemaTransNewAll OBJECT1 {type next prev}"

proc schemaTransNewAll {type {except {}} {_fields {}}} {
   if {$_fields != ""} {
      upvar $_fields fields
   }
   set trans [schemaTransNew]

   foreach el [schemaGetFromType $type] {
      set el [lindex $el 0]
      if {[lsearch $except $el] == -1} {
	 fitsBinSchemaTransEntryAdd $trans $el $el
	 lappend fields $el
      }
   }

   return $trans
}


#
ftclHelpDefine photo schemaTransNewAdd \
    "Add elements given in list to a SCHEMATRANS, and return it. \
     Note that only those elements which _really_ exist will be added."

proc schemaTransNewAdd {type list {_fields {}}} {
   if {$_fields != ""} {
      upvar $_fields fields
   }

   set trans [schemaTransNew]

   foreach el [schemaGetFromType $type] {
      set el [lindex $el 0]
      if {[lsearch $list $el] != -1} {
	 fitsBinSchemaTransEntryAdd $trans $el $el
	 lappend fields $el
      }
   }

   return $trans
}


#
###############################################################################
#
ftclHelpDefine photo declare_schematrans \
    "Declare various types to photo's version of fits binary tables"

proc declare_schematrans {ncolor} {
   global ROWS
#
# OBJMASK
#
   if ![fitsBinSchemaTransIsDefined "OBJMASK"] {
      set trans [schemaTransNewAll OBJMASK {id size s data ndata sum user cached_span_index cached_data_index}]
      fitsBinSchemaTransEntryAdd $trans s s -fitsData heap
      
      fitsBinDeclareSchemaTrans $trans OBJMASK
      handleDel $trans
   }
#
# OBJC_IO
#
   if ![fitsBinSchemaTransIsDefined "OBJC_IO"] {
      set trans [schemaTransNewAll OBJC_IO {ncolor aimage test dgpsf}]
      
      fitsBinDeclareSchemaTrans $trans OBJC_IO
      handleDel $trans
   }
#
# ATLAS_IMAGE. Note that ncolor is treated specially in the binary table code.
#
   if ![fitsBinSchemaTransIsDefined "ATLAS_IMAGE"] {
      set trans [schemaTransNew]
      foreach el "id run rerun camCol field parent" {
	 fitsBinSchemaTransEntryAdd $trans $el $el
      }
      fitsBinSchemaTransEntryAdd $trans ncolor ncolor -fitsData heap \
	  -heapType uchar
      
      fitsBinDeclareSchemaTrans $trans ATLAS_IMAGE
      handleDel $trans
   }
#
# TEST_INFO
#
   if ![fitsBinSchemaTransIsDefined "TEST_INFO"] {
      set trans [schemaTransNewAll TEST_INFO {}]
      
      fitsBinDeclareSchemaTrans $trans TEST_INFO
      handleDel $trans
   }
#
# OBJC
#
   if ![fitsBinSchemaTransIsDefined "OBJC"] {
      set trans [schemaTransNewAll OBJC {color peaks}]
      fitsBinSchemaTransEntryAdd $trans color color -dimen $ncolor
      
      fitsBinDeclareSchemaTrans $trans OBJC
      handleDel $trans
   }
#
# OBJECT1
#
   if ![fitsBinSchemaTransIsDefined "OBJECT1"] {
      set trans [schemaTransNewAll OBJECT1 \
		     {region mask dgpsf aratio majaxis flags comp \
			  peaks npeak peak_size npix satur_DN }]
      
      fitsBinDeclareSchemaTrans $trans OBJECT1
      handleDel $trans
   }
#
# MASK
#
   if ![fitsBinSchemaTransIsDefined "MASK"] {
      set trans [schemaTransNew]
      fitsBinSchemaTransEntryAdd $trans MNAME name -fitsData heap
      fitsBinSchemaTransEntryAdd $trans MNROW nrow
      fitsBinSchemaTransEntryAdd $trans MNCOL ncol
      fitsBinSchemaTransEntryAdd $trans MROWS rows -fitsData heap
      fitsBinSchemaTransEntryAdd $trans MROW0 row0
      fitsBinSchemaTransEntryAdd $trans MCOL0 col0
      
      fitsBinDeclareSchemaTrans $trans MASK
      handleDel $trans
   }
#
# REGION
#
   if ![fitsBinSchemaTransIsDefined "REGION"] {
      set trans [schemaTransNew]
#   fitsBinSchemaTransEntryAdd $trans RNAME name -fitsData heap
      fitsBinSchemaTransEntryAdd $trans RNROW nrow
      fitsBinSchemaTransEntryAdd $trans RNCOL ncol
      fitsBinSchemaTransEntryAdd $trans RTYPE type
      fitsBinSchemaTransEntryAdd $trans RROWS rows_fl32 -fitsData heap
# don't write mask as it's a SPANMASK
#   fitsBinSchemaTransEntryAdd $trans RMASK mask
      fitsBinSchemaTransEntryAdd $trans RROW0 row0
      fitsBinSchemaTransEntryAdd $trans RCOL0 col0

      fitsBinDeclareSchemaTrans $trans REGION
      handleDel $trans
   }
#
# CELL_PROF
#
   if ![fitsBinSchemaTransIsDefined "CELL_PROF"] {
      set trans [schemaTransNewAll CELL_PROF {}]
      fitsBinDeclareSchemaTrans $trans CELL_PROF
      handleDel $trans
   }
#
# PSF_COEFFS
#
   if ![fitsBinSchemaTransIsDefined "PSF_COEFFS"] {
      set trans [schemaTransNewAll PSF_COEFFS {}]
      fitsBinDeclareSchemaTrans $trans PSF_COEFFS
      handleDel $trans
   }
#
# DGPSF
#
   if ![fitsBinSchemaTransIsDefined "DGPSF"] {
      set trans [schemaTransNewAll DGPSF {type residuals}]
      fitsBinDeclareSchemaTrans $trans DGPSF
      handleDel $trans
   }
#
# FIELDSTAT
#
   if ![fitsBinSchemaTransIsDefined "FIELDSTAT"] {
      set trans [schemaTransNewAll FIELDSTAT {ncolor}]
      fitsBinDeclareSchemaTrans $trans FIELDSTAT
      handleDel $trans
   }
#
# CALIB_IO
#
   if ![fitsBinSchemaTransIsDefined "CALIB_IO"] {
      set trans [schemaTransNewAll CALIB_IO {ncolor node incl mjd}]
      fitsBinDeclareSchemaTrans $trans CALIB_IO
      handleDel $trans
   }
#
# PSF_KL_COMP
#
   if ![fitsBinSchemaTransIsDefined "PSF_KL_COMP"] {
      set trans [schemaTransNewAll PSF_KL_COMP {}]
      fitsBinDeclareSchemaTrans $trans PSF_KL_COMP
      handleDel $trans
   }
}

#################
# given a corrected-image region header, create a FITS HDR with all the
# info needed for target selection. This HDR is written as part of the
# primary header in the FITS binary table with all the PHOTO output.
#
proc make_table_header {seed} {
   global run rerun field camCol filterlist fieldparams ccdrow
#
# convert field numbers to frame numbers
#
   set hdr [hdrNew]
   
   insert_id_values -frames $hdr

   hdrInsWithAscii $hdr "CREATIME" [ctime -utc] \
       "The local time when this file was created"
   hdrInsWithInt $hdr "SEED" $seed "Seed for photo random number generator"
   
   hdrInsWithInt $hdr "REF_BAND" [exprGet $fieldparams.ref_band_index] \
       "index of band used for canonical centre"
   hdrInsWithInt $hdr "RUN" $run "Imaging run number"
   hdrInsWithInt $hdr "RERUN" $rerun "Processing rerun number"
   hdrInsWithInt $hdr "CAMCOL" $camCol "Column in the imaging camera"
   hdrInsWithInt $hdr "FIELD" $field "Field sequence number"
   hdrInsWithAscii $hdr "FILTERS" $filterlist "Filter order in arrays"
   
   return $hdr
}

proc print_fieldstat {fieldstat} {
   puts [structPrint $fieldstat]
   loop i 0 [expr [exprGet $fieldstat.ncolor]] {
      set fs [handleBindFromHandle [handleNew] *$fieldstat.filter<$i>]
      puts [structPrint $fs]	
      handleDel $fs
   }
}

###############################################################################
#
# Do we want to skip processing this field, and just write a minimal
# set of output files, as if we'd timed out?

proc skip_field {field} {
   global skipFieldList

   if {[lsearch $skipFieldList $field] < 0} {
      return 0
   } else {
      return 1
   }
}
