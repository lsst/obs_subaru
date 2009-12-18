proc testGalaxy {} {

  if {[tst_starCountsFind]!=0} {
    error "Trouble in tst_airmassFind"
    return 1
  }
  return 0

}

proc tst_starCountsFind {} {

  set nstars [starCountsFind 0 90 20]

  set err [abs [expr 1439.835961-$nstars]]

  if ($err>0.01) {return 1}

  return 0
}
