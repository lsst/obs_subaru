#
# Test the photo FITS binary table package
#
source [envscan \$PHOTO_DIR]/etc/photoStartup.tcl

set file foo.fts;			# file to write
set ncolor 2;				# number of colours in OBJCs

set null [handleNew]

proc declare_schematrans {ncolor} {
   global ROWS
#
# OBJMASK
#
   set trans [schemaTransNewAll OBJMASK {id data size s user}]
   fitsBinSchemaTransEntryAdd $trans s s -fitsData heap

   fitsBinDeclareSchemaTrans $trans OBJMASK
   handleDel $trans
#
# OBJC
#
   set trans [schemaTransNewAll OBJC {children color parent peaks sibbs test}]
   fitsBinSchemaTransEntryAdd $trans color color -dimen $ncolor

   fitsBinDeclareSchemaTrans $trans OBJC
   handleDel $trans
#
# OBJC_IO
#
   set trans [schemaTransNewAll OBJC_IO {aimage test}]

   fitsBinDeclareSchemaTrans $trans OBJC_IO
   handleDel $trans
#
# ATLAS_IMAGE. Note that ncolor is treated specially in the binary table code.
#
   set trans [schemaTransNew]
   fitsBinSchemaTransEntryAdd $trans id id
   fitsBinSchemaTransEntryAdd $trans ncolor ncolor -fitsData heap

   fitsBinDeclareSchemaTrans $trans ATLAS_IMAGE
   handleDel $trans
#
# OBJECT1
#
   set trans [schemaTransNewAll OBJECT1 {mask flags2 peaks model}]
#   fitsBinSchemaTransEntryAdd $trans filter filter -fitsData string
      
   fitsBinDeclareSchemaTrans $trans OBJECT1
   handleDel $trans
#
# MASK
#
   set trans [schemaTransNew]
   fitsBinSchemaTransEntryAdd $trans MNAME name -fitsData heap
   fitsBinSchemaTransEntryAdd $trans MNROW nrow
   fitsBinSchemaTransEntryAdd $trans MNCOL ncol
   fitsBinSchemaTransEntryAdd $trans MROWS rows -fitsData heap
   fitsBinSchemaTransEntryAdd $trans MROW0 row0
   fitsBinSchemaTransEntryAdd $trans MCOL0 col0

   fitsBinDeclareSchemaTrans $trans MASK
   handleDel $trans
#
# REGION
#
   set trans [schemaTransNew]
   fitsBinSchemaTransEntryAdd $trans RNAME name -fitsData heap
   fitsBinSchemaTransEntryAdd $trans RNROW nrow
   fitsBinSchemaTransEntryAdd $trans RNCOL ncol
   fitsBinSchemaTransEntryAdd $trans RTYPE type
   fitsBinSchemaTransEntryAdd $trans RROWS $ROWS -fitsData heap
#   fitsBinSchemaTransEntryAdd $trans RMASK mask
   fitsBinSchemaTransEntryAdd $trans RROW0 row0
   fitsBinSchemaTransEntryAdd $trans RCOL0 col0

   fitsBinDeclareSchemaTrans $trans REGION
   handleDel $trans
}

#
# Check that the OBJC/OBJECT1 and OBJC_IO structures are in sync; we both
# check that all members in OBJC_IO correspond to (and have the same type
# as) members of OBJC or OBJECT1, then check that there are no members of
# OBJC or OBJECT1 that don't appear in OBJC_IO. As, in fact, there are some
# such members, provision is made for exceptions to be made.
#
proc check_objc_objcIo {} {
   set ai_s [schemaGetFromType ATLAS_IMAGE]
   set object1_s [schemaGetFromType OBJECT1]
   set objc_s [schemaGetFromType OBJC]
# note that output_s is the union of an OBJC_IO and an ATLAS_IMAGE
   set output_s [concat \
		     [schemaGetFromType OBJC_IO] \
		     [schemaGetFromType ATLAS_IMAGE]]

   set errs 0;				# number of errors detected

   set ncolor 5;			# dimension of arrays in OBJC_IO
#
# First check that all fields in the OBJC_IO correspond to fields in
# OBJC (for scalars) or OBJECT1 (for arrays of size $ncolor)
#
   foreach el $output_s {
      set name [lindex $el 0]
      regexp \
	  {((const )?(struct )?[a-zA-Z_][a-zA-Z_0-9]*\**)((\[[0-9]+\])*)} \
	  [lindex $el 1] {} type dum1 dum2 dimen
      if {[regexp "\\\[$ncolor\\\](.*)" $dimen {} dimen]} {
	 # an array of length $ncolor, so it should come from OBJECT1
	 set type "$type$dimen"
	 if [catch {keylget object1_s $name}] {
	    if {$name == "drow" || $name == "dcol" || \
		    $name == "regmask" || $name == "pix"} {
	       ;			# not in OBJECT1
	    } else {
	       echo TEST-ERR: $name is in OBJC_IO/ATLAS_IMAGE but not in OBJECT1
	       incr errs
	    }
	 } else {
	    # found it, but is it of the right type?
	    set obj1_type [keylget object1_s $name]
	    if {$type != $obj1_type} {
	       echo TEST-ERR: found $name in OBJECT1 with wrong type \
		   ($obj1_type not $type)
	       incr errs
	    }
	 }
      } else {
	 # a scalar, so it should come from OBJC or ATLAS_IMAGE
	 set type "$type$dimen"
	 if {[catch {keylget objc_s $name}] && [catch {keylget ai_s $name}]} {
	    if {$name == "master_mask" || $name == "test" || \
		    $name == "shallow_copy" || $name == "npix"} {
	       ;			# they really aren't in an OBJC
	    } else {
	       regsub {^objc_} $name {} name
	       if [catch {keylget objc_s $name}] {
		  echo "TEST-ERR:" \
		      "$name is in OBJC_IO/ATLAS_IMAGE but not in OBJC"
		  incr errs
	       }
	    }
	 } else {
	    # found it, but is it of the right type?
	    if ![keylget objc_s $name objc_type] {
	       keylget ai_s $name objc_type
	    }
	    if {$name == "id"} {	# OBJC is const; OBJC_IO isn't
	       regsub {^const } $objc_type {} objc_type
	    }
	    if {$type != $objc_type} {
	       if {$name != "parent"} {
		  echo TEST-ERRS: found $name in OBJC with wrong type \
		      ($objc_type not $type)
		  incr errs
	       }
	    }
	 }
      }
   }
#
# Now check that all fields in OBJCs occur in OBJC_IOs or ATLAS_IMAGEs
#
   set except [list children color flags flags3 model nchild peaks sibbs];# these fields aren't saved

   foreach el $objc_s {
      set name [lindex $el 0]
      if {[lsearch $except $name] >= 0} {
	 continue;
      }
      
      if [catch {keylget output_s $name}] {
	 echo TEST-ERR: $name is in OBJC but not in OBJC_IO/ATLAS_IMAGE
	 incr errs
      }
   }
#
# and check OBJECT1s too
#
   set except [list aratio test majaxis comp model npix peaks region profMed \
		  chisq_star nu_star chisq_deV nu_deV chisq_exp nu_exp flags3\
		  satur_DN]

   foreach el $object1_s {
      set name [lindex $el 0]
      if {[lsearch $except $name] >= 0} {
	 continue;
      }
      
      if [catch {keylget output_s $name}] {
	 echo TEST-ERR: $name is in OBJECT1 but not in OBJC_IO/ATLAS_IMAGE
	 incr errs
      }
   }

   return $errs
}

#
# Here are the OBJC read/write procs
#
proc write_tbl {file ncolor} {
   global ROWS
   
   set objc [objcNew $ncolor]

   set omask [objmaskNew -nspan 1]
   handleSet $omask.s<0>.y 10
   handleSet $omask.s<0>.x1 100
   handleSet $omask.s<0>.x2 200
   handleSet $omask.npix 101
   handleSet $omask.nspan 1
   handleSetFromHandle $objc.aimage->master_mask &$omask
   handleSetFromHandle $objc.aimage->npix $omask.npix
   handleDel $omask

   loop i 0 $ncolor {
      set obj [object1New]

      set reg [regNew 10 20]; regIntSetVal $reg 127
      handleSetFromHandle $objc.aimage->pix<$i> $reg.$ROWS
      handleSetFromHandle $obj.region &$reg; handleDel $reg

      handleSet $obj.rowc [expr $i+1.234]

      set reg [regNew 1 1]; regIntSetVal $reg 127
      handleSetFromHandle $obj.model &$reg; handleDel $reg
      
      handleSetFromHandle $objc.color<$i> &$obj; handleDel $obj

   }

   set fd [fitsBinTblOpen $file w]
   set hdr [hdrNew]
   hdrInsWithAscii $hdr "TEST" "Hello World" "Test writing headers"
   
   fitsBinTblHdrWrite $fd "OBJC" -hdr $hdr
   hdrFreeAll $hdr; hdrDel $hdr

   loop i 0 2 {
      fitsBinTblRowWrite $fd $objc
   }
   fitsBinTblEnd $fd
#
# write a second table to the same file
#
   fitsBinTblHdrWrite $fd "OBJC"
   fitsBinTblRowWrite $fd $objc
   fitsBinTblEnd $fd

   fitsBinTblClose $fd

   return $objc;
}

proc read_tbl {file} {
   set fd [fitsBinTblOpen $file r]
   set hdr [hdrNew]
   set nrow [fitsBinTblHdrRead $fd "OBJC" -hdr $hdr]
   hdrGetAsAscii $hdr "TEST"
   hdrFreeAll $hdr; hdrDel $hdr
   #
   # find the value of objc->ncolor
   #
   set objc [objcNew 5]
   catch {			# ignore errors because regions/mask are NULL
      fitsBinTblRowRead $fd $objc
   }
   fitsBinTblRowUnread $fd
   set ncolor [exprGet $objc.ncolor]
   #
   # And read the data
   #
   loop i 0 $nrow {
      if {[exprGet $objc.color<0>] != 0x0} {
	 loop c 0 $ncolor {
	    set reg [handleBindFromHandle [handleNew] *$objc.color<$c>->region]
	    handleDel $reg
	 }
      }
      objcDel $objc
      
      set objc [objcNew $ncolor]
      loop i 0 $ncolor {
	 set obj [object1New]
	 set reg [regNew 0 0]
	 handleSetFromHandle $obj.region &$reg; handleDel $reg
	 handleSetFromHandle $objc.color<$i> &$obj; handleDel $obj
      }
      fitsBinTblRowRead $fd $objc
   }
   
   fitsBinTblClose $fd

   return $objc;
}

#
# Here are the OBJC_IO read/write procs
#
proc write_io_tbl {file objc} {
   set objcIo [objcIoNewFromObjc $objc]
#
# set ATLAS_IMAGE fields through some hackery
#
   set mmask [handleBindFromHandle [handleNew] *$objc.aimage->master_mask]
   set reg [regNew 300 300]; regIntSetVal $reg 1234
   loop i 0 2 {
      objmaskSetFromReg $mmask $reg
      handleSetFromHandle $objcIo.aimage->master_mask &$mmask
      handleSetFromHandle $objcIo.aimage->pix<$i> $mmask.data
      handleSet $mmask.data 0
   }
   handleDel $mmask; regDel $reg

   set OBJfd [fitsBinTblOpen OBJ$file w]
   set ATLfd [fitsBinTblOpen ATL$file w]
   fitsBinTblHdrWrite $OBJfd "OBJC_IO"
   fitsBinTblHdrWrite $ATLfd "ATLAS_IMAGE"
   
   loop i 0 2 {
      fitsBinTblRowWrite $OBJfd $objcIo
      fitsBinTblRowWrite $ATLfd *$objcIo.aimage
   }
   fitsBinTblEnd $OBJfd; fitsBinTblEnd $ATLfd
   fitsBinTblClose $OBJfd; fitsBinTblClose $ATLfd

   return $objcIo;
}

proc read_io_tbl {file} {
   set OBJfd [fitsBinTblOpen OBJ$file r]
   set ATLfd [fitsBinTblOpen ATL$file r]
   set OBJnrow [fitsBinTblHdrRead $OBJfd "OBJC_IO"]
   set ATLnrow [fitsBinTblHdrRead $ATLfd "ATLAS_IMAGE"]

   if {$OBJnrow != $ATLnrow} {
      echo TEST-ERR: Different number of rows in OBJC_IO and ATLAS_IMAGE tables
      return ""
   }
   #
   # Find ncolor
   #
   set objcIo [objcIoNew]
   fitsBinTblRowRead $OBJfd $objcIo
   fitsBinTblRowUnread $OBJfd
   set ncolor [exprGet $objcIo.ncolor]
   objcIoDel $objcIo -deep
   #
   # Read the data
   #
   loop i 0 $OBJnrow {
      if {$i > 0} {
	 objcIoDel $objcIo -deep
      }

      set objcIo [objcIoNew]
      fitsBinTblRowRead $OBJfd $objcIo
      fitsBinTblRowRead $ATLfd *$objcIo.aimage
   }
   
   fitsBinTblClose $OBJfd; fitsBinTblClose $ATLfd

   return $objcIo;
}

###############################################################################
#
# Comparison functions
#
# compare these fields
#
set els [list \
	     rowc \
	     rowc \
	     colc \
	     sky \
	     skyErr \
	     Q \
	     U \
	     type \
	     fiberCounts \
	     psfCounts \
	     fiberCounts \
	     petroCounts \
	     petroRad \
	     nprof \
	     profMean<0> \
	     profErr<0> \
	    ]

proc comp_objc {objc0 objc1} {
   global els ROWS
   set els_obj1 [concat $els [list id npix majaxis aratio]]
   
   if {[exprGet $objc0.ncolor] != [exprGet $objc1.ncolor]} {
      echo "TEST-ERR: ncolor differs: " \
	  "[exprGet $objc0.ncolor] != [exprGet $objc1.ncolor]"
      return 1
   }

   set diffs 0;				# number of differences
   set mm0 [handleBindFromHandle [handleNew] *$objc0.aimage->master_mask]
   set mm1 [handleBindFromHandle [handleNew] *$objc1.aimage->master_mask]
   set diffs [expr $diffs+[comp_objmask $mm0 $mm1]]
   handleDel $mm0; handleDel $mm1

   loop c 0 [exprGet $objc0.ncolor] {
      set obj0 [handleBindFromHandle [handleNew] *$objc0.color<$c>]
      set obj1 [handleBindFromHandle [handleNew] *$objc1.color<$c>]
      foreach el $els_obj1 {
	 if {[exprGet $obj0.$el] != [exprGet $obj1.$el]} {
	    echo "TEST-ERR: color<$c>: $el differs: " \
		"[exprGet $obj0.$el] != [exprGet $obj1.$el]"
	    incr diffs
	 }
      }

      if {[exprGet ((short**)$obj0.region->$ROWS)<2><3>] !=
	  [exprGet ((short**)$obj1.region->$ROWS)<2><3>]} {
	 echo "TEST-ERR: color<$c>: region->$ROWS<2><3> differs: " \
	     "[exprGet ((short**)$obj0.region->$ROWS)<2><3>] != " \
	     "[exprGet ((short**)$obj1.region->$ROWS)<2><3>]"
	 incr diffs
      }

      handleDel $obj0; handleDel $obj1;
   }

   return $diffs;
}

proc comp_objcIo {objcIo0 objcIo1} {
   global els
   
   if {[exprGet $objcIo0.ncolor] != [exprGet $objcIo1.ncolor]} {
      echo "TEST-ERR: ncolor differs: " \
	  "[exprGet $objcIo0.ncolor] != [exprGet $objcIo1.ncolor]"
      return 1
   }

   set diffs 0;				# number of differences

   set ai0 [handleBindFromHandle [handleNew] *$objcIo0.aimage]
   set ai1 [handleBindFromHandle [handleNew] *$objcIo1.aimage]
   set diffs [expr $diffs+[comp_aimage $ai0 $ai1]]
   handleDel $ai0; handleDel $ai1
   
   foreach el $els {
      loop c 0 [exprGet $objcIo0.ncolor] {
	 if {[exprGet $objcIo0.$el<$c>] != [exprGet $objcIo1.$el<$c>]} {
	    echo "TEST-ERR: $el<$c> differs: " \
	       "[exprGet $objcIo0.$el<$c>] != [exprGet $objcIo1.$el<$c>]"
	    incr diffs
	 }
      }
   }

   return $diffs;
}

proc comp_aimage {ai0 ai1} {
   set diffs 0

   set mm0 [handleBindFromHandle [handleNew] *$ai0.master_mask]
   set mm1 [handleBindFromHandle [handleNew] *$ai1.master_mask]
   set diffs [expr $diffs+[comp_objmask $mm0 $mm1]]
   handleDel $mm0; handleDel $mm1

   if {[exprGet $ai0.ncolor] != [exprGet $ai1.ncolor]} {
      echo "TEST-ERR: ATLAS_IMAGE->ncolor differs: " \
	  "[exprGet $ai0.ncolor] != [exprGet $ai1.ncolor]"
      incr diffs;
   }

   loop c 0 [exprGet $ai0.ncolor] {
      if {[exprGet $ai0.pix<$c><0>] != [exprGet $ai1.pix<$c><0>]} {
	 echo "TEST-ERR: ATLAS_IMAGE->pix<$c><0> differs: " \
	     "[exprGet $ai0.pix<$c><0>] != [exprGet $ai1.pix<$c><0>]"
	 incr diffs
      }
   }

   return $diffs
} 

proc comp_objmask {mm0 mm1} {
   set diffs 0
   
   foreach el [list nspan row0 col0 rmin rmax cmin cmax] {
      if {[exprGet $mm0.$el] != [exprGet $mm1.$el]} {
	 echo "TEST-ERR: OBJMASK: $el differs: " \
	     "[exprGet $mm0.$el] != [exprGet $mm1.$el]"
	 incr diffs
      }
   }
   foreach el [list y x1 x2] {
      if {[exprGet $mm0.s<0>.$el] != [exprGet $mm1.s<0>.$el]} {
	 echo "TEST-ERR: OBJMASK: s<0>.$el differs: " \
	     "[exprGet $mm0.s<0>.$el] != [exprGet $mm1.s<0>.$el]"
	 incr diffs
      }
   }

   return $diffs
}
#
# Actually run tests
#
set status 0

declare_schematrans $ncolor

echo Checking consistency between OBJC/OBJECT1s and OBJC_IO/ATLAS_IMAGEs
if {[check_objc_objcIo] != "0"} {
   echo TEST-ERR: found inconsistency between \
       OBJC/OBJECT1s and OBJC_IO/ATLAS_IMAGEs
   incr status
}

echo Writing OBJC table
if {[set objc0 [write_tbl $file $ncolor]] == ""} {
   echo "TEST-ERR: Failed to write OBJC table"
   incr status
}

echo Reading OBJC table back
if {[set objc1 [read_tbl $file]] == ""} {
   echo "TEST-ERR: Failed to read OBJC table"
   incr status
}

echo Comparing answers
if {[comp_objc $objc0 $objc1]} {
   echo "TEST-ERR: Re-read OBJCs differ from input OBJCs"
   incr status
}

#
# Now test writing OBJCs via OBJC_IO/ATLAS_IMAGEs
#
echo Converting OBJC to OBJC_IO/ATLAS_IMAGE and writing it out
if {[set objcIo0 [write_io_tbl $file $objc0]] == ""} {
   echo "TEST-ERR: Failed to write OBJC_IO table"
   incr status
}

echo Reading OBJC_IO/ATLAS_IMAGE tables back
if {[set objcIo1 [read_io_tbl $file]] == ""} {
   echo "TEST-ERR: Failed to read OBJC_IO table"
   incr status
}

echo Comparing answers for OBJC_IO/ATLAS_IMAGESs
if {[comp_objcIo $objcIo0 $objcIo1]} {
   echo "TEST-ERR: Re-read OBJC_IO/ATLAS_IMAGEs differ from inputs"
   incr status
}
#
# Clean up
#
fitsBinForgetSchemaTrans NULL;

exec rm -f $file OBJ$file ATL$file
foreach obj [list $objc0 $objc1] {
   loop i 0 $ncolor {
      set reg [handleBindFromHandle [handleNew] *$obj.color<$i>->region]
      handleDel $reg
   }
   objcDel $obj
}
objcIoDel $objcIo0
objcIoDel $objcIo1 -deep

if $status {
   error "Failed some tests"
}
