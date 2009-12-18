proc testDustGetval {} {

# If DUST_DIR not defined, claim victory
    if {[envscan \$DUST_DIR] == ""} {
	return 0
    }

    foreach cmd [list \
	    tst_atDustGetval tst_atVDustGetval \
	    tst_atDustGetvalFast tst_atVDustGetvalFast \
	    tst_atDustMaskParse tst_atExtinctionGet
    ] {
	if {[$cmd] != 0} {
	    error "Trouble in $cmd in testDustGetval"
	    return 1
	}
    }
    return 0
}

proc tst_atDustGetval {} {

    set value [atDustGetval 121 21.5]
    if {[abs $value-0.244344]>0.0001} {
	return 1
    }
    set value [atDustGetval 121 -21.5]
    if {[abs $value-0.292574]>0.0001} {
	return 1
    }
    set value [atDustGetval 121 -21.5 -interpolate 0]
    if {[abs $value-0.273948]>0.0001} {
	return 1
    }

    set value [atDustGetval 121 21.5 -mapName T]
    if {[abs $value-16.110113]>0.0001} {
	return 1
    }
    set value [atDustGetval 121 -21.5 -mapName T]
    if {[abs $value-17.615322]>0.0001} {
	return 1
    }

    set value [atDustGetval 121 21.5 -mapName X]
    if {[abs $value-2.762813]>0.0001} {
	return 1
    }
    set value [atDustGetval 121 -21.5 -mapName X]
    if {[abs $value-1.287945]>0.0001} {
	return 1
    }

    set value [atDustGetval 121 21.5 -mapName I100]
    if {[abs $value-4.809772]>0.0001} {
	return 1
    }
    set value [atDustGetval 121 -21.5 -mapName I100]
    if {[abs $value-12.347813]>0.0001} {
	return 1
    }

    set value [atDustGetval 121 21.5 -mapName mask]
    if {[abs $value-3.000]>0.0001} {
	return 1
    }
    set value [atDustGetval 121 -21.5 -mapName mask]
    if {[abs $value-66.0000]>0.0001} {
	return 1
    }

    return 0
}

proc tst_atVDustGetval {} {

    set v1 [vFromL 121]
    set v2 [vFromL -21.5]
    set v3 [vectorExprNew 1]

    atVDustGetval $v1 $v2 $v3
    if {[abs [exprGet $v3.vec<0>]-0.292574]>0.0001} {
	return 1
    }
    atVDustGetval $v1 $v2 $v3 -interpolate 0
    if {[abs [exprGet $v3.vec<0>]-0.273948]>0.0001} {
	return 1
    }

    atVDustGetval $v1 $v2 $v3 -mapName T
    if {[abs [exprGet $v3.vec<0>]-17.615322]>0.0001} {
	return 1
    }
    atVDustGetval $v1 $v2 $v3 -mapName T -interpolate 0
    if {[abs [exprGet $v3.vec<0>]-17.610271]>0.0001} {
	return 1
    }

    vectorExprDel $v1
    vectorExprDel $v2
    vectorExprDel $v3
    return 0
}

proc tst_atDustGetvalFast {} {
    set value [atDustGetvalFast 121 -21.5]    
    if {[abs $value-0.292574]>0.0001} {
	return 1
    }
    set value [atDustGetvalFast 121 -21.5 -interpolate 0]
    if {[abs $value-0.273948]>0.0001} {
	return 1
    }
    atDustGetvalFastClean

    set value [atDustGetvalFast 121 -21.5 -mapName T]
    if {[abs $value-17.615322]>0.0001} {
	return 1
    }
    set value [atDustGetvalFast 121 -21.5 -mapName T -interpolate 0]
    if {[abs $value-17.610271]>0.0001} {
	return 1
    }
    atDustGetvalFastClean -mapName T

    return 0
}

proc tst_atVDustGetvalFast {} {

    set v1 [vFromL 121]
    set v2 [vFromL -21.5]
    set v3 [vectorExprNew 1]

    atVDustGetvalFast $v1 $v2 $v3
    if {[abs [exprGet $v3.vec<0>]-0.292574]>0.0001} {
	return 11
    }
    atVDustGetvalFast $v1 $v2 $v3 -interpolate 0
    if {[abs [exprGet $v3.vec<0>]-0.273948]>0.0001} {
	return 12
    }
    atDustGetvalFastClean

    atVDustGetvalFast $v1 $v2 $v3 -mapName T
    if {[abs [exprGet $v3.vec<0>]-17.615322]>0.0001} {
	return 13
    }
    atVDustGetvalFast $v1 $v2 $v3 -mapName T -interpolate 0
    if {[abs [exprGet $v3.vec<0>]-17.610271]>0.0001} {
	return 14
    }
    atDustGetvalFastClean -mapName T

    vectorExprDel $v1
    vectorExprDel $v2
    vectorExprDel $v3

    return 0
}

proc tst_atDustMaskParse {} {

    set val [atDustMaskParse 252]
    if {[lindex $val 0] != "no_IRAS"} {
	return 1
    }
    if {[lindex $val 1] != "big_obj"} {
	return 1
    }
    if {[lindex $val 2] != "no_list"} {
	return 1
    }
    if {[lindex $val 3] != "source"} {
	return 1
    }
    if {[lindex $val 4] != "glitch"} {
	return 1
    }
    if {[lindex $val 5] != "asteroi"} {
	return 1
    }
    if {[lindex $val 6] != "0hcons"} {
	return 1
    }

    return 0
}

proc tst_atExtinctionGet {} {
    set value [atExtinctionGet 121 -21.5]
    if {abs([keylget value u] - 1.5082) > 0.0001} {
	return 1
    }
    if {abs([keylget value g] - 1.1097) > 0.0001} {
	return 1
    }
    if {abs([keylget value r] - 0.8049) > 0.0001} {
	return 1
    }
    if {abs([keylget value i] - 0.6103) > 0.0001} {
	return 1
    }
    if {abs([keylget value z] - 0.4327) > 0.0001} {
	return 1
    }

    return 0
}
