######################################################################
# procs to turn ascii catalogs into lists of KNOWNOBJs that can 
# then be written as fits tables.
######################################################################

proc gsctable {catalog} {

   set kolist [chainNew KNOWNOBJ]
   set file [open $catalog]
   
   while {[gets $file line] >= 0} {
      scan $line "%f %f %f" ra dec mag
      set ko [knownobjNew] 
      handleSet $ko.class STAR
      handleSet $ko.ra $ra
      handleSet $ko.dec $dec
      handleSet $ko.mag $mag
#      handleSet $ko.catalog "HST GSC"
      chainElementAddByPos $kolist $ko TAIL AFTER
      handleDel $ko
   }
   close $file
   return $kolist
}


proc kowrite {kolist file} {
   
   set koTbl [knownobjTbl]
   set tblcol [schemaToTbl $kolist $koTbl -schemaName KNOWNOBJ]
   fitsWrite $tblcol $file -binary -pdu MINIMAL 
   chainDestroy $kolist knownobjDel
   return $tblcol
}


proc fake_trans {} {
   set trans [transNew] 
   handleSet $trans.a 3.7151
   handleSet $trans.b 0.
   handleSet $trans.c 0.
   handleSet $trans.d 5.37e-7
   handleSet $trans.e 0.
   handleSet $trans.f .0015
   
   set transtbl [transTbl]
   set tblcol [schemaToTbl $trans $transtbl -schemaName TRANS]
   fitsWrite $tblcol surveytrans.fit -binary -pdu MINIMAL
   transDel $trans
}

proc koread {file} {
   set catalog [chainNew KNOWNOBJ]
   set strans [knownobjTbl]
   set tblcol [handleNewFromType TBLCOL]
   fitsRead $tblcol $file -binary -hdu 1
   tblToSchema $tblcol $catalog $strans -schemaName KNOWNOBJ \
       -proc knownobjNew
   schemaTransDel $strans
   handleDelFromType $tblcol
   handleDel $tblcol
   return $catalog
}

proc fake_ko_for_cutting {file} {
   set row 960.2
#   set col 872.7
   set col 832.7
   set nrow 75
   set ncol 50

   # make a known object
   echo "Making knownobject"
   set ko [knownobjNew]
   handleSet $ko.row $row
   handleSet $ko.col $col
   handleSet $ko.nrow $nrow
   handleSet $ko.ncol $ncol
   echo [sp $ko]

# put it on a chain
   set kobjs [chainNew KNOWNOBJ]
   chainElementAddByPos $kobjs $ko TAIL AFTER
   schema2Fits $kobjs $file
}

proc find_kobjc {chain} {
   set crsr [chainCursorNew $chain]
   set kos [chainNew OBJC]
   set file [open foo w]
   while {[set el [chainWalk $chain $crsr]] != ""}  {
      puts $file [sp $el]
      set class [exprGet $el.obj_class]
      echo $class
      if {$class == "(enum) FAINT_KNOWNOBJ"} {
	 chainElementAddByPos $kos [chainElementRemByCursor $chain $crsr] \
	     TAIL AFTER
      }
   }
   chainCursorDel $chain $crsr
   close $file
   return $kos
}
