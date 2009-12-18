##############################################################################
#<AUTO>
#
# FILE: telescopeAxes.tcl
#
#<HTML>
# Calculate and plot the telescope axis rates required for a scan.
#</HTML>
#
#</AUTO>
##############################################################################
##############################################################################
#<AUTO EXTRACT>
#
# TCL VERB: stripeAxisRates
#
# <HTML>
#    Calculates and plots the telescope axis rates which will be encountered
#       during the course of a scan, given a stripe number and a starting
#       time.
#    Required arguments:
#       stripe  (integer 1 -- 45, or a SEGUE stripe number 1000 <= stripe <= 1720)
#       utStart (string HH:MM)
#    Optional arguments, switches:
#       -mjd         (integer day, defaults to today)
#       -startLambda (double, defaults to lambdaMin for the stripe)
#       -endLambda   (double, defaults to lambdaMax for the stripe)
#	-startMu     (double, defaults to muMin for the Segue stripe)
#	-endMu       (double, defaults to muMax for the Segue stripe)
#       -file        (string, the name of a Postscript file for the
#                     plot. If not present, plots to XWIN)
#       -polar       (No argument. If this switch is present, the plot will
#                     be a polar plot of the telescope track, with the zenith
#                     at the pole, otherwise the default plot is a Cartesian
#                     plot of the three telescope axis rates versus lambda or mu 
#                     longitude)
# </HTML>
#
#</AUTO>
##############################################################################
set stripeAxisRatesArgs {
   {stripeAxisRates "Predict telescope axis rates for a scan.\n"}
   {<stripe>      INTEGER  ""      stripe      "Survey stripe number"}
   {<utStart>     STRING   ""      utStart     "UT at start of scan (HH:MM)"}
   {-mjd          INTEGER  "0"     mjdStart    "Integer MJD at start of scan (needed only if not today)"}
   {-startLambda  DOUBLE   "-999"  lambdaStart "Beginning Survey Longitude (stripe lambdaMin by default)"}
   {-endLambda    DOUBLE   "-999"  lambdaEnd   "Ending Survey Longitude (stripe lambdaMax by default)"}
   {-startMu      DOUBLE   "-999"  muStart     "Beginning SEGUE GC mu (SEGUE stripe muMin by default)"}
   {-endMu        DOUBLE   "-999"  muEnd       "Ending SEGUE GC mu (SEGUE stripe muMax by default)"}
   {-file         STRING   ""      pgfile    "Write plot to this Postscript file, rather than /XWIN"}
   {-polar        CONSTANT 0       polar       "Display polar plot of Telescope Az/Alt track vice \n    Cartesian plot of Az/Alt/Rotator rates vs lambda."}
}
ftclHelpDefine astromProcs stripeAxisRates [shTclGetHelpInfo stripeAxisRates \
   $stripeAxisRatesArgs]

proc stripeAxisRates { args } {
   global at_deg2Rad
   upvar #0 stripeAxisRatesArgs formal_list

   # Parse arguments.
   if {[shTclParseArg $args $formal_list stripeAxisRates] == 0} {return}
   set polar [ftclPassed $args $formal_list -polar]

# this procedure only handles northern survey stripes
   #if {$stripe < 1 || $stripe > 45} {
      #echo "Sorry, this procedure only valid for stripes 1 --> 45"
      #return
   #}
# demand HH:MM for ut
   set colon [string first ":" $utStart]
   if {$colon == -1 } {
     echo "UT must be in the form HH:MM"
     return
   }
# number of seconds per frame:
   set ct 26322
   set secsPerFrame [expr 1361.*($ct/1.e6)]
# get survey limits for this stripe
   set strlim [stripeLimits $stripe]
	if {$stripe < 100} {
   set eta [keylget strlim eta]
   if {$lambdaStart == -999} {
      set lambdaStart [keylget strlim lambdaMin]
   }
   if {$lambdaEnd == -999} {
      set lambdaEnd [keylget strlim lambdaMax]
   }

	if {$lambdaEnd < $lambdaStart} {
		set lambdaEnd [expr $lambdaEnd+360]
	} 
		echo lambdaStart $lambdaStart lambdaend $lambdaEnd

   set lambda $lambdaStart
   set deltaLambda [expr $secsPerFrame/240.]
		echo $lambda $lambdaEnd
	} else {
	set nu [keylget strlim nu]
   if {$muStart == -999} {
      set muStart [keylget strlim muMin]
   }
   if {$muEnd == -999} {
      set muEnd [keylget strlim muMax]
   }
	if {$muEnd < $muStart} {
		set muEnd [expr $muEnd+360]
	} 
		echo mu startEnd $muStart $muEnd

   set mu $muStart
   set deltaMu [expr $secsPerFrame/240.]

	}

# get initial timeStamp
   set uthours [string range $utStart 0 [expr $colon-1]]
   set utminutes [string range $utStart [expr $colon+1] [string length $utStart]]
   if {[string range $uthours 0 0] == "0" && [string length $uthours] > 1} {
      set uthours [string range $uthours 1 1]
   }
   if {[string range $utminutes 0 0] == "0" && [string length $utminutes] > 1} {
      set utminutes [string range $utminutes 1 1]
   }
   set ut [expr $uthours+$utminutes/60.]
   if {$mjdStart == 0 } { 
      set mjdStart [expr int([tstampToMJD [tstampNow]])]
   }
   set ts [tstampFromMJD [expr $mjdStart+$ut/24.]]
# and initial hour angle
   set lst [tstampToLst $ts]
	if {$stripe < 100} {
   set eqcoords [surveyToEq $lambdaStart $eta]
	} else {
	set node [nodeFromStripeNumber $stripe]
	set incl [inclFromStripeNumber $stripe]
	set eqcoords [GCToEq $mu $nu -node $node -incl $incl]
	}
   set ra [keylget eqcoords ra]
   set dec [keylget eqcoords dec]
   set ha [expr ($lst-$ra)/15.]
   set ha [format %5.1f $ha]
#echo Beginning RA $ra, LST $lst, HA $ha

# flag for first time through
   set first 1

#initialize tcl lists
   set azimuths {}
   set altitudes {}
   set rotations {}
   set azrates {}
   set altrates {}
   set parates {}
   set lambdas {}
   set mus {}

#loop through the pseudo-frames til exceed lambdaEnd or muEnd
   while  1  {
	set le [eqToSurvey $ra $dec]
	set lambda [keylget le lambda]
	if {$lambda < -90 } {
		set lambda [expr $lambda + 360]
	}
	if {[expr abs($lambda-90) < 2] || [expr abs($lambda-270) < 2]} {
		echo lambda hit pole
		break
	}
	set eta [keylget le eta]
      set azelpa [surveyToAzelpa $lambda $eta $ts]
      set alt [keylget azelpa el]
# convert az/pa to 2.5m convention:
      set az [expr 180.-[keylget azelpa az]]
      if {$az < 0.} {set  az [expr $az+360.]}
      set pa [keylget azelpa pa]
      if {$pa < 0.} {set pa [expr $pa+360.]}
# the first rate value is meaningless, just set last values and skip to next
      if {$first} {
         set lastaz $az
         set lastalt $alt
         set lastpa $pa
# increment stuff for next pseudo-frame
         tstampIncr $ts $secsPerFrame
		if {$stripe < 100} {
         	set lambda [expr $lambda+$deltaLambda]
		set radec [surveyToEq $lambda $eta]
		set ra [keylget radec ra]
		set dec [keylget radec dec]
		} else {
		set mu [expr $mu+$deltaMu]
		set radec [GCToEq $mu $nu -node $node -incl $incl]
		set ra [keylget radec ra]
		set dec [keylget radec dec]
		}
# no longer the "first" time through
         set first 0
         continue
      }

#kludge wrap crossings by using previous rate values (assume a wrap crossing
#   if delta values exceed 90 degrees)
      if {[expr abs($az-$lastaz) < 90.]} {
         set azrate [expr 3600.*($az-$lastaz)/$secsPerFrame]
      }
      if {[expr abs($alt-$lastalt) < 90.]} {
         set altrate [expr 3600.*($alt-$lastalt)/$secsPerFrame]
      }
      if {[expr abs($pa-$lastpa) < 90.]} {
         set parate [expr 3600.*($pa-$lastpa)/$secsPerFrame]
      }
# print out values for debugging
#      set st [format "%13.8f  %7.3f  %7.3f  %8.3f  %7.3f  %7.3f  %7.3f" \
#         $lambda $az $alt $pa $azrate $altrate $parate]
#      echo $st

# add new values to tcl lists
      append azimuths $az " "
      append altitudes $alt " "
      append rotations $pa " "
	if {$stripe < 100} {
      append lambdas $lambda  " "
	} else {
	append mus $mu " "
	}	
      append azrates $azrate " "
      append altrates $altrate " "
      append parates $parate " "
		#echo $parate
# are we done yet?
	if {$stripe < 100} {
           if {$lambda > $lambdaEnd} break
	   if {$lambdaEnd > 270 && $lambda < 0} break
	} else {
	   if {$mu > $muEnd} break
	}

# if not, set-up for next pseudo-frame
      set lastaz $az
      set lastalt $alt
      set lastpa $pa
      tstampIncr $ts $secsPerFrame
	
		if {$stripe < 100} {
         set lambda [expr $lambda+$deltaLambda]
		#echo delta $deltaLambda ($lambda)
		set radec [surveyToEq $lambda $eta]
		set ra [keylget radec ra]
		set dec [keylget radec dec]
		#echo $lambda $eta $ra $dec
		} else {
		set mu [expr $mu+$deltaMu]
		set radec [GCToEq $mu $nu -node $node -incl $incl]
		set ra [keylget radec ra]
		set dec [keylget radec dec]
		#echo mu nu $mu $nu ra dec $ra $dec
		}
   }
#final ha
   set lst [tstampToLst $ts]
	if {$stripe < 100} {
   		set eqcoords [surveyToEq $lambdaEnd $eta]
	} else {
		set eqcoords [GCToEq $muEnd $nu -node $node -incl $incl]
	}
   set ra [keylget eqcoords ra]
   set haend [expr ($lst-$ra)/15.]
   set haend [format %5.1f $haend]
#echo Ending RA $ra, LST $lst, HA $haend

# convert tcl lists to vectors
   set vaz [vFromL $azimuths]
   set valt [vFromL $altitudes]
   set vrot [vFromL $rotations]
	if {$stripe < 100 } {
   set vlambdas [vFromL $lambdas]
	} else {
   set vmus [vFromL $mus]
	}
   set vazrates [vFromL $azrates]
   set valtrates [vFromL $altrates]
   set vparates [vFromL $parates]

   set altratemin [vExtreme $valtrates min]
   set altratemax [vExtreme $valtrates max]
   set azratemin [vExtreme $vazrates min]
   set azratemax [vExtreme $vazrates max]
   set paratemin [vExtreme $vparates min]
   set paratemax [vExtreme $vparates max]
# print out highest rates
   echo [format "Min/Max Rates: Altitude %6.1f --> %6.1f" $altratemin $altratemax]
   echo [format "(arcsec/tsec)  Azimuth  %6.1f --> %6.1f" $azratemin $azratemax]
   echo [format "               Rotator  %6.1f --> %6.1f" $paratemin $paratemax]

# plot
   if {$polar} {
      set pgs [pgstateNew]
      if {$pgfile == ""} {set device /XWIN} else {set device /PS}
      pgstateSet $pgs -device $device -just 1 -axis -2 -xopt "" -yopt "" -symb 1
#   pgVport .2 .8 .2 .8
      pgstateOpen $pgs
      set vsize [exprGet $vaz.dimen]
      set zd [vectorExprNew $vsize]
      set xx [vectorExprNew $vsize]
      set yy [vectorExprNew $vsize]
      set azrad [vectorExprNew $vsize]
      exprEval "$zd = 1. - $valt/90.;
                $xx = $zd*cos($at_deg2Rad*$vaz);
                $yy = $zd*sin($at_deg2Rad*$vaz)"

      vPlot $pgs $xx $yy -xmin -.75 -xmax .75 -ymin -.75 -ymax .75
   
# label start (S) and end (E) of track
#      pgText [exprGet $xx.vec<0>] [exprGet $yy.vec<0>] S
#      pgText [exprGet $xx.vec<[expr $vsize-1]>] [exprGet $yy.vec<[expr $vsize-1]>] E

# draw a circle at elevation of 30. degrees for orientation
      exprEval "$zd = 1.-30./90.;
                $vaz@0 = 361.*$$/$vsize; 
                $xx = $zd*cos($at_deg2Rad*$vaz);
                $yy = $zd*sin($at_deg2Rad*$vaz)"
      pgstateSet $pgs -isNewplot 0 -symb 1 -isLine 1
      vPlot $pgs $xx $yy -xmin -.75 -xmax .75 -ymin -.75 -ymax .75

# and another at elevation of 85. degrees 
      exprEval "$zd = 1.-85./90.;
                $vaz@0 = 361.*$$/$vsize; 
                $xx = $zd*cos($at_deg2Rad*$vaz);
                $yy = $zd*sin($at_deg2Rad*$vaz)"
      pgstateSet $pgs -isNewplot 0 -symb 1 -isLine 1
      vPlot $pgs $xx $yy -xmin -.75 -xmax .75 -ymin -.75 -ymax .75
      pgText -1.2 -1.1 "Telescope Track, Stripe $stripe, Begin/End HA: $ha/$haend (decimal hours) "
      pgText -.9 -1.2 "Inner/Outer circles at Elevations of 85/30 degrees"

# Label cardinal points
      pgMove .6667 0.
      pgDraw .72 0.
      pgText  .75 -.025 South
      pgMove -.72 0.
      pgDraw -.6667 0.
      pgText -.95 -.025 North
      pgMove 0. .6667
      pgDraw 0. .72
      pgText -.08 .75 East
      pgMove 0. -.6667
      pgDraw 0. -.72
      pgText -.08 -.80 West

# If a plot file, rename it
      if {$pgfile != ""} {
         frename pgplot.ps $pgfile
         pgEnd
      }
      pgstateDel $pgs
      vectorExprDel $vazrates 
      vectorExprDel $valtrates 
      vectorExprDel $vparates 
	if {$stripe < 100} {
      vectorExprDel $vlambdas
	} else {
      vectorExprDel $vmus
	}
      vectorExprDel $zd
      vectorExprDel $xx
      vectorExprDel $yy
      vectorExprDel $azrad
      tstampDel $ts
      return
   }

   set pgs [pgstateNew]
   if {$pgfile == ""} {set device /XWIN} else {set device /PS}
   pgstateSet $pgs -device $device -nxwindow 1 -nywindow 4 -yfract 0  -xopt BCTS -yopt NBCTS -symb 1
   pgstateOpen $pgs

	if {$stripe < 100} {
   vPlot $pgs $vlambdas $vazrates
	} else {
   vPlot $pgs $vmus $vazrates
	}
   pgLabel " " "Azimuth Rate"
		if {$stripe < 100 } {
   set x [expr $lambdaStart - 0.21*($lambdaEnd-$lambdaStart)]
		} else {
   set x [expr $muStart - 0.21*($muEnd-$muStart)]
		}
   set y [expr $azratemin - 0.75*($azratemax-$azratemin)]
		if {$stripe < 100 } {
   if {$deltaLambda < 0} {set y [expr -1.*$y]}
		} else {
   if {$deltaMu < 0} {set y [expr -1.*$y]}
		}
   pgPtext $x $y 90. 0.5 "(Rates are arcsec/timesec)"
		if {$stripe < 100} {
   vPlot $pgs $vlambdas $vparates
		} else {
   vPlot $pgs $vmus $vparates
	}
   pgLabel " " "Rotator Rate"
   pgstateSet $pgs -xopt NBCTS
		if {$stripe < 100 } {
   vPlot $pgs $vlambdas $valtrates
   pgLabel "Lambda (for Stripe $stripe; Eta $eta)" "Altitude Rate" " "
   set x [expr $lambdaStart+.1*($lambdaEnd-$lambdaStart)]
		} else {
   vPlot $pgs $vmus $valtrates
   pgLabel "Mu (for Stripe $stripe; Incl: $incl)" "Altitude Rate" " "
   set x [expr $muStart+.1*($muEnd-$muStart)]
		}
   set y [expr $altratemin-.75*($altratemax-$altratemin)]
   pgPtext $x $y 0. 0. "Beginning/Ending Hour Angles (Decimal Hours): $ha, $haend"

   vectorExprDel $vazrates 
   vectorExprDel $valtrates 
   vectorExprDel $vparates 
	if {$stripe < 100} {
   vectorExprDel $vlambdas
		} else {
   vectorExprDel $vmus
	}
   vectorExprDel $vaz
   vectorExprDel $valt
   vectorExprDel $vrot

   tstampDel $ts
# If a plot file, rename it
   if {$pgfile != ""} {
      frename pgplot.ps $pgfile
      pgEnd
   }
   pgstateDel $pgs
   
   return 
}
