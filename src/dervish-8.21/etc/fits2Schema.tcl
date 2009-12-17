#This file contains support code for converting Fits ascii or binary tables
#to chains and back automatically.
#
#Procedures defined here:
#	fits2Schema file type {hdr NULL}  <--- optional hdr set if not NULL
#	schema2Fits chain file {hdr NULL}  <--- optional header written to file
#	schemaRecurseGetFromType type
#	recursTrans inresult arg type nstar dim offset  (private routine)
#	schemaRecursePrintFromType type
#	slowLinkNext iterator
#	linkPurge iterin
#	linkSelect list arg op value
#	schemaTransAdd handle arg elemName type (private)
#	schemaTransAuto type {direction write} {argList ""}
#	tblPrint tbl
#	tblArrayGet tbl col
#	fldPrint tbl col
#	tblFldScaleInfoGet tbl col
#	tblFix tbl
#	tblDataGet tbl col index
#	tblDataPrint tbl index
#	typeNewFromTbl tbl schemaName
#	mergeTypeNew type1 type2 outtype (private)
#	chainMatchType chain1 chain2  predicate
#	typeSizeOf
#	typePad
#	typedef
#	makeio
#	makeioSub
#       schemaGetFull
###############################################################################

proc schemaGetFull { handle } {
    return [schemaGetFullFromType [handleType $handle]]
}

set fits2SchemaArgs {
    {fits2Schema "Read in a FITS table file and create the appropriate objects.\n" }
    {<file> STRING "" file "FITS file to be read"}
    {<type> STRING "" type \
	 "name of schema to create (or NULL to just read header)"}
    {[hdr]  STRING NULL hdr "optional header, set if not NULL"}
    {-hdu   STRING 1 hdu "hdu to read in file"}
    {-sc   STRING "" sc "use this schemaTrans instead of auto-generating"}    
    {-existingType   CONSTANT "" existingType "use an existing type"}
}
ftclHelpDefine shFitsIo fits2Schema [shTclGetHelpInfo fits2Schema $fits2SchemaArgs]

proc fits2Schema {args} {
   upvar #0 fits2SchemaArgs formal_list
   if {[shTclParseArg $args $formal_list fits2Schema] == 0} {
       return
   }

   if {($type == "" || $type == "NULL") && $hdr == "NULL"} {
      error "You mustn't omit both type and header"
   }

   set tbl [tblColNew]
   if [catch { fitsRead $tbl $file -hdu $hdu } msg] {
      tblColDel $tbl
      return -code error -errorinfo $msg $msg
   }

   if {$type == "" || $type == "NULL"} {# we just wanted a header
      hdrCopy $tbl.hdr $hdr
      handleDelFromType $tbl

      return $hdr
   }

   if {![info exists existingType]} {
        typeNewFromTbl $tbl $type
   }

   if {$sc == ""} {
        set xtbl [schemaTransAuto $type write]
   } else {
        set xtbl $sc
   }

   set chain [chainNew $type]
#If number of objects is large, tblToSchema will bomb due to mallocing
#temporary storage that overflows memory (even though it is actually not used).
#So let's loop through 1000 objects at a time
   set list [tblInfoGet $tbl]
   set objCnt [keylget list ROWCNT]
   set nblock [int [expr ($objCnt-1)/1000+1]]
   loop i 0 $nblock {
	set startrow [expr $i*1000]
	set stoprow [min $objCnt [expr ($i+1)*1000]]
	tblToSchema $tbl $chain $xtbl -schemaName $type -row $startrow \
		-stopRow $stoprow
	}
   if {$hdr != "NULL"} {
       hdrCopy $tbl.hdr $hdr
   }
   handleDelFromType $tbl
   schemaTransDel $xtbl
   return $chain
}

set schema2FitsArgs {
    {schema2Fits "Convert a chain of objects to a FITS file.\n"}
    {<chain> STRING "" chain "Handle to chain of objects"}
    {<file>  STRING "" file "FITS file name to be written"}
    {[hdr]   STRING NULL hdr "optional header, set if not NULL"}
    {-pdu_hdr STRING NULL pdu_hdr "Header for PDU"}
    {-append CONSTANT -append opts "append to existing file"}
}

ftclHelpDefine shFitsIo schema2Fits [shTclGetHelpInfo schema2Fits $schema2FitsArgs]

proc schema2Fits {args} {

   upvar #0 schema2FitsArgs formal_list
   if {[shTclParseArg $args $formal_list schema2Fits] == 0} {
       return
   }
   if {![ftclPassed $args $formal_list -append] && $pdu_hdr == "NULL"} {
       set opts "-pdu minimal"
   }

   set type [chainTypeDefine $chain]
   set xtbl [schemaTransAuto $type]
   set tbl [schemaToTbl $chain $xtbl -schemaName $type]

   if {$pdu_hdr != "NULL"} {
      set phdr [hdrNew]
      hdrCopy $pdu_hdr $phdr
      if [catch { hdrGetLine $phdr SIMPLE }] {
	 hdrInsertLine $phdr 1 [hdrMakeLineWithLogical SIMPLE 1]
      }
      if [catch { hdrGetLine $phdr BITPIX }] {
	 hdrInsertLine $phdr 2 [hdrMakeLineWithInt "BITPIX" 8]
      }
      if [catch { hdrGetLine $phdr NAXIS }] {
	 hdrInsertLine $phdr 3 [hdrMakeLineWithInt "NAXIS" 0]
      } 
      if [catch { hdrGetLine $phdr EXTEND }] {
	 hdrInsertLine $phdr 4 [hdrMakeLineWithLogical EXTEND 1]
      }
      if [catch { hdrGetLine $phdr END }] {
	 hdrInsertLine $phdr [expr [hdrGetLineTotal $phdr]+1] "END"
      }
      
      hdrWriteAsFits $phdr $file
      append opts " -append"

      hdrDel $phdr
   }

   if {$hdr != "NULL"} {
      hdrCopy $hdr $tbl.hdr
   }   
   if {[catch "eval fitsWrite $tbl $file -binary $opts"]} then {
	eval fitsWrite $tbl $file -binary -standard FALSE $opts
   }
   tblColDel $tbl
   schemaTransDel $xtbl
}

#############################################################################

#schemaGet routines
##############################################################################
#
proc schemaRecurseGetFromType {type} {
   set result ""
   recursTrans result "" $type 0 NULL ""
   return $result
   }

#######################################################################

proc recursTrans {inresult arg type nstar dim offset} {
upvar 1 $inresult result
   set test [schemaKindGetFromType $type]
   if {$test == "PRIM" || $test == "ENUM" || $nstar != 0} {
# type is primitive.
	lappend result [list $arg $type $nstar $dim $offset]
	return
	}
# type is complex.  Is it an array?  If so, expand each array element.
   if {$dim != "NULL"} then {
	set ndim [llength $dim]
	set dimtot 1
	for {set i [expr $ndim-1]} {$i >= 0} {set i [expr $i-1]} {
	   set dim1($i) $dimtot
	   set dim2($i) [lindex $dim $i]
	   set dimtot [expr $dimtot*$dim2($i)]
	   }
	loop j 0 $dimtot {
	   set subarg $arg
	   for {set i 0} {$i < $ndim} {incr i} {
		set subdim [fmod [floor [expr $j/$dim1($i)]] $dim2($i)]
		append subarg <$subdim>
		}
	   recursTrans result $subarg $type $nstar NULL $offset
	   }
	return
	}	   
#Complex inline type.	
   set allschema [schemaGetFullFromType $type]
   lvarpop allschema
   foreach f $allschema {
	set subarg [lindex $f 0]
	set subtype [lindex $f 1]
	set substar [lindex $f 2]
	set subdim [lindex $f 3]
	set suboff [lindex $f 4]
	recursTrans result "$arg $subarg" $subtype $substar $subdim $suboff
	}
   }

proc schemaRecursePrintFromType {type} {
   set biglist [schemaRecurseGetFromType $type]
   set nelem [llength $biglist]
   for {set i 0} {$i < $nelem} {incr i} {
	set list [lindex $biglist $i]
	set arglist [lindex $list 0]
	set type [lindex $list 1]
	set nstar [lindex $list 2]
	set dim [lindex $list 3]
	set offset [lindex $list 4]
	set narg [llength $arglist]
	set arg [lindex $arglist 0]
	set j 1
	while {$j < $narg} {
	   set arg $arg.[lindex $arglist $j]
	   incr j
	   }
	set j 0
	while {$j < $nstar} {
	   append type *
	   incr j
	   }
	if {$dim != "NULL"} then {
	   for {set j 0} {$j < [llength $dim]} {incr j} {
		set type $type\[[lindex $dim $j]\]
		}
	   }
	echo [format "%-30s%s" $arg $type]
	}
   }


#################################################################
#schemaTransAuto routines
#Create and fill a translation table automatically, given a type name

#First, a private function
proc schemaTransAdd {handle arg elemName type} {
   set narg [llength $elemName]
#Simple argument name?
   if {$narg == 1} then {
#echo	eval schemaTransEntryAdd $handle name $arg $elemName $type
	eval schemaTransEntryAdd $handle name $arg $elemName $type
	return
	}
#Compound name
   schemaTransEntryAdd $handle name $arg [lindex $elemName 0] struct
   set j 1
   while {$j < [expr $narg-1]} {
#echo	eval schemaTransEntryAdd $handle cont [lindex $elemName [expr $j-1]] \
	   [lindex $elemName $j] struct
	eval schemaTransEntryAdd $handle cont [lindex $elemName [expr $j-1]] \
	   [lindex $elemName $j] struct
	incr j
	}
#echo   eval schemaTransEntryAdd $handle cont [lindex $elemName [expr $narg-2]] \
	[lindex $elemName [expr $narg-1]] $type
   eval schemaTransEntryAdd $handle cont [lindex $elemName [expr $narg-2]] \
	[lindex $elemName [expr $narg-1]] $type
   }

#And now, the main routine
#direction = read for going from table to schema
#	     write for going from schema to table
#argList is null for translating the entire definition; otherwise it is
#a tcl list of all arguments to be included in the table

proc schemaTransAuto {type {direction write} {argList ""}} {
   set schList [schemaRecurseGetFromType $type]
#   lvarpop schList
   set handle [schemaTransNew]
   while {1} {
	if {$schList == ""} break
	set elemList [lvarpop schList]
	set elemName [lindex $elemList 0]
#Compress elemName:
        set narg [llength $elemName]
        set arg [lindex $elemName 0]
        set hdrArg $arg
        for {set i 1} {$i < $narg} {incr i} {
           set arg $arg.[lindex $elemName $i]
           set hdrArg [lindex $elemName $i]
           }
#For arrays, we need a compressed handle name with last
#arg not present
        set handleArg $handle
        for {set i 0} {$i < [expr $narg-1]} {incr i} {
           set handleArg $handleArg.[lindex $elemName $i]
           }

	set elemType [lindex $elemList 1]
	set elemIdir [lindex $elemList 2]
	set elemDim  [lindex $elemList 3]
	if {$elemDim != "NULL"} then {
	   set ndim [llength $elemDim]
	} else {
	   set ndim 0
	   }
	if {$argList != ""} then {
	   if {[lsearch $argList $arg] == -1} then continue
	   }
	if {$elemType == "TYPE"} then continue
	if {$elemType == "char" && $elemIdir == 1} then {
	   if {$direction == "write"} then {
		set heaptype "string"
		set heaplen "strlen($arg)"
		} else {
		set heaptype "string"
		set heaplen "1"
		}
	   schemaTransAdd $handle $arg $elemName "heap -heaptype $heaptype \
		-heaplength $heaplen"
	   continue
	   }
#Add 1-d character arrays as strings; otherwise, as a character array
	if {$elemType == "char" && $elemIdir == 0 && $ndim == 1} then {
	   schemaTransAdd $handle $arg $elemName string
	   continue
	   }
	if {$elemIdir != 0} then continue
	if {[schemaKindGetFromType $elemType] == "PRIM"} then {
	   schemaTransAdd $handle $arg $elemName \"$elemType\"
	   continue
	   }
	}
   return $handle
   }
#########################################################################
#TBLCOL support utilities
proc tblPrint {tbl} {
   set list [tblInfoGet $tbl]
   set rowcnt [keylget list ROWCNT]
   set nfield [keylget list FLDCNT]
   echo Number of rows = $rowcnt
   echo [format "%-3s %-15s%-15s%-15s" Col "Data type" "Dimensions" "Parameter"]
   for {set i 0} {$i < $nfield} {incr i} {
	set list [tblFldInfoGet $tbl -col $i]
	set type [keylget list TYPE]
	set dim [keylget list DIM]
	set dim [lrange $dim 1 end]
	set ttype [keylget list TTYPE]
#	set tunit [keylget list TUNIT]
	echo [format "%3d  %-15s%-15s%-15s" $i $type $dim $ttype]
	}
   return
   }

# {TYPE FLOAT} {DIM {72917 10}} {NSTAR 0} {EMPTY 0} {TTYPE {Photomet}} {TUNIT {MJy
# /sr}}

proc tblArrayGet {tbl col} {
   linkInit $tbl.fld iter
   for {set i 0} {$i <= $col} {incr i} {
	set handle [slowLinkNext iter]
	}
   return $handle
   }

proc fldPrint {tbl col} {
   set handle [tblArrayGet $tbl $col]
   exprPrint -header -enum -recurse $handle
   exprPrint -header -enum -recurse "*((TBLFLD *)($handle.info))"
   handleDel $handle
   }

proc tblFldScaleInfoGet {tbl col} {
   set handle [tblArrayGet $tbl $col]
   set TZERO [handleShow "((TBLFLD *)($handle.info))->TZERO"]
   set TSCALE [handleShow "((TBLFLD *)($handle.info))->TSCAL"]
   set list "\{TZERO $TZERO\} \{TSCALE $TSCALE\}"
   handleDel $handle
   return $list
   }

proc tblFix {tbl} {
   linkInit $tbl.fld iter
   while {1} {
	set handle [slowLinkNext iter]
	if {$handle == ""} then break
#	arraySet "(*((TBLFLD *)($handle.info)))" TTYPE \
#	   [string trim [string toupper \
#	   [arrayShow "(*((TBLFLD *)($handle.info)))" TTYPE]] \" ]
        handleSet "(*((TBLFLD *)($handle.info))).TTYPE" \
                  [expr \ 
                  [string toupper [exprGet (*((TBLFLD *)($handle.info))).TTYPE"]]] 
	if {[handleShow $handle.data.schemaType] == "STR" && [handleShow \
	   $handle.dimCnt] > 2} then {handleSet $handle.data.schemaType CHAR}        
	}
   }

proc tblDataGet {tbl col index} {
   set handle [tblArrayGet $tbl $col]
   set nrow [handleShow $tbl.rowCnt]
   if {$index >= $nrow} then {error "Index too big (perhaps)"}
   set type [handleShow $handle.data.schemaType]
   if {$type != "STR"} then {
	   set data [handleShow "(($type *)($handle.data.dataPtr))<$index>"]
   } else {
	   set data [handleShow "(($type *)($handle.arrayPtr))<$index>"]
	   }
   handleDel $handle
   return $data
   }

proc tblDataPrint {tbl index} {
   set nrow [handleShow $tbl.rowCnt]
   if {$index >= $nrow} then {error "Index too big (perhaps)"}
   linkInit $tbl.fld iter
   set col 0
   while {1} {
	set handle [slowLinkNext iter]
	if {$handle == ""} then break
	set type [handleShow $handle.data.schemaType]
	set list [tblFldInfoGet $tbl -col $col]
	set ttype [keylget list TTYPE]
	if {$type != "STR"} then {
#	   exprPrint -header -enum -recurse "(($type *)($handle.data.dataPtr))<$index>"
	   echo [format "%-20s %s" $ttype [handleShow \
		"(($type *)($handle.data.dataPtr))<$index>"]]
	} else {
#	   exprPrint -header -enum -recurse "(($type *)($handle.arrayPtr))<$index>"
	   echo [format "%-20s %s" $ttype [handleShow \
		"(($type *)($handle.arrayPtr))<$index>"]]
	   }
	if {$type == "TBLHEAPDSC"} then {
		set heapdsc [handleBindFromHandle [handleNew] \
			"(($type *)($handle.data.dataPtr))<$index>"]
		set info [tblFldInfoGet $tbl -col $col]
		set heaptype [keylget info HEAPTYPE]
		if {$heaptype == "STR"} then {
		   echo [handleShow "((char *)($heapdsc.ptr))"]
		} else {
		   for {set k 0} {$k < [handleShow $heapdsc.cnt]} {incr k} {
		      echo [handleShow "(($heaptype *)($heapdsc.ptr))<$k>"]
		      }
		   }
		handleDel $heapdsc
		}
	incr col
	}
   }

#############################################################################
#Create a dervish type from the description in a TBLCOL structure
#Works only for simple data types.

proc typeNewFromTbl {tbl schemaName} {
#Is schemaname defined already?
   if {[lsearch [typesList] $schemaName] >= 0} then return
   set structDef ""
   set info [tblInfoGet $tbl]
   set nfield [keylget info FLDCNT]
   for {set i 0} {$i < $nfield} {incr i} {
       set info [tblFldInfoGet $tbl -col $i]
       set type [keylget info TYPE]
       set name [keylget info TTYPE]
       set dim  [lrange [keylget info DIM] 1 end]
       if {$type == "TBLHEAPDSC"} then {
	   set type [keylget info HEAPTYPE]
       }
       if {$type == "STR"} then {
	   set type char
       }
#  Leave commented till time when dervish tblcol code generally applies 
#  TZERO and TSCALE.
       #Reset type if scale/zero are set
#       if [catch {set tzero [keylget info TZERO]}] {
#	   set tzero 0
#       }
#       if [catch {set tscale [keylget info TSCALE]}] {
#	   set tscale 1
#       }
#       if {$tzero != 0 || ($tscale != 0 && $tscale != 1)} then {
#	   if {$type == "SHORT"} then {set type "FLOAT"}
#	   if {$type == "INT"} then {set type "DOUBLE"}
#       }
       if {$dim != ""} then {
	   set ndim [llength $dim]
	   for {set j 0} {$j < $ndim} {incr j} {
	       set thedim [lindex $dim $j]
	       set name $name\<$thedim\>
	   }
       }
       set name [string tolower $name]
       set structDef "$structDef $type $name\;"
   }
   eval typedef struct {$structDef} $schemaName
}

##########################################################################
#Create a new merged object type
proc mergeTypeNew {type1 type2 merge} {
	set def "typedef struct {"
	append def "\n   $type1 *object1\;"
	append def "\n   $type2 *object2\;"
	append def "\n   double res1\;"
	append def "\n   double res2\;"
	append def "\n   } $merge"
 	eval $def
	return
	}

##########################################################################
#Match two chains and output a chain with a new merged data type.
proc chainMatchType {chain1 chain2 pred} {
   set type1 [chainTypeDefine $chain1]
   set type2 [chainTypeDefine $chain2]
   set newtype ${type1}${type2}MERGE
   if {[lsearch [typesList] $newtype] == -1} then {
	mergeTypeNew $type1 $type2 $newtype
	}
   set chain [chainMatch $chain1 $chain2 $pred -type $newtype]
   return $chain
   }

##########################################################################
#More easily used interface to schemaDefine facility.  Permit a more C-like
#syntax for defining dervish TYPEs at run-time
#
#example:
#   typedef struct {
#	int i;
#	float a<12>;
#	REGION *reg;
#	MASK mask;
#	} MYTYPE;
#
#Restrictions: Embedded structs like mask above must be predefined.
#Arrays can be specified either with [] or <> notation
#
# Enums should work!
#
#For a given data type, find the maximum size of each embedded primitive.
#C compiler will pad to the largest embedded primitive type!

############################################################################
proc typeSizeOf {type} {
   set schema [schemaGetFullFromType $type]
   set schemaElem [lindex $schema 0]
   set size [lindex $schemaElem 2]
   return $size
   }

############################################################################
proc typepad {type} {
   set schema [schemaRecurseGetFromType $type]
   set max 0
   loop i 0 [llength $schema] {
	set schemaElem [lindex $schema $i]
	set schemaType [lindex $schemaElem 1]
	set nstar [lindex $schemaElem 2]
	if {$nstar > 0} then {set schemaType PTR}
	set max [max $max [typeSizeOf $schemaType]]
	}
   return $max
   }
   
############################################################################
proc typedef {args} {
   set nsch 1
#There are different valid formats for typedef's.
   set len [llength $args]
#2 arguments: example - typedef SECTION RANGE.  This cannot be represented
#in dervish.  The TCL proc makeio parses legal C structures and sends them
#here with possible only 2 args.  Thus we need to recover gracefully.
   if {$len == 2} {
	echo "Bypassing 2-argument input to typedef"
	return
	}
   if {$len != 3 && $len != 4} then {error "Wrong number of arguments"}
   if {$len == 3} then {
	set struct [lindex $args 0]
	set structdef [lindex $args 1]
	set type [lindex $args 2]
	set tag $type
	}
#If 4 args, we have a tag (not needed)
   if {$len == 4} then {
	set struct [lindex $args 0]
	set tag [lindex $args 1]
	set structdef [lindex $args 2]
	set type [lindex $args 3]
	}
   if {$struct != "struct" && $struct != "enum"} then {
	echo "Can only create STRUCTs or ENUMs at present"
	return
	}
# <cr>'s are likely part of the structdef argument.  Purge here
   regsub -all \n $structdef ";" structdef
   regsub -all \t $structdef " " structdef
   regsub -all \;\; $structdef \; structdef
   set pad 0
#Find and zonk all comments
   while {1} {
	if {![regexp -indices (/\[*\]) $structdef c1]} then break
	set first [string range $structdef 0 [expr [lindex $c1 0]-1]]
	set last [string range $structdef [expr [lindex $c1 1]+1] end]
	if {![regexp -indices (\[*\]/) $last c2]} then {error "unmatched /* "}
	set last [string range $last [expr [lindex $c2 1]+1] end]
	set structdef ${first}${last}
	}
   set offset 0
   set nelem 0
#Do enums here
   if {$struct == "enum"} then {
	set value 0
#Split the definition based on commas
	set split [split $structdef ,]
	loop i 0 [llength $split] {
	   set structElem [lindex $split $i]
	   regsub = $structElem " = " structElem
	   if {[llength $structElem] == 0} then continue
	   set name [lindex $structElem 0] 
#Do we have an = sign?
	   if {[lindex $structElem 1] == "="} then {
		set value [lindex $structElem 2]
#Allow for variable substitution
		catch {global $value}
		if {[info exists $value]} then {
		   set value [set $value]}
		}
	   if {[catch {format %d $value}]} then {
		echo Non-numerical value $value used for enum
		}
	   incr nelem
	   set schemaElem($nsch)($nelem) "$name $type $value NULL 0"
	   set $name $value
	   incr value
	   }
	set size [typeSizeOf int]
	set schema($nsch) "$type ENUM $size $nelem"
	schemaDefine schema schemaElem
	return
	}
#Split the definition based on semicolons
   set split [split $structdef \;]
   loop i 0 [llength $split] {
	set structElem [lindex $split $i]
#Save for error message later, want original, not one with substitutions. (efb)
        set localStructElem $structElem
        set localPad 0
	if {[llength $structElem] == 0} then continue
#A complex line might be   unsigned short int ** abc<4>;
#The ** pattern need not have spaces on either side.
#Normalize here
#Delete trailing spaces around a ** pattern
	regsub -all "(\[*]+)\[ ]*" $structElem \\1 structElem
#Insert leading spaces
	regsub -all "(\[^*])\[*]" $structElem "\\1 *" structElem
#Likewise for the dimension information
	regsub -all \[\[\] $structElem < structElem
	regsub -all \[\]\] $structElem > structElem
#Delete all leading space before the open <
	regsub -all "\[ ]*<\[ ]*" $structElem < structElem
	regsub -all "\[ ]*>\[ ]*" $structElem > structElem
#Replace short int with short, long int with long.  Curse these variations..
	regsub -all "short\[ ]+int" $structElem short structElem
	regsub -all "long\[ ]+int" $structElem long structElem
	set elemType [lvarpop structElem]
	if {$elemType == "const"} then {set elemType [lvarpop structElem]}
	if {$elemType == "unsigned"} then {
		set elemType [list $elemType [lvarpop structElem]]
		}
	if {$elemType == "signed"} then {
		set elemType [list $elemType [lvarpop structElem]]
		}
	if {$elemType == "struct"} then {
	   set elemType [lvarpop structElem]
	   if {$elemType == "$tag"} then {set elemType $type}
	   }
	set vars $structElem
#Might be multiple variables specified
	regsub -all , $vars " " vars
#Parse indirections
	loop k 0 [llength $vars] {
	   set var [lindex $vars $k]
	   set nstar 0
	   while {1} {
		if {[string index $var 0] != "*"} then break
		incr nstar
		set var [string range $var 1 end]
		}
#Parse array dimensions
	   set ndim 0
	   set dim ""
	   set dimtot 1
	   regsub -all ">" $var "" var
	   set var [split $var <]
	   set name [lindex $var 0]
	   loop j 1 [llength $var] {
		incr ndim
		set idim [lindex $var $j]
#Allow for variable substitutions
#Look up 1 level for a variable definition
#		upvar 1 $idim cvar
#Nah, make it global.  Too much nesting gets in the way otherwise
		catch {global $idim}
		if {[info exists $idim]} then {
		   set idim [set $idim]}
		set dimtot [expr $dimtot*$idim]
		append dim $idim " "
		}
	   set dim [string trimright $dim " "]
	   if {$ndim == 0} then {set dim NULL}
	   if {($dim == 1) && ($elemType == "char")} {
	       echo "WARNING: typedef: in $type,  $elemType $name<$dim> will only have\
		       space for a null terminator."
	   }
	   incr nelem
#Pad according to the current data type.
	   if {$nstar == 0} then {
		set thispad [typepad $elemType]
	   } else {
		set thispad [typepad PTR]
		}
	   set offset [int [expr ([floor [expr ($offset-1)/$thispad]]+1)*$thispad]]
	   set schemaElem($nsch)($nelem) [list $name $elemType $nstar $dim $offset]
#Get size of 1 element
	   if {$nstar == 0} then {
		set size [typeSizeOf $elemType]
	   } else {
		set size [typeSizeOf PTR]
		}
	   set totsize [expr $size*$dimtot]
	   set offset [expr $offset+$totsize]
	   if {$nstar == 0} then {
# Save pad for this variable in order to detect errors in variable
# syntax.
	        set localPad [typepad $elemType]
	   } else {
	        set localPad [typepad PTR]
		}
	   set pad [max $pad $localPad]
	   }
# if we did not get a value for localPad, then there was an error. this is
# probably an error in the expression syntax. (efb)
           if {$localPad == 0} then {
	       echo "Invalid expression - $localStructElem"
	       return
	   }
	}
   set offset [int [expr ([floor [expr ($offset-1)/$pad]]+1)*$pad]]
   set schema($nsch) "$type STRUCT $offset $nelem"
   # schemaDefine may return an error.  Since this is our last 
   # statement before an information-less return we can just let
   # tcl handle this error.
   schemaDefine schema schemaElem
   return
   }
##########################################################################
#
#Top level routines operates on a group of .h files and load schema in one go.
#This is how the dervish makeio facility operates.

proc makeio {args} {
#Check 1st argument.  If it is -pragma, remove from the argument list and
#pass it as a flag to makeioSub
   if {[lindex $args 0] == "-pragma"} then {
	lvarpop args
	set flag 1
   } else {
	set flag 0
	}
   foreach arg $args {
	set files [glob $arg]
	foreach file $files {
	   makeioSub $flag $file
	   }
	}
   return
   }

#Read a .h file and parse off the typedef's.
#If flag is 1, we check for pragmaSCHEMA and relatives.
#
proc makeioSub {flag file} {
   set fid [open $file]
   while {1} {
	set line [gets $fid]
	if {[eof $fid]} then break
#C allows braces to not have surrounding whitespace - an anathema to TCL
#Fix that here.  Also, convert tabs to spaces
	regsub -all \[\{] $line " \{ " line
	regsub -all \[\}] $line " \} " line
	regsub -all \t $line " " line
#Should also take care of character strings here
#Search for #define statements.
	if {[regsub #define $line "" line]} then {
	   set var [lindex $line 0]
	   set val [lindex $line 1]
#Let us make the variable name global; make it available to other files
	   global $var
	   set $var $val
	   continue
	   }
	append list $line " "
	}
   close $fid
#Find and zonk all comments.  I might have a "typedef" embedded in one!
#Convert all /* pragma SCHEMA */ statements to #schema#, which is not valid C
#Convert all /* pragma CONSTRUCTOR */ statements to #schema#, which is not valid
#No constructors, however, are allowed.
   regsub -all \n $list " " list
   regsub -all "pragma\[ ]*NOCONSTRUCTOR" $list \
	" " list
   regsub -all "/\[*]\[ ]*pragma\[ ]*SCHEMA\[ ]*\[*]/" $list \
	"#schema#" list
   regsub -all "/\[*]\[ ]*pragma\[ ]*CONSTRUCTOR\[ ]*\[*]/" $list \
	"#schema#" list
   regsub -all "/\[*]\[ ]*pragma\[ ]*USER\[ ]*\[*]/" $list \
	"#schema#" list
   regsub -all "/\[*]\[ ]*pragma\[ ]*LOCK\[ ]*\[*]/" $list \
	"#schema#" list
   while {1} {
	if {![regexp -indices (/\[*\]) $list c1]} then break
	set first [string range $list 0 [expr [lindex $c1 0]-1]]
	set last [string range $list [expr [lindex $c1 1]+1] end]
	if {![regexp -indices (\[*\]/) $last c2]} then {error "unmatched /* "}
	set last [string range $last [expr [lindex $c2 1]+1] end]
	set list "${first} ${last}"
	}
#Find and zonk all character strings.
   while {1} {
#First, zonk any embedded quotes
	regsub -all \\\" $list " " list
	if {![regexp -indices \" $list c1]} then break
	set first [string range $list 0 [expr [lindex $c1 0]-1]]
	set last [string range $list [expr [lindex $c1 1]+1] end]
	if {![regexp -indices \" $last c2]} then {error "unmatched \" "}
	set last [string range $last [expr [lindex $c2 1]+1] end]
	set list "${first} ${last}"
	}
#search for a "typedef" string
   while {1} {
	if {![regexp -indices "typedef " $list c1]} then break
	set list [string range $list [lindex $c1 0] end]
#We could use lvarpop, but it is slow
	set command [lindex $list 0]
	set list [lrange $list 1 end]
#Because of possible variations on specification of typedefs, I will
#just look for the next list item that ends with a semicolon.
#We could use lvarpop, but it is slow.
	while {1} {
	   set item [lindex $list 0]
	   if {$item == ""} then continue
	   set list [lrange $list 1 end]
	   if {[regexp -indices (\;\$) $item c1]} then {
		set item [string trimright $item \;]
		if {$item != ""} then {lappend command $item}
		break
		}
	   lappend command $item
	   }
#If flag is 1, we check for a #schema# following the typedef
	if {$flag == 0 || [lindex $list 0] == "#schema#"} then {
echo Defining [lindex $command [expr [llength $command]-1]]
	   eval $command
	   }
	}
   return
   }
##########################################################################
ftclHelpDefine shSchema schemaRecurseGetFromType "schemaRecurseGetFromType type"
ftclHelpDefine shSchema schemaRecursePrintFromType \
   "schemaRecursePrintFromType type"
ftclHelpDefine shSchema schemaTransAuto \
   "schemaTransAuto type \{direction write\} \{argList \"\"\}"
ftclHelpDefine shTbl tblPrint "tblPrint tbl"
ftclHelpDefine shTbl tblArrayGet "tblArrayGet tbl col"
ftclHelpDefine shTbl fldPrint "fldPrint tbl col"
ftclHelpDefine shTbl tblFldScaleInfoGet "tblFldScaleInfoGet tbl col"
ftclHelpDefine shTbl tblFix "tblFix tbl"
ftclHelpDefine shTbl tblDataGet "tblDataGet tbl col index"
ftclHelpDefine shTbl tblDataPrint "tblDataPrint tbl index"
ftclHelpDefine shTbl typeNewFromTbl "typeNewFromTbl tbl schemaName"
ftclHelpDefine shChain chainMatchType "chainMatchType chain1 chain2 predicate"
ftclHelpDefine shSchema typedef "\{struct | enum\} \[tag\] \{definition} type;"
ftclHelpDefine shSchema makeio "filespec \[filespec ...\]"
ftclHelpDefine shSchema schemaGetFull "Usage: schemaGetFull <handle expr>\n\t\t\
Return the complete definition of a schema in the form\n\t\t\
used by schemaDefine."
return
