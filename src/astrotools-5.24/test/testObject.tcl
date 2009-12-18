set M_PI 3.1415926535

proc testObject {} {

  if {[tst_daCentroidFind 29.]!=0} {
    error "Trouble in tst_daCentroidFind"
    return 1
  }
  if {[tst_daCentroidFind -9.]!=0} {
    error "Trouble in tst_daCentroidFind"
    return 1
  }
  if {[tst_gaussianWidthFind 29.]!=0} {
    error "Trouble in tst_gaussianWidthFind"
    return 1
  }
  if {[tst_gaussianWidthFind -9.]!=0} {
    error "Trouble in tst_gaussianWidthFind"
    return 1
  }
  if {[tst_centroidFind 29.]!=0} {
    error "Trouble in tst_centroidFind"
    return 1
  }
  if {[tst_centroidFind -9.]!=0} {
    error "Trouble in tst_centroidFind"
    return 1
  }
  if {[tst_2ndMoments 29.]!=0} {
    error "Trouble in tst_2ndMoments"
    return 1
  }
  if {[tst_2ndMoments -9.]!=0} {
    error "Trouble in tst_2ndMoments"
    return 1
  }
  return 0

}

proc tst_daCentroidFind {theta} {
global M_PI

  set rowpos 50.
  set colpos 50.
  set reg [regNew 101 101]
  regSetWithDbl $reg 0
  regGaussiansAdd $reg $rowpos $colpos 1000 -rsig 1.0 -csig 1.5
  regMap $reg -theta $theta
  set newrow [expr 50.5+($rowpos-50.5)*[cos $theta*$M_PI/180.]+($colpos-50.5)*[sin [expr $theta*$M_PI/180.]]]
  set newcol [expr 50.5-($rowpos-50.5)*[sin $theta*$M_PI/180.]+($colpos-50.5)*[cos [expr $theta*$M_PI/180.]]]
  set list [daCentroidFind $reg $newrow $newcol 0 3 0 0]

  set row [keylget list row]
  set col [keylget list col]
  set asig [keylget list asig]
  set bsig [keylget list bsig]
  set pa [keylget list pa]
  if ($row<[expr $newrow-.1]||$row>[expr $newrow+.1]) {error "tst_daCentroidFind failed"; return 1}
  if ($col<[expr $newcol-.1]||$col>[expr $newcol+.1]) {error "tst_daCentroidFind failed"; return 1}
  if ($asig<1.4||$asig>1.6) {error "tst_daCentroidFind failed"; return 1}
  if ($bsig<.9||$bsig>1.1) {error "tst_daCentroidFind failed"; return 1}
  if ($pa<($theta-1)||$pa>($theta+1)) {error "tst_daCentroidFind failed"; return 1}

  regDel $reg

  return 0
}

proc tst_gaussianWidthFind {theta} {
global M_PI

  set rowpos 50.
  set colpos 50.
  set reg [regNew 101 101]
  regSetWithDbl $reg 0
  regGaussiansAdd $reg $rowpos $colpos 1000 -rsig 1.0 -csig 1.5
  regMap $reg -theta $theta
  set newrow [expr 50.5+($rowpos-50.5)*[cos $theta*$M_PI/180.]+($colpos-50.5)*[sin [expr $theta*$M_PI/180.]]]
  set newcol [expr 50.5-($rowpos-50.5)*[sin $theta*$M_PI/180.]+($colpos-50.5)*[cos [expr $theta*$M_PI/180.]]]
  set list [gaussianWidthFind $reg $newrow $newcol 0]

  set row [keylget list row]
  set col [keylget list col]
  set asig [keylget list asig]
  set bsig [keylget list bsig]
  set pa [keylget list pa]
  if ($row<[expr $newrow-.1]||$row>[expr $newrow+.1]) {error "tst_gaussianWidthFind failed"; return 1}
  if ($col<[expr $newcol-.1]||$col>[expr $newcol+.1]) {error "tst_gaussianWidthFind failed"; return 1}
  if ($asig<1.4||$asig>1.6) {error "tst_gaussianWidthFind failed"; return 1}
  if ($bsig<.9||$bsig>1.1) {error "tst_gaussianWidthFind failed"; return 1}
  if ($pa<($theta-1.)||$pa>($theta+1.)) {error "tst_gaussianWidthFind failed"; return 1}

  regDel $reg

# Test on a completely flat region
  set reg [regNew 25 25]
  regSetWithDbl $reg 10
  gaussianWidthFind $reg 10 10 9

  regDel $reg
  return 0
}

proc tst_centroidFind {theta} {
global M_PI

  set rowpos 50.
  set colpos 50.
  set reg [regNew 101 101]
  regSetWithDbl $reg 0
  regGaussiansAdd $reg $rowpos $colpos 1000 -rsig .6 -csig 2.6
  regMap $reg -theta $theta
  set newrow [expr 50.5+($rowpos-50.5)*[cos $theta*$M_PI/180.]+($colpos-50.5)*[sin [expr $theta*$M_PI/180.]]]
  set newcol [expr 50.5-($rowpos-50.5)*[sin $theta*$M_PI/180.]+($colpos-50.5)*[cos [expr $theta*$M_PI/180.]]]
  set list [centroidFind $reg $newrow $newcol 0 3 0 0]

  set row [keylget list row]
  set col [keylget list col]
  set asig [keylget list asig]
  set bsig [keylget list bsig]
  set pa [keylget list pa]
  if ($row<[expr $newrow-.1]||$row>[expr $newrow+.1]) {error "tst_centroidFind failed"; return 1}
  if ($col<[expr $newcol-.1]||$col>[expr $newcol+.1]) {error "tst_centroidFind failed"; return 1}
  if ($asig<2.5||$asig>2.7) {error "tst_centroidFind failed"; return 1}
  if ($bsig<.5||$bsig>.8) {error "tst_centroidFind failed"; return 1}
  if ($pa<($theta-1.)||$pa>($theta+1.)) {error "tst_centroidFind failed"; return 1}

  regDel $reg

  return 0
}

proc tst_2ndMoments {theta} {
global M_PI

  set rowpos 50.
  set colpos 50.
  set reg [regNew 101 101]
  regSetWithDbl $reg 0
  regGaussiansAdd $reg $rowpos $colpos 1000 -rsig .6 -csig 2.6
  regMap $reg -theta $theta
  set newrow [expr 50.5+($rowpos-50.5)*[cos $theta*$M_PI/180.]+($colpos-50.5)*[sin [expr $theta*$M_PI/180.]]]
  set newcol [expr 50.5-($rowpos-50.5)*[sin $theta*$M_PI/180.]+($colpos-50.5)*[cos [expr $theta*$M_PI/180.]]]

  set list [2ndMoments $reg $newrow $newcol 0 11 11]

  set asig [keylget list asigma]
  set bsig [keylget list bsigma]
  set pa [keylget list pa]
  if ($asig<2.3||$asig>2.9) {error "tst_2ndMoments failed"; return 1}
  if ($bsig<.5||$bsig>.9) {error "tst_2ndMoments failed"; return 1}
  if ($pa<($theta-1.)||$pa>($theta+1.)) {error "tst_2ndMoments failed"; return 1}

  regDel $reg

  return 0

}
