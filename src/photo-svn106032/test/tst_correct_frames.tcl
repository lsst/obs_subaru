#
# Non-comprehensive tests for the Correct Frames module
#
source [envscan \$PHOTO_DIR]/etc/photoStartup.tcl
# We do NOT want to use a random seed! These tests should be deterministic.
# Note that they are not quite; the gaussian random number generator keeps a
# single value cached so it is possible for these tests to have two outcomes
# if run more than once
set seed 843080201
#set seed [getclock]; echo Seed is $seed
set rand [phRandomNew 200000 -seed $seed -type 2]
#
global verbose; set verbose 1
global diagnostics; set diagnostics -1
global logfile; set logfile 0
global fieldstat; 
global fieldparams; 
global softpars ccdpars defpars
#
# Usually from plan file:
#
set idB_format		{idB-%06d-%s%d.fit  $run $filter $camCol}
set psFF_format		{psFF-%06d-%s%d.fit $run $filter $camCol}

#
# Parameters to determine which `field' we are dealing with
#
param2Chain [envscan \$PHOTO_DIR]/test/tst_params softpars
param2Chain [envscan \$PHOTO_DIR]/etc/fpParam.par defpars
set read_fpC_files 0
set compress_image_files 0

global camCol run runstr outputdev field filter filterlist psfilterlist
set outputdev "tmp"; catch {mkdir $outputdev}
set camCol 1
set run 0
set runstr [format %06d $run]
set field 0
set nfilter 1
set id_values(PS_ID) "tst"
set read_psBB_files 0
#
# Scaling factor for quartile regions; usually read from the tblcol
#
global tscal; set tscal 256
#
# Utilities
#
# Set the bias regions of a frame to $bias1 and $bias2
#
proc set_bias {reg nPreBias0 sPreBias0 nPostBias0 sPostBias0  bias0 \
		   nPrescan0 sPrescan0 nPostscan0 sPostscan0  bias0 \
		   nPreBias1 sPreBias1 nPostBias1 sPostBias1  bias1 \
		   nPrescan1 sPrescan1 nPostscan1 sPostscan1  bias1 } {

   loop i 0 2 {
      foreach t "PreBias PostBias Prescan Postscan" {
	 if {[set n${t}$i] > 0} {
	    set tmp [subRegNew $reg \
			 [exprGet $reg.nrow] [set n${t}$i] 0 [set s${t}$i]]
	    regIntSetVal $tmp [set bias$i]
	    regDel $tmp
	 }
      }
   }
}
#
# Set a frame to a constant value, adding in the left and right biases
#
proc set_value {reg val ccdpars bias0  bias1} {
   set ncol [exprGet $reg.ncol]
   set nrow [exprGet $reg.nrow]
   if { [exprGet $ccdpars.namps] == 1 } {
      set splitcol [expr [exprGet $ccdpars.sData0] + \
			[exprGet $ccdpars.nData0]/2.0]
      set bias1 $bias0
   } else {
      set splitcol [exprGet $ccdpars.sData1]
   }

   loop i 0 2 {
      foreach t "PreBias PostBias Prescan Postscan" {
	 foreach w "n s" {
	    set ${w}${t}$i [exprGet $ccdpars.${w}${t}$i]
	 }
      }
   }

   set tmp [subRegNew $reg $nrow [expr $splitcol+1] 0 0]
   regIntSetVal $tmp [expr $val+$bias0]
   regDel $tmp

   set tmp [subRegNew $reg $nrow [expr $ncol-($splitcol+1)] 0 $splitcol]
   regIntSetVal $tmp [expr $val+$bias1]
   regDel $tmp

   set_bias $reg \
       $nPreBias0 $sPreBias0 $nPostBias0 $sPostBias0  $bias0 \
       $nPrescan0 $sPrescan0 $nPostscan0 $sPostscan0  $bias0 \
       $nPreBias1 $sPreBias1 $nPostBias1 $sPostBias1  $bias1 \
       $nPrescan1 $sPrescan1 $nPostscan1 $sPostscan1  $bias1
   
   # Now for bad columns.

   set defects [handleBindFromHandle [handleNew] *$ccdpars.CCDDefect]
   loop c 0 [chainSize $defects] {
      set defect [chainElementGetByPos $defects $c]
      set bc  [expr [exprGet $defect.dfcol0]+$nPrescan0]
      set type [exprGet $defect.dfaction]
      if {$type == "BADCOL"} {
	 # only put in bad cols for now
	 set tmp [subRegNew $reg $nrow 1 0 $bc]
	 regIntSetVal $tmp 10000
	 regDel $tmp
      }
      handleDel $defect
   }
   handleDel $defects
}

#
# Run the C code from the PS-pipeline to handle 2-D biases and quartiles,
# generating 1-D biases and flats.
#
proc make_flats {i_sky i_bias i_ldrift i_rdrift} {
   global camCol runstr outputdev field filter filterlist tscal
   global softpars hardpars ccdpars ccdrow
   global id_values

   set id_values(ASTRO_ID) "unknown"
   
   set ffninterp [keylget softpars ffninterp]
   set ffsigrej [keylget softpars ffsigrej]
   set ffmode [keylget softpars ffmode]
   set bias_scale [keylget softpars bias_scale]
   #
   # Fake the 2d bias and get the ccdpars structure for current filter
   #
   set bias [regNew -type U16 1 2128]
   regIntSetVal $bias [expr $i_bias*$bias_scale]

   fetch_ccdpars [envscan \$PHOTO_DIR]/test tst_opConfig.par \
       [envscan \$PHOTO_DIR]/test/tst_opECalib.par \
       [envscan \$PHOTO_DIR]/test/tst_opBC.par ccdpars $filter $camCol $softpars

   # set the global "ccdrow" array, which tells which row each filter is in
   set ccdrow($filter) [keylget softpars ccdrow_$filter]


#
# Create the flatfield, with the bias level 1 (unscaled) DN different from
# the bias frame above
#
   set quartiles_nc [exprGet $ccdpars($filter).ncols]
   set quartiles [quartilesNew 1 $quartiles_nc]
   handleSet $quartiles.tscal $tscal
   set perc [list 0.5 1 1.5]
   loop q 0 3 {
      set tmp [handleBindFromHandle [handleNew] *$quartiles.data<$q>]
      set_value $tmp [expr int($tscal*$i_sky*[lindex $perc $q])] \
	  $ccdpars($filter) \
	  [expr ($i_bias+$i_ldrift)*$tscal] \
	  [expr ($i_bias+$i_rdrift)*$tscal]
      handleDel $tmp
   }

   set flatlist [flatfieldsCalc $quartiles $bias $bias_scale 0 0 $ccdpars($filter)]
   set biases($filter) $bias
 
   set 1dbiasdrift [keylget flatlist biasdrift1d]
   set O1dbiasdrift [keylget flatlist Obiasdrift1d]
   set E1dbiasdrift [keylget flatlist Ebiasdrift1d]
   set flatregion [keylget flatlist flat2d]
   set skylevels [keylget flatlist skylevel1d]

   upvar calib1byframe calib1byframe
   set calib1byframe [calib1byframeNew $filterlist]
   set psf [dgpsfNew -sigmax1 1 -b 0]
   handleSet $psf.sigma1_2G 1.0
   handleSet $psf.sigma2_2G 3.0
   handleSet $psf.b_2G 0
   
   handleSet $calib1byframe.field $field
   handleSetFromHandle $calib1byframe.calib<0>->psf &$psf
   handleSet $calib1byframe.calib<0>->filter<0> $filter
   set calib [handleBindFromHandle [handleNew] *$calib1byframe.calib<0>]

   set trans [transNew]
   handleSetFromHandle $calib.toGCC &$trans
   handleDel $trans
   
   # interpolate flatfield
   set flatfield [flatfieldsInterp $flatregion]
   # interpolate sky and bias
   skybiasInterp $skylevels $1dbiasdrift $O1dbiasdrift $E1dbiasdrift 0 $ffninterp $ffsigrej $ffmode \
                       $ccdpars($filter) $calib 

   set sky [exprGet $calib.sky]
   set lbias [exprGet $calib.lbias]
   set rbias [exprGet $calib.rbias]

   set fieldstr [format %04d $field]
   hdrInsWithInt $biases($filter).hdr "TSHIFT" $bias_scale "bias scale"
   hdrInsWithAscii $biases($filter).hdr "BIAS_ID" "unknown"
   hdrInsWithAscii $flatfield.hdr "PS_ID" "unknown"
   regWriteAsFits $biases($filter) \
       $outputdev/idB-$runstr-$filter$camCol.fit
   regWriteAsFits $flatfield \
       $outputdev/psFF-$runstr-$filter$camCol.fit
   
   set bias [regPixGet $biases($filter) 0 0]

   regDel $skylevels
   regDel $biases($filter)
   regDel $flatfield
   regDel $flatregion
   regDel $1dbiasdrift
   regDel $O1dbiasdrift
   regDel $E1dbiasdrift
   handleDel $calib
   
   quartilesDel $quartiles
#
# Did we pass?
#
   set ret 0

   if {$sky != $i_sky} {
      echo "TEST-ERR: Sky estimate is wrong ($sky not $i_sky)"
      set ret 1
   }

   set delta [expr int($i_bias*$bias_scale)-$bias]
   if {$delta < -1 || $delta > 1} {
      echo "TEST-ERR: Bias estimate is wrong ($bias not $i_bias)"
      set ret 1
   }

   set delta [expr int($i_ldrift*$bias_scale)-$lbias]
   if {$delta < -1 || $delta > 1} {
      echo TEST-ERR: Left bias drift is wrong \
	  ([expr $lbias.0/$bias_scale] not $i_ldrift)
      set ret 1
   }
      
   if {[exprGet $ccdpars($filter).namps] == 2} {
      set delta [expr int($i_rdrift*$bias_scale)-$rbias]
      if {$delta < -1 || $delta > 1} {
	 echo TEST-ERR: Right bias drift is wrong \
	     ([expr $rbias.0/$bias_scale] not $i_rdrift)
	 set ret 1
      }
   }
   return $ret
}

#
# Create a frame, and run it through correct frames (with flats and biases
# as produced by make_flats). Then check that correcting postage stamps
# works, and that the -regout flag isn't needed
#
proc tst_correct_frame {i_sky i_bias} {
   global filter run runstr camCol
   global outputdev fieldparams
   global softpars hardpars ccdpars 
   global ccdrow display

   upvar calib1byframe calibs
      set bias_scale [keylget softpars bias_scale]

   set rawfile $outputdev/R-$run-$camCol-$filter-0.fit
   set raw [regNew]

   echo Creating raw
   set lbias [expr $i_bias+[exprGet $calibs.calib<0>->lbias]/$bias_scale]
   set rbias [expr $i_bias+[exprGet $calibs.calib<0>->rbias]/$bias_scale]
   
   regDel $raw
   set raw [regNew 1489 2128]
   set_value $raw $i_sky $ccdpars($filter) $lbias $rbias
   set hdr [hdrNew]
   hdrInsWithLogical $hdr DUMMY T
   handleSetFromHandle $raw.hdr $hdr
   handleDel $hdr
   regWriteAsFits $raw $rawfile

   set corrows [exprGet $raw.nrow]
   if {[exprGet $ccdpars($filter).namps] == 1} {
      set corcols [exprGet $ccdpars($filter).nData0] 
   } else {
      set corcols [expr [exprGet $ccdpars($filter).nData0] + \
		       [exprGet $ccdpars($filter).nData1] ]
   }
   set rawncols [exprGet $raw.ncol]
   set big [regNew $corrows [expr $corcols+40]]
   set corrimage [subRegNew $big $corrows $corcols 0 20]
   set mask [spanmaskNew]

   handleSet (long)$corrimage.mask [exprGet &$mask]; handleDel $mask
   handleSet $corrimage.col0 0

   init_correct_frames $outputdev biasvector $outputdev flatvector \
       $rawncols $corrows $corcols \
       cr_keep cr_min_sigma cr_min_e cr_cond3 cr_cond32 fullWell

   if {[info exists display] && $display} {
      display $raw "raw frame"
   }
   
   correct_frames $filter $raw $corrimage $biasvector($filter) \
       $flatvector($filter) $calibs skyTypical skySigma "" \
       $outputdev \
       $cr_keep $cr_min_sigma $cr_min_e $cr_cond3 $cr_cond32

   if {[info exists display] && $display} {
      display $corrimage "corrected frame"
   }

   set soft_bias [softBiasGet]

   # Since tst_opConfig has the pre and post scan regions adjacent, here
   # we treat them as one.
   set nPrescan0 [exprGet $ccdpars($filter).nPrescan0]

   if { [exprGet $ccdpars($filter).namps] == 1 } {
      set splitcol [expr [exprGet $ccdpars($filter).sData0] + \
			[exprGet $ccdpars($filter).nData0]/2]
   } else {
      set splitcol [exprGet $ccdpars($filter).sData1]
   }

#
# See if we passed
#
   set ret 0

   foreach x [list 0 $splitcol [expr $splitcol+1] \
		  [expr [exprGet $corrimage.ncol]-1]] {
      set pix [regPixGet $corrimage 0 $x]
      if {$pix != $i_sky+$soft_bias} {
	 echo "TEST-ERR: "\
	     "Flat fielding changed rows<0><$x> from $i_sky+$soft_bias to $pix"
	 set ret 1
      }
   }
   set defects [handleBindFromHandle [handleNew] *$ccdpars($filter).CCDDefect]
   loop c 0 [chainSize $defects] {
      set defect [chainElementGetByPos $defects $c]
      set bc  [expr [exprGet $defect.dfcol0]+$nPrescan0]
      set type [exprGet $defect.dfaction]
      if {$type == "BADCOL"} {
	 set pix [regPixGet $corrimage 0 $bc]
	 set ipix [expr {([regPixGet $corrimage 0 [expr $bc-1]] +
			  [regPixGet $corrimage 0 [expr $bc+1]])/2}]
	 if {$pix != $ipix} {
	    set ret 1
	    echo "TEST-ERR: Column $bc is not fixed; saw $pix not $ipix"
	 }
      }
      handleDel $defect
   }
   handleDel $defects

   spanmaskDel *$corrimage.mask
   handleSet (long)$corrimage.mask 0
   regDel $corrimage
   #
   # Now see if we can flatten postage stamps
   #
   global filterlist
   set ifilter [lsearch $filterlist $filter]
   set corrimage [regNew -name corrected 29 29]
   set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<0>]
   set mask [spanmaskNew]
   handleSet (long)$corrimage.mask [exprGet &$mask]; handleDel $mask
   
   foreach x [list 100 [expr $splitcol-10] [expr $splitcol+10]] {
      set ps [subRegNew $raw 29 29 10 $x]
      handleSet $corrimage.row0 10
      handleSet $corrimage.col0 $x

      handleSet $fieldparams.frame<$ifilter>.fullWell<0> $fullWell(0,$filter)
      handleSet $fieldparams.frame<$ifilter>.fullWell<1> $fullWell(1,$filter)
      correctFrames $ps $biasvector($filter) $flatvector($filter) \
	  $ccdpars($filter) $fparams $bias_scale \
	  -regout $corrimage -stamp \
	  -leftbias [exprGet $calibs.calib<$ifilter>->lbias] \
	  -rightbias [exprGet $calibs.calib<$ifilter>->rbias]

      set pix [regPixGet $corrimage 10 10]
      if {$pix != $i_sky+$soft_bias} {
	 echo "TEST-ERR: Flat fielding changed pstamp" \
	   "at $x from $i_sky to [expr $pix-$soft_bias]"
	 set ret 1
      }
      
      regDel $ps
   }
   #
   # Now see if it works if we don't specify the output region
   #
   set ps [subRegNew $raw 29 29 10 100]
   handleSet $fieldparams.frame<$ifilter>.fullWell<0> $fullWell(0,$filter)
   handleSet $fieldparams.frame<$ifilter>.fullWell<1> $fullWell(1,$filter)
   set tmp [correctFrames $ps $biasvector($filter) $flatvector($filter) \
		$ccdpars($filter) $fparams $bias_scale -stamp \
		-leftbias [exprGet $calibs.calib<$ifilter>->lbias] \
		-rightbias [exprGet $calibs.calib<$ifilter>->rbias]]
   set pix [regPixGet $corrimage 10 10]
   if {$pix != $i_sky+$soft_bias} {
      echo "TEST-ERR: Flat fielding without -regout changed pstamp " \
	"from $i_sky to [expr $pix-$soft_bias]"
      set ret 1
   }
   spanmaskDel *$tmp.mask
   handleSet (long)$tmp.mask 0
   regDel $ps; regDel $tmp

   handleDel $fparams
   spanmaskDel *$corrimage.mask
   handleSet (long)$corrimage.mask 0
   regDel $corrimage
   #
   # Clean up
   #
   regDel $raw
   ccdparsDel $ccdpars($filter)
   regDel $biasvector($filter); regDel $flatvector($filter)

   return $ret
}
#
#
# Run all of those tests
#
set failed 0

set sky 1000; set bias 30; set lbias 0.999; set rbias 10.87
set lbias 10; set rbias 20

set case(r) single
set case(i) double
foreach filter {r i} {
   echo Testing $case($filter) amplifier case
   
   set filterlist [list $filter]
   set psfilterlist [list $filter]
   set fieldparams [fieldparamsNew "r"]
   handleSet $fieldparams.fieldnum $run
   set fieldstat [fieldstatNew]
   
   if 1 {
      echo Running make_flats
      if {[make_flats $sky $bias $lbias $rbias]} {
	 echo "TEST-ERR: make_flats failed"
	 incr failed
      }
   }

   if ![info exists calib1byframe] {	# We didn't run make_flats
      set bias_scale [keylget softpars bias_scale]
      
      set calib1byframe [calib1byframeNew $filterlist]

      handleSet $calib1byframe.field $field
      loop i 0 [llength $filterlist] {
	 handleSet $calib1byframe.calib<$i>->sky $sky
	 handleSet $calib1byframe.calib<$i>->skyslope 0
	 handleSet $calib1byframe.calib<$i>->lbias [expr $lbias*$bias_scale]
	 handleSet $calib1byframe.calib<$i>->rbias [expr $rbias*$bias_scale]
	 
	 set trans [transNew]
	 handleSet $calib1byframe.calib<$i>->toGCC &$trans
	 handleDel $trans
      }
   }
   
   echo Running tst_correct_frame
   if {[tst_correct_frame 2000 $bias]} {
      echo "TEST-ERR: correct_frame failed"
      incr failed
   }
   
   calib1byframeDel $calib1byframe
   fieldstatDel $fieldstat
   fieldparamsDel $fieldparams
   
   # delete test frames if passes all tests 
   if {$failed != 1} {
      set files [glob $outputdev/*.fit]
      foreach file $files {
	 exec rm $file
      }
   }
}
if $failed {
   error "$failed tests failed"
}
