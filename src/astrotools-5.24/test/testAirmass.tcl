proc testAirmass {} {

  if {[tst_airmassFind]!=0} {
    error "Trouble in tst_airmassFind"
    return 1
  }
  return 0

}

proc tst_airmassFind {} {

  set tstamp [tstampNew 1995 5 25 4 0 0]
  set airmass [airmassFind 240.55208 31.875 $tstamp]

  set err [abs [expr 1.25145-$airmass]]

  if ($err>0.01) {
      echo trouble in tst_airmassFind:  airmass=$airmass
      return 1
  }
  
  tstampDel $tstamp

  return 0
}

proc tst_meridianAirmassFind {} {

  set airmass [meridianAirmassFind -18.5]

  set err [abs [expr 1.596383-$airmass]]

  if ($err>0.01) {return 1}

  return 0
}

proc tst_airmassFromZd {} {

  set airmass [airmassFromZd 60.0]

  set err [abs [expr 1.99450-$airmass]]

  if ($err>0.01) {return 1}

  return 0
}

proc tst_airmassFindHaDec {} {

  set airmass [airmassFindHaDec 1.0 -18.5]

  set err [abs [expr 1.668501-$airmass]]

  if ($err>0.01) {return 1}

  return 0
}

proc tst_zdDec2Ha {} {

  set ha [zdDec2Ha 10.0 32.5]

  set err [abs [expr 0.791799-$ha]]

  if ($err>0.01) {return 1}

  return 0
}
