#++
# FACILITY:	Sloan Digital Sky Survey
#		Dervish
#
# ABSTRACT:	Test TBLCOL manipulation.
#
# ENVIRONMENT:	Extended Tcl 7.
#		tst_TBLCOL.tcl
#
#		This test heavily depends on tst_TBLCOLgen.tcl.  The BINTABLE
#		and TABLE extension header information are available in Tcl
#		global variables:
#
#		   ascNAXIS1	binNAXIS1	Bytes per row:	NAXIS1
#		   ascNAXIS2	binNAXIS2	Row count:	NAXIS2
#		   ascFIELDS	binFIELDS	Field count:	TFIELDS
#		   ascFldBCOL			Field offset:	TBCOLn
#		   ascFldFORM	binFldFORM	Data type:	TFORMn
#		   ascFldTYPE	binFldTYPE	Field label:	TTYPEn
#		   ascFldSCAL	binFldSCAL	Scaling value:	TSCALn
#		   ascFldZERO	binFldZERO	Zero point:	TZEROn
#				binFldDIM	Array dimension: TDIMn
#		   ascFldNULL	binFldNULL	Null value:	TNULLn
#
#		Data from the table rows is also availabe in Tcl global var-
#		iables that are lists, each element being a field:
#
#		   ascRow(n)	binRow(n)
#
#		where n is the table row index (0-indexed).  Data from the
#		Binary Table heap is available in the Tcl global variable:
#
#		   binHeap(off,cnt)
#
#		where off is the byte offset from the start of the heap and
#		cnt is the number of entries in the heap at that offset.
#
# AUTHOR:	Tom Nicinski, Creation date: 14-Sep-1995
#--
#*******************************************************************************

global binNAXIS1 binNAXIS2 binFIELDS            binFldFORM binFldTYPE binFldSCAL binFldZERO binFldDIM binFldNULL
global ascNAXIS1 ascNAXIS2 ascFIELDS ascFldBCOL ascFldFORM ascFldTYPE ascFldSCAL ascFldZERO           ascFldNULL

global ascRow binRow binHeap			;# Data in Tables

source tst_TBLCOLgen.tcl			;# This is required

#
# Define mappings between FITS table data types (TFORM) and schema.
#

set ascMapToSchema(I)	{INT}
set ascMapToSchema(F)	{FLOAT}
set ascMapToSchema(E)	{FLOAT}
set ascMapToSchema(D)	{DOUBLE}
set ascMapToSchema(A)	{STR}

set binMapToSchema(B)	{UCHAR}			;# Dervish extension
set binMapToSchema(H)	{CHAR}
set binMapToSchema(I)	{SHORT}
set binMapToSchema(U)	{USHORT}		;# Dervish extension
set binMapToSchema(J)	{INT}
set binMapToSchema(V)	{UINT}			;# Dervish extension
set binMapToSchema(E)	{FLOAT}
set binMapToSchema(D)	{DOUBLE}
set binMapToSchema(L)	{LOGICAL}
set binMapToSchema(A)	{STR}
set binMapToSchema(P)	{TBLHEAPDSC}

foreach lclIdx [array names ascMapToSchema] { set ascMapToFORM($ascMapToSchema($lclIdx)) $lclIdx }
foreach lclIdx [array names binMapToSchema] { set binMapToFORM($binMapToSchema($lclIdx)) $lclIdx }

#*******************************************************************************

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
   if {$severity == {F}} { exit 1 }		;# Fatal causes immediate exit
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

#*******************************************************************************

#*******************************************************************************
#
# Generate the test FITS file with ASCII and Binary Tables.
#

tstErrReset					;# Reset error counts

set tstFITSfile tst_TBLCOL.fits

set tstTbls {r v}		;# Handles (read and verify respecitively)

if  [catch {tstFITStblGen $tstFITSfile TRUE} lclErr] { tstErr F "Unable to create test FITS file $tstFITSfile" $lclErr }

#*******************************************************************************

#*******************************************************************************
#
# Perform some checks on a TBLCOL handle.
#
# A blank TBLCOL handle should contain zero field and row counts.
#

foreach lclHand $tstTbls {
   if  [catch {set tstTbl($lclHand) [tblColNew]} lclErr] {
      tstErr F "tblColNew: unable to obtain $lclHand TBLCOL handle" $lclErr
   }

   if  [catch {set lclTblInfo($lclHand) [tblInfoGet $tstTbl(r)]} lclErr] {
      tstErr E "tblInfoGet: unable to get information on $lclHand TBLCOL $tstTbl($lclHand)" $lclErr
   }

   if  [catch {set lclROWCNT [keylget lclTblInfo($lclHand) ROWCNT]} lclErr] {
      tstErr E "Improperly initialized $lclHand TBLCOL $tstTbl($lclHand)" "Invalid ROWCNT" $lclErr
   } elseif {$lclROWCNT != 0} {
      tstErr E "Improperly initialized $lclHand TBLCOL $tstTbl($lclHand)" "Invalid ROWCNT" "ROWCNT expected to be zero"
   }

   if  [catch {set lclFLDCNT [keylget lclTblInfo($lclHand) FLDCNT]} lclErr] {
      tstErr E "Improperly initialized $lclHand TBLCOL $tstTbl($lclHand)" "Invalid FLDCNT" $lclErr
   } elseif {$lclFLDCNT != 0} {
      tstErr E "Improperly initialized $lclHand TBLCOL $tstTbl($lclHand)" "Invalid FLDCNT" "FLDCNT expected to be zero"
   }
}

tstErrExit E

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
# Add new fields.
#

set lclTblRowCnt 3
set lclFldType INT

#
#  o   The initial field should require the Table row count to be specified.
#

if ![catch {set lclFldIdx [tblFldAdd $tstTbl(r) -type $lclFldType]} lclErr] {
   set lclTblInfo(r) [tblInfoGet $tstTbl(r)]
   tstErr E "tblFldAdd: able to add initial field without specifying row count (-dim)" \
            "TBLCOL $tstTbl(r) row count is [keylget lclTblInfo(r) ROWCNT]"
   tstErrExit E
}

#
#   o	Add a simple field which is legitimate for FITS ASCII and Binary Tables.
#

set lclFldType INT

if  [catch {set lclFldIdx [tblFldAdd $tstTbl(r) -type $lclFldType -dim $lclTblRowCnt]} lclErr] {
   tstErr E "tblFldAdd: unable to add $lclFldType field to TBLCOL $tstTbl(r)" $lclErr
   tstErrExit E
}

set lclTblInfo(r) [tblInfoGet $tstTbl(r)]

if {[keylget lclTblInfo(r) FLDCNT] != 1} {
   tstErr E "tblFldAdd: initial field resulted in [keylget lclTblInfo(r) FLDCNT] fields in TBLCOL $tstTbl(r)"
}

if {[keylget lclTblInfo(r) ROWCNT] != $lclTblRowCnt} {
   tstErr E "tblFldAdd: initial field row count ($lclTblRowCnt) differs from TBLCOL $tstTbl(r)\
             ([keylget lclTblInfo(r) ROWCNT])"
}

if  [catch {set lclFldInfo [tblFldInfoGet $tstTbl(r) -col $lclFldIdx]} lclErr] {
   tstErr E "tblFldInfoGet: unable to get information on TBLCOL $tstTbl($lclHand) field $lclFldIdx" $lclErr
   tstErrExit E
}

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
# Test writing out the Table.
#
#   o	At this point, both FITS Standard ASCII and Binary Tables should be
#	possible.
#

if  [catch {fitsWrite $tstTbl(r) -pdu minimal -ascii  -standard TRUE $tstFITSfile.tmp} lclErr] {
   tstErr E "fitsWrite: unable to write Binary Table conforming to FITS Standard to $tstFITSfile.tmp" $lclErr
   system "rm -f $tstFITSfile.tmp"
   tstErrExit E
}

if  [catch {fitsRead  $tstTbl(v) -hdu 1 -ascii $tstFITSfile.tmp} lclErr] {
   tstErr E "fitsRead: unable to read Binary Table from $tstFITSfile.tmp" $lclErr
   system "rm -f $tstFITSfile.tmp"
   tstErrExit E
}

### compare the tables

if  [catch {fitsWrite $tstTbl(r) -pdu minimal -binary -standard TRUE $tstFITSfile.tmp} lclErr] {
   tstErr E "fitsWrite: unable to write Binary Table conforming to FITS Standard to $tstFITSfile.tmp" $lclErr
   system "rm -f $tstFITSfile.tmp"
   tstErrExit E
}

if  [catch {fitsRead  $tstTbl(v) -hdu 1 -binary $tstFITSfile.tmp} lclErr] {
   tstErr E "fitsRead: unable to read Binary Table from $tstFITSfile.tmp" $lclErr
   system "rm -f $tstFITSfile.tmp"
   tstErrExit E
}

### compare the tables

#
#   o	Add an unsigned integer (larger than 8 bits) field that is not part of
#	the FITS Standard.
#
#	   o   An ASCII Table should be able to handle it.
#	   o   A Binary Table should not be able to handle it.
#

set lclFldType USHORT

if  [catch {set lclFldIdx [tblFldAdd $tstTbl(r) -type $lclFldType]} lclErr] {
   tstErr E "tblFldAdd: unable to add $lclFldType field to TBLCOL $tstTbl(r)" $lclErr
   tstErrExit E
}

if  [catch {fitsWrite $tstTbl(r) -pdu minimal -ascii  -standard TRUE $tstFITSfile.tmp} lclErr] {
   tstErr E "fitsWrite: unable to write ASCII Table conforming to FITS Standard to $tstFITSfile.tmp" \
            "$lclFldType should be convertable to FITS Standard TFORM" $lclErr
}

system "rm -f $tstFITSfile.tmp"

###

#
#   o	Add a heap field to prevent writing out as an ASCII Table.
#

###

system "rm -f $tstFITSfile.tmp"

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
# Continue with regular testing.
#

set lclFldType INT
set lclFldDim {2 4}

if  [catch {set lclFldIdx [tblFldAdd $tstTbl(r) -type $lclFldType -dim "$lclTblRowCnt $lclFldDim"]} lclErr] {
   tstErr E "tblFldAdd: unable to add $lclFldType field to TBLCOL $tstTbl(r)" $lclErr
   tstErrExit E
}

set lclTblInfo(r) [tblInfoGet $tstTbl(r)]

if {[keylget lclTblInfo(r) ROWCNT] != $lclTblRowCnt} {
   tstErr E "tblFldAdd: initial field row count ($lclTblRowCnt) differs from TBLCOL $tstTbl(r)\
             ([keylget lclTblInfo(r) ROWCNT])"
}

if  [catch {set lclFldInfo [tblFldInfoGet $tstTbl(r) -col $lclFldIdx]} lclErr] {
   tstErr E "tblFldInfoGet: unable to get information on TBLCOL $tstTbl($lclHand) field $lclFldIdx" $lclErr
   tstErrExit E
}

if {[keylget lclFldInfo DIM] != "$lclTblRowCnt $lclFldDim"} {
   tstErr E "Field dimensions ([keylget lclFldInfo DIM]) do not match -dim {$lclTblRowCnt $lclFldDim}"
   tstErrExit E
}

#
#   o      o   Give the field a name to be referenced later.
#

set tblFldName {MultiDim}		;# Used throughout tests

if ![catch {tblFldInfoSet $tstTbl(r) -col $lclFldIdx JUNK {Oh oh}} lclErr] {
   tstErr E "tblFldInfoSet: able to set invalid field member JUNK"
}

if  [catch {tblFldInfoSet $tstTbl(r) -col $lclFldIdx TTYPE  $tblFldName} lclErr] {
   tstErr E "tblFldInfoSet: unable to set field TTYPE to \"$tblFldName\"" $lclErr
}

if  [catch {set lclFldInfo [tblFldInfoGet $tstTbl(r) -field $tblFldName]} lclErr] {
   tstErr E "tblFldInfoGet: unable to get information on TBLCOL $tstTbl($lclHand) field \"$tblFldName\" ($lclFldIdx)" $lclErr
}

if {$lclFldIdx != [keylget lclFldInfo COL]} {
   tstErr E "tblFldInfoGet: located wrong field, [keylget lclFldInfo COL] (expected $lclFldIdx), with TTYPE \"$tblFldName\""
}

tstErrExit E

#
#   o   Make sure Table row count is enforced.
#

set lclRowCnt  [expr {$lclTblRowCnt + 1}]	;# Choose something wrong
set lclFldType INT

if ![catch {set lclFldIdx [tblFldAdd $tstTbl(r) -type $lclFldType -dim $lclRowCnt]} lclErr] {
   set lclTblInfo(r) [tblInfoGet $tstTbl(r)]
   tstErr E "tblFldAdd: able to add field with different row count ($lclRowCnt) than TBLCOL $tstTbl(r)\
             ([keylget lclTblInfo(r) ROWCNT])"
   tstErrExit E
}

#
#   o   Fail on an attempt to get information for more fields.
#

set lclTblInfo(r) [tblInfoGet $tstTbl(r)]

if ![catch {tblFldInfoGet $tstTbl(r) -col -1} lclErr] {
   tstErr E "tblFldInfoGet: able to access (0-indexed) field with -1"
}

if ![catch {tblFldInfoGet $tstTbl(r) -col [keylget lclTblInfo(r) FLDCNT]} lclErr] {
   tstErr E "tblFldInfoGet: able to access (0-indexed) field [keylget lclTblInfo(r) FLDCNT] -- beyond TBLCOL boundary"
}

#
#   o	Check that dimensions are obeyed.
#

set lclTblInfo(r) [tblInfoGet    $tstTbl(r)]
set lclFldInfo       [tblFldInfoGet $tstTbl(r) -field $tblFldName]
set lclFldIdx        [keylget lclFldInfo COL]

if ![catch {tblFldSet $tstTbl(r) -col $lclFldIdx -1 0} lclErr] {
   tstErr E "tblFldSet: able to set (0-indexed) field $lclFldIdx datum at index -1 (0-indexed)"
}

###if ![catch {tblFldSet $tstTbl(r) -col $lclFldIdx %%% 0} lclErr] {
###}

tstErrExit E

### More tests (set values) ###
#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
#   o	Add a new field that uses heap.
#

set lclTblInfo(r) [tblInfoGet $tstTbl(r)]
set lclFldType DOUBLE
set tblFldHeapName {Heap}

if ![catch {set lclFldIdx [tblFldAdd $tstTbl(r) -type TBLHEAPDSC -dim $lclTblRowCnt]} lclErr] {
   tstErr E "tblFldAdd: able to add TBLHEAPDSC (heap) field to TBLCOL $tstTbl(r) without specifying -heaptype
}

if ![catch {set lclFldIdx [tblFldAdd $tstTbl(r) -type TBLHEAPDSC -heaptype TBLHEAPDSC]} lclErr] {
   tstErr E "tblFldAdd: able to add TBLHEAPDSC (heap) field to TBLCOL $tstTbl(r) of type TBLHEAPDSC (heap)"
}

if ![catch {set lclFldIdx [tblFldAdd $tstTbl(r) -heaptype $lclFldType]} lclErr] {
   tstErr E "tblFldAdd: able to add TBLHEAPDSC (heap) field to TBLCOL $tstTbl(r) without specifying -heapcnt"
}

loop lclHeapIdx 0 [expr {$lclTblRowCnt - 1}] { lappend lclHeapCnt $lclHeapIdx }

if ![catch {set lclFldIdx [tblFldAdd $tstTbl(r) -heaptype $lclFldType -heapcnt $lclHeapCnt]} lclErr] {
   tstErr E "tblFldAdd: able to add TBLHEAPDSC (heap) field of type $lclFldType to TBLCOL $tstTbl(r) with too few -heapcnt"
}

loop lclHeapIdx $lclHeapIdx [expr {$lclHeapIdx + 2}] { lappend lclHeapCnt $lclHeapIdx }

if ![catch {set lclFldIdx [tblFldAdd $tstTbl(r) -heaptype $lclFldType -heapcnt $lclHeapCnt]} lclErr] {
   tstErr E "tblFldAdd: able to add TBLHEAPDSC (heap) field of type $lclFldType to TBLCOL $tstTbl(r) with too many -heapcnt"
}

set lclHeapCnt {};  loop lclHeapIdx 0 $lclTblRowCnt { lappend lclHeapCnt $lclHeapIdx }

if  [catch {set lclFldIdx [tblFldAdd $tstTbl(r) -heaptype $lclFldType -heapcnt $lclHeapCnt]} lclErr] {
   tstErr E "tblFldAdd: unable to add TBLHEAPDSC (heap) field of type $lclFldType to TBLCOL $tstTbl(r)" $lclErr
}

if  [catch {tblFldInfoSet $tstTbl(r) -col $lclFldIdx TTYPE $tblFldHeapName} lclErr] {
   tstErr E "tblFldInfoSet: unable to set field TTYPE to \"$tblFldHeapName\"" $lclErr
}

if  [catch {set lclFldInfo [tblFldInfoGet $tstTbl(r) -col $lclFldIdx]} lclErr] {
   tstErr E "tblFldInfoGet: unable to get field $lclFldIdx information from TBLCOL $tstTbl(r)" $lclErr
}

tstErrExit E

if  [catch {set lclRow [tblFldGet $tstTbl(r) -col $lclFldIdx {}]} lclErr] {
   tstErr E "tblFldGet: unable to get all field $lclFldIdx values from TBLCOL $tstTbl(r)" $lclErr
}

loop lclRowIdx 0 $lclTblRowCnt {
   if  [catch {set lclRow [tblFldGet $tstTbl(r) -col $lclFldIdx $lclRowIdx]} lclErr] {
      tstErr E "tblFldGet: unable to get field $lclFldIdx values from TBLCOL $tstTbl(r)" $lclErr
   }
   if {[llength $lclRow] != [lindex $lclHeapCnt $lclRowIdx]} {
      tstErr E "TBLCOL $tstTbl(r) field $lclFldIdx (row $lclRowIdx) heap count ([llength $lclRow]) does not match expected\
                count of [lindex $lclHeapCnt $lclRowIdx]"
   }
   ### Check for zero values ###
}

### More tests (fill heap data, etc.) ###

tstErrExit E

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
# Clean up a TBLCOL handle.
#

if  [catch {tblColDel $tstTbl(r)}      lclErr] { tstErr E "tblColDel: unable to delete TBLCOL handle $tstTbl(r)" $lclErr }
if  [catch {set tstTbl(r) [tblColNew]} lclErr] { tstErr F "tblColNew: unable to obtain new read TBLCOL handle" $lclErr }

tstErrExit E

#*******************************************************************************

#*******************************************************************************
#
# Get a directory of the FITS file.
#

if  [catch {set tstFITSdir [fitsDirGet $tstFITSfile]} lclErr] { tstErr E "Unable to get directory of $tstFITSfile" $lclErr }

set tstFITShduCnt [llength $tstFITSdir]

loop lclHDU 0 $tstFITShduCnt {
   if {[keylget tstFITSdir "HDU$lclHDU.TYPE"] == {TABLE}}    { set tstFITShdu(TABLE)    $lclHDU } else {
   if {[keylget tstFITSdir "HDU$lclHDU.TYPE"] == {BINTABLE}} { set tstFITShdu(BINTABLE) $lclHDU }      }
}

if {[lsearch -exact [array names tstFITShdu] TABLE]    == -1} { tstErr W "Unable to locate an ASCII Table in $tstFITSfile" }
if {[lsearch -exact [array names tstFITShdu] BINTABLE] == -1} { tstErr W "Unable to locate a Binary Table in $tstFITSfile" }

#*******************************************************************************

#*******************************************************************************
#
# Check the ASCII Table.
#

if {[lsearch -exact [array names tstFITShdu] TABLE]    != -1} {
   #
   # Check whether an ASCII Table can be located with -xtension.
   #

   if  [catch {fitsRead $tstTbl(r) $tstFITSfile -xtension TABLE} lclErr] {
      tstErr W "fitsRead: unable to read ASCII Table from $tstFITSfile" "Using fitsRead -xtension TABLE" $lclErr
   }

   #
   # Read in the ASCII Table we'll test against.
   #
   #   o   The previous handle contents are implicitly deleted.
   #

   if  [catch {fitsRead $tstTbl(r) $tstFITSfile -hdu $tstFITShdu(TABLE) -ascii}  lclErr] {
      tstErr E "fitsRead: unable to read ASCII Table from $tstFITSfile"  $lclErr
   }

   set lclTblInfo(r) [tblInfoGet $tstTbl(r)]

   if {[keylget lclTblInfo(r) FLDCNT] != $ascFIELDS} {
      tstErr E "TABLE: field count ([keylget lclTblInfo(r)]) differs from expected $ascFIELDS"
   }

   if {[keylget lclTblInfo(r) ROWCNT] != $ascNAXIS2} {
      tstErr E "TABLE: row count ([keylget lclTblInfo(r)]) differs from expected $ascNAXIS2"
   }

   #
   #   o   Check the fields.
   #

   tstErrExit E

   loop lclFldIdx 0 $ascFIELDS {
      if  [catch {set lclFldInfo [tblFldInfoGet $tstTbl(r) -col $lclFldIdx]} lclErr] {
         tstErr E "tblFldInfoGet: unable to get field $lclFldIdx information from TBLCOL $tstTbl(r)" $lclErr
         tstErrExit E
      }
   ### Check field types, etc.
   }

   ### Check

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   #
   # Write the Table as an ASCII Table and a Binary Table.
   #
   #   o   Reread each table and comparing it against the original.
   #

   if  [catch {fitsWrite $tstTbl(r) -pdu minimal -ascii -standard TRUE $tstFITSfile.tmp} lclErr] {
      tstErr E "fitsWrite: unable to write ASCII Table to $tstFITSfile.tmp" $lclErr
      system "rm -f $tstFITSfile.tmp"
      tstErrExit E
   }

   if  [catch {fitsRead  $tstTbl(v) -hdu 1 -ascii $tstFITSfile.tmp} lclErr] {
      tstErr E "fitsRead: unable to read ASCII Table from $tstFITSfile.tmp" $lclErr
      system "rm -f $tstFITSfile.tmp"
      tstErrExit E
   }

   ### check that the two Tables are the same

   system "rm -f $tstFITSfile.tmp"

   if  [catch {fitsWrite $tstTbl(r) -pdu minimal -binary -standard TRUE $tstFITSfile.tmp} lclErr] {
      tstErr E "fitsWrite: unable to write Binary Table to $tstFITSfile.tmp" $lclErr
      system "rm -f $tstFITSfile.tmp"
      tstErrExit E
   }

   if  [catch {fitsRead  $tstTbl(v) -hdu 1 -binary $tstFITSfile.tmp} lclErr] {
      tstErr E "fitsRead: unable to read Binary Table from $tstFITSfile.tmp" $lclErr
      system "rm -f $tstFITSfile.tmp"
      tstErrExit E
   }

   ### check that the two Tables are the same

   system "rm -f $tstFITSfile.tmp"
}

tstErrExit E

#*******************************************************************************

#*******************************************************************************
#
# Check the Binary Table.
#

if {[lsearch -exact [array names tstFITShdu] BINTABLE] != -1} {
   #
   # Read in the Binary Table we'll test against.
   #
   #   o   The previous handle contents are implicitly deleted.
   #

   if  [catch {fitsRead $tstTbl(r) $tstFITSfile -hdu $tstFITShdu(BINTABLE) -binary} lclErr] {
      tstErr E "fitsRead: unable to read Binary Table from $tstFITSfile" $lclErr
   }

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   #
   # Write the Table as a Binary Table.
   #
   #   o   Reread each table and comparing it against the original.
   #

   system "rm -f $tstFITSfile.tmp"

   if  [catch {fitsWrite $tstTbl(r) -pdu minimal -binary -standard FALSE $tstFITSfile.tmp} lclErr] {
      tstErr E "fitsWrite: unable to write Binary Table to $tstFITSfile.tmp" $lclErr
      system "rm -f $tstFITSfile.tmp"
      tstErrExit E
   }

   if  [catch {fitsRead  $tstTbl(v) -hdu 1 -binary $tstFITSfile.tmp} lclErr] {
      tstErr E "fitsRead: unable to read Binary Table from $tstFITSfile.tmp" $lclErr
      system "rm -f $tstFITSfile.tmp"
      tstErrExit E
   }

   ### check that the two Tables are the same

   system "rm -f $tstFITSfile.tmp"
}

#*******************************************************************************

tstErrExit E

tblColDel $tstTbl(r)
tblColDel $tstTbl(v)

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_TBLCOL.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_TBLCOL"
    return 1
}

return {}

#*******************************************************************************
