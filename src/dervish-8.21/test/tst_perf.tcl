#
# DERVISH performance test driver
#
# 
# DLP -- 20-Feb 1996
#
#


#
# look up the user CPU time from the file
# containing out baseline of CPU user times.
#
proc timeForTest {test} {
    set node [exec uname -a]
    set node [lindex $node 1]
    set rc [catch {getpars $node} perfType]
    set perfType [string trim $perfType]
    if {$rc == 0 } {
	set time4Test [getpars $test]
	set time4Test [keylget time4Test $perfType] 
    } else {
	echo "TEST-ERR: tst_perf.tcl does not know of machine $node, edit tst_perf.par"
	set time4Test 0
    }
    return $time4Test
}    

#
# Proble the output of timerLap for user CPU time,
# and print messages apropos to the actual time -- warn
# if out of tolerance.
#
proc compare2Base {tLap base actual} {
    set tActual $tLap
    set tActual [keylget tActual CPU]
    set tActual [keylget tActual UTIME]
    set tolerance 1.20
    set tCompare [expr $base*$tolerance]
    
    if {$tActual>$tCompare} {
	echo "TEST-ERR: time for this test is: $tActual CPU seconds"
	echo "TEST-ERR: it exceeds 120% of the allowed baseline $base"
	echo "TEST-ERR: Zero baseline indicates that we have no benchmark for this machine"
    }
    return
}

#
# ============================================
#
# The real work. Do a test, assisted by the above procuedures to
# get the USER CPU time, and to compare it to a previous
# baseline, The previous baseline  is sourced in the
# in immediately below
#
inipar tst_perf.par

#
# Perforamance of regPixGet
#

set nrow 20480
set ncol 1
set reg [regNew $nrow $ncol]
timerStart
loop r 0 [expr $nrow-1] {regPixGet $reg $r 0}
compare2Base [timerLap] [timeForTest regPixGet] regPixGet
regDel $reg


#
# Template to add a new test (change the stuff in paratheses)
#
# (setup whatever)
# timerStart
# (iterate many times, -- alphas are fast)
# compare2Base [timerLap] [timeForTest regPixGet] (name of your test)
# (clean up after yourself)


