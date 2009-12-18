#***************
#
#  idStamp
#
#***************

ftclHelpDefine astrotools idStamp [shTclGetHelpInfo idStamp { { idStamp "returns a unique timestamp-pid string \n used to stamp pipeline outputs for OPDB" } }]


proc idStamp {} {

return "[fmtclock [getclock] %Y-%m-%dT%H:%M:%S] [format %05d [pid]]"

}

#*************
#
# astrotoolsVersion
#
#*************

proc astrotoolsVersion {} {
   set name "\$Name: v5_24 $";
   
   if {[regexp {Name: ([^\* ]+) \$$} $name foo name]} {
      return $name
   } else {
      return "NOCVS:[versionStamp ASTROTOOLS_DIR]"
   }
}

#
# get a version from the product's directory name; to be used only if
# the version name from cvs version fails.
#
ftclHelpDefine astrotools versionStamp \
    [shTclGetHelpInfo versionStamp \
	 [list \
	       [list versionStamp \
[format "%s%s" "returns a guess at the this pipeline's product version e.g.
   versionStamp ASTROTOOLS_DIR

Please don't use this directly; rather include a proc such as
the following in your pipeline:

proc astrotoolsVersion {} {
   set name \"\\\$" "Name:$\";

   if {\[regexp {Name: (\[^\\\* \]+) \\\$\$} \$name foo name\]} {
      return \$name
   } else {
      return \"NOCVS:\[versionStamp ASTROTOOLS_DIR\]\"
   }
}
"]] \
		{ <dir> STRING "" dir "directory name i.e. <PRODUCT>_DIR"} \
	      ]\
]

proc versionStamp {dir} {
   global env

   if [info exists env($dir)] {
	set pa [file tail $env($dir)]
	set pb  [file tail [file dir $env($dir)]]
	if { [regexp {Linux|OSF1|IRIX|NULL|ANY} $pa] } {
        	set version $pb
	} else {
        	set version $pa
	}
      return $version
   } else {
      return "unknown"
   }
}

#***********************************************************************
#
# stripeNumberFromEta
#
#***********************************************************************

set stripeNumberFromEtaArgs {
  { stripeNumberFromEta "returns the stripe number closes to the eta value" }
  { <eta> STRING "" eta "eta" }
}

ftclHelpDefine astrotools stripeNumberFromEta [shTclGetHelpInfo stripeNumberFromEta $stripeNumberFromEtaArgs]

proc stripeNumberFromEta { args } {
#
# access help and parse inputs
#
    upvar #0 stripeNumberFromEtaArgs formal_list

    if { [shTclParseArg $args $formal_list stripeNumberFromEta] == 0 } { return }
#
# find the stripe number
#
    global at_stripeSeparation
# better be 2.5 degrees separation...
    set stripeNumber [expr int( ($eta + 58.75)/$at_stripeSeparation )]
    return $stripeNumber
}

#***********************************************************************
#
# etaFromStripeNumber
#
#***********************************************************************

set etaFromStripeNumberArgs {
  { etaFromStripeNumber "returns the eta of the stripe number" }
  { <stripeNumber> STRING "" stripeNumber "stripe number" }
}

ftclHelpDefine astrotools etaFromStripeNumber [shTclGetHelpInfo etaFromStripeNumber $etaFromStripeNumberArgs]

proc etaFromStripeNumber { args } {
#
# access help and parse inputs
#
    upvar #0 etaFromStripeNumberArgs formal_list

    if { [shTclParseArg $args $formal_list etaFromStripeNumber] == 0 } { return }
#
# find the eta
#
    global at_stripeSeparation
	if { $stripeNumber <= 46 } {
    set eta [expr $stripeNumber*$at_stripeSeparation - 57.5 ]
	} else {
    set eta [expr $stripeNumber*$at_stripeSeparation - 57.5 - 180.0 ]
	}
    return $eta
}

#***********************************************************************
#
# inclFromStripeNumber
#
#***********************************************************************

set inclFromStripeNumberArgs {
  { inclFromStripeNumber "returns the inclination of the stripe number" }
  { <stripeNumber> STRING "" stripeNumber "the stripe number" }
}

ftclHelpDefine astrotools inclFromStripeNumber [shTclGetHelpInfo inclFromStripeNumber $inclFromStripeNumberArgs]

proc inclFromStripeNumber { args } {
#
# access help and parse inputs
#
    upvar #0 inclFromStripeNumberArgs formal_list

    if { [shTclParseArg $args $formal_list inclFromStripeNumber] == 0 } { return }
#
# find the inclination
#
	if {$stripeNumber <= 100 } {
    global at_surveyCenterDec
    set incl [expr [etaFromStripeNumber $stripeNumber] + $at_surveyCenterDec]
	} else {
	set incl [GCinclFromStripeNumber $stripeNumber]		
	}
    return $incl
}

#***********************************************************************
#
# nodeGet
#
#***********************************************************************

set nodeGetArgs {
  { nodeGet "returns the node of all great circles, which is always 95 degrees\n\ntakes no command line arguments" }
}

ftclHelpDefine astrotools nodeGet [shTclGetHelpInfo nodeGet $nodeGetArgs ]

proc nodeGet { args } {
#
# access help and parse inputs
#
    upvar #0 nodeGetArgs formal_list

    if { [shTclParseArg $args $formal_list nodeGet] == 0} { return }
#
# find the node
#
    global at_surveyCenterRa
    set node [expr $at_surveyCenterRa-90.0]
    return $node
}

#***********************************************************************
#
# GCTheta
#
#***********************************************************************

set GCThetaArgs {
  { GCTheta "returns the angle in degrees between the increasing mu great circle \n at a given mu and increasing RA " }
  { <mu> STRING "" mu "Great Circle mu (degrees)" }
  { <incl>       STRING "" incl       "stripe inclination (degrees)" }
  { [node]    STRING "95" node    "node of great circle (degrees)" }
	}


ftclHelpDefine astrotools GCTheta [shTclGetHelpInfo GCTheta $GCThetaArgs ]

proc GCTheta { args } {
#
# access help and parse inputs
#
	global at_rad2Deg at_deg2Rad

    upvar #0 GCThetaArgs formal_list

    if { [shTclParseArg $args $formal_list GCTheta] == 0} { return }

	set muprime [expr $mu-$node]
   set theta [expr $at_rad2Deg*atan2(cos($muprime*$at_deg2Rad)*sin($incl*$at_deg2Rad),cos($incl*$at_deg2Rad))]
	return $theta

}
#
#***********************************************************************
#
# mtPatchNameFromSurvey
#
#***********************************************************************

set mtPatchNameFromSurveyArgs {
  { mtPatchNameFromSurvey "returns the name of the MT patch for given survey coordinates and camera column" }
  { <lambda> STRING "" lambda "lambda" }
  { <eta>    STRING "" eta    "eta" }
  { <camCol> STRING "" camCol "camera column" }
} 

ftclHelpDefine astrotools mtPatchNameFromSurvey [ shTclGetHelpInfo mtPatchNameFromSurvey $mtPatchNameFromSurveyArgs ]

proc mtPatchNameFromSurvey { args } {
#
# access help and parse inputs
#
    upvar #0 mtPatchNameFromSurveyArgs formal_list

    if { [shTclParseArg $args $formal_list mtPatchNameFromSurvey] == 0 } { return }
#
# find the patch name
#
    set deg [expr 5*int((95.0+$lambda)/5.0)]
    set in [stripeNumberFromEta $eta]
    set name [format mtPatch-%03d-%02d-%1d $deg $in $camCol]
    return $name
}

#***********************************************************************
#
# surveyFromMtPatchName
#
#***********************************************************************

set surveyFromMtPatchNameArgs {
  { surveyFromMtPatchName "returns keyed list with the survey coodinates for the MT patch name" }
  { <name> STRING "" name "MT Patch name" }
}

ftclHelpDefine astrotools surveyFromMtPatchName [shTclGetHelpInfo surveyFromMtPatchName $surveyFromMtPatchNameArgs]

proc surveyFromMtPatchName { args } {
#
# access help and parse inputs
#
    upvar #0 surveyFromMtPatchNameArgs formal_list

    if { [shTclParseArg $args $formal_list surveyFromMtPatchName] == 0 } { return }
#
# find the survey coordinates
#    
    set a [split $name "-"]
    set deg [lindex $a 1]
    set in [lindex $a 2]
    set c [lindex $a 3]
    regsub {^0*} $deg {} deg
    if {$deg == "" } { set deg 0 }
    regsub {^0*} $in {} in
    if { $in == "" } { set in 0 }
    set eta [etaFromStripeNumber $in]
    set lambda [expr $deg - 95 ]
    set out "{lambda $lambda} {eta $eta}"
    return $out
}

#***********************************************************************
#
# lookupCCDAstrom
#
#***********************************************************************

set lookupCCDAstromArgs {
  { lookupCCDAstrom "chainList and hrd are set with the command\n
\t'set chainList \[param2Chain opCamera-MJD.par hdr\]'\n
camRow and camCol are the row and column location of the CCD
xrot, yrot is the center of the plate rotation (nominally 0.0, 0.0)
rotation is the rotation of the plate (near 0 degrees)
returns a keyed list of\n
\tcamRow - the same as you input
\tcamCol - the same as you input
\tdewarID - which dewar this CCD is in
\txPos, yPos - the effective position in the focal plane (mm)
\tangle - the effecive rotation of the chip (degrees)
\tarcsecPerPix - the local plate scale at this CCD\n
You need to provide a global plate scale to transform xPos, yPos to
a location on the sky.\n
An example is in astrotools/test/atSurveyGeom.tcl\n " }
  { <chainList> STRING "" chainList "chain list" }
  { <hdr>       STRING "" hdr       "header from opCamera-MJD.par" }
  { <camRow>    STRING "" camRow    "row of the CCD" }
  { <camCol>    STRING "" camCol    "column of the CCD" }
  { [xrot]      STRING "0" xrot     "x coordinate for plate rotation" }
  { [yrot]      STRING "0" yrot     "y coordinate for plate rotation" }
  { [rotation]  STRING "0" rotation "plate rotation angle" }
}
  
ftclHelpDefine astrotools lookupCCDAstrom [shTclGetHelpInfo lookupCCDAstrom $lookupCCDAstromArgs]

proc lookupCCDAstrom { args } {
#
# access help and parse inputs
#
    upvar #0 lookupCCDAstromArgs formal_list

    if { [shTclParseArg $args $formal_list lookupCCDAstrom] == 0 } { return }
#
# 
    global at_deg2Rad
    loop i 0 [llength $chainList] {
	set typelist [chainTypeGet [lindex $chainList $i]]
	if {[keylget typelist type] == "CCDGEOMETRY"} {
	    set ccdchn [lindex $chainList $i]
	    set ccdsize [chainSize $ccdchn]
	} elseif {[keylget typelist type] == "DEWARGEOMETRY"} {
	    set dewarchn [lindex $chainList $i]
	    set dewarsize [chainSize $dewarchn]
	}
    }

    set good "none"
    loop i 0 $ccdsize {
	set cur [chainElementGetByPos $ccdchn $i]
	set row [exprGet $cur.camRow]
	set col [exprGet $cur.camCol]
	if { $row == $camRow && $col == $camCol } {
	    foreach p "dewarID camRow camCol rowRef colRef xc yc theta sfactc pscale" {
		set $p [exprGet $cur.$p]
	    }
	    set good $cur
	    handleDel $cur
	    break
	}
	handleDel $cur
    }
    if { $good == "none" } {
	echo "no such chip found"
	return -1
    }
    
    set gooddewar "none"
    loop i 0 $dewarsize {
	set cur [chainElementGetByPos $dewarchn $i]
	if {$dewarID == [exprGet $cur.dewarID]} {
	    foreach p "xb yb thetai sfacti" {
		set $p [exprGet $cur.$p]
	    }
	    set gooddewar $cur
	    handleDel $cur
	    break
	} 
	handleDel $cur
    }
    if { $gooddewar == "none" } {
	echo "no dewar found $dewarID"
	return -1
    }
    
    # locate the chip in the camera
    set radians [expr $thetai*$at_deg2Rad]
    set ct [cos $radians]
    set st [sin $radians]
    set xPosInCamera [expr $xb + $sfacti*($ct*$xc - $st*$yc)]
    set yPosInCamera [expr $yb + $sfacti*($st*$xc + $ct*$yc)]

    # rotate the whole camera
    set radians [expr $rotation*$at_deg2Rad]
    set ct [cos $radians]
    set st [sin $radians]
    set xPos [expr \
	    +$ct*($xPosInCamera - $xrot) + $st*($yPosInCamera - $yrot) + $xrot]
    set yPos [expr \
	    -$st*($xPosInCamera - $xrot) + $ct*($yPosInCamera - $yrot) + $yrot]

    set angle [expr $thetai + $theta + $rotation]
    set scalef [keylget hdr scalef]
    set arcsecPerPixel [expr $scalef * $pscale * $sfactc]
    set out ""
    foreach p "camRow camCol dewarID rowRef colRef xPos yPos angle arcsecPerPixel" {
	keylset out $p [set $p]
    }
    
    return $out
}

#***********************************************************************
#
# nuFromScanLine
#
#***********************************************************************

set nuFromScanLineArgs {
  { nuFromScanLine "returns nu, given a param file of type opCamera,\nthe column (1-6) and either N or S (north or south)" }
  { <paramfile> STRING "" paramfile "the param file" }
  { <camCol>    STRING "" camCol    "camera column" }
  { <strip>     STRING "" strip     "strip, either N or S" }
} 

ftclHelpDefine astrotools nuFromScanLine [shTclGetHelpInfo nuFromScanLine $nuFromScanLineArgs] 

proc nuFromScanLine { args } {

#
# access help and parse inputs
#
  upvar #0 nuFromScanLineArgs formal_list

  if { [shTclParseArg $args $formal_list nuFromScanLine] == 0 } { return }
# 
  global at_asec2Deg

  set chainList [param2Chain $paramfile hhh]
  set ccdinfo  [lookupCCDAstrom $chainList $hhh 1 $camCol]
  set xPos [keylget ccdinfo xPos]
  if (![string compare $strip N]) {set offset [keylget hhh xBoreN]
  } else {set offset [keylget hhh xBoreS]}
  set totaloffset [expr $xPos-$offset]
  set scale [keylget hhh scalef]
  set degoffset [expr $totaloffset*$scale*$at_asec2Deg]
  foreach chain $chainList {genericChainDel $chain}

  return $degoffset

}

#***********************************************************************
#
# loadStripeLimits
#
#***********************************************************************

set loadStripeLimitsArgs {
  { loadStripeLimits "returns chain of stipe start and stop lambdasfor use with\n\tstripeLimits <stripeNumber> <StripeLimitsChain>" }
  { -extended CONSTANT "1" extended "load the so-called \"extended\" stripes 46-99, useful in the Southern Survey." }
}

ftclHelpDefine astrotools loadStripeLimits [shTclGetHelpInfo loadStripeLimits $loadStripeLimitsArgs]

proc loadStripeLimits { args } {
#
# acesss help and parse inputs
#
        upvar #0 loadStripeLimitsArgs formal_list

        if { [shTclParseArg $args $formal_list loadStripeLimits] == 0 } { return }
        if {![info exists extended]} { set extended 0 }

#
# create chain of stripe limits
#
	set chn [param2Chain [envscan \$ASTROTOOLS_DIR/etc/atStripeDef.par] hhh]
	if {$extended} { \
		chainJoin $chn [param2Chain [envscan \$ASTROTOOLS_DIR/etc/atExtendedStripeDef.par] hhh]	
	}
	return $chn
}

#***********************************************************************
#
# stripeLimits
#
#***********************************************************************

set stripeLimitsArgs {
  { stripeLimits "returns keyed list with lambdaMin, lambdaMax, and eta for the indicated stipe.\nIf stripeChn (loaded with loadStripeLimits) is specified it works faster, if no stripeChn is specified it loads and deletes the stripeChain each time." }
  { <stripenumber> STRING "" stripenumber "stripe number" }
  { [stripeChn]    STRING "X" stripeChn "stripe chain" }
}

ftclHelpDefine astrotools stripeLimits [shTclGetHelpInfo stripeLimits $stripeLimitsArgs]

proc stripeLimits { args } {
#
# access help and parse inputs
#
        upvar #0 stripeLimitsArgs formal_list

        if { [shTclParseArg $args $formal_list stripeLimits] == 0 } { return }
#
# find the stripe limits
#
	if {$stripenumber > 100} {
		set gclim [GCStripeLimits $stripenumber]
		return $gclim
	}
	set delchn 0
	if { $stripeChn == "X"} {
	set delchn 1
	set stripeChn [loadStripeLimits -extended]
	}
	set one [chainSearch $stripeChn "{stripeNumber == $stripenumber}"]
	if {[chainSize $one] == 0 } {
		return "no such stripenumber $stripenumber in chn"
	}
	set cur [chainElementGetByPos $one 0]
	set lmin [exprGet $cur.lambdaMin]
	set lmax [exprGet $cur.lambdaMax]
	set eta [exprGet $cur.eta]
	handleDel $cur
	chainDel $one
	set out "{lambdaMin $lmin} {lambdaMax $lmax} {eta $eta}"
	if {$delchn == 1 } {
		genericChainDestroy $stripeChn
	}
	return $out
}



proc nodeFromStripeNumber {stripe} {

	if {$stripe <= 100} {
		return 95.0
	} else {
		set lim [GCStripeLimits $stripe]
		set node [keylget lim node]
		return $node
	}	

}

proc GCinclFromStripeNumber {stripe} {

	if {$stripe <=100} {
		echo invalid GC stripe
		return -999
	} else {
		set lim [GCStripeLimits $stripe]
		set incl [keylget lim incl]
		return $incl
	}	

}

proc GCStripeLimits {stripe {testb -999}} {

global env

		if {$stripe >= 1000} {
		  set l [expr ($stripe-1000)/2.0]
		   set flip 0 
		   set blow -60
	           set bhigh 60
			if {$l > 122.93 && $l < 302.93} {
				set flip 1
				set blow 60
				set bhigh -60
			}
		if {$testb != -999} {
			set blow $testb
			set bhigh $testb
		}
		  set csl [atCheckGCStripeSegment $l $blow $bhigh $flip]
		set node [format %.6f [keylget csl node]]
		set incl [format %.6f [keylget csl incl]]
		set muMin [format %.2f [keylget csl muMax]]
		set muMax [format %.2f [keylget csl muMin]]
		set nu 0.0
		} else {
		#check the special stripe table
		#first check the specialStripes product exists, if 
		#so, use that, else try a default one
		if {![file exist $env(ASTROTOOLS_DIR)/etc/atGCStripeDef.par]} {
				echo can't find auxillary stripe file $env(ASTROTOOLS_DIR)/etc/atGCStripeDef.par
				set node 0
				set incl 0
				set muMin 0
				set muMax 0
				set nu 0
			} else {
	if {[info exists env(SPECIALSTRIPES_DIR)] && [file exist $env(SPECIALSTRIPES_DIR)/etc/ssGCStripeDef.par]} {
			set tab [param2Chain $env(SPECIALSTRIPES_DIR)/etc/ssGCStripeDef.par hh]
			}  else {
			set tab [param2Chain $env(ASTROTOOLS_DIR)/etc/atGCStripeDef.par hh]	
			}
			set look [chainSearch $tab "{stripeNumber == $stripe}"]
			set sz [chainSize $look]
			if {$sz != 1} {
				echo no such stripe $stripe defined
				set node 0
				set incl 0
				set muMin 0
				set muMax 0
				set nu 0
			} else {
			set cur [chainElementGetByPos $look 0]
			set node [format %.6f [exprGet $cur.node]]
			set incl [format %.6f [exprGet $cur.incl]]
			set muMin [format %.2f [exprGet $cur.muMin]]
			set muMax [format %.2f [exprGet $cur.muMax]]
			set nu 0
			  }
			}
			
		}

		return "{muMin $muMin} {muMax $muMax} {nu $nu} {node $node} {incl $incl}"

}


proc atCheckGCStripeSegment {l startb endb flip} {

	set rd1 [galToEq $l 30]
	set rd2 [galToEq $l -30]
	set ra1 [keylget rd1 ra]
	set dec1 [keylget rd1 dec]
	set ra2 [keylget rd2 ra]
	set dec2 [keylget rd2 dec]
	set ni [defineGC $ra1 $dec1 $ra2 $dec2]
	set node [keylget ni node]
	set incl [keylget ni inc]
	if {$flip == 1} {
		set incl [expr $incl + 180]
	}

	set rd [galToEq $l $startb]
	set ra [keylget rd ra]
	set dec [keylget rd dec]
	set mn [eqToGC $ra $dec -incl $incl -node $node]
	set mustart [keylget mn mu]

	set rd [galToEq $l $endb]
	set ra [keylget rd ra]
	set dec [keylget rd dec]
	set mn [eqToGC $ra $dec -incl $incl -node $node]
	set muend [keylget mn mu]

	#loop mu 0 360 { 
		#set rd [GCToEq $mu 0 -node $node -incl $incl]
		#set ra [keylget rd ra]
		#set dec [keylget rd dec]
		#set lb [eqToGal $ra $dec]
		#set l [keylget lb gLong]
		#set b [keylget lb gLat]
		#echo $mu $l $b
	#}

	return "{node $node} {incl $incl} {muMin $mustart} {muMax $muend}"

}
