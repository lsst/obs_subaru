# Before you run this file, you will need to get the ascii list of
# GSC stars out of the VERSANT database.  To do this:
#
# setup versant
# cd /usrdevel/s1/wpeng/catalogSGI/bin    (this should go in the GSC product)
# listStar GSC5 -ra from 185 to 187 -dc from 32.996 to 33.446 > psin.ascii
# >>> Keep typing y<return> until the unix prompt comes back <<<<
# stopdb GSC5
#
# You will then have to edit the ascii file to remove the top header and
# all of the places (every 10 stars or 100 rows) where it asks you if
# you want to see the next 10 stars.  Then start up photo and run this
# script.
#
set file [open psin.ascii r]
set starclist [listNew STARC]
loop i 0 150 {
  lgets $file line
  set id [lindex $line 5][lindex $line 3]
  if {[string length $id] == 0} {break}
  lgets $file line
  set ra [lindex $line 3]
  set dec [lindex $line 4]
  lgets $file line
  set rmag [lindex $line 2]
  lgets $file line
  lgets $file line
  lgets $file line
  lgets $file line
  lgets $file line
  lgets $file line
  lgets $file line
  set umag [expr $rmag+2.5]
  set gmag [expr $rmag+0.8]
  set imag [expr $rmag-0.4]
  set zmag [expr $rmag-0.8]
  set maglist [list $umag $gmag $rmag $imag $zmag]
  set filterlist [list u g r i z]
  set starc [starcInit $id $ra 0.001 $dec 0.0003 $filterlist $maglist {0.1 0.1 0.1 0.1 0.1} #]
  listAdd $starclist $starc
}
echo $starclist
set tbl [starcTbl [llength $filterlist]]
set tblcol [schemaToTbl $starclist $tbl -schemaName=STARC]
fitsWrite $tblcol astrom_stds.fit -binary -pdu=MINIMAL -standard=F
close $file
