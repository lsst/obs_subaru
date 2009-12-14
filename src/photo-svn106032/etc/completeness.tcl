# some procs and a script to run through detected stars and compare
#   to catalog stars
# 
# This isn't "production quality" code, but a simple tool or two
#   for quick testing of completeness of detection of objects
#
# Michael Richmond 5/4/95

#
# use AFs and HGs to compare the set of stars in the CHAINs of OBJCs
#   called "catstars" (all stars in catalog) vs. "detstars" (detected stars)
#
proc plothist { catstars detstars magzero } {


    set temp [afFromChain $catstars color<0>->totalCounts]
    set catAf [afMag $magzero $temp]
    afDel $temp
    set temp [afFromChain $detstars color<0>->totalCounts]
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
source [envscan \$PHOTO_DIR]/etc/jpgUtils.tcl

# Attention!
# you should set the following TWO values by hand when you run this script
# the field number 
set ref 3
# the field color
set color r

# this should ALWAYS be zero
set colorindex 0
if { $color == "u" } {
  set magzero 27.73
}
if { $color == "g" } {
  set magzero 28.54 
}
if { $color == "r" } {
  set magzero 28.22
} 
if { $color == "i" } {
  set magzero 27.82 
}
if { $color == "z" } {
  set magzero 26.30 
}

# read in the positions of objects from the catalog
set ff [envscan \$PHOTODATA_DIR]/frames_4/output/high_0_${ref}_1_${color}5.lst
set refobjc [readCatalog $ff -x=-73]

# read in objects from the PHOTO output dump
#set ff [envscan \$PHOTODATA_DIR]/frames_4/output/objclist-0-1-${ref}.dump
#dumpOpen $ff r
#set detobj [dumpHandleRead]
#dumpClose

# read in the corrected image
set ff [envscan \$PHOTODATA_DIR]/frames_4/output/C-0-1-${color}-${ref}.fit
set reg [regReadAsFits [regNew] $ff]
set ff [envscan \$PHOTODATA_DIR]/frames_4/output/C-0-1-${color}-${ref}.fit
set reg [regReadAsFits [regNew] $ff]
set ff [envscan \$PHOTODATA_DIR]/frames_4/output/M-0-1-${color}-${ref}.fit
set mask [maskReadAsFits [maskNew] $ff]
handleSetFromHandle $reg.mask &$mask

# now display it, and display circles around STARS not detected
set sao [saoDisplay $reg "-histeq"]
saoLabel off $sao
saoMaskColorSet {green orange yellow red blue purple magenta cyan}
saoMaskDisplay $mask -p bitset -s $sao


# make a copy of the catalog objects; we'll remove all which aren't stars
set catstars [chainCopy $refobjc]
# and this will be a chain full of all DETECTED stars
set detstars [chainCopy $refobjc]

# first, we remove all non-stars from the two chains
set catcrsr [chainCursorNew $catstars]
set detcrsr [chainCursorNew $detstars]
while { [set objc [chainWalk $catstars $catcrsr NEXT] ] != "" } {

  chainWalk $detstars $detcrsr NEXT

  set rowc [exprGet $objc.color<$colorindex>->rowc]
  set colc [exprGet $objc.color<$colorindex>->colc]
  if { $rowc < 1 || $colc < 1 } {
    continue
  }

  # set "type" to be 0 if the object is a star or quasar, or 1 otherwise
  # the field actually contains catalog type, which doesn't match the enum
#  set type [exprGet $objc.color<$colorindex>->faint_type]
#  if { $type == "(enum) FAINT_UNK" || $type == "(enum) FAINT_STAR" } {
#    set type 0
#  } else {
#    set type 1
#    chainElementRemByCursor $catstars $catcrsr
#    chainElementRemByCursor $detstars $detcrsr
#  }
}
chainCursorDel $catstars $catcrsr
chainCursorDel $detstars $detcrsr

set size [chainSize $catstars]
echo "there are $size stars in the catalog chain "
set size [chainSize $detstars]
echo "there are $size stars in the detected chain "


# now, make a second loop to remove undetected stars from the "detstars"
#
set i 0
set detcrsr [chainCursorNew $detstars]
while { [set objc [chainWalk $detstars $detcrsr NEXT] ] != "" } {

  set rowc [exprGet $objc.color<$colorindex>->rowc]
  set colc [exprGet $objc.color<$colorindex>->colc]
  if { $rowc < 1 || $colc < 1 } {
    continue
  }

  #    note if mag_zero = 28.22, then     mag    20    21   22   23   24
  #                                     counts  1940  772  308  122   49
  set counts [exprGet $objc.color<$colorindex>->totalCounts]

  set rad 0
  if { $counts > 772 } {
    set rad 20 
  }
  if { $rad == 0 && $counts > 308 } {
    set rad 16 
  } 
  if { $rad == 0 && $counts > 122 } {
    set rad 12
  }
  if { $rad == 0 && $counts > 49 } {
    set rad 8
  }
  if { $rad == 0 && $counts < 49 } {
    set rad 4
  }

#  if { $rad < 12 } {
#    continue
#  }
  set rad2 [expr $rad*2]

#  echo "obj $i at $rowc $colc has counts $counts rad $rad, type $type "

   set found 0
   if { [regMaskGetAsPix $reg $rowc $colc] != 0 } {
     set found 1
   }
   if { [regMaskGetAsPix $reg [expr $rowc-1] $colc] != 0 } {
     set found 1
   }
   if { [regMaskGetAsPix $reg [expr $rowc+1] $colc] != 0 } {
     set found 1
   }
   if { [regMaskGetAsPix $reg $rowc [expr $colc-1]] != 0 } {
     set found 1
   }
   if { [regMaskGetAsPix $reg $rowc [expr $colc+1]] != 0 } {
     set found 1
   }
#   if { $found == 0 } {
#     echo "star $i at $rowc $colc has counts $counts, not detected " 
#     saoDrawCircle $rowc $colc $rad -s $sao
#     chainElementRemByCursor $detstars $detcrsr
#   }
   if { $found == 1 } {
     echo "object $i at $rowc $colc has counts $counts, is detected " 
     saoDrawBox $rowc $colc $rad2 $rad2 0 -s $sao
     chainElementRemByCursor $detstars $detcrsr
   }
   set i [expr $i+1]
}
chainCursorDel $detstars $detcrsr
set size [chainSize $detstars]
#echo "after looking, there are $size detected stars"
echo "after looking, there are $size undetected objects"



# end of script
