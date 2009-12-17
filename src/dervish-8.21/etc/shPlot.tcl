# dervishPlot
# xsectionPlot
# ePlot equivalent of vPlot for expression.

###############################################################################
###############################################################################
# 
#                  dervishPlot
#
# Define the formal arguments for the command - See man ftclParseArg
#
set dervishPlotArgs {
   {dervishPlot "Plots either a column or a row (or both) of a region\n" }
   {<region>    STRING "" region "region name"}
   {-row  STRING "" row "row number to be plotted"}
   {-col  STRING "" col "column number to be plotted"}
   {-xmin STRING 0  xmin "x-axis minimum value"}
   {-xmax STRING "" xmax "x-axis maximum value"}
   {-ymin STRING "" ymin "y-axis minimum value"}
   {-ymax STRING "" ymax "y-axis maximum value"}
   {-point STRING "" point "plot w/ points (default: dot), not lines " }
   {-title STRING "" title "plot title when plotting row or col x-section (not both)"}
   {-coltitle STRING "" coltitle "plot title when plotting col x-section"}
   {-log   CONSTANT "" log "set y axis on logscale"}
   {-file  STRING "" file "create the plot in the /PS device"}
   {-sec   STRING 7  secs "seconds to wait in between plots (if plotting 2)"}
   {-overplot CONSTANT "" overplot "plot on top of previous graph" }

}

# Define help for the command at the outer layer - Dervish initialization time
#   ftclHelpDefine <module> < cmdname> <help string>
#   shTclGetHelpInfo <cmdname> <formal arg list>
#
ftclHelpDefine shPlot dervishPlot [shTclGetHelpInfo dervishPlot $dervishPlotArgs]

# The procedure rcPlot is used by dervishPlot
proc rcPlot { x y xmin xmax ymin ymax xlabel ylabel title file log point \
                       overplot } {
   if {$ymin == ""} {set ymin [lindex [lsort $y] 0] }
   if {$ymax == ""} {set ymax [lindex [lsort $y] [expr {[llength $y] - 1}]] }
   if {$overplot == 0} {
     if {$file != ""} {pgBegin /PS} else {
        if {[pgQinf state] == "CLOSED"} {pgBegin} else {pgPage}}
   }
   pgWindow $xmin $xmax $ymin $ymax
   if {$log != ""} {pgBox BCNT 0 0 LBCNTS 0 0} else {pgBox}
   if {$point != ""} {pgPoint $x $y $point} else {pgLine  $x $y}
   pgLabel $xlabel $ylabel $title
   if {$file != ""} {pgEnd}
  }

proc dervishPlot {args} {
# We need to access the level 0 dervishPlotArgs argument list
   upvar #0 dervishPlotArgs formal_list
#
# Parse the argument list according to the formal specification
# shTclParseArg intercepts errors and help requests and provides help
# so we can exit on errors.
#
   if {[shTclParseArg $args $formal_list dervishPlot] == 0} {
      return
   }
   if {![ftclPassed $args $formal_list -row] && ![ftclPassed $args $formal_list -col]} {
        echo "Set either -row or -col option"
        return "" 
   }
   
   set lstr ""
   if [ftclPassed $args $formal_list -log] {
       set lstr "Log of"
       set log yes 
   } else {
       set log ""
   }
   set ylabel "$lstr Pixel Value"

   if [ftclPassed $args $formal_list -point] {
      if {![ctype digit $point]} { set point -1 }
   }
 
   if [ftclPassed $args $formal_list -xmax] {
       set regNcol $xmax
       set regNrow $regNcol
   } else {
       set regInfo [regInfoGet $region]
       set regNrow [keylget regInfo nrow]
       set regNcol [keylget regInfo ncol] 
   }

   if [ftclPassed $args $formal_list -overplot] {
       set overplot 1
   } else {
       set overplot 0
   }

   set cymax $ymax 
   set cymin $ymin
   set x ""
   set y ""
   if {$row != ""} {
      set xlabel "Column"
      if {$title != ""} {set rtitle $title} else {
	  set rtitle "Plot of Row $row" }
      set xmax $regNcol
      loop lcol $xmin $xmax {
          set tmp [regPixGet $region $row $lcol]
          if {$log == ""} {lappend y [format %016.6f $tmp]} else {
             if {$tmp > 0} {lappend y [format %016.6f [log10 $tmp]]} else {
		 lappend y [format %016.6f $tmp]}}
	  lappend x $lcol }
      rcPlot $x $y $xmin $xmax $ymin $ymax $xlabel $ylabel $rtitle $file $log $point $overplot
      if {$col != ""} {sleep $secs}
   }

   if {$col != ""} {
      set xlabel "Row"
      if {$coltitle != ""} {set ctitle $coltitle} else {
	  if {$title != ""} {set ctitle $title} else {
	      set ctitle "Plot of Column $col" }}
      set xmax $regNrow
      loop row $xmin $xmax {
          set tmp [regPixGet $region $row $col]
          if {$log == ""} {lappend cy [format %016.6f $tmp]} else {
             if {$tmp > 0} {lappend cy [format %016.6f [log10 $tmp]]} else {
		 lappend cy [format %016.6f $tmp]}} 
          lappend cx $row }
      rcPlot $cx $cy $xmin $xmax $cymin $cymax $xlabel $ylabel $ctitle $file $log $point $overplot
   }
}
#
# Procedure to plot a slice of an image either along a row or along a column
#

# The dervishPlot procedure has been expanded to do the plotting of both a row
# and a column cross-section with one call.  Therefore this routine is being 
# gutted and will exist only as a shell which calls dervishPlot.  We need this
# routine to capture plotting commands issued from the mouse commands on
# fsao. The only differebce between the two is we have
# to get rid of the 'choice' argument which is the first argument and we have
# to add a '-row' and/or '-col' depending on what is wanted.
set xPlotArgs {
   {xsectionPlot "Plot a slice of an image along a row and a column\n"} 
   {<choice> STRING "" choice "R, C, or B  (i.e., Row, Column, or Both)"}
   {<region> STRING "" region "region name"}
   {<row>    STRING "" row    "row number to be plotted"}
   {<col>    STRING "" col    "column number to be plotted"}
   {<fsao>   STRING "" fsao   "fsao number (ignored - needed since mouse commands return it)"}
   {-xmin    STRING 0  xmin   "x-axis minimum value"}
   {-xmax    STRING "" xmax   "x-axis maximum value"}
   {-ymin    STRING "" ymin   "y-axis minimum value"}
   {-ymax    STRING "" ymax   "y-axis maximum value"}
   {-point   STRING "" point  "plot w/ points (default: dot), not lines " }
   {-title   STRING "" title  "plot title when plotting row or col x-section (not both)"}
   {-coltitle STRING "" coltitle "plot title when plotting col x-section"}
   {-log     STRING "" log    "set y axis on logscale"}
   {-file    STRING "" file   "create the plot in the /PS device"}
   {-sec     STRING 7  secs   "seconds to wait in between plots (if plotting 2)"}
}

ftclHelpDefine shPlot xsectionPlot [shTclGetHelpInfo xsectionPlot $xPlotArgs]

proc xsectionPlot {choice reg row col fsao args} {
# NOTE: The "fsao" number returned from mouse click is not used
   upvar #0 dervishPlotArgs dervishPlotArgs
   if {$choice == "b"} {set choice "B"}
   if {$choice == "c"} {set choice "C"}
   if {$choice == "r"} {set choice "R"}

   if {$choice == "R"} {eval dervishPlot $reg -row $row $args} else {
      if {$choice == "C"} {eval dervishPlot $reg -col $col $args} else {
         if {$choice == "B"} {eval dervishPlot $reg -row $row -col $col $args}}}
}



###############################################################################
###############################################################################
# 
#                  ePlot
#
set ePlotArgs {
   {ePlot "Plots two expressions\n" }
   {<pgstate> STRING "" pgstate "pgstate to write on"}
   {<exprX> STRING "" exprX "X vector"}
   {<exprY> STRING "" exprY "Y vector"}
   {-exprXErr  STRING "" exprXErr "error on X"}
   {-exprYErr STRING "" exprYErr "error on Y"}
   {-exprMask STRING "" exprMask "mask values, set to 1 for good points"}
   {-exprSymbol STRING "" exprSymbol "PGPLOT symbol values"}
   {-exprColor STRING "" exprColor "PGPLOT color values"}
   {-xmin DOUBLE 0 xmin "minimum x of the plot"}
   {-xmax DOUBLE 0 xmax "maximum x of the plot"}
   {-ymin DOUBLE 0 ymin "minimum y of the plot"}
   {-ymax DOUBLE 0 ymax "maximum y of the plot"}
}      

ftclHelpDefine shEPlot ePlot [shTclGetHelpInfo ePlot $ePlotArgs]

proc ePlot {args} {
   upvar #0 ePlotArgs formal_list
   if {[shTclParseArg $args $formal_list ePlot] == 0} {
      return
   }

   # We don't know how to efficiently get the size of the expressions 
   # So we could the expression evaluator to get a 'decent' looking size
   # this is a slow solution.
   # Try this for Jeff Munn - changed by Bruce Greenawalt
   set DefaultMaxSize [exprEval $exprX]
   echo $DefaultMaxSize
   #
   # We actually rather use a fixed maximum size and then resize the
   # vectors
   # set DefaultMaxSize 32767

#   set vectorX_ [vectorExprNew $DefaultMaxSize]
#   set vectorY_ [vectorExprNew $DefaultMaxSize]
#   set sizex [exprEval "$vectorX_ = $exprX"]
#   if { $sizex >= $DefaultMaxSize } {
#       echo WARNING! 
#       echo ONLY $DefaultMaxSize elements are being processed !!
#       echo Please report this problem to sdss-bugs@sdss.fnal.gov
#   }
#   set sizey [exprEval "$vectorY_ = $exprY"]
#   if { $sizey >= $DefaultMaxSize } {
#       echo WARNING! 
#       echo ONLY $DefaultMaxSize elements are being processed !!
#       echo Please report this problem to sdss-bugs@sdss.fnal.gov
#   }
#   # Resize the vector to their Usefull size!
#   set vectorX [vectorExprNew $sizex]
#   exprEval "$vectorX = $vectorX_"
#   set vectorY [vectorExprNew $sizey]
#   exprEval "$vectorY = $vectorY_"
#   
#   vectorExprDel $vectorX_
#   vectorExprDel $vectorY_

   # Resize the vector to their Usefull size!
   set vectorX [vectorExprNew $DefaultMaxSize]
   set vectorY [vectorExprNew $DefaultMaxSize]
   exprEval "$vectorX = $exprX"
   exprEval "$vectorY = $exprY"
   

   set Arguments "$pgstate $vectorX $vectorY"
   set cleanScript "vectorExprDel $vectorX; vectorExprDel $vectorY"

   if {[ftclPassed $args $formal_list -exprXErr] } {
       set vectorXErr [vectorExprNew $DefaultMaxSize]
       exprEval "$vectorXErr = $exprXErr"
       #vectorExprEval \{[exprGet (DOUBLE<$DefaultMaxSize>)*$vectorXErr.vec -unfold -flat]\}
       set Arguments "$Arguments -vectorXErr $vectorXErr"
       set cleanScript "$cleanScript ; vectorExprDel $vectorXErr"
   }
   if {[ftclPassed $args $formal_list -exprYErr] } {
       set vectorYErr [vectorExprNew $DefaultMaxSize]
       exprEval "$vectorYErr = $exprYErr"
       #vectorExprEval \{[exprGet (DOUBLE<$DefaultMaxSize>)*$vectorYErr.vec -unfold -flat]\}
       set Arguments "$Arguments -vectorYErr $vectorYErr"
       set cleanScript "$cleanScript ; vectorExprDel $vectorYErr"
   }
   if {[ftclPassed $args $formal_list -exprMask] } {
       set vectorMask [vectorExprNew $DefaultMaxSize]
       exprEval "$vectorMask = $exprMask"
       #vectorExprEval \{[exprGet (DOUBLE<$DefaultMaxSize>)*$vectorMask.vec -unfold -flat]\}
       set Arguments "$Arguments -vectorMask $vectorMask"
       set cleanScript "$cleanScript ; vectorExprDel $vectorMask"
   }   
   if {[ftclPassed $args $formal_list -exprSymbol] } {
       set vectorSymbol [vectorExprNew $DefaultMaxSize]
       exprEval "$vectorSymbol = $exprSymbol"
       #vectorExprEval \{[exprGet (DOUBLE<$DefaultMaxSize>)*$vectorSymbol.vec -unfold -flat]\}
       set Arguments "$Arguments -vectorSymbol $vectorSymbol"
       set cleanScript "$cleanScript ; vectorExprDel $vectorSymbol"
   }   
   if {[ftclPassed $args $formal_list -exprColor] } {
       set vectorColor [vectorExprNew $DefaultMaxSize]
       exprEval "$vectorColor = $exprColor"
       #vectorExprEval \{[exprGet (DOUBLE<$DefaultMaxSize>)*$vectorColor.vec -unfold -flat]\}
       set Arguments "$Arguments -vectorColor $vectorColor"
       set cleanScript "$cleanScript ; vectorExprDel $vectorColor"
   }   
   if [ftclPassed $args $formal_list -xmin] {
      set Arguments "$Arguments -xmin $xmin" 
   }
   if [ftclPassed $args $formal_list -xmax] {
      set Arguments "$Arguments -xmax $xmax" 
   }
   if [ftclPassed $args $formal_list -ymin] {
      set Arguments "$Arguments -ymin $ymin" 
   }
   if [ftclPassed $args $formal_list -ymax] {
      set Arguments "$Arguments -ymax $ymax" 
   }
   
   eval vPlot $Arguments
   eval $cleanScript
}


ftclHelpDefine dervish pgInit "
Initialize the pg variable to be a PGSTATE
USAGE:  pgInit \[device\]
where the default device is /xwindow

OUTPUT:  set the global variable pg
"

proc pgInit { {device /xwindow} } {
    global pg
    set pg [pgstateNew]
    handleSet $pg.device $device
    pgstateSet $pg -icError 0 -symb 1
    pgstateOpen $pg
    return
}
