#
#  Test the maskReadAsFits command using the following cases -
#     o The current mask is too small to hold the FITS data.
#     o The current mask is too big to hold the FITS data.
#     o The current mask is just right to hold the FITS data.
#
#  Test the reading and writing through pipes.
#
#  NOTE: many many many more tests could be done.
#
set failed 0

#
# A proc to return failure
#
proc fail { cmd err } {
   echo TEST-ERR: $cmd
   regsub -all {ERROR : } [string trimleft $err] "TEST-ERR: " err
   echo TEST-ERR: $err
   echo TEST-ERR:
   global failed; set failed 1
}

set mask [maskNew 512 512]
maskSetWithNum $mask -and 72
set stat [catch {maskWriteAsFits $mask small.mask} errVal]
if { $stat == 1} {
   fail "Error with maskWriteAsFits during initial create" $errVal
}

maskDel $mask
set mask [maskNew]
set stat [catch {maskReadAsFits $mask small.mask} errVal]
if { $stat == 1} {
   fail "Error with maskReadAsFits and mask of size 0" $errVal
}

maskDel $mask
set mask [maskNew 1024 1024]
set stat [catch {maskReadAsFits $mask small.mask} errVal]
if { $stat == 1} {
   fail "Error with maskReadAsFits and mask of size 1024 by 1024" $errVal
}

set stat [catch {maskReadAsFits $mask small.mask} errVal]
if { $stat == 1} {
   fail "Error with maskReadAsFits and mask of size 512 by 512" $errVal
}
maskDel $mask

##############################################################################
# Now test the reading and writing through pipes.
#
set mask [maskNew]
set stat [catch {maskReadAsFits $mask small.mask} errVal]
if { $stat == 1} {
   fail "Error with maskReadAsFits during initial read to test pipes" $errVal
}
set stat [catch {shPipeOut compress -file small.mask.Z} errVal]
if { $stat == 1} {
   fail "Error with shPipeOut and compress" $errVal
}
set writePipe $errVal
pid $writePipe
set stat [catch {maskWriteAsFits $mask $writePipe -pipe} errVal]
if { $stat == 1} {
   fail "Error with maskWriteAsFits while trying to write a compressed file" $errVal
}
# Now read the compressed file back in and pipe it through uncompress
#     Note: it seems like pipes are asynch. If the following sleep is not
#           present then repeated running this test produces errors.  kurt.
sleep 2
set mask2 [maskNew]
set stat [catch {shPipeIn uncompress -file small.mask.Z} errVal]
if { $stat == 1} {
   fail "Error with shPipeIn and uncompress" $errVal
}
set readPipe $errVal
set stat [catch {maskReadAsFits $mask2 $readPipe -pipe} errVal]
if { $stat == 1} {
   fail "Error with maskReadAsFits during uncompress read to test pipes" $errVal
}
maskDel $mask
maskDel $mask2
exec rm small.mask small.mask.Z
if $failed {
   error "Failed some maskReadAsFits tests"
}

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_maskIo.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_maskIo"
    return 1
}
