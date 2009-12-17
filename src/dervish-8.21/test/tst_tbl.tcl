# The following script tests TBLCOL/Schema conversion routines as well as 
# TBLCOL/FITS routines.
#
# Wei Peng, 05/04/94

#################################################################
#								#
# Verbs that are tested in this script are :			#
#	schemaTransNew						#
#	schemaTransEntryAdd					#
#	schemaTransEntryClearAll				#
#	schemaTransEntryImport					#
#	fitsRead						#
#	fitsWrite						#
#	schemaToTbl						#
#	tblToSchema						#
#								#
# Testing is in following areas : 				#
#	basic operations, 					#
#	array specification/manipulations, 			#
#	muilti-level indirection,				#
# 	different kinds of container, 				#
#	portable enumeration, 					#
#	heap capability,					#
#	object recycle.						#
#								#
#################################################################

# test if basic operations of schemaToTbl or tblToSchema are correct

proc testBasics { xtbl } {
  
   set region [regNew -mask]
   handleSet $region.nrow 20
   handleSet $region.ncol 30
   handleSet $region.mask->ncol 40
   handleSet $region.mask->name BillClinton

# general elementary type

   set status [catch { schemaTransEntryAdd $xtbl name nrow nrow int } err]
   if { $status == 1 } {
      echo TEST-ERR: schemaTransEntryAdd fails in basics test 
      echo TEST-ERR: $err
   }
   set status [catch { schemaTransEntryAdd $xtbl name ncol ncol int } err]
   if { $status == 1 } {
      echo TEST-ERR: schemaTransEntryAdd fails in basics test
      echo TEST-ERR: $err
   }

# indirection

   set status [catch { schemaTransEntryAdd $xtbl name mncol mask struct } err]
   if { $status == 1 } {
      echo TEST-ERR: schemaTransEntryAdd fails in basics test
      echo TEST-ERR: $err
   }
   set status [catch { schemaTransEntryAdd $xtbl cont mask ncol int} err]
   if { $status == 1 } {
      echo TEST-ERR: schemaTransEntryAdd fails in basics test
      echo TEST-ERR: $err
   }

# indirection + string

   set status [catch {schemaTransEntryAdd $xtbl name verb mask struct} err]
   if { $status == 1 } {
      echo TEST-ERR: schemaTransEntryAdd fails in indirection basics test
      echo TEST-ERR: $err
   }
   set status [catch {schemaTransEntryAdd $xtbl cont mask name string -dimen 20} err]
   if { $status == 1 } {
      echo TEST-ERR: schemaTransEntryAdd in string basics test
      echo TEST-ERR: $err
   }

   set status [catch {set table [schemaToTbl $region $xtbl]} err]
   if { $status == 1 } {
      echo TEST-ERR: schemaToTbl fails in basics test
      echo TEST-ERR: $err
   }
   set status [catch {fitsWrite $table junk.fits -ascii -pdu MINIMAL} err]
   if { $status == 1 } {
      echo TEST-ERR: fitsWrite fails in basics test
      echo TEST-ERR: $err
   }

# read back

   set ntable [handleNewFromType TBLCOL]
   set status [catch {fitsRead $ntable junk.fits -hdu 1} err]
   if { $status == 1 } {
      echo TEST-ERR: fitsRead fails in basics test
      echo TEST-ERR: $err
   }
   
   set nregion [regNew -mask]
   set status [catch {tblToSchema $ntable $nregion $xtbl} err]
   if { $status == 1 } {
      echo TEST-ERR: tblToSchema fails in basics test
      echo TEST-ERR: $err
   }

# now get the values previously set

   set val [exprGet $nregion.nrow]
   if { [expr $val] != 20 } {
        echo TEST-ERR: should get 20 not [expr $val]
   }

   set val [exprGet $nregion.ncol]
   if { [expr $val] != 30 } {
        echo TEST-ERR: should get 30 not [expr $val]
   }
   set val [exprGet $nregion.mask->ncol]
   if { [expr $val] != 40 } {
        echo TEST-ERR: should get 40 not [expr $val]
   }
   set val [exprGet $nregion.mask->name]
   if { [string compare $val "BillClinton"] != 0 } {
        echo TEST-ERR: should get BillClinton, not $val
   }

   set status [catch {schemaTransEntryClearAll $xtbl} err]
   if { $status == 1 } {
       echo TEST-ERR:  can't clear the translation table
       echo TEST-ERR: $err
   }
   exec rm junk.fits
   regDel $nregion
   regDel $region
   tblColDel $table
   tblColDel $ntable
   return $xtbl
}
   


#
# test if array specification operates correctly
#

proc testArray { xtbl } {

   set region [regNew -mask]
   handleSet $region.nrow 20
   handleSet $region.ncol 30
   handleSet $region.mask->ncol 40

# now schemaTransEntryAdd should behave well

   schemaTransEntryAdd $xtbl name int<0> nrow int
   schemaTransEntryAdd $xtbl name int<1> ncol int
   schemaTransEntryAdd $xtbl name int<2> mask struct -proc maskNew
   schemaTransEntryAdd $xtbl cont mask ncol int

   set status [catch {set table [schemaToTbl $region $xtbl]} err ]
   if { $status == 1 } {
        echo TEST-ERR : schemaToTbl fails in array test
        echo TEST-ERR: $err
   }

   set status [catch {fitsWrite $table junk.fits -binary -pdu MINIMAL} err ]
   if { $status == 1 } {
        echo TEST-ERR : fitsWrite fails in array test
        echo TEST-ERR: $err
   }

# read back

   set ntable [handleNewFromType TBLCOL]
   set status [catch {fitsRead $ntable junk.fits -hdu 1} err ]
   if { $status == 1 } {
        echo TEST-ERR : fitsRead fails in array test
        echo TEST-ERR: $err
   }
   
   set nregion [regNew]
   set status [catch {tblToSchema $ntable $nregion $xtbl} err ]
   if { $status == 1 } {
        echo TEST-ERR : tblToSchema fails in array test
        echo TEST-ERR: $err
   }

# now get the values previously set

   set val [exprGet $nregion.nrow]
   if { [expr $val] != 20 } {
        echo TEST-ERR: should get 20 not [expr $val]
   }

   set val [exprGet $nregion.ncol]
   if { [expr $val] != 30 } {
        echo TEST-ERR: should get 30 not [expr $val]
   }
   set val [exprGet $nregion.mask->ncol]
   if { [expr $val] != 40 } {
        echo TEST-ERR: should get 40 not [expr $val]
   }

   schemaTransEntryClearAll $xtbl
   exec rm junk.fits
   regDel $region
   regDel $nregion
   tblColDel $table
   tblColDel $ntable
   return $xtbl
}
   
#
#  test multi-indirection and container
#

proc testMulti { xtbl } {

    set container [chainNew REGION]
    loop i 0 2 {
      set region [regNew -mask]
      handleSet $region.nrow 2$i
      chainElementAddByPos $container $region
    }

    # build a translation table and generate a FITS file

    schemaTransEntryAdd $xtbl name int<0> nrow int
    set status [catch {set table [schemaToTbl $container \
                $xtbl -schemaName REGION]} err]
    if { $status == 1 } {
         echo TEST-ERR: schemaToTbl fails in multi-indirection test
         echo TEST-ERR: $err
    }

    set status [catch {fitsWrite $table junk.fits -binary -pdu MINIMAL} err]
    if { $status == 1 } {
         echo TEST-ERR: fitsWrite fails in multi-indirection test
         echo TEST-ERR: $err
    }

    # redo the table and read back for a REGINFO schema

    schemaTransEntryClearAll $xtbl
    schemaTransEntryAdd $xtbl name nrow subRegs<0>  struct -proc "regNew -mask" -dimen 3
    schemaTransEntryAdd $xtbl cont subRegs<0> mask    struct 
    schemaTransEntryAdd $xtbl cont mask       nrow    int 
    schemaTransEntryAdd $xtbl name nrow subRegs<1>  struct -proc "regNew -mask" -dimen 3
    schemaTransEntryAdd $xtbl cont subRegs<1>  mask    struct 
    schemaTransEntryAdd $xtbl cont mask       nrow    int 
    schemaTransEntryAdd $xtbl name nrow subRegs<2>  struct -proc "regNew -mask" -dimen 3
    schemaTransEntryAdd $xtbl cont subRegs<2> mask    struct
    schemaTransEntryAdd $xtbl cont mask       nrow    int

    tblColDel $table
    set table [handleNewFromType TBLCOL]
    set status [catch {fitsRead $table junk.fits -hdu 1} err]
    if { $status == 1 } {
         echo TEST-ERR: fitsRead fails in multi-indirection test
         echo TEST-ERR: $err
    }
   
    # clean up first
    loop i 0 2 {
	regDel [chainElementRemByPos $container 0]
    }
    chainDel $container

   # convert table to schema

    set container [handleNewFromType ARRAY]
    set status [catch {tblToSchema $table $container $xtbl -schemaName REGINFO} err]
    if { $status == 1 } {
         echo TEST-ERR: tblToSchema fails in multi-indirection test
         echo TEST-ERR: $err
    }

    schemaTransEntryClearAll $xtbl
    exec rm junk.fits
    tblColDel $table
    handleDelFromType $container
    return $xtbl
}
    
#
#  test portable enum
#

proc testEnum { xtbl } {

     set region [regNew]
     handleSet $region.type TYPE_U32

     # build translation table

     schemaTransEntryAdd $xtbl name enum_type  type  enum

     # write fits

     set status [catch {set table [schemaToTbl $region $xtbl]} err ]
     if { $status == 1 } {
         echo TEST-ERR: schemaToTbl fails in portable enum test
         echo TEST-ERR: $err
     }
     set status [catch {fitsWrite $table junk.fits -binary -pdu MINIMAL} err ]
     if { $status == 1 } {
         echo TEST-ERR: fitsWrite fails in portable enum test
         echo TEST-ERR: $err
     }
     
     # read the data back 

     set nregion [regNew]
     tblColDel $table
     set table [handleNewFromType TBLCOL]
     set status [catch {fitsRead $table junk.fits -hdu 1} err ]
     if { $status == 1 } {
         echo TEST-ERR: fitsRead fails in portable enum test
         echo TEST-ERR: $err
     }

     set status [catch {tblToSchema $table $nregion $xtbl} err ]
     if { $status == 1 } {
         echo TEST-ERR: tblToSchema fails in portable enum test
         echo TEST-ERR: $err
     }

     # check for data validity

     set val [exprGet $nregion.type]
     if { [expr $val] != "(enum) TYPE_U32" } {
       echo TEST-ERR: should get enum value TYPE_U32, not [expr $val]
     }

     schemaTransEntryClearAll $xtbl
     exec rm junk.fits
     regDel $region
     regDel $nregion
     tblColDel $table
     return $xtbl
}
     

#
#  test heap  (use char in heap )
#

proc testHeap { xtbl } {
   global nregion
   set region [regNew -mask]
   handleSet $region.name "Hello"
   handleSet $region.mask->name "World"

   # set a few other values

   handleSet $region.nrow       20
   handleSet $region.mask->nrow 20

   # build translation table and generate fits

#   schemaTransEntryAdd $xtbl name data1 name heap \
#         -heaplength [string length [exprGet $region.name]] -heaptype char
   schemaTransEntryAdd $xtbl name data1 name heap \
          -heaplength strlen(name) -heaptype string
   schemaTransEntryAdd $xtbl name data2 mask struct
#   schemaTransEntryAdd $xtbl cont mask  name heap \
#          -heaplength [string length [exprGet $region.mask->name]] -heaptype char
   schemaTransEntryAdd $xtbl cont mask  name heap \
           -heaplength strlen(mask->name) -heaptype string
   schemaTransEntryAdd $xtbl name nrow1 nrow int
   schemaTransEntryAdd $xtbl name nrow2 mask struct
   schemaTransEntryAdd $xtbl cont mask  nrow int

   set status [catch {set table [schemaToTbl $region $xtbl]} err]
   if { $status == 1 } {
       echo TEST-ERR: schemaToTbl fails in heap test
       echo TEST-ERR: $err
   }
   set status [catch {fitsWrite $table junk.fits -binary \
             -pdu MINIMAL -standard F} err]
   if { $status == 1 } {
       echo TEST-ERR: fitsWrite fails in heap test
       echo TEST-ERR: $err
   }

   # read back in

   set nregion [regNew -mask]
   tblColDel $table
   set table [handleNewFromType TBLCOL]
   set status [catch {fitsRead $table junk.fits -hdu 1} err]
   if { $status == 1 } {
       echo TEST-ERR: fitsRead fails in heap test
       echo TEST-ERR: $err
   }

   set status [catch {tblToSchema $table $nregion $xtbl} err]
   if { $status == 1 } {
       echo TEST-ERR: tblToSchema fails in heap test
       echo TEST-ERR: $err
   }

   # check data validity. 

   set val [exprGet $nregion.name]
   set val [string range $val 1 5]
   if { [string compare $val Hello] != 0 } {
      echo TEST-ERR: Heap data distortion: should not be $val
   }
   
   set val [exprGet $nregion.mask->name]
   set val [string range $val 1 5]
   if { [string compare $val World] != 0 } {
      echo TEST-ERR: heap data distortion: should not be $val
   }

   set val [exprGet $nregion.nrow]
   if { [expr $val] != 20 } {
      echo TEST-ERR: heap data distortion: should not be $val
   }

   set val [exprGet $nregion.mask->nrow]
   if { [expr $val] != 20 } {
      echo TEST-ERR: heap data distortion: should not be $val
   }

   schemaTransEntryClearAll $xtbl
   exec rm junk.fits
   regDel $region
   regDel $nregion
   tblColDel $table
   return $xtbl
}


#
#  test object reuse
#

proc testReuse { xtbl } {
 
     set container [chainNew HG]
     loop i 1 5 {
       set hg [hgNew]
       handleSet $hg.id $i
       handleSet $hg.entries $i
       chainElementAddByPos $container $hg
     }

    # build table and generates fits file

    schemaTransEntryAdd $xtbl name data_id id int
    schemaTransEntryAdd $xtbl name entries entries int

    set status [catch {set table [schemaToTbl $container $xtbl \
                   -schemaName HG]} err]
    if { $status == 1 } {
        echo TEST-ERR: schemaToTbl fails in object recycle test
        echo TEST-ERR: $err
    }

    set status [catch {fitsWrite $table junk.fits -binary -pdu MINIMAL} err]
    if { $status == 1 } {
        echo TEST-ERR: fitsWrite fails in object recycle test
        echo TEST-ERR: $err
    }

    # copying 1 entry into a new translation table

    set nxtbl [schemaTransNew]
    schemaTransEntryImport $xtbl $nxtbl -to 2


    set ncontainer [chainNew HG]
    tblColDel $table
    set table [handleNewFromType TBLCOL]
    set status [catch {fitsRead $table junk.fits -hdu 1} err]
    if { $status == 1 } {
        echo TEST-ERR: fitsRead fails in object recycle test
        echo TEST-ERR: $err
    }

    set status [catch {tblToSchema $table $ncontainer $nxtbl -schemaName HG -proc hgNew} err]
    if { $status == 1 } {
        echo TEST-ERR: tblToSchema fails in object recycle test
        echo TEST-ERR: $err
    }


   # now do again, use the second entry in the original translation table

    schemaTransEntryClearAll $nxtbl
    schemaTransEntryImport $xtbl $nxtbl -from 1 


    tblToSchema $table $ncontainer $nxtbl -schemaName HG -objectReuse

   # check the data validity

    if { [chainSize $container] != [chainSize $ncontainer] } {
       echo TEST-ERR: input [chainSize $container] HGs in FITS, but got \
                  chainSize $ncontainer]
    }

    set i 1
    set cursor [chainCursorNew $ncontainer]
    while {[set object [chainWalk $ncontainer $cursor]] != ""}  {
        if { [exprGet $object.id] != $i } {
             echo TEST-ERR: Object reuse failure. Id should be $i, not \
                   [exprGet $object.id] 
        }
        if { [exprGet $object.entries] != $i } {
            echo TEST-ERR: Object reuse failure. Entries should be $i, not \
                  [exprGet $object.entries] 
        }
        incr i
    }

    schemaTransEntryClearAll $nxtbl
    schemaTransDel $nxtbl
    schemaTransEntryClearAll $xtbl
    exec rm junk.fits
    chainCursorDel $ncontainer $cursor
    loop i 1 5 {	
	hgDel [chainElementRemByPos $ncontainer 0]
	hgDel [chainElementRemByPos $container 0]
    }
    chainDel $container
    tblColDel $table
    chainDel $ncontainer
    return $xtbl
}


proc newstring { i } {

   # a set of strings
  
   if { $i == 0 } {set str {c h i c a g o} }
   if { $i == 1 } {set str {n e w y o r k} }
   if { $i == 2 } {set str {l o s a n g e l e s} }
   if { $i == 3 } {set str {h o u s t o n} }
   if { $i == 4 } {set str {d e t r o i t} }
   if { $i == 5 } {set str {s e a t l e} }
   if { $i == 6 } {set str {s a n f r a n c i s c o} }
   if { $i > 6 } { set str {d a l l a s}}
   return $str
}


   
proc testFixedArray { xtbl } {

#    Use TBLFLD's fixed array structure to test 
#    don't get confused when TBLFLD is used as an object and when
#    it's used in TBLCOL structure that holds a FITS file

        set chain [chainNew TBLFLD]
        set n 5
        loop i 0 $n {
           set schema [handleNewFromType TBLFLD]
           set str [newstring $i]
           loop j 0 [llength $str] {
             	handleSet $schema.TTYPE<$j> [lindex $str $j]
           }
           chainElementAddByPos $chain $schema
	}
	
	schemaTransEntryAdd $xtbl name TTYPE TTYPE char
	set tblcol [schemaToTbl $chain $xtbl -schemaName TBLFLD]
        fitsWrite $tblcol junk.fits -pdu MINIMAL -binary -standard F

# read in

        set ntblcol [handleNewFromType TBLCOL]
        fitsRead $ntblcol junk.fits -hdu 1
        set nchain [chainNew TBLFLD]
        tblToSchema $ntblcol $nchain $xtbl -proc "handleNewFromType TBLFLD" -schemaName TBLFLD

# check data

       set crsr [chainCursorNew $nchain]
       chainCursorSet $nchain $crsr
       set i 0
       while {[set iterator [chainWalk $nchain $crsr]] != ""}  {
           set str_schema {}
           set str_prestored [newstring $i]

           loop j 0 [llength $str_prestored] {
             set char [exprGet ((TBLFLD)$iterator).TTYPE<$j>]
             lappend str_schema $char
           }

           if { $str_schema != $str_prestored } {
              echo TEST-ERR: testing fixed array gets $str_schema , not $str_prestored
           }
           incr i
       }

       schemaTransEntryClearAll $xtbl
       chainCursorDel $nchain $crsr
       loop i 0 $n {
	   handleDelFromType [chainElementRemByPos $nchain 0]
	   handleDelFromType [chainElementRemByPos $chain 0]
       }
       chainDel $chain
       chainDel $nchain
       tblColDel $tblcol
       tblColDel $ntblcol
       exec rm junk.fits
}


proc testUseOfChain { xtbl } {

# test if chain as a container works OK

        set chain [chainNew REGION]
        set n 5
        loop i 0 $n {
           set schema [regNew -name jk -mask]
           handleSet $schema.nrow 1
           handleSet ((MASK*)$schema.mask)->nrow 2
           chainElementAddByPos $chain $schema
	}
	
	schemaTransEntryAdd $xtbl name regname name  string -dimen 20
        schemaTransEntryAdd $xtbl name nrow    nrow  int    
        schemaTransEntryAdd $xtbl name masknam mask  struct -proc maskNew
        schemaTransEntryAdd $xtbl cont mask    name  string -dimen 20
        schemaTransEntryAdd $xtbl name masknro mask  struct -proc maskNew
        schemaTransEntryAdd $xtbl cont mask    nrow  int

	set tblcol [schemaToTbl $chain $xtbl -schemaName REGION]
        fitsWrite $tblcol junk.fits -pdu MINIMAL -binary -standard F

# read in

        set ntblcol [handleNewFromType TBLCOL]
        fitsRead $ntblcol junk.fits -hdu 1
        set nchain [chainNew REGION]
        tblToSchema $ntblcol $nchain $xtbl -schemaName REGION -proc "regNew -mask"

# check data

        set cursor [chainCursorNew $nchain]

        loop i 0 $n {

           set obj [chainWalk $nchain $cursor] 

           # put the stuff into a link

           set val [exprGet ((REGION)$obj).nrow]
           if { [expr $val] != 1 } {
              echo TEST-ERR: test fails in use of chains. Get $val, not 1
           }

           set val [exprGet ((MASK*)((REGION)$obj).mask)->nrow]
           if { [expr $val] != 2 } {
              echo TEST-ERR: test fails in use of chains. Get $val, not 2
           }

           set val [exprGet ((REGION)$obj).name]
           if { [string compare $val jk] != 0 } {
              echo TEST-ERR: test fails in use of chains. Get $val, not string "jk"
           }

           set val [exprGet ((MASK*)((REGION)$obj).mask)->name]
           if { [string compare $val jk-mask] != 0 } {
              echo TEST-ERR: test fails in use of chains. Get $val, not string "jk-mask"
           }

       }

       schemaTransEntryClearAll $xtbl
       chainCursorDel $nchain $cursor
       loop i 0 $n {
	   regDel [chainElementRemByPos $nchain 0]
	   regDel [chainElementRemByPos $chain 0]
       }
       chainDel $chain
       chainDel $nchain
       tblColDel $tblcol
       tblColDel $ntblcol
       exec rm junk.fits
}           
             
#set xtbl [schemaTransNew]
#testBasics $xtbl
#testArray $xtbl
#testMulti $xtbl
#testEnum $xtbl
#testReuse $xtbl
#testHeap $xtbl
#testFixedArray $xtbl
#testUseOfChain $xtbl
#schemaTransDel $xtbl
#return

