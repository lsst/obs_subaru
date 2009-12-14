#
# This file runs tests on the PSF finding functions. It generates
# a list of 'normal' PS stars, and a list of saturated PS stars and
# checks all functions implemented in PS pipeline to determine dgpsf
# parameteres (sigmas for two gaussians etc). 
# 
global abort_on_error softpars ccdpars stampsize wingsize max_error minL nfail sky verbose 
 
source [envscan \$PHOTO_DIR]/etc/photoStartup.tcl
source [envscan \$PHOTO_DIR]/test/tst_procs.tcl
 
set soft_param_file [envscan \$PHOTO_DIR]/test/tst_params
param2Chain $soft_param_file softpars

set stampsize [keylget softpars stampsize]  
set wingsize [keylget softpars wingsize]
set psf_def_rad [keylget softpars psf_def_rad]  
set psf_max_val [keylget softpars psf_max_val]
set psf_min_val [keylget softpars psf_min_val]
set star1_aperature [keylget softpars star1_aperature]

set fiparams [fieldparamsNew "r"]
set fparams [handleBindFromHandle [handleNew] $fiparams.frame<0>]
set soft_bias [softBiasGet]
handleSet $fparams.dark_variance [keylget softpars dark_variance]
handleSet $fparams.gain [keylget softpars gain]
handleSet $fparams.fullWell<0> [keylget softpars saturation_level]
handleSet $fparams.fullWell<1> [keylget softpars saturation_level]
handleDel $fparams

set sky 100;				 
 
#############################################################################
# This procedure makes a region and adds an object described by dgpsf
# to position (r,c). Software bias and sky are also added, as well as
# random noise. 
proc make_region_with_dgpsf {regsize r c dgpsf soft_bias sky} {
   global rand
   set reg [regNew $regsize $regsize]
   regIntSetVal $reg [expr $soft_bias+$sky]
   dgpsfAddToReg $reg $dgpsf $r $c  
   # add random noise
   regIntNoiseAdd $reg $rand $soft_bias 3 
 
   return $reg
}

#############################################################################
# display dgpsf of arbitrary intensity  
proc show_dgpsf {intensity} {
   global stampsize sky soft_bias

   # make a dgpsf and set its amplitude to given intensity
   set dgpsf [dgpsfNew -sigmax1 1.0 -sigmax2 2.0 -b 0.2 -p0 0.05 \
                       -beta 3.0 -sigmap 1.0]

   handleSet $dgpsf.sigma1_2G 1.0
   handleSet $dgpsf.sigma2_2G 3.0
   handleSet $dgpsf.b_2G 0.2
   
   handleSet $dgpsf.a $intensity

   set r [expr $stampsize / 2]
   set c [expr $stampsize / 2]

   set reg [make_region_with_dgpsf $stampsize $r $c $dgpsf $soft_bias $sky]   

   # Deleting 
   regDel $reg; dgpsfDel $dgpsf; 

   return 0
}

#############################################################################
# make a chain of n regions, nregs, each of regsize size, with a dgpsf of given 
# intensity on each of them (first nregions elements in intens)
proc get_reg_chain {nregions regsize dgpsf intens} {
   global sky soft_bias
   upvar $intens intensity

   set nregs [chainNew REGION] 
   set r [expr $regsize / 2]; set c $r

   loop i 0 $nregions { 
       handleSet $dgpsf.a $intensity($i)
       set reg [make_region_with_dgpsf $regsize $r $c $dgpsf $soft_bias $sky]
       chainElementAddByPos $nregs $reg TAIL AFTER
       handleDel $reg;   
   }
  
   return $nregs
}


#############################################################################
# display a mosaic of n dgpsfs with intensity increasing from int1 to int2 
proc show_dgpsf_mosaic {n int1 int2} {
   global stampsize sky soft_bias

   if {$n < 2} {
       set n 2
   }

   # define dgpsf 
   set dgpsf [dgpsfNew -sigmax1 0.94 -sigmax2 2.70 -b 0.15 -p0 0.05 \
                       -beta 3.0 -sigmap 1.1]
   handleSet $dgpsf.sigma1_2G 1.0
   handleSet $dgpsf.sigma2_2G 3.0
   handleSet $dgpsf.b_2G 0.2

   # define intensity array
   set delint [expr ($int2 - $int1) / ($n - 1)]   
   loop i 0 $n {
       set intensity($i) [expr $int1 + $i * $delint]
   }

   set reg_chain [get_reg_chain $n $stampsize $dgpsf intensity]

   if {[chainSize $reg_chain] > 0} {
        set mosaic [ps_mosaic  $reg_chain $stampsize]
        tv $mosaic 
      	regDel $mosaic
   }
 
   chainDestroy $reg_chain regDel
   dgpsfDel $dgpsf; 

   return 0
}

#############################################################################
# display a mosaic of regions of size regsize given in star1clist; print 
# flags if print_flags is set
proc show_star1clist_mosaic {star1clist regsize filters print_flags} {
 
   set cursor [chainCursorNew $star1clist]
   set regions [chainNew REGION]
   while {[set star [chainWalk $star1clist $cursor]] != ""} {
       foreach filter $filters {
          set ifilter [lsearch $filters $filter]
          set onestar [handleBindFromHandle [handleNew] \
	            *$star.star1<$ifilter>->object->region]
          chainElementAddByPos $regions $onestar TAIL AFTER
          handleDel $onestar
          if {$print_flags} {
            echo " Filter: $filter:"
            set flags [exprGet (int)$star.star1<$ifilter>->flags] 
            print_flags_info $flags
          }
       }
   }
   chainCursorDel $star1clist $cursor

  # displaying 
  set mosaic [ps_mosaic $regions $regsize]
  tv $mosaic  
  regDel $mosaic   
  chainDel $regions

  return 0
}

#############################################################################
# compare and possibly print relevant values for two psfs
# The error checking assumes that beta=3, that is, that power-law wing part
# degenerates to p0*sigmap^3. If beta will be a fitted parameter (see proc
# findPsf), than it might be different than 3, and this test could fail. 
proc compare_psfs {origpsf newpsf print_flag} {
    global abort_on_error 

    ### first unpack the parameters
     # original psf
     set sx1 [exprGet $origpsf.sigmax1]
     set sx2 [exprGet $origpsf.sigmax2]
     set b [exprGet $origpsf.b]
     set p0 [exprGet $origpsf.p0]
     set beta [exprGet $origpsf.beta]
     set sp [exprGet $origpsf.sigmap]
     set p0sp3 [expr $p0*$sp*$sp*$sp]
     set p0bsp  [expr $p0*pow($beta,$beta/2.0)*pow($sp,$beta)]
     set r0 [expr pow($p0,1.0/$beta)*pow($beta,0.5)*$sp]
     set I20 [expr pow($r0/20.0,$beta)]
     if {$print_flag} {
       set string [format "%6.3f %6.3f %6.4f %9.6f %6.3f %6.3f %6.3f %7.2e %7.4f" \
		   $sx1 $sx2 $b $p0 $beta $sp $r0 $I20 $p0bsp]
       echo "                  s1     s2     b       p0     beta  sigmap    r0  I(20)  p0*beta^(beta/2)*sp^beta"
       echo " original psf: $string"
     }
     # new psf
     set nsx1 [exprGet $newpsf.sigmax1]
     set nsx2 [exprGet $newpsf.sigmax2]
     set nb [exprGet $newpsf.b]
     set np0 [exprGet $newpsf.p0]
     set nbeta [exprGet $newpsf.beta]
     set nsp [exprGet $newpsf.sigmap]
     set np0sp3 [expr $np0*$nsp*$nsp*$nsp]
     set np0bsp  [expr $np0*pow($nbeta,$nbeta/2.0)*pow($nsp,$nbeta)]
     set nr0 [expr pow($np0,1.0/$nbeta)*pow($nbeta,0.5)*$nsp]
     set nI20 [expr pow($nr0/20.0,$nbeta)]

     if {$print_flag} {
       set string [format "%6.3f %6.3f %6.4f %9.6f %6.3f %6.3f %6.3f %7.2e %7.4f" \
	   	   $nsx1 $nsx2 $nb $np0 $nbeta $nsp $nr0 $nI20 $np0bsp]
       echo "      new psf: $string"
     }
     # differences
     set dsx1 [expr 100 * abs($nsx1 - $sx1) / $sx1] 
     set dsx2 [expr 100 * abs($nsx2 - $sx2) / $sx2]
     set db   [expr 100 * abs($nb - $b) / $b] 
     set dp0 [expr 100 * abs($np0 - $p0) / $p0] 
     set dbeta [expr 100 * abs($nbeta - $beta) / $beta] 
     set dsp [expr 100 * abs($nsp - $sp) / $sp] 
     set dp0sp3 [expr 100 * abs($np0sp3 - $p0sp3) / $p0sp3] 
     set dp0bsp [expr 100 * abs($np0bsp - $p0bsp) / $p0bsp] 
     set dr0 [expr 100 * abs($nr0 - $r0) / $r0]
     set dI20 [expr 100 * abs($nI20 - $I20) / $I20]
     if {$print_flag} {
       set string [format "%6.3f %6.3f %6.4f %9.6f %6.3f %6.3f %6.3f %7.2e %7.4f" \
		   $dsx1 $dsx2 $db $dp0 $dbeta $dsp $dr0 $dI20 $dp0bsp]
       echo " \% difference: $string"
     }

    ### check the differences
     # first set the tolerances (all in %)
     set maxdsx1    5;  # sigma for the first gaussian
     set maxdsx2   10;  # sigma for the second gaussian
     set maxdb     20;  # amplitude for the second gaussian
     set maxdp0sp3 30;  # p0 and sigmap are degenerate
     set maxdp0bsp 30;  # p0 and sigmap are degenerate
     set maxdI20   50;  # intensity at r=20
     # compare differences and tolerances     
     set toler {maxdsx1 maxdsx2 maxdb maxdI20}
     set diffs {dsx1 dsx2 db dI20}
     set param {sigmax1 sigmax2 b I(r=20)}
     set fail 0
     foreach d $diffs {
        set i [lsearch $diffs $d] 
        set dif [expr $$d]
        set tol [expr $[lindex $toler $i]]      
        if {$dif > $tol} {
           incr fail
           echo " Failed psf on parameter [lindex $param $i]:"
           echo "  tolerance (%): $tol, dif (%): $dif"
           if {[info exists abort_on_error] && $abort_on_error} {       
              error "Get the info by calling compare_psfs $origpsf $newpsf 1"
           }
        }
     }

  return $fail
}
 
 

#############################################################################
# get the lists starlist, starmasks, winglist, and wingmasks needed to run
# findPsf. form the lists for nframes, each frame having PSperframe PS stars
# (unsaturated), and nwings saturated stars (for power-law wings)
proc get_the_lists {dgpsf nframes PSperframe nwings filters starlist starmasks winglist wingmasks} {
   global softpars sky soft_bias
   upvar $starlist slist
   upvar $starmasks smasks
   upvar $winglist wlist
   upvar $wingmasks wmasks

  ### read software parameters
   set stampsize [keylget softpars stampsize]  
   set wingsize [keylget softpars wingsize]
   set psf_def_rad [keylget softpars psf_def_rad] 
   set wing_def_rad [keylget softpars wing_def_rad]
   set star1_aperature [keylget softpars star1_aperature]
   set nsigma1 [keylget softpars psf_nsigma1]
   set nsigma2 [keylget softpars psf_nsigma2]
   set niter [keylget softpars psf_niter]
   set critical_amplitude [keylget softpars psf_critical_amplitude]
   set min_starnum [keylget softpars min_starnum]
   set gain [keylget softpars gain]
   set dark_variance [keylget softpars dark_variance]
   set psf_threshold [keylget softpars psf_threshold]
   set bias_scale [keylget softpars bias_scale]
   set nfilter [llength $filters]
   set sigma_badsky [keylget softpars sigma_badsky] 
   set sigma_multi [keylget softpars sigma_multi] 
   set second_peak [keylget softpars second_peak] 
   set cr_min_s [getsoftpar cr_min_sigma]           
   set cr_min_e [getsoftpar cr_min_e]            
   set cr_cond3 [getsoftpar cr_cond3]
   set cr_cond32 [getsoftpar cr_cond32]

   # some initializations
   set star1clist [chainNew STAR1C]
   set wingstar1clist [chainNew STAR1C]
   foreach filter $filters {
      set ps_rejects($filter) [chainNew STAR1]
      set wing_rejects($filter) [chainNew STAR1]
   }

  ### define intensities for PS and wing stars
   if {$PSperframe < 2} {
       set PSperframe 2
   }
   if {$nwings < 2} {
       set nwings 2
   }
   # intensity range for PS stars
   set int1 1000.0
   set int2 80000.0
   set delint [expr ($int2 - $int1) / ($PSperframe - 1)]   
   loop i 0 $PSperframe {
       set PSintensity($i) [expr $int1 + $i * $delint]
   }
   # intensity range for wing stars
   set int1 65000.0
   set int2 10000000.0
   set delint [expr ($int2 - $int1) / ($nwings - 1)]   
   loop i 0 $nwings {
       set Wintensity($i) [expr $int1 + $i * $delint]
   }
 
  ### now generate all auxiliary quantities needed for makeStar1clist
   # frame parameters
   global fiparams
   set fparams [handleBindFromHandle [handleNew] $fiparams.frame<0>]

   # flat vector
   set flatvector [regNew 1 2048]
   regIntSetVal $flatvector 2048

   # bias
   set bias [regNew 1 2128]
   regIntSetVal $bias [expr $soft_bias * $bias_scale]
   # ccdpars
   foreach filter $filters {
       fetch_ccdpars [envscan \$PHOTO_DIR]/test/tst_opConfig.par \
                     [envscan \$PHOTO_DIR]/test/tst_opECalib.par \
                     [envscan \$PHOTO_DIR]/test/tst_opBC.par \
                     ccdpars $filter 1 $softpars
   }
   # calib
   set calib [calib1New [lindex $filters 0]]
   handleSet $calib.dark_variance $dark_variance  
   handleSet $calib.gain $gain 
   handleSet $calib.lbias  0
   handleSet $calib.rbias  0
   handleSet $calib.sky $sky
   handleSet $calib.skysig [expr $sky/100.0]
   handleSet $calib.skyerr [expr $sky/300.0]
   set trans [transNew]
   handleSetFromHandle $calib.toGCC &$trans
   handleDel $trans
   # flag to get corrected regions
   set make_psf_regions "-region" 
   # set make_psf_regions "" 
 
  ### Loop over each filter ### 
   foreach filter $filters {
      #echo "     filter: $filter"
      set ifilter [lsearch $filters $filter]
      set id0 0; set wid0 0
      ## loop over frames
      loop field 0 $nframes {
      #echo "            frame: $field"
          # generate a chain of regions (instead of reading scFang fits files)
          set pstamps [get_reg_chain $PSperframe $stampsize $dgpsf PSintensity]
          if {0} {
              # displaying 
              set mosaic [ps_mosaic $pstamps  $stampsize]
              tv $mosaic  
              regDel $mosaic  
          } 
          # correct the PS regions and produce masks
          eval makeStar1clist $star1clist $smasks($filter) $pstamps \
	     $ifilter $nfilter $id0 $field $flatvector $bias  \
	     $ccdpars($filter) $bias_scale $calib $stampsize $sigma_multi $sigma_badsky $second_peak \
             0 $star1_aperature $psf_def_rad $cr_min_s $cr_min_e $cr_cond3 $cr_cond32 \
             $fparams $psf_threshold $make_psf_regions   
 
	  incr id0 [exprGet $pstamps.nElements]
 	  chainDestroy $pstamps regDel
          # generate a chain of regions (instead of reading scWing fits files)
          set wings [get_reg_chain $nwings $wingsize $dgpsf Wintensity] 
          if {0} {
              # displaying 
              set mosaic [ps_mosaic $wings  $wingsize]
              tv $mosaic  
              regDel $mosaic  
          } 
 
          # correct the wing regions and produce masks
          eval makeStar1clist $wingstar1clist $wmasks($filter) $wings \
	     $ifilter $nfilter $wid0 $field $flatvector $bias  \
	     $ccdpars($filter) $bias_scale $calib $wingsize $sigma_multi $sigma_badsky $second_peak \
             1 $star1_aperature $wing_def_rad $cr_min_s $cr_min_e $cr_cond3 $cr_cond32 \
             $fparams $psf_threshold $make_psf_regions   

	  incr wid0 [exprGet $wings.nElements]
 	  chainDestroy $wings regDel
      }; # end of loop over frames
   }; # end of loop over filters
 

  ### displaying corrected frames
   if {0} {
     echo " PS stars:" 
     show_star1clist_mosaic $star1clist $stampsize $filters 1
     echo " Wing stars:" 
     show_star1clist_mosaic $wingstar1clist $wingsize $filters 1
   }

  ### rejecting bad PS stars (setting flags on, actual separation is done below)
   # psfReject $star1clist $filters $nsigma $niter $min_starnum $critical_amplitude 

  ### split star1clist into starlist and ps_rejects by using info from flags
   # same for wing stars, but ignore flags
   foreach filter $filters {
      set ifilter [lsearch $filters $filter]
      set flags 0x0
      makeStarlist $star1clist $slist($filter) $ps_rejects($filter) $ifilter \
                   0 [expr [chainSize $star1clist] - 1] $flags
      set flags 0x1fff
      makeStarlist $wingstar1clist $wlist($filter) $wing_rejects($filter) $ifilter \
                   0 [expr [chainSize $wingstar1clist] - 1] $flags  
   }

   # clean up
   regDel $flatvector
   regDel $bias
   calib1Del $calib
   handleDel $fparams 
   chainDestroy $star1clist star1cDel
   chainDestroy $wingstar1clist star1cDel
   foreach filter $filters {
      ccdparsDel $ccdpars($filter)
      chainDestroy $ps_rejects($filter) star1Del
      chainDestroy $wing_rejects($filter) star1Del
   }

   return 0
}

#############################################################################
# make star lists and check that function findPsf measures correct parameters
# describing DGPSF. Lists will extend over nframes frames with PSperframe 
# stamps on each frame, and nwings (for all frames together)
proc check_findPsf {nframes PSperframe nwings} {

   global softpars ccdpars stampsize wingsize verbose
 
  ### initialization
   set filterlist {u r z} 
   set psf_nframe [keylget softpars psf_nframe]  
   set nowing_option [keylget softpars nowing_option]   
   set err_min [keylget softpars soft_err_min]   
   set err_max [keylget softpars  soft_err_max]   
   set nfilter [llength $filterlist]
   set nsigma1 [keylget softpars psf_nsigma1]
   set nsigma2 [keylget softpars psf_nsigma2]
   set niter [keylget softpars psf_niter]
   set min_starnum [keylget softpars min_starnum]
   set chisq_max  [keylget softpars chisq_max]   
   set chisq_cutoff  [keylget softpars chisq_cutoff]  
   set ncellclip [keylget softpars ncellclip] 
   set nbadcell [keylget softpars nbadcell] 
   set maxdev [keylget softpars maxdev] 
   set clip_mod [keylget softpars clip_mod]   

   # echo " ** Filterlist: $filterlist **"
   initCorrectFrames
   initCellFitobj [envscan [keylget softpars cellprofile_file]] $nfilter
   initProfileExtract
   
   foreach filter $filterlist {
      set starlist($filter) [chainNew STAR1]
      set winglist($filter) [chainNew STAR1]
      set starmasks($filter) [chainNew OBJMASK]
      set wingmasks($filter) [chainNew OBJMASK]
      set frameinfolist($filter) [chainNew FRAME_INFORMATION]
   }

   set calib1byframelist [chainNew CALIB1BYFRAME]
   loop field 0 $nframes {
      set cframe [calib1byframeNew $filterlist]
      handleSet $cframe.field $field
      chainElementAddByPos $calib1byframelist $cframe TAIL AFTER
      handleDel $cframe
   }

   ### generating lists of stars based on a canonical dgpsf
   # define canonical dgpsf 
   set dgpsf [dgpsfNew -sigmax1 1.0 -sigmax2 2.5 -b 0.1 -p0 0.05 \
                       -beta 3.0 -sigmap 1.0]
   handleSet $dgpsf.sigma1_2G 1.0
   handleSet $dgpsf.sigma2_2G 3.0
   handleSet $dgpsf.b_2G 0.2

   # make the lists with this dgpsf
   get_the_lists $dgpsf $nframes $PSperframe $nwings $filterlist starlist starmasks winglist wingmasks

   ###  main call for fitting PSF ###
   foreach f $filterlist {
      set gains($f) 1; set dvs($f) 0
   }

   echo "FILTERS: $filterlist"
 
   echo "err_min err_max ncellclip nbadcell maxdev clip_mod chisq_max"
   echo "  $err_min    $err_max     $ncellclip        $nbadcell      $maxdev      $clip_mod      $chisq_max"
   loop frame 0 [chainSize $calib1byframelist] {
      foreach f $filterlist {   
          set ifilter [lsearch $filterlist $f]
          findPsf starlist starmasks winglist wingmasks $calib1byframelist frameinfolist gains dvs $filterlist\
                   $ifilter $psf_nframe $err_min $err_max $ncellclip $frame $frame $nbadcell $maxdev $clip_mod\
                   $chisq_max $chisq_cutoff $nsigma1 $nsigma2 $niter $min_starnum  $nowing_option      
      }
   }
 
   ### count how many psf parameters are not reproduced properly
   set nfail 0
   set cursor [chainCursorNew $calib1byframelist]
   while {[set cal [chainWalk $calib1byframelist $cursor NEXT]] != ""} {
        foreach filter $filterlist {
	    set ifilter [lsearch $filterlist $filter]
            set fitpsf [handleBindFromHandle [handleNew] *$cal.calib<$ifilter>->psf]
            # allowed tolerances are defined in compare_psfs
            incr nfail [compare_psfs $dgpsf $fitpsf 1]
            handleDel $fitpsf
        }
   }
   chainCursorDel $calib1byframelist $cursor
 

   # clean up
   finiCorrectFrames
   finiProfileExtract
   finiCellFitobj
   dgpsfDel $dgpsf
   chainDestroy $calib1byframelist calib1byframeDel
   foreach filter $filterlist {
       chainDestroy $starlist($filter) star1Del
       chainDestroy $frameinfolist($filter) frameinformationDel
       chainDestroy $starmasks($filter) objmaskDel  
       chainDestroy $winglist($filter) star1Del
       chainDestroy $wingmasks($filter) objmaskDel 
   }

   return $nfail
}

#########################################################################
# This proc tests finding dgpsf from a list of PS and wing stars. It is
# implemented in the same way as relevant parts of ps_pipeline.tcl, except
# that it uses artificial PS and wing stars with known properties

proc test_0 {} {

  # auxiliary procs
  #show_dgpsf 50000
  #show_dgpsf_mosaic 4 1000 100000

  # define the extent of checking
  set nframes 2
  set PSperframe 6
  set Wingsperframe 3

  # check_findPsf returns number of failures
  set nfail [check_findPsf $nframes $PSperframe $Wingsperframe]
  if {$nfail} {
     echo "TEST-ERR: $nfail failures for findPsf."
  }  

  return $nfail
}


#########################################################################
########     Main        #################

  # take care of memory reporting
  set startMem [memSerialNumber]
  set seed  819396102   
  if ![info exists rand] {
      set rand [phRandomNew 100000 -seed $seed -type 2]
  }

  # loop over all tests
  if {![info exists tests]} {
     set tests [list 0]; # 0=findPsf
  }
  set name(0) "findPsf"
  set nfail 0
  foreach test $tests {
     echo "Testing $name($test)"
    # incr nfail [test_$test]
  }  

  phRandomDel $rand; unset rand
  fieldparamsDel $fiparams

  # Check for memory leaks
  assert {[memBlocksGetRange [expr $startMem+1] [memSerialNumber]] == {}}

if $nfail {
   error "Failed $nfail tests"
}

