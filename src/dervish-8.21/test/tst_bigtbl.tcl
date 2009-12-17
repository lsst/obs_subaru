# a comprehensive test  involving the objects

# features to test:
#  1)  multiple objects with multiple indirections
#  2)  variable array (i.e, *-ed object)
#  3)  fixed size array
#  4)  heap
#  5)  multi-d specification at FITS side

#  these testing are done via a test structurue defined in shCSchemaCnv.h

set dim 3

proc setTestObj {block i} {

   # set ia, int; fa, float; dbl, double

   global dim

   handleSet ((TRANSTEST)$block).ia -$i
   handleSet ((TRANSTEST)$block).fa -2$i.0
   handleSet ((TRANSTEST)$block).dbl -2$i.0

   # set ib, int[10]

   loop j 0 10 {
      handleSet ((TRANSTEST)$block).ib<$j> $j
   }

   # set fb, float [20]; cb, char[20]

   loop j 0 20 {
      handleSet ((TRANSTEST)$block).fb<$j> 1$j.0
      if { $j > 4 } { handleSet ((TRANSTEST)$block).cb<$j> \000
      } else { handleSet ((TRANSTEST)$block).cb<$j> [lindex {h e l l o} $j] }
   }

   # set the region stuff: ra
 
   handleSetFromHandle (REGION*)((TRANSTEST)$block).ra &[regNew -mask]
   handleSet ((REGION*)((TRANSTEST)$block).ra)->name "tregion"
   handleSet ((REGION*)((TRANSTEST)$block).ra)->type TYPE_FL64
   handleSet ((REGION*)((TRANSTEST)$block).ra)->mask->name "tmask"
   


   # initialize rb, which is a pointer-to-pointer, to 3*sizeof(PTR)=12.

   handleSetFromHandle ((TRANSTEST)$block).rb &[shMalloc [expr 3*[sizeof PTR]]]

   loop j 0 $dim {
     set region [regNew -mask]
     handleSet $region.name region$j$j
     handleSet $region.type [lindex {TYPE_FL64 TYPE_FL32 TYPE_S32} $j]
     set k 3
     handleSetFromHandle $region.rows_s8  &[shMalloc [expr $k*[sizeof PTR]]]
     loop m 0 $k {
        handleSetFromHandle (PTR)((CHAR**)$region.rows_s8)<$m> &[shMalloc 4]
        handleSet  (CHAR)((CHAR**)$region.rows_s8)<$m><0> a
        handleSet  (CHAR)((CHAR**)$region.rows_s8)<$m><1> b
        handleSet  (CHAR)((CHAR**)$region.rows_s8)<$m><2> c
        handleSet  (CHAR)((CHAR**)$region.rows_s8)<$m><3> d
        
     }
     
     handleSetFromHandle ((TRANSTEST)$block).rb<$j> &$region
     handleSet ((REGION*)((TRANSTEST)$block).rb<$j>)->mask->nrow 10
     handleSet ((REGION*)((TRANSTEST)$block).rb<$j>)->mask->ncol 10
   }

   heapSetOrCmp $block set 
}

proc heapSetOrCmp { obj cmd } {

    # set heap or compare heap. Heap is **heap[3][2]. 
    # after set, heap should be [3][2][4][5]

    if { [string compare $cmd set] == 0 } {
        set k 4
        loop i 0 3 {
           loop j 0 2  {
              handleSetFromHandle (PTR)((INT**)((TRANSTEST)$obj).heap<$i><$j>) &[shMalloc [expr $k*[sizeof PTR]]]
              loop l 0 $k {
                 handleSetFromHandle (PTR)((INT**)((TRANSTEST)$obj).heap<$i><$j>)<$l> &[shMalloc [expr 5*[sizeof INT]]]
              }
           }
        }

       loop i 0 3 {
          loop j 0 2 {
            loop l 0  4 {
               loop m 0 5 {
                 handleSet (INT)((INT**)((TRANSTEST)$obj).heap<$i><$j>)<$l><$m> [expr $i+$j+$l+$m]

               }
             }
          }
       }

       return 0

    }

    if { [string compare $cmd cmp] == 0 } {

       loop i 0 3 {
          loop j 0 2 {
            loop l 0  4 {
               loop m 0 5 {
                 set val1 [lindex [exprGet -header (INT)((INT**)((TRANSTEST)$obj).heap<$i><$j>)<$l><$m>] 1]
                 set val2 [expr $i+$j+$l+$m]
                 if { [expr $val1] != $val2 } {
                     echo TEST-ERR: should get $val2 for heap<$i><$j><$l><$m>, but only get $val1
                 }
               }
             }
          }
       }
       
      return 0
    } 

    echo Unrecognized command in heapSetOrCmp
    return 1
}




proc compTestObj {obj1  obj2 } {

    # compare ia, fa, and dbl

    global dim

    set status 0

    set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).ia] 1]
    set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).ia] 1]

    if { $val1 != $val2 } {
           echo TEST-ERR:  unequal int values for ia. $val1 vs. $val2
           set status -1
           
    }

    set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).fa] 1]
    set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).fa] 1]

    if { $val1 != $val2 } {
           echo TEST-ERR:  unequal float values for fa. $val1 vs. $val2
           set status -1
    }
 
    set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).dbl] 1]
    set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).dbl] 1]

    if { $val1 != $val2 } {
           echo TEST-ERR:  unequal double values for dbl. $val1 vs. $val2
           set status -1
    }



    # compare ib , int[10];

    loop j 0 10 {
       set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).ib<$j>] 1]
       set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).ib<$j>] 1]
       if { $val1 != $val2 } {
           echo TEST-ERR:  unequal int values for ib<$j>. $val1 vs. $val2
           set status -1
       }
    }
    
    # compare fb, float [20]; cb, char[20]

    loop j 0 20 {
       set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).fb<$j>] 1]
       set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).fb<$j>] 1]
       if { $val1 != $val2 } {
           echo TEST-ERR:  unequal float values fb<$j>. $val1 vs. $val2
           set status -1
       }

       set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).cb<$j>] 1]
       set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).cb<$j>] 1]
       if { $val1 != $val2 } {
           echo TEST-ERR:  unequal char values cb<$j>. $val1 vs. $val2
           set status -1
       }

      
   }

   # compare the single region : ra, REGION*;

     set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).ra->name] 1]
     set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).ra->name] 1]
     if { $val1 != $val2 } {
           echo TEST-ERR:  unequal string values for ra->name. $val1 vs. $val2
           set status -1
     }
     
     set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).ra->type] 1]
     set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).ra->type] 1]
     if { $val1 != $val2 } {
           echo TEST-ERR:  unequal enum values for ra->type. $val1 vs. $val2
           set status -1
     }


     set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).ra->mask->name] 1]
     set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).ra->mask->name] 1]
     if { $val1 != $val2 } {
           echo TEST-ERR:  unequal string values for ra->mask->name. $val1 vs. $val2
           set status -1
      }

     
     

    # compare what's in regions;

    loop j 0 $dim {

        set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).rb<$j>->mask->nrow] 1]
        set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).rb<$j>->mask->nrow] 1]
        if { $val1 != $val2 } {
           echo TEST-ERR:  unequal float values for rb<$j>->mask->nrow. $val1 vs. $val2
           set status -1
        }

        set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).rb<$j>->mask->ncol] 1]
        set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).rb<$j>->mask->ncol] 1]
         if { $val1 != $val2 } {
           echo TEST-ERR:  unequal float values for rb<$j>->mask->ncol. $val1 vs. $val2
           set status -1
        }

        set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).rb<$j>->name] 1]
        set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).rb<$j>->name] 1]
        if { $val1 != $val2 } {
            echo TEST-ERR:  unequal string values for rb<$j>->name $val1 vs. $val2
            set status -1
        }

        set val1 [lindex [exprGet -header ((TRANSTEST)$obj1).rb<$j>->type] 1]
        set val2 [lindex [exprGet -header ((TRANSTEST)$obj2).rb<$j>->type] 1]
        if { $val1 != $val2 } {
            echo TEST-ERR:  unequal enum values for rb<$j>->type $val1 vs. $val2
            set status -1
        }
       
       loop k 0 3 {
          loop m 0 4 {
             set val1 [lindex [exprGet -header (CHAR)((CHAR**)((TRANSTEST)$obj1).rb<$j>->rows_s8)<$k><$m>] 1]
             set val2 [lindex [exprGet -header (CHAR)((CHAR**)((TRANSTEST)$obj2).rb<$j>->rows_s8)<$k><$m>] 1]
             if { $val1 != $val2 } {
               echo TEST-ERR:  unequal string values for rb<$j>->rows_s8<$k><$m>, $val1 vs. $val2
                 set status -1
             }
           }
       }

    }

    heapSetOrCmp $obj2 cmp

    return $status;
}
       

proc comprehensiveTest { xtbl } {

  # create a few good TRANSTEST objects

    global dim
    set n 5 
    set chain  [chainNew PTR]
    loop i 0 $n {
        set block [shMalloc [sizeof TRANSTEST]]
        setTestObj $block $i
        chainElementAddByPos $chain $block
    }
 

    set crsr [chainCursorNew $chain]
    chainCursorSet $chain $crsr


    # build a translation table, set both fa and dbl to float type

    schemaTransEntryAdd $xtbl name dbl dbl double
    schemaTransEntryAdd $xtbl name fa fa float

    # then, nasty heap specifications

    schemaTransEntryAdd $xtbl name bghepa1 heap    heap  -dimen 4 -heaplength 5 -heaptype int
    schemaTransEntryAdd $xtbl name bgheap2 heap<1> heap  -dimen 4 -heaplength 5 -heaptype int
    schemaTransEntryAdd $xtbl name bgheap3 heap<2><1><2> heap  -dimen 4 -heaplength 5 -heaptype int
    schemaTransEntryAdd $xtbl name ia  ia int
    
    # throw a bunch of arrays in FITS, randomly

    schemaTransEntryAdd $xtbl name ib<0>  ib int
    schemaTransEntryAdd $xtbl name ib<1>  ib int
    schemaTransEntryAdd $xtbl name ib<2>  ib int

    # throw a bunch of arrays in FITS, randomly

     schemaTransEntryAdd $xtbl name fb<5><7><9>  fb float
     schemaTransEntryAdd $xtbl name fb<2><3><10> fb float

     schemaTransEntryAdd $xtbl name cb  cb char
     schemaTransEntryAdd $xtbl name cbstr  cb string

     schemaTransEntryAdd $xtbl name ra_name ra   struct
     schemaTransEntryAdd $xtbl cont ra      name string -dimen 10

     schemaTransEntryAdd $xtbl name ra_mask ra   struct
     schemaTransEntryAdd $xtbl cont ra      mask struct 
     schemaTransEntryAdd $xtbl cont mask    name string -dimen 10

     schemaTransEntryAdd $xtbl name enumtype<0> ra   struct
     schemaTransEntryAdd $xtbl cont ra       type enum 

    loop j 0 $dim {
        schemaTransEntryAdd $xtbl name rb_name<$j> rb<$j>   struct   -dimen $dim   
        schemaTransEntryAdd $xtbl cont rb<$j>      name     string   -dimen 10

        schemaTransEntryAdd $xtbl name enumtype<[expr $j+1]> rb<$j>   struct   -dimen $dim  
        schemaTransEntryAdd $xtbl cont rb<$j>                type      enum   

        schemaTransEntryAdd $xtbl name rb1_2d$j   rb<$j>   struct   -dimen $dim
        schemaTransEntryAdd $xtbl cont rb<$j>      rows_s8  heap   -dimen $dim -heaplength 4 -heaptype char
      
        schemaTransEntryAdd $xtbl name rb2_0$j   rb<$j>   struct   -dimen $dim
        schemaTransEntryAdd $xtbl cont rb<$j>    rows_s8<0>  heap  -dimen 3 -heaplength 4 -heaptype char

        schemaTransEntryAdd $xtbl name rb2_1$j   rb<$j>   struct   -dimen $dim
        schemaTransEntryAdd $xtbl cont rb<$j>    rows_s8<1>  heap  -dimen 3 -heaplength 4 -heaptype char

        schemaTransEntryAdd $xtbl name rb2_2$j   rb<$j>   struct   -dimen $dim
        schemaTransEntryAdd $xtbl cont rb<$j>    rows_s8<2>  heap  -dimen 3 -heaplength 4 -heaptype char

        schemaTransEntryAdd $xtbl name msk_nrow rb<$j>   struct  -dimen $dim
        schemaTransEntryAdd $xtbl cont rb<$j>      mask struct 
        schemaTransEntryAdd $xtbl cont mask        nrow int

        schemaTransEntryAdd $xtbl name msk_ncol     rb<$j>   struct  -dimen $dim
        schemaTransEntryAdd $xtbl cont rb<$j>      mask      struct 
        schemaTransEntryAdd $xtbl cont mask        ncol      int


    }

    
    schemaTransWriteToFile $xtbl tbl.junk
    set xtbl [schemaTransNew]
    schemaTransEntryAddFromFile $xtbl tbl.junk


    chainTypeSet $chain TRANSTEST
    set tblcol [schemaToTbl $chain $xtbl -schemaName TRANSTEST]
    fitsWrite $tblcol junk.fits -binary -pdu MINIMAL -standard F


    #  read back

    set ntblcol [handleNewFromType TBLCOL]
    fitsRead $ntblcol junk.fits -hdu 1 -binary


    # redo the 1st 2 entries in the table, swap source field

    schemaTransEntryDel $xtbl 0
    schemaTransEntryDel $xtbl 0
    schemaTransEntryAdd $xtbl name fa dbl double
    schemaTransEntryAdd $xtbl name dbl fa float


    set nchain [chainNew TRANSTEST]
    tblToSchema $ntblcol $nchain $xtbl -schemaName TRANSTEST

    set obj1 [shMalloc [sizeof TRANSTEST]]
    set obj2 [shMalloc [sizeof TRANSTEST]]


    loop i 0 $n  {
      set iterator1 [chainElementGetByPos $chain $i]
      set iterator2 [chainElementGetByPos $nchain $i]

       handleSetFromHandle (TRANSTEST)$obj1 (TRANSTEST)$iterator1
       handleSetFromHandle (TRANSTEST)$obj2 (TRANSTEST)$iterator2

       set status [catch {compTestObj $obj1 $obj2} err]
       if { $status == 1 } {
          echo ERROR-MESSAGE: $err
       }

    }

    exec rm junk.fits tbl.junk
    schemaTransEntryClearAll $xtbl
}


set xtbl [schemaTransNew]
comprehensiveTest $xtbl
schemaTransDel $xtbl
