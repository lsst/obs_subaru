##############################################################################
#  atWcs.tcl                                                   jrp-25jan01
#<AUTO EXTRACT>
#
# TCL VERB: atWcsAddToHdr
#
# <HTML>
# Add World Coordinate System (WCS) header keyword/value lines to a header.
#
# Required arguements:
#   trans:  Handle to the relevant astrom TRANS struct
#   hdr:    Handle to the header to be modified
#   node:   Node of the great circle (degrees)
#   incl:   Inclination of the great circle (degrees)
#
#Caveats:  this procedure applies the astrom solution TRANS values to
#    determine RA/DEC WCS header info for a field.  It is designed
#    to be appropriate for SDSS frames which have 1361 rows and
#    2048 columns.  It will work pretty well for corrected frames
#    which are 1489 rows and 2048 columns, but it will be grossly in
#    error for raw idR frames which are 1361 rows and 2128 columns
#    (since it expects that pixel [0,0] is the first pixel of the
#    imaged frame, i.e. there are no over- or under-scan columns).
#
#    Since a reg.hdr (which is what this routine will usually expect
#    to modify) does not contain a keyword NAXIS1 or NCOLS or the
#    like, this routine does not check for proper frame size.
#    
#    It is worth noting that asTrans structs are tagged to FIELD
#    number, just as corrected frames (fpC) are.  (Note that idR frames
#    are tagged to FRAME number, not FIELD number.)
#
#    The asTrans file is composed of numerous HDU's.  Best practice is to
#    use atReadTrans to be certain that you get the proper info. 
#
#    Executing
#       astls> atReadTrans $transfile $ccd
#   
#    will return several things in a keyed list:
#       1. chain -  the chain of TRANS structs for this run/ccd
#       2. node  -  the node of the greact circle scanned
#       3. incl  -  the inclination of the greact circle scanned
#       4. equinox - equinox of the TRANS corrdinates
#       5. field0 - first field in the trans chain
#       6. nfields - number of fields in the chain
#  
#    so, to be sure you get the right TRANS struct:
#       astls> set stuff [atReadTrans $transfile $ccd] 
#       astls> set transChain [keylget stuff chain]
#       astls> set node [keylget stuff node]
#       astls> set incl [keylget stuff incl]
#       astls> set smallChain [chainSearch $transChain "{ id == $field }"]
#       (if [chainSize $smallChain isn't == 1, there's a problem)
#       astls> set trans [chainElementGetByPos $smallChain HEAD}]
#
#
#</HTML></AUTO>
##############################################################################
set atWcsAddToHdrArgs {
   {hdrAddWcs "Add WCS header information to a header.\n"}
   {<trans>    STRING   ""   trans   "Handle to the relevant astrom TRANS struct"}
   {<hdr>      STRING   ""   hdr     "Handle to the relevant header"}
   {<node>     DOUBLE   ""   node    "Node of the Great Circle (degrees)"}
   {<incl>     DOUBLE   ""   incl    "Inclination of the Great Circle (degrees)"}
}
ftclHelpDefine astrotoolsProcs atWcsAddToHdr \
   [shTclGetHelpInfo atWcsAddToHdr $atWcsAddToHdrArgs]

proc atWcsAddToHdr { args } {
   upvar #0 atWcsAddToHdrArgs formal_list

   # Parse arguments.
   if {[shTclParseArg $args $formal_list atWcsAddToHdr] == 0} {return}

   global at_deg2Rad

   # Get ra/dec at center of frame:
   # Remember that WCS expects that the first pixel in a frame is
   #    pixel [1,1], (not [0,0] a la SDSS) and that the center of that pixel is
   #    [1.0,1.0], not [0.5 0.5] (a la SDSS).
   #
   # So, we would expect to specify the WCS reference pixel as 680.5, 1024.5.
   #    Then we'd expect SDSS pixel [r,c] to correspond to WCS pixel [r+1,c+1]
   #
   # Empirically, however, this leads to an 0.5 pixel systematic error.
   #     
   # This systematic error was first shown using the latest saoImage (called 
   #    "ds9").  Then IRAF was tried with exactly the same results.
   # Either these two packages use the same algorithm with a bug in it,
   #    or there is some bug in SDSS I haven't been able to find
   #
   # So we use WCS reference pixel of [680.0,1024.0], and get the 
   #    RA/DEC for that pixel by using SDSS convention for pixel
   #    [679,1023]
   #
   
   # This script was originally written by Jeff P. for corrected frames.  
   # Chris S. made the following changes:

   # If the keyword BINFAC is not present, we assume it is a corrected frame.
   # If the key word BINFAC is present and equal to 4, 
   # assume it is a binned image.
   # Otherwise, we do not know what it is and throw an error.

   # Also, I changed the way the reference pixel works and I believe
   # these results are not consistent with ds9.

   set midRow  744
   set midCol 1024

   set endRow 1488
   set endCol 2048

   set present [catch "hdrGetAsInt $hdr BINFAC" binfac]
   if {$present} {
       set binfac 1.0
   } 
### I talked to Chris about this, his response was:
#I was being too cautious in allowing only binfac=4.
#
#If the keyword is not present, assume it is 1
#Otherwise, trust the header!
#
# the code is being changed to reflect that fact by editing out the remaining checks after 
#checking for it's presence and assuming 1 if it is not present
#
#elseif {$binfac == 4} {
#       set binfac 4.0
#   } else {
#       error "atWcsAddToHdrArgs:  what is this with hdr=$hdr"
#   }

   set midRowInWCS [expr ($midRow/$binfac)+0.5]
   set midColInWCS [expr ($midCol/$binfac)+0.5]

   set munu [rc2munu $trans $midRow $midCol]
   set muref [keylget munu mu]
   set nuref [keylget munu nu]
   set radec [GCToEq $muref $nuref -node $node -inclination $incl]
   set raref  [keylget radec ra]
   set decref [keylget radec dec]
# need the cosine of the declination at reference pixel:
   set cosdecref [expr cos($decref*$at_deg2Rad)]

# get RA/DEC at frame boundaries at the centerlines
   set munu [rc2munu $trans $midRow 0.0]
   set mu0 [keylget munu mu]
   set nu0 [keylget munu nu]
   set radec [GCToEq $mu0 $nu0 -node $node -inclination $incl]
   set ra0  [keylget radec ra]
   set dec0 [keylget radec dec]

   set munu [rc2munu $trans 0.0 $midCol]
   set mu1 [keylget munu mu]
   set nu1 [keylget munu nu]
   set radec [GCToEq $mu1 $nu1 -node $node -inclination $incl]
   set ra1  [keylget radec ra]
   set dec1 [keylget radec dec]
   
   set munu [rc2munu $trans $endRow $midCol]
   set mu2 [keylget munu mu]
   set nu2 [keylget munu nu]
   set radec [GCToEq $mu2 $nu2 -node $node -inclination $incl]
   set ra2  [keylget radec ra]
   set dec2 [keylget radec dec]

   set munu [rc2munu $trans $midRow $endCol]
   set mu3 [keylget munu mu]
   set nu3 [keylget munu nu]
   set radec [GCToEq $mu3 $nu3 -node $node -inclination $incl]
   set ra3  [keylget radec ra]
   set dec3 [keylget radec dec]

# CD matrix:
# d(DEC)/d(Col)
   set cd21 [expr $binfac*($dec3-$dec0)/$endCol]
# d(DEC)/d(Row)
   set cd22 [expr $binfac*($dec2-$dec1)/$endRow]
# d(RA)/d(Col)
# NOTE -- in response to PR2829, do the wrapping around 360 correctly!
   set cd11 [expr $cosdecref*$binfac*[atWCSDeltaRa $ra3 $ra0]/$endCol]
#d(RA)/d(Row)
   set cd12 [expr $cosdecref*$binfac*[atWCSDeltaRa $ra2 $ra1]/$endRow]

# Modify the header
   hdrInsWithAscii $hdr ATVSN [astrotoolsVersion] "ASTROTOOLS version tag"
   hdrInsWithAscii $hdr RADECSYS "ICRS"   "International Celestial Ref. System"
   hdrInsWithAscii $hdr CTYPE1 "RA---TAN"
   hdrInsWithAscii $hdr CTYPE2 "DEC--TAN"
   hdrInsWithAscii $hdr CUNIT1 "deg"
   hdrInsWithAscii $hdr CUNIT2 "deg"
   hdrInsWithDbl   $hdr CRPIX1 $midColInWCS "Column Pixel Coordinate of Ref. Pixel"
   hdrInsWithDbl   $hdr CRPIX2 $midRowInWCS "Row Pixel Coordinate of Ref. Pixel"
   hdrInsWithDbl   $hdr CRVAL1 $raref  "RA at Reference Pixel"
   hdrInsWithDbl   $hdr CRVAL2 $decref "DEC at Reference Pixel"
   hdrInsWithDbl   $hdr CD1_1 $cd11  "RA  degrees per column pixel"
   hdrInsWithDbl   $hdr CD1_2 $cd12  "RA  degrees per row pixel"
   hdrInsWithDbl   $hdr CD2_1 $cd21  "DEC degrees per column pixel"
   hdrInsWithDbl   $hdr CD2_2 $cd22  "DEC degrees per row pixel"
   return
}


##############################################################################
#<AUTO EXTRACT>
#
# TCL VERB: atWCSDeltaRa
#
# <HTML>
# Find the difference in Ra between two points, accounting for wrapping
# around ra=360.
# </HTML>
#
#</AUTO>
##############################################################################
set atWCSDeltaRaArgs {
    {atWCSDeltaRa "Find the difference in Ra, accounting Ra=360\n"}
    {<ra0>		DOUBLE   ""   ra0        "ra0"}
    {<ra1>		DOUBLE  ""   ra1         "ra1"}
}
ftclHelpDefine astrotoolsProcs atWCSDeltaRa \
	[shTclGetHelpInfo atWCSDeltaRa $atWCSDeltaRaArgs]

proc atWCSDeltaRa { args } {
    upvar #0 atWCSDeltaRaArgs formal_list
    if {[shTclParseArg $args $formal_list atWCSDeltaRa] == 0} {return}
    
    set delta [expr $ra0-$ra1]
    if {$delta < -180} {set delta [expr $delta+360]}
    if {$delta >  180} {set delta [expr $delta-360]}
    #echo [format "in atWCSDeltaRa:  ra0=%10.6f ra1=%10.6f delta=%10.6f" \
	#    $ra0 $ra1 $delta]
    return $delta
}

##############################################################################
#<AUTO EXTRACT>
#
# TCL VERB: atReadTrans
#
# <HTML>
# Read the TRANS structures for a single CCD from a TRANS file into a
# chain of ASTROTOOLS TRANS structures.  Returns the handle to the TRANS chain.
# </HTML>
#
#</AUTO>
##############################################################################
set atReadTransArgs {
    {atReadTrans "Read a TRANS file into a TRANS chain for a single CCD.\n"}
    {<file>		STRING   ""   file        "TRANS file"}
    {<ccd>		INTEGER  ""   ccd         "CCD id"}
}
ftclHelpDefine astromProcs atReadTrans \
	[shTclGetHelpInfo atReadTrans $atReadTransArgs]

proc atReadTrans { args } {
    upvar #0 atReadTransArgs formal_list

    # Parse arguments.
    if {[shTclParseArg $args $formal_list atReadTrans] == 0} {return}

    # Determine the HDU to read, and read other PDU keywords
    set camRow [string range $ccd 0 0]
    set camCol [string range $ccd 1 1]
    switch $camRow {
       1 { set filter r }
       2 { set filter i }
       3 { set filter u }
       4 { set filter z }
       5 { set filter g }
       default { error "atReadTrans: Illegal camrow $camRow }
    }

    if {$filter == "o"} {
	set filter "l"
    } elseif {$filter == "s"} {
	set filter "t"
    }
    if {$camRow == 6 || $camRow == 9} {incr camCol 6}
    set hdr [hdrNew]
    if {[catch {hdrReadAsFits $hdr $file} err]} {
	hdrDel $hdr
	error "atReadTrans: couldn't read '$file' PDU: $err"
    }
    foreach tuple {
	{CCDARRAY Ascii}
	{CAMCOLS Ascii} 
	{FILTERS Ascii} 
	{INCL Dbl} 
	{NODE Dbl}
	{EQUINOX Dbl} 
	{FIELD0 Int}
	{NFIELDS Int}
    } {
	set key [lindex $tuple 0]
	set type [lindex $tuple 1]
	if {[catch {hdrGetAs$type $hdr $key} $key]} {
	    if {$key == "CCDARRAY"} {
		echo "WARNING: atReadTrans: CCDARRAY keyword missing from '$file' PDU: assume old format for HDU ordering"
		set CCDARRAY photo
	    } else {
		hdrDel $hdr
		error "atReadTrans: keyword $key missing from '$file' PDU"
	    }
	}
    }
    set CCDARRAY [string trim $CCDARRAY]
    hdrDel $hdr
    if {[set iFilter [lsearch -exact $FILTERS $filter]] == -1} {
	error "atReadTrans: filter $filter not in '$file'"
    }
    if {[set iCamCol [lsearch -exact $CAMCOLS $camCol]] == -1} {
	error "atReadTrans: camCol $camCol not in '$file'"
    }
    if {$CCDARRAY == "astro"} {
	set hdu [expr 1 + [llength $CAMCOLS] * $iFilter + $iCamCol]
    } else {
	set hdu [expr 1 + [llength $FILTERS] * $iCamCol + $iFilter]
    }

    # Read the appropriate HDU.  Verify its the correct one.
    set t [tblColNew]
    fitsRead $t $file -binary -hdu $hdu
    if {$camCol > 6} {incr camCol -6}
    foreach var {camRow camCol} {
	set varUp [string toupper $var]
	if {[catch {hdrGetAsInt $t.hdr $varUp} $varUp]} {
	    tblColDel $t
	    error "atReadTrans: keyword $varUp missing from '$file', HDU $hdu"
	}
	if {[set $var] != [set $varUp]} {
	    if {$var == "camCol"} {
		if {[set $var] != [expr [set $varUp] - 6]} {
		    tblColDel $t
		    error "atReadTrans: keyword $varUp = [set $varUp], not [set $var]"
		}
	    }
	}
    }

    # Allocate the TRANS chain
    set c [chainNew TRANS $NFIELDS]

    # Copy the TBLCOL info to the chain
    set expr "$c.id = $t.field"
    foreach var {a b c d e f dRow0 dRow1 dRow2 dRow3 dCol0 dCol1 dCol2 dCol3 csRow csCol ccRow ccCol riCut mjd airmass} {
	append expr "; $c.$var = $t.$var"
    }
    if {[exprEval $expr] != $NFIELDS} {
	error "atReadTrans: too few iterations"
    }
    tblColDel $t

    # Return the chain and associate info
    return "{chain $c} {node $NODE} {incl $INCL} {equinox $EQUINOX} {field0 $FIELD0} {nFields $NFIELDS}"
}

# plot lines of constant RA and DEC on a frame in mu/nu space
proc gridMap { trans incl { label "" } {ps 0} } {
   set node 95.
   set munu [rc2munu $trans 0. 0.]
   set mu0 [keylget munu mu]
   set nu0 [keylget munu nu]

   set munu [rc2munu $trans 0. 2047.]
   set mu1 [keylget munu mu]
   set nu1 [keylget munu nu]

   set munu [rc2munu $trans 0. 2047.]
   set mu2 [keylget munu mu]
   set nu2 [keylget munu nu]

   set munu [rc2munu $trans 1360. 2047.] 
   set mu3 [keylget munu mu]
   set nu3 [keylget munu nu]

   echo [format "%8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f" \
       $mu0 $nu0 $mu1 $nu1 $mu2 $nu2 $mu3 $nu3]

   set radec [GCToEq $mu0 $nu0 -node $node -inclination $incl]
   set ra0 [keylget radec ra]
   set dec0 [keylget radec dec]

   set radec [GCToEq $mu1 $nu1 -node $node -inclination $incl]
   set ra1 [keylget radec ra]
   set dec1 [keylget radec dec]

   set radec [GCToEq $mu2 $nu2 -node $node -inclination $incl]
   set ra2 [keylget radec ra]
   set dec2 [keylget radec dec]

   set radec [GCToEq $mu3 $nu3 -node $node -inclination $incl]
   set ra3 [keylget radec ra]
   set dec3 [keylget radec dec]

   set ramin [expr $ra0 < $ra1 ? $ra0 : $ra1 ]
   set ramin [expr $ra1 < $ramin ? $ra1 : $ramin ]
   set ramin [expr $ra2 < $ramin ? $ra2 : $ramin ]
   set ramin [expr $ra3 < $ramin ? $ra2 : $ramin ]

   set decmin [expr $dec0 < $dec1 ? $dec0 : $dec1 ]
   set decmin [expr $dec1 < $decmin ? $dec1 : $decmin ]
   set decmin [expr $dec2 < $decmin ? $dec2 : $decmin ]
   set decmin [expr $dec3 < $decmin ? $dec3 : $decmin ]

   set ramax [expr $ra0 > $ra1 ? $ra0 : $ra1]
   set ramax [expr $ra1 > $ramax ? $ra1 : $ramax
   set ramax [expr $ra2 > $ramax ? $ra2 : $ramax
   set ramax [expr $ra3 > $ramax ? $ra3 : $ramax

   set decmax [expr $dec0 > $dec1 ? $dec0 : $dec1]
   set decmax [expr $dec1 > $decmax ? $dec1 : $decmax]
   set decmax [expr $dec2 > $decmax ? $dec2 : $decmax]
   set decmax [expr $dec3 > $decmax ? $dec3 : $decmax]

   echo [format "%8.3f %8.3f" $ramin $decmin]

   set pgs [pgstateNew]
   if {$ps} { pgstateSet $pgs -device /PS }
   pgstateSet $pgs -symb 2 -just 1 -plotTitle $label
   pgstateOpen $pgs

   loop ra [expr 100.*$ramin - 20] [expr 100.*$ramax + 20] {
      set dms {}
      set dns {}
      loop dec [expr 100.*$decmin -20]  [expr 100.* $decmax +20] 5 {
         set munu [eqToGC [expr $ra/100.] [expr $dec/100.] -node $node \
	    -inclination $incl]
         append dms [keylget munu mu] " "
         append dns [keylget munu nu] " "
      }
      set vdms [vFromL $dms]
      set vdns [vFromL $dns]
      vPlot $pgs $vdns $vdms -xmin $nu0 -xmax $nu3 -ymin $mu0 -ymax $mu3
      pgstateSet $pgs -isNewplot 0
      vectorExprDel $vdms
      vectorExprDel $vdns
   }
   pgstateSet $pgs -symb 1
   loop dec [expr 1000.*$decmin -200]  [expr 1000.* $decmax +200]  {
      set dms {}
      set dns {}
      loop ra [expr 100.*$ramin - 20] [expr 100.*$ramax + 20] 5 {
         set munu [eqToGC [expr $ra/100.] [expr $dec/1000.] -node $node \
	    -inclination $incl]
         append dms [keylget munu mu] " "
         append dns [keylget munu nu] " "
      }
      set vdms [vFromL $dms]
      set vdns [vFromL $dns]
      vPlot $pgs $vdns $vdms -xmin $nu0 -xmax $nu3 -ymin $mu0 -ymax $mu3
      pgstateSet $pgs -isNewplot 0
      vectorExprDel $vdms
      vectorExprDel $vdns
   }
   if {$ps} {pgstateClose $pgs}
   return 
}

# simple transApply (affine transformation only, no distortion terms)
proc rc2munu { trans row col } {
   set a [exprGet $trans.a]
   set b [exprGet $trans.b]
   set c [exprGet $trans.c]
   set d [exprGet $trans.d]
   set e [exprGet $trans.e]
   set f [exprGet $trans.f]

   set mu [expr $a + $b*$row + $c*$col]
   set nu [expr $d + $e*$row + $f*$col]

   return "{mu $mu} {nu $nu}"
}

# using rc2munu, compute ra/dec for the given row/col
proc rc2radec { trans incl row col { node 95. } } {
   set munu [rc2munu $trans $row $col]
   set mu [keylget munu mu]
   set nu [keylget munu nu]
   set radec [GCToEq $mu $nu -node $node -inclination $incl]
   set ra  [keylget radec ra]
   set dec [keylget radec dec]

   set rahms [degToHMS $ra]
   set dechms [degToDMS $dec]
   echo [format "%11s %11s    %12.8f  %12.8f" $rahms $dechms $ra $dec]
}

proc atFindFieldsThatWrap {run rerun {dataRoot /data/dp3.b/data} } {
    global chain stuff at_deg2Rad
    set fl "X r i u z g"
    # In response to PR2829, find all fields that overlap ra=360
    
    set midRow  744
    set midCol 1024
    
    set endRow 1488
    set endCol 2048
    
    set binfac 1.0
    set midRowInWCS [expr ($midRow/$binfac)+0.5]
    set midColInWCS [expr ($midCol/$binfac)+0.5]
    
    
    set tfn $dataRoot/$run/$rerun/astrom/asTrans-[format %06d $run].fit
    foreach camrow "1 2 3 4 5" {
	foreach camcol "1 2 3 4 5 6" {
	    set ccd $camrow$camcol
	    set stuff [atReadTrans $tfn $ccd]
	    set chain [keylget stuff chain]
	    set node [keylget stuff node]
	    set incl [keylget stuff incl]
	    set cursor [chainCursorNew $chain]
	    while {[set trans [chainWalk $chain $cursor]] != ""} {
		
		set munu [rc2munu $trans $midRow $midCol]
		set muref [keylget munu mu]
		set nuref [keylget munu nu]
		set radec [GCToEq $muref $nuref -node $node -inclination $incl]
		set raref  [keylget radec ra]
		set decref [keylget radec dec]
		# need the cosine of the declination at reference pixel:
		set cosdecref [expr cos($decref*$at_deg2Rad)]
		
		# get RA/DEC at frame boundaries at the centerlines
		set munu [rc2munu $trans $midRow 0.0]
		set mu0 [keylget munu mu]
		set nu0 [keylget munu nu]
		set radec [GCToEq $mu0 $nu0 -node $node -inclination $incl]
		set ra0  [keylget radec ra]
		set dec0 [keylget radec dec]
		
		set munu [rc2munu $trans 0.0 $midCol]
		set mu1 [keylget munu mu]
		set nu1 [keylget munu nu]
		set radec [GCToEq $mu1 $nu1 -node $node -inclination $incl]
		set ra1  [keylget radec ra]
		set dec1 [keylget radec dec]
		
		set munu [rc2munu $trans $endRow $midCol]
		set mu2 [keylget munu mu]
		set nu2 [keylget munu nu]
		set radec [GCToEq $mu2 $nu2 -node $node -inclination $incl]
		set ra2  [keylget radec ra]
		set dec2 [keylget radec dec]
		
		set munu [rc2munu $trans $midRow $endCol]
		set mu3 [keylget munu mu]
		set nu3 [keylget munu nu]
		set radec [GCToEq $mu3 $nu3 -node $node -inclination $incl]
		set ra3  [keylget radec ra]
		set dec3 [keylget radec dec]
		
		set field  [exprGet $trans.id]
		if {[expr abs($ra0 - $ra3) > 1.0] || [expr abs($ra2 - $ra1) > 1.0]} {
		    echo $run $rerun $camcol $field [lindex $fl $camrow]
		}
	    }
	}
    }
}


