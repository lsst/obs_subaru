#
# atNextNewMoonLocal
#

# I changed this to use a WEST longitude, according to our new convention.
# Beware when using this at other locations!

set atNextNewMoonLocalArgs {
  { atNextNewMoon "time for next new moon, including LST for arbitrary longitude" }
  { <tstamp> STRING "" tstart "time stamp" }
  { -obsLongitude STRING "105.8198778" obsLongitude "observatory longitude, default APO 3.5m" }
}


ftclHelpDefine astrotools atNextNewMoonLocal [shTclGetHelpInfo atNextNewMoonLocal $atNextNewMoonLocalArgs]

proc nextNewMoonLocal { args } {

# access help and parse inputs 

  upvar #0 atNextNewMoonLocalArgs formal_list

  if { [shTclParseArg $args $formal_list atNextNewMoonLocal ] == 0 } { return }

# find the next new moon using default timestep of 1 hour

  set tStart [ lindex $args 0 ]
  set tMoon [ nextNewMoon $tStart ] 

# decrease start time by 2 hours look for nextNewMoon in order 
# to test whether or not are within 2 hours of the new moon

  tstampIncr $tStart -7200
  set tMoonTest [ nextNewMoon $tStart ]
  set testDiff [ deltaTstamp $tMoon $tMoonTest ]
  tstampIncr $tStart +7200
  tstampDel $tMoonTest
  if { $testDiff > 1.0 } {
    tstampIncr $tStart -3600
    set tMoonTestClose [ nextNewMoon $tStart -deltaHours 0.02 ]
    tstampIncr $tStart +3600
# check to make sure newMoon is after StartTime
    set testDiff [ deltaTstamp $tMoonTestClose $tStart ]
    if { $testDiff > 1.0 } { 
	tstampDel $tMoon
      set tMoon $tMoonTestClose
    }

# if startTime not near new moon, get more accuracy 
  } else {
    tstampIncr $tMoon -7200
    set tMoon2 [ nextNewMoon $tMoon -deltaHours 0.02 ]
      tstampDel $tMoon
    set tMoon $tMoon2
 } 

  set dateMJD [ format "%7.1f" [ tstampToMJD $tMoon ] ]
  set dateLST [ degToHMS [ tstampToLst $tMoon -long $obsLongitude ] ]
  set dateLST [ string range $dateLST 0 10 ]

# create a keyed list with the answer

  keylset theAnswer LST    $dateLST 
  keylset theAnswer MJD    $dateMJD
  keylset theAnswer TSTAMP $tMoon

  return $theAnswer

}  
