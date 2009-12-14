#
# Test RHL's line-orientated FITS binary Tables
#
source [envscan \$PHOTO_DIR]/etc/photoStartup.tcl
#
# Add all elements in a struct to a SCHEMATRANS, and return it. Any elements
# listed in the optional parameter $except are _not_ added. E.g.
#      schemaTransNewAll OBJECT1 {type next prev}
#
proc schemaTransNewAll {type {except {}}} {
   set trans [schemaTransNew]

   foreach el [schemaGetFromType $type] {
      set el [lindex $el 0]
      if {[lsearch $except $el] == -1} {
	 fitsBinSchemaTransEntryAdd $trans $el $el
      }
   }

   return $trans
}

proc fitsTblPrint {type {exec ""} {opt ""}} {
   set file "/tmp/binFits[pid]"
   set fd [fitsBinTblOpen $file w]
   exec rm $file
   fitsBinTblHdrWrite $fd $type
   eval fitsBinTblMachinePrint $type $exec $opt
   fitsBinTblClose $fd
}

proc write_tbl {{file tbl.fts} {mode w} {nline 1} {ncolor 2}} {
   set fd [fitsBinTblOpen $file $mode]

   if {$mode == "w"} {
      #
      # Create data to write
      #
      set objc [objcNew $ncolor]

      loop i 0 $ncolor {
	 set tmp [object1New]
	 handleSetFromHandle $objc.color<$i> &$tmp
	 handleDel $tmp;
	 
	 set tmp [regNew -name "Reg" 8 4]
	 handleSetFromHandle $objc.color<$i>->region &$tmp
	 handleDel $tmp
      
	 set tmp [maskNew -name "RegMask" 8 4]
	 handleSetFromHandle $objc.color<$i>->region->mask &$tmp
	 handleDel $tmp

	 set tmp [maskNew -name "ObjMask" 8 4]
	 handleSetFromHandle $objc.color<$i>->mask &$tmp
	 handleDel $tmp
      
	 set tmp [dgpsfNew]
	 handleSetFromHandle $objc.color<$i>->dgpsf &$tmp
	 handleDel $tmp
      
	 set tmp [handleBindNew [exprGet $objc.color<$i>->region] REGION]
	 regIntSetVal $tmp 1
	 handleDel $tmp
	 
	 handleSet $objc.color<$i>->npix [expr ($i+1)*10];
	 handleSet $objc.color<$i>->skySig 1234
      }

      set tmp [maskNew -name "MasterM" 4 8]
      handleSetFromHandle $objc.master_mask &$tmp
      handleDel $tmp
      maskClear *$objc.master_mask
      if 1 {
#
# write table
#
      fitsBinTblHdrWrite $fd "OBJC"
      handleSet $objc.parent_id 10;
      if 0 {
	 loop i 0 $nline {
	    fitsBinTblRowWrite $fd $objc;
	 }
      } else {
	 set chain [chainNew OBJC]
	 set curs [chainCursorNew $chain]
	 loop i 0 $nline {
	    chainElementAddByCursor $chain $objc $curs
	 }
	 fitsBinTblChainWrite $fd $chain
	 chainCursorDel $chain $curs
	 chainDel $chain
      }
      
      fitsBinTblEnd $fd;
      objcDel $objc;
   }
#
# Create more data
#
      set reg [regNew -name "rhl" 10 4 -type S16]
      set mask [maskNew -name mask 10 4]
      handleSet (long)$reg.mask [exprGet &$mask]
      handleDel $mask
#
# And write it
#
      fitsBinTblHdrWrite $fd "REGION"
      fitsBinTblRowWrite $fd $reg;
      fitsBinTblEnd $fd;

      regDel $reg;

      fitsBinForgetSchemaTrans NULL;
   }

   fitsBinTblClose $fd
}

#
# Carry our tests
#
set failed 0

set file tbl.fts

echo Writing a table
declare_schematrans 2
if {[catch {write_tbl $file}]} {
   echo "TEST-ERR: failed to write binary table"
   set failed 1
}

echo Reading it back with fitsRead
set tblcol [tblColNew]
if {[catch {fitsRead $tblcol $file -hdu 1}]} {
   echo "TEST-ERR: failed to read first binary table in $file"
   set failed 1
}

if {[catch {fitsRead $tblcol $file -hdu 2}]} {
   echo "TEST-ERR: failed to read second binary table in $file"
   set failed 1
}

#
# Clean up
#
tblColDel $tblcol
exec rm -f $file

#
# return error/success
#

global interactive
if {[catch {set interactive}]} {
   exit $failed
}
