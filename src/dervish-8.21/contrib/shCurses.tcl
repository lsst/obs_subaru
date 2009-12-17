# Auxiliary routines to make use of curses routine, screenEdit
#
# Format of screenEdit is:
#	screenEdit message beginIndex arrayName ncolName arrayVal ncolVal
#where message is a 1-line message, beginIndex is the starting cursor pos,
# arrayName is a tcl array with a set of names, indexed starting at 0
# (index 0 is a field label; index 1 is the start of the real data)
# arrayVal is another tcl array with a set of values for each name, same
# indexing, ncolName is the number of columns used to display arrayName,
# arrayVal is the number of columns to display arrayVal.  beginIndex is
# used to position the cursor on startup.  You can
# edit the Val fields (use arrow keys, <Ctrl-K>, <CR>, <Del>).
# The update values are copied back to the arrayVal array.
# Simple, huh?
#
# Procedures defined here:
#
# structsEdit <handle>		Edit the structure pointed to by the handle.

##############################################################################
# structsEdit does recursive translation like structPrint
#

proc structsEdit {hndl {outline ""}} {
#Complex handle expressions with dereferenced pointers mess up the following.
#Embed in parentheses to cure
   if {$outline != ""} then {upvar 1 $outline outlist}
   set arrayName(0) $hndl
   set arraySchema(0) STRUCT
   set arrayVal(0) [handleType $hndl]
   set begin 1
   while {1} {
	set nelem 0
	recursEdit "($hndl)" arrayName arraySchema arrayVal nelem
	set message(0) ""
	set response [screenEdit "" $begin arrayName -40 arrayVal 80]
	set command [lindex $response 0]
	set line [lindex $response 1]
	set begin $line
	set outlist \
	   [list $arrayName($line) $arraySchema($line) $arrayVal($line)]
#Update output values if CTRL-X was used to exit
	if {$command == "X"} {
	   for {set i 1} {$i <= $nelem} {incr i} {
	      if {$arraySchema($i) == "PRIM"} then {
		catch {handleSet $arrayName($i) $arrayVal($i)}
		}
	      if {$arraySchema($i) == "ENUM"} then {
		catch {handleSet $arrayName($i) $arrayVal($i)}
		}
	      if {$arraySchema($i) == "ARRAY"} then {
		set handle [lindex $arrayName($i) 0]
		set arg [lindex $arrayName($i) 1]
                catch {handleSet $handle.$arg $arrayVal($i)}
		}
#	      if {$arrayName($i) == "PTR"}
	      }
#Redisplay after a CTRL-X
	   continue
	   }
#Follow a link
	if {$command == "L"} then {
	   set name [lindex $outlist 0]
	   set schema [lindex $outlist 1]
	   set val [lindex $outlist 2]
	   if {$schema != "PTR"} then {continue}
#Get the type of the target of the pointer
	   set type [handleType *($name)]
	   if {$type == "LIST*"} then {
		listEdit "*((LIST *)($name))"
		continue
		}
	   if {$type == "CHAIN*"} then {
		chainEdit "*((CHAIN *)($name))"
		continue
		}
#There are lots of weird pointers out there.  Just catch -em.
	   catch {structsEdit "*($name)"}
	   } else break
	}
#For convenience, set response to null if CTRL-Z was used to exit.
   if {$command == "Z"} then {set response ""}
   return $response
   }

##############################################################################
proc recursEdit { hndl inName inSchema inVal inElem} {
   upvar 1 $inName arrayName
   upvar 1 $inSchema arraySchema
   upvar 1 $inVal arrayVal
   upvar 1 $inElem nelem
   set allschema [schemaGet $hndl]
   set test [schemaKindGet $hndl]
   if {$test == "PRIM" || $test == "ENUM"} {
# Handle is a primitive type.
	set arg ""
	elemEdit $hndl $arg arrayName arraySchema arrayVal nelem
	} else {
	foreach f $allschema {
# Handle is a complicated type.
	   set arg [lindex $f 0]
	   elemEdit $hndl $arg arrayName arraySchema arrayVal nelem
	   }
	}
}

##############################################################################
proc elemEdit { hndl arg inName inSchema inVal inElem} {
   upvar 1 $inName arrayName
   upvar 1 $inSchema arraySchema
   upvar 1 $inVal arrayVal
   upvar 1 $inElem nelem
# if arg = "", we have a primitive type
  set allschem [schemaGet $hndl]
  if {$arg == ""} {
	incr nelem
	set arrayName($nelem) $hndl
	set arraySchema($nelem) PRIM
#Pointers have a PTR schema
	if {[handleType $hndl] == "PTR"} then {set arraySchema($nelem) PTR}
	set arrayVal($nelem) [handleShow $hndl]
	return
	}
   set schema [keylget allschem $arg]
# Get the type of the handle.
   set type [handleType $hndl.$arg]
# If input is a pointer, check to see if it is an array
   if { $type == "PTR"} {
	if { [regexp {[][<>]} $schema] } {
# it is an array

# check to see if it is an array of primitives or of structs
	   set type [lindex [split $schema {*[]<>}] 0]
	   set subschema [schemaGetFromType $type]
	   if {[llength $subschema] <= 1} {
#It's a primitive.  Print a bunch of values on 1 line.
		set val [exprGet -unfold -flat -enum $hndl.$arg]
#Trash double quotes that I stick on for character strings.
#		set val [string trim $val \"]
		incr nelem
		set arrayName($nelem) "$hndl $arg"
		set arraySchema($nelem) ARRAY
		set arrayVal($nelem) $val
		return
	   } else {
#It's a complex type.  Print each array element one line at a time.
		arrayEditByElem $hndl $arg arrayName arraySchema arrayVal nelem
		return
		}
	   }
	# is it a known handle?
	set val [handleShow $hndl.$arg]
	if {![catch {handleNameGet $val} hval]} {
	   set val [format "(handle)%s" $hval]
	   }
	incr nelem
#	set arrayName($nelem) "$hndl $arg"
# For simple pointers, don't split handle and argument as separate entities.
	set arrayName($nelem) "$hndl.$arg"
	set arraySchema($nelem) PTR
	set arrayVal($nelem) $val
	return
	}
   recursEdit $hndl.$arg arrayName arraySchema arrayVal nelem
}
############################################################################
# New version of arrayEdit - print 1 line per array element regardless.
#
proc arrayEditByElem {hndl arg inarrayName inarraySchema inarrayVal innelem} {
   upvar 1 $inarrayName arrayName
   upvar 1 $inarraySchema arraySchema
   upvar 1 $inarrayVal arrayVal
   upvar 1 $innelem nelem
   set allschem [schemaGet $hndl]
   set schema [keylget allschem $arg]
   set sch [split $schema {[]<>}]
   set type [lindex $sch 0]
   set dimen {}
   for {set i 1} {$i < [llength $sch]} { set i [expr $i+2] } {
	lappend dimen [lindex $sch $i]
	}
# Is it a scalar after all?  More than 2 dimensions?
   if { [llength $dimen] == 0 || [llength $dimen] > 2} then {
	elemTest $hndl.$arg ""
	}
   if { [llength $dimen] == 1 } {
	for {set i 0} {$i < [lindex $dimen 0]} { incr i } {
	   recursEdit $hndl.$arg<$i> arrayName arraySchema arrayVal nelem
	   }
	}
   if { [llength $dimen] == 2 } {
	for {set i 0} {$i < [lindex $dimen 0]} { incr i } {
	   for {set j 0} {$j < [lindex $dimen 1]} { incr j } {
		recursEdit $hndl.$arg<$i><$j> arrayName arraySchema arrayVal nelem
		}
	   }
	}
   }

##############################################################################
#
#
proc chainPage {chainHandle} {
#Complex handle expressions with dereferenced pointers mess up the following.
#Embed in parentheses to cure
   if {[handleType $chainHandle] != "CHAIN"} then {error "Not a CHAIN"}
   set cursor [chainCursorNew $chainHandle]
   if {[chainSize $chainHandle] == 0} then return
   chainCursorSet $chainHandle $cursor
   set hndl [chainWalk $chainHandle $cursor THIS]
   while {$hndl != ""} {
	set response [structsEdit $hndl outline]
#Always delete handle - else they pile up like rabbits.  This is a weakness,
#alas, of the new chain API
	handleDel $hndl
	if {$response == ""} then {
	   chainCursorDel $chainHandle $cursor
	   return
	   }
	set command [lvarpop response]
	set line [lvarpop response]
	if {$command == "N"} then {
	   set hndl [chainWalk $chainHandle $cursor]
	   }
	if {$command == "P"} then {
	   set hndl [chainWalk $chainHandle $cursor PREVIOUS]
	   }
	}
   chainCursorDel $chainHandle $cursor
   return
   }


##############################################################################
#
# Edit a chain, one line per object.
#
proc chainEdit {listHandle {field ""}} {

   if {[handleType $listHandle] != "CHAIN"} then {error "Not a CHAIN"}
   if {[chainSize $listHandle] == 0} then return
#field can have several items.  Concatenate all but the last one into
#field1, let the last one be field2.  This is done so we can use arrayShow
#to extract a field value.
   set refresh 1
   set beginline 1
   while {1} {
	if {$refresh == 1} {
	   set cursor [chainCursorNew $listHandle]
	   chainCursorSet $listHandle $cursor
	   set hndl [chainWalk $listHandle $cursor THIS]
	   if {$hndl == ""} then {return}
	   set arrayName(0) $listHandle
	   set class [handleType $hndl]
# Don't mess with blank fields
	   if {$field == ""} {
		puts stdout "Select Attribute from next display" nonewline
		flush stdout
		sleep 1
		schemaEdit $class field
		}
	   if {$field == ""} return
	   set field [split $field .]
	   set flen [llength $field]
	   set field1 ""
	   if {$flen > 1} {
		for {set i 1} {$i < $flen} {incr i} {
		   append field1 . [lindex $field [expr $i-1]]
		   }
		}
	   set field2 [lindex $field [expr $flen-1]]
	   set arrayVal(0) $class.$field
	   set index 0
#Create a new handle for each item on the list
	   while {$hndl != ""} {
		incr index
		set handleSave($index) [handleNew]
		handleBindFromHandle $handleSave($index) $hndl
		set arrayName($index) $handleSave($index)
		set err [catch {set arrayVal($index) \
			[exprGet -unfold -flat -enum $hndl$field1.$field2]}]
#		set err [catch {set arrayVal($index) \
#		   [string trim [arrayShow $hndl$field1 $field2] \"]}]
		if {$err == 1} then {
		   set arrayVal($index) \
"Error: Tried to display $hndl$field1.$field2 in [handleType $hndl]"
		   }
		handleDel $hndl
		if {[catch {set hndl [chainWalk $listHandle $cursor]}]} then {
		   break
		   }
		}
	   chainCursorDel $listHandle $cursor
	   set nelem $index
	   }
	set refresh 0
#No field is editable
	set response [screenEdit "" $beginline arrayName -40 arrayVal -80]
	if {$response == ""} then break
#nelem is number of elements excluding index 0, which is a header
	set nelem [expr {[array size arrayName] - 1}]
#This is a simple link to follow.  All names are Link#'s
	set command [lindex $response 0]
	set line [lindex $response 1]
	set beginline $line
	if {$command == "Z"} then break
	if {$command == "L"} then {
	   structsEdit $arrayName($line)
	   }

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Edit the list
	if {$command == "X"} then {
	   }
	}
   for {set i 1} {$i <= $nelem} {incr i} {
	handleDel $handleSave($i)
	}
   return $response
   }

##############################################################################

proc schemaEdit {schemaElem {inline ""}} {
   if {$inline != ""} then {
	upvar 1 $inline outline
	}
   if {[llength $schemaElem] == 1} then {
	set arg ""
	set type $schemaElem
   } else {
	set arg [lindex $schemaElem 0]
	set type [lindex $schemaElem 1]
	}
   set outline ""
   if {[catch {set schemaList [schemaArrayGetFromType $type]}]} {
	echo "Type is invalid or too complex"
	flush stdout
	sleep 1
	return ""
	}
   set len [llength $schemaList]
   set arrayName(0) $arg
   set arrayVal(0) $type
   for {set i 1} {$i <= $len} {incr i} {
	if {$len == 0} break
	set next [lvarpop schemaList]
	if {[llength $next] == 0} then {
	   set arrayName($i) $arg
	   set arrayVal($i) ""
	   continue
	   }
	if {[llength $next] == 1} then {
	   set arrayName($i) $arg
	   set arrayVal($i) [lindex $next 0]
	   continue
	   }
	set nextarg [lindex $next 0]
	if {[string index $nextarg 0] == "<" || $arg == ""} then {
	   set arrayName($i) $arg$nextarg
	} else {
	   set arrayName($i) $arg.$nextarg
	   }
	set arrayVal($i) [lindex $next 1]
	}
   set line 1
   while {1} {
	set msg(0) "<CTRL_X> to make a selection" 
	set response [screenEdit msg $line arrayName -40 arrayVal -40]
	set command [lindex $response 0]
	set line [lindex $response 1]
	if {$command == "N"} then {
	   set response [schemaEdit "$arrayName($line) $arrayVal($line)" \
		outline]
	   set command [lindex $response 0]
	   }
	if {$command == "X"} then {
	   set outline $arrayName($line)
	   set command "A"
	   break
	   }
# Control A: We are unwinding the stack and saving the result.  Build up
# the full argument name
	if {$command == "A"} then {
#	   set outline $arrayName($line).$outline
	   break
	   }
	if {$command == "R"} continue
	if {$command == "P"} then {
	   set command "R"
	   break
	   }
	if {$command == "Z"} then break
	}
   return "$command $line"
   }

############################################################################
# Enhance version of schemaGetFromType.  If the input type is of the form of
# an array, all elements of the array are returned.  Otherwise, function
# just like schemaGetFromType.  schemaElem can be of the form "name type"
# or just "type".  The former is needed to parse arrays because we want to
# return a list of the form {name<0> int} {name<1> int} ...
#
proc schemaArrayGetFromType {schema} {
   set sch [split $schema {[]<>}]
   set type [lindex $sch 0]
   set dimen {}
   for {set i 1} {$i < [llength $sch]} { set i [expr $i+2] } {
      lappend dimen [lindex $sch $i]
   }
# Is it a scalar after all?  More than 2 dimensions?
   if { [llength $dimen] == 0 || [llength $dimen] > 2} then {
        return [schemaGetFromType $schema]
        }
#Is the dimension really a number? If not, it is likely a Versant class
#specifier
   if { [llength $dimen] == 1 && [ctype digit [lindex $dimen 0]] == 0} {
        return [schemaGetFromType $schema]
        }

   set ans ""
   if { [llength $dimen] == 1 } {
      for {set i 0} {$i < [lindex $dimen 0]} { incr i } {
	lappend ans "<$i> $type"
	 }
      }
   if { [llength $dimen] == 2 } {
      for {set i 0} {$i < [lindex $dimen 0]} { incr i } {
         for {set j 0} {$j < [lindex $dimen 1]} { incr j } {
	    lappend ans "<$i><$j> $type"
            }
         }
      }
   return $ans
}

