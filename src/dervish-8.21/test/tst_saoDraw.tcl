
set rgn [regNew]
saoDisplay [regReadAsFits $rgn small.fits] "-geometry +1+1"
regDel $rgn

set row [list 100 200 300 400 500]
set col [list 100 200 300 400 500]
set rrad [list 10 20 30 40 50]
set crad [list 20 30 40 50 60]
set angle [list 0 20 40 60 80]
set row2 [list 110 220 330 440 550]
set col2 [list 110 220 330 440 550]
set text [list "mike" "cambot" "gypsy" "tom servo" "crooooow"]

set rtn [catch {saoDrawCircle $row $col $rrad} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset
set rtn [catch {saoDrawCircle $row $col $rrad -e} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset

set rtn [catch {saoDrawEllipse $row $col $rrad $crad $angle} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset
set rtn [catch {saoDrawEllipse $row $col $rrad $crad $angle -e} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset

set rtn [catch {saoDrawBox $row $col $rrad $crad $angle} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset
set rtn [catch {saoDrawBox $row $col $rrad $crad $angle -e} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset

set rtn [catch {saoDrawPolygon $row $col $row $col2 $row2 $col} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset
set rtn [catch {saoDrawPolygon $row $col $row $col2 $row2 $col -e} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset

set rtn [catch {saoDrawPoint $row $col $row $col2 $row2 $col} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset
set rtn [catch {saoDrawPoint $row $col $row $col2 $row2 $col -e} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset

set rtn [catch {saoDrawArrow $row $col $rrad $angle} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset
set rtn [catch {saoDrawArrow $row $col $rrad $angle -e} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset

set rtn [catch {saoDrawText $row $col $text} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset
set rtn [catch {saoDrawText $row $col $text -e} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}
saoReset


saoDel -ai
# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    # Make sure that the only memory still around is from the cbdp
    set handles [lindex $rtn 0]
    set blocks [lindex $rtn 1]
    if {($handles != "")||([lindex [lindex [lindex $blocks 0] 1] 3] != "tclSaoDisplay.c")} {
	echo TEST-ERR: Memory leaks in tst_saoDraw.tcl
	echo TEST-ERR: $rtn
	error "Error in tst_saoDraw.tcl"
	return 1
    }
}
return 0
exit
