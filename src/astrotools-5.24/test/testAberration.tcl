proc testAberration { } {
    global l
    global ta at_deg2Rad
    set ta(ts) [tstampNew 1999 1 1 0 0 0]
    set ta(mjd) [tstampToMJD $ta(ts)]
    set ta(epv) [earthPosVel $ta(mjd) -helio 1]
    foreach p "x y z" {set ta($p) [keylget ta(epv) $p]}
    set ta(longlat) [xyzToLb $ta(x) $ta(y) $ta(z)]
    set ta(dec) [lindex $ta(longlat) 1]
    set ta(ra) [lindex $ta(longlat) 0]
    set ta(aaa) [atAberrationApply $ta(mjd) 2000 $ta(ra) $ta(dec)]
    set ta(ra_apparent) [keylget ta(aaa) ra]
    set ta(dec_apparent) [keylget ta(aaa) dec]

    set ta(del_ra)  [expr 3600.0*($ta(ra_apparent)-$ta(ra))]
    set ta(del_dec) [expr 3600.0*($ta(dec_apparent)-$ta(dec))]

    # delta ra should be 22.57 arcseconds
    if {[abs $ta(del_ra)-22.487] > 0.01} {
	error "Bad del_ra -- should be about 22.57 but is actually $ta(del_ra)"
    }

    # Here is the earth's velocity/c
    set v 0.0001

    # --- moving in +y direction ---
    # looking +x
    set l [atAberrationGet 0 $v 0 0 0]
    if {[abs [keylget l delRa]-0.00573] > 0.000001} {
	error "trouble from atAberrationGet 1"
    }

    # looking +y
    set l [atAberrationGet 0 $v 0 90 0]
    if {[abs [keylget l delRa]-0.00000] > 0.000001} {
	error "trouble from atAberrationGet 2:  l=$l"
    }

    # looking -x
    set l [atAberrationGet 0 $v 0 180 0]
    if {[abs [keylget l delRa]+0.00573] > 0.000001} {
	error "trouble from atAberrationGet 3:  l=$l"
    }

    # looking -y
    set l [atAberrationGet 0 $v 0 270 0]
    if {[abs [keylget l delRa]+0.00000] > 0.000001} {
	error "trouble from atAberrationGet 4:  l=$l"
    }


    # --- moving in -x direction ---
    # looking +x
    set l [atAberrationGet -$v 0 0 0 0]
    if {[abs [keylget l delRa]-0.00000] > 0.00001} {
	error "trouble from atAberrationGet 5:  l=$l"
    }

    # looking +y
    set l [atAberrationGet -$v 0 0 90 0]
    if {[abs [keylget l delRa]-0.005730] > 0.00001} {
	echo HELLO
	error "trouble from atAberrationGet 6:  l=$l"
    }

    # looking -x
    set l [atAberrationGet -$v 0 0 180 0]
    if {[abs [keylget l delRa]-0.00000] > 0.00001} {
	error "trouble from atAberrationGet 7:  l=$l"
    }

    # looking -y
    set l [atAberrationGet -$v 0 0 270 0]
    if {[abs [keylget l delRa]+0.00573] > 0.00001} {
	error "trouble from atAberrationGet 8:  l=$l"
    }


    # moving in +y and -x direction
    set c45 [cos $at_deg2Rad*45]
    set v45 [expr $v*$c45]
    set l [atAberrationGet -$v45 $v45 0 45 0]
    if {[abs [keylget l delRa]-0.00573] > 0.00001} {
	error "trouble from atAberrationGet 9:  l=$l"
    }

    # moving in +z direction
    set l [atAberrationGet 0 0 $v 45 0]
    if {[abs [keylget l delDec]-0.00573] > 0.00001} {
	error "trouble from atAberrationGet 10:  l=$l"
    }

    # now rotate that by 45 degrees
    set v4545 [expr $v*$c45*$c45]
    set l [atAberrationGet -$v4545 $v4545 $v45 45 0]
    set delRa [keylget l delRa]
    set delDec [keylget l delDec]
    if {[abs $delRa-0.004051] > 0.000001} {
	error "trouble from atAberrationGet 11:  l=$l"
    }
    if {[abs $delDec-0.004051] > 0.000001} {
	error "trouble from atAberrationGet 12:  l=$l"
    }

    set ral "  10  20 30 40 50 60 70 80 90 100"
    set decl "-30 -15 -1  1 23 24 25 75 76  77"
    set vRaIn [vFromL $ral]
    set vDecIn [vFromL $decl]
    atVAberrationApply $ta(mjd) 2000 $vRaIn $vDecIn

    loop i 0 [llength $ral] {
	set ra [lindex $ral $i]
	set dec [lindex $decl $i]
	set l [atAberrationApply $ta(mjd) 2000 $ra $dec]
	atc $i $vRaIn $vDecIn $l
    }
    
    vectorExprDel $vRaIn
    vectorExprDel $vDecIn
    tstampDel $ta(ts)

    return 0
}

proc atc {i vRaIn vDecIn l} {
    set vra [exprGet $vRaIn.vec<$i>]
    set vdec [exprGet $vDecIn.vec<$i>]
    set ra [keylget l ra]
    set dec [keylget l dec]
    if {[abs $vra-$ra] > 0.1/3600} {
	error "bad ra from atvAberrationApply for i=$i"
    }
    if {[abs $vdec-$dec] > 0.1/3600} {
	error "bad dec from atvAberrationApply for i=$i"
    }
    return
}


proc taKent { } {
    global tak
    set tak(ral)  "100 101 100"
    set tak(decl) "  0   0   1"

    set tak(m.ra) [vFromL $tak(ral)]
    set tak(m.dec) [vFromL $tak(decl)]

    set tak(a.ra) [vFromL $tak(ral)]
    set tak(a.dec) [vFromL $tak(decl)]

    set tak(ts) [tstampNew 1999 1 1 0 0 0]
    set tak(mjd) [tstampToMJD $tak(ts)]
    atVAberrationApply $tak(mjd) 2000 $tak(a.ra) $tak(a.dec)

    set t1 [takTheta $tak(m.ra) $tak(m.dec) 0 1 2]
    set t2 [takTheta $tak(a.ra) $tak(a.dec) 0 1 2]
    set theta [expr $t1-$t2]
    echo delta theta 1 is [expr $theta*3600.0] arcsec

    set t1 [takTheta $tak(m.ra) $tak(m.dec) 1 2 0]
    set t2 [takTheta $tak(a.ra) $tak(a.dec) 1 2 0]
    set theta [expr $t1-$t2]
    echo delta theta 2 is [expr $theta*3600.0] arcsec

    set t1 [takTheta $tak(m.ra) $tak(m.dec) 2 0 1]
    set t2 [takTheta $tak(a.ra) $tak(a.dec) 2 0 1]
    set theta [expr $t1-$t2]
    echo delta theta 0 is [expr $theta*3600.0] arcsec

}

proc takTheta {rav decv a b c} {
    global list
    global at_rad2Deg
    foreach l "a b c" {
	set ra($l) [exprGet $rav.vec<[set $l]>]
	set dec($l) [exprGet $decv.vec<[set $l]>]
	set list [lbToXyz $ra($l) $dec($l)]
	set ixyz 0
	foreach xyz "x y z" {
	    set v($xyz.$l) [lindex $list $ixyz]
	    incr ixyz
	}
    }
    set absum 0
    set cbsum 0
    foreach xyz "x y z" {

	set v($xyz.a-b) [expr $v($xyz.a)-$v($xyz.b)]
	set absum [expr $absum+[pow $v($xyz.a-b) 2]]

	set v($xyz.c-b) [expr $v($xyz.c)-$v($xyz.b)]
	set cbsum [expr $cbsum+[pow $v($xyz.c-b) 2]]

    }
    set num [expr \
	    $v(x.a-b)*$v(x.c-b) + $v(y.a-b)*$v(y.c-b) + $v(z.a-b)*$v(z.c-b)]
    set den [sqrt $absum*$cbsum]
    set cost [expr $num/$den]
    set theta [expr $at_rad2Deg*[acos $cost]]
    # echo a=$a b=$b c=$c theta=$theta
    return $theta
}

proc bobCheck { } {
    bc1 50986.291666667 18.000000000 0.0000000 18.000378446 0.0000178
    bc1 50986.491666667 18.000000000 0.0000000 18.000375035 0.0000253
    bc1 51118.100000000 1.000000000 45.0000000 1.000463871  45.0029475 
}

proc bc1 {mjd raIn decIn raBob decBob} {
    echo ---------------------------------------------------------------
    set l [atAberrationApply $mjd 2000 [expr $raIn*15.0] $decIn]
    echo mjd=$mjd raIn=$raIn hours decIn=$decIn degrees
    set raOut [expr [keylget l ra]/15.0]
    set decOut [keylget l dec]
    echo raOut=$raOut hours
    echo decOut=$decOut degrees
    echo raBob=$raBob hours
    echo decBob=$decBob degrees
    set raDel [expr ($raOut-$raBob)*15.0*3600]
    set decDel [expr ($decOut-$decBob)*3600.0]
    echo [format "raDel=%10.6f  decDel=%10.6f (arcseconds)" $raDel $decDel]
    return
}

proc bobCheck2 { } {
    set mjd 50986.291666667
    echo [atAberrationApply $mjd 2000.0 270.0 15.0]
    echo [atAberrationApply $mjd 2000.0 270.0 30.0]
    echo [atAberrationApply $mjd 2000.0 270.0 45.0]
    echo [atAberrationApply $mjd 2000.0 270.0 60.0]
    echo [atAberrationApply $mjd 2000.0 270.0 75.0]

    echo [atAberrationApply $mjd 2000.0 300.0 15.0]
    echo [atAberrationApply $mjd 2000.0 300.0 30.0]
    echo [atAberrationApply $mjd 2000.0 300.0 45.0]
    echo [atAberrationApply $mjd 2000.0 300.0 60.0]
    echo [atAberrationApply $mjd 2000.0 300.0 75.0]

}
