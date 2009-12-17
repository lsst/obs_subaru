# Grammar of the Test Expressions:

# test_expression: { simple_expressions }
# simple_expressions: simple_expressions simple_expressions
# simple_expression: { dest_handle dest_field { Operations } }
# Operations: Operation Operations
# Operations: { string src_handle src_field { ValueList } }`

# and the equivalent expression Evaluator syntax would be:

# exprEval "dest_handle.dest = string src_handle.src_field 
#                              string src_handle.src_field 
#                              ......;
#           dest_handle.dest = string src_handle.src_field 
#                              string src_handle.src_field 
#                              ......; "

# Typically for each simple_expressions you will have to:
#     create the src_handle references in the simple_expression
#     (simple handles, chains, vectors, tblcol, etc ... )
#     fillChains $Operations        # to correctly populate a chains

# and after executing the expression evaluator (exprEval "[createExprEvalScript #test_expression]")
# you should 
#      set result [calcResults $V]    # To get the expected results
#      set success [verifyResult $vec __vector $result]
#                                     # To compare the expected results
#                                     # with the real ones.
# And for each simple_expressions
#      cleanChains $simple_expressions
#                       # to undo the job of fillChains (but keep the elements
#                       # themselves
#      chainPurge $each_chains        # to delete the elements
#      delete all the containers used by all the simple_expressions
#      (chains, vectors, tblcols, handles)

# Limitation of this framework:
#   can not have variable indexes
#   can not do any computation in indexes
#   can not use something like (*...).
#   can not use list (chain, vector, etc. ) of different length

source tst_SE_def.tcl
source tst_SE_func.tcl

proc simplePtTest {} {
    set vec [vectorExprNew 10]
    set ch [chainNew PT]

    set V " { {} $ch id { 13 12 11 10 9 8 7 6 5 4 3 2 1 } } "

    fillChains $V
    exprEval [createExprEvalScript " { $vec __vector { $V } } "]
    set result [calcResults $V]
    if { [verifyResult $vec __vector $result] != 0 } {
	echo TEST-ERR(exprEval): Vector to Vector assignment failed
	return 1; 
    }
    cleanChains $V
    chainPurge $ch ptDel

    loop i 0 12 { chainElementAddByPos $ch [genericNew PT] }
    set V2 " { {} $vec __vector {98 93 29 32 32 894 3289 6 287 32987 123 123}}"
    fillChains $V2
    exprEval [createExprEvalScript " { $ch row { $V2 } }"]
    set result [calcResults $V2]
    if { [verifyResult $ch row $result]  != 0 } {
	echo TEST-ERR(exprEval): Vector to Chain (PT) assignment failed 
	return 1; 
    }
    cleanChains $V2
    chainPurge $ch ptDel
    
    set V3 "{ {} $vec __vector {0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9} } \
	    { + $ch row { 513 175 308 534 946 171 701 226 494 124 } } \
	    { * {} {} 2 } \
	    { + $ch col { 83 389 276 367 982 534 764 645 766 779 } } \
	    { * {} {} 3 } \
	    { - {} {} 5 }"
    fillChains $V3
    exprEval [createExprEvalScript " { $vec __vector {$V3 }} "]
    set result [calcResults $V3]
    if { [verifyResult $vec __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Vector to Chain (PT) assignment/operations failed
	return 1; 
    }
    cleanChains $V3
    chainPurge $ch ptDel

    chainDel $ch
    vectorExprDel $vec
    
    return 0;
}


proc sssToVecTest { } {
    set vec1 [vectorExprNew 10]

    set chain1 [chainNew PT]
    set chain2 [chainNew SSSS]
    set chain3 [chainNew RR]
    set handle1 [genericNew SSSS]

    set expression " \
	{ {} $vec1    __vector		 {0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9} }\
	{ +  $chain1  row		 {29 83 12 32 83 28 75 43 36 74} } \
	{ *  $chain2  value		 {23 23 49 98 83 02 39 98 24 42} } \
	{ +  $chain2  sss.total		 {02 39 43 87 23 98 74 23 98 73} } \
	{ +  $chain2  sss.ss<3><3>.a<5> {60 24 45 78 7 47 15 24 94 61 } }\
	{ *  $chain2  sssptr->id 	 {98 47 79 74 38 47 52 9 59 34 } } \
	{ +  $chain2  s2<3>.total 	 {14 77 71 44 70 9 96 55 74 57} } \
	{ +  $chain2  s2<5>.ss<2><1>.d 	 {70.223999 22.644043 49.478149 12.472534 8.392334 38.964844 27.725220 36.807251 98.345947 53.540039 } } \
	{ +  $chain2  s2<5>.ss<4><2>.a<5> {63 78 18 30 28 68 29 56 41 30 } } \
	{ +  $handle1 value		  32 } \
	{ +  $chain3  r<3>.ncol		 {44 56 48 60 41 13 25 3 97 11 } } \
	{ +  $chain3  a<1><2><3><4> 	 {37 64 35 55 35 56 47 16 61 17} }" 

    fillChains $expression
    exprEval [createExprEvalScript "{ $vec1 __vector { $expression }}"]
    set result [calcResults $expression]
    if { [verifyResult $vec1 __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Complexe structures' Chain to Vector assignment failed
	return 1; 
    }
    cleanChains $expression

    chainPurge $chain1 ptDel
    chainPurge $chain2 genericDel
    chainPurge $chain3 genericDel
    chainDel $chain1 
    chainDel $chain2 
    chainDel $chain3 
    genericDel $handle1
    vectorExprDel $vec1

    return 0
}

proc sssToVecTest2 { } {
    set vec1 [vectorExprNew 10]

    set chain1 [chainNew PT]
    set chain2 [chainNew SSSS]
    set chain3 [chainNew RR]
    set handle1 [genericNew SSSS]

    set expression " \
	{ {} $vec1    __vector		 {0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9} }\
	{ +  $chain1  row		 {29 83 12 32 83 28 75 43 36 74} } \
	{ +  $handle1 value		  32 } \
	{ +  $chain3  r<3>.ncol		 {44 56 48 60 41 13 25 3 97 11 } } \
	{ +  $chain3  a<1><2><3><4> 	 {37 64 35 55 35 56 47 16 61 17} }" 

    fillChains $expression
    exprEval [createExprEvalScript "{ $vec1 __vector {$expression}}"]
    set result [calcResults $expression]
    if { [verifyResult $vec1 __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Array Transversal test failed.
	return 1; 
    }
    cleanChains $expression

    chainPurge $chain1 ptDel
    chainPurge $chain2 genericDel
    chainPurge $chain3 genericDel
    chainDel $chain1 
    chainDel $chain2 
    chainDel $chain3 
    genericDel $handle1
    vectorExprDel $vec1

    return 0
}


proc sssToVecTest3 { } {
    set vec1 [vectorExprNew 10]

    set chain1 [chainNew PT]
    set chain2 [chainNew SSSS]
    set chain3 [chainNew RR]
    set handle1 [genericNew SSSS]

    set expression " \
	    { {}  $chain2  value		 {23 23 49 98 83 02 39 98 24 42} } \
	    { +  $chain2  sss.total		 {02 39 43 87 23 98 74 23 98 73} } \
	    { +  $chain2  sss.ss<3><3>.a<5> {60 24 45 78 7 47 15 24 94 61 } }\
	    { *  $chain2  sssptr->id 	 {98 47 79 74 38 47 52 9 59 34 } } \
	    { +  $chain2  s2<3>.total 	 {14 77 71 44 70 9 96 55 74 57} } \
	    { +  $chain2  s2<5>.ss<2><1>.d 	 {70.223999 22.644043 49.478149 12.472534 8.392334 38.964844 27.725220 36.807251 98.345947 53.540039 } } \
	    { +  $chain2  s2<5>.ss<4><2>.a<5> {63 78 18 30 28 68 29 56 41 30 } }" 

    fillChains $expression
    exprEval [createExprEvalScript "{$vec1 __vector {$expression}}"]
    set result [calcResults $expression]
    if { [verifyResult $vec1 __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Short versions of Complexe structures' Chain to Vector assignment failed
	return 1; 
    }
    cleanChains $expression

    chainPurge $chain1 ptDel
    chainPurge $chain2 genericDel
    chainPurge $chain3 genericDel
    chainDel $chain1 
    chainDel $chain2 
    chainDel $chain3 
    genericDel $handle1
    vectorExprDel $vec1

    return 0
}

proc sssToVecTest4 { } {
    set vec1 [vectorExprNew 10]
    set chain2 [chainNew SSSS]

    set expression "{ {}  $chain2  sss.total {02 39 43 87 23 98 74 23 98 73} }"
    fillChains $expression
    exprEval [createExprEvalScript "{ $vec1 __vector { $expression}}"]
    set result [calcResults $expression]
    if { [verifyResult $vec1 __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Simple Chain to Vector assignment failed
	return 1; 
    }
    cleanChains $expression

    chainPurge $chain2 genericDel
    chainDel $chain2 
    vectorExprDel $vec1

    return 0
}


proc __Test { target field expr } {
    fillChains $expr
    exprEval [createExprEvalScript "{ {$target} {$field} {$expr}}"]
    set result [calcResults $expr]
    if { [verifyResult $target $field $result]  != 0 } {
	return 1; 
    }
    cleanChains $target
    return 0 
}

proc __vecTest { vec expr } {
    return [__Test $vec __vector $expr]
}

proc vecTest {} {
    set vec1 [vectorExprNew 10]
    set vec2 [vectorExprNew 10]
    set vec3 [vectorExprNew 10]

    #    exprEval "$vec3 = $vec1 + $vec2"
    set expr1 " \
	    { {} $vec1 __vector {0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9}} \
	    { +  $vec2 __vector {9   8   7   6   5   4   3   2   1   0} } "
    if { [__vecTest $vec3 $expr1] == 1 } { 
	echo TEST-ERR(exprEval): Vector to Vector assignment failed
	return 1 
    }

    #    exprEval "$vec3 = sin($vec1) - cos($vec2)"
    set expr2 " \
	    { {sin(}  $vec1 __vector {0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9}} \
	    { {)-cos(} $vec2 __vector {9   8   7   6   5   4   3   2   1   0} } \
	    { {)} {} {} {} } "
 
    if { [__vecTest $vec3 $expr2] == 1 } { 
	echo TEST-ERR(exprEval): Vector to Vector assignment/cosinus failed
	return 1 
    }

    #    exprEval "$vec3 = asin(sin($vec1)) - $vec1"
    set expr3 " \
	    { {asin(sin(}  $vec1 __vector {0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9}} \
	    { {))-} $vec1 __vector {0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9} } " 
    if { [__vecTest $vec3 $expr3] == 1 } { 
	echo TEST-ERR(exprEval): Vector to Vector assigment/sinus failed
	return 1 
    }

    set chain1 [chainNew PT]
    set expr4 " \
	    { {} $vec1 __vector {0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9} } \
	    { +  $chain1 row    {0   1   2   3   4   5   6   7   8   9} }"  
    if { [__vecTest $vec3 $expr3] == 1 } { 
	echo TEST-ERR(exprEval): PT Chain to Vector assignment failed
	return 1 
    }
    chainPurge $chain1 genericDel
    
    vectorExprDel $vec1
    vectorExprDel $vec2
    vectorExprDel $vec3
    chainDel $chain1

    return 0
}    

proc sXtest {} {
    set chain1 [chainNew S2]
    set vec [vectorExprNew 10]
    
    set expression " \
	    { {} $chain1 a<2><3>.f { 4 8 12 16 20 24 28 32 36 40} } \
	    { +  $chain1 a<1><2>.f { 2.33333 6.33333 10.3333 14.3333 18.3333 \
	                            22.3333 26.3333  30.3333 34.3333 38.3333} } "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): Array reading failed.
	return 1 
    }
    cleanChains $chain1
    chainPurge $chain1 genericDel

    set expression " \
	    { {} $chain1 b<2>.id { 4 8 12 16 20 24 28 32 36 40} } \
	    { +  $chain1 b<1>.id { 2 6 10 14 18 22 26 33 34 38} } "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): Array reading failed.
	return 1 
    }
    cleanChains $chain1
    chainPurge $chain1 genericDel

    chainDel $chain1
    vectorExprDel $vec

    return 0
}

proc shortTest {} {
    set vec [vectorExprNew 1]
    set ch [chainNew SSSS]
#    chainElementAddByPos $ch [genericNew SSSS]
 
#    echo [exprEval "$vec = $ch.sss.total" -debug 1]
#    return [exprEval "$vec = $ch.sss.total"]
    set expression "{ {} $ch sss.total { 2 } }"
    set rtn [__vecTest $vec $expression]
    
    vectorExprDel $vec
    chainPurge $ch genericDel
    chainDel $ch

    return $rtn
}

proc tblColTest {} {
    set t1 [tblColNew]
    tblFldAdd $t1 -type INT -dim {10 2 2}
    tblFldAdd $t1 -type DOUBLE -dim {10}
    tblFldInfoSet $t1 -col 0 TTYPE field1
    tblFldInfoSet $t1 -col 1 TTYPE field2
    
    tblFldSet $t1 -field field2 {0} { \
	    0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9}
	
    tblFldSet $t1 -field field1 {0 0 0} { \
	    000 001 010 011 100 101 110 111 \
	    200 201 210 211 300 301 310 311 \
	    400 401 410 411 500 501 510 511 \
	    600 601 610 611 700 701 710 711 \
	    800 801 810 811 900 901 910 911}
    
    set v1 [vectorExprNew 10]

    set expression " { {} $t1 field2 {0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9} } "
    if { [__vecTest $v1 $expression] == 1 } { 
	echo TEST-ERR(exprEval): TBLCOL test failed
	return 1 
    }

    set expression " { {} $t1 field1<1><1> {11 111 211 311 411 511 611 711 811 911} } "
    if { [__vecTest $v1 $expression] == 1 } { 
	echo TEST-ERR(exprEval): TBLCOL test failed
	return 1
    }    

    set expression " { {} $v1 __vector {0 1 2 3 4 5 6 7 8 9} } "
    if { [__Test $t1 field1<1><1> $expression] == 1 } { 
	echo TEST-ERR(exprEval): TBLCOL test failed
	return 1 
    }    
    
    tblColDel $t1
    vectorExprDel $v1
    return 0
}
  
proc tblColTest_Quote {} {
    set t1 [tblColNew]
    tblFldAdd $t1 -type INT -dim {10 2 2}
    tblFldAdd $t1 -type DOUBLE -dim {10}
    tblFldAdd $t1 -type DOUBLE -dim {10}
    tblFldAdd $t1 -type DOUBLE -dim {10}
    tblFldInfoSet $t1 -col 0 TTYPE "field 1"
    tblFldInfoSet $t1 -col 1 TTYPE "field.2"
    tblFldInfoSet $t1 -col 2 TTYPE "long.field.name_just_for?the fun: of it"
    tblFldInfoSet $t1 -col 3 TTYPE "???!!!"
    
    tblFldSet $t1 -field "field.2" {0} { \
	    0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9}
    tblFldSet $t1 -field "long.field.name_just_for?the fun: of it" {0} { \
	   683.357346 3489.699123 2457.980540 3291.647253 8940.467570 4827.599735 6941.460291 5847.272730 6954.906528 7075.082274}
    tblFldSet $t1 -field "???!!!" {0} { \
	   4630.108124 1526.268347 2746.234252 4819.756096 8611.595016 1489.571324 6359.070138 1991.564183 4454.746780 1057.891081 }
	
    tblFldSet $t1 -field "field 1" {0 0 0} { \
	    000 001 010 011 100 101 110 111 \
	    200 201 210 211 300 301 310 311 \
	    400 401 410 411 500 501 510 511 \
	    600 601 610 611 700 701 710 711 \
	    800 801 810 811 900 901 910 911}
    
    set v1 [vectorExprNew 10]

    set expression " { {} $t1 field.2 {0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9} } "
    if { [__vecTest $v1 $expression] == 1 } { 
	echo TEST-ERR(exprEval): Tblcol field name test failed
	return 1 
    }
    set expression " { {} $t1 {long.field.name_just_for?the fun: of it} { 683.357346 3489.699123 2457.980540 3291.647253 8940.467570 4827.599735 6941.460291 5847.272730 6954.906528 7075.082274} }"
    if { [__vecTest $v1 $expression] == 1 } { 
	echo TEST-ERR(exprEval): Tblcol field name test failed
	return 1 
    }
    set expression " { {} $t1 {???!!!} {4630.108124 1526.268347 2746.234252 4819.756096 8611.595016 1489.571324 6359.070138 1991.564183 4454.746780 1057.891081} }"
    if { [__vecTest $v1 $expression] == 1 } { 
	echo TEST-ERR(exprEval): Tblcol field name test failed
	return 1 
    }

    set expression " { {} $t1 {field 1<1><1>} {11 111 211 311 411 511 611 711 811 911} } "
    if { [__vecTest $v1 $expression] == 1 } { 
	echo TEST-ERR(exprEval): Tblcol field name test failed
	return 1 
    }    
    set expression " { {} $v1 __vector {0 1 2 3 4 5 6 7 8 9} } "
    if { [__Test $t1 {long.field.name_just_for?the fun: of it} $expression] == 1 } { return 1 }    
    set expression " { {} $v1 __vector {0 1 2 3 4 5 6 7 8 9} } "
    if { [__Test $t1 {???!!!} $expression] == 1 } { 
	echo TEST-ERR(exprEval): Tblcol field name test failed
	return 1 
    }    
    set expression " { {} $v1 __vector {0 1 2 3 4 5 6 7 8 9} } "
    if { [__Test $t1 {field 1<1><1>} $expression] == 1 } { 
	echo TEST-ERR(exprEval): Tblcol field name test failed
	return 1 
    }    

    tblColDel $t1
    vectorExprDel $v1
    return 0
}

proc S_STRDEL { object } {
    set addr [exprGet $object.name -nostring]
    set b [memBlocksGet]
    if { [catch { keylget b $addr }] == 0 } {
	set hand [handleBindNew $addr PTR]
	shFree $hand
    }
    genericDel $object
}

proc stringTest {} {
    set ori_ch [chainNew S_STR]
    set res_ch [chainNew S_STR]
    set vec [vectorExprNew 24]

    set result_init ""
    loop i 0 [expr 3*8] { 
	set obj [genericNew S_STR]
	handleSet $obj.name $obj
	chainElementAddByPos $res_ch $obj;
	set result_init "$result_init \"{Original Label}\" ";
    }

    set expression "{ {strcpy( } $ori_ch Label { {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label_1} {Label 1} {Label 1} {Label()1} {Label 1} {Label 1} {Label 1} {Label 1}  {Label^1} {Label\\1} {Label 1} {Label 1} {Label 1} {Label 1} } } { {)} {} {} {} }"
    fillChains $expression

    exprEval "strcpy ($res_ch.Label,\"Original Label\")"
    if { [verifyResult $res_ch Label $result_init]  != 0 } {
	echo TEST-ERR(exprEval): Strcpy Test Failed.
	return 1; 
    }

    set result { {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1}  {Label 1} {Label_1} {Label 1}  {Label 1} {Label()1} {Label 1}  {Label 1} {Label 1} {Label 1}  {Label^1} {Label\\1} {Label 1}  {Label 1} {Label 1} {Label 1} }
    exprEval "strcpy ($res_ch.Label,$ori_ch.Label)"
    if { [verifyResult $res_ch Label $result]  != 0 } {
	echo TEST-ERR(exprEval): Strcpy Test Failed.
	return 1; 
    }

    set result ""
    loop i 0 24 { set result "$result 0" }
    exprEval "$vec = strcmp ($res_ch.Label,$ori_ch.Label)"
    if { [verifyResult $vec __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Strcmp Test Failed.
	return 1; 
    }

    set expression "{ {strcpy( } $ori_ch Label { {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label 1} {Label_1} {Label 1} {Label 1} {Label()1} {Label 1} {Label 1} {Label 1} {Label 1}  {Label^1} {Label\\1} {Label 1} {Label 1} {Label 1} {Label 1} } } { {)} {} {} {} }"
    fillChains $expression
    
    exprEval "strcpy ($res_ch.Label,\"Original Label\")"
    if { [verifyResult $res_ch Label $result_init]  != 0 } {
	echo TEST-ERR(exprEval): Strcpy Test Failed.
	return 1; 
    }
    exprEval "strncpy ($res_ch.Label,$ori_ch.Label,12-9)"
    set result ""
    loop i 0 24 { set result "$result \"Labginal Label\" " }
    if { [verifyResult $res_ch Label $result]  != 0 } {
	echo TEST-ERR(exprEval): Strncpy Test Failed.
	return 1; 
    }
    
    exprEval "$vec = strcasecmp(strcat($res_ch.Label , strcpy($ori_ch.Label,\"A la Vista Baby\")),\"lABgInAL LABELa LA VISTA BABY\")"
    set result ""
    loop i 0 24 { set result "$result 0" }
    if { [verifyResult $vec __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Strcasecmp/strcat Test Failed.
	return 1; 
    }    

    set expression "{ {strlen( } $ori_ch Label { {1} {_2} __3 ___4 ____5 _____6  ______7 _______8  ________9 ________10 _________11 __________12 ___________13 ____________14 _____________15 _____________16 ______________17 _______________18 ________________19 _________________20 _________________21 __________________22 ___________________23 ____________________24 {kjhsad kasjhd 1987 ksajhd} } } { {)} {} {} {} }"
    fillChains $expression
    set result ""
    loop i 0 24 { set result "$result [expr $i+1]" }
    exprEval "$vec = strlen ($ori_ch.Label)"
    if { [verifyResult $vec __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Strlen Test Failed.
	return 1; 
    }    
    
    set Long_String {"This is a very Long string et meme qu'elle contient un peu de francais"}
    exprEval "$vec = strncmp( \
                              strncat( \
                                       strncpy( \
                                                strcpy($res_ch.Label,\"\"), \
                                                $Long_String, \
                                                strlen($Long_String)-$$), \
                                       strcpy( $ori_ch.Label, $Long_String), \
                                       $$ \
                                      ), \
			       $Long_String, \
                              $$)" 
    set result ""
    loop i 0 24 { set result "$result 0" }
    if { [verifyResult $vec __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Multiple Instruction Test 1 Failed.
	return 1; 
    }    

    exprEval "$vec = strncmp( \
                              stroffset( \
                                 strncat( \
                                       strcpy ( \
				                stroffset ( \
						            strncpy( \
                                                                     strcpy($res_ch.Label,\"\"), \
								     $Long_String, \
								     strlen($Long_String)-$$), \
						            strlen($Long_String)-$$), \
                                                \"\"), \
                                       stroffset( \
				                strcpy( $ori_ch.Label, $Long_String), \
						strlen($Long_String)-$$), \
                                       $$ \
                                      ), \
                                 -strlen($Long_String)+$$), \
                              $Long_String, \
                              $$)" 
 
    set result ""
    loop i 0 24 { set result "$result 0" }
    if { [verifyResult $vec __vector $result]  != 0 } {
	echo TEST-ERR(exprEval): Multiple Instruction Test 2 Failed.
	return 1; 
    }    
    
    cleanChains $expression
    chainPurge $res_ch S_STRDEL
    chainDel $res_ch
    chainPurge $ori_ch S_STRDEL
    chainDel $ori_ch
    vectorExprDel $vec

    return 0
}

proc MemCpyVerifySS { ch } {
    upvar 1 id_val id_val f_val f_val d_val d_val a_val a_val

    if { [verifyResult $ch id $id_val] != 0 } {
	echo TEST-ERR(exprEval): Memcopy test -- id field not properly copied
	return 1; 
    }
    if { [verifyResult $ch f $f_val] != 0 } {
	echo TEST-ERR(exprEval): Memcopy test -- f field not properly copied
	return 1; 
    }
    if { [verifyResult $ch d $d_val] != 0 } {
	echo TEST-ERR(exprEval): Memcopy test -- d field not properly copied
	return 1; 
    }
    loop i 0 15 { 
	if { [verifyResult $ch a<$i> $a_val($i)] != 0 } {
	    echo TEST-ERR(exprEval): Memcopy test -- a<$i> field not properly copied
	    return 1; 
	}
    }
    return 0
}

# Test the ability of exprEval to copy  block of memory when
# both side of the assignment are 'compatible'
proc MemCpyTest { } {

    set chain1 [chainNew SS]
    set chain2 [chainNew SS]


    set id_val "27 -648 -382 69 895 -656 404 -547 -10 -750"
    set f_val "-832.153320 -220.703125 -445.495605 -263.854980 966.918945 70.800781 531.372070 292.968750 534.301758 560.485840"
    set d_val " 645.935059 -696.105957 250.976562 -370.605469 -306.152344 834.411621 39.550781 -197.631836 213.562012 570.861816"
    set a_val(0) " 746 -146 -283 -236 -913 -678 44 393 -805 -198"
    set a_val(1) " 546 -510 -314 -539 -404 -390 774 -926 302 -202"
    set a_val(2) " 352 465 875 -533 677 934 557 -136 348 618"
    set a_val(3) " -682 -440 -729 728 500 -583 -720 -410 605 -562"
    set a_val(4) " 126 431 -604 979 -499 -138 510 721 789 956"
    set a_val(5) " -209 -135 -745 -84 -524 972 305 208 -516 -90"
    set a_val(6) " 579 -842 -47 -694 -508 890 228 976 -45 599"
    set a_val(7) " 488 -238 -40 53 -803 188 -305 -713 559 421"
    set a_val(8) " -107 409 -809 925 102 480 158 275 563 -624"
    set a_val(9) " -395 -434 368 -414 130 -163 -386 -110 131 -24"
    set a_val(10) " 213 -168 -739 -488 -928 954 -770 -243 293 -299"
    set a_val(11) " 106 -283 130 -48 -672 230 -655 109 -415 744"
    set a_val(12) " 670 689 791 189 81 -663 309 381 -472 -786"
    set a_val(13) " 629 -617 -153 -296 678 -725 -474 -645 -40 -239"
    set a_val(14) " 9 5 -296 51 -758 39 214 465 113 -311"
    set a_val(15) " 603 182 -466 341 104 577 775 780 -863 601"
    set chainFillExpression " \
	    { {} $chain1 id { $id_val } } \
            { {} $chain1 f  { $f_val } } \
            { {} $chain1 d  { $d_val } } \ 
            { {} $chain1 a<0> { $a_val(0) } } \
            { {} $chain1 a<1> { $a_val(1) } } \
            { {} $chain1 a<2> { $a_val(2) } } \
            { {} $chain1 a<3> { $a_val(3) } } \
            { {} $chain1 a<4> { $a_val(4) } } \
            { {} $chain1 a<5> { $a_val(5) } } \
            { {} $chain1 a<6> { $a_val(6) } } \
            { {} $chain1 a<7> { $a_val(7) } } \
            { {} $chain1 a<8> { $a_val(8) } } \
            { {} $chain1 a<9> { $a_val(9) } } \
            { {} $chain1 a<10> { $a_val(10) } } \
            { {} $chain1 a<11> { $a_val(11) } } \
            { {} $chain1 a<12> { $a_val(12) } } \
            { {} $chain1 a<13> { $a_val(13) } } \
            { {} $chain1 a<14> { $a_val(14) } } \
            { {} $chain1 a<15> { $a_val(15) } } "
    fillChains $chainFillExpression
   
    set chainEmptyExpression "\ 
            { {} $chain2 id { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 f { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 d { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<0> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<1> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<2> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<3> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<4> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<5> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<6> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<7> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<8> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<9> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<0> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<11> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<12> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<13> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<14> { 0 0 0 0 0 0 0 0 0 0 } } \
            { {} $chain2 a<15> { 0 0 0 0 0 0 0 0 0 0 } } "
    fillChains $chainEmptyExpression

    exprEval "$chain2 = $chain1"

    if { [MemCpyVerifySS $chain2] != 0 } { return 1 }

    fillChains $chainEmptyExpression
    exprEval "$chain2.a = $chain1.a"
    if { [exprGet [chainElementGetByPos $chain2 0].id] != 0 } {
	echo TEST-ERR(exprEval): Memcopy test -- id field unexpectivily copied
	return 1; 
    }
    loop i 0 15 { 
	if { [verifyResult $chain2 a<$i> $a_val($i)] != 0 } {
	    echo TEST-ERR(exprEval): Memcopy test -- a<$i> field not properly copied
	    return 1; 
	}
    }

    cleanChains $chainFillExpression
    chainPurge $chain1 genericDel
    chainPurge $chain2 genericDel
    chainDel $chain1
    chainDel $chain2
    

    return 0
}

proc MemCpyTest2 {} {
    
    set chain1 [chainNew SS]
    set chain2 [chainNew SSS]
    set chain3 [chainNew SSS]
    set chain4 [chainNew SS]

    set id_val "27 -648 -382 69 895 -656 404 -547 -10 -750"
    set f_val "-832.153320 -220.703125 -445.495605 -263.854980 966.918945 70.800781 531.372070 292.968750 534.301758 560.485840"
    set d_val " 645.935059 -696.105957 250.976562 -370.605469 -306.152344 834.411621 39.550781 -197.631836 213.562012 570.861816"
    set a_val(0) " 746 -146 -283 -236 -913 -678 44 393 -805 -198"
    set a_val(1) " 546 -510 -314 -539 -404 -390 774 -926 302 -202"
    set a_val(2) " 352 465 875 -533 677 934 557 -136 348 618"
    set a_val(3) " -682 -440 -729 728 500 -583 -720 -410 605 -562"
    set a_val(4) " 126 431 -604 979 -499 -138 510 721 789 956"
    set a_val(5) " -209 -135 -745 -84 -524 972 305 208 -516 -90"
    set a_val(6) " 579 -842 -47 -694 -508 890 228 976 -45 599"
    set a_val(7) " 488 -238 -40 53 -803 188 -305 -713 559 421"
    set a_val(8) " -107 409 -809 925 102 480 158 275 563 -624"
    set a_val(9) " -395 -434 368 -414 130 -163 -386 -110 131 -24"
    set a_val(10) " 213 -168 -739 -488 -928 954 -770 -243 293 -299"
    set a_val(11) " 106 -283 130 -48 -672 230 -655 109 -415 744"
    set a_val(12) " 670 689 791 189 81 -663 309 381 -472 -786"
    set a_val(13) " 629 -617 -153 -296 678 -725 -474 -645 -40 -239"
    set a_val(14) " 9 5 -296 51 -758 39 214 465 113 -311"
    set a_val(15) " 603 182 -466 341 104 577 775 780 -863 601"
    set ssFillExpression " \
	    { {} $chain1 id { $id_val } } \
            { {} $chain1 f  { $f_val } } \
            { {} $chain1 d  { $d_val } } \ 
            { {} $chain1 a<0> { $a_val(0) } } \
            { {} $chain1 a<1> { $a_val(1) } } \
            { {} $chain1 a<2> { $a_val(2) } } \
            { {} $chain1 a<3> { $a_val(3) } } \
            { {} $chain1 a<4> { $a_val(4) } } \
            { {} $chain1 a<5> { $a_val(5) } } \
            { {} $chain1 a<6> { $a_val(6) } } \
            { {} $chain1 a<7> { $a_val(7) } } \
            { {} $chain1 a<8> { $a_val(8) } } \
            { {} $chain1 a<9> { $a_val(9) } } \
            { {} $chain1 a<10> { $a_val(10) } } \
            { {} $chain1 a<11> { $a_val(11) } } \
            { {} $chain1 a<12> { $a_val(12) } } \
            { {} $chain1 a<13> { $a_val(13) } } \
            { {} $chain1 a<14> { $a_val(14) } } \
            { {} $chain1 a<15> { $a_val(15) } } "
    fillChains $ssFillExpression

    set zero "0 0 0 0 0 0 0 0 0 0"
    set sssEmpty "{ {} $chain2 id {$zero} } { {} $chain3 id {$zero} }"
    set ssEmpty "{ {} $chain4 id {$zero} }"
    # this does create the element of the chain
    # we rely on genericNew to create them zeroed out.
    fillChains $sssEmpty
    fillChains $ssEmpty
    
    exprEval "$chain2.ss<$$><1> = $chain1"
    exprEval "$chain4 = $chain2.ss<$$><1>"

    if { [MemCpyVerifySS $chain4] != 0 } { return 1 }
    # empty chain4
    cleanChains $ssEmpty
    fillChains $ssEmpty

    exprEval "$chain3.ss = $chain2.ss"
    exprEval "$chain4 = $chain3.ss<$$><1>"

    if { [MemCpyVerifySS $chain4] != 0 } { return 1 }

    # reset every thing.
    cleanChains $sssEmpty
    fillChains $sssEmpty
    
    exprEval "$chain2.ss<$$/5><$$ - 5*((int)$$/5)> = $chain1"
    # tcl 'expr' function round int to the next superior int !!
    loop i 0 10 { 
	set h [chainElementGetByPos $chain4 $i]
	set h2 [chainElementGetByPos $chain2 $i]
	handleSetFromHandle $h "$h2.ss<[expr $i/5]><[expr $i-5*[expr $i/5]]>"
    }
    
    if { [MemCpyVerifySS $chain4] != 0 } { return 1 }
    # empty chain4
    cleanChains $ssEmpty
    fillChains $ssEmpty

    exprEval "$chain3.ss<0> = $chain2.ss<0>"
    exprEval "$chain3.ss<1> = $chain2.ss<1>"
    loop i 0 10 { 
	set h [chainElementGetByPos $chain4 $i]
	set h2 [chainElementGetByPos $chain3 $i]
	handleSetFromHandle $h "$h2.ss<[expr $i/5]><[expr $i-5*[expr $i/5]]>"
    }
    if { [MemCpyVerifySS $chain4] != 0 } { return 1 }

    # This is NOT sure to work !!!
    cleanChains $sssEmpty
    cleanChains $ssEmpty
    cleanChains $ssFillExpression
    chainPurge $chain1 genericDel
    chainPurge $chain2 genericDel
    chainPurge $chain3 genericDel
    chainPurge $chain4 genericDel
    chainDel $chain1
    chainDel $chain2
    chainDel $chain3
    chainDel $chain4
    
    return 0
}

# Test a specific bug in the interaction  of lex and yacc.
# Lex was keeping aroung a symbol when failing.  Consequence:
# when parsing the next expression it was unexpectively failing
# because of the "added" symbol!
proc bad_test {} {
    set h_1 [genericNew S_STR]
    set h_2 [genericNew S_STR]
    handleSet $h_1.Label "OneOneOne"
    handleSet $h_2.Label "TwoTwoTwo"
    set c1 [catch {exprEval "strncpy($h_1.Label,h9909.Label,3)"} err1]
#    echo Catch retuns: --$c1-- , --$err1--.
    set c2 [catch {exprEval "strncpy($h_1.Label,$h_2.Label,3)"} err2]
#    echo Catch retuns: --$c2-- , --$err2--.
    set c3 [catch {exprEval "strncpy($h_1.Label,$h_2.Label,3)"} err3]
#    echo Catch retuns: --$c3-- , --$err3--.
    set c3 [catch {exprEval "strncpy($h_1.Label,$h_2.Label,3)"} err3]
#    echo Catch retuns: --$c3-- , --$err3--.
    set c3 [catch {exprEval "strncpy($h_1.Label,$h_2.Label,3)"} err3]
#    echo Catch retuns: --$c3-- , --$err3--.
    set c3 [catch {exprEval "strncpy($h_1.Label,$h_2.Label,3)"} err3]
#    echo Catch retuns: --$c3-- , --$err3--.
    
    # We expect c1 to be a 1 since we put a bad handle
    # We expect c2 to be a 0 since we tried to write a correct expression:
    if { $c1 != 1 } {
	echo TEST-ERR(exprEval): Did not detect a bad handle.
	return 1; 
    }
    if { $c2 != 0 } {
	echo TEST-ERR(exprEval): Unexpected syntax error!
	return 1; 
    }
    genericDel $h_1
    genericDel $h_2

    return 0	
}

# Test The logical operation:
#   x?y:z,and, or, etc ...

proc logical_test {} {
    set size 20
    set chain1 [chainNew SS]
    set vec [vectorExprNew $size]

    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {?} $chain1 f { [produceFloat $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z test failed
	return 1 
    }
    cleanChains $chain1
    chainPurge $chain1 genericDel

    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {?} $chain1 f { [produceFloat $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 2nd test failed
	return 1 
    }
    cleanChains $chain1
    chainPurge $chain1 genericDel

    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {*} $chain1 a<2> { [produceInt $size 0 2] } } \
	    { {?} $chain1 f { [produceFloat $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 3rd test failed
	return 1 
    }
    cleanChains $chain1
    chainPurge $chain1 genericDel

    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {*} $chain1 f { [produceFloat $size 0 2] } } \
	    { {?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 4th test failed
	return 1 
    }
    cleanChains $chain1
    chainPurge $chain1 genericDel
   
    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {*} $chain1 f { [produceFloat $size 0 2] } } \
	    { {+} $chain1 a<3> { [produceInt $size 0 2] } } \
	    { {?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 5th test failed
	return 1 
    }
    cleanChains $chain1
    chainPurge $chain1 genericDel
   
    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {&&} $chain1 f { [produceFloat $size 0 2] } } \
	    { {?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 6th test failed
	return 1 
    }
    cleanChains $chain1
    chainPurge $chain1 genericDel
   
    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {&&} $chain1 f { [produceFloat $size 0 2] } } \
	    { {|| !} $chain1 a<4> { [produceInt $size -2 0] } } \
	    { {?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 7th test failed
	return 1 
    }
    chainPurge $chain1 genericDel
   
    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {>} $chain1 f { [produceFloat $size 0 2] } } \
	    { {|| !} $chain1 a<4> { [produceInt $size -2 0] } } \
	    { {?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 8th test failed
	return 1 
    }
    chainPurge $chain1 genericDel

    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 2] } } \
	    { {<=} $chain1 f { [produceFloat $size 0 2] } } \
	    { {+} $chain1 a<4> { [produceInt $size -2 0] } } \
	    { {?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 9th test failed
	return 1 
    }
    chainPurge $chain1 genericDel

    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 5] } } \
	    { {==} $chain1 a<4> { [produceInt $size 0 5] } } \
	    { {?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {/} $chain1 a<0> { [produceInt $size -100 -1] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size -10000 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 10th test failed
	return 1 
    }
    chainPurge $chain1 genericDel

    set expression " \
	    { {}  $chain1 f { [produceFloat $size -1 1] } } \
	    { {-} $chain1 d { [produceFloat $size -1 1] } } \
	    { {<= 0.001 ?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 a<3> { [produceInt $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 11th test failed
	return 1 
    }
    chainPurge $chain1 genericDel

    set expression " \
	    { {}  $chain1 id { [produceInt $size 0 3] } } \
	    { {!=} $chain1 a<4> { [produceInt $size 0 3] } } \
	    { {?} $chain1 a<2> { [produceInt $size -100 0] } } \
	    { {+} $chain1 a<0> { [produceInt $size -100 0] } } \
	    { {:} $chain1 d { [produceFloat $size 0 100] } } \
	    { {*} $chain1 a<1> { [produceInt $size 0 100] } } \
	 "
    if { [__vecTest $vec $expression] == 1 } { 
	echo TEST-ERR(exprEval): x?y:z 12th test failed
	return 1 
    }
    chainPurge $chain1 genericDel

    chainDel $chain1
    vectorExprDel $vec
    return 0
}

proc bitwise_test {} {
    set size 20
    set chain1 [chainNew SS]
    set vec [vectorExprNew $size]

    set expression " \
	    { {} $chain1 a<0> { [produceInt $size 0 32767] } } \
	    { &  $chain1 a<1> { [produceInt $size 0 32767] } } \
	    "
    if { [__Test $chain1 id $expression] == 1 } { 
	echo TEST-ERR(exprEval): '&' Bit wise test failed.
	return 1 
    }
    chainPurge $chain1 genericDel
    set expression " \
	    { {} $chain1 a<0> { [produceInt $size 0 32767] } } \
	    { |  $chain1 a<1> { [produceInt $size 0 32767] } } \
	    "
    if { [__Test $chain1 id $expression] == 1 } { 
	echo TEST-ERR(exprEval): '|' Bit wise test failed.
	return 1 
    }
    chainPurge $chain1 genericDel
    set expression " \
	    { {} $chain1 a<0> { [produceInt $size 0 32767] } } \
	    { ^  $chain1 a<1> { [produceInt $size 0 32767] } } \
	    "
    if { [__Test $chain1 id $expression] == 1 } { 
	echo TEST-ERR(exprEval): '^' Bit wise test failed.
	return 1 
    }
    chainPurge $chain1 genericDel
    set expression " \
	    { {} $chain1 a<0> { [produceInt $size 0 32767] } } \
	    { ^  $chain1 a<1> { [produceInt $size 0 32767] } } \
	    { &  $chain1 a<2> { [produceInt $size 0 32767] } } \
	    { |  $chain1 a<3> { [produceInt $size 0 32767] } } \
	    "
    if { [__Test $chain1 id $expression] == 1 } { 
	echo TEST-ERR(exprEval): '^,&,|' Bit wise test failed.
	return 1 
    }
    chainPurge $chain1 genericDel
    set expression " \
	    { {} $chain1 a<0> { [produceInt $size 0 32767] } } \
	    { >>  $chain1 a<1> { [produceInt $size 0 8] } } \
	    "
    if { [__Test $chain1 id $expression] == 1 } { 
	echo TEST-ERR(exprEval): '>>' Bit wise test failed.
	return 1 
    }
    chainPurge $chain1 genericDel
    set expression " \
	    { {} $chain1 a<0> { [produceInt $size 0 32767] } } \
	    { <<  $chain1 a<1> { [produceInt $size 0 8] } } \
	    "
    if { [__Test $chain1 id $expression] == 1 } { 
	echo TEST-ERR(exprEval): '<<' Bit wise test failed.
	return 1 
    }
    chainPurge $chain1 genericDel

    chainDel $chain1
    vectorExprDel $vec
    return 0
}

proc break_test {} {
    set size 20
    set v1 [vectorExprNew $size]
    set v2 [vectorExprNew $size]
    set v3 [vectorExprNew $size]
    
    set expression " \
	    { {}  $v1 __vector { [produceFloat $size -100 100] } } \
	    { {< 0 ? continue :} $v2 __vector { [produceInt $size 98 99] } } \
	    "
    fillChains $expression
    set result ""
    loop i 0 [exprGet $v1.dimen] {
	if { [exprGet $v1.vec<$i>] < 0 } {
	    set result "$result [exprGet $v3.vec<$i>]" 
	} else { 
	    set result "$result [exprGet $v2.vec<$i>]" 
	}
    }
    set iter [exprEval [createExprEvalScript "{ {$v3} {__vector} {$expression}}"]]
    if { $iter != $size } {
	echo "TEST ERROR: only $iter iterations ($size expected) in break_test 1st test"
    }
    if { [verifyResult $v3 __vector $result]  != 0 } {
	return 1; 
    }

    set expression " \
	    { {}  $v1 __vector { [produceFloat $size -100 100] } } \
	    { {< 0 ? continue : break;} $v2 __vector { [produceInt $size 98 99] } } \
	    "
    fillChains $expression
    set result ""
    loop i 0 [exprGet $v1.dimen] {
	if { [exprGet $v1.vec<$i>] < 0 } {
	    continue
	} else { 
	    break
	}
    }
    set iter [exprEval [createExprEvalScript "{ {$v3} {__vector} {$expression}}"]]
    if { $iter != $i } {
	echo "TEST ERROR: only $iter iterations ($size expected) in break_test 1st test"
    }

    cleanChains $expression
    vectorExprDel $v1
    vectorExprDel $v2
    vectorExprDel $v3
    return 0
}

proc index_test {} {
    set size 20
    set chain1 [chainNew WITH_ARRAY]
    set vec1 [vectorExprNew $size]
    set vecJ [vectorExprNew $size]

    loop i 0 $size {
	set expression " \
		{ {} $chain1 i<$i> { [produceInt $size 0 31] } } "
	fillChains $expression
    }
    set expression "{ {} $chain1 j { [produceInt $size 0 31] } }"
    fillChains $expression
    
    if { [__vecTest $vecJ $expression] == 1 } { 
	echo TEST-ERR(exprEval): In index_test Simple vec = chain.j failed!
	return 1 
    }
    
    exprEval "$vec1 = $chain1.i<$chain1.j>"
    set res ""
    loop i 0 $size {
	set h [chainElementGetByPos $chain1 $i]
	set res "$res [exprGet $h.i<[exprGet $h.j]>]"
    }
    if { [verifyResult $vec1 __vector $res]  != 0 } {
	echo TEST-ERR(exprEval): \$vec1 = \$chain1.i<\$chain1.j> failed.
	return 1; 
    }
     
    exprEval "$vec1 = $chain1.i<$vecJ>"
    if { [verifyResult $vec1 __vector $res]  != 0 } {
	echo TEST-ERR(exprEval): $vec1 = $chain1.i<$vecJ> failed.
	return 1; 
    }
     
    exprEval "$vec1 = $chain1.i<$chain1.j+1-1>"
    if { [verifyResult $vec1 __vector $res]  != 0 } {
	echo TEST-ERR(exprEval): \$vec1 = \$chain1.i<\$chain1.j+1-1> failed.
	return 1; 
    }

    exprEval "$vec1 = $chain1.i<$vecJ-$chain1.j>"
    set res ""
    loop i 0 $size {
	set h [chainElementGetByPos $chain1 $i]
	set res "$res [exprGet $h.i<0>]"
    }
    if { [verifyResult $vec1 __vector $res]  != 0 } {
	echo TEST-ERR(exprEval): \$vec1 = \$chain1.i<\$vecJ-\$chain1.j> failed.
	return 1; 
    }

    exprEval "$vec1 = $chain1.i<$chain1.i<$chain1.i<$chain1.j>>>"
    set res ""
    loop i 0 $size {
	set h [chainElementGetByPos $chain1 $i]
	set res "$res [exprGet $h.i<[exprGet $h.i<[exprGet $h.i<[exprGet $h.j]>]>]>]"
    }
    if { [verifyResult $vec1 __vector $res]  != 0 } {
	echo TEST-ERR(exprEval): \$vec1 = \$chain1.i<\$chain1.i<\$chain1.i<$chain1.j>>> failed.
    	return 1; 
    }

    cleanChains $expression
    chainPurge $chain1 genericDel

    chainDel $chain1
    vectorExprDel $vec1
    vectorExprDel $vecJ
    return 0
}

# List of test :

proc all_test {} {
    set res 0
    set res [expr $res || [shortTest]]
    set res [expr $res || [sXtest]]
    set res [expr $res || [vecTest]]
    set res [expr $res || [sssToVecTest]]
    set res [expr $res || [sssToVecTest2]]
    set res [expr $res || [sssToVecTest3]]
    set res [expr $res || [sssToVecTest4]]
    set res [expr $res || [simplePtTest]]
    set res [expr $res || [tblColTest]]
    set res [expr $res || [tblColTest_Quote]]
    set res [expr $res || [stringTest]]
    set res [expr $res || [bad_test]]
    set res [expr $res || [MemCpyTest]]
    set res [expr $res || [MemCpyTest2]]
    set res [expr $res || [logical_test]]
    set res [expr $res || [bitwise_test]]
    set res [expr $res || [break_test]]
    set res [expr $res || [index_test]]
    return $res
}


# Execute the test:

set result 0
set result [all_test]


# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_SEval.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_SEval.tcl"
    return 1
}

return $result
#exit $result
