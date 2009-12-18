###############################################################################
#
# The following procedures are defined below:
#
#    regPlotAsContour
#    regPlotAsContourPS
#
# To use them with the FSAO, they must be hooked up with the 
# mouseDefine command.  Since mouse commands return an extra
# parameter, shell procedures were created and should be specified
# to mouseDefine, ala:
#
#    mouseDefine RECTANGLE_CL regPlotAsContourFsao 
#    mouseDefine RECTANGLE_CL regPlotAsContourPSFsao
#
###############################################################################

###############################################################################
#
#    regPlotAsContourPSFsao
#
###############################################################################

set regPlotAsContourPSFsaoArgs {
   { regPlotAsContourPSFsao "see regPlotAsContourPS" }
   { <reg>      STRING "" reg     "region" }
   { <rowCntr>  STRING "" rowCntr "row"}
   { <colCntr>  STRING "" colCntr "column"}
   { <theta>    STRING "" theta   "theta"}
   { <nrows>    STRING "" nrows   "nrows"}
   { <ncols>    STRING "" ncols   "ncols"}
   { <fsao>     STRING "" fsao    "fsao"}
   { <clist>    STRING "" clist   "clist"}
}

ftclHelpDefine astrotools regPlotAsContourPSFsao [shTclGetHelpInfo regPlotAsContourPSFsao $regPlotAsContourPSFsaoArgs]


proc regPlotAsContourPSFsao { args } {
# "fsao" number returned from mouse click is not used

#
# access help and parse inputs
#
  upvar #0 regPlotAsContourPSFsaoArgs formal_list

  if { [shTclParseArg $args $formal_list regPlotAsContour] == 0 } { return }

#
# now make the plot
#
  regPlotAsContourPS $reg $rowCntr $colCntr $theta $nrows $ncols $clist
}

###############################################################################
#
#    regPlotAsContourPS
#
###############################################################################

set regPlotAsContourPSArgs {
  { regPlotAsContourPS  "see regPlotAsContour\nsame as regPlotAsContour but write to pgPlot.ps" }
  { <reg>     STRING "" reg     "region" }
  { <rowCntr> STRING "" rowCntr "rowCntr" }
  { <colCntr> STRING "" colCntr "colCntr" }
  { <theta>   STRING "" theta   "theta" }
  { <nrows>   STRING "" nrows   "nrows" }
  { <ncols>   STRING "" ncols   "ncols" }
  { <clist>   STRING "" clist   "clist" }
}

ftclHelpDefine astrotools regPlotAsContourPS [shTclGetHelpInfo regPlotAsContourPS $regPlotAsContourPSArgs]

proc regPlotAsContourPS { args } {
	global pg

#
# access help and parse inputs
#
   upvar #0 regPlotAsContourPSArgs formal_list

   if { [shTclParseArg $args $formal_list regPlotAsContourPS] == 0 } { return }

	pgInit /VPS
	regPlotAsContourPS $reg $rowCntr $colCntr $theta $nrows $ncols $clist

}

###############################################################################
#
#    regPlotAsContourFsao
#
###############################################################################

set regPlotAsContourFsaoArgs {
  { regPlotAsContourFsao "see regPlotAsContour" }
  { <reg>     STRING "" reg     "region" }
  { <rowCntr> STRING "" rowCntr "rowCntr" }
  { <colCntr> STRING "" colCntr "colCntr" }
  { <theta>   STRING "" theta   "theta" }
  { <nrows>   STRING "" nrows   "nrows" }
  { <ncols>   STRING "" ncols   "ncols" }
  { <fsao>    STRING "" fsao    "fsao" }
  { <clist>   STRING "" clist   "clist" }
}

ftclHelpDefine astrotools regPlotAsContourFsao [shTclGetHelpInfo regPlotAsContourFsao $regPlotAsContourFsaoArgs]


proc regPlotAsContourFsao { args } {
# "fsao" number returned from mouse click is not used

#
# access help and parse inputs
#
  upvar #0 regPlotAsContourFsaoArgs formal_list

  if { [shTclParseArg $args $formal_list regPlotAsContourFsao] == 0 } { return }

#
# make the plot
#
  regPlotAsContour $reg $rowCntr $colCntr $theta $nrows $ncols $clist
}

###############################################################################
#
#    regPlotAsContour
#
###############################################################################

set regPlotAsContourArgs {
  { regPlotAsContour "To use with FSAO must be hooked with mouseDefine.  Since mouse commands \nreturn an extra parameter, a shell procedure was created and should \nbe specified to mouseDefine:\n\tmouseDefine RECTANGLE_CL regPlotAsContourFsao\n" }
  { <reg>     STRING "" reg     "region" }
  { <rowCntr> STRING "" rowCntr "rowCntr" }
  { <colCntr> STRING "" colCntr "colCntr" }
  { <theta>   STRING "" theta   "theta" }
  { <nrows>   STRING "" nrows   "nrows" }
  { <ncols>   STRING "" ncols   "ncols" }
  { <clist>   STRING "" clist   "1-dimensional array of NC contour levels" }
}

ftclHelpDefine astrotools regPlotAsContour [shTclGetHelpInfo regPlotAsContour $regPlotAsContourArgs]

proc regPlotAsContour { args } {

#
# access help and parse inputs
#
  upvar #0 regPlotAsContourArgs formal_list

  if { [shTclParseArg $args $formal_list regPlotAsContour] == 0 } { return }

#
echo "    "
echo "... contouring..."

#
# Pick up row and col to REALLY iterate over
#
set inf [regInfoGet $reg]
set rowd2 [expr $nrows/2.0]
set r0 [expr $rowCntr-$rowd2]
set r0 [max 0.0 $r0]
set rN [expr $rowCntr+$rowd2]
set rN [min $rN [keylget inf nrow]]
set r0 [int [floor $r0]]
set rN [int [floor $rN]]

set cold2 [expr $ncols/2.0]
set c0 [expr $colCntr-$cold2]
set c0 [max 0.0 $c0]
set cN [expr $colCntr+$cold2]
set cN [min $cN [keylget inf ncol]]
set c0 [int [floor $c0]]
set cN [int [floor $cN]]

# loop over image region getting pixels, in order.
# 	this routine forms an array by forming lists of lists of intensity values.
#
set list {}
loop c $c0 $cN {
   set l {}
   loop r $r0 $rN {
      lappend l [regPixGet $reg $r $c]
   }
   lappend list $l
}

#
# set contour levels
#
if {[llength $clist] == 0} {
	set sreg [subRegNew $reg [expr $rN-$r0] [expr $cN-$c0] $r0 $c0]
	set stats [regStatsFind $sreg]
	set high [keylget stats "high"]
	set ilow [keylget stats "low"]
	set sigma [keylget stats "sigma"]
	set mean [keylget stats mean]
	set low [expr $mean+$sigma]
	regDel $sreg
	

# assume factor of 2 spacing of levels
	set spacing $sigma
	set range [expr $high-$low]
   	set nlevels [expr $range/$spacing]
	set nlevels [int [floor $nlevels]]
	if {$nlevels > 10} {
	   set nlevels 10
        }
	echo $nlevels "= nlevels"	
	echo "range = " $range "sigma = " $sigma
	set clevel $low
	loop level 0 $nlevels {
		set clevel [expr $clevel+$spacing]
		lappend clist $clevel
	}
	echo ""

	echo region inside rl: $r0 rh: $rN  cl: $c0 ch: $cN
	echo "      " stats: low=[floor $low] high=[floor $high]  mean=[floor $mean] sigma=[floor $sigma]
	echo "      " contouring from [floor $low] to [floor $high]
}

pgEnv  [expr $c0+1] $cN $rN [expr $r0+1] 1 1
pgCont $list  1 [expr $cN-$c0] 1 [expr $rN-$r0] $clist [list $c0 1 0 $r0 0 1]
echo $clist
}

# Commented out until we have a way to source this from astrotools startup
#mouseDefine RECTANGLE_CL regPlotAsContourFsao 
