#
# Test the following features of dervish as related to fsaoimage.
#
#	1. saoDisplay
#	2. saoMaskDisplay & saoMaskColorSet
#	3. saoDraw*
#	4. saoDel
#
# NOTE: FSAOIMAGE MUST BE SETUP FOR THIS SCRIPT TO RUN!!!!


# TESTING saoDisplay
set row 40
set col 80
set reg [regNew $row $col -mask]
set k 0
for {set i 0} {$i < $row} {incr i} {
   for {set j 0} {$j < $col} {incr j} {
      set rtn [catch {regPixSetWithDbl $reg $i $j $k} errVal]
      if { $rtn == 1} {
         echo TEST-ERR: Error with regPixSetWithDbl
         echo TEST-ERR: $errVal
         exit
      }
      incr k
   }
}
if {[catch {set fsao [saoDisplay $reg "-geometry +1+1"]} errVal]} {
   echo TEST-ERR: Connecting to display
   echo TEST-ERR: $errVal
   exit
}


# TESTING saoMaskDisplay using a tcl script
set k 0
for {set i 0} {$i < $row} {incr i} {
   for {set j 0} {$j < $col} {incr j} {
      set rtn [catch {maskSetAsPix *($reg.mask) $i $j $k} errVal]
      if { $rtn == 1} {
         echo TEST-ERR: Error with maskSetAsPix
         echo TEST-ERR: $errVal
         exit
      }
      incr k
   }
}
set rtn [catch {saoMaskColorSet {RED BLUE RED BLUE RED BLUE RED BLUE} } errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoMaskColorSet
   echo TEST-ERR: $errVal
   exit
}
saoMaskDisplay *($reg).mask -t {expr {($maskVal&1) ? 1 : 0}}


# TESTING saoDraw* routines
set rtn [catch {regReadAsFits $reg small.fits} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with regReadAsFits
   echo TEST-ERR: $errVal
   exit
}

set rtn [catch {saoDisplay $reg $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with second saoDisplay
   echo TEST-ERR: $errVal
   exit
}

set rad {10 20 30}
set rowVal {66 61 409}
set colVal {378 465 441}
set rtn [catch {saoDrawCircle $rowVal $colVal $rad -s $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}

set rowDiam {40 50 60}
set colDiam {40 50 60}
set rowVal {258 258 258}
set colVal {259 259 259}
set angle {0 120 240}
set rtn [catch {saoDrawBox $rowVal $colVal $rowDiam $colDiam $angle -s $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawBox
   echo TEST-ERR: $errVal
   exit
}

set rtn [catch {saoDrawArrow 258 259 150 130 -s $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawArrow
   echo TEST-ERR: $errVal
   exit
}

set rowVal {74 90 169 143 91 74}
set colVal {159 157 99 60 78 159}
set rtn [catch {saoDrawPolygon 74 159 90 157 169 99 143 60 91 78 74 159 -s $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawPolygon
   echo TEST-ERR: $errVal
   exit
}

# test saoDraw* routines with negative inputs.

set rad {10 20 30}
set rowVal {-66 -61 -209}
set colVal {-78 -165 -141}
set rtn [catch {saoDrawCircle $rowVal $colVal $rad -s $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawCircle
   echo TEST-ERR: $errVal
   exit
}

set rowDiam {-40 -50 -60}
set colDiam {-40 -50 -60}
set rowVal {-158 -158 -158}
set colVal {-159 -159 -159}
set angle {0 -120 -240}
set rtn [catch {saoDrawBox $rowVal $colVal $rowDiam $colDiam $angle -s $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawBox
   echo TEST-ERR: $errVal
   exit
}

set rtn [catch {saoDrawArrow -8 -59 -150 -130 -s $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawArrow
   echo TEST-ERR: $errVal
   exit
}

set rowVal {-74 90 -169 143 -91 74}
set colVal {159 -157 99 -60 78 -159}
set rtn [catch {saoDrawPolygon -74 159  143 -60  90 -157  74 -159  -169 99  -91 78 -s $fsao} errVal]
if { $rtn == 1} {
   echo TEST-ERR: Error with saoDrawPolygon
   echo TEST-ERR: $errVal
   exit
}


# clean up
sleep 4
saoReset $fsao
saoDel $fsao
regDel $reg

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    # Make sure that the only memory still around is from the cbdp
    set handles [lindex $rtn 0]
    set blocks [lindex $rtn 1]
    if {($handles != "")||([lindex [lindex [lindex $blocks 0] 1] 3] != "tclSaoDisplay.c")} {
	echo TEST-ERR: Memory leaks in tst_fsao.tcl
	echo TEST-ERR: $rtn
	error "Error in tst_fsao.tcl"
	return 1
    }
}
return 0
exit









