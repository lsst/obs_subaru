###################################################################
#
# Test the different utilities to access handles content
#
#   o exprPrint (not tested since it is output is put on stdout?)
#   o exprGet
#   o handleSet
#   o handleSetFromHandle
#
# CONCERN: Use the contrib tcl function typedef and sizeof.
#          uses shMalloc and shFree
#          uses maskNew, handlePtr, handleBind
#
###################################################################

set ARRAY_SIZE 3


#++
# ROUTINE:		tstErr
#
#	Report an error.  The severity can be "I" (informational), "W" (warning)
#	and "E" (error).  If the severity is an error, the test is exited.  An
#	invalid severity is considered to be an error.
#
# RETURN VALUES:	None
#
# CONCERNS:		None
#
# AUTHOR:  Tom Nicinski, Creation date: 14-Sep-1995
#--

set tstErrSev(I) {INFO}
set tstErrSev(W) {WARN}
set tstErrSev(E) {ERR}
set tstErrSev(F) {ERR}				;# Fatal causes immediate exit

proc tstErr {severity msg args} {
   global tstErrSev
   global tstErrCnt

   if {[lsearch -exact [array names tstErrSev] $severity] == -1} { set severity {F} }
   set  lclErrSev $tstErrSev($severity)
   puts stderr "TEST-$lclErrSev: $msg"
   foreach lclArg $args {
      puts stderr "TEST-$lclErrSev: $lclArg"
   }
#   if {$severity == {F}} { exit 1 }		;# Fatal causes immediate exit
   incr tstErrCnt($severity)
}

proc tstErrReset {{severity {}}} {
   global tstErrSev
   global tstErrCnt

   if {$severity == {}} { set severity [array names tstErrSev] }

   foreach lclErrSev $severity { set tstErrCnt($lclErrSev) 0 }
}

proc tstErrExit {severity} {
   global tstErrCnt

   if {$tstErrCnt($severity) > 0} { exit 1 }
}

# Compare The value of v1 and v2
# If one of the two values is non-numeric it returns
# a typical equality test.
# If both values are numeric it returns true if both
# are equal within epsilon % !!
# (i.e ((v2-v1)/v1) < $epsilon )
set epsilon 0.01
proc areEqual {v1 v2} {
    global epsilon
    # to get rid of eventual extraneous { }
    if { [llength $v1]==1 } { set v1 [lindex $v1 0] }
    if { [llength $v2]==1 } { set v2 [lindex $v2 0] }
    if { [catch {expr $v1}] || [catch {expr $v2}] } {
	# non numeric value
	if { $v1 == $v2 } { 
	    return 1 
	} else { 
	    return 0 
	}
    }
    if { $v1 == 0 } {
	if { $v2 == 0 } {
	    return 1
	} else {
	    set v3 $v1
	    set v1 $v2
	    set v2 $v3
	}
    }
    # $v1 and $v2 are numeric and $v1 is NOT zero.
    if {abs(($v2-$v1)/$v1) < $epsilon} { 
	return 1 
    } else { 
	return 0 
    }
}

#*******************************************************************************

#*******************************************************************************
# ROUTINE:		testObjectCreate
#
#	return an handle to a bogus object.
#
# RETURN VALUES:	handle name of the new TEST structure.
#
# SIDE EFFECT:          define the BOGUS schema type
#
# CONCERN:              use the global varibale ARRAY_SIZE.
#
#    We need the test stucture (BOGUS) to have all its 
#    member of primitive types at the beginning,
#    followed by enums and then by type involving

proc testObjectCreate {} {
    global ARRAY_SIZE
    global Fields
    global FieldsNumber 
    global PrimitiveFields

    set type BOGUS
# in the following we use ".." to eval ARRAY_SIZE
    if { ![string match {* BOGUS *} [typesList]] } {
	if [ catch { typedef struct bogus "        
	   char       *name;
           char        Label<80>;
           int         nrow;
           short       index;
           float       grade;
           PIXDATATYPE pixType;
           MASK       *mask;     
           HDR         hdr;
           MASK        all_mask<$ARRAY_SIZE>;
        " $type } lclErr] {
	    tstErr F "typedef: unable to create the BOGUS schema type" $lclErr
	}
    }
    if [catch {set handle [shMalloc [ sizeof $type]]} lclErr] {
	tstErr F "shMalloc: unable to allocate memory forthe $type schema type" $lclErr
    }	
    if [catch {handleBind $handle [handlePtr $handle] $type} lclErr] {
	tstErr F "handleBind: unable to bind the memory to the handle" $lclErr
    }
    set Fields(1,1) name
    set Fields(1,2) $handle
    set Fields(1,3) "char*"
    set Fields(2,1) Label
    set Fields(2,2) "This is a Label"
    set Fields(3,2) "char<80>"    
    set Fields(3,1) nrow    
    set Fields(3,2) 10     
    set Fields(3,3) int    
    set Fields(4,1) index    
    set Fields(4,2) -50    
    set Fields(4,3) short    
    set Fields(5,1) grade     
    set Fields(5,2) 343.23
    set Fields(5,3) float
    set Fields(6,1) pixType
    set Fields(6,2) TYPE_U16
    set Fields(6,3) PIXDATATYPE

    set PrimitiveFields 6

    set Fields(7,1) mask
    set Fields(7.2) "0x0"
    set Fields(7,3) "MASK*"
    set Fields(8,1) hdr
    set Fields(8,2) "..."
    set Fields(8,3) HDR
    set Fields(9,1) all_mask
    set Fields(9,2) "..."
    set Fields(9,3) "all_mask<$ARRAY_SIZE>"
    set FieldsNumber 9

    return $handle
}


#*******************************************************************************
# ROUTINE:		testObjectFill
#
#       Fill the TEST structure with known values.
#       This function test handleSet and also the simpliest form
#       of exprGet (exprGet of a primitive or enum value).
#
# RETURN VALUES:	handle name of the new TEST structure.
#

proc testObjectFill { obj } {
    global ARRAY_SIZE
    global Fields
    global FieldsNumber 
    global PrimitiveFields
    global maskHandle

    for { set i 1 } { $i <= $PrimitiveFields } { incr i } {
	if [catch {handleSet $obj.$Fields($i,1) $Fields($i,2)} lclErr] { 
	    tstErr E "handleSet: unable to set a $Fields($i,3) with a string ($obj.$Fields($i,1))" $lclErr
	}
    }

    set maskHandle [maskNew -name struct_mask 25 25]
    if [catch {handleSet $obj.mask [handlePtr $maskHandle]} lclErr] { 
	tstErr E "handleSet: unable to set a MASK* ($obj.mask)" $lclErr}
    if [catch {handleSet $obj.hdr.modCnt    100} lclErr] { 
	tstErr E "handleSet: unable to set an unsigned  ($obj.hdr.modCnt)" $lclErr}
    if [catch {handleSet $obj.hdr.hdrVec    0} lclErr] { 
	tstErr E "handleSet: unable to set a HDR_VEC* ($obj.hdr.hdrVec)" $lclErr}
    for { set i 0 } { $i < $ARRAY_SIZE } { incr i } {
	if [catch {handleSet $obj.all_mask<$i>.name all_mask_$i} lclErr] { 
	    tstErr E "handleSet: unable to set a char* with a string ($obj.all_mask<$i>.name)" $lclErr}
	if [catch {handleSet $obj.all_mask<$i>.nrow [expr ($i+1)*10]} lclErr] { 
	    tstErr E "handleSet: unable to set an int ($obj.all_mask<$i>.nrow)" $lclErr}
	if [catch {handleSet $obj.all_mask<$i>.ncol [expr $i+1]} lclErr] { 
	    tstErr E "handleSet: unable to set an int ($obj.all_mask<$i>.ncol)" $lclErr}
    }
    return $obj
}

# list of created thing:
# createTestObject: one handle
# fillTestObject: (automatically) malloc space for the name fields
#                 

#*******************************************************************************
# ROUTINE:		testObjectFree
#
#       Free all memory allocated for the test object.
#       We assume that it has been allocated by testObjectCreate and
#       filled by testObjectFill
#
# RETURN VALUES:	handle name of the new TEST structure.
#

proc testObjectFree { obj } {
    global ARRAY_SIZE
    for { set i 0 } { $i < $ARRAY_SIZE } { incr i } {
	if [catch {set name [exprGet -nostring $obj.all_mask<$i>.name]} lclErr] { 
	tstErr E "exprGet: unable to get the value of the address of $obj.all_mask<$i>.name" $lclErr}
	if [catch {shFree [handleBindNew $name PTR]} lclErr] { 
	tstErr E "shFree: unable to free memory for $obj.all_mask<$i>.name" $lclErr}
    }
    if [catch {set maskPtr [exprGet $obj.mask]} lclErr] { 
	tstErr E "exprGet: unable to get the value of the address of $obj.mask" $lclErr}
    if [catch {set maskHdl [handleNameGet $maskPtr]} lclErr] { 
	tstErr E "handleNameGet: Could not get any handle associated with $obj.mask" $lclErr}
    if [catch {maskDel $maskHdl} lclErr] { 
	tstErr E "maskDel: " $lclErr}

    if [catch {set name [exprGet -nostring $obj.name]} lclErr] { 
	tstErr E "exprGet: unable to get the value of the address of $obj.name" $lclErr}
    if [catch {shFree [handleBindNew $name PTR]} lclErr] { 
	tstErr E "shFree: unable to free memory for $obj.name" $lclErr}
    
    # these following is needed because we override
    # the handle created by shMalloc!
    if [catch {shFree [handleBindNew [handlePtr $obj] PTR]} lclErr] { 
	tstErr E "shFree: unable to free memory for $obj"}
    if [catch {handleDel $obj} lclErr] { 
	tstErr E "handleDel: " $lclErr}
}

proc testValue { listValue obj field value } {
    if [catch {set result [keylget listValue $field] } lclErr] {
	tstErr E "exprGet: Bad construction of the list" $lclErr 
    } else {
	if { $result != $value } {
	    tstErr E "handleSet/exprGet: Bad value for $obj.$field $result instead of $value" $lclErr 
	}
    }
}    

proc testPtr { listValue obj field } {
    if [catch {set result [keylget listValue $field] } lclErr] {
	tstErr E "exprGet: Bad construction of the list" $lclErr 
    } else {
	if { ([llength $result] != 1) || ( [string range $result 0 1] != "0x") } {
	    tstErr E "handleSet/exprGet: Bad value ($result) for pointer $obj.all_mask" $lclErr 
	}
    }
}


proc testExprGet { obj } {
    global ARRAY_SIZE
    global Fields
    global FieldsNumber 
    global PrimitiveFields
    global maskHandle

# test exprGet $obj
    if [catch {set res [exprGet $obj]} lclErr] { 
	tstErr E "exprGet: unable to get $obj" $lclErr 
	return 1
    } 
    # loop on all primitive type except the enums.
    for { set i 1 } { $i < $PrimitiveFields } { incr i } {
	testValue $res $obj $Fields($i,1) $Fields($i,2)
    }
    testValue $res $obj pixType "(enum) $Fields($PrimitiveFields,2)"
    testValue $res $obj mask [handlePtr $maskHandle]
    testValue $res $obj hdr "..."
    testPtr   $res $obj all_mask

# test exprGet -recurse -nostring $obj
    if [catch {set res [exprGet -recurse -nostring $obj]} lclErr] { 
	tstErr E "exprGet: unable to get $obj with -recurse -nostring " $lclErr 
	return 1
    }
    testPtr   $res $obj name
    testPtr   $res $obj Label
    # loop on all primitive type except the enums and the strings
    for { set i 3 } { $i < $PrimitiveFields } { incr i } {
	testValue $res $obj $Fields($i,1) $Fields($i,2)
    }
    testValue $res $obj pixType "(enum) $Fields($PrimitiveFields,2)"
    testValue $res $obj mask [handlePtr $maskHandle]
    testValue $res $obj hdr "{modCnt 100} {hdrVec 0x0}"
    testPtr   $res $obj all_mask

# test exprGet -header -unfold -enum $obj
    if [catch {set res [exprGet -header -unfold -enum $obj]} lclErr] { 
	tstErr E "exprGet: unable to get $obj with -header -unfold -enum " $lclErr 
	return 1
    }
    testValue $res $obj type BOGUS
    for { set i 1 } { $i <= $PrimitiveFields } { incr i } {
	testValue $res $obj $Fields($i,1) $Fields($i,2)
    }
    testValue $res $obj mask [handlePtr $maskHandle]
    testValue $res $obj hdr "..."
    set array [keylget res all_mask]
    for { set i 0 } { $i < $ARRAY_SIZE } { incr i } {
	testValue $array $obj.all_mask <$i> "..."
    }
    
# test exprGet -header -unfold -enum -recurse $obj
    if [catch {set res [exprGet -header -unfold -enum -recurse $obj]} lclErr] { 
	tstErr E "exprGet: unable to get $obj with -header -unfold -enum -recurse " $lclErr
	return 1
    } 
    testValue $res $obj type BOGUS
    for { set i 1 } { $i <= $PrimitiveFields } { incr i } {
	testValue $res $obj $Fields($i,1) $Fields($i,2)
    }
    testValue $res $obj mask [handlePtr $maskHandle]
    testValue $res $obj hdr "{modCnt 100} {hdrVec 0x0}"
    set array [keylget res all_mask]
    for { set i 0 } { $i < $ARRAY_SIZE } { incr i } {
	set elem [keylget array <$i>]
	testValue $elem $obj.all_mask name all_mask_$i
	testValue $elem $obj.all_mask nrow [expr ($i+1)*10]
	testValue $elem $obj.all_mask ncol [expr $i+1]
    }

# test exprGet $obj.Label
    if [catch {set res [lindex [exprGet $obj.Label] 0]} lclErr] { 
	tstErr E "exprGet: unable to get $obj.Label" $lclErr 
	return 1
    } 
    if { $res != $Fields(2,2) } {
	tstErr E "handleSet/exprGet: Bad value ($res) obtained by exprGet $obj.Label (expected: $Fields(2,2))" $lclErr 
    }

# test exprGet -nostring $obj.Label
    if [catch {set res [exprGet -nostring $obj.Label]} lclErr] { 
	tstErr E "exprGet: unable to get $obj.Label with  -nostring" $lclErr 
	return 1
    } 
    if { ([llength $res] != 1) || ( [string range $res 0 1] != "0x") } {
	tstErr E "handleSet/exprGet: Bad value ($res) for pointer $obj.Label (exprGet -nostring $obj.Label)" $lclErr 
    }
     
# test exprGet -header $obj.Label
    if [catch {set res [exprGet -header $obj.Label]} lclErr] { 
	tstErr E "exprGet: unable to get $obj.Label with -header" $lclErr 
	return 1
    } 
    testValue $res $obj type STR
    if { [lindex $res 1] != $Fields(2,2) } {
	tstErr E "handleSet/exprGet: Bad value ($res) obtained by exprGet $obj.Label (expected: $Fields(2,2))" $lclErr 
    }

# test exprGet -header -nostring $obj.Label
    if [catch {set res [exprGet -header -nostring $obj.Label]} lclErr] { 
	tstErr E "exprGet: unable to get $obj.Label with -header -nostring " $lclErr 
	return 1
    } 
    testValue $res $obj type STR
    set res [lindex $res 1]
    if { ([llength $res] != 1) || ( [string range $res 0 1] != "0x") } {
	tstErr E "handleSet/exprGet: Bad value ($res) for pointer $obj.Label (exprGet -header -nostring $obj.Label)" $lclErr 
    }
    
    for { set i 1 } { $i <= $PrimitiveFields } { incr i } {
	if [catch {set res [lindex [exprGet -enum $obj.$Fields($i,1)] 0]} lclErr] { 
	    tstErr E "exprGet: unable to get $obj.nrow" $lclErr 
	}
	if { $res != $Fields($i,2) } {
	    tstErr E "handleSet/exprGet: Bad value for $obj.$Fields($i,1) $res instead of $Fields($i,2)" $lclErr 
	}
    }
    
    for { set i 1 } { $i <= $PrimitiveFields } { incr i } {
	if [catch {set res [lindex [exprGet -enum -header $obj.$Fields($i,1)] 1]} lclErr] { 
	    tstErr E "exprGet: unable to get $obj.nrow" $lclErr 
	}
	if { $res != $Fields($i,2) } {
	    tstErr E "handleSet/exprGet: Bad value for $obj.$Fields($i,1) $res instead of $Fields($i,2)" $lclErr 
	}
    }

}

proc testExprPrint { obj } {
    if [catch {exprPrint $obj} lclErr] { 
	tstErr E "exprPrint: unable to print $obj" $lclErr 
    }
    if [catch {exprPrint $obj -recurse -nostring} lclErr] { 
	tstErr E "exprPrint: unable to print $obj" $lclErr 
    }
    if [catch {exprPrint $obj -header -unfold -enum } lclErr] { 
	tstErr E "exprPrint: unable to print $obj" $lclErr 
    }
    if [catch {exprPrint -header -unfold -enum -recurse $obj} lclErr] { 
	tstErr E "exprPrint: unable to print $obj with -header -unfold -enum -recurse " $lclErr 
    } 
    if [catch {exprPrint $obj.Label} lclErr] { 
	tstErr E "exprPrint: unable to print $obj.Label" $lclErr 
    } 
    if [catch {exprPrint -header $obj.Label} lclErr] { 
	tstErr E "exprPrint: unable to print $obj.Label with -header" $lclErr 
    } 
    if [catch {exprPrint -nostring $obj.Label} lclErr] { 
	tstErr E "exprPrint: unable to print $obj.Label with  -nostring" $lclErr 
    } 
    if [catch {exprPrint -header -nostring $obj.Label} lclErr] { 
	tstErr E "exprPrint: unable to print $obj.Label with -header -nostring " $lclErr 
    } 
}


proc testSchemaGetFull { obj } {

    # We can not test the last parameters since they change
    # from one architecture to another ...

    set SIZE 10
    set value(0) {BOGUS STRUCT 196 9}
    set value(1) {name char 1 NULL 0}
    set value(2) {Label char 0 80 4}
    set value(3) {nrow int 0 NULL 84}
    set value(4) {index short 0 NULL 88}
    set value(5) {grade float 0 NULL 92}
    set value(6) {pixType PIXDATATYPE 0 NULL 96}
    set value(7) {mask MASK 1 NULL 100}
    set value(8) {hdr HDR 0 NULL 104}
    set value(9) {all_mask MASK 0 3 112}

    # first see if schemaGetFull and schemaGetFullFromType agree
    if [catch {set res1 [schemaGetFull $obj]} lclErr] {
	tstErr E "schemaGetFull: unable to get $res1" $lclErr
	return 1
    }
    if [catch {set res2 [schemaGetFullFromType [lindex [handleGet $obj] 1]]} lclErr] {
	tstErr E "schemaGetFullFromType: unable to get $res2" $lclErr
	return 1
    }
    if { $res1 != $res2 } {
	tstErr E "schemaGetFull doesn't match schemaGetFullFromType"
	return 1
    }

    set elem [lindex $res1 0]
    for { set j 0 } { $j < 2 } { incr j } {
	set val [lindex $elem $j]
	if { $val != [lindex $value(0) $j]} {
	    tstErr E "schemaGetFull: Wrong value: $val instead of $value($j)" $lclErr
	}
    }
    # now check individual lists from schemaGetFull against our master list
    for { set i 1 } { $i < $SIZE } { incr i } {
	#get each list
	set elem [lindex $res1 $i]
	for { set j 0 } { $j < 3 } { incr j } {
	    set val [lindex $elem $j]
	    if { $val != [lindex $value($i) $j]} {
		tstErr E "schemaGetFull: Wrong value: $val instead of $value($i)" $lclErr
	    }
	}
    }
}


proc testSchemaGet { obj } {

    set SIZE 9
    set value(0) {name char*}
    set value(1) {Label {char[80]}}
    set value(2) {nrow int}
    set value(3) {index short}
    set value(4) {grade float}
    set value(5) {pixType PIXDATATYPE}
    set value(6) {mask MASK*}
    set value(7) {hdr HDR}
    set value(8) {all_mask {MASK[3]}}

    # first see if schemaGet and schemaGetFromType agree
    if [catch {set res1 [schemaGet $obj]} lclErr] {
	tstErr E "schemaGet: unable to get $res1" $lclErr
	return 1
    }
    if [catch {set res2 [schemaGetFromType [lindex [handleGet $obj] 1]]} lclErr] {
	tstErr E "schemaGetFromType: unable to get $res2" $lclErr
	return 1
    }
    if { $res1 != $res2 } {
	tstErr E "schemaGet doesn't match schemaGetFromType"
	return 1
    }

    # now check individual lists from schemaGet against our master list
    for { set i 0 } { $i < $SIZE } { incr i } {
	#get each list
	set val [lindex $res1 $i]
	if { $val != $value($i) } {
	    tstErr E "schemaGet: Wrong value: $val instead of $value($i)" 
	}
    }
}


proc testSchemaPrint { obj } {
    if [catch {schemaPrint $obj} lclErr] {
	tstErr E "schemaPrint: unable to print $obj" $lclErr 
	return 1
    } 
    if [catch {schemaPrintFromType [lindex [handleGet $obj] 1]} lclErr] {
	tstErr E "schemaPrintFromType: unable to print $obj" $lclErr 
	return 1
    } 
}



proc testSchemaKindGet { obj } {
    set SIZE 9
    set values(0) PRIM
    set values(1) PRIM
    set values(2) PRIM
    set values(3) PRIM
    set values(4) PRIM
    set values(5) ENUM
    set values(6) PRIM
    set values(7) STRUCT
    set values(8) PRIM

    if [catch {set res1 [schemaKindGet $obj]} lclErr] {
	tstErr E "schemaKindGet: unable to set $res1" $lclErr 
	return 1
    } 
    if [catch {set res2 [schemaKindGetFromType [lindex [handleGet $obj] 1]]} lclErr] {
	tstErr E "schemaKindGetFromType: unable to set $res2" $lclErr 
	return 1
    }
    if {$res1 != $res2} {
	tstErr E "schemaKindGet:$res1 does not match schemaKindGetFromType:$res2"
	return 1
    }
    set vals [schemaGet $obj]
    for { set i 0 } { $i < $SIZE } {incr i} {
	set val [lindex $vals $i]
	set val [lindex $val 0]
	set kind [schemaKindGet $obj.$val]
	if { $kind != $values($i) } {
	    tstErr E "schemaKindGet: Wrong value: $kind instead of $value($i) for $val"
	}
    }
}



proc testArray { } {

    set SIZE 10
    set value(0) 10.0
    set value(1) 30.0
    set value(2) 20 
    set value(3) -903.32
    set value(4) 20e-32
    set value(5) 234
    set value(6) 234.34
    set value(7) 994.9
    set value(8) 0.03
    set value(9) 23.3
    for { set i 0 } { $i < $SIZE } {incr i} {
	lappend values $value($i)
    }
    set values "{ $values }"

    if { ![string match {* ARRAYTEST *} [typesList]] } {
	typedef struct test { float f[10]; } ARRAYTEST
    }

    set arr [testNew ARRAYTEST]
    if [catch {handleSet $arr $values } lclErr] { 
	tstErr E "handleSet: unable to set $arr as a struct" $lclErr 
	return 1
    } 
    if [catch {set res [exprGet -unfold $arr]} lclErr] { 
	tstErr E "exprGet: unable to get $arr" $lclErr 
	return 1
    }
    if [catch {set res [keylget res f]} lclErr] {
	tstErr E "exprGet: unvalid keyed list ! $res" $lclErr
	return 1
    }
    for { set i 0 } { $i < $SIZE } {incr i} {
	if [catch {set val [keylget res <$i>]} lclErr] {
	    tstErr E "exprGet: unvalid keyed list ! $res" $lclErr
	}
	if { $val != $value($i) } {
	    tstErr E "exprGet/handleSet: Wrong value for f<$i>: $val instead of $value($i)" $lclErr
	}
    }
    testDel $arr
    
    set arr [testNew ARRAYTEST]
    for { set i 0 } { $i < $SIZE } {incr i} {
	if [catch {handleSet $arr.f<$i> $value($i)} lclErr] {
	    tstErr E "handleSet: unable to set $arr.f<$i> with $value($i)" $lclErr
	}
    }
    for { set i 0 } { $i < $SIZE } {incr i} {
	if [catch {set val [exprGet $arr.f<$i>]} lclErr] {
	    tstErr E "exprGet: unable to get $arr.f<$i>" $lclErr
	}
	if { $val != $value($i) } {
	    tstErr E "exprGet/handleSet: Wrong value for f<$i>: $val instead of $value($i)" $lclErr
	}
    }
    testDel $arr
}

proc testMultiArray { } {
    set SIZE 2
    set MIN -32000
    set MAX  32000
    loop i 0 $SIZE {
	set value($i) [randGet -min $MIN -max $MAX]
    }
    
    if { ![string match {* ARRAYTEST2 *} [typesList]] } {
	eval "typedef struct test { float f<$SIZE>; int g<$SIZE><$SIZE>; int *c; }" ARRAYTEST2
    }

    loop i 0 $SIZE {
	lappend values $value($i)
	lappend intval [int $value($i)]
    }
    loop i 0 $SIZE {
	lappend intvals $intval
    }
#    set intvals [join $intvals]
    set val "{ $values } {  $intvals } 0x0 "

    set arr [testNew ARRAYTEST2]
    if [catch {handleSet $arr $val } lclErr] { 
	tstErr E "handleSet(testMultiArray): unable to set $arr as a struct" $lclErr 
	return 1
    } 
    if [catch {set res [exprGet -unfold $arr]} lclErr] { 
	tstErr E "exprGet(testMultiArray): unable to get $arr" $lclErr 
	return 1
    }
    if [catch {set resf [keylget res f]} lclErr] {
	tstErr E "exprGet(testMultiArray): unvalid keyed list ! $res" $lclErr
	return 1
    }
    loop i 0 $SIZE {
	if [catch {set val [keylget resf <$i>]} lclErr] {
	    tstErr E "exprGet(testMultiArray): unvalid keyed list ! $resf" $lclErr
	}
	if { ! [areEqual $val $value($i)] } {
	    tstErr E "exprGet/handleSet(testMultiArray): Wrong value for f<$i>: $val instead of $value($i)" $lclErr
	}
    }
    if [catch {set resg [keylget res g]} lclErr] {
	tstErr E "exprGet(testMultiArray): unvalid keyed list ! $res" $lclErr
	return 1
    }
    loop i 0 $SIZE {
	if [catch {set val [keylget resg <$i>]} lclErr] {
	    tstErr E "exprGet(testMultiArray): unvalid keyed list ! $resg" $lclErr
	}
	loop j 0 $SIZE {
	    if [catch {set val2 [keylget val <$j>]} lclErr] {
		tstErr E "exprGet(testMultiArray): unvalid keyed list ! $val" $lclErr
	    }
	    if { ! [areEqual $val2 $value($j)] } {
		tstErr E "exprGet/handleSet(testMultiArray): Wrong value for g<$i><$j>: $val instead of $value($j)" $lclErr
	    }
	}
    }
    testDel $arr

    set arr [genericNew ARRAYTEST2]
    loop i 0 $SIZE {
	loop j 0 $SIZE {
	    if [catch {handleSet $arr.g<$i><$j> $value($j)} lclErr] {
		tstErr E "handleSet(testMultiArray): unable to set $arr.f<$i> with $value($i)" $lclErr
	    }
	    if [catch {set val [exprGet $arr.g<$i><$j>]} lclErr] {
		tstErr E "exprGet(testMultiArray): unable to get $arr.g<$i><$j>" $lclErr
	    }
	    if { ! [areEqual $val $value($j)] } {
		tstErr E "exprGet/handleSett(testMultiArray): Wrong value for g<$i>,$j>: $val instead of $value($i)" $lclErr
	    }
	}
    }
    testDel $arr

    set arr [genericNew ARRAYTEST2]
    set array [shMalloc [expr $SIZE*$SIZE*$SIZE*[sizeof int]]]
    if [catch {handleSet $arr.c [handlePtr $array]} lclErr] {
	tstErr E "handleSet(testMultiArray): unable to set $arr.c with [handlePtr $array]" $lclErr
    }
    set cast "INT<$SIZE><$SIZE><$SIZE>"
    set ArrVal ""
    loop i 0 $SIZE {
	set ArrValj ""
	loop j 0 $SIZE {
	    set ArrValk ""
	    loop k 0 $SIZE {
		set value([expr $SIZE*$SIZE*$i+$SIZE*$j+$k]) [int [randGet -min -250 -max 250]]
		if [catch {handleSet (($cast)(*$arr.c))<$i><$j><$k> $value([expr $SIZE*$SIZE*$i+$SIZE*$j+$k])} lclErr] {
		    tstErr E "handleSet(testMultiArray): unable to set $arr.c<$i><$j><$k> with values([expr $SIZE*$SIZE*$i+$SIZE*$j+$k]))" $lclErr
		}
		if [catch {set val [exprGet (($cast)(*$arr.c))<$i><$j><$k>]} lclErr] {
		    tstErr E "exprGet(testMultiArray): unable to get $arr.c<$i><$j><$k>" $lclErr
		}
		if { ! [areEqual $val $value([expr $SIZE*$SIZE*$i+$SIZE*$j+$k])] } {
		    tstErr E "exprGet/handleSett(testMultiArray): Wrong value for c<$i><$j><$k>: $val instead of $value([expr $SIZE*$SIZE*$i+$SIZE*$j+$k])" $lclErr
		}
		lappend ArrValk $value([expr $SIZE*$SIZE*$i+$SIZE*$j+$k])
	    }
	    lappend ArrValj $ArrValk
	}
	lappend ArrVal $ArrValj
    }
    shFree $array
    testDel $arr

    set arr [genericNew ARRAYTEST2]
    set array [shMalloc [expr $SIZE*$SIZE*$SIZE*[sizeof int]]]
    set cast "INT<$SIZE><$SIZE><$SIZE>"
    if [catch {handleSet $arr.c [handlePtr $array]} lclErr] {
	tstErr E "handleSet(testMultiArray): unable to set $arr.c with [handlePtr $array]" $lclErr
    }
    if [catch {handleSet (($cast)(*$arr.c)) $ArrVal} lclErr] {
	tstErr E "handleSet(testMultiArray): unable to set $arr.c with $ArrVal" $lclErr
    }
    if [catch {set vals [exprGet -unfold -flat ($cast)(*$arr.c)]} lclErr] {
	tstErr E "exprGet(testMultiArray): unable to get (char<$SIZE><$SIZE><$SIZE>)(*$arr.c)" $lclErr
    }
    loop i 0 $SIZE {
	loop j 0 $SIZE {
	    loop k 0 $SIZE {
		set val [lindex $vals [expr $SIZE*$SIZE*$i+$SIZE*$j+$k]]
		if { ! [areEqual $val $value([expr $SIZE*$SIZE*$i+$SIZE*$j+$k])] } {
		    tstErr E "exprGet/handleSet(testMultiArray): Wrong value for c<$i><$j><$k>: $val instead of $value([expr $SIZE*$SIZE*$i+$SIZE*$j+$k])" $lclErr
		}
	    }
	}
    }
    shFree $array
    testDel $arr

}

proc testStruct { } {
    set SIZE 17
    set value(0) blabla
    set value(1) 10
    set value(2) 20
    set value(3) TYPE_FL32
    set value(4) 0x30
    set value(5) 0x40
    set value(6) 0x50
    set value(7) 0x60
    set value(8) 0x70
    set value(9) 0x80
    set value(10) 0x90
    set value(11) 0x100
    set value(12) 0x110
    set value(13) 120
    set value(14) 130
    set value(15) {140 150}
    set value(16) 0x140946

    for { set i 0 } { $i < $SIZE } {incr i} {
	lappend values $value($i)
    }

    set value(15) {{modCnt 140} {hdrVec 0x96}}

    set reg [regNew]
    set save [testNew REGION]
    handleSetFromHandle $save $reg
    
    if [catch {handleSet $reg $values } lclErr] { 
	tstErr E "handleSet: unable to set $reg as a struct: $values" $lclErr 
	return 1
    } 
    if [catch {set res [exprGet -recurse -enum $reg]} lclErr] { 
	tstErr E "exprGet: unable to get $reg" $lclErr 
	return 1
    }
    for { set i 0 } { $i < $SIZE } { incr i } {
	#get each field
	set val [lindex $res $i]
	#get the name of the field
	set name [lindex $val 0]
	#get the value out of the field
	set val [lindex $val 1]
	if { $val != $value($i) } {
	    tstErr E "exprGet/handleSet: Wrong value for $reg.$name : $val instead of $value($i)" $lclErr
	}
    }

    handleSetFromHandle $reg $save
    regDel $reg
    testDel $save
}

proc testNew { type } {
    set handle [shMalloc [ sizeof $type]]
    handleBind $handle [handlePtr $handle] $type
    return $handle
}

proc testDel { obj } {
    # these following is needed because we override
    # the handle created by shMalloc!
    if [catch {shFree [handleBindNew [handlePtr $obj] PTR]} lclErr] { 
	tstErr E "shFree: unable to free memory for $obj"
    }
    if [catch {handleDel $obj} lclErr] { 
	tstErr E "handleDel: " $lclErr
    }    
}

proc testCalloc {} {
    set num 100
    set size 20

    if [catch {set h [shCalloc $num $size]} lclErr] {
	# this should not happen. It should have core dumped.
	tstErr E "shCalloc: unable to shCalloc memory"
    }
    set values [exprGet (char<[expr $num*$size]>)$h -flat -nostring -unfold]
    loop i 0 [expr $num*$size] {
	set value [lindex $values $i]
	if {$value != 0} { 
	    tstErr E "shCalloc: element number $i badly initialized ($value)"
	}
    }
    if [catch {shFree $h} lclErr] { 
	tstErr E "shFree: unable to free memory for $h"
    }
}

#*******************************************************************************

#*******************************************************************************
#
# Body of the test.

tstErrReset				;# Reset error counts

set object [testObjectFill [testObjectCreate]]
testExprGet $object
testExprPrint $object
testSchemaGetFull $object
testSchemaGet $object
testSchemaPrint $object
testSchemaKindGet $object
testObjectFree $object

testArray
testStruct
testMultiArray

testCalloc

# Test for any memory leaks
set rtn ""
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_chainutils.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_expr.tcl"
    return 1
}

#*******************************************************************************

tstErrExit E










