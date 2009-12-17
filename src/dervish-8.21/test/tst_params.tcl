#++
# FACILITY:	Sloan Digital Sky Survey
#		Ftcl/Dervish
#
# ABSTRACT:	Test ftclParams parmeter files.
#               chain2Param and param2Chain
#
# ENVIRONMENT:	Extended Tcl 7.
#		tst_inipar.tcl
#
#               
#		
# AUTHOR:	Philippe Canal, Creation date: 17-04-1995
#--
#*******************************************************************************

proc tst_param2Chain {} {
    set chn [param2Chain ocNew.par hdr]
    if { [file exist /tmp/cmpchn] } {system "/bin/rm /tmp/cmpchn"}
    if { [file exist /tmp/cmpchn2] } {system "/bin/rm /tmp/cmpchn2"}
    chain2Param /tmp/cmpchn $chn $hdr
    set chn2 [param2Chain /tmp/cmpchn hdr2]
    chain2Param /tmp/cmpchn2 $chn2 $hdr2
    set result [system "diff /tmp/cmpchn /tmp/cmpchn2"]
    #echo result is $result
    set size [chainSize $chn2]
    if {$size != 15 } { return 1}
    set cur [chainElementGetByPos $chn2 6]
    set name [exprGet $cur.name]
    if { $name != "o" } {return 1}
    set zp [exprGet $cur.zeroPt]
    if { $zp != 22.1 } {return 1}
    if { [file exist /tmp/cmpchn] } {system "/bin/rm /tmp/cmpchn"}
    if { [file exist /tmp/cmpchn2] } {system "/bin/rm /tmp/cmpchn2"}

    handleDel $cur
    set i [chainSize $chn]
    loop j 0 $i {
	set hndl [chainElementRemByPos $chn 0]
	genericDel $hndl
    }

    set i [chainSize $chn2]
    loop j 0 $i {
	set hndl [chainElementRemByPos $chn2 0]
	genericDel $hndl
    }
    
    chainDel $chn
    chainDel $chn2
    return 0
}

set param_list "{Value1 3} {Value1 3} {Value2 3.5} {Value3 three} \
	{_Strange_starting_param 3} {&another#strange!one 2} \
	{1231231 1231231} {abc {hello there}} {def hi} {123 {where am i}}"

proc tst_inipar {} {
    global param_list

    inipar test.par
    loop i 0 [llength $param_list] {
	set params [lindex $param_list $i]
	set param [lindex $params 0]
	set result [lindex $params 1]
	set got [getpars $param]
	if { $got != $result } {
	    puts stderr "ERROR for ftclParams: misread parameter $param"
	    return 1
	}
    }
}


if { [tst_inipar] != 0 } { return 1 }
if { [tst_chain2Param] != 0} { return 1 }

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_param.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_param"
    return 1
}

return

