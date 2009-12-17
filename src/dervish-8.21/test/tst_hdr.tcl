###################################################################
#
# Test the hdrCopy command using the following cases:
#
#   o The header values are the same for both headers
#   o The memory tree contains the appropriate number of items
#
# Perform each of the above statements for the two headers.
#
###################################################################

# Modification:
# Nov 14, 1994:  One cannot assume that the memory tree is empty when
#    Vijay       this file is sourced. The original writer of this file
#                depended on that assumption. A better way is to get
#                the current allocated memory block length and whenever
#                we need to know how many items we've added/subtracted,
#                get the curreny allocated memory block length and
#                subtract it from the original one.

set initialMemLength [llength [memBlocksGet]]

###################################################################
#
# These statements create the headers and also performs the hdrCopy
# command.
#
###################################################################

set failed 0;				# return value

set hdrone [hdrNew]
set hdrtwo [hdrNew]

set $hdrone [source ../test/header_values]

set status [catch { hdrCopy $hdrone $hdrtwo } err]
if { $status != 0 } {
   echo TEST-ERR: hdrCopy fails, header not copied
   echo TEST-ERR: $err
   set failed 1;
   }

###################################################################
#
# These statements get the line total for each of the two headers
# and check to see if the line totals are equivalent.
#
###################################################################

set status [catch {hdrGetLineTotal $hdrone} err]
if { $status != 0 } {
   echo TEST-ERR: hdrCopy fails: no lines in <source> header
   echo TEST-ERR: $err
   set failed 1;
   }

set status [catch {hdrGetLineTotal $hdrtwo} err]
if { $status != 0 } {
   echo TEST-ERR: hdrCopy fails: no lines in <destination> header
   echo TEST-ERR: $err
   set failed 1;
   }

set hdone_linetl [hdrGetLineTotal $hdrone]
set hdtwo_linetl [hdrGetLineTotal $hdrtwo]

if {$hdone_linetl != $hdtwo_linetl} {
    echo TEST-ERR: hdrCopy fails: number of lines is different
    set failed 1;
}

###################################################################
#
# These statements check that the contents of the two headers
# are equivalent after they have been copied.
#
###################################################################

set lines $hdone_linetl

for { set i 0 } {$i < $lines} {incr i} {
    set hdrone_line [hdrGetLineCont $hdrone $i] 
    set hdrtwo_line [hdrGetLineCont $hdrtwo $i]
    set status [catch {string first $hdrtwo_line $hdrone_line } err]
    if { $status != 0 } {
       echo TEST-ERR: hdrCopy fails: header contents are different
       echo TEST-ERR: $err
       set failed 1;
    }
  }

###################################################################
#
# These statements test to ensure that the correct number of items
# are on the memory tree for the specified header.
#
###################################################################

hdrDel $hdrone
hdrDel $hdrtwo

set hdrnew_lines [hdrNew]
set hdrtwo_lines [hdrNew]

hdrInsWithAscii $hdrnew_lines  NAME PAUL name
hdrInsWithAscii $hdrnew_lines  MIDINT J middleinit
hdrInsWithAscii $hdrnew_lines  LAST GREEN lastname
hdrInsWithAscii $hdrnew_lines  SEX MALE sex 
hdrInsWithAscii $hdrnew_lines  TITLE TECH title

set status [catch { hdrCopy $hdrnew_lines $hdrtwo_lines } err]
if { $status != 0 } {
   echo TEST-ERR: hdrCopy fails, header not copied
   echo TEST-ERR: $err
   set failed 1;
   }

##############################################################
#
# Do a comparison of the tree_get elements and the hdlone_ptr
# for each element in the list
#
##############################################################
    

set status [catch {memBlocksGet} err]
if { $status != 0 } {
   echo TEST-ERR: hdrCopy fails: memBlocksGet fails 
   echo TEST-ERR: $err
   set failed 1;
   }

set tree_get $err
set trees [expr [llength $tree_get]-$initialMemLength]
set hdlone_ptr [handlePtr $hdrnew_lines]

set j 0
set k 0

if { $trees == 14 } {
   for { set i 0 } { $i < $trees } { incr i } {
       set item [lindex $err $i]
       set comp [string first $hdlone_ptr $item]
       if { $comp == -1 } {
          incr j} {
          incr k}
       }
       if { $k < 1 } { 
          echo TEST-ERR: hdrCopy fails: handle pointer not in memory tree
	  set failed 1;
       }
  }

if { $trees != 14 } {
   echo TEST-ERR: memBlocksGet fails: incorrect number of items in memory tree
   set failed 1;
}


hdrDel $hdrnew_lines
hdrDel $hdrtwo_lines

############################################################ 
#
# Test the hdrDel command using the following cases:
#
#   o The header is present in the memory tree
#   o The header has been deleted
#   o The memory tree is empty after the deletion of the header
#
################################################################ 


################################################################ 
#
# These statements check to ensure that the header is present
# in the memory tree
#
################################################################ 

set hdrone [hdrNew]
set $hdrone [source ../test/header_values]

set status [catch {memBlocksGet} err]
set tree_get $err

if { $status != 0 } {
   echo TEST-ERR: memBlocksGet fails 
   echo TEST-ERR: $err
   set failed 1;
   }


set status [catch {handlePtr $hdrone} err]
set hdl_ptr $err

if { $status != 0} {
   echo TEST-ERR: handlePtr fails 
   echo TEST-ERR: $err
   set failed 1;
   }

set comp [string first $hdl_ptr $tree_get] 

if { $comp == -1} {
   echo TEST-ERR: hdrDel fails: header is not present in memory tree
   set failed 1;
}

################################################################
#
# These statement perform a deletion of the header contents.
#
################################################################
set status [catch {hdrDel $hdrone} err]
set hdr_del $err

if { $status !=0 } {
   echo TEST-ERR: hdrDel fails
   echo TEST-ERR: $err
   set failed 1;
   }

if { $hdr_del != {} } {
   echo TEST-ERR: hdrDel fails: header not deleted
   set failed 1;
}

################################################################
#
# These statements check to make sure that the memory tree is
# empty after the deletion of the header.
# 
################################################################

set status [catch {memBlocksGet} err]
set tree_get $err

if { $status !=0 } {
   echo TEST-ERR: memBlocksGet fails
   echo TEST-ERR: $err
   set failed 1;
   }

set trees [expr [llength $tree_get]-$initialMemLength]
if { $trees != 0} {
   echo TEST-ERR: memBlocksGet fails: memory tree not empty
   set failed 1;
}

#################################################################
#
# Test the hdrDelByKeyword command using the following cases:
# 
#   o The deletion of a header keyword
#   o The modification count changed in the header structure
#   o The deletion of a non-existent header keyword
#   o The deletion of a header keyword with its removal from the
#     memory tree
#
################################################################# 


################################################################# 
#
# These statements create the header
#
################################################################# 

set hdrone [hdrNew]
set $hdrone [source ../test/header_values]
set mod [exprGet $hdrone.modCnt]

################################################################# 
#
# These statements check to ensure that the keyword exist in the
# header
#
################################################################# 


set status [catch {hdrGetAsAscii $hdrone LAST} err]
set keyword $err

if {$status != 0} {
   echo TEST-ERR: hdrGetAsAscii fails
   echo TEST-ERR: $err
   set failed 1;
   }

set status [catch {hdrGetLineTotal $hdrone} err]
set lines $err

if {$status != 0} {
   echo TEST-ERR: hdrGetLineTotal fails
   echo TEST-ERR: $err
   set failed 1;
   }

set comp [string first $keyword "LAST HUSBAND last"]
if {$comp == -1} {
   echo TEST-ERR: keyword line doesn't exist
   set failed 1;
}

################################################################# 
#
# These statements delete a keyword from the header
#
################################################################# 

set status [catch [hdrDelByKeyword $hdrone LAST] err]
set hdr_del $err

if {$status != 0} {
   echo TEST-ERR: hdrDelByKeyword fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$hdr_del != {} } {
   echo TEST-ERR: keyword not deleted from header
   set failed 1;
}

set newlines [hdrGetLineTotal $hdrone]   
if {$newlines != $lines-1} {
   echo TEST-ERR: header lines not decreased by one
   set failed 1;
}

set j 0 
set k 0 
for { set i 0 } { $i < $newlines } { incr i } { 
    set status [catch {hdrGetLineCont $hdrone $i} err]

    if { $status != 0 } {
       echo TEST-ERR: hdrGetLineCont fails
       echo TEST-ERR: $err
       set failed 1;
       }
     
     set search [string first $keyword $err]

     if {$search == -1} {
        incr j} {
        incr k}
        

     } 

if { $k > 1 }  {
   echo TEST-ERR: keyword still exists
   set failed 1;
} 

##############################################################
#
# These statements check that the modification counter was  
# changed to the proper number
#
##############################################################

set status [catch {exprGet $hdrone.modCnt} err]
set modc $err

if { $status != 0 } {
   echo TEST-ERR: exprGet fails
   echo TEST-ERR: $err
   set failed 1;
       }

if {$modc != $mod+1} {
   echo TEST-ERR: modification count incorrect
   set failed 1;
}

##############################################################
#
# These statements try to delete a non-existent keyword
#
##############################################################

set status [catch {hdrDelByKeyword $hdrone DEPT} err]
set keyword $err

if { $status != 1 } {
   echo TEST-ERR: hdrGetLine fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

hdrDel $hdrone

##############################################################
#
# These statements create a header, delete a keyword and
# ensure that the keyword is no longer on the memory tree
#
##############################################################
set hdrnew_lines [hdrNew]

hdrInsWithAscii $hdrnew_lines  NAME PAUL name
hdrInsWithAscii $hdrnew_lines  MIDINT J middleinit
hdrInsWithAscii $hdrnew_lines  LAST GREEN lastname
hdrInsWithAscii $hdrnew_lines  SEX MALE sex
hdrInsWithAscii $hdrnew_lines  TITLE TECH title

set lines [hdrGetLineTotal $hdrnew_lines]

set status [catch [hdrDelByKeyword $hdrnew_lines LAST] err]
set hdr_del $err

if {$status != 0} {
   echo TEST-ERR: hdrDelByKeyword fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$hdr_del != {} } {
   echo TEST-ERR: keyword not deleted from header
   set failed 1;
}

set newlines [hdrGetLineTotal $hdrnew_lines]   
if {$newlines != $lines-1} {
   echo TEST-ERR: header lines not decreased by one
   set failed 1;
}

set j 0 
set k 0 
for { set i 0 } { $i < $newlines } { incr i } { 
    set status [catch {hdrGetLineCont $hdrone $i} err]
    if { $status != 0 } {
       echo TEST-ERR: hdrGetLineCont fails
       echo TEST-ERR: $err
       set failed 1;
       }
     
     set search [string first $keyword $err]

     if {$search == -1} {
        incr j} {
        incr k}
        

     } 

if { $k > 1 }  {
   echo TEST-ERR: keyword still exists
   set failed 1;
} 

set status [catch {memBlocksGet} err]
if { $status != 0 } {
   echo TEST-ERR: hdrCopy fails: memBlocksGet fails
   echo TEST-ERR: $err
   set failed 1;
   }

set tree_get $err
set trees [expr [llength $tree_get]-$initialMemLength]
set hdlone_ptr [handlePtr $hdrnew_lines]
set j 0
set k 0

if { $trees == 6 } {
   for { set i 0 } { $i < $trees } { incr i } {
   set item [lindex $err $i]
   set comp [string first $hdlone_ptr $item]
   if { $comp == -1 } {
      incr j} {
      incr k}

      }
      if { $k < 1 } {
         echo TEST-ERR: hdrCopy fails: handle pointer not in memory tree
	 set failed 1;
         }
      }

if { $trees != 6 } {
   echo TEST-ERR: memBlocksGet fails: incorrect number of items in memory tree
   set failed 1;
}

hdrDel $hdrnew_lines

#################################################################
#
# Test the hdrGetAsAscii command using the following cases:
#
#   o The value of the keyword obtained is correct 
#   o The obtaining of a non-existent header keyword  
#
#################################################################
  

#################################################################
#
# These statements obtain a keyword with the correct contents
#
#################################################################


set hdrone [hdrNew]
set $hdrone [source ../test/header_values]

set status [catch {hdrGetAsAscii $hdrone LAST} err]
set keyword $err

if {$status != 0}  {
   echo TEST-ERR: hdrGetAsAscii fails
   echo TEST-ERR: $err
   set failed 1;
   }

set status [catch {hdrGetLineTotal $hdrone} err]
set lines $err

if {$status != 0} {
   echo TEST-ERR: hdrGetLineTotal fails
   set failed 1;
}

set comp [string first $keyword " LAST HUSBAND last"]
if {$comp == -1} {
   echo TEST-ERR: keyword contents incorrect
   set failed 1;
}

#################################################################
#
# These statements try to obtain a non-existent keyword 
#
#################################################################

set status [catch {hdrGetAsAscii $hdrone JOB} err]
unset keyword
set keyword $err

if { $status == 0 }  {
   echo TEST-ERR: hdrGetAsAscii fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

hdrDel $hdrone

#################################################################
#
# Test the hdrGetAsDbl command using the following cases:
#
#   o The value of the keyword obtained is correct 
#   o The obtaining of a non-existent header keyword  
#   o The obtaining of a value that is not double
#
#################################################################
  

#################################################################
#
# These statements obtain a keyword with the correct contents
#
#################################################################


set hdrone [hdrNew]
set $hdrone [source ../test/Getasdbl_values]

set status [catch {hdrGetAsDbl $hdrone PAY} err]
set keyword $err

if {$status != 0}  {
   echo TEST-ERR: hdrGetAsDbl fails
   echo TEST-ERR: $err
   set failed 1;
   }

#  This test now fails as it is doing the comparison wrong.  It should
#  really compare the numbers that were input and output.  This would
#  mean a restructuring of many of the tests in this file.
#set comp [string first $keyword " PAY 35.000000 pay"]
#if {$comp == -1} {
#   echo TEST-ERR: keyword contents incorrect
#   set failed 1;
#}

#################################################################
#
# These statements try to obtain a non-existent keyword 
#
#################################################################

set status [catch {hdrGetAsDbl $hdrone TAX} err]
unset keyword
set keyword $err

if { $status == 0 }  {
   echo TEST-ERR: hdrGetAsDbl fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

#################################################################
#
# These statements obtain a keyword that is not double 
#
#################################################################
set status [catch {hdrGetAsDbl $hdrone AGE} err]
unset keyword
set keyword $err

if { $status == 0 }  {
   echo TEST-ERR: hdrGetAsDbl fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

hdrDel $hdrone

#################################################################
#
# Test the hdrGetAsInt command using the following cases:
#
#   o The value of the keyword obtained is correct 
#   o The obtaining of a non-existent header keyword  
#   o The obtaining of a value that is not an integer
#
#################################################################
  

#################################################################
#
# These statements obtain a keyword with the correct contents
#
#################################################################


set hdrone [hdrNew]
set $hdrone [source ../test/Getasint_values]

set status [catch {hdrGetAsInt $hdrone AGE} err]
set keyword $err

if {$status != 0}  {
   echo TEST-ERR: hdrGetAsInt fails
   echo TEST-ERR: $err
   set failed 1;
   }

set comp [string first $keyword " AGE 24 age"]
if {$comp == -1} {
   echo TEST-ERR: keyword contents incorrect
   set failed 1;
}

#################################################################
#
# These statements try to obtain a non-existent keyword 
#
#################################################################

set status [catch {hdrGetAsInt $hdrone KIDS} err]
unset keyword
set keyword $err

if { $status == 0 }  {
   echo TEST-ERR: hdrGetAsInt fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

#################################################################
#
# These statements obtain a keyword that is not an integer 
#
#################################################################
set status [catch {hdrGetAsInt $hdrone TAX} err]
unset keyword
set keyword $err

if { $status == 0 }  {
   echo TEST-ERR: hdrGetAsInt fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

hdrDel $hdrone

#################################################################
#
# Test the hdrGetAsLogical command using the following cases:
#
#   o The value of the keyword obtained is correct 
#   o The obtaining of a non-existent header keyword  
#   o The obtaining of a value that is not logical 
#
#################################################################
  

#################################################################
#
# These statements obtain a keyword with the correct contents
#
#################################################################


set hdrone [hdrNew]
set $hdrone [source ../test/Getaslogical_values]

set status [catch {hdrGetAsLogical $hdrone MARRIED} err]
set keyword $err

if {$status != 0}  {
   echo TEST-ERR: hdrGetAsLogical fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == 1} {
   set keyword T} {
   set keyword F}

set comp [string first $keyword " MARRIED T married"]
if {$comp == -1} {
   echo TEST-ERR: keyword contents incorrect
   set failed 1;
}

#################################################################
#
# These statements try to obtain a non-existent keyword 
#
#################################################################

set status [catch {hdrGetAsLogical $hdrone DECEASED} err]
unset keyword
set keyword $err

if { $status == 0 }  {
   echo TEST-ERR: hdrGetAsLogical fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

#################################################################
#
# These statements obtain a keyword that is not logical 
#
#################################################################
set status [catch {hdrGetAsLogical $hdrone AGE} err]
unset keyword
set keyword $err

if { $status == 0 }  {
   echo TEST-ERR: hdrGetAsLogical fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

hdrDel $hdrone

###################################################################            
#
# Test the hdrGetLine command using the following cases:
#
#   o To obtain a line with the correct value  
#   o The obtaining of a non-existent keyword line 
#
###################################################################


###################################################################
#
# These statements create the headers and also perform the
# hdrGetLine command.
#
###################################################################

set hdrone [hdrNew]
set $hdrone [source ../test/header_values]

set status [catch {hdrGetLine $hdrone SEX} err]
set get_line $err 

if { $status != 0 } {
   echo TEST-ERR: hdrGetLineno fails
   echo TEST-ERR: $err
   set failed 1;
   }

###################################################################
#
# These statements ensure that the line contains the correct value 
#
###################################################################

set comp [string first $get_line "SEX FEMALE sex"]
if { $comp != -1} { 
   echo TEST-ERR:  line contents are incorrect
   set failed 1;
   }

###################################################################
#
# These statements try to obtain a non-existent keyword  
#
###################################################################

set status [catch {hdrGetLine $hdrone DEPT} err]
set keyword $err

if { $status == 0 } {
   echo TEST-ERR: hdrGetLine fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword =={}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

hdrDel $hdrone

###################################################################            
#
# Test the hdrGetLineCont command using the following cases:
#
#   o To obtain the correct value for the specified keyword
#   o The obtaining of a non-existent keyword line 
#
###################################################################


###################################################################
#
# These statements create the headers and also perform the
# hdrGetLineCont command.
#
###################################################################

set hdrone [hdrNew]
set $hdrone [source ../test/header_values]

set status [catch {hdrGetLineCont $hdrone 3} err]
set line $err

if { $status != 0 } {
   echo TEST-ERR: hdrGetLineCont fails
   echo TEST-ERR: $err
   set failed 1;
   }
set comp [string first $line "SEX FEMALE sex"]
if {$comp != -1} {
   echo TEST-ERR: line contents are incorrect
   set failed 1;
}

###################################################################
#
# These statements try to obtain a non-existent keyword line. 
#
###################################################################

set status [catch {hdrGetLineCont $hdrone DEPT} err]
set keyword $err 

if { $status == 0 } {
   echo TEST-ERR: hdrGetLine fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

hdrDel $hdrone

###################################################################            
#
# Test the hdrGetLineno command using the following cases:
#
#   o To obtain the correct line number containing the specified
#     keyword 
#   o The obtaining of a non-existent keyword line 
#
###################################################################


###################################################################
#
# These statements create the headers and also perform the
# hdrGetLineno command.
#
###################################################################

set hdrone [hdrNew]
set $hdrone [source ../test/header_values]

set status [catch {hdrGetLineno $hdrone SEX} err]
set get_line $err 

if { $status != 0 } {
   echo TEST-ERR: hdrGetLineno fails
   echo TEST-ERR: $err
   set failed 1;
   }

###################################################################
#
# These statements ensure that the line number is the correct 
# value for the specified keyword.
#
###################################################################

if { $get_line != 3} {
   echo TEST-ERR: hdrGetLineno fails: line number are incorrect
   echo TEST-ERR: $err
   set failed 1;
   }

###################################################################
#
# These statements try to obtain a non-existent keyword line. 
#
###################################################################

set status [catch {hdrGetLine $hdrone DEPT} err]
set keyword $err 

if { $status == 0 } {
   echo TEST-ERR: hdrGetLine fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$keyword == {}} {
   echo TEST-ERR: keyword value is non-existent
   set failed 1;
}

hdrDel $hdrone

################################################################### 
#
# Test the hdrInsWithInt command using the following cases:
#
#   o Insertion of  a keyword with and without a comment into 
#     the header  
#   o Check the modification counter in the header structure to
#     ensure that it changed
#   o Insertion of an incorrect value in the header
#   o Insertion of a keyword to ensure that only one new item is
#     is added to the memory tree 
#
###################################################################




###################################################################
#
# These statements create the header. 
#
###################################################################

set hdrone [hdrNew]
set $hdrone [source ../test/header_values]
set mod [exprGet $hdrone.modCnt]

###################################################################
#
# These statements insert a line without a comment into the header 
#
###################################################################

set status [catch {hdrInsWithInt $hdrone KIDS 0 kids} err]
if {$status != 0} {
   echo TEST-ERR: hdrInsWithInt fails inserting line
   echo TEST-ERR: $err
   set failed 1;
   }

set status [catch {hdrGetLineno $hdrone KIDS} err]
set hdr_line $err

if {$hdr_line != 5} {
   echo TEST-ERR: line containing a comment not inserted
   set failed 1;
}

set status [catch {hdrGetLineTotal $hdrone} err]
set total $err

if {$status != 0} {
   echo TEST-ERR: hdrGetLineTotal fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$total != 6} {
   echo TEST-ERR: line with comment not inserted in header
   set failed 1;
}

###################################################################
#
# These statements insert a line with a comment into the header 
#
###################################################################

set status [catch {hdrInsWithInt $hdrone EMP# 116} err]
if {$status != 0} { 
   echo TEST-ERR: hdrInsWithInt fails inserting line with comment
   echo TEST-ERR: $err
   set failed 1;
   }

set status [catch {hdrGetLineno $hdrone EMP#} err]
set hdr_line $err

if {$hdr_line != 6} {
   echo TEST-ERR: line without a comment not inserted
   set failed 1;
}

set status [catch {hdrGetLineTotal $hdrone} err]
set total $err

if {$status != 0} {
   echo TEST-ERR: hdrGetLineTotal fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$total != 7} {
  echo TEST-ERR: line without comment not inserted in header
   set failed 1;
}


###################################################################
#
# These statements check to ensure that the modification count
# is correct
#
###################################################################

set status [catch {exprGet $hdrone.modCnt} err]
set modc $err

if {$status !=0} {
   echo TEST-ERR:  exprGet fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$modc != $mod+2} {
   echo TEST-ERR: modification count incorrect
   set failed 1;
}
 
###################################################################
#
# These statements try to insert an incorrect value into the header
#
###################################################################

set status [catch {hdrInsWithInt $hdrone JOB 1.6} err]
set line $err

if {$status ==0} {
   echo TEST-ERR: hdrInsWithInt inserted incorrect value into header
   echo TEST-ERR: $err
   set failed 1;
   }

set status [catch {hdrGetLineTotal $hdrone} err]
set total $err

if {$status != 0} {
   echo TEST-ERR: hdrGetLineTotal fails
   echo TEST-ERR: $err
   set failed 1;
   }

if {$total != 7} {
  echo TEST-ERR: too many lines in header
   set failed 1;
}

hdrDel $hdrone

###################################################################
#
# These statements ensure that only one new item is added to the 
# memory tree
#
###################################################################

set hdrone [hdrNew]

set $hdrone [source ../test/header_values]
set status [catch {hdrInsWithInt $hdrone AGE 47} err]
if {$status != 0} {
   echo TEST-ERR: hdrInsWithInt fails when adding to tree
   echo TEST-ERR: $err
   set failed 1;
   }

set status [catch {memBlocksGet} err]
set tree_get $err

if {$status != 0} {
   echo TEST-ERR: hdrInsWithInt fails
   echo TEST-ERR: $err
   set failed 1;
   }

set trees [expr [llength $tree_get]-$initialMemLength]
set hdlone_ptr [handlePtr $hdrone]

set j 0
set k 0

if { $trees == 8 } {
   for { set i 0 } { $i < $trees } { incr i } {
   set item [lindex $err $i]
   set comp [string first $hdlone_ptr $item]
   if { $comp == -1 } {
      incr j} {
      incr k}
      }
      if { $k < 1 } {
         echo TEST-ERR: hdrCopy fails: handle pointer not in memory tree
	 set failed 1;
         }
      }

if { $trees != 8 } {
   echo TEST-ERR: memBlocksGet fails: incorrect number of items in memory tree
   set failed 1;
}

hdrDel $hdrone

###################################################################            
#
# Test the hdrNew command using the following cases:
#
#   o The proper number of items were added to the memory tree
#   o The deletion of the header
#   o The memory tree is empty after the deletion of the header
#
###################################################################

###################################################################
# 
# These statements create the header and performs the hdrNew
# command. 
#
###################################################################

set status [catch {hdrNew} err]
set hdr $err 

if { $status != 0 } {
   echo TEST-ERR: hdrNew fails: new header not created
   echo TEST-ERR: $err
   set failed 1;
   }

set $hdr [source ../test/hdrNew_values]

###################################################################
#
# These statements ensure that the proper number of items are on 
# the memory tree.
#
###################################################################

set status [catch {memBlocksGet} err]
set tree_get $err 

if { $status != 0 } {
   echo TEST-ERR: memBlocksGet fails
   echo TEST-ERR: $err
   set failed 1;
   }

set trees [expr [llength $tree_get]-$initialMemLength]

if { $trees != 7 } {
   echo TEST-ERR: hdrNew fails:  incorrect number of items on memory tree
   set failed 1;
   }

###################################################################
#
# These statements delete the header.
#
###################################################################

set status [catch {hdrDel $hdr} err]
set hdr_del $err 

if { $status != 0 } {
   echo TEST-ERR: hdrDel fails
   echo TEST-ERR: $err
   set failed 1;
   }

if { $hdr_del != {} } {
   echo TEST-ERR: hdrDel fails:  header not deleted
   set failed 1;
   }

###################################################################
#
# These statements ensure that the memory tree is contains only 
# what it did when we first started interpreting this file
#
###################################################################

set status [catch {memBlocksGet} err]
set tree_get $err 

if { $status != 0 } {
   echo TEST-ERR: memBlocksGet fails
   echo TEST-ERR: $err
   set failed 1;
   }

set trees [llength $tree_get]

if { $trees != $initialMemLength } {
   echo TEST-ERR: memory tree is not empty
   echo TEST-ERR: $err
   set failed 1;
   }


###################################################################
# test hdrMakeLineWith{Ascii,Dbl,Int,Logical}; they should return
# strings with length 80
#
if { [string length [hdrMakeLineWithAscii key val "some comment"]] != 80 } {
   echo TEST-ERR: hdrMakeLineWithAscii   returned line with length not 80
   set failed 1;   
}
if { [string length [hdrMakeLineWithDbl key 10.12 "some comment"]] != 80 } {
   echo TEST-ERR: hdrMakeLineWithDbl     returned line with length not 80
   set failed 1;
}
if { [string length [hdrMakeLineWithInt key 10000 "some comment"]] != 80 } {
   echo TEST-ERR: hdrMakeLineWithInt     returned line with length not 80
   set failed 1;
}
if { [string length [hdrMakeLineWithLogical key T "some comment"]] != 80 } {
   echo TEST-ERR: hdrMakeLineWithLogical returned line with length not 80
   set failed 1;
}

if $failed {
   error "Failed some tests"
}

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_hdr.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_hdr"
    return 1
}
