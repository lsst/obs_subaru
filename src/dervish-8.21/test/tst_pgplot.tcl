#
#   Tcl version of pgdemo1
#
proc pgex1 {} {
    set XS {1. 2. 3. 4. 5.}
    set YS {1. 4. 9. 16. 25.}
    pgEnv 0. 10. 0. 20. 0 1
    pgLabel "(x)" "(y)" "PGPLOT Example 1: y = x\\u2"
    pgPnts 5 $XS $YS 9

    for {set i 1} {$i < 61} {incr i} {
        set x [expr 0.1 * $i]
        lappend XR $x
        lappend YR [expr $x*$x]
    }
    pgLine $XR $YR
}

proc pgex2 {} {
    pgEnv -2. 10. -0.4 1.2 0 1
    pgLabel "(x)" "sin(x)/x" "PGPLOT Example 2: Sinc Function"

    for {set i 1} {$i < 101} {incr i} {
        set x [expr ($i-20)/6.]
        lappend XR $x
        if {$x != 0.0} {
            lappend YR [expr sin($x)/$x]
        } else {
            lappend YR 1.0
        }
    }
    pgLine $XR $YR
}

proc pgex3 {} {
    set pi 3.14159265
    pgEnv 0. 720. -2.0 2.0 0 -2
    pgSave
    pgSci 14
    pgBox "G" 30.0 0 "G" 0.2 0
    pgSci 5
    pgBox "ABCTSN" 90.0 3 "ABCTSNV" 0.0 0
    pgSci 3
    pgLabel "x (degrees)" "f(x)" "PGPLOT Example 3"

    for {set i 1} {$i < 361} {incr i} {
        set x [expr 2.0 * $i]
        set arg [expr $x/180.0*$pi]
        lappend XR $x
        lappend YR [expr sin($arg)+(0.5*cos(2.0*$arg))+(0.5*sin(1.5*$arg+$pi/3.0))]
    }

    pgSci 6
    pgSls 2
    pgSlw 3
    pgLine $XR $YR
    pgUnsa
}

proc pgex5 {} {
    set red 2
    set green 3
    set cyan 5
    set freq {26. 38. 80. 160. 178. 318. 365. 408. 750. 1400. 2695. 2700. 5000. 10695. 14900.}
    set flux {38. 66.4 89. 69.8 55.9 37.4 46.8 42.4 27. 15.8 9.09 9.17 5.35 2.56 1.73}
    set err {6. 6. 13. 9.1 2.9 1.4 2.7 3. .34 .8 .2 .46 .15 .08 .01}

    pgSave
    pgSci $cyan
    pgEnv -2. 2. -0.5 2.5 1 30
    pgLabel "Frequency, \\gn (GHz)" "Flux Density, S\\d\\gn\\u (Jy)" "PGPLOT Example 5:  Log-Log plot"

    for {set i 1} {$i < 101} {incr i} {
        set x [expr 1.3 + ($i*0.03)]
        lappend XP [expr $x-3.0]
        lappend YP [expr 5.18-(1.15*$x)-(7.72*exp(-$x))]
    }
    pgSci $red
    pgLine $XP $YP

    for {set i 0} {$i < 15} {incr i} {
        set fr [lindex $freq $i]
        set fl [lindex $flux $i]
        lappend XP1 [expr log10($fr)-3.0]
        lappend YP1 [expr log10($fl)]
    }
    pgSci $green
    pgPnts 15 $XP1 $YP1 17

    for {set i 0} {$i < 15} {incr i} {
        set fl [lindex $flux $i]
        set e [lindex $err $i]
        set yhi [expr log10([expr $fl+2.*$e])]
        set ylo [expr log10([expr $fl-2.*$e])]
        set x [lindex $XP1 $i]
        pgErry $x $ylo $yhi 1.0
    }
    pgUnsa
}

proc pgex6 {} {
    set twopi [expr 2.0*3.14159265]
    pgSave
    pgEnv 0. 8. 0. 8. 1 -2
    pgSci 3
    pgLabel " " " " "PGPLOT Example 6:  routine PGPOLY"

    pgSfs 1
    for {set i 3} {$i < 10} {incr i} {
        pgSci [expr $i - 2]
        for {set j 1} {$j <= $i} {incr j} {
            set tp [expr $twopi*($j-1)/$i]
            lappend X [expr $i-2 + (0.5*cos($tp))]
            lappend Y [expr 6 + 0.5*sin($tp)]
        }
        pgPoly $X $Y
        unset X
        unset Y
    }

    pgSfs 2
    for {set i 3} {$i < 10} {incr i} {
        pgSci [expr $i - 2]
        for {set j 1} {$j <= $i} {incr j} {
            set tp [expr $twopi*($j-1)/$i]
            lappend X [expr $i-2 + 0.5*cos($tp)]
            lappend Y [expr 3 + 0.5*sin($tp)]            
        }
        pgPoly $X $Y
        unset X 
        unset Y
    }
    pgUnsa
}

proc pgex8 {} {
    set black 0
    set white 1
    set red 2
    set green 3
    set blue 4
    set cyan 5
    set magent 6
    set yellow 7
    set full 1
    set dashed 2
    set dotdsh 3
    set dotted 4
    set fancy 5
    set normal 1
    set roman 2
    set italic 3
    set script 4
    set solid 1
    set hollow 2
    pgPage
    pgBbuf
    pgSave
    pgVport 0.1 0.6 0.1 0.6
    pgWindow 0.0 630.0 -2.0 2.0
    pgSci $cyan
    pgBox "ABCTS" 90.0 3 "ABCTSV" 0.0 0
    pgSci $red
    pgBox "N" 90.0 3 "VN" 0.0 0

    for {set i 1} {$i < 361} {incr i} {
        set x [expr 2.0 * $i]
        lappend XR $x
        lappend YR [expr sin($x/57.29577951)]
    }
    pgSci $magent
    pgSls $dashed
    pgLine $XR $YR

    pgWindow 90.0 720.0 -2.0 2.0
    pgSci $yellow
    pgSls $dotted
    pgLine $XR $YR
    pgSls $full
    pgVport 0.45 0.85 0.45 0.85
    pgWindow 0.0 180.0 -2.0 2.0
    pgSci $black
    pgRect 0.0 180.0 -2.0 2.0
    pgSci $blue
    pgBox "ABCTSM" 60.0 3 "VABCTSM" 1.0 2
    pgSci $white
    pgSls $dashed
    pgLine $XR $YR

    pgUnsa
    pgEbuf
}

proc pgex11 {} {
    set nvert 20
    set t 1.618
    set t1 [expr 1.0+$t]
    set t2 [expr -1.0*$t]
    set t3 [expr -1.0*$t1]
    set vert1 [list $t $t $t $t $t2 $t2 $t2 $t2 $t1 $t1 $t3 $t3 0.0 0.0 0.0 0.0 1.0 -1.0 1.0 -1.0]
    set vert2 [list $t $t $t2 $t2 $t $t $t2 $t2 1.0 -1.0 1.0 -1.0 $t1 $t1 $t3 $t3 0.0 0.0 0.0 0.0]
    set vert3 [list $t $t2 $t $t2 $t $t2 $t $t2 0.0 0.0 0.0 0.0 1.0 -1.0 1.0 -1.0 $t1 $t1 $t3 $t3]
    set vert [list $vert1 $vert2 $vert3]

    pgBbuf
    pgSave
    pgEnv -4. 4. -4. 4. 1 -2
    pgSci 2
    pgSls 1
    pgSlw 1
    pgLabel " " " " "PGPLOT Example 11: Dodecahedron"

    for {set i 0} {$i < $nvert} {incr i} {
        set zz [lindex $vert3 $i]
        pgPnts 1 [expr [lindex $vert1 $i]+(0.2*$zz)] [expr [lindex $vert2 $i]+(0.3*$zz)] 9
    }

    pgSlw 3
    for {set i 1} {$i < $nvert} {incr i} {
        for {set j 0} {$j < $i} {incr j} {
            set r 0.
            for {set k 0} {$k < 3} {incr k} {
                set vki [lindex [lindex $vert $k] $i]
                set vkj [lindex [lindex $vert $k] $j]
                set vkij [expr $vki-$vkj]
                set r [expr $r+($vkij*$vkij)]
            }
            set r [expr sqrt($r)]
            if {abs([expr $r-2.0]) <= 0.1} {
                set zz [lindex $vert3 $i]
                lappend X [expr [lindex $vert1 $i]+(0.2*$zz)]
                lappend Y [expr [lindex $vert2 $i]+(0.3*$zz)]
                set zz [lindex $vert3 $j]
                lappend X [expr [lindex $vert1 $j]+(0.2*$zz)]
                lappend Y [expr [lindex $vert2 $j]+(0.3*$zz)]
                pgLine $X $Y
                unset X
                unset Y
            }
        }
    }

    pgUnsa
    pgEbuf
}

proc pgex12 {} {
    set nv 16
    pgBbuf
    pgSave
    pgSch 0.7
    pgSci 2
    pgEnv -1.05 1.05 -1.05 1.05 1 -1
    pgLabel " " " " "PGPLOT Example 12: PGARRO"
    pgSci 1

    set k 1
    set d [expr 360.0/57.29577951/$nv]
    set a [expr -$d]
    for {set i 1} {$i <= $nv} {incr i} {
        set a [expr $a + $d]
        set x [expr cos($a)]
        set y [expr sin($a)]
        set xt [expr 0.2*cos($a-$d)]
        set yt [expr 0.2*sin($a-$d)]
        pgSah $k [expr 80.0-(3.0*$i)] [expr (0.5*$i)/$nv]
        pgSch [expr 0.25*$i]
        pgArrow $xt $yt $x $y
        incr k
        if {$k > 2} {
            set k 1
        }
    }

    pgUnsa
    pgEbuf
}

proc pgex13 {} {
    set n 10
    set x1 [list 0.0 0.0 0.0 0.0 -8000.0 100.3 205.3 -45000.0 0.0 0.0]
    set x2 [list 8000.0 8000.0 8000.0 8000.0 8000.0 101.3 201.1 -100000.0 -100000.0 -100000.0]
    set xopt [list "BSTN" "BSTNZ" "BSTNZH" "BSTNZD" "BSNTZHFO" "BSTNZD" "BSTNZHI" "BSTNZHP" "BSTNZDY" "BSNTZHFOY"]
#   set bsl 
    pgPage 
    pgSave
    pgBbuf
    pgSch 0.7
    for {set i 1} {$i <= $n} {incr i} {
        set di [expr $i-1]
        set ri [expr $i*1.0]
        set rn [expr $n*1.0]
        pgVport 0.15 0.85 [expr (0.7+($rn-$ri))/$rn] [expr (0.7+($rn-$i+1))/$rn]
        pgWindow [lindex $x1 $di] [lindex $x2 $di] 0.0 1.0
        set xx [lindex $xopt $di]
        pgTbox "$xx" 0.0 0 " " 0.0 0
        pgLabel "Option = $xx" " " " "
        if {$i == 1} {
            pgMtext "B" -1.0 0.5 0.5 "Axes drawn with PGTBOX"
        }
    }
    pgEbuf
    pgUnsa
}

proc pgex14 {} {
    set pi [expr acos(-1.0)]
    set n 33
    set m 8
    set thinc [expr 2.0*$pi/$n]
    for {set i 0} {$i < $n} {incr i} {
        lappend xi 0.0
        lappend yi 0.0
    }

    pgBbuf
    pgSave
    pgEnv -1. 1. -1. 1. 1 -2
    pgLabel " " " " "PGPLOT Example 14: PGPOLY and PGSCR"
    for {set j 1} {$j <= $m} {incr j} {
        set r 1.0
        set rjm [expr ($j*1.0)/($m*1.0)]
        set g [expr 1.0 - $rjm]
        set b $g
        pgScr $j $r $g $b
        set theta [expr -($j*1.0)*$pi/($n*1.0)]
        set r $rjm
        for {set i 0} {$i < $n} {incr i} {
            set theta [expr $theta+$thinc]
            lappend xo [expr $r*cos($theta)]
            lappend yo [expr $r*sin($theta)]
        }
        for {set i 0} {$i < $n} {incr i} {
            set in [expr (($i+1)%$n)]
            set xt [list [lindex $xo $i] [lindex $xo $in] [lindex $xi $i]]
            set yt [list [lindex $yo $i] [lindex $yo $in] [lindex $yi $i]]
            pgSci $j
            pgSfs 1
            pgPoly $xt $yt
            pgSfs 2
            pgSci 1
            pgPoly $xt $yt
        }
        unset xi
        unset yi
	for {set i 0} {$i < $n} {incr i} {
            lappend xi [lindex $xo $i]
            lappend yi [lindex $yo $i]
        }
        unset xo
        unset yo
    }
    pgUnsa
    pgEbuf
}

proc pgex15 {} {
    set nv 17
    set d [expr 360.0/$nv]
    set a [expr -$d]
    for {set i 0} {$i < $nv} {incr i} {
        set a [expr $a+$d]
        lappend X [expr cos($a/57.29577951)]
        lappend Y [expr sin($a/57.29577951)]
    }

    pgBbuf
    pgSave
    pgSch 0.5
    pgSci 2
    pgEnv -1.05 1.05 -1.05 1.05 1 -1
    pgLabel " " " " "PGPLOT Example 15: PGMOVE and PGDRAW"
    pgScr 0 0.2 0.3 0.3
    pgScr 1 1.0 0.5 0.2
    pgScr 2 0.2 0.5 1.0
    pgSci 1

    set nvm1 [expr $nv - 1]
    for {set i 0} {$i < $nvm1} {incr i} {
        set ip1 [expr $i+1]
        for {set j $ip1} {$j < $nv} {incr j} {
            set xi [lindex $X $i]
            set yi [lindex $Y $i]
            set xj [lindex $X $j]
            set yj [lindex $Y $j]
            pgMove $xi $yi
            pgDraw $xj $yj
        }
    }
    pgUnsa
    pgEbuf
}

# Now run the actual demo

set waitTime 1000
pgBegin
pgAsk 0
pgex1
after $waitTime
pgex2
after $waitTime
pgex3
after $waitTime
pgSubp 2 2
pgex5
pgex6
after $waitTime
pgSubp 1 1
pgex8
after $waitTime
pgex11
after $waitTime
pgex12
after $waitTime
pgex13
after $waitTime
pgex14
after $waitTime
pgex15
after $waitTime
pgEnd

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_pgplot.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_pgplot"
    return 1
}

