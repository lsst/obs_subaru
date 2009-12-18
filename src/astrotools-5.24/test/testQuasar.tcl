proc testQuasar {} {

  if {[tst_quLocusCheck]!=0} {
    error "Trouble in tst_quLocusCheck"
    return 1
  }
  if {[tst_quLocusKlmFromXyz]!=0} {
    error "Trouble in tst_quLocusKlmFromXyz"
    return 1
  }
  return 0

}

proc tst_quLocusCheck {} {

  set inlocuslist [quLocusCheck {1 1 1} {-300 0 0 -300 0 -300} locusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=-1) {return 1}

  set inlocuslist [quLocusCheck {1 1 1} {0 0 0 -300 0 -300} badlocusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=-2) {return 1}

  set inlocuslist [quLocusCheck {2 2 2} {0 0 0 -300 0 -300} badlocusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=-2) {return 1}

  set inlocuslist [quLocusCheck {1 1 1} {0 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1.4 1.4 1.4} {0 0 0 0 0 0} locusfile 100 0.7 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.6 1.6 1.6} {0 0 0 0 0 0} locusfile 100 0.0 0 0.8 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.6 1.6 1.6} {0 0 0 0 0 0} locusfile 100 0.0 0 1.1 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.9 0.9 0.9} {0 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.5 1.5 1.5} {0 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 -1} {0 0 0 0 0 -100} locusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {4 4 -1} {0 0 0 0 0 -300} badlocusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=-3) {return 1}

  set inlocuslist [quLocusCheck {5 -1 5} {0 0 0 -300 0 0} badlocusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {-1 3 3} {-300 0 0 0 0 0} badlocusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 -1} {0 0 0 0 0 -200} locusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 -1} {0 0 0 0 0 -300} locusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 3} {0 0 0 0 0 -100} locusfile 100 0 0 1000 0 0]
  #echo $inlocuslist
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 3} {0 0 0 0 0 -200} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 1 3} {0 0 0 0 0 -300} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 -1 1} {0 0 0 -100 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 -1 1} {0 0 0 -200 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 -1 1} {0 0 0 -300 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 3 1} {0 0 0 -100 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 3 1} {0 0 0 -200 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 3 1} {0 0 0 -300 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {-1 1 1} {-100 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {-1 1 1} {-200 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {-1 1 1} {-300 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {3 1 1} {-100 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {3 1 1} {-200 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {3 1 1} {-300 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {3 0.8 1} {-100 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {3 0.6 1} {-100 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {3 0.4 1} {-100 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {3 0.2 1} {-100 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {3 0 1} {-100 0 0 0 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 1 -1} {0 0 0 -300 0 -100} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 -4 0.8} {0 0 0 -200 0 -100} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 4 1.1} {0 0 0 -200 0 -100} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 1.1 4} {0 0 0 -100 0 -200} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 1.1 1.6} {0 0 0 -200 0 -200} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 0.9 1.3} {0 0 0 -200 0 -200} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 -1 1} {0 0 0 -100 0 -300} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 1 -1} {-300 0 0 0 0 -100} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {-4 1 0.8} {-200 0 0 0 0 -100} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {4 1 1.1} {-200 0 0 0 0 -100} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.1 1 4} {-100 0 0 0 0 -200} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.1 1 1.6} {-200 0 0 0 0 -200} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.9 1 1.3} {-200 0 0 0 0 -200} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {-1 1 1} {-100 0 0 0 0 -300} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 -1 1} {-300 0 0 -100 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {-4 0.8 1} {-200 0 0 -100 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {4 1.1 1} {-200 0 0 -100 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.1 4 1} {-100 0 0 -200 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.1 1.6 1} {-200 0 0 -200 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.9 1.3 1} {-200 0 0 -200 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {-1 1 1} {-100 0 0 -300 0 0} locusfile 100 0 0 1000 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0 0.8 1} {-300 0 0 0 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1.05 0.8 1} {-100 0 0 0 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {2.1 0.8 1} {-200 0 0 0 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {2.2 0.8 1} {-200 0 0 0 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.8 0 1} {0 0 0 -300 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 1.05 1} {0 0 0 -100 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.8 2.1 1} {0 0 0 -200 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 2.2 1} {0 0 0 -200 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 0} {0 0 0 0 0 -300} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 1.05} {0 0 0 0 0 -100} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 2.1} {0 0 0 0 0 -200} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 2.2} {0 0 0 0 0 -200} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 1.05} {-200 0 0 0 0 -100} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 1.05} {-100 0 0 0 0 -100} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 1.05} {-100 0 0 0 0 -200} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 0.8 1.05} {0 0 0 -200 0 -100} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 0.8 1.05} {0 0 0 -100 0 -100} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 0.8 1.05} {0 0 0 -100 0 -200} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 1.05 1} {-200 0 0 -100 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 1.05 1} {-100 0 0 -100 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.8 1.05 1} {-100 0 0 -200 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 1} {0.0008 -0.0004 0 0.0008 -0.0004 0.0008} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.99 0.99 0.99} {0.0008 0 0 0.0008 0 0.0008} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.99 0.99 0.99} {0.0008 0 0 0.0008 0 0.0008} locusfile 100 0 0 2 0 1]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.99 0.99 0.99} {0 0 0 0 0 0} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {2.155 2.155 2.155} {0.0008 -0.0004 0 0.0008 -0.0004 0.0008} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {2.155 2.155 2.155} {0.0008 -0.0004 0 0.0008 -0.0004 0.0008} locusfile 100 0 0 2 0 1]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1.5 1.5 1.5} {0.0008 -0.0004 0 0.0008 -0.0004 0.0008} locusfile 100 0 0 2 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 2.12} {0.01 0 0 0.01 0 0.01} locusfile 100 0 0 2 0 2]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.8 1 2.12} {0.01 0 0 0.01 0 -200} locusfile 100 0 0 2 0 4]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 1} {0 0 0 0 0 0} locusfile2 100 0 0 0 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1.01 1 1} {0 0 0 0 0 0} locusfile2 100 0 0 0 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 1 -1} {0 0 0 0 0 0} locusfile2 100 0 0 0 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 -1.01} {0 0 0 0 0 0} locusfile2 100 0 0 0 0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {2 1 1} {0 0 0 0 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {-2.01 1 1} {0 0 0 0 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.707 1.707 1} {0 0 0 0 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1.708 0.292 1} {0 0 0 0 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.5 1.5 2.414} {0 0 0 0 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1.51 1.51 2.415} {0 0 0 0 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {-3 1 1} {1.0 0 0 1.0 0 1.0} locusfile2 100 0 0 0 0 4]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {-3.0001 1 1} {1.0 0 0 1.0 0 1.0} locusfile2 100 0 0 0 0 4]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.8 0.2 1} {0 0 0 0 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.8 0.2 1} {-200 0 0 -200 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1.8 0.2 1} {-200 0 0 -100 0 0} locusfile2 100 0 3.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 1 2} {0 0 0 0 0 0} locusfile3 100 0 0.0 0 0.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1 1 2.0001} {0 0 0 0 0 0} locusfile3 100 0 0.0 0 0.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1.707 1 1.707} {0 0 0 0 0 0} locusfile3 100 0 1.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {1.708 1 1.708} {0 0 0 0 0 0} locusfile3 100 0 1.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {1 1.001 1} {0 0 0 0 0 0} locusfile3 100 0 1.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.293 1 0.293} {0 0 0 0 0 0} locusfile3 100 0 1.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.292 1 0.292} {0 0 0 0 0 0} locusfile3 100 0 1.0 0 1.0 0]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  set inlocuslist [quLocusCheck {0.292 1 0.292} {0.0026 0 0 0 0 0.0026} locusfile3 100 0 1.0 0 1.0 1]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=1) {return 1}

  set inlocuslist [quLocusCheck {0.292 1 0.292} {0.0026 0 0 0 0 0.0026} locusfile3 100 0 1.0 0 1.0 0.9]
  set inlocus [keylget inlocuslist inlocus]
  if ($inlocus!=0) {return 1}

  #set inlocuslist [quLocusCheck {} {} locusfile 100 0 0 2 0 0]
  #set inlocus [keylget inlocuslist inlocus]
  #if ($inlocus!=0) {return 1}

  return 0

}

proc tst_quLocusKlmFromXyz {} {

  set klmlist [quLocusKlmFromXyz {2.0 2.0 2.0} locusfile 10]
  set k [keylget klmlist k]
  set l [keylget klmlist l]
  set m [keylget klmlist m]
  if ([abs $k-1.73205]>.00001) {return 1}
  if ([abs $l]>.00001) {return 1}
  if ([abs $m]>.00001) {return 1}

  return 0

}
