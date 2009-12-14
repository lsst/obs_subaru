#
# Read in sample catalog file and objclist dump file.
#
proc sampleRead {cat obj} {
    upvar $cat catalog $obj object
    set catalog [readCatalog [envscan \$PHOTODATA_DIR]/frames_4/output/high_0_3_1_r5.lst -x=-73]
    dumpOpen [envscan \$PHOTODATA_DIR]/frames_4/output/objclist-0-1-3.dump r
    set object [dumpHandleRead]
    dumpClose


    # remove any OBJCs with NULL elements in the color<0> position
    set cursor [chainCursorNew $object]
    set objc [chainElementGetByPos $object HEAD]
    while { 1 } {
      if { [ exprGet $objc.color<0> ] == 0 } {
        set objc [chainElementRemByCursor $object $cursor]
        objcDel $objc
      }
      if { [ set objc [chainWalk $object $cursor NEXT] ] == "" } {
        break
      }
    }
}

#
# Match two list of OBJC structure in terms of position (rowc and colc).
#
proc match_objclist {cat obj rad mA mB umA umB} {
    upvar $mA matchA $mB matchB $umA unmatchA $umB unmatchB
    set output [objcListMatch $cat $obj $rad]
    set matchA   [lindex [lindex $output 0] 3]
    set matchB   [lindex [lindex $output 0] 7]
    set unmatchA [lindex [lindex $output 1] 3]
    set unmatchB [lindex [lindex $output 1] 7]
}

#
# Draw scatter plot for the members of matched OBJC list.
# Parameters :
#    cat     matched OBJC list from catalog.
#    obj     matched OBJC list from objclist.
#    xexp    member of $cat to be plotted (color<0> will be added).
#    yexp    member of $obj to be plotted (color<0> will be added).
#    -mag    value converted to magnitude will be plotted.
#    -diff   difference of $xexp and $yexp will be plotted.
#    -Xlist  list specified here will be used for X-axis.
#    -Xexp   member specified here will be used for X-axis
#            (color<0> will be added).
#    -Xmag   value converted to magnitude will be plotted for $Xlist->$Xmag.
#    -xmin   specify minimum x value
#            (only changes bounding box, do not select data).
#    -xmax   specify maximum x value (same as xmin).
#    -ymin   specify minimum y value (same as xmin).
#    -ymax   specify maximum y value (same as xmin).
#
proc compPlot {args} {
    set context compPlot
    ftclParseSave $context
    ftclFullParse {cat obj xexp yexp -mag -diff -Xlist -Xexp -Xmag \
	    -xmin -xmax -ymin -ymax} $args
    set cat [ftclGetStr cat]
    set obj [ftclGetStr obj]
    set xexp [ftclGetStr xexp]
    set yexp [ftclGetStr yexp]
    if [ftclPresent mag] { set mag 1 } else { set mag 0 }
    if [ftclPresent diff] { set diff 1 } else { set diff 0 }
    if [ftclPresent Xlist] { set Xlist [ftclGetStr Xlist] } else { set Xlist 0}
    if [ftclPresent Xexp] { set Xexp [ftclGetStr Xexp] } else { set Xexp 0 }
    if [ftclPresent Xmag] { set Xmag 1 } else { set Xmag 0 }
    if [ftclPresent xmin] { set xmin [ftclGetStr xmin] } else { set xmin 9999 }
    if [ftclPresent xmax] { set xmax [ftclGetStr xmax] } else { set xmax 9999 }
    if [ftclPresent ymin] { set ymin [ftclGetStr ymin] } else { set ymin 9999 }
    if [ftclPresent ymax] { set ymax [ftclGetStr ymax] } else { set ymax 9999 }
    set afx [afFromChain $cat color<0>->$xexp]
    set afy [afFromChain $obj color<0>->$yexp]
    set xtitle ""
    set ytitle ""

    # this should be a command-line option: the zero point of simulation mag
    #   scale (i.e. 1 DN = this mag)
    set magzero 28.22

    if $mag {
	set afxm [afMag $magzero $afx]
	set afym [afMag $magzero $afy]
	afDel $afx
	afDel $afy
	set afx $afxm
	set afy $afym
	set xtitle [concat $xtitle (mag)]
	set ytitle [concat $ytitle (mag)]
    }
    if $diff {
	set afX [afFromAf $afx]
	set afY [afOper $afx m $afy]
	set xtitle [concat $xtitle $cat.$xexp]
	set ytitle [concat $ytitle $cat.$xexp - $obj.$yexp]
    } else {
	set afX [afFromAf $afx]
	set afY [afFromAf $afy]
	set xtitle [concat $xtitle $cat.$xexp]
	set ytitle [concat $ytitle $obj.$yexp]
    }
    if { $Xlist != "0" } {
	afDel $afX
	afDel $afx
	set afx [afFromChain $Xlist color<0>->$Xexp]
	set xtitle ""
	if $Xmag {
	    set afxm [afMag $magzero $afx]
	    afDel $afx
	    set afx $afxm
	    set xtitle [concat $xtitle (mag)]
	}
	set afX [afFromAf $afx]
	set xtitle [concat $xtitle $Xlist.$Xexp]
    }
    #
    set pg [pgstateNew]


    doCompPlot /XWINDOW $pg $afX $afY $cat $xtitle $ytitle \
	    $xmin $xmax $ymin $ymax


    showImageByCursor $obj $afX $afY
    puts stdout "Print? (y/<n>) " nonewline
    gets stdin ch
    pgstateClose $pg
    pgstateDel $pg
    if {$ch == "y" || $ch == "Y"} {
	puts stdout "Output file name? " nonewline
	gets stdin fname
	set pg [pgstateNew]
	doCompPlot $fname/PS $pg $afX $afY $cat $xtitle $ytitle \
		$xmin $xmax $ymin $ymax
	pgstateClose $pg
	pgstateDel $pg
    }
    afDel $afX
    afDel $afY
    afDel $afx
    afDel $afy
    ftclParseRestore $context
}

#
# Procedure which carry out actual drawing.
#
proc doCompPlot {device pg afX afY cat xtitle ytitle \
    _xmin _xmax _ymin _ymax} {
    set afXs [afFromAf $afX]
    set afXg [afFromAf $afX]

# for testing from home
set dis 1
if { $dis != 0 } {
    pgstateSet $pg -device $device
    pgstateOpen $pg
}
    if { $_xmin == "9999" } { set _xmin [afExtreme $afX min] }
    if { $_xmax == "9999" } { set _xmax [afExtreme $afX max] }
    if { $_ymin == "9999" } { set _ymin [afExtreme $afY min] }
    if { $_ymax == "9999" } { set _ymax [afExtreme $afY max] }

    # Stars
#    the following line causes an error, since "afMaskSetFromChain" is broken
#    afMaskSetFromChain $afXs $cat color<0>->flags == 0
    mark_galaxies $afXs $cat
analyze_bin_af $afXs $afY 12 25 

if { $dis != 0 } {
    pgstateSet $pg -symb 22 -icMark 2
    afPlot $pg $afXs $afY -afMask none \
	    -xmin $_xmin -xmax $_xmax -ymin $_ymin -ymax $_ymax
}

if { $dis != 0 } {
    pgstateSet $pg -isNewplot 0
}
    # Galaxies
#    afMaskSetFromChain $afXg $cat color<0>->flags > 1
    mark_stars $afXg $cat
    pgstateSet $pg -symb 17 -icMark 3
    afPlot $pg $afXg $afY -afMask none \
	    -xmin $_xmin -xmax $_xmax -ymin $_ymin -ymax $_ymax
    afDel $afXs
    afDel $afXg
    # Plot title
if { $dis != 0 } {
    titlePlot $xtitle $ytitle
}
#    graph_plot $afX $afY
}

#
# Return new af converted to magnitude.
#
proc afMag {const af} {
    afClip $af < 1
    afLogTrans $af -base 10
    set af25 [afSetWithConst [afDefine [afNew] -nValue [afSize $af]] 2.5]
    set af1 [afOper $af25 * $af]
    afDel $af25
    set con [afSetWithConst [afDefine [afNew] -nValue [afSize $af1]] $const]
    set af2 [afOper $con m $af1]
    afDel $con
    afDel $af1
    return $af2
}

#
# Select an object in afPlot by clicking the point.
# Show brief parameters and atlas image.
#
proc showImageByCursor {obj afX afY} {
    echo 
    echo Select a point by clicking left button.
    echo Exit by clicking right button.
    echo 
    while {1} {
	set output [pgCurse 0 0]
	set xval [lindex $output 0]
	set yval [lindex $output 1]
	set button [lindex $output 2]
	if { $button == "X" } { break }
	if { $button == "A" } {
	    set objc [objcFromValueOfArrays $obj $afX $afY -vx=$xval -vy=$yval]
	    echo $objc
	    objcPrintPretty $objc
 	   set region [handleBindFromHandle [handleNew] *objc.color<0>->region]
	   set mask [handleBindFromHandle [handleNew] *objc.color<0>->mask]
	    set sao [saoDisplay $region "-histeq"]
	    saoMaskColorSet {green orange yellow red blue purple magenta cyan}
	    saoMaskDisplay $mask -p bitset -s $sao
	    handleDel $mask
	    handleDel $region
	    handleDel $objc
	}
    }
}

#
# Calculate the statics of two af's. 
# Draw regression line.
#
proc graph_plot {args} {
    set context graph_plot
    ftclParseSave $context
    ftclFullParse {afx afy -fixA -fixB -noline} $args
    set afx [ftclGetStr afx]
    set afy [ftclGetStr afy]

 set x_mean [afMean $afx]
 set y_mean [afMean $afy]
 set x_sd [afSigma $afx]
 set y_sd [afSigma $afy]

 set r 2
#r=2 is error

if [ftclPresent fixA] {
 set A [ftclGetStr fixA]
 if [ftclPresent fixB] {
  set B [ftclGetStr fixB]
  set regress [afRegress $afx $afy -fixA=$A -fixB=$B]
  set lineTitle "No Fit"
 } else {
  set regress [afRegress $afx $afy -fixA=$A]
  set B [lindex $regress 1] 
  set lineTitle "A-fixed Fit"
 } 
} else {  
 if [ftclPresent fixB]  {
  set B [ftclGetStr fixB]
  set regress [afRegress $afx $afy -fixB=$B] 
  set lineTitle "B-fixed Fit"
 } else {
  set regress [afRegress $afx $afy]
  set B [lindex $regress 1]
  set r [lindex $regress 3]
  set lineTitle "Normal Fit"
 }
 set A [lindex $regress 0]
}

# echo $regress
 set A [lindex $regress 0]
 set B [lindex $regress 1]
 set r [lindex $regress 2]
 set s [lindex $regress 3]

 set win [lindex [pgQwin] 0] 
 set xmin [lindex $win 0]
 set xmax [lindex $win 1]
 set ymin [lindex $win 2]
 set ymax [lindex $win 3]

#
  set scale 30.
#

if (![ftclPresent noline]) {
 
#echo $xmin $xmax $ymin $ymax
 #
 set x0 $xmin 
 set x1 $xmax

 if ($A!=0) then {

  set x2 [expr ($ymin-$B)/($A)]  
  set x3 [expr ($ymax-$B)/($A)]  

#echo $x0 $x1 $x2 $x3

if ($x2<$x3) {
   if ($x0<$x2) { 
	set x0 $x2
     }
   if ($x1>$x3) { 
      set x1 $x3
     } 
} else {
   if ($x0<$x3) { 
	set x0 $x3
     }
   if ($x1>$x2) { 
      set x1 $x2
     } 
}
}  

set y0 [expr $x0*$A+$B]
set y1 [expr $x1*$A+$B]

 pgMove $x0 $y0
 pgDraw $x1 $y1

#using underscript... rather foolish (g_g)

if (($r<=1.0)&&($r>=-1.0)) {
 pgText $xmax [expr $ymax-5*(($ymax)-($ymin))/$scale] [format %s%10.10s "\d\d  r\dxy\u" [format %5.4f $r]]
}

 pgText $xmax [expr $ymax-7*(($ymax)-($ymin))/$scale] [format %s%s "\d\d  " $lineTitle]

 pgText $xmax [expr $ymax-8*(($ymax)-($ymin))/$scale] " \d\dy=Ax+B"
 pgText $xmax [expr $ymax-9*(($ymax)-($ymin))/$scale] [format %s%10.10s "\d\d   A" [format %8.4f $A]]
 pgText $xmax [expr $ymax-10*(($ymax)-($ymin))/$scale] [format %s%10.10s "\d\d   B" [format %8.4f $B]]
 pgText $xmax [expr $ymax-11*(($ymax)-($ymin))/$scale] [format %s%10.10s "\d\d   \gs" [format %8.4f $s]]

}

 pgText $xmax [expr $ymax-0*(($ymax)-($ymin))/$scale] [format %s%10.10s "\d\d X\gm" [format %8.4f $x_mean]]
 pgText $xmax [expr $ymax-1*(($ymax)-($ymin))/$scale] [format %s%10.10s "\d\d  \gs" [format %8.4f $x_sd]]
 pgText $xmax [expr $ymax-2*(($ymax)-($ymin))/$scale] [format %s%10.10s "\d\d Y\gm" [format %8.4f $y_mean]]
 pgText $xmax [expr $ymax-3*(($ymax)-($ymin))/$scale] [format %s%10.10s "\d\d  \gs" [format %8.4f $y_sd]]

 set tm [exec date]

 pgText [expr $xmax-5*(($xmax)-($xmin))/$scale] [expr $ymax+8*(($ymax)-($ymin))/$scale] [format %s%s "\d" $tm ]

 ftclParseRestore $context
}

#
# Draw title upper-left of the screen.
#
proc titlePlot {xtitle ytitle} {
    set win [lindex [pgQwin] 0] 
    set xmin [lindex $win 0]
    set xmax [lindex $win 1]
    set ymin [lindex $win 2]
    set ymax [lindex $win 3]

    set scale 30

    pgText $xmin [expr $ymax+4*(($ymax)-($ymin))/$scale] [format %s%s "\d Xtitle : " $xtitle]
    pgText $xmin [expr $ymax+2*(($ymax)-($ymin))/$scale] [format %s%s "\d Ytitle : " $ytitle]
}

#
# Draw histgram for each pixels in a region.
#
proc regHist {reg min max nbin} {
    set hg [hgNew]
    hgDefine $hg -min=$min -max=$max -nbin=$nbin
    hgReg $hg $reg
    set pg [pgstateNew]
    pgstateOpen $pg
    hgPlot $pg $hg
    puts stdout "Print? (y/<n>) " nonewline
    gets stdin ch
    pgstateClose $pg
    pgstateDel $pg
    if {$ch == "y" || $ch == "Y"} {
	puts stdout "Output file name? " nonewline
	gets stdin fname
	set pg [pgstateNew]
	pgstateSet $pg -device [concat $fname /PS]
	pgstateOpen $pg
	hgPlot $pg $hg
	pgstateClose $pg
	pgstateDel $pg
    }
    hgDel $hg
}

#
# Draw histgram for detected objects.  This will give three plots:
#    1. histogram of all objects in catalog, in one-mag bins
#    2. histogram of all detected objects, in one-mag bins
#    3. histogram of ratio of detected to catalog, in one-mag bins
#
# Unfortunately, "hgPlot" does not allow over-plotting.
#
proc objHist {cat det} {

    # now remove all objects which aren't of the desired type; we define
    # a variable "type", which will be 0 for stellar sources and 1 for 
    # galaxies
    #
    # first, do this to the catalog objects
    set newcat [chainCopy $cat]
    set cursor [chainCursorNew $newcat]
    while { [set objc [chainWalk $newcat $cursor NEXT] ] != "" } {
      set type [exprGet $objc.color<0>->faint_type]
      if { $type == "(enum) FAINT_UNK" || $type == "(enum) FAINT_STAR" } {
        set type 0
      } else {
        set type 1
      }
      if { $type == 1 } {
        chainElementRemByCursor $newcat $cursor
      } else {
        set rowc [exprGet $objc.color<0>->rowc]
        set colc [exprGet $objc.color<0>->colc]
        set counts [exprGet $objc.color<0>->totalCounts]
        if { $counts > 100000 } {
          echo "keeping star at $rowc $colc $counts"
        }
      }
    }
    echo "chainSize of entire catalog is " [chainSize $cat]
    echo "chainSize of catalog stars only is " [chainSize $newcat]

    # next, do this to the detected objects
    set newdet [chainCopy $det]
    set cursor [chainCursorNew $newdet]
    while { [set objc [chainWalk $newdet $cursor NEXT] ] != "" } {
      set type [exprGet $objc.color<0>->faint_type]
      if { $type == "(enum) FAINT_UNK" || $type == "(enum) FAINT_STAR" } {
        set type 0
      } else {
        set type 1
      }
      if { $type == 1 } {
        chainElementRemByCursor $newdet $cursor
      }
    }
    echo "chainSize of entire detected obj is " [chainSize $det]
    echo "chainSize of detected stars only is " [chainSize $newdet]


    # this should be a command-line option: the zero point of simulation mag
    #   scale (i.e. 1 DN = this mag)

    set magzero 28.22
    set temp [afFromChain $newcat color<0>->totalCounts]
    set catAf [afMag $magzero $temp]
    afDel $temp
    set temp [afFromChain $newdet color<0>->totalCounts]
    set detAf [afMag $magzero $temp]
    afDel $temp

    #
    set catHg [hgFromAf $catAf -min 10 -max 30 -nbin 20]
    set detHg [hgFromAf $detAf -min 10 -max 30 -nbin 20]
    set divHg [hgOper $detHg / $catHg]
    hgFill $divHg 29 -weight 1.0
    #
    set pg [pgstateNew]
    pgstateOpen $pg
    hgPlot $pg $catHg
    puts stdout "Print? (y/<n>) " nonewline
    gets stdin ch
    if {$ch == "y" || $ch == "Y"} {
        pgstateClose $pg
        pgstateDel $pg
	puts stdout "Output file name? " nonewline
	gets stdin fname
	set pg [pgstateNew]
	pgstateSet $pg -device [concat $fname /PS]
	pgstateOpen $pg
	hgPlot $pg $catHg
	pgstateClose $pg
	pgstateDel $pg
        set pg [pgstateNew]
        pgstateOpen $pg
    }

    #
    # these two "pgstateSet" lines aren't heeded by "hgPlot", but may be
    # used by future fix to hgPlot
    pgstateSet $pg -isNewplot 0
    pgstateSet $pg -lineStyle 2
    hgPlot $pg $detHg
    puts stdout "Print? (y/<n>) " nonewline
    gets stdin ch
    if {$ch == "y" || $ch == "Y"} {
        pgstateClose $pg
        pgstateDel $pg
	puts stdout "Output file name? " nonewline
	gets stdin fname
	set pg [pgstateNew]
	pgstateSet $pg -device [concat $fname /PS]
	pgstateOpen $pg
	hgPlot $pg $detHg
	pgstateClose $pg
	pgstateDel $pg
        set pg [pgstateNew]
        pgstateOpen $pg
    }

    #
    hgPlot $pg $divHg
    puts stdout "Print? (y/<n>) " nonewline
    gets stdin ch
    pgstateClose $pg
    pgstateDel $pg
    if {$ch == "y" || $ch == "Y"} {
	puts stdout "Output file name? " nonewline
	gets stdin fname
	set pg [pgstateNew]
	pgstateSet $pg -device [concat $fname /PS]
	pgstateOpen $pg
	hgPlot $pg $divHg
	pgstateClose $pg
	pgstateDel $pg
    }

    afDel $catAf
    afDel $detAf
    hgDel $catHg
    hgDel $detHg
    hgDel $divHg
}


#
# given an AF and a CHAIN of OBJCs with the same number of elements, 
#   so that AF elem 5 corresponds to CHAIN elem 5, 
#   We set all "star" and "qso" elements of the given AF to have
#   (magnitude) values 99.0.
#
# This procedure is a stand-in for afMaskSetFromChain, which was
#   broken as of 1/23/95.
#
proc mark_stars { af chain } {

  set size [afSize $af]
  loop i 0 $size {
    set objc [chainElementGetByPos $chain $i]
    set type [exprGet $objc.color<0>->flags]
    if { $type < 2 } {
      afSet $af $i 99.0
    }
  }
}

#
# given an AF and a CHAIN of OBJCs with the same number of elements, 
#   so that AF elem 5 corresponds to CHAIN elem 5, 
#   We set all "galaxy" elements of the given AF to have
#   (magnitude) values 99.0.
#
# This procedure is a stand-in for afMaskSetFromChain, which was
#   broken as of 1/23/95.
#
proc mark_galaxies { af chain } {

  set size [afSize $af]
  loop i 0 $size {
    set objc [chainElementGetByPos $chain $i]
    set type [exprGet $objc.color<0>->flags]
    if { $type >= 2 } {
      afSet $af $i 99.0
    }
  }
}

#
# analyze the given AFs in bins; bin the elements by the
#   "Xaf" value, then use the corresponding elements in 
#   "Yaf" array to calc stats.
#   The given min and max bins must be integers.
#
proc analyze_bin_af { Xaf Yaf min_bin max_bin } {

  set size [afSize $Xaf]
  
  loop bin $min_bin $max_bin {

    set binplus [expr $bin+1]

    # first, we count the number of elements in this bin
    set num 0
    loop i 0 [expr $size] {
      set val [afGetValue $Xaf $i]
      if { $val >= $bin && $val < $binplus} {
        set num [expr $num+1]  
      }
    }
    # now create another AF with only values in the bin
    if { $num == 0 } { continue }
    set af2 [afNew]
    afDefine $af2 -nValue $num
    set num 0
    loop i 0 [expr $size] {
      set val [afGetValue $Xaf $i]
      set yval [afGetValue $Yaf $i]
      if { $val >= $bin && $val < $binplus} {
        afSet $af2 $num $yval
        set num [expr $num+1]  
      }
    }
    # and now we can calculate stats in this bin
    set mean [afSigmaClip $af2 -sigmaClip 4.0 -nIter 3]

    echo "$bin .. $binplus   N=$num   $mean"
    afDel $af2
  }
}
    

