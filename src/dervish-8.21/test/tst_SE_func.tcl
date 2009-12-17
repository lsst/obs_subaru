# Values is a list of elements of the form:
# { operator, chain, address_in_handle, {list of values} }
# For example: { +, chain, row, { 10 20 30 40 } }
# If the third field is __vector the second field is then
# considered as a vector.
#
# NB. We dont handle pointer inside structure ... :(
# 
# We define the following functions:
#
# fillChains <Values> 
# updateChains <Values>
# calcResults <Values>
# verifyResult <ch> <field|__vector> <Results>
# createExprEvalScript <ch> <field|__vector> <Values>
#
# len_comp <v1> <v2>	Compare two string on their length
# areEqual <v1> <v2>    return true if v1 and v2 are equal
#                       with $epsilon% precision
# handleSetAndCreate <handle> <value>
#               Set the object pointed to by handle. It 
#               mainly insure that all pointers on the path 
#               to the object are defined (and defines them 
#               if need).
# freeAddrAndDelHandle <addr>
#               Free the allocated space at addr and remove
#               "An" handle pointing to it, if there is one.
# getUsedElementsPointers <handle>
#               Get a list of handle to the pointers that 
#               have been/would be created by 
#               handleSetAndCreate.
# composeHandle <handle> <fields>
#               return $handle.$fields unless $fields 
#               starts with stars. For example if fiels is
#               ***f it returns: ***$handle.f



# fillChains <Values>
#               fill the chains, vector and handle referenced
#               in $Values with the correct values.
#               Use the existing elements if any, otherwise
#               create them with genericNew [chainTypeGet ...]
# CleanChains <Values>
#               Undo all the work done by fillChains.
#               But do not remove the elements.
# chainPurge <chain> <Del_function>
#               remove all elements of the chain and 
#               destroy them with Del_function


# calcResults <Values>
#               Return a list of the expected values in
#               the left side of the expression evaluator
#               assignment
# verifyResults <container> <field> <Results>
#               Compare the results obtained by calcResults to those
#               obtained by exprEval (and stored in the $field field
#               of his element of the chain ch)
#               I.e. we verify the result of:
#                 exprEval "$ch.$field = ....."
#               The field is ignored if container is a simple
#                 handle or a vector.
# createExprEvalScript {<container> <field> <Values>}
#               Execute the expression Evaluator corresponding to the
#               content of ch, field and Values (as described in fillChains)
#               I.e. :
#                        exprEval " $ch.field = ...... "
#               The field is ignored if container is a simple
#                 handle or a vector.

# Limitation of this framework:
#   can not have variable indexes
#   can not do any computation in indexes
#   can not use something like (*...).
#   can not use list (chain, vector, etc. ) of different length

# Variable to define the order of the information in
# a 'field' in our 'programming' language

# Place of the operation and/or string in between operand
set op_pos 0
# Place of the handle contain the Top level object (on which
# we will iterate) (e.g Chain, Vector, Tblcol, or nothing for 
# constant)
set chain_pos 1
# Address within the Top level object.  Typically the field
set addr_pos 2
# List of values taken by the object.
set values_pos 3

# Dummy field name for vector.
set vector_token __vector

# DEBUG flag -- tests will produce
# more output if this variable is set.
set tst_Se_dbg 0

# Compare two string length
# return     for
#   -1       strlen(v1) <  strlen(v2)
#    0       strlen(v1) == strlen(v2)
#   +1       strlen(v1) >  strlen(v2)
# This function is necessary for lsort.
proc len_comp {v1 v2} {
    if {[string length $v1]<[string length $v2]} {
	return -1
    } elseif {[string length $v1]>[string length $v2]} {
	return 1
    } else {
	return 0
    } 
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

# This function Set the object pointed to by handle.
# It mainly insure that all pointers on the path to the
# object are defined (and defines them if need).
#
# RETURN :
#   Success -- the value in ($value)
#   Failure -- only when handleSet handleDel GenericNew 
#              fail annd provoke a Tcl_Error.
#
proc handleSetAndCreate { handle value } {
    set stars ""
    
    if { [string match \** $handle] } {
	regexp "^(\\**)(.*)$" $handle full stars work_handle
    } else {
	set work_handle $handle
    }
    if { [string match *-* $work_handle] } {
	# We have a dereferenced pointer.
	set root ""
	set scanned $work_handle
	
	while { [regexp "^(\[^\\-\]*)->(.*)" $scanned full part1 end] } {
	    set root $root$part1
	    if { [exprGet $root] == 0 } {
		regexp "^(.*)\(\\.|->)(\[^-\.\]*)$" $root full start op field
		set sch [schemaGet $start]
		if { "$sch" == "{ptr*}" } {
		    set sch [schemaGet *$start]
		}
		regexp "(\[^\\*\]*)" [keylget sch $field] full type
		set new [genericNew $type]
		handleSet $root [handlePtr $new]
#		echo "Set $root to [exprGet $root] (expected: [handlePtr $new]"
		handleDel $new
	    }
	    set scanned $end 
	    set root $root->
	}
    }
    if { $stars != "" } {
	# First get the schema type of the referenced object.
	regexp "^(.*)\(\\.|->)(\[^-\.\]*)$" $work_handle full start op field
	set sch [schemaGet $start]
	if { "$sch" == "{ptr*}" } {
	    set sch [schemaGet *$start]
	}
	regexp "(\[^\\*\]*)" [keylget sch $field] full type

	# Allocated space for the pointer of all the layers of indirection
	loop i 0 [expr [string length $stars]-1] {
	    set new [genericNew PTR]
	    handleSet $work_handle [handlePtr $new]
#	    echo "Set $work_handle to [exprGet $work_handle] (expected: [handlePtr $new])"
	    handleDel $new
	    set work_handle *$work_handle
	}

	# Create the actual element.
	set new [genericNew $type]
	handleSet $work_handle [handlePtr $new]
#	echo "Set $work_handle to [exprGet $work_handle] (expected: [handlePtr $new])"
	handleDel $new
    }
    handleSet $handle $value
#    echo "Set $handle to [exprGet $handle] (expected: $value)"
    return $value
};


# Free the memory allocated at $addr.
# It also free a handle associated with it, if there is 
# one.
proc freeAddrAndDelHandle { addr } {
    if { $addr != 0 } {
	set h ""
	catch { set h [handleNameGet $addr]}
	if { $h != "" } { 
	    genericDel $h 
	} else {
	    shFree [handleBindNew $addr PTR]
	}
    }
}


# This function return a list of all pointers that are
# on the path to the end object pointed by $handle.
# eg. getUsedElementsPointers **h1.p->q.r.q
# will returns:
#     *h1.p->q.r.q h1.p->q.r.q h1.p
# Nota Bene:
#     This function only act as a PARSER it has no
#     actual knowledge about the real underlying schema.
proc getUsedElementsPointers { handle } {
    set stars ""
    set list {}
    
    if { [string match \** $handle] } {
	regexp "^(\\**).*$" $handle work_handle stars 
    } else {
	set work_handle $handle
    }
    if { $stars != "" } {
	# We have stars in front of the handle.
	loop i 0 [expr [string length $stars]-0] {
	    regexp "^\\*(.*)$" $work_handle full work_handle
	    set list [concat $list $work_handle]
	}
    }
    if { [string match *-* $work_handle] } {
	# We have a dereferenced pointer.
	set root ""
	set scanned $work_handle
	while { [regexp "^(.*)->\[^-\]*$" $scanned full root] } {
	    set list [concat $list $root]
	    set scanned $root
	}
    }
    return $list
}

# return $handle.$fields unless $fields starts with stars.
# For example if fiels is ***f it returns: ***$handle.f
proc composeHandle { handle fields } {
    if { [regexp "^(\\**)(.*)$" $fields full stars field] } {
	return $stars$handle.$field
    } else {
	return $handle.$fields
    }
}


proc fillChains { Values } {
    global op_pos addr_pos chain_pos values_pos vector_token
    set ch_hd(0,0) {}
 
    set nb_elem [llength [lindex [lindex $Values 0] $values_pos]]
    loop i 0 [llength $Values] {
	set val [lindex $Values $i]
	set ch [lindex $val $chain_pos]
	set hdlType ""
	if { $ch != "" }  { 
	    catch { set hdlType [handleType $ch] }
	}
	if { $hdlType == "VECTOR" } {
	    vectorExprSet $ch "\{ [lindex $val $values_pos] \}"
	} elseif { $hdlType == "CHAIN" } {
	    set Type [chainTypeGet $ch]
	    set Type [keylget Type type]
	    loop j 0 $nb_elem {
		set elem ""
		catch { set elem [chainElementGetByPos $ch $j] }
		#		if { $New } 
		if { $elem == "" } {
		    if { [catch {set x $ch_hd($ch,$j)}]  } then {
			set ch_hd($ch,$j) [genericNew $Type]
			chainElementAddByPos $ch $ch_hd($ch,$j)
		    }
		    handleSetAndCreate [composeHandle $ch_hd($ch,$j) [lindex $val $addr_pos]] [lindex [lindex $val $values_pos] $j]	    
		} else {
#		    set elem [chainElementGetByPos $ch $j]
		    handleSetAndCreate [composeHandle $elem [lindex $val $addr_pos]] [lindex [lindex $val $values_pos] $j]	    
		}		    
	    }
	} elseif { $hdlType == "TBLCOL" } {
	    # Do nothing for now
	} elseif { $hdlType != "UNKNOWN" && $hdlType != "" } {
#	    echo "This is the handle type --$hdlType--"
	    handleSetAndCreate [composeHandle $ch [lindex $val $addr_pos]] [lindex $val $values_pos]	    
	}
    }
}

#proc fillChains { Values } {
#    __fillChains $Values 1
#}

#proc updateChains { Values } {
#    __fillChains $Values 0
#}


# Remove all the space that have been allocated by fillChains
proc cleanChains { Values } {
    global op_pos addr_pos chain_pos values_pos vector_token
    set ch_hd(0,0) {}
    set star_list {}
    set list {}
 
    set nb_elem [llength [lindex [lindex $Values 0] $values_pos]]
    loop i 0 [llength $Values] {
	set val [lindex $Values $i]
	set ch [lindex $val $chain_pos]
	set hdlType ""
	if { $ch != "" }  { 
	    catch { set hdlType [handleType $ch] }
	}
	if { $hdlType == "VECTOR" } {
	    # Do nothing ... exterior vectorExprDel needed
	} elseif { $hdlType == "CHAIN" } {
	    loop j 0 $nb_elem {
		set hd [chainElementGetByPos $ch $j]
		set list [concat $list [getUsedElementsPointers [composeHandle $hd [lindex $val $addr_pos]]]]
	    }
	} elseif { $hdlType == "TBLCOL" } {
	    # Do nothing for now
	} elseif { $hdlType != "UNKNOWN" && $hdlType != "" } {
#	    echo "This is the handle type --$hdlType--"
	    set list [concat $list [getUsedElementsPointers [composeHandle $ch [lindex $val $addr_pos]]]]
	}
    }
    # Put all the handle beginning by a star at the end of
    # the list.
    set list [lsort -decreasing $list]
    # Separate the two list.
    set start_star [lsearch -regexp $list "\\*"]
    set star_list [lrange $list $start_star end]
    set list [lrange $list 0 [expr $start_star-1]]

    # First delete the handle with stars
    loop i 0 [llength $star_list] {
	set h [lindex $star_list $i]
	    freeAddrAndDelHandle [exprGet $h]
	    handleSet $h 0x0
    }
    # Then all the others in order! 
    set list [lsort -decreasing -command len_comp $list]
    loop i 0 [llength $list] {
	set h [lindex $list $i]
	    freeAddrAndDelHandle [exprGet $h]
	    handleSet $h 0x0
    }    
}

proc chainPurge { chain DelFunction } {
    loop i 0 [exprGet $chain.nElements] {
	$DelFunction [chainElementRemByPos $chain HEAD]
    }
}

proc produceInt { n min max } {
    set res "" ; 
    loop i 0 $n { 
	set res "$res [int [randGet -min $min -max $max]]"; 
    }
    return $res
}

proc produceFloat  { n min max } {
    set res "" ; 
    loop i 0 $n { 
	set res "$res [randGet -min $min -max $max]"; 
    }
    return $res
}

# Using the same list as fillChains
# Calculate and return the list of result expected 
# from exprEval.

proc calcResults { Values } {
    global op_pos addr_pos chain_pos values_pos
    set res_list {}

    set nb_elem [llength [lindex [lindex $Values 0] $values_pos]]
    loop j 0 $nb_elem {
	set res {}
	loop i 0 [llength $Values] {
	    set val [lindex $Values $i]
	    set operation [lindex $val $op_pos]
	    set values [lindex $val $values_pos]
	    if { [llength $values] == 1 } {
		set index 0
	    } else {
		set index $j
	    };
	    if { $operation != "" } {
		set res "$res [lindex $val $op_pos] [lindex $values $index]"
	    } else {
		set res [lindex [lindex $val $values_pos] $index]
	    }
	}

	set res_list [concat $res_list [expr $res]]
    }
    return $res_list
}

# Compare the results obtained by calcResults to those
# obtained by exprEval (and stored in the $field field
# of his element of the chain ch)
# I.e. we verify the result of:
#  exprEval "$ch.$field = ....."

proc verifyResult { ch field Results } {
    global vector_token
    
    if { $field == $vector_token } {
	loop i 0 [exprGet $ch.dimen] {
	    if { ![areEqual [exprGet $ch.vec<$i>] [lindex $Results $i]] } {
		echo "Mismatch at $ch.vec<$i>, found  [exprGet $ch.vec<$i>] instead of [lindex $Results $i]"
		return 1
	    }
	}
    } elseif { [handleType $ch] == "CHAIN" } {
	loop i 0 [llength $Results] {
	    if { ![areEqual [exprGet [chainElementGetByPos $ch $i].$field] [lindex $Results $i]] } {
		echo "Mismatch at $ch ($i).$field, found  [exprGet [chainElementGetByPos $ch $i].$field] instead of [lindex $Results $i]"
		return 1
	    }
	}
    } elseif { [handleType $ch] == "TBLCOL" } {
	set indexes ""
	regexp "^(\[^<\[\]*)(.*)$" $field full f end
	while { $end != "" } {
	    if { [regexp "^\[<\[\](\[^>\]*)\[>\](.*)" $end full ind end] } { 
		set indexes [concat $ind $indexes]
	    }
	}
	loop i 0 [llength $Results] {
	    set expected [lindex $Results $i]
	    set got [tblFldGet $ch -field $f "$i $indexes" ]
	    if { ![areEqual $got $expected] } {
		echo "Mismatch at $field index $i in tblcol $ch: found $got instead of $expected"
		return 1
	    }
	}
    } else {
	if { ![areEqual [exprGet $ch.$field] [lindex $Results [expr [llength $Results]-1]]] } {
	    echo "Mismatch at $ch ($i).$field, found  [exprGet [chainElementGetByPos $ch $i].$field] instead of [lindex $Results $i]"
	    return 1
	}
    }

    return 0
}


# Execute the expression Evaluator corresponding to the
# content of ch, field and Values (as described in fillChains)
# I.e. :
#          exprEval " $ch.field = ...... "
# test_expression: { simple_expressions }
# simple_expressions: simple_expressions simple_expressions
# simple_expression: { dest_handle dest_field { Operations } }
# Operations: Operation Operations
# Operations: { string src_handle src_field { ValueList } }

# and the equivalent expression Evaluator syntax would be:

# exprEval "dest_handle.dest = string src_handle.src_field 
#                              string src_handle.src_field 
#                              ......;
#           dest_handle.dest = string src_handle.src_field 
#                              string src_handle.src_field 
#                              ......; "


proc createExprEvalScript { Expressions } {
    global op_pos addr_pos chain_pos values_pos vector_token tst_Se_dbg

    set script ""
    loop k 0 [llength $Expressions] {
	set simple_expression [lindex $Expressions $k]
	set ch [lindex $simple_expression 0]
	set field [lindex $simple_expression 1]
	set Values [lindex $simple_expression 2]

	if {$tst_Se_dbg != 0} { echo $Values [lindex $simple_expression 2]}

	set left_type [handleType $ch]
	if { $left_type == "CHAIN" } {
	    set script "$script $ch.$field = "
	} elseif { $left_type == "TBLCOL" } {
	    set indexes ""
	    regexp "^(\[^<\[\]*)(.*)$" $field full f end
	    set script "$script $ch.\"$f\"$end = "
	} else {
	    set script "$script $ch = "
	}
	
	loop i 0 [llength $Values] {
	    set val [lindex $Values $i]
	    
	    set operation [lindex $val $op_pos]
	    set ch [lindex $val $chain_pos]
	    if { $ch != "" } {
		set right_field [lindex $val $addr_pos]
		set right_type [handleType $ch]
		
		#	    if { $right_type == "CHAIN" || $right_type == "TBLCOL" } 
		if { $right_field != $vector_token && $right_field != "" } {
		    if { $right_type != "TBLCOL" } {
			set right_operand "$ch.$right_field"
		    } else {
			set indexes ""
			regexp "^(\[^<\[\]*)(.*)$" $right_field full f end
			set right_operand "$ch.\"$f\"$end"
		    }
		} else {
		    set right_operand $ch
		}
	    } else {
		set right_operand [lindex $val $values_pos]
	    }
	    if { $operation != "" } {
		set script "$script [lindex $val $op_pos] $right_operand "
	    } else {
		set script "$script $right_operand "
	    }
	}
    }

    if { $tst_Se_dbg != 0 } { echo $script }
    return $script
#    return [exprEval $script]
#    echo [exprEval $script -debug 1]
#    return "NOTHING"
}





