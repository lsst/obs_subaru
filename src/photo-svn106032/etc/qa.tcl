#
# modified slightly by MWR to add interactive object descriptions
#

proc frames_results {outputdir run column color icolor frame} {
  global objectList iframe

  # Read in the mask, the image, and the list of objects
  set reg [regReadAsFits [regNew] $outputdir/C-$run-$column-$color-$frame.fit]
  set mask [maskReadAsFits [maskNew] $outputdir/M-$run-$column-$color-$frame.fit]
  dumpOpen $outputdir/objclist-$run-$column-$frame.dump r
  set objectList [dumpHandleRead]
  dumpClose


# decompress objclist 
  objcRiceDecompress $objectList
  # Display the image with mask and the noise image.  Label the objects on
  #  the image.
  set sao [saoDisplay $reg "-histeq -lprbuttons"]
  saoMaskColorSet {green orange yellow red blue purple magenta cyan}
  saoMaskDisplay $mask -p bitset -s $sao
  saoLabel off $sao
  
  set size [chainSize $objectList]
  loop i 0 $size {
     set objc [chainElementGetByPos $objectList $i]
     if { [exprGet $objc.color<$icolor>] != 0 } {

       set rs [exprGet $objc.color<$icolor>->xmin]
       set re [exprGet $objc.color<$icolor>->xmax]
       set cs [exprGet $objc.color<$icolor>->ymin]
       set ce [exprGet $objc.color<$icolor>->ymax]

       if { [exprGet $objc.color<$icolor>->blend] != 0 }  {
            saoDrawPolygon $rs $cs $re $cs $re $ce $rs $ce $rs $cs -e $sao
       } else {
            saoDrawPolygon $rs $cs $re $cs $re $ce $rs $ce $rs $cs $sao
       }
     }
     handleDel $objc
  }

  # now, set up link to mouse button and tell the user 
  set iframe $icolor
  mouseDefine POINT_L frames_mouseproc_text
  mouseDefine POINT_M frames_mouseproc_display
  echo "enter Cursor mode "
  echo "click LEFT button at object of interest for text"
  echo "click MIDDLE button at object of interest for display "
}

proc framestat_results {outputdir run column startframe endframe ncolors} {
  # Read in each framestat and put it on a chain
  set framestatlist [chainNew FRAMESTAT]
  set framestatTbl [framestatTbl $ncolors]
  loop i $startframe [expr $endframe+1] {
    set framestat [framestatNew $ncolors]
    set tblcol [handleNewFromType TBLCOL]
    fitsRead $tblcol $outputdir/framestat-$run-$column-$i.fit -binary -hdu=1
    tblToSchema $tblcol $framestat $framestatTbl -proc={framestatNew $ncolors} -schemaName=FRAMESTAT
    chainElementAddByPos $framestatlist $framestat TAIL AFTER
    handleDelFromType $tblcol
  }
  schemaTransDel $framestatTbl
  set frameNums [afFromChain $framestatlist frameNum]
  set nobjects [afFromChain $framestatlist nobjects]
  set nstars [afFromChain $framestatlist nstars]
  set ngals [afFromChain $framestatlist ngals]
  set pgstate [pgstateNew]
  pgstateOpen $pgstate
  pgstateSet $pgstate "-plotTitle=Objects from run $run" -symb=2 -icError=0
  set plotmax [afExtreme $nobjects max]
  afPlot $pgstate $nobjects $frameNums -ymin=0 -ymax=$plotmax
  afPlotLegend 0 2 nobjects $startframe $endframe 0 $plotmax
  pgstateSet $pgstate -isNewplot=0 -symb=4
  afPlot $pgstate $ngals $frameNums
  afPlotLegend 1 4 ngals $startframe $endframe 0 $plotmax
  pgstateSet $pgstate -isNewplot=0 -symb=3
  afPlot $pgstate $nstars $frameNums
  afPlotLegend 2 3 nstars $startframe $endframe 0 $plotmax
  afDel $nstars
  afDel $ngals
  afDel $nobjects
  # This section will be much easier when afFromChain takes handle expr
  set framestat [chainElementGetByPos $framestatlist HEAD]
  set ncolors [exprGet $framestat.ncolors]
  handleDel $framestat
  loop color 0 $ncolors {
    # For now, get lists of FRSTATs
    set frstatlist [frstatFromFramestat $framestatlist $color]
    set maxpix [afFromChain $frstatlist maxpix]
    pgstateSet $pgstate \
      "-plotTitle=Maximum pixel from run $run color $color" -symb=2 \
      -isNewplot=1
    afPlot $pgstate $maxpix $frameNums -ymin=0 -ymax=[afExtreme $maxpix max]
    set minpix [afFromChain $frstatlist minpix]
    set meanpix [afFromChain $frstatlist meanpix]
    set sigpix [afFromChain $frstatlist sigpix]
    set sky [afFromChain $frstatlist sky]
    set plotmax [afExtreme $minpix max]
    if ([afExtreme $meanpix max]>$plotmax) \
      {set plotmax [afExtreme $meanpix max]}
    if ([afExtreme $sigpix max]>$plotmax) \
      {set plotmax [afExtreme $sigpix max]}
    if ([afExtreme $sky max]>$plotmax) \
      {set plotmax [afExtreme $sky max]}
    pgstateSet $pgstate \
      "-plotTitle=Frame statistics from run $run color $color" -symb=4 \
      -isNewplot=1
    afPlot $pgstate $minpix $frameNums -ymin=0 -ymax=$plotmax
    afPlotLegend 0 4 minpix $startframe $endframe 0 $plotmax
    pgstateSet $pgstate -symb=3 -isNewplot=0
    afPlot $pgstate $meanpix $frameNums
    afPlotLegend 1 3 meanpix $startframe $endframe 0 $plotmax
    pgstateSet $pgstate -symb=2
    afPlot $pgstate $sigpix $frameNums
    afPlotLegend 2 2 sigpix $startframe $endframe 0 $plotmax
    pgstateSet $pgstate -symb=5
    afPlot $pgstate $sky $frameNums
    afPlotLegend 3 5 sky $startframe $endframe 0 $plotmax
    set nbadpix [afFromChain $frstatlist nbadpix]
    pgstateSet $pgstate \
      "-plotTitle=Bad pixels from run $run color $color" -symb=2 -isNewplot=1
    afPlot $pgstate $nbadpix $frameNums -ymin=0 -ymax=[afExtreme $nbadpix max]
    set nbrightobj [afFromChain $frstatlist nbrightobj]
    set nfaintobj [afFromChain $frstatlist nfaintobj]
    set brightmax [afExtreme $nbrightobj max]
    set faintmax [afExtreme $nfaintobj max]
    if ($brightmax>$faintmax) {
      set plotmax $brightmax 
      } else {
      set plotmax $faintmax
    }
    pgstateSet $pgstate \
      "-plotTitle=Objects found in run $run color $color" -symb=2 \
      -isNewplot=1
    afPlot $pgstate $nbrightobj $frameNums -ymin=0 -ymax=$plotmax
    afPlotLegend 0 2 nbrightobj $startframe $endframe 0 $plotmax
    pgstateSet $pgstate -isNewplot=0 -symb=4
    afPlot $pgstate $nfaintobj $frameNums
    afPlotLegend 1 4 nfaintobj $startframe $endframe 0 $plotmax
    afDel $minpix; afDel $maxpix; afDel $meanpix; afDel $sigpix; afDel $sky
    afDel $nbadpix; afDel $nbrightobj; afDel $nfaintobj
  }
  echo $framestatlist
  chainDestroy $framestatlist framestatDel
}

proc afPlotLegend {pos symb title xmin xmax ymin ymax} {
  set dx [expr ($xmax-$xmin)/40]
  if ($dx==0) {set dx 0.05}
  set dy [expr ($ymax-$ymin)/20]
  set x [expr $xmin+$dx]
  set y [expr $ymax-$pos*$dy]
  pgPoint $x $y $symb
  pgText [expr $x+$dx] $y $title
}

proc frstatFromFramestat {framestatlist color} {
  set frstatlist [chainNew FRSTAT]
  set size [chainSize $framestatlist]
  loop i 0 $size {
    set framestat [chainElementGetByPos $framestatlist $i]
     set frstat [handleBindFromHandle [handleNew] *$framestat.filter<$color>]
    chainElementAddByPos $frstatlist $frstat TAIL AFTER
    echo [exprGet $frstat]
    handleDel $frstat
  }
  return $frstatlist
}

#
# There are some global vars needed to pass information between procs in
# this file, but we'll declare them explicitly rather than using a function
# such as set_globals to declare them. It's easier to read
#
# proc set_globals {} {
#  uplevel global objectList iframe
# }

# this proc handles output from the mouse, printing out objc data
proc frames_mouseproc_text { reg x y saonum } {
   global objectList iframe

   set objc [objcClosest $objectList $x $y $iframe]
   objcPrintPretty $objc
}

# this proc takes output from the mouse and spawns a new saoDisplay 
# window which shows the nearest object only.
proc frames_mouseproc_display { reg x y saonum } {
   global objectList iframe

   set objc [objcClosest $objectList $x $y $iframe]
   loop icolor 0 [exprGet $objc.ncolor] {
     if { [exprGet $objc.color<$icolor>] != 0 } {
	set region [handleBindFromHandle [handleNew] \
		    *$objc.color<$icolor>->region] 
	set mask [handleBindFromHandle [handleNew] \
		      *$objc.color<$icolor>->mask] 
       set sao [saoDisplay $region "-histeq"]
       saoMaskDisplay $mask -p bitset -s $sao
#        handleDel $mask
#        handleDel $region
	echo region=$reg mask=$mask
     }

   }

}

