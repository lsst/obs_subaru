# 
#                  dervishPlot2
# This procedure plots either a column or a row of a region.
#  It accepts as arguments : 
#      region    : region name
#      -row/-col : row or column number to be plotted
#      -xmin     : x-axis minimum value
#      -xmax     : x-axis maximum value
#      -ymin     : y-axis minimum value
#      -ymax     : y-axix maximum value
#      -title    : plot title
#      -log      : if specified then y-axis is log scaled
#      -file     : create the plot in the /PS device
#

proc dervishPlot2 {args} {
   ftclFullParse {reg -row -col -xmin -xmax -ymin -ymax -title -log -file} $args

   set region [ftclGetStr reg]
   if {[ftclPresent row] && [ftclPresent col]} {
        echo "Set either -row or -col option"
        return "" }

   if {![ftclPresent row] && ![ftclPresent col]} {
        echo "Set either -row or -col option"
        return "" }

   set ttitle ""
   set lstr ""
   if [ftclPresent log] {set lstr "Log of"}
   set ylabel "$lstr Pixel Value"

   if [ftclPresent row] {
      set row [ftclGetStr row]; set xlabel "Column"; set ttitle "Plot of Row $row"}
   if [ftclPresent col] {
      set col [ftclGetStr col]; set xlabel "Row";    set ttitle "Plot of Column $col"}
   if [ftclPresent title] {set ttitle [ftclGetStr title]}
 
   if [ftclPresent xmin] {set xmin [ftclGetStr xmin]} else {set xmin 0}
   if [ftclPresent xmax] {set xmax [ftclGetStr xmax]} else {
       set regInfo [regInfoGet $region]
       set regNrow [keylget regInfo nrow]
       set regNcol [keylget regInfo ncol]
       if [ftclPresent col] {set xmax $regNrow} 
       if [ftclPresent row] {set xmax $regNcol} }

   if [ftclPresent row] {
      loop col $xmin $xmax {
          set tmp [regPixGet $region $row $col]
          if {![ftclPresent log]} {lappend y [format %012.2f $tmp]}
          if [ftclPresent log] {
             if {$tmp > 0} {lappend y [format %012.2f [log10 $tmp]]} else {
             lappend y [format %012.2f $tmp]}} 
          lappend x $col } }

   if [ftclPresent col] {
      loop row $xmin $xmax {
          set tmp [regPixGet $region $row $col]
          if {![ftclPresent log]} {lappend y [format %012.2f $tmp]}
          if [ftclPresent log] {
             if {$tmp > 0} {lappend y [format %012.2f [log10 $tmp]]} else {
             lappend y [format %012.2f $tmp]}} 
          lappend x $row } }

   if [ftclPresent ymin] {set ymin [ftclGetStr ymin]} else {
      set ymin [lindex [lsort $y] 0] }
   if [ftclPresent ymax] {set ymax [ftclGetStr ymax]} else {
      set ymax [lindex [lsort $y] [expr {[llength $y] - 1}]] }

   if [ftclPresent file] {pgBegin /PS} else {
      if {[pgQinf state] == "CLOSED"} {pgBegin} else {pgPage}}
   pgWindow $xmin $xmax $ymin $ymax
   if [ftclPresent log] {pgBox BCNT 0 0 LBCNTS 0 0} else {pgBox}
   pgLine  $x $y
   pgLabel $xlabel $ylabel $ttitle
   if [ftclPresent file] {pgEnd}
}


