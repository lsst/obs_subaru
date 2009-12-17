# TCL test file for chains


###############################################################################
# Test of chain TYPE extensions
###############################################################################
proc testType {} {
   set chain [chainNew MASK]
   set type_list [chainTypeGet $chain]
   if {[keylget type_list type] != "MASK"}  {
       echo "testType: expected type of chain to be MASK."
       return 1
   }

   # Now let's change the type to GENERIC and see if it works...
   chainTypeSet $chain GENERIC
   set type_list [chainTypeGet $chain]
   if {[keylget type_list type] != "GENERIC"}  {
       echo "testType: expected type of chain to be GENERIC."
       return 1
   }

   chainDel $chain

   # Let's test chainTypeDefine now...
   set chain [chainNew GENERIC]
   set msk1 [maskNew]
   set msk2 [maskNew]
   set reg1 [regNew]
   chainElementAddByPos $chain $msk1   
   chainElementAddByPos $chain $msk2
   if {[chainTypeDefine $chain] != "MASK"}  {
       echo "testType: expected type of chain to be MASK.."
       return 1
   }
   chainElementAddByPos $chain $reg1
   if {[chainTypeDefine $chain] != "GENERIC"}  {
       echo "testType: expected type of chain to be GENERIC.."
       return 1
   }

   set mask [chainElementRemByPos $chain 0]
   maskDel $mask
   set mask [chainElementRemByPos $chain 0]
   maskDel $mask
   set reg [chainElementRemByPos $chain 0]
   regDel $reg

   chainDel $chain

   return 0
}

###############################################################################
# Test of chain join and chain copy extension
###############################################################################
proc testJoinAndCopy {} {
   set src [chainNew MASK]
   set tgt [chainNew REGION]

   set msk1 [maskNew]
   set msk2 [maskNew]
   set reg1 [regNew]
   set reg2 [regNew]

   chainElementAddByPos $src $msk1
   chainElementAddByPos $src $msk2
   chainElementAddByPos $tgt $reg1
   chainElementAddByPos $tgt $reg2

   # Try to jon $tgt to $src. Will result in an error since their types do
   # not match...
   if {[catch {chainJoin $src $tgt}] != 1} {
       echo "testJoinAndCopy: expected an error to happen here, attempting to"
       echo "join a REGION chain to a MASK chain"
       return 1
   }

   # Good. Now change the type of $src to GENERIC and then try to join the
   # chains. Should succeed now...
   chainTypeSet $src GENERIC
   chainJoin $src $tgt

   # $tgt has been destroyed. The size of the new chain should be 4
   if {[chainSize $src] != 4}  {
       echo "chainJoinAndCopy: expected size of joined chain to be 4"
       return 1
   }

   # Let's also test the chainCopy extension
   set cpy [chainCopy $src]
   if {[chainSize $cpy] != 4}  {
       echo "chainJoinAndCopy: expected size of copied chain to be 4"
       return 1
   }
   set type_list [chainTypeGet $cpy]
   if {[keylget type_list type] != "GENERIC"}  {
       echo "testType: expected type of copied chain to be GENERIC"
       return 1
   }

   # Clean up
   set mask [chainElementRemByPos $src 0]
   set mask [chainElementRemByPos $src 0]
   set reg [chainElementRemByPos $src 0]
   set reg [chainElementRemByPos $src 0]
   chainDel $src

   set mask [chainElementRemByPos $cpy 0]
   maskDel $mask
   set mask [chainElementRemByPos $cpy 0]
   maskDel $mask
   set reg [chainElementRemByPos $cpy 0]
   regDel $reg
   set reg [chainElementRemByPos $cpy 0]
   regDel $reg

   chainDel $cpy

   return 0
}

###############################################################################
# Test of chain sort extension
###############################################################################
proc testSort {} {
   set chain [chainNew PT]
   set pt1   [ptNew]
   set pt2   [ptNew]
   set pt3   [ptNew]
   ptDefine  $pt1 30 29 9
   ptDefine  $pt2 10 99 8
   ptDefine  $pt3 70 87 5

   chainElementAddByPos $chain $pt1
   chainElementAddByPos $chain $pt2
   chainElementAddByPos $chain $pt3

   chainSort $chain row -increasing
   
   # We should expect sorted chain to contain: 
   #      $pt2(10,99,8) -> $pt1(30,29,9) -> $pt3(70,87,5) -> 0
   set pt1 [chainElementGetByPos $chain HEAD]
   set members_list [exprGet $pt1]
   if {[keylget members_list row] != 10}  {
       echo "testSort: expected first element on sorted chain to contain a"
       echo "          row value of 10"
       return 1
   }

   set pt2 [chainElementGetByPos $chain 1]
   set members_list [exprGet $pt2]
   if {[keylget members_list row] != 30}  {
       echo "testSort: expected second element on sorted chain to contain a"
       echo "          row value of 30"
       return 1
   }

   set pt3 [chainElementGetByPos $chain TAIL]
   set members_list [exprGet $pt3]
   if {[keylget members_list row] != 70}  {
       echo "testSort: expected last element on sorted chain to contain a"
       echo "          row value of 70"
       return 1
   }

   # Ok, now sort this sorted chain on a new field: col. The current sorted
   # chain looks like:
   #      $pt2(10,99,8) -> $pt1(30,29,9) -> $pt3(70,87,5) -> 0
   # If we sort it again on col, non-increasing sort, we should get:
   #      $pt2(10,99,8) -> $pt3(70,87,5) -> $pt1(30,29,9) ->0
   chainSort $chain col
   set pt1 [chainElementGetByPos $chain HEAD]
   set members_list [exprGet $pt1]
   if {[keylget members_list col] != 99}  {
       echo "testSort: expected first element on sorted chain to contain a"
       echo "          col value of 99"
       return 1
   }

   set pt2 [chainElementGetByPos $chain 1]
   set members_list [exprGet $pt2]
   if {[keylget members_list col] != 87}  {
       echo "testSort: expected second element on sorted chain to contain a"
       echo "          col value of 87"
       return 1
   }

   set pt3 [chainElementGetByPos $chain TAIL]
   set members_list [exprGet $pt3]
   if {[keylget members_list col] != 29}  {
       echo "testSort: expected last element on sorted chain to contain a"
       echo "          col value of 29"
       return 1
   }

   set pt [chainElementRemByPos $chain 0]
   ptDel $pt
   set pt [chainElementRemByPos $chain 0]
   ptDel $pt
   set pt [chainElementRemByPos $chain 0]
   ptDel $pt
   chainDel $chain

   return 0
}

###############################################################################
# Test of chain position extensions
###############################################################################
proc testPos {} {
   set chain [chainNew GENERIC]
   set mask1 [maskNew -name MASK1]
   set mask2 [maskNew -name MASK2]
   set mask3 [maskNew -name MASK3]
   set reg1 [regNew -name REG1]
   set reg2 [regNew -name REG2]
   set reg3 [regNew -name REG3]

   chainElementAddByPos $chain $mask1
   chainElementAddByPos $chain $reg1
   chainElementAddByPos $chain $mask2 HEAD BEFORE
   chainElementAddByPos $chain $reg2 2 AFTER
   chainElementAddByPos $chain $reg3 3 AFTER
   chainElementAddByPos $chain $mask3 2 BEFORE

   # The chain looks like:
   # MASK2 -> MASK1 -> MASK3 -> REG1 -> REG2 -> REG3 -> 0

   # Get some elements at random and see if they are what we expect

   set hndl [chainElementGetByPos $chain 3]
   set members [exprGet $hndl]
   if {[keylget members name] != "REG1"}  {
       echo "testPos: expected 3rd element to be REG1"
       return 1
   }

   set hndl [chainElementGetByPos $chain 2]
   set members [exprGet $hndl]
   if {[keylget members name] != "MASK3"}  {
       echo "testPos: expected 2nd element to be MASK3"
       return 1
   }

   set hndl [chainElementGetByPos $chain TAIL]
   set members [exprGet $hndl]
   if {[keylget members name] != "REG3"}  {
       echo "testPos: expected last element to be REG3"
       return 1
   }

   set hndl [chainElementGetByPos $chain HEAD]
   set members [exprGet $hndl]
   if {[keylget members name] != "MASK2"}  {
       echo "testPos: expected first element to be MASK2"
       return 1
   }

   # Get the type of some elements at random and see if they are what
   # we expect

   set members [chainElementTypeGetByPos $chain HEAD]
   if {[keylget members type] != "MASK"}  {
       echo "testPos: expected type of HEAD element to be MASK"
       return 1
   }

   set members [chainElementTypeGetByPos $chain TAIL]
   if {[keylget members type] != "REGION"}  {
       echo "testPos: expected type of TAIL element to be REGION"
       return 1
   }

   set members [chainElementTypeGetByPos $chain 4]
   if {[keylget members type] != "REGION"}  {
       echo "testPos: expected type of 4th element to be REGION"
       return 1
   }

   set members [chainElementTypeGetByPos $chain 5]
   if {[keylget members type] != "REGION"}  {
       echo "testPos: expected type of 5th element to be REGION"
       return 1
   }

   set members [chainElementTypeGetByPos $chain 1]
   if {[keylget members type] != "MASK"}  {
       echo "testPos: expected type of 1st element to be MASK"
       return 1
   }

   set members [chainElementTypeGetByPos $chain 2]
   if {[keylget members type] != "MASK"}  {
       echo "testPos: expected type of 2nd element to be MASK"
       return 1
   }

   # Now let's delete some elements and see what happens

   set mask2 [chainElementRemByPos $chain 2]
   set members [exprGet -header $mask2]

   if {[keylget members type] != "MASK"}  {
       echo "testPos: 2nd element removed. Expected type to be MASK"
       return 1
   }

   if {[keylget members name] != "MASK3"}  {
       echo "testPos: 2nd element removed. Expected name to be MASK1"
       return 1
   }
   maskDel $mask2

   set reg2 [chainElementRemByPos $chain 3]
   set members [exprGet -header $reg2]
   if {[keylget members type] != "REGION"}  {
       echo "testPos: -3rd element removed. Expected type to be REGION"
       return 1
   }

   if {[keylget members name] != "REG2"}  {
       echo "testPos: 3rd element removed. Expected name to be REGION2"
       return 1
   }
   regDel $reg2

   if {[chainSize $chain] != 4}  {
       echo "testPos: After removing 2 elements, chain size should be 4"
       return 1
   }

   # Now we have 4 elements, lets get handles to them and delete them
   # The elements left are: MASK0 -> MASK1 -> REG2 -> REG3 -> 0
   set hndl [chainElementGetByPos $chain HEAD]
   maskDel $hndl
   set hndl [chainElementGetByPos $chain 1]
   maskDel $hndl
   set hndl [chainElementGetByPos $chain 2]
   regDel $hndl
   set hndl [chainElementGetByPos $chain TAIL]
   regDel $hndl

   chainDel $chain
   return 0
}

###############################################################################
# Test of walking using a cursor
###############################################################################
proc testWalk {} {
   set chain [chainNew GENERIC]
   set pt    [ptNew]
   set mask  [maskNew]
   set reg   [regNew]
   set hg    [hgNew]

   set crsr [chainCursorNew $chain]
   chainElementAddByCursor $chain $pt $crsr
   chainElementAddByCursor $chain $mask $crsr
   chainElementAddByCursor $chain $reg $crsr
   chainElementAddByCursor $chain $hg $crsr

   # Walking from the front of the chain
   set answers "PT MASK REGION HG"
   set i 0
   chainCursorSet $chain $crsr HEAD
   while {[set elem [chainWalk $chain $crsr]] != ""}  {
       if {[handleType $elem] != [lindex $answers $i]}  {
           return 1
       }
       set i [expr $i+1]          
   }
   set i 1
   chainCursorSet $chain $crsr 1
   while {[set elem [chainWalk $chain $crsr NEXT]] != ""}  {
       if {[handleType $elem] != [lindex $answers $i]}  {
           return 1
       }
       set i [expr $i+1]          
   }

   # Walking from the back of the chain
   set i [chainSize $chain]
   set i [expr $i-1]
   chainCursorSet $chain $crsr TAIL
   while {[set elem [chainWalk $chain $crsr PREVIOUS]] != ""}  {
       if {[handleType $elem] != [lindex $answers $i]}  {
           return 1
       }
       set i [expr $i-1]
   }   
   set i 1
   chainCursorSet $chain $crsr 1
   while {[set elem [chainWalk $chain $crsr PREVIOUS]] != ""}  {
       if {[handleType $elem] != [lindex $answers $i]}  {
           return 1
       }
       set i [expr $i-1]
   }   

   # Finally, traverse a chain all the way, and traverse it again
   chainCursorSet $chain $crsr HEAD
   chainWalk $chain $crsr NEXT
   chainWalk $chain $crsr NEXT
   chainWalk $chain $crsr NEXT
   chainWalk $chain $crsr NEXT

   # Now we should get an error on walking...
   if {[chainWalk $chain $crsr NEXT] != ""}  {
       echo "testWalk: expected an error return here."
       return 1
   }

   # Now traverse the chain again. The next fetch should give us PT
   set elem [chainWalk $chain $crsr NEXT]
   if {[handleType $elem] != "PT"}  {
       echo "testWalk: expected PT here..."
       return 1
   }

   # Once more...now traverse from the TAIL
   chainCursorSet $chain $crsr TAIL
   chainWalk $chain $crsr PREVIOUS
   chainWalk $chain $crsr PREVIOUS
   chainWalk $chain $crsr PREVIOUS
   chainWalk $chain $crsr PREVIOUS

   # Now we should get an error on walking...
   if {[chainWalk $chain $crsr PREVIOUS] != ""}  {
       echo "testWalk: expected an error return here."
       return 1
   }

   # Now traverse the chain again. The next fetch should give us HG
   set elem [chainWalk $chain $crsr NEXT]
   if {[handleType $elem] != "HG"}  {
       echo "testWalk: expected HG here..."
       return 1
   }

   # Once more...do a flip flop
   chainCursorSet $chain $crsr 2
   set elem [chainWalk $chain $crsr THIS]
   if {[handleType $elem] != "REGION"}  {
       echo "testWalk: expected REGION here..."
       return 1
   }
   
   set elem [chainWalk $chain $crsr NEXT]
   if {[handleType $elem] != "HG"}  {
       echo "testWalk: expected HG here..."
       return 1
   }

   chainWalk $chain $crsr PREVIOUS
   chainWalk $chain $crsr PREVIOUS

   if {[handleType [chainWalk $chain $crsr THIS]] != "MASK"}  {
       echo "testWalk: expected MASK here..."
       return 1
   }

   # All's well
   set hndl [chainElementRemByPos $chain 0]
   ptDel $hndl
   set hndl [chainElementRemByPos $chain 0]
   maskDel $hndl
   set hndl [chainElementRemByPos $chain 0]
   regDel $hndl
   set hndl [chainElementRemByPos $chain 0]
   hgDel $hndl

   chainCursorDel $chain $crsr
   chainDel $chain

   return 0
}

###############################################################################
# Test of copying a cursor
###############################################################################
proc chainCursorCopyTest {}  {
   set chain [chainNew MASK]

   chainElementAddByPos $chain [maskNew -name "MASK0"]
   chainElementAddByPos $chain [maskNew -name "MASK1"]

   set origCrsr [chainCursorNew $chain]
   set mask1 [chainWalk $chain $origCrsr]
   set m1 [exprGet $mask1]
   set name1 [keylget m1 name]

   set newCrsr [chainCursorCopy $chain $origCrsr]
   set mask2 [chainWalk $chain $newCrsr THIS]
   set m2 [exprGet $mask2]
   set name2 [keylget m2 name]

   if {$name1 != $name2}  {
       echo "testChainCursorCopy: copied cursor does not match original one(0)"
       return 1
   }

   set mask1 [chainWalk $chain $origCrsr]
   set m1 [exprGet $mask1]
   set name1 [keylget m1 name]

   set mask2 [chainWalk $chain $newCrsr]
   set m2 [exprGet $mask2]
   set name2 [keylget m2 name]

   if {$name1 != $name2}  {
       echo "testChainCursorCopy: copied cursor does not match original one(1)"
       return 1
   }

   chainCursorDel $chain $newCrsr
   chainCursorSet $chain $origCrsr HEAD
   set mask1 [chainWalk $chain $origCrsr]
   maskDel $mask1
   set mask1 [chainWalk $chain $origCrsr]
   maskDel $mask1
   chainCursorDel $chain $origCrsr
   chainDel $chain

   return 0
}

###############################################################################
# Test of removing elements by cursor
###############################################################################
proc testRemByCursor {}  {
   set chain [chainNew GENERIC]
   set pt    [ptNew]
   set mask  [maskNew]
   set reg   [regNew]
   set hg    [hgNew]

   # This procedure tests the chain capability that if two cursors are
   # pointing to the same chain element, and one cursor is used to delete
   # that element; then the other cursor can still be used to traverse
   # the chain. 

   set crsr1 [chainCursorNew $chain]
   chainElementAddByCursor $chain $pt $crsr1
   chainElementAddByCursor $chain $mask $crsr1
   chainElementAddByCursor $chain $reg $crsr1
   chainElementAddByCursor $chain $hg $crsr1
   set crsr2 [chainCursorNew $chain]

   chainCursorSet $chain $crsr1 1
   chainCursorSet $chain $crsr2 1

   # Now remove the 2nd element. Remember, we have crsr2 still pointing to it
   set elem [chainElementRemByCursor $chain $crsr1]
   if {[handleType $elem] != "MASK"}  {
       echo "testRemByCursor: expected removed element to be MASK"
       return 1
   }

   # Now if we traverse (NEXT) the chain using crsr2, we should get REGION
   set elem [chainWalk $chain $crsr2 NEXT]
   if {[handleType $elem] != "REGION"}  {
       echo "testRemByCursor: expected NEXT element to be REGION"
       return 1
   }

   chainCursorSet $chain $crsr1 2
   chainCursorSet $chain $crsr2 2

   set elem [chainElementRemByCursor $chain $crsr1]
   if {[handleType $elem] != "HG"}  {
       echo "testRemByPos: expected removed element to be HG"
       return 1
   }
   set elem [chainWalk $chain $crsr2 PREVIOUS]
   if {[handleType $elem] != "REGION"}  {
       echo "testRemByCursor: expected PREVIOUS element to be REGION"
       return 1
   }

   # All's well
   chainCursorDel $chain $crsr1
   chainCursorDel $chain $crsr2
   set elem [chainElementRemByPos $chain 0]
   ptDel $elem
   set elem [chainElementRemByPos $chain 0]
   regDel $reg

   chainDel $chain
   maskDel $mask
   hgDel $hg

   return 0
}

###############################################################################
# Test of initializing cursors
###############################################################################
proc testCursorsInit {} {
   set chain [chainNew GENERIC]
   set pt    [ptNew]
   set mask  [maskNew]
   set reg   [regNew]

   chainElementAddByPos $chain $pt
   chainElementAddByPos $chain $mask
   chainElementAddByPos $chain $reg

   set crsr1 [chainCursorNew $chain]
   if {[chainCursorCount $chain] != 1}  {
       error "testCursorsInit: expected chain cursor count to be 1."
       return 1
   }

   chainCursorSet $chain $crsr1 1
   set mask [chainWalk $chain $crsr1 THIS]
   set members [exprGet -header $mask]
   if {[keylget members type] != "MASK"}  {
       error "testCursorsInit: expected $crsr1 to be set to a MASK"
       return 1
   }

   set crsr2 [chainCursorNew $chain]
   if {[chainCursorCount $chain] != 2}  {
       error "testCursorsInit: expected chain cursor count to be 2"
       return 1
   }

   chainCursorSet $chain $crsr2 TAIL
   set reg [chainWalk $chain $crsr2 THIS]
   set members [exprGet -header $reg]
   if {[keylget members type] != "REGION"}  {
       error "testCursorsInit: expected $crsr2 to be set to a REGION"
       return 1
   }   

   chainCursorDel $chain $crsr1
   if {[chainCursorCount $chain] != 1}  {
       error "testCursorsInit: expected chain cursor count to be 1.."
       return 1
   }

   chainCursorDel $chain $crsr2
   if {[chainCursorCount $chain] != 0}  {
       error "testCursorsInit: expected chain cursor count to be 0"
       return 1
   }

   ptDel [chainElementRemByPos $chain 0]
   maskDel [chainElementRemByPos $chain 0]
   regDel [chainElementRemByPos $chain 0]
   chainDel $chain

   return 0
}

###############################################################################
# Test of chains using cursors
###############################################################################
proc testCursors {} {
   set chain [chainNew GENERIC]
   set elem1 [maskNew]
   set elem2 [regNew]
   set elem3 [hgNew]
   chainElementAddByPos $chain $elem1

   # Get cursor, set it to HEAD and add remaining element using the cursor
   set crsr1 [chainCursorNew $chain]
   chainCursorSet $chain $crsr1
   chainElementAddByCursor $chain $elem2 $crsr1
   chainElementAddByCursor $chain $elem3 $crsr1

   handleDel $elem1
   handleDel $elem2
   handleDel $elem3

   if {[chainSize $chain] != 3}  {
       echo "testCursors: expected a size of 3"
       return 1
   }
   
   # Now get a new cursor and set it to the 2nd element
   set crsr2 [chainCursorNew $chain]
   chainCursorSet $chain $crsr2 1

   if {[chainCursorCount $chain] != 2}  {
       echo "testCursors: expected cursor count to be 2"
       return 1
   }

   # Now use $crsr1 to traverse backwards. It is pointing to the last element
   # on the chain (which should be type HG). First let's make sure that
   # the last element IS of type HG
   set members [exprGet -header [chainWalk $chain $crsr1 THIS]]
   set lastElem [chainWalk $chain $crsr1 THIS]
   if {[handleType $lastElem] != [keylget members type]}  {
       echo "testCursors: expected last element to be type HG"
       return 1
   }
   handleDel $lastElem
   
   # Go back..element cursor will be positioned on is of type region.
   # Delete this element, recall that $crsr2 is still pointing to this
   # element; so this deletion will make $crsr2 invalid.
   set hndl [chainWalk $chain $crsr1 PREVIOUS]
   handleDel $hndl
   set hndl [chainElementRemByCursor $chain $crsr1]
   regDel $hndl

   if {[chainSize $chain] != 2}  {
       echo "testCursors: expected chain size to be 2"
       return 1
   }

   # Now if we try to walk the chain using $crsr2, we should get the HG
   # element.
   set members [exprGet -header [chainWalk $chain $crsr2 NEXT]]
   set lastElem [chainWalk $chain $crsr2 THIS]
   if {[handleType $lastElem] != [keylget members type]} {
       echo "testCursors: expected an element of type HG here"
       return 1
   }

   # All's well.
   chainCursorDel $chain $crsr1
   chainCursorDel $chain $crsr2
   maskDel [chainElementRemByPos $chain 0]
   hgDel [chainElementRemByPos $chain 0]
   chainDel $chain
   
   return 0
}


###############################################################################
# Test of dumping and restoring chains
# 
#    we should have another test for dumping GENERIC chains...
###############################################################################
proc testDumps {} {

   # first, we try dumping a simple, three-REGION chain 
   set chain [chainNew REGION]
   set reg1 [regNew 10 10]
   chainElementAddByPos $chain $reg1 TAIL AFTER
   set reg2 [regNew 20 20]
   chainElementAddByPos $chain $reg2 TAIL AFTER
   set reg3 [regNew 30 30]
   chainElementAddByPos $chain $reg3 TAIL AFTER
   dumpOpen chain.dump w
   if {[ catch { dumpHandleWrite $chain }] != 0} {
     echo "testDumps: failed on dumpHandleWrite simple chain"
     return 1
   }
   if {[ catch { dumpClose }] != 0} {
     echo "testDumps: failed on dumpClose after Write for simple chain"
     return 1
   }

   # now read the chain back
   dumpOpen chain.dump r
   if {[ catch { set chain2 [ dumpHandleRead ] }] != 0} {
     echo "testDumps: failed on dumpHandleRead simple chain"
     return 1
   }
   if {[catch {dumpClose}] != 0} {
     echo "testDumps: failed on dumpClose after Read for simple chain"
     return 1
   }
   echo "The previous error message is a feature -- ignore it"

   # check to see that the information is properly inside the chain
   if { [chainSize $chain2] != 3 } {
     echo "testDumps: chainSize of simple chain wrong after reading"
     return 1
   }
   set celem [chainElementGetByPos $chain2 0]
   if { [exprGet $celem.nrow] != 10 } {
     echo "testDumps: region 1 has wrong size"
     return 1
   }
   set celem [chainElementGetByPos $chain2 1]
   if { [exprGet $celem.nrow] != 20 } {
     echo "testDumps: region 2 has wrong size"
     return 1
   }
   set celem [chainElementGetByPos $chain2 2]
   if { [exprGet $celem.nrow] != 30 } {
     echo "testDumps: region 3 has wrong size"
     return 1
   }

   # okay, now clean up
   regDel [chainElementRemByPos $chain 0]
   regDel [chainElementRemByPos $chain 0]
   regDel [chainElementRemByPos $chain 0]
   chainDel $chain

   regDel [chainElementRemByPos $chain2 0]
   regDel [chainElementRemByPos $chain2 0]
   regDel [chainElementRemByPos $chain2 0]
   chainDel $chain2
   exec rm chain.dump

   return 0
}

# chainCompare on  Chains
proc chainCompTest {} {
    set chain1 [chainNew PT]
    set p [ptNew]; handleSet $p.row 1 ;handleSet $p.col 1 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 2 ;handleSet $p.col 1 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 3 ;handleSet $p.col 2 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 4 ;handleSet $p.col 2 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 1 ;handleSet $p.col 1 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 9 ;handleSet $p.col 9 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 2 ;handleSet $p.col 3 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 3 ;handleSet $p.col 4 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 4 ;handleSet $p.col 4 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 5 ;handleSet $p.col 5 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 6 ;handleSet $p.col 5 
    chainElementAddByPos $chain1 $p

    set chain2 [chainNew PT]
    set p [ptNew]; handleSet $p.row 1 ;handleSet $p.col 3 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 2 ;handleSet $p.col 3 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 3 ;handleSet $p.col 4 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 1 ;handleSet $p.col 1 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 2 ;handleSet $p.col 1 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 3 ;handleSet $p.col 2 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 4 ;handleSet $p.col 2 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 4 ;handleSet $p.col 4 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 5 ;handleSet $p.col 5 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 6 ;handleSet $p.col 5 
    chainElementAddByPos $chain2 $p

    set result [chainCompare $chain1 $chain2]
    set vMask [keylget result vMask]
    set closest [keylget result chain]

    if {[exprGet $vMask.vec<0> ] != 1} {return 1}
    if {[exprGet $vMask.vec<1> ] != 1} {return 1}
    if {[exprGet $vMask.vec<2> ] != 1} {return 1}
    if {[exprGet $vMask.vec<3> ] != 1} {return 1}
    if {[exprGet $vMask.vec<4> ] != 1} {return 1}
    if {[exprGet $vMask.vec<5> ] != 0} {return 1}
    if {[exprGet $vMask.vec<6> ] != 1} {return 1}
    if {[exprGet $vMask.vec<7> ] != 1} {return 1}
    if {[exprGet $vMask.vec<8> ] != 1} {return 1}
    if {[exprGet $vMask.vec<9> ] != 1} {return 1}
    if {[exprGet $vMask.vec<10>] != 1} {return 1}

    set nChain1 [chainSize $chain1]
    set nClosest [chainSize $closest]
    if {$nChain1 != $nClosest} {return 1}

    set answers "1 2 3 4 1 6 2 3 4 5 6"
    loop i 0  [chainSize $closest] {
	set p [chainElementGetByPos $closest $i]
	set ans [lindex $answers $i]
	if {[exprGet $p.row] != $ans} {return 1}
    }

    # clean up
    vectorExprDel $vMask
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainDel $closest
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    chainDel $chain1
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    chainDel $chain2
    return 0
}
proc chainCompTest2 {} {
    set chain1 [chainNew PT]
    set p [ptNew]; handleSet $p.row 1 ;handleSet $p.col 1 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 2 ;handleSet $p.col 1 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 3 ;handleSet $p.col 2 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 4 ;handleSet $p.col 2 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 1 ;handleSet $p.col 1 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 9 ;handleSet $p.col 9 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 2 ;handleSet $p.col 3 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 3 ;handleSet $p.col 4 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 4 ;handleSet $p.col 4 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 5 ;handleSet $p.col 5 
    chainElementAddByPos $chain1 $p
    set p [ptNew]; handleSet $p.row 6 ;handleSet $p.col 5 
    chainElementAddByPos $chain1 $p

    set chain2 [chainNew PT]
    set p [ptNew]; handleSet $p.row 1 ;handleSet $p.col 3 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 2 ;handleSet $p.col 3 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 3 ;handleSet $p.col 4 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 1 ;handleSet $p.col 1 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 2 ;handleSet $p.col 1 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 3 ;handleSet $p.col 2 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 4 ;handleSet $p.col 2 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 4 ;handleSet $p.col 4 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 5 ;handleSet $p.col 5 
    chainElementAddByPos $chain2 $p
    set p [ptNew]; handleSet $p.row 6 ;handleSet $p.col 5 
    chainElementAddByPos $chain2 $p

    set closest [chainCompare2 $chain1 $chain2]

    set nChain1 [chainSize $chain1]
    set nClosest [chainSize $closest]
    if {$nChain1 != $nClosest} {return 1}

    set answers "1 2 3 4 1 6 2 3 4 5 6"
    loop i 0  [chainSize $closest] {
	set p [chainElementGetByPos $closest $i]
	set ans [lindex $answers $i]
	if {[exprGet $p.row] != $ans} {return 1}
    }

    # clean up
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainElementRemByPos $closest 0
    chainDel $closest
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    ptDel [chainElementRemByPos $chain1 0]
    chainDel $chain1
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    ptDel [chainElementRemByPos $chain2 0]
    chainDel $chain2

    return 0
}

###############################################################################
# Entry point
###############################################################################

if {[testType] != 0}  {
    error "Error in testType"
}

if {[testJoinAndCopy] != 0}  {
    error "Error in testJoinAndCopy"
}

if {[testSort] != 0}  {
    error "Error in testSort"
}

if {[testPos] != 0}  {
   error "Error in testing position extensions"
}

if {[testCursorsInit] != 0}  {
    error "Error in testCursorsInit"
}

if {[testCursors] != 0}  {
    error "Error in testCursors"
}

if {[testWalk] != 0}  {
   error "Error in testWalk"
}

if {[testRemByCursor] != 0}  {
    error "Error in testRemByCursor"
}

if {[testDumps] != 0}  {
    error "Error in testDumps"
}

if {[chainCompTest] != 0} {
    error "error in chainCompTest" 
}

if {[chainCompTest2] != 0} {
    error "error in chainCompTest2" 
}

if {[chainCursorCopyTest] != 0}  {
    error "error in chainCursorCopyTest"
}

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_chain.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_chain"
    return 1
}
