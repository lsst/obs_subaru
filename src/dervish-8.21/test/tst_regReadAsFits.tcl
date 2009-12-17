#
#  Test the regReadAsFits command using the following cases -
#     o The current region is too small to hold the FITS data.
#     o The current region is too big to hold the FITS data.
#     o The current region is just right to hold the FITS data.
#
#  Do each of the above for regions that do and do not have a mask.
#  Also do the above tests for different region types.
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

set reg [regNew]
set stat [catch {regReadAsFits $reg small.fits} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 0" $errVal
}

regDel $reg
set reg [regNew 1024 1024]
set stat [catch {regReadAsFits $reg small.fits} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 1024 by 1024" $errVal
}

set stat [catch {regReadAsFits $reg small.fits} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 512 by 512" $errVal
}

regDel $reg
set reg [regNew 10 10 -mask]
set stat [catch {regReadAsFits $reg small.fits} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 10 by 10 with mask" $errVal
}

regDel $reg
set reg [regNew 1024 1024 -mask]
set stat [catch {regReadAsFits $reg small.fits} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 1024 by 1024 with mask" \
       $errVal
}

set stat [catch {regReadAsFits $reg small.fits} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 512 by 512 with mask." \
       $errVal
}

# transform an S16 FITS file into a U16 region.
regDel $reg
set reg [regNew -type U16]
set stat [catch {regReadAsFits $reg small.fits -keeptype} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 0, type U16" $errVal
}

# transform an S16 FITS file into a U8 region.
regDel $reg
set reg [regNew -type U8]
set stat [catch {regReadAsFits $reg small.fits -keeptype} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 0, type U8" $errVal
}

# transform an S16 FITS file into a S8 region.
regDel $reg
set reg [regNew -type S8]
set stat [catch {regReadAsFits $reg small.fits -keeptype} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 0, type S8" $errVal
}

# transform an S16 FITS file into a U32 region.
regDel $reg
set reg [regNew -type U32]
set stat [catch {regReadAsFits $reg small.fits -keeptype} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 0, type U32" $errVal
}

# transform an S16 FITS file into a S32 region.
regDel $reg
set reg [regNew -type S32]
set stat [catch {regReadAsFits $reg small.fits -keeptype} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 0, type S32" $errVal
}

# transform an S16 FITS file into a FL32 region.
regDel $reg
set reg [regNew -type FL32]
set stat [catch {regReadAsFits $reg small.fits -keeptype} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits and region of size 0, type FL32" $errVal
}
regDel $reg


##############################################################################
# Now write a FITS files out, then read it back and compare.
# Let's loop over all the types.
#
foreach testtype {S8 U8 S16 U16 S32 U32 FL32} {
    set reg1 [regNew -type $testtype 10 10] 
    loop row 0 10 {
	loop col 0 10 {
	    regPixSetWithDbl $reg1 $row $col [expr $row+$col]
	}
    }
    regWriteAsFits $reg1 junk.fits
    set reg2 [regReadAsFits [regNew -type $testtype] junk.fits]
    set stat [regComp $reg1 $reg2]
    if { $stat != "same"} {
	fail "Error when comparing regions after write/read to disk" $testtype
    }
    regDel $reg1
    regDel $reg2
}
exec rm junk.fits


##############################################################################
# Now test the reading and writing through pipes.
#
set reg [regNew]
set stat [catch {regReadAsFits $reg small.fits} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits during initial read to test pipes" $errVal
}
set stat [catch {shPipeOut compress -file small.fits.Z} errVal]
if { $stat == 1} {
   fail "Error with shPipeOut and compress" $errVal
}
set writePipe $errVal
set stat [catch {regWriteAsFits $reg $writePipe -pipe} errVal]
if { $stat == 1} {
   fail "Error with regWriteAsFits while trying to write a compressed file" $errVal
}
close $writePipe

# Now read the compressed file back in and pipe it through uncompress
set reg2 [regNew]
set stat [catch {shPipeIn uncompress -file small.fits.Z} errVal]
if { $stat == 1} {
   fail "Error with shPipeIn and uncompress" $errVal
}
set readPipe $errVal
set stat [catch {regReadAsFits $reg2 $readPipe -pipe} errVal]
if { $stat == 1} {
   fail "Error with regReadAsFits during initial read to test pipes" $errVal
}
close $readPipe

# Now compare the original region data with the new one to make sure they
# are the same.
set stat [regComp $reg $reg2]
if { $stat != "same"} {
   fail "Error when doing regComp, regions are not the same" $errVal
}
regDel $reg
regDel $reg2

if $failed {
   error "Failed some regReadAsFits tests"
}

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_regReadAsFits.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_regReadAsFits"
    return 1
}









