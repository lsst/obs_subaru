#
#test SubRegGet
#
proc testSubRegGet {} {
   set nrp 1000
   set ncp 2000
   set reg [regNew $nrp $ncp]
# Stress it
   loop i 0 20 {
      lappend subrn [subRegNew $reg 10 10 10 10]
   }

#make subregions against subregions
   foreach subr $subrn {
      subRegNew $subr 1 1 1 1
   }
#
#Get subregions and do some checking
#
   set subs [subRegGet $reg]
   set nsubs [llength $subs]
   if {$nsubs != 40} {echo TEST-ERR: Hey! expected 40 subregions, got $nsubs}
   foreach sub $subs {
      set testlist [subRegGet $sub]
      if {[llength $testlist] != 0} {echo TEST-ERR: Hey! subregion $sub has subregions $testlist}
      regDel $sub
   } 
   regDel $reg
}

testSubRegGet

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_subRegGet.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_subRegGet"
    return 1
}

return 0
exit 0








