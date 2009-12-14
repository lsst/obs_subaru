global starsep maxerr

source [envscan \$PHOTO_DIR]/etc/photoStartup.tcl

set starsep 10
set maxerr 0.005
set checkerrs 0
######################################################################
# Procs to initialize the structs we need to make MT input
######################################################################

proc star1Init {frame ra dec flux fluxErr} {

   set star [star1pcNew]

   handleSet $star.frame $frame
   handleSet $star.ra $ra
   handleSet $star.dec $dec
   handleSet $star.apCounts $flux
   handleSet $star.apCountsErr $fluxErr
   
   return $star
}

proc frameinfoInit {frame airmass mjd} {
   
   set fi [frameinfoNew]
   handleSet $fi.field $frame
   handleSet $fi.airmass $airmass
   handleSet $fi.mjd $mjd
   return $fi
}

proc starcInit {ncolors ra dec mjd filters mags magerrs} {
   
   assert {[llength $filters] == $ncolors}
   assert {[llength $mags] == $ncolors}
   assert {[llength $magerrs] == $ncolors}

   set star [starcNew $ncolors]
   handleSet $star.ra $ra
   handleSet $star.dec $dec
   handleSet $star.mjd $mjd
   starcFilterSet $star $filters 

   loop i 0 $ncolors {
      handleSet $star.mag<$i>->mag [lindex $mags $i]
      handleSet $star.mag<$i>->magErr [lindex $magerrs $i]
   }
   
   return $star
}

proc extinctionInit {ncolors mjd filters mags magerrs} {

   assert {[llength $filters] == $ncolors}
   assert {[llength $mags] == $ncolors}
   assert {[llength $magerrs] == $ncolors}

   set ext [extinctionNew $ncolors]
   handleSet $ext.mjd $mjd

   loop i 0 $ncolors {
      handleSet $ext.k<$i>->passBand<0> \
	  [lindex $filters $i]
      handleSet $ext.k<$i>->mag \
	  [lindex $mags $i]
      handleSet $ext.k<$i>->magErr\
	  [lindex $magerrs $i]
   }
   return $ext
}

proc calib1bytimeInit {ncolors mjd filters fluxes fluxerrs} {
   
   assert {[llength $filters] == $ncolors}
   assert {[llength $fluxes] == $ncolors}
   assert {[llength $fluxerrs] == $ncolors}

   set tcal [calib1bytimeNew $filters]
   handleSet $tcal.mjd $mjd
      
   loop i 0 $ncolors {
      handleSet $tcal.calib<$i>->flux20 \
	  [lindex $fluxes $i]
      handleSet $tcal.calib<$i>->flux20Err\
	  [lindex $fluxerrs $i]
   }
   return $tcal
}

proc calib1byframeInit {frame filters} {

   set fcal [calib1byframeNew $filters]
   handleSet $fcal.field $frame
   
   return $fcal
}

######################################################################
# Proc to check results
######################################################################
proc checkflux {cal flux fluxerr proc test} {
   global maxerr checkerrs
   
   set result 0
   set flux20 [exprGet $cal.flux20]
   set err [expr ($flux20 - $flux)/$flux20]
   if {[abs $err] > $maxerr} {
      echo "$proc: $test flux20 is incorrect"
      incr result 1
   }
   if {$checkerrs} {
   set flux20Err [exprGet $cal.flux20Err]
   set err [expr ($flux20Err - $fluxerr)/$flux20Err]
   if {[abs $err] > $maxerr} {
      echo "$proc: $test flux20Err is incorrect"
      incr result 1
   }
   }
   return $result
}


######################################################################
# Make the actual inputs
######################################################################


######################################################################
# This proc makes a chain containing 1 MT patches in 2 bands. 
# The patch contains 3 starcs

proc makepatches { } {

   set patches [chainNew CHAIN]

#patch 1: Flux20 at g-r=0 is 50 in g and 500 in r (neglecting airmass)
   set patch [chainNew STARC]

   set star [starcInit 2 179.0 30.0 100.2 {g r} {14.0 15.0} {.02 .02}]
   chainElementAddByPos $patch $star TAIL AFTER
   handleDel $star

   set star [starcInit 2 180.0 30.0 100.2 {g r} {15.0 15.0} {.02 .02}]
   chainElementAddByPos $patch $star TAIL AFTER
   handleDel $star

   set star [starcInit 2 181.0 30.0 100.2 {g r} {15.0 14.0} {.02 .02}]
   chainElementAddByPos $patch $star TAIL AFTER
   handleDel $star

   chainElementAddByPos $patches $patch TAIL AFTER
   handleDel $patch

   return $patches
}

######################################################################
# This proc makes a chain 3 extinction structs. The extinction 
# increases by .01 magnitude / .1 day

proc make3extinctions {} {
   
   set chain [chainNew EXTINCTION]

   set ext [extinctionInit 2 100.0 {g r} {.1 .1} {.001 .001}]
   chainElementAddByPos $chain $ext TAIL AFTER
   handleDel $ext
   
   set ext [extinctionInit 2 100.4 {g r} {.5 .5} {.001 .001}]
   chainElementAddByPos $chain $ext TAIL AFTER
   handleDel $ext

   set ext [extinctionInit 2 100.8 {g r} {.9 .9} {.001 .001}]
   chainElementAddByPos $chain $ext TAIL AFTER
   handleDel $ext
   
   return $chain
}

######################################################################
# This proc makes a chain 3 frameinfos.
# 
# Notes: expected results with these frameinfos and extinctions above:
#
# Frame Airmass K    Ext(mags) Extcorr     Flux20(50)corr Flux20(500)corr
# &time                        (10^0.4*Ext)
# 0 100.1  0    0.2  0         1.0000      50.0           500.0
# 1 100.2 1.0   0.3  0.3       1.3182567   37.928879      379.2888
# 2 100.3 1.1   0.4  0.44      1.4996848   33.340339      333.40339
#
proc makeframestuff {frameinfos calib1byframes} {
   upvar $frameinfos finfos $calib1byframes cframes

   set finfos [chainNew FRAMEINFO]

   set fi [frameinfoInit 13 0.0 100.1]
   chainElementAddByPos $finfos $fi TAIL AFTER
   handleDel $fi

   set fi [frameinfoInit 14 1.0 100.2]
   chainElementAddByPos $finfos $fi TAIL AFTER
   handleDel $fi

   set fi [frameinfoInit 15 1.1 100.3]
   chainElementAddByPos $finfos $fi TAIL AFTER
   handleDel $fi

   set cframes [chainNew CALIB1BYFRAME]

   set cf [calib1byframeInit 13 {g r}]
   chainElementAddByPos $cframes $cf TAIL AFTER
   handleDel $cf

   set cf [calib1byframeInit 14 {g r}]
   chainElementAddByPos $cframes $cf TAIL AFTER
   handleDel $cf

   set cf [calib1byframeInit 15 {g r}]
   chainElementAddByPos $cframes $cf TAIL AFTER
   handleDel $cf

}

######################################################################
# This proc makes a 
# 

proc makeStarlists {lists} {
   upvar $lists starlist

   # g starlist
   set starlist(g) [chainNew STAR1PC]

   set star [star1Init 13 179.0 30.0 12308.3 100.]
   chainElementAddByPos $starlist(g) $star TAIL AFTER
   handleDel $star

   set star [star1Init 13 180.0 30.0 5000 100.]
   chainElementAddByPos $starlist(g) $star TAIL AFTER
   handleDel $star

   set star [star1Init 13 181.0 30.0 5100 100.]
   chainElementAddByPos $starlist(g) $star TAIL AFTER
   handleDel $star


   # r starlist
   set starlist(r) [chainNew STAR1PC]

   set star [star1Init 13 179.0 30.0 49000 100.]
   chainElementAddByPos $starlist(r) $star TAIL AFTER
   handleDel $star

   set star [star1Init 13 180.0 30.0 50000 100.]
   chainElementAddByPos $starlist(r) $star TAIL AFTER
   handleDel $star
   
   set star [star1Init 13 181.0 30.0 128106 100.]
   chainElementAddByPos $starlist(r) $star TAIL AFTER
   handleDel $star


}

proc makeCframes {} {

   set chain [chainNew CALIB1BYFRAME]

   set fcal [calib1byframeInit 13 {g r} ]
   chainElementAddByPos $chain $fcal TAIL AFTER
   handleDel $fcal

   set fcal [calib1byframeInit 14 {g r} ]
   chainElementAddByPos $chain $fcal TAIL AFTER
   handleDel $fcal

   set fcal [calib1byframeInit 15 {g r} ]
   chainElementAddByPos $chain $fcal TAIL AFTER
   handleDel $fcal

}

######################################################################
# Actual test scripts
######################################################################

######################################################################
# Test with no new patches: with no samedate oldcal and then with 
# samedate oldcal
######################################################################
proc fpp_with_nopatches {} {

   global starsep maxerr checkerrs
  
   set result 0

   # Make inputs
   # use a single extinction and frame. Starlists can be empty

   set starlist(g) [chainNew STAR1]
   set starlist(r) [chainNew STAR1]
   
   set patches [chainNew CHAIN]

   set extinctions [chainNew EXTINCTION]
   set ext [extinctionInit 2 100.0 {g r} {0. 0.} {0.001 0.001}] 
   chainElementAddByPos $extinctions $ext TAIL AFTER
   handleDel $ext

   set frameinfos [chainNew FRAMEINFO]
   set fi [frameinfoInit 13 1.0 100.1] 
   chainElementAddByPos $frameinfos $fi TAIL AFTER
   handleDel $fi

   set oldoldcal [calib1bytimeInit 2 99.0 {g r} {50 500} {1 10}]
   set oldcal [calib1bytimeInit 2 100.3 {g r} {500 50} {10 1}]

   set calib1bytimes [chainNew CALIB1BYTIME]

   set calib1byframes [chainNew CALIB1BYFRAME]
   set fcal [calib1byframeInit 13 {g r}]
   chainElementAddByPos $calib1byframes $fcal TAIL AFTER

   # Run FPP with oldoldcal
   set sys_ph_err 0.001
   set n_max 0
   loop i 0 2 { 
      set flux20_m($i) 0
      set flux20_c($i) 0
      set flux20_chi2($i) 0
   }

   set outstr \
       [findPhotoParams {g r} {r g} {0. 0.} {0. 0.} starlist $patches \
	    $extinctions $frameinfos $oldoldcal $starsep $sys_ph_err 0.05 $n_max \
	    flux20_m flux20_c flux20_chi2 $calib1bytimes $calib1byframes]

   set patchcal [keylget outstr patchcal]
   set framecal [keylget outstr framecal]
   if { ($patchcal != "") || $framecal != ""} {
      echo "fpp_with_nopatches: (oldoldcal) $outstr"
      set result 1
   }
   
   set f0 [handleBindFromHandle [handleNew] *$fcal.calib<0>]
   set ooc0 [handleBindFromHandle [handleNew] *$oldoldcal.calib<0>]
   set oocf20 [exprGet $ooc0.flux20]

   incr result [checkflux $f0 $oocf20 1.007 fpp_with_nopatches (oldoldcal)]
   # Run FPP with oldcal

   set outstr \
       [findPhotoParams {g r} {r g} {0. 0.} {0. 0.} starlist $patches \
	    $extinctions $frameinfos $oldcal $starsep $sys_ph_err 0.05 $n_max \
	    flux20_m flux20_c flux20_chi2 $calib1bytimes $calib1byframes]

   set patchcal [keylget outstr patchcal]
   set framecal [keylget outstr framecal]
   if { ($patchcal != "") || $framecal != "g r"} {
      echo "fpp_with_nopatches: (oldcal) $outstr"
      set result 1

   }
   
   set oc0 [handleBindFromHandle [handleNew] *$oldcal.calib<0>]
   set ocf20 [exprGet $oc0.flux20]
   incr result [checkflux $f0 $ocf20 10.07 fpp_with_nopatches (oldcal)]

   # cleanup
   handleDel $f0; handleDel $ooc0; handleDel $oc0
   chainDel $starlist(g); chainDel $starlist(r);
   chainDel $patches; chainDel $calib1bytimes;
   chainDestroy $extinctions extinctionDel
   chainDestroy $frameinfos frameinfoDel
   calib1bytimeDel $oldoldcal; calib1bytimeDel $oldcal
   chainDestroy $calib1byframes calib1byframeDel

   return $result
}

######################################################################
# Test with a patch that matches stars in both bands. 
# Oldcal is out of date, so interpolator is tested by extinctions only.
######################################################################
proc fpp_with_matching_patch {} {
   global starsep maxerr checkerrs
   
   set result 0
   
   makeStarlists starlist
   set patches [makepatches]
   set extinctions [make3extinctions]
   makeframestuff frameinfos calib1byframes
   set oldoldcal [calib1bytimeInit 2 99.0 {g r} {500 50} {10 1}]
   set calib1bytimes [chainNew CALIB1BYTIME]
   set sys_ph_err 0.001
   set n_max 0
   loop i 0 2 { 
      set flux20_m($i) 0
      set flux20_c($i) 0
      set flux20_chi2($i) 0
   }

   set outstr \
       [findPhotoParams {g r} {r g} {0. 0.} {0. 0.} starlist $patches \
	    $extinctions $frameinfos $oldoldcal $starsep $sys_ph_err 0.05 $n_max \
	    flux20_m flux20_c flux20_chi2 $calib1bytimes $calib1byframes]

   
   # check outstr
   set patchcal [keylget outstr patchcal]
   set framecal [keylget outstr framecal]
   if { ($patchcal != "g r") || $framecal != "g r"} {
      echo "fpp_with_matching_patch: (oldoldcal) $outstr"
      set result 1

   }

   # check patchcal values
   set tcal [chainElementGetByPos $calib1bytimes 0]
   set cal0 [handleBindFromHandle [handleNew] *$tcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$tcal.calib<1>]
   
   incr result [checkflux $cal0 50 0.69 fpp_with_matching_patch patchcal(g)]
   handleDel $cal0

   incr result [checkflux $cal1 500 5.34 fpp_with_matching_patch patchcal(r)]
   handleDel $cal1
   
   # check framecal values

   # zeroth frame had airmass 0, so values should be same as patchcal
   set fcal [chainElementGetByPos $calib1byframes 0]
   set cal0 [handleBindFromHandle [handleNew] *$fcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$fcal.calib<1>]
   
   incr result [checkflux $cal0 50 0.69 fpp_with_matching_patch framecal(0,g)]
   handleDel $cal0

   incr result [checkflux $cal1 500 5.34 fpp_with_matching_patch framecal(0,r)]
   handleDel $cal1
   
   # first frame has airmass 1.0, extinction should  be 0.3 mag
   set fcal [chainElementGetByPos $calib1byframes 1]
   set cal0 [handleBindFromHandle [handleNew] *$fcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$fcal.calib<1>]
   
   incr result [checkflux $cal0 37.928879 0.527 fpp_with_matching_patch \
		    framecal(1,g)]
   handleDel $cal0

   incr result [checkflux $cal1 379.2888 4.098 fpp_with_matching_patch \
		    framecal(1,r)]
   handleDel $cal1

   # cleanup 
   chainDestroy $starlist(g) star1pcDel; chainDel $starlist(r)
   patchesDestroy $patches; chainDestroy $extinctions extinctionDel
   chainDestroy $calib1byframes calib1byframeDel
   chainDestroy $calib1bytimes calib1bytimeDel
   chainDestroy $frameinfos frameinfoDel
   calib1bytimeDel $oldoldcal

   return $result
}
######################################################################
# Test with a patch that matches stars in 1 band (g)
# Oldcal is out of date, so interpolator is tested by extinctions only.
######################################################################
proc fpp_with_partial_match_and_old_oldcal {} {
   global starsep maxerr checkerrs
   
   set result 0

   makeStarlists starlist
   chainDestroy $starlist(r) star1pcDel;
   set starlist(r) [chainNew STAR1PC]
   set patches [makepatches]
   set extinctions [make3extinctions]
   makeframestuff frameinfos calib1byframes
   set oldoldcal [calib1bytimeInit 2 99.0 {g r} {500 50} {10 1}]
   set calib1bytimes [chainNew CALIB1BYTIME]
   set sys_ph_err 0.001
   set n_max 0
   loop i 0 2 { 
      set flux20_m($i) 0
      set flux20_c($i) 0
      set flux20_chi2($i) 0
   }

   set outstr \
       [findPhotoParams {g r} {r g} {0. 0.} {0. 0.} starlist $patches \
	    $extinctions $frameinfos $oldoldcal $starsep $sys_ph_err 0.05 $n_max \
	    flux20_m flux20_c flux20_chi2 $calib1bytimes $calib1byframes]
   
   # check outstr
   set patchcal [keylget outstr patchcal]
   set framecal [keylget outstr framecal]
   if { ($patchcal != "g") || $framecal != "g"} {
      echo "fpp_with_partial_match_and_old_oldcal: $outstr"
      set result 1
   }
   
   # check that patchcal contains calib for g only
   set tcal [chainElementGetByPos $calib1bytimes 0]
   set cal0 [handleBindFromHandle [handleNew] *$tcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$tcal.calib<1>]
   
   incr result [checkflux $cal0 50 0.69 fpp_with_partial_match_and_old_oldcal \
		    patchcal(g)]
   handleDel $cal0

   incr result [checkflux $cal1 -100 -100 \
		    fpp_with_partial_match_and_old_oldcal patchcal(r)]
   handleDel $cal1

   # check that framecals contain patchcal info in g and oldoldcal in r

   # zeroth frame had airmass 0, so values should be same as 
   # patchcal in g and oldoldcal in r
   set fcal [chainElementGetByPos $calib1byframes 0]
   set cal0 [handleBindFromHandle [handleNew] *$fcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$fcal.calib<1>]
   
   incr result [checkflux $cal0 50 0.69 fpp_with_partial_match_and_old_oldcal \
		    framecal(0,g)]
   handleDel $cal0

   incr result [checkflux $cal1 50 1 fpp_with_partial_match_and_old_oldcal \
		    framecal(0,r)]
   handleDel $cal1


   # cleanup 
   chainDestroy $starlist(g) star1pcDel; chainDel $starlist(r)
   patchesDestroy $patches; chainDestroy $extinctions extinctionDel
   chainDestroy $calib1byframes calib1byframeDel
   chainDestroy $calib1bytimes calib1bytimeDel
   chainDestroy $frameinfos frameinfoDel
   calib1bytimeDel $oldoldcal

   return $result
}
######################################################################
# Test with a patch that matches stars in 1 band (g) and uses a 
# same date oldcal. Oldcal fluxes are same as expected patchcal ones
# so we can only worry about interpolation of extinctions.
######################################################################
proc fpp_with_partial_match_and_samedate_oldcal {} {
   global starsep maxerr checkerrs
   
   set result 0

   makeStarlists starlist
   chainDestroy $starlist(r) star1pcDel;
   set starlist(r) [chainNew STAR1PC]
   set patches [makepatches]
   set extinctions [make3extinctions]
   makeframestuff frameinfos calib1byframes
   set oldcal [calib1bytimeInit 2 100.0 {g r} {50 500} {10 1}]
   set calib1bytimes [chainNew CALIB1BYTIME]
   set sys_ph_err 0.001
   set n_max 0
   loop i 0 2 { 
      set flux20_m($i) 0
      set flux20_c($i) 0
      set flux20_chi2($i) 0
   }
   
   set outstr \
       [findPhotoParams {g r} {r g} {0. 0.} {0. 0.} starlist $patches \
	    $extinctions $frameinfos $oldcal $starsep $sys_ph_err 0.05 $n_max \
	    flux20_m flux20_c flux20_chi2 $calib1bytimes $calib1byframes]

   
   # check outstr
   set framecal [keylget outstr framecal]
   set patchcal [keylget outstr patchcal]
   if { ($patchcal != "g") || $framecal != "g"} {
      echo "fpp_with_partial_match_and_samedate_oldcal: $outstr"
      set result 1
   }

   # check that patchcal contains calib for g only
   set tcal [chainElementGetByPos $calib1bytimes 0]
   set cal0 [handleBindFromHandle [handleNew] *$tcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$tcal.calib<1>]

   incr result [checkflux $cal0 50 0.69 \
		    fpp_with_partial_match_and_samedate_oldcal patchcal(g)]

   incr result [checkflux $cal1 -100 -100 \
		    fpp_with_partial_match_and_samedate_oldcal patchcal(r)]
   handleDel $cal1

   # check that framecals contain patchcal info in g and oldcal in r

   # zeroth frame had airmass 0, so values should be same as patchcal
   set fcal [chainElementGetByPos $calib1byframes 0]
   set cal0 [handleBindFromHandle [handleNew] *$fcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$fcal.calib<1>]
   
   incr result [checkflux $cal0 50 0.69 \
		   fpp_with_partial_match_and_samedate_oldcal framecal(0,g)]
   handleDel $cal0

   incr result [checkflux $cal1 500 1 \
		    fpp_with_partial_match_and_samedate_oldcal framecal(0,r)]
   handleDel $cal1
   

   # cleanup 
   chainDestroy $starlist(g) star1pcDel; chainDel $starlist(r)
   patchesDestroy $patches; chainDestroy $extinctions extinctionDel
   chainDestroy $calib1byframes calib1byframeDel
   chainDestroy $calib1bytimes calib1bytimeDel
   chainDestroy $frameinfos frameinfoDel
   calib1bytimeDel $oldcal

   return $result
}
######################################################################
# Test with a patch that matches stars in both bands. 
# There is no extinction in g.
######################################################################
proc fpp_with_bad_extinction {} {
   global starsep maxerr checkerrs
   
   set result 0

   makeStarlists starlist
   set patches [makepatches]

   set extinctions [chainNew EXTINCTION]
   set ext [extinctionInit 2 100.0 {g r} {0. 0.} {-100. 0.001}] 
   chainElementAddByPos $extinctions $ext TAIL AFTER
   handleDel $ext

   makeframestuff frameinfos calib1byframes
   set oldoldcal [calib1bytimeInit 2 99.0 {g r} {500 50} {10 1}]
   set calib1bytimes [chainNew CALIB1BYTIME]
   set sys_ph_err 0.001
   set n_max 0
   loop i 0 2 { 
      set flux20_m($i) 0
      set flux20_c($i) 0
      set flux20_chi2($i) 0
   }

   set outstr \
       [findPhotoParams {g r} {r g} {0. 0.} {0. 0.} starlist $patches \
	    $extinctions $frameinfos $oldoldcal $starsep $sys_ph_err 0.05 $n_max \
	    flux20_m flux20_c flux20_chi2 $calib1bytimes $calib1byframes]

   
   # check outstr
   set patchcal [keylget outstr patchcal]
   set framecal [keylget outstr framecal]
   if { ($patchcal != "r") || $framecal != "r"} {
      echo "fpp_with_bad_extinction: (oldoldcal) $outstr"
      set result 1
   }

   # check patchcal values
   set tcal [chainElementGetByPos $calib1bytimes 0]
   set cal0 [handleBindFromHandle [handleNew] *$tcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$tcal.calib<1>]

   incr result [checkflux $cal0 -100 -100 fpp_with_bad_extinction patchcal(g)]
   handleDel $cal0

   incr result [checkflux $cal1 500 5.34 fpp_with_bad_extinction patchcal(r)]
   handleDel $cal1

   # check framecal values

   # zeroth frame had airmass 0, so values should be same as patchcal in r
   # g should have error values (-100)

   set fcal [chainElementGetByPos $calib1byframes 0]
   set cal0 [handleBindFromHandle [handleNew] *$fcal.calib<0>]
   set cal1 [handleBindFromHandle [handleNew] *$fcal.calib<1>]
   
   incr result [checkflux $cal0 -100 -100 fpp_with_bad_extinction framecal(0,g)]
   handleDel $cal0

   incr result [checkflux $cal1 500 5.34 fpp_with_bad_extinction framecal(0,r)]
   handleDel $cal1

   # cleanup 
   chainDestroy $starlist(g) star1pcDel; chainDel $starlist(r)
   patchesDestroy $patches; chainDestroy $extinctions extinctionDel
   chainDestroy $calib1byframes calib1byframeDel
   chainDestroy $calib1bytimes calib1bytimeDel
   chainDestroy $frameinfos frameinfoDel
   calib1bytimeDel $oldoldcal

   return $result
}

######################################################################
# Run tests
set failed 0
 
set tests {nopatches matching_patch partial_match_and_old_oldcal \
           partial_match_and_samedate_oldcal bad_extinction}
 
foreach test $tests {
  echo "Testing fpp_with_$test"
  if {[fpp_with_$test] > 0} {
     echo "TEST-ERR: fpp_with_$test fails"
     set failed 1
  } else {
     echo "  OK."
  } 
}

if $failed {
   error "Failed findPhotoParams"
}
