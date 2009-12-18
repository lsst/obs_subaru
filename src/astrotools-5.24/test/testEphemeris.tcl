proc testEphemeris {} {

    foreach cmd [list \
	    tst_nextNewMoon tst_nextFullMoon tst_nextNewMoonLocal \
	    tst_moonPhase tst_riseSet tst_mjdRiseSet tst_hourAngle
    ] {
	set value [$cmd]
	if {$value != 0} {
	    error Trouble in $cmd
	    return 1
	}
    }
  return 0
}


proc tst_nextNewMoon {} {
  set date [tstampNew 1999 2 1 0 0 0]
  set newmoon [nextNewMoon $date]

  if ([exprGet $newmoon.year]!=1999) {return 1}
  if ([exprGet $newmoon.month]!=2) {return 1}
  if ([exprGet $newmoon.day]!=16) {return 1}
  tstampDel $date
  tstampDel $newmoon
  return 0
}

proc tst_nextFullMoon {} {
  set date [tstampNew 1997 1 1 0 0 0]
  set fullmoon [nextFullMoon $date]

  if ([exprGet $fullmoon.year]!=1997) {return 1}
  if ([exprGet $fullmoon.month]!=1) {return 1}
  if ([exprGet $fullmoon.day]!=23) {return 1}
  tstampDel $date
  tstampDel $fullmoon
  return 0
}

proc tst_nextNewMoonLocal {} {
  set date [tstampNew 1999 1 1 0 0 0]
  set newMoon1 [ nextNewMoonLocal $date ]
  set newTstamp1 [ keylget newMoon1 TSTAMP ]
  set newDay1 [ keylget newMoon1 MJD ]
  handleSet $newTstamp1.minute 0

  set newMoon2 [ nextNewMoonLocal $newTstamp1 ]
  set newDay2 [ keylget newMoon2 MJD ]

  if { [ expr $newDay1 - $newDay2 ] != 0 } { return1 }
  tstampDel $date
  tstampDel $newTstamp1
  tstampDel [keylget newMoon2 TSTAMP]
  return 0
}

proc tst_moonPhase {} {

  set moonPhase [moonPhase 45.0 20.0 60.0]

  set err [abs [expr $moonPhase - 0.0464]]

  if ($err>0.001) {return 1}

  return 0
}

proc tst_riseSet {} {

  set riseSet [riseSet 45.0 90.0 100.0 33.0 1995.5 0 90.0 rise]

    if {$riseSet != -1} {return 1}

  return 0
}

proc tst_mjdRiseSet {} {

  set riseSet [mjdRiseSet 45.0 90.0 100.0 33.0 50084.25 0 90.0 rise]

    if {$riseSet != -1} {return 1}

  return 0
}

proc tst_hourAngle {} {

  set hourAngle [hourAngle 45.0 0.0 0.0]

  set err [abs [expr $hourAngle-45.0]]

  if ($err>0.01) {return 1}

  return 0
}

