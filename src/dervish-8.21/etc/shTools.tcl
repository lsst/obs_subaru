# Steve Kent and al. additions/modifications to dervishStartup.tcl
#
# handleShow		Show the value of what a handle points to.
##############################################################################


# Translate a handle and return the result.  Primitive types are printed
# explicitly.  Complex types return the handle itself.
# With the new (1/95) exprGet handleShow only does the following:
# return either an handle expression if this is a complexe type
# or a value if this is a primitive type.
# Also replace non printable characters in strings. 
#

# smallHandleShow is the same as hadnelShow without the char replacement
proc smallHandleShow {hndl} {
    set test [schemaKindGet $hndl]
    if {$test == "PRIM"} {
	return [expr [exprGet -enum $hndl] ]
    }
    if {$test == "ENUM"} {
	return [expr [exprGet -enum $hndl] ]
    }	
    return $hndl
}


proc handleShow {hndl} {
    set test [schemaKindGet $hndl]
    if {$test == "PRIM"} {

	# Handle is a primitive type.

        set type [lindex [schemaGet $hndl] 0]

	# lreplace function gets rid of the leading type returned by membersGet
	#	set val [lindex [lreplace [membersGet $hndl] 0 0] 0]
	# the new (1/95) exprGet by default does not return an header so

        set val [lindex [exprGet $hndl] 0]
        set val [string trim $val { }]

	if {$type != "char"} {
	    return $val
	}
	# character.  Return just 1 character.  Substitute non-printing charactesr
	# with a .
	if {[string length $val] == 3} then {
	    set val [string range $val 1 1]
	    return $val
	} else {
	    set val ""
	    return $val
	}
    }
    if {$test == "ENUM"} {
	
	#handle is an enumerated type

        return [exprGet -enum $hndl]
    }

    # Handle is a complicated or unknown type.

    return $hndl
}


ftclHelpDefine shHg afFromL \
"
Convert a dervish list into an AF.

USAGE:  afFromL list {err null} {name \"none\"}
  list -- the list to convert
  err  -- the optional list of errors to use
  name -- what to name the af, defaults to \"none\"

RETURN:
A handle to a new af
"
proc afFromL {list {err null} {name "none"}} {
    set n [llength $list]
    set af [afNew]
    afDefine $af -name $name  -nValue $n
    loop i 0 $n {
	if {$err!="null"} {
	    set error [lindex  $err $i]
	} else {
	    set error 0.0
	}
        afSet $af $i [lindex $list $i] -error $error
    }
    return $af
}


ftclHelpDefine shVector vFromL \
"
Convert a dervish list into an VECTOR.

USAGE:  vFromL list {err null} {name \"none\"}
  list -- the list to convert
  name -- what to name the vector, defaults to \"none\"

RETURN:
A handle to a new vector
"
proc vFromL {list {name "none"} } {
    set vec [vectorExprEval "{$list}"]
    vNameSet $vec $name
    return $vec
}

ftclHelpDefine shTools genericChainDestroy \
"
Delete everything on a chain and the chain

USAGE:  genericChainDestroy chain destructor
where
  chain is the chain to be destroyed
  destructor is the specific destructor to use on each element;
    genericDel is the default

RETURN:
  0 if it worked
  1 otherwise
"

proc genericChainDestroy {chain {destructor genericDel}} {
    set cursor [chainCursorNew $chain]
    while {[set o [chainWalk $chain $cursor]] != ""} {
	chainElementRemByCursor $chain $cursor
	$destructor $o
    }
    chainCursorDel $chain $cursor
    chainDel $chain
    return 0
}

alias chainDestroy "genericChainDestroy"
# JTA: These names are too long for their usefulness
alias ceg chainElementGetByPos
alias cea chainElementAddByPos


ftclHelpDefine shVector chainNewFromChain \
"
Make a new Chain of type of the input chain

USAGE:  chainNewFromChain chain {n 0} 
  chain -- the chain to get the type from
  n -- optionally, how many members on the new chain

RETURN:
A handle to a new chain
"
proc chainNewFromChain {chain {n 0} } {
	set type [chainTypeGet $chain]
 	set type [keylget type type]
   	set new [chainNew $type $n]
	return $new
}

ftclHelpDefine shTools exprFmt \
"
Format print of an element of a vector

USAGE:  exprFmt vector i {fmt \"%2.2f\"}
  vector -- the vector
  i -- the i'th element, to be printed
  fmt -- optionally, define a format

RETURN:
A formated string
"
proc exprFmt {vec i {fmt "%2.2f"}} {
        return [format $fmt [exprGet $vec.vec<$i>] ]
}


ftclHelpDefine shTools chainSearchOne \
"
Search a chain.  
Throw an error if not exactly one match found, else return the single matching element.


USAGE: chainSearchOne chain predicate
  chain -- the chain
  predicate -- search predicate

RETURN:
The one matching element off of the chain.
"
proc chainSearchOne { chain predicate} {

    if {[catch {chainSearch $chain $predicate} result]} {
		error "chainSearchOne: $result"
    }
    if {[chainSize $result] != 1} {
		chainDel $result
		error "chainSearchOne: search did not return exactly one element"
    }
    set elem [chainElementGetByPos $result HEAD]
    chainDel $result
    return $elem
}
