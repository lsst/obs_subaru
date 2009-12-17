#
# This file contain performance test for the expression evaluator.
# It only contain pure dervish test.  Object-oriented and database oriented
# test are found in the sdb product.
#
#

# Define some extra type.

typedef struct {
    int i;
    int j;
} SUB_STRUCT;

typedef struct { 
    int i;
    long l;
    SUB_STRUCT s;
    float f<32>;
} TYPE1

set vector_size 10000
set repeat 1
set sizes "1 5 10 50 100 500 1000 5000 10000 50000 100000"
set counts "10000 10000 10000 10000 10000 10000 10000 10000 1000 500 100"

set s_sizes "1 5 10 50 100 500 1000 5000"
set s_counts "10000 10000 10000 10000 10000 1000 100 20"

set PerfFile SEval.perf


# Test to compare vectorExprEval to exprEval.

proc VecExprAdd { vector_size repeat } {

    puts "Initializing vector ... "
    set vec1 [vectorExprNew $vector_size]
    set vec2 [vectorExprNew $vector_size]

    puts "Start addition evaluation through vectorExprEval"
    set res "[time {set vec4 [vectorExprEval "$vec1 + $vec2"];vectorExprDel $vec4} $repeat] for vectorExprEval"

    vectorExprDel $vec1
    vectorExprDel $vec2

    return $res
}

proc SEvalVecAdd { vector_size repeat } {

    puts "Initializing vector ... "
    set vec1 [vectorExprNew $vector_size]
    set vec2 [vectorExprNew $vector_size]
    set vec3 [vectorExprNew $vector_size]

    puts "Start addition evaluation through expression evaluator"
    set res "[time {exprEval " $vec3 = $vec1 + $vec2"} $repeat] for exprEval"

    vectorExprDel $vec1;vectorExprDel $vec2;vectorExprDel $vec3
    return $res
}

proc VecExprMul { vector_size repeat } {

    puts "Initializing vector ... "
    set vec1 [vectorExprNew $vector_size]
    set vec2 [vectorExprNew $vector_size]

    puts "Start multiplication evaluation through vectorExprEval"
    set res "[time {set vec4 [vectorExprEval "$vec1 * $vec2"];vectorExprDel $vec4} $repeat] for vectorExprEval"

    vectorExprDel $vec1
    vectorExprDel $vec2

    return $res
}

proc SEvalVecMul { vector_size repeat } {

    puts "Initializing vector ... "
    set vec1 [vectorExprNew $vector_size]
    set vec2 [vectorExprNew $vector_size]
    set vec3 [vectorExprNew $vector_size]

    puts "Start multiplication evaluation through expression evaluator"
    set res "[time {exprEval " $vec3 = $vec1 * $vec2"} $repeat] for exprEval"

    vectorExprDel $vec1;vectorExprDel $vec2;vectorExprDel $vec3
    return $res
}

proc VecExprDiv { vector_size repeat } {

    puts "Initializing vector ... "
    set vec1 [vectorExprNew $vector_size]
    set vec2 [vectorExprNew $vector_size]
    # We are test the SPEED so we can assume the expression evaluator works!
    # The following expression fill the vector with only 1.23!
    exprEval "$vec2 = 7.89"

    puts "Start division evaluation through vectorExprEval"
    set res "[time {set vec4 [vectorExprEval "$vec1 / $vec2"];vectorExprDel $vec4} $repeat] for vectorExprEval"

    vectorExprDel $vec1
    vectorExprDel $vec2

    return $res
}
proc SEvalVecDiv { vector_size repeat } {

    puts "Initializing vector ... "
    set vec1 [vectorExprNew $vector_size]
    set vec2 [vectorExprNew $vector_size]
    set vec3 [vectorExprNew $vector_size]
    # We are test the SPEED so we can assume the expression evaluator works!
    # The following expression fill the vector with only 1.23!
    exprEval "$vec2 = 7.89"

    puts "Start division evaluation through expression evaluator"
    set res "[time {exprEval " $vec3 = $vec1 / $vec2"} $repeat] for exprEval"

    vectorExprDel $vec1;vectorExprDel $vec2;vectorExprDel $vec3
    return $res
}

proc VecExprLog { vector_size repeat } {

    puts "Initializing vector ... "
    set vec2 [vectorExprNew $vector_size]
    # We are test the SPEED so we can assume the expression evaluator works!
    # The following expression fill the vector with only 1.23!
    exprEval "$vec2 = 1.23"
    puts "Start log evaluation through vectorExprEval"
    set res "[time {set vec4 [vectorExprEval "lg($vec2)"];vectorExprDel $vec4} $repeat] for vectorExprEval"
    vectorExprDel $vec2

    return $res
}
proc SEvalVecLog { vector_size repeat } {

    puts "Initializing vector ... "
    set vec2 [vectorExprNew $vector_size]
    # We are testing the SPEED so we can assume the expression evaluator works!
    # The following expression fill the vector with only 1.23!
    exprEval "$vec2 = 1.23"
    set vec3 [vectorExprNew $vector_size]
    puts "Start log evaluation through expression evaluator"
    set res "[time {exprEval " $vec3 = log($vec2)"} $repeat] for exprEval"
    vectorExprDel $vec2;vectorExprDel $vec3
    return $res
}

# Tests to Compare exprEval to higher level tcl functions.

proc SEvalChainAdd { chain_size repeat } {

    puts "Initializing chain ... "
    set chain [chainNew TYPE1]
    loop i 0 $chain_size {
	set h [genericNew TYPE1]
	handleSet $h.i [randGet -min -200 -max 200]
	handleSet $h.f<0> [randGet -min -200 -max 200]
	chainElementAddByPos $chain $h
    }

    puts "Start Add evaluation with chain through expression evaluator"
    set res "[time {exprEval "$chain.l = $chain.i + $chain.f<0>"} $repeat] for exprEval"

    loop i 0 $chain_size {
	set h [chainElementRemByPos $chain 0]
	genericDel $h
    }
    chainDel $chain
    return $res
}

proc tclChainAdd { chain_size repeat } {

    puts "Initializing chain ... "
    set chain [chainNew TYPE1]
    loop i 0 $chain_size {
	set h [genericNew TYPE1]
	handleSet $h.i [randGet -min -200 -max 200]
	handleSet $h.f<0> [randGet -min -200 -max 200]
	chainElementAddByPos $chain $h
    }

    puts "Start Add evaluation with chain through tcl functions"
    set res "[time { \
        loop i 0 $chain_size { \
	    set h [chainElementGetByPos $chain $i]; \
	    handleSet $h.l [expr [exprGet $h.i]+[exprGet $h.f<0>] ];\
        }   } $repeat] for exprEval"

    loop i 0 $chain_size {
	set h [chainElementRemByPos $chain 0]
	genericDel $h
    }
    chainDel $chain
    return $res
}



# Theses following test requires sdb and database, we use the 
# global variable db to (eventually) pass the database pointer.

# this test is currently NOT clean!
proc SEvalAl2New { vector_size repeat } {
    set res "[time {exprEval "new al2 (1.0,1.0,32)"} $repeat]"
    return $res
}    

# This function will run any test using exprEval and report timing information
# about them
# WARNING: If the expression exprEval contain constructor they will be
#          executed "counts" number of time!!!!!

proc hookingExprEval { cmd } {
    global hookPerfFile hookInfo repeat res
    puts "Start the evaluation of $cmd for $repeat times"
    set result "[time {set val [straightExprEval $cmd]} $repeat] for $cmd"
    set tst_time [lindex $result 0]
    set res "$res$cmd: $tst_time ms \t$repeat samplings\n"
    return $val
}

proc TestByAlias { Tests PerfFile info } {
    global hookPerfFile hookInfo res
    rename exprEval straightExprEval
    alias exprEval hookingExprEval 
    
    set hookPerfFile $PerfFile
    set hookInfo $info
    set res ""
    $Tests
    
    unalias exprEval
    rename straightExprEval exprEval
    return $res
}

# We expect test to return a set of lines of the format:
#   128 milliseconds per iterations for ....
proc PerfTest { test sizes counts } {
    set res ""
    set len [llength $sizes]
    set clen [llength $counts]
    loop i 0 $len {
	if { $clen == 1 } { 
	    set j 0 
	} else { 
	    if { $i > $clen } {
		set j $clen 
	    } else {
		set j $i 
	    }
	}
	set size [lindex $sizes $i]
	set count [lindex $counts $j]
	echo [set t "$test:\t$size elements\t$count iterations"]
	set tst_time [lindex [$test $size $count] 0]
	set res "$res$test: $tst_time ms \t[expr 1.0*$tst_time/$size] micros/iter"
	set res "$res\t$size iter\t$count samplings\n"
    }
    return $res
}

proc Update_record { file info msg } {
    set fd [open "|funame -n"]
    regexp "(\[^\\.\]+)" [gets $fd] full machine
    close $fd
    set fd [open "|date"]
    set date [gets $fd]
    close $fd

    set fd [open $file a+]
    puts $fd "$machine\t$info\t$date\n$msg"
    close $fd

    return $msg
}

proc QuickAliasTest { cmds } {
    global PerfFile

    foreach cmd $cmds {
	Update_record $PerfFile {} [TestByAlias $PerfFile {}]
    }
}

proc misc_tests {} {

    QuickAliasTest "simplePtTest shortTest sXtest vecTest sssToVecTest sssToVecTest2 sssToVecTest3 sssToVecTest4 simplePtTest tblColTest tblColTest_Quote stringTest bad_test MemCpyTest MemCpyTest2 logical_test bitwise_test break_test index_test"

}
proc vec_test { PerfFile info} {
    global sizes counts
    Update_record $PerfFile $info [PerfTest VecExprAdd $sizes $counts]
    Update_record $PerfFile $info [PerfTest SEvalVecAdd $sizes $counts]
    Update_record $PerfFile $info [PerfTest VecExprMul $sizes $counts]
    Update_record $PerfFile $info [PerfTest SEvalVecMul $sizes $counts]
    Update_record $PerfFile $info [PerfTest VecExprDiv $sizes $counts]
    Update_record $PerfFile $info [PerfTest SEvalVecDiv $sizes $counts]
    Update_record $PerfFile $info [PerfTest VecExprLog $sizes $counts]
    Update_record $PerfFile $info [PerfTest SEvalVecLog $sizes $counts]
}

proc vec_test_echo {PerfFile info } {
    global sizes counts
    echo [Update_record $PerfFile $info [PerfTest VecExprAdd $sizes $counts]]
    echo [Update_record $PerfFile $info [PerfTest SEvalVecAdd $sizes $counts]]
    echo [Update_record $PerfFile $info [PerfTest VecExprMul $sizes $counts]]
    echo [Update_record $PerfFile $info [PerfTest SEvalVecMul $sizes $counts]]
    echo [Update_record $PerfFile $info [PerfTest VecExprDiv $sizes $counts]]
    echo [Update_record $PerfFile $info [PerfTest SEvalVecDiv $sizes $counts]]
    echo [Update_record $PerfFile $info [PerfTest VecExprLog $sizes $counts]]
    echo [Update_record $PerfFile $info [PerfTest SEvalVecLog $sizes $counts]]
}

proc chain_test {PerfFile info} {
    global sizes counts
    Update_record $PerfFile $info [PerfTest tclChainAdd $sizes $counts]
    Update_record $PerfFile $info [PerfTest SEvalChainAdd $sizes $counts]
}

proc chain_test_echo {PerfFile info} {
    global sizes counts
    echo [Update_record $PerfFile $info [PerfTest tclChainAdd $sizes $counts]]
    echo [Update_record $PerfFile $info [PerfTest SEvalChainAdd $sizes $counts]]
}

proc all_test {PerfFile info} {
    vec_test PerfFile info
    chain_test PerfFile info
}

proc all_test_echo {PerfFile info} {
    vec_test PerfFile info
    chain_test PerfFile info
}

alias test_default "all_test_echo $PerfFile {}"

