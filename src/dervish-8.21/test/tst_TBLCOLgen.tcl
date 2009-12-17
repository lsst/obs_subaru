#++
# FACILITY:	Sloan Digital Sky Survey
#		Dervish
#
# ABSTRACT:	Generate a test FITS file that contains a Binary Table and an
#		ASCII Table.
#
# ENVIRONMENT:	Extended Tcl.
#		tst_TBLCOLgen.tcl
#
# AUTHOR:	Tom Nicinski, Creation date:
#--
#*******************************************************************************
#
# Define some global constants.
#

set fitsLineSize   80				;# Bytes per line of FITS header
set fitsRecSize  2880				;# FITS logical record size

#*******************************************************************************

#++
# ROUTINE:		tstFITStblGen
#
#	Generate a FITS file with ASCII (TABLE) and Binary Table (BINTABLE)
#	extensions.  A mininal PDU (Primary Data Unit) can also be generated.
#
# RETURN VALUES:	None
#
# CONCERNS:
#
#   o	Dervish must have properly functioning REGIONs.
#--
#*******************************************************************************

proc tstFITStblGen {fitsFileSpec {PDU FALSE}} {

   global fitsLineSize
   global fitsRecSize

   set tmpFileSpec $fitsFileSpec.tmp

   # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
   #
   # Generate a FITS Binary Table HDU (Header Data Unit).
   #
   #   o   fitsFileSpec is the file specification of the resulting FITS file.
   #
   #   o   tmpFileSpec is the file specification for a file that is generated
   #       temporarily and then removed.  It is based on fitsFileSpec.
   #
   #   o   A primary HDU (header only) is generated first if PDU is set to
   #       {TRUE}, since the FITS standard requires this.
   #
   # Because Dervish adds extra keywords (some duplicates of the ones being set
   # here) and not in the correct places, we need to build our own header.
   #
   #   o   Fill in the header (hardwire values).
   #
   #   o   No gap is present between the record storage area and the heap area:
   #
   #       .------> +-------------------------+ <--.
   #       |        |                         |    |
   #       |        |   Binary Table Header   | n * (FITS logical record size)
   #       |        |                         |    |
   #       |   .--> +-------------------------+ <--+
   #       |   |    |                         |    |
   #       |   |    |   Record Storage Area   |  NAXIS1 * NAXIS2
   #       | THEAP  |                         |    |
   #       |   |    +-------------------------+ <--+
   #       |   |    |           gap           |    |
   #       |   `--> +-------------------------+    |
   #       |        |                         |  PCOUNT
   #       |        |        Heap Area        |    |
   #       |        |                         |    |
   #       |        +-------------------------+ <--'
   #       |        | 0 ...                   |
   #       |        |                   ... 0 |
   #       `--.---> +-------------------------+
   #          |
   #          p * (FITS logical record size)
   #

global binNAXIS1 binNAXIS2 binFIELDS            binFldFORM binFldTYPE binFldSCAL binFldZERO binFldDIM binFldNULL

set binNAXIS1  172			;# Bytes per row
set binNAXIS2    3			;# Row   count
set binFIELDS   15			;# Field count
set binFldFORM {Reserved {1I}    {1U}  {1PI(8)} {15A} {1E}  {1D}  {2J} {24V}     {8A}     {1B} {0J}   {1PA}  {2L} {1H} {1PB}  {1A} }
set binFldTYPE {Reserved {U16*}  {U16} {vI16}   {STR} {F32} {F64} {1D} {3D U32}  {2D STR} {U8} {0cnt} {vSTR} {}   {S8} {vS8*} {}   }
set binFldSCAL {Reserved {    1} {}    {1.0}    {1.0} {}    {}    {}   {}        {}       {}   {}     {}     {}   {}   {  1}  {}   }
set binFldZERO {Reserved {32768} {}    {0.0}    {0.0} {}    {}    {}   {}        {}       {}   {}     {}     {}   {}   {128}  {}   }
set binFldDIM  {Reserved {}      {}    {}       {}    {}    {}    {}   {(2,3,4)} {(4,2)}  {}   {}     {}     {}   {}   {}     {}   }
set binFldNULL {Reserved {}      {-1}  {}       {}    {}    {}    {}   {}        {}       {}   {}     {}     {}   {}   {}     {}   }
#                        |       |     |        |     |     |     |    |         |        |    |      |      |    |    |      |
#                        V       V     V        V     V     V     V    V         V        V    V      V      V    V    V      V
proc binRecRow {reg row  f1      f2    f3       f4    f5    f6    f7   f8        f9       f10  f11    f12    f13  f14  f15    f16} {
   set byteOff {Reserved 0       2     4        12    27    31    39   47       143       151  152    152    160  162  163    171  172}
   upvar binFldFORM up_binFldFORM  ;#  |                                                               `-----------------. \
                                   ;#  `-------------------------------------------------------------------------------. | |
   tblRecPutShort $reg $row [lindex $byteOff  1]        [binRecFldColCnt [lindex $up_binFldFORM  1]]       $f1    ;#   | | |
   tblRecPutShort $reg $row [lindex $byteOff  2]        [binRecFldColCnt [lindex $up_binFldFORM  2]]       $f2    ;# hardwired
   tblRecPutLong  $reg $row [lindex $byteOff  3]  2                                                        $f3    ;# <-' | |
   tblRecPutASCII $reg $row [lindex $byteOff  4]        [binRecFldColCnt [lindex $up_binFldFORM  4]]       $f4  0 ;#     | |
   tblRecPutLong  $reg $row [lindex $byteOff  5]        [binRecFldColCnt [lindex $up_binFldFORM  5]]       $f5    ;#     | |
   tblRecPutLong  $reg $row [lindex $byteOff  6] [expr {[binRecFldColCnt [lindex $up_binFldFORM  6]] * 2}] $f6    ;#     | |
   tblRecPutLong  $reg $row [lindex $byteOff  7]        [binRecFldColCnt [lindex $up_binFldFORM  7]]       $f7    ;#     | |
   tblRecPutLong  $reg $row [lindex $byteOff  8]        [binRecFldColCnt [lindex $up_binFldFORM  8]]       $f8    ;#     | |
   tblRecPutASCII $reg $row [lindex $byteOff  9]        [binRecFldColCnt [lindex $up_binFldFORM  9]]       $f9  0 ;#     | |
   tblRecPutByte  $reg $row [lindex $byteOff 10]        [binRecFldColCnt [lindex $up_binFldFORM 10]]       $f10   ;#     | |
   tblRecPutLong  $reg $row [lindex $byteOff 11]        [binRecFldColCnt [lindex $up_binFldFORM 11]]       $f11   ;#     | |
   tblRecPutLong  $reg $row [lindex $byteOff 12]  2                                                        $f12   ;# <---' |
   tblRecPutASCII $reg $row [lindex $byteOff 13]        [binRecFldColCnt [lindex $up_binFldFORM 13]]       $f13 0 ;#       |
   tblRecPutByte  $reg $row [lindex $byteOff 14]        [binRecFldColCnt [lindex $up_binFldFORM 14]]       $f14   ;#       |
   tblRecPutLong  $reg $row [lindex $byteOff 15]  2                                                        $f15   ;# <-----'
   tblRecPutASCII $reg $row [lindex $byteOff 16]        [binRecFldColCnt [lindex $up_binFldFORM 16]]       $f16 0

   #
   # Provide this row to other Tcl users.
   #

   global binRow
   foreach lclArg [lrange [info args binRecRow] 2 end] {  ;# Our args less reg, row
      lappend binRow($row) [eval {set $lclArg}]
   }
}

   proc binHeap {type reg row byteOff cnt values} {
      tblRecPut$type $reg $row $byteOff $cnt $values

      global binHeap
      set binHeap($byteOff,$cnt) $values
   }

   # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
   #
   # Generate a FITS ASCII Table HDU (Header Data Unit).
   #

global ascNAXIS1 ascNAXIS2 ascFIELDS ascFldBCOL ascFldFORM ascFldTYPE ascFldSCAL ascFldZERO           ascFldNULL

   set ascNAXIS1  140			;# Bytes per row
   set ascNAXIS2    2			;# Row   count
   set ascFIELDS    9			;# Field count
   set ascFldBCOL {Reserved {1}       {6}    {11}      {26}     {53}     {85}     {116}    {123}    {133}    {140}    }
   set ascFldFORM {Reserved {I5}      {I5}   {A15}     {F27.25} {E32.26} {D31.25} {F7.2}   {E10.3}  {F7.5}   {A1}     }
   set ascFldTYPE {Reserved {U16a}    {U16b} {ASCII}   {Float}  {Float}  {Double} {Fixed1} {Fixed2} {Fixed3} {Filler} }
   set ascFldSCAL {Reserved {    1.0} {}     {}        {   1.0} {}       {}       {}       {}       {}       {}       }
   set ascFldZERO {Reserved {32768.0} {}     {}        {   0.0} {}       {}       {}       {}       {}       {}       }
   set ascFldNULL {Reserved {***}     {-1}   {}        {}       {}       {}       {}       {}       {}       {}       }
#                           |         |      |         |        |        |        |        |        |        |
#                           V         V      V         V        V        V        V        V        V        V
   proc ascRecRow {reg row  f1        f2     f3        f4       f5       f6       f7       f8       f9       f10}     {
      set byteOff {Reserved 0         5      10        25       52       84       115      122      132      139      140}
      upvar ascFldFORM up_ascFldFORM
      tblRecPutASCII $reg $row [lindex $byteOff  1] [ascRecFldWidth [lindex $up_ascFldFORM  1]] $f1  32
      tblRecPutASCII $reg $row [lindex $byteOff  2] [ascRecFldWidth [lindex $up_ascFldFORM  2]] $f2  32
      tblRecPutASCII $reg $row [lindex $byteOff  3] [ascRecFldWidth [lindex $up_ascFldFORM  3]] $f3  32
      tblRecPutASCII $reg $row [lindex $byteOff  4] [ascRecFldWidth [lindex $up_ascFldFORM  4]] $f4  32
      tblRecPutASCII $reg $row [lindex $byteOff  5] [ascRecFldWidth [lindex $up_ascFldFORM  5]] $f5  32
      tblRecPutASCII $reg $row [lindex $byteOff  6] [ascRecFldWidth [lindex $up_ascFldFORM  6]] $f6  32
      tblRecPutASCII $reg $row [lindex $byteOff  7] [ascRecFldWidth [lindex $up_ascFldFORM  7]] $f7  32
      tblRecPutASCII $reg $row [lindex $byteOff  8] [ascRecFldWidth [lindex $up_ascFldFORM  8]] $f8  32
      tblRecPutASCII $reg $row [lindex $byteOff  9] [ascRecFldWidth [lindex $up_ascFldFORM  9]] $f9  32
      tblRecPutASCII $reg $row [lindex $byteOff 10] [ascRecFldWidth [lindex $up_ascFldFORM 10]] $f10 32

      #
      # Provide this row to other Tcl users.
      #

      global ascRow
      foreach lclArg [lrange [info args ascRecRow] 2 end] {  ;# Our args less reg, row
         lappend ascRow($row) [eval {set $lclArg}]
      }
   }

   # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
   #
   #   o   Useful procedures for filling the data area.
   #

   proc binRecFldColCnt {form} {
      if {[regexp {[ \t]*([0-9]+)}           $form lcl_temp lcl_cnt]   == 0} {
         return 0
      } else {
         return $lcl_cnt
      }
   }

   proc ascRecFldWidth  {form} {
      if {[regexp {[ \t]*[a-zA-Z]([0-9]+)\.?} $form lcl_temp lcl_width] == 0} {
         return 0
      } else {
         return $lcl_width
      }
   }

   proc tblASCIItoByte {asciiChar} {
      scan $asciiChar {%c} asciiDec
      return              $asciiDec
   }

   proc tblRecPutASCII {reg  row byteOff cnt values {fill 0}} {
      if {$cnt <= 0} { return }
      incr byteOff -1
      loop valueIdx         0 $cnt {
         if {[set value [string index $values $valueIdx]] == {}} { break }
         regPixSetWithDbl $reg $row [incr byteOff] [tblASCIItoByte $value]
      }
      loop valueIdx [incr valueIdx] $cnt {
         regPixSetWithDbl $reg $row [incr byteOff] $fill
      }
   }

   proc tblRecPutByte  {reg  row byteOff cnt values} {
      if {$cnt <= 0} { return }
      incr byteOff -1
      loop valueIdx         0 $cnt {
         if {[set value [lindex $values $valueIdx]] == {}} { break }
         regPixSetWithDbl $reg $row [incr byteOff] [expr { $value        & 0xFF}]
      }
      loop valueIdx [incr valueIdx] $cnt {
         regPixSetWithDbl $reg $row [incr byteOff]  0
      }
   }

   proc tblRecPutShort {reg  row byteOff cnt values} {
      if {$cnt <= 0} { return }
      incr byteOff -1
      set  valueIdx                     0
      loop valueIdx                     0  $cnt {
         if {[set value [lindex $values $valueIdx]] == {}} { break }
         regPixSetWithDbl $reg $row [incr byteOff] [expr {($value >>  8) & 0xFF}]
         regPixSetWithDbl $reg $row [incr byteOff] [expr { $value        & 0xFF}]
      }
      loop valueIdx [incr valueIdx] $cnt {
         regPixSetWithDbl $reg $row [incr byteOff]  0
         regPixSetWithDbl $reg $row [incr byteOff]  0
      }
   }

   proc tblRecPutLong  {reg  row byteOff cnt values} {
      if {$cnt <= 0} { return }
      incr byteOff -1
      loop valueIdx         0 $cnt {
         if {[set value [lindex $values $valueIdx]] == {}} { break }
         regPixSetWithDbl $reg $row [incr byteOff] [expr {($value >> 24) & 0xFF}]
         regPixSetWithDbl $reg $row [incr byteOff] [expr {($value >> 16) & 0xFF}]
         regPixSetWithDbl $reg $row [incr byteOff] [expr {($value >>  8) & 0xFF}]
         regPixSetWithDbl $reg $row [incr byteOff] [expr { $value        & 0xFF}]
      }
      loop valueIdx [incr valueIdx] $cnt {
         regPixSetWithDbl $reg $row [incr byteOff]  0
         regPixSetWithDbl $reg $row [incr byteOff]  0
         regPixSetWithDbl $reg $row [incr byteOff]  0
         regPixSetWithDbl $reg $row [incr byteOff]  0
      }
   }

   proc tblHdrFmt {type keyword value {comment {}}} {
      global fitsLineSize
      upvar  tblHdr up_tblHdr
      append up_tblHdr [format "%-${fitsLineSize}.${fitsLineSize}s" [hdrMakeLineWith${type} $keyword $value $comment]]
   }

   proc tblHdrAdd {text} {
      global fitsLineSize
      upvar  tblHdr up_tblHdr
      append up_tblHdr [format "%-${fitsLineSize}.${fitsLineSize}s" $text]
   }

#  Cannot do things this way as format no longer accepts strings longer
#  than 1000 bytes.  So we will append to the tblHdr $fitsLineSize at a 
#  time to get $fitsRecSize blocks.
#   proc tblHdrPad {} {
#      global fitsRecSize
#      upvar  tblHdr up_tblHdr

#      set tblHdrLen  [string length $up_tblHdr]
#      if {[set tblHdrPadLen  [expr {((($tblHdrLen / $fitsRecSize) + 1) * $fitsRecSize) - $tblHdrLen}]] == $fitsRecSize} {
#           set tblHdrPadLen 0
#      }
#      append up_tblHdr [format "%-${tblHdrPadLen}.${tblHdrPadLen}s" {}]
#   }

   proc tblHdrPad {} {
      global fitsRecSize
      global fitsLineSize
      upvar  tblHdr up_tblHdr
      set fitsNumLines [expr {$fitsRecSize / $fitsLineSize}]
      set tblNumLines  [expr {[string length $up_tblHdr] / $fitsLineSize}]
      if {$tblNumLines > $fitsNumLines} {
	  set fitsNumPadLineMax [expr {((($tblNumLines / $fitsNumLines) + 1) * $fitsNumLines) - $tblNumLines}]
	  set tblNumLines 0
      } else {
	  set fitsNumPadLineMax $fitsNumLines
      }
      loop i $tblNumLines $fitsNumPadLineMax {
	  append up_tblHdr [format "%-${fitsLineSize}.${fitsLineSize}s" {}]
      }
   }

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   #
   # Generate a primary header (no data).
   #

   set tblHdr {}

   if {$PDU == {TRUE}} {

      tblHdrFmt Logical	SIMPLE		         T
      tblHdrFmt Int	BITPIX		         8
      tblHdrFmt Int	NAXIS		         0
      tblHdrFmt Logical	EXTEND		         T
      tblHdrAdd		END
      tblHdrPad
   }

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   #
   # Generate an ASCII Table.
   #
   # Generate the TABLE extension header.
   #

###   set tblHdr {}

   tblHdrFmt Ascii	XTENSION	{TABLE}
   tblHdrFmt Int	BITPIX		         8
   tblHdrFmt Int	NAXIS		         2
   tblHdrFmt Int	NAXIS1		$ascNAXIS1
   tblHdrFmt Int	NAXIS2		$ascNAXIS2
   tblHdrFmt Int	PCOUNT		         0
   tblHdrFmt Int	GCOUNT		         1
   tblHdrFmt Int	TFIELDS		$ascFIELDS
   tblHdrFmt Ascii	EXTNAME		{Dervish ASCII Table}
   tblHdrFmt Int	EXTVER		         0
   tblHdrFmt Int	EXTLEVEL	         0

   loop ascFld 1 [expr {$ascFIELDS + 1}] {
      if {[set tblBCOL [lindex $ascFldBCOL $ascFld]] != {}} { tblHdrFmt Int   TBCOL${ascFld} $tblBCOL }
      if {[set tblFORM [lindex $ascFldFORM $ascFld]] != {}} { tblHdrFmt Ascii TFORM${ascFld} $tblFORM }
      if {[set tblTYPE [lindex $ascFldTYPE $ascFld]] != {}} { tblHdrFmt Ascii TTYPE${ascFld} $tblTYPE }
      if {[set tblSCAL [lindex $ascFldSCAL $ascFld]] != {}} { tblHdrFmt Dbl   TSCAL${ascFld} $tblSCAL }
      if {[set tblZERO [lindex $ascFldZERO $ascFld]] != {}} { tblHdrFmt Dbl   TZERO${ascFld} $tblZERO }
      if {[set tblNULL [lindex $ascFldNULL $ascFld]] != {}} { tblHdrFmt Ascii TNULL${ascFld} $tblNULL }
   }

   tblHdrFmt Ascii	IVKEYWD		{This is an unrecognized keyword}
   tblHdrAdd {COMMENT   1         2         3         4         5         6         7}
   tblHdrAdd {COMMENT 890123456789012345678901234567890123456789012345678901234567890123456789}
   tblHdrAdd		END
   tblHdrPad
   # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
   #
   # Fill in fudged data into the ASCII Table proper.
   #

   set tblReg [regNew -type U8 $ascNAXIS2 $ascNAXIS1]
   regSetWithDbl $tblReg [tblASCIItoByte {*}]	;# Avoid explicit field padding

   ascRecRow $tblReg 0	{   65} {   66} {<abcdefghijklm>} {NaN}                         {0.31415926535897932384626433E+01}\
			                                  {3.1415926535897932384626433D+00} { -314  } { -314E-02 } { -314  } {}
   ascRecRow $tblReg 1	{32832} {32833} {>nopqrstuvwxyz<} {3.1415926535897932384626433} {0.31415926535897932384626433E+01}\
			                                  {NaN}                             { -314  } { -314E-02 } { -314  } {}

   # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
   #
   # Write the FITS file out.
   #
   #   o   Get rid of the FITS header that Dervish generated and prepend ours.
   #

   regWriteAsFits $tblReg   $tmpFileSpec
   regDel         $tblReg

   set       fitsFile [open $fitsFileSpec  w]
   puts     $fitsFile $tblHdr nonewline			;# Prepend our header

   set       dataFile [open $tmpFileSpec   r]

   for {set fitsHdrFlush 1} {$fitsHdrFlush} { } {	;# Flush FITS logical
      loop fitsRead 0 $fitsRecSize $fitsLineSize {	;# ... record chunks,
         set fitsLine [read  $dataFile $fitsLineSize]	;# ... stopping with
         if {[regexp {^END +$} $fitsLine]} {		;# ... the record
            set fitsHdrFlush 0				;# ... containing END
         }
      }
   }

   copyfile $dataFile $fitsFile				;# Append data portion
   close    $fitsFile
   close    $dataFile

   exec rm  $tmpFileSpec

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   #
   # Generate a Binary Table.
   #
   # Generate the BINTABLE extension header.
   #
   #   o  The Binary Table's heap   M U S T   be contained within the same FITS
   #      record as the last byte of data in the Record Storage Area (RSA). So,
   #      don't make things too big.
   #
   #   o  The heap will be filled in as a REGION rows. To simplify matters, the
   #      heap size is a multiple of the row size and the gap (if any) between
   #      between the RSA and the heap area is a multiple of the row size also.
   #

   set tblHdr {}

   set tblHeapGap  1		;# Generate RSA/heap gap (in $binNAXIS1 units)
   set tblHeapSize 2		;# Heap size             (in $binNAXIS1 units)

   tblHdrFmt Ascii	XTENSION	{BINTABLE}
   tblHdrFmt Int	BITPIX		         8
   tblHdrFmt Int	NAXIS		         2
   tblHdrFmt Int	NAXIS1		$binNAXIS1
   tblHdrFmt Int	NAXIS2		$binNAXIS2
   tblHdrFmt Int	PCOUNT		[expr {($tblHeapGap + $tblHeapSize) * $binNAXIS1}]
   tblHdrFmt Int	GCOUNT		         1
   tblHdrFmt Int	TFIELDS		$binFIELDS
   if {$tblHeapGap > 0} {
      tblHdrFmt Int	THEAP		[expr {$binNAXIS1 * ($binNAXIS2 + $tblHeapGap)}]
   }
   tblHdrFmt Ascii	EXTNAME		{Dervish Binary Table}
   tblHdrFmt Int	EXTVER		         0
   tblHdrFmt Int	EXTLEVEL	         0

   loop binFld 1 [expr {$binFIELDS + 1}] {
      if {[set tblFORM [lindex $binFldFORM $binFld]] != {}} { tblHdrFmt Ascii TFORM${binFld} $tblFORM }
      if {[set tblTYPE [lindex $binFldTYPE $binFld]] != {}} { tblHdrFmt Ascii TTYPE${binFld} $tblTYPE }
      if {[set tblSCAL [lindex $binFldSCAL $binFld]] != {}} { tblHdrFmt Dbl   TSCAL${binFld} $tblSCAL }
      if {[set tblZERO [lindex $binFldZERO $binFld]] != {}} { tblHdrFmt Dbl   TZERO${binFld} $tblZERO }
      if {[set tblDIM  [lindex $binFldDIM  $binFld]] != {}} { tblHdrFmt Ascii TDIM${binFld}  $tblDIM  }
      if {[set tblNULL [lindex $binFldNULL $binFld]] != {}} { tblHdrFmt Int   TNULL${binFld} $tblNULL }
   }

   tblHdrFmt Ascii	IVKEYWD		{This is an unrecognized keyword}
   tblHdrAdd {COMMENT  1         2         3         4         5         6         7         8}
   tblHdrAdd {COMMENT 901234567890123456789012345678901234567890123456789012345678901234567890}
   tblHdrAdd		END
   tblHdrPad

   # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
   #
   # Fill in fudged data into the Binary Table proper.
   #
   #   o   The 24V array (2,3,4) is filled such that each hexadecimal digit will
   #       indicate an index (in row-major, 0-indexed).
   #
   #          .--------- slowest varying subscript (o)
   #          | .-------                           (n)     TDIMn = '(l,m,n)'
   #          | | .-----                           (m)
   #          | | | .--- fastest varying subscript (l)     subscript o is the
   #          V V V V                                      Binary Table row #
   #          0 0 0 0
   #

   set tblReg [regNew -type U8 [expr {$binNAXIS2 + $tblHeapGap + $tblHeapSize}] $binNAXIS1]
   regSetWithDbl $tblReg [tblASCIItoByte {*}]	;# Avoid explicit field padding

   binRecRow $tblReg 0	{   65} {   66} {7  0} {<abcdefghijklm>} {0x40490FDB} {0x400921FB 0x54442D18} { 66   67} \
			{    0    1     16   17     32   33 \
			   256  257    272  273    288  289 \
			   512  513    528  529    544  545 \
			   768  769    784  785    800  801 }  {<ab><cd>}  {127}  {}  {0   0}  {TF}  {127}  {4 47}  {}
   binRecRow $tblReg 1	{32832} {32833} {0  0} {>nopqrstuvwxyz<} {0x40490FDB} {0x400921FB 0x54442D18} { 50   51} \
			{ 4096 4097   4112 4113   4128 4129\
			  4352 4353   4368 4369   4384 4385\
			  4608 4609   4624 4625   4640 4641\
			  4864 4865   4880 4881   4896 4897}   {<wx><yz>}  {128}  {}  {5  33}  {FT}  {128}  {2 43}  {}
   binRecRow $tblReg 2	{32768} {32768} {8 17} {<0123456789!@#>} {0x402DF854} {0x4005BF0A 0x8B04919C} {-66  -67} \
			{ 8192 8193   8208 8209   8224 8225 \
			  8448 8449   8464 8465   8480 8481 \
			  8704 8705   8720 8721   8736 8737 \
			  8960 8961   8976 8977   8992 8993 }  {<01><23>}  {255}  {}  {5  38}  {TF}  { -1}  {7 51}  {}

   #
   # Fill in fudged data into the Binary Table heap.
   #

   binHeap Short $tblReg [expr {3 + $tblHeapGap}]  0 7 {  0  1  2  3  4  5  6 }
   binHeap Short $tblReg [expr {3 + $tblHeapGap}] 17 8 {  9 10 11 12 13 14 15 16 }
   binHeap ASCII $tblReg [expr {3 + $tblHeapGap}] 33 5 {hello}
   binHeap ASCII $tblReg [expr {3 + $tblHeapGap}] 38 5 {there}
   binHeap Byte  $tblReg [expr {3 + $tblHeapGap}] 43 2 { 125 126 }		   ;# W/ TSCAL/TZERO <==> {-3 -2}
   binHeap Byte  $tblReg [expr {3 + $tblHeapGap}] 47 4 { 127 128 129 130 }	   ;# W/ TSCAL/TZERO <==> {-1  0 1 2}
   binHeap Byte  $tblReg [expr {3 + $tblHeapGap}] 51 7 { 0 1 127 128 129 254 255 } ;# W/ TSCAL/TZERO <==> {-128 -127 -1 0 1 126 127}

   # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
   #
   # Write the FITS file out.
   #
   #   o   Get rid of the FITS header that Dervish generated and prepend ours.
   #

   regWriteAsFits $tblReg   $tmpFileSpec
   regDel         $tblReg

   set       fitsFile [open $fitsFileSpec  a]
   puts     $fitsFile $tblHdr nonewline			;# Prepend our header

   set       dataFile [open $tmpFileSpec   r]

   for {set fitsHdrFlush 1} {$fitsHdrFlush} { } {	;# Flush FITS logical
      loop fitsRead 0 $fitsRecSize $fitsLineSize {	;# ... record chunks,
         set fitsLine [read  $dataFile $fitsLineSize]	;# ... stopping with
         if {[regexp {^END +$} $fitsLine]} {		;# ... the record
            set fitsHdrFlush 0				;# ... containing END
         }
      }
   }

   copyfile $dataFile $fitsFile				;# Append data portion
   close    $fitsFile
   close    $dataFile

   exec rm  $tmpFileSpec

   #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}

#*******************************************************************************
