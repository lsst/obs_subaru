# regPixGetStatus
# regPixGetArray
################################################################################
# Get the mean, high and lo pixel values and stdev in a box with the upper
# left, height, and width defined.....   T. Mckay 4/5/93

proc regPixGetStatus {rowwidth colwidth region rowcen colcen} {
  set sum 0
  set npixels 0
  set sumsq 0
  set pixmax 0
  set pixmin -1
  set rowcor [int [floor $rowcen]]
  set colcor [int [floor $colcen]]
  set rInfo [regInfoGet $region]
  set type [keylget rInfo type]
  loop i $rowcor [expr $rowcor+$rowwidth] {
   loop j $colcor [expr $colcor+$colwidth] {
    set pixval [regPixGet $region $i $j]
    if {$pixmin == -1} {set pixmin $pixval}
    lappend pixlist $pixval
    set pixvalsq [expr $pixval*$pixval]
    set sum [expr $sum+$pixval]
    set sumsq [expr $sumsq+$pixvalsq]
    set npixels [expr $npixels+1]
    set pixmax [max $pixmax $pixval]
    set pixmin [min $pixmin $pixval]
   }
  }
  set mean [expr [double $sum]/$npixels]
  set stdevsq [expr $sumsq*$npixels]
  set stdevsq [expr $stdevsq-[expr $sum*$sum]]
  set num $stdevsq
  set denom [expr $npixels*$npixels]
  set denom [expr $denom-$npixels]
  set pixlist [lsort $pixlist]
  set pixelem [llength $pixlist]
  set pixelem [expr $pixelem/2]
  set median [lindex $pixlist $pixelem]
  set stdev [expr [double $num]/$denom]
  
  if {[catch {set stdev [sqrt $stdev]} errStr] != 0} {
      echo $errStr
      echo Error calculating standard deviation.
      set stdev -1.0
  }

  set line ""
  append line " High      Low       Median     Mean      Standard\n"
  append line " Value     Value     Value      Value     Deviation\n"
  append line "---------------------------------------------------\n"
  if {$type != "FL32"} {
      append line [format "%6u" $pixmax]
      append line [format "    %6u" $pixmin]
      append line [format "     %6u" $median]
  } else {
      append line [format "%.3f" $pixmax]
      append line [format "   %.3f" $pixmin]
      append line [format "   %.3f" $median]
  }
  append line [format "    %4.3f" $mean]
  append line [format "     %6f" $stdev]
  return $line
}

################################################################################
# Print the mean, high and lo pixel values and stdev in a box with the upper
# left, height, and width defined..... 

proc regPixPrintStatus {rowwidth colwidth region rowcen colcen} {

    echo [regPixGetStatus $rowwidth $colwidth $region $rowcen $colcen]

}

################################################################################
#
# Define the procedures which will print an array of pixel values surrounding
# the input row and column number
#

proc regPixGetArrayFsao {rowoff coloff reg row col fsao} {
   #
   # NOTE: This procedure is meant to be hooked up to FSAO with mouseDefine. 
   # Since mouse commands return an extra this "shell" procedure was created.
   #
# "fsao" number returned from mouse click is not used
return [regPixGetArray $rowoff $coloff $reg $row $col]
}

proc regPixPrintArrayFsao {rowoff coloff reg row col fsao} {
   #
   # NOTE: This procedure is meant to be hooked up to FSAO with mouseDefine. 
   # Since mouse commands return an extra this "shell" procedure was created.
   #
# "fsao" number returned from mouse click is not used
regPixPrintArray $rowoff $coloff $reg $row $col
}

proc regPixGetArray {rowoff coloff reg row col} {
   set row [int [floor $row]]
   set col [int [floor $col]]
   set rInfo [regInfoGet $reg]
   set nrow [keylget rInfo nrow]
   set ncol [keylget rInfo ncol]
   set type [keylget rInfo type]
   if {$row >= $nrow  || $col >= $ncol} {
      echo "\nRequested portion to print is outside the image."
      echo "Maximum row is $nrow and maximum column is $ncol."
      return
   }

# Set up the boundaries for the pixels that will get output
   set rowmax [expr $row+$rowoff]
   set rowmin [expr $row-$rowoff]
   set colmax [expr $col+$coloff]
   set colmin [expr $col-$coloff]

# Check the boundaries for overrun or underrun
   if {$rowmin < 0} {set rowmin 0}
   if {$colmin < 0} {set colmin 0}
   if {$rowmax >= $nrow} {set rowmax [incr nrow -1]}
   if {$colmax >= $ncol} {set colmax [incr ncol -1]}

# Output the heading
   set line [format "\nRow/Col"]
   for {set i $colmin} {$i <= $colmax} {incr i} {
      append line [format "%6u" $i]
   }
#   echo $line
   append line "\n"
#   echo "-------"
   append line "-------\n"

# Now output the values
   for {set i $rowmin} {$i <= $rowmax} {incr i} {
#      set line [format "%5u |" $i]
      append line [format "%5u |" $i]
      for {set j $colmin} {$j <= $colmax} {incr j} {
	 if {$type != "FL32"} {
	     append line [format "%6u" [regPixGet $reg $i $j]]
	 } else {
	     append line [format "%.2f " [regPixGet $reg $i $j]]
         }
      }
#   echo $line
   append line "\n"
   }

# Now compute the statistics and put them out.  The statistics starts with a
# box whose upper left hand corner is the row and column entered.
append line [regPixGetStatus [expr $rowoff*2+1] [expr $coloff*2+1] $reg $rowmin $colmin]

return $line
}

proc regPixPrintArray {rowoff coloff reg row col} {

    echo [regPixGetArray $rowoff $coloff $reg $row $col]
}

