# This file contains utility routines to read an ascii file and find the
# minimum and maximum of a list.  There are small shell procedures to make
# different types of plots from an ascii file of numbers.  Each calls the
# plotFromFileWithEverything procedure which has all of the switches for
# which type of plot is requested.
set tcl_precision 17

# scan only reads 6 digits of accuracy.  This reads them all.
proc scantoarray {linevar arrayname} {
  upvar $linevar line $arrayname a
  set i 0
  while {1} {
    if {[regexp {[-,+,0-9,e,E,.]+} $line a($i)]==0} {break}
    set start [expr [string first $a($i) $line]+[string length $a($i)]]
    set line [string range $line $start [string length $line]]
    regsub {^[ ]*} $line "" junk
    set line $junk
    set i [expr $i+1]
  }
}

proc listMax {list} {
  set lmax [lindex $list 0]
  loop i 1 [llength $list] {
    set lmax [max $lmax [lindex $list $i]]
  }
  return $lmax
}

proc listMin {list} {
  set lmin [lindex $list 0]
  loop i 1 [llength $list] {
    set lmin [min $lmin [lindex $list $i]]
  }
  return $lmin
}

set facil shPlot

# From ascii file, plot ypos column versus xpos column 
#	- only numbers are read.
ftclHelpDefine $facil plotFromFile "plotFromFile file xpos ypos"
proc plotFromFile {file xpos ypos} {
  plotFromFileWithEverything $file $xpos $ypos # # # # n 0 line # n
}

ftclHelpDefine $facil appendFromFile "appendFromFile file xpos ypos"
proc appendFromFile {file xpos ypos} {
  plotFromFileWithEverything $file $xpos $ypos # # # # n 0 line # y
}

ftclHelpDefine $facil savePlotFromFile "savePlotFromFile file xpos ypos"
proc savePlotFromFile {file xpos ypos} {
  plotFromFileWithEverything $file $xpos $ypos # # # # y 0 line # n
}

ftclHelpDefine $facil plotFromFileWithLimits "plotFromFileWithLimits file xpos ypos xmin xmax ymin ymax"
proc plotFromFileWithLimits {file xpos ypos xmin xmax ymin ymax} {
  plotFromFileWithEverything $file $xpos $ypos $xmin $xmax $ymin $ymax n 0 line # n
}
ftclHelpDefine $facil saveplotFromFileWithLimits "saveplotFromFileWithLimits file xpos ypos xmin xmax ymin ymax"
proc savePlotFromFileWithLimits {file xpos ypos xmin xmax ymin ymax} {
  plotFromFileWithEverything $file $xpos $ypos $xmin $xmax $ymin $ymax y 0 line # n
}

ftclHelpDefine $facil plotFromFileLogLin "plotFromFileLogLin file xpos ypos"
proc plotFromFileLogLin {file xpos ypos} {
  plotFromFileWithEverything $file $xpos $ypos # # # # n 1 line # n
}

ftclHelpDefine $facil appendFromFileLogLin "appendFromFileLogLin file xpos ypos"
proc appendFromFileLogLin {file xpos ypos} {
  plotFromFileWithEverything $file $xpos $ypos # # # # n 1 line # y
}

ftclHelpDefine $facil plotPointsFromFile "plotPointsFromFile file xpos ypos"
proc plotPointsFromFile {file xpos ypos {symbol 1}} {
  plotFromFileWithEverything $file $xpos $ypos # # # # n 0 point $symbol n
}

ftclHelpDefine $facil appendPointsFromFile "appendPointsFromFile file xpos ypos"
proc appendPointsFromFile {file xpos ypos {symbol 1}} {
  plotFromFileWithEverything $file $xpos $ypos # # # # n 0 point $symbol y
}

ftclHelpDefine $facil savePlotPointsFromFile "savePlotPointsFromFile file xpos ypos"
proc savePlotPointsFromFile {file xpos ypos {symbol 1}} {
  plotFromFileWithEverything $file $xpos $ypos # # # # y 0 point $symbol n
}

ftclHelpDefine $facil plotPointsFromFileWithLimits "plotPointsFromFileWithL:imits file xpos ypos xmin xmax ymin ymax"
proc plotPointsFromFileWithLimits {file xpos ypos xmin xmax ymin ymax {symbol 1}} {
  plotFromFileWithEverything $file $xpos $ypos $xmin $xmax $ymin $ymax n 0 point $symbol n
}

ftclHelpDefine $facil savePlotPointsFromFileWithLimits "savePlotPointsFromFileWithL:imits file xpos ypos xmin xmax ymin ymax"
proc savePlotPointsFromFileWithLimits {file xpos ypos xmin xmax ymin ymax {symbol 1}} {
  plotFromFileWithEverything $file $xpos $ypos $xmin $xmax $ymin $ymax y 0 point $symbol n
}

proc plotFromFileWithEverything {file xpos ypos xmin xmax ymin ymax save log linetype symbol append} {
  set x ""
  set y ""
  if ($log==0) {set plottype 2} else {set plottype 20}
  set file [open $file r]
  set endfile [gets $file line]
  while {[string compare $endfile -1]} {
    scantoarray line a
    lappend x $a($xpos)
    if ($log==0) {
      lappend y $a($ypos)
    } else {
      lappend y [log10 $a($ypos)]
    }
    set endfile [gets $file line]
  }
  close $file
  if ([string compare $append y]) {
    if ([string compare $save y]) {
      pgBegin
    } else {
      pgBegin /CPS
    }
  }
  pgAsk 0
  if ([string compare $append y]) {
    if (![string compare $xmin #]) {
      if [string compare $x ""] {
        pgEnv [listMin $x] [listMax $x] [listMin $y] [listMax $y] 0 $plottype
      } else {
        echo "Warning - pgEnv not invoked"
      }
    } else {
      pgEnv $xmin $xmax $ymin $ymax 0 2
    }
  }
  if [string compare $x ""] {
    if (![string compare $linetype line]) {
      pgLine $x $y
    } else {
      if (![string compare $linetype point]) {
        pgPoint $x $y $symbol
      }
    }
    if (![string compare $save y]) {echo "Type pgEnd before printing!"}
  } else {
    echo "Warning - no points in file"
  }
}
