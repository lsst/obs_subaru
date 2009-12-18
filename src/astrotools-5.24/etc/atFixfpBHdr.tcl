#
# Drive atFixfpBHdr for a run/rerun
#

proc atFixAllBHdr { run rerun {colList "1 2 3 4 5 6"} \
	{imageRoot /data/dp6.a/imaging} {keepOrig 0} {verbose 0}} {
    foreach camcol $colList {
	set pf $imageRoot/$run/$rerun/photo/fpPlan-$camcol.par
	param2Chain $pf hdr
	set startField [keylget hdr startField]
	set endField [keylget hdr endField]
	loop i $startField [expr $endField+1] {
	    foreach f "u g r i z" {
		echo [exec date]
		echo Begin atFixfpBHdr for camcol=$camcol field=$i $f
		flush stdout
		atFixfpBHdr $run $rerun $camcol $i $f $imageRoot $imageRoot $keepOrig $verbose
	    }
	}
    }
}


##############################################################################
#  atFixfpBHdr.tcl                                                 cs  16apr01
#
# Copied from:
#  atFixfpCHdr.tcl                                                 dlt-05feb01
#<AUTO EXTRACT>
#
# TCL VERB: atFixfpBHdr
#
# <HTML>
# Reads an sdss-compressed fpB file into a region, removes blank lines and
#   duplicate-keyword lines from its header, adds World Coordinate System 
#   (WCS) header keyword/value lines to the header, writes out the region
#   with its updated header into a new fpC file, and gzip-compresses the
#   new fpC file.
#
#   Returns 0 if successful.
#   Returns 1 if unsucessful.
#
#
# Required arguments:
#   run:           the run number of the fpC file
#   re-run:        the re-run number of the fpC file
#   col:           the column number of the fpC file
#   field:         the field number of the fpC file
#   filter:        the filter of the fpC file
#   inParentDir:   upper level directory in which the original fpC file resides 
#                  e.g., /data/dp3.b/data 
#                  (see Caveats)
#   outParentDir:  upper level directory in which the update fpC file will resides 
#                  (see Caveats)
#
# Optional arguments:
#   verbose:       verbosity level
#                   0 = limited running text output (default), 
#                  >0 = more running text output
#                  >1 = even more running text output
#   keepOrig:      keep a copy of the original fpBIN file after creating the updated one?
#                   0 = no (default),
#                   1 = yes 
#
# Caveats:
#   (1) atFixfpBHdr assumes that the input fpC files are sdss-compressed
#       e.g., fpB-000094-r5-0268.fit.R or .fit.gz or just .fit
#
#   (2) atFixfpBHdr assumes that the input fpB files sit in the following directory structure:
#       $inParentDir/<run#>/<reRun#>/corr/<column#>/<fpCfile>
#       e.g., /data/dp3.b/data/94/5/objcs/5/fpBIN-000094-r5-0268.fit.R
#
#   (3) atFixfpBHdr assumes that the (uncompressed) asTrans file corresponding
#       to the fpC file sits in the following directory structure:
#       $inParentDir/<run#>/<reRun#>/astrom/<asTransfile>
#       e.g., /data/dp3.b/data/94/5/astrom/asTrans-000094.fit
#
#   (4) atFixfpBHdr assumes that the outParentDir directory already exists
#
#   (5) atFixfpBHdr will output the updated fpB file into a subdirectory
#       structure under the outParentDir directory which mimics the subdirectory
#       structure under the inParentDir directory in which the original fpC file 
#       resides 
#       e.g., if inParentDir = /data/dp3.b/data and outParentDir = /home/s1/dtucker, 
#             then the updated version of...   /data/dp3.b/data/94/5/objcs/5/fpBIN-000094-r5-0268.fit.R
#             will end up in...                /home/s1/dtucker/94/5/objcs/5/fpBIN-000094-r5-0268.fit.gz
#
#   (6) the caveats implicit in the use of JRP's atWcsAddToHdr also apply here.
#
#
# Non-caveat:
#   (1) it is safe to have inParentDir = outParentDir
#       e.g., inParentDir = /data/dp3.b/data and outParentDir = /data/dp3.b/data
#             will not cause files to be overwritten.
#
#
# Examples:
#   atFixfpBHdr   94 5 3 347 i /data/dp3.b/data /home/s1/dtucker 1
#   atFixfpBHdr 1359 2 5 105 i /data/dp3.b/data /home/s1/dtucker 0
#
#
#</HTML></AUTO>
##############################################################################

ftclHelpDefine astrotoolsProcs atFixfpBHdr \
   "
   USAGE:  atFixfpCHdr   <run> <reRun> <col> <field> <filter> <inParentDir> <outParentDir> \[ keepOrig \] \[ verbose \]

           Description:
           Reads an sdss-compressed fpBIN file into a region, removes blank lines and
           duplicate-keyword lines from its header, adds World Coordinate System 
           (WCS) header keyword/value lines to the header, writes out the region
           with its updated header into a new fpBIN file, and gzip-compresses the
           new fpBIN file.

           Returns 0 if successful.
           Returns 1 if unsuccessful.

           Required arguments:
           run:           the run number of the fpBIN file
           re-run:        the re-run number of the fpBIN file
           col:           the column number of the fpBIN file
           field:         the field number of the fpBIN file
           filter:        the filter of the fpBIN file
           inParentDir:   upper level directory in which the original fpBIN file resides 
                          e.g., /data/dp3.b/data 
           outParentDir:  upper level directory in which the update fpBIN file will resides 

           Optional arguments:
           keepOrig:      keep a copy of original fpBIN file after creating the updated one?
                           0 = no (default),
                           1 = yes 
           verbose:       verbosity level
                           0 = limited running text output (default), 
                          >0 = more running text output
                          >1 = even more running text output

           For more details, see HTML documentation.
   "

proc atFixfpBHdr { run reRun col field filter inParentDir outParentDir {keepOrig 0} {verbose 0} } {


    # find binned frame name; does it exist?...
    set imageDir       $inParentDir/$run/$reRun/objcs/$col
    if { ![file isdirectory $imageDir] } {
	echo "Error:  $imageDir is NOT a directory!"
	echo "        Exiting!!!"
	return 1
    }
    set imageName      fpBIN-[format %06d $run]-$filter$col-[format %04d $field].fit
    set fullImageName  $imageDir/$imageName
    if { \
	    (![file exists $fullImageName.R]) && \
	    (![file exists $fullImageName]) && \
	    (![file exists $fullImageName.gz]) \
	} {
	echo "Error:  neither $fullImageName, $fullImageName.gz, nor $fullImageName.R exist!"
	echo "        Exiting!!!"
	return 1
    }

    # find asTrans file for the corrected frame; does it exist?...
    set astromDir      $inParentDir/$run/$reRun/astrom
    if { ![file isdirectory $astromDir] } {
	echo "Error:  $astromDir is NOT a directory!"
	echo "        Exiting!!!"
	return 1
    }
    set astromName     asTrans-[format %06d $run].fit
    set fullAstromName $astromDir/$astromName
    if { ![file exists $fullAstromName] } {
	echo "Error:  $fullAstromName does NOT exist!"
	echo "        Exiting!!!"
	return 1
    }


    # If it does not exist, create output directory...
    set outImageDir    $outParentDir/$run/$reRun/objcs/$col
    if { ![file isdirectory $outParentDir] } {
	echo "Error:  $outParentDir is not a directory!"
	echo "        Exiting!!!"
	return 1
    }
    if { ![file exists $outImageDir] } {
	if {$verbose > 0} {
	    echo "$outImageDir does not yet exist"
	    echo "Creating..."
	    flush stdout
	}
        if { [catch { exec mkdir -p $outImageDir }] } {
            echo "Error:  \"mkdir -p $outImageDir\" failed"
            echo "        Exiting!!!"
            return 1
        }
    }


    # read corrected frame into a region...
    if {[file exists $fullImageName.R]} {
	set fd [open "|[envscan \$ASTROTOOLS_DIR]/bin/sdssdecompress \
		-c $fullImageName.R"]
	set region [regReadAsFits [regNew] -pipe $fd]
	close $fd
	if {$verbose > 0} {echo Renaming original $imageName.R to $imageName.orig.R}
	exec mv $fullImageName.R $fullImageName.orig.R
    } elseif {[file exists $fullImageName.gz]} {
	exec gunzip $fullImageName.gz
	set region [regReadAsFits [regNew] $fullImageName]
	if {$verbose > 0} {echo Renaming original $imageName.gz to $imageName.orig.gz}
	exec mv $fullImageName $fullImageName.orig
	exec gzip $fullImageName.orig
    } elseif {[file exists $fullImageName]} {
	set region [regReadAsFits [regNew] $fullImageName]
	if {$verbose > 0} {echo Renaming original $imageName to $imageName.orig}
	exec mv $fullImageName $fullImageName.orig
    }

    # what's the original header look like?
    if {$verbose > 1} {
	echo; echo "Original header for $imageName:"; echo 
	hdrPrint $region.hdr
	echo; echo 
	flush stdout
    }


    # clean up the header of the binned frame...
    cleanBinnedFrameHeader $region.hdr $verbose


    # grab the appropriate TRANS structure for the corrected frame...
    if { [catch {set camrow [hdrGetAsInt $region.hdr CAMROW]}] } {
	echo "Error:  Could not extract CAMROW from image header!"
	echo "        Exiting!!!"
	return 1
    }
    if { [catch {set camcol [hdrGetAsInt $region.hdr CAMCOL]}] } {
	echo "Error:  Could not extract CAMCOL from image header!"
	echo "        Exiting!!!"
	return 1
    }
    set ccd $camrow$camcol
    set stuff      [atReadTrans $fullAstromName $ccd]
    set transChain [keylget stuff chain]
    set node       [keylget stuff node]
    set incl       [keylget stuff incl]
    set smallChain [chainSearch $transChain "{ id == $field }"]
    if { [chainSize $smallChain] != 1 } {
	echo "Error:  Problem extracting TRANS information!"
	echo "        Exiting!!!"
	return 1
    }
    set trans      [chainElementGetByPos $smallChain HEAD]


    # If there exists no header line for the EQUINOX keyword, create one;
    #  if the line does exist, replace it with a double-precision-valued EQUINOX line.
    if { [catch {set equinoxLineno [hdrGetLineno $region.hdr EQUINOX]}] } {
	hdrInsWithDbl $region.hdr EQUINOX 2000.0 "Equinox of the Coordinates (Julian Years)"
    } else {
	set equinoxLine [hdrGetLineCont $region.hdr $equinoxLineno]
	regsub -all {'} [string trim [lindex $equinoxLine 2]] {} equinox
	set newContents [hdrMakeLineWithDbl EQUINOX $equinox "Equinox of the Coordinates (Julian Years)"]
	hdrReplaceLine $region.hdr $equinoxLineno $newContents
    }


    # Delete any existing WCS info from the header
    foreach key "RADECSYS CTYPE1 CTYPE2 CUNIT1 CUNIT2 CRPIX1 CRPIX2 CRVAL1 CRVAL2 CD1_1 CD1_2 CD2_1 CD2_2" {
	catch "hdrDelByKeyword $region.hdr $key"
    }

    # Append World Coordinate System info to the header of the corrected frame
    atWcsAddToHdr $trans $region.hdr $node $incl


    # what's the newly updated header look like?
    if {$verbose > 1} {
	echo; echo "Updated header for $imageName:"; echo 
	hdrPrint $region.hdr
	echo; echo 
	flush stdout
    }


    # write out and gzip-compress the updated corrected frame...
    regWriteAsFits $region $outImageDir/$imageName
    if {[file exists $outImageDir/$imageName.gz]} {
	if {$verbose > 0} {
	    echo "$outImageDir/$imageName.gz already exists..."
	    echo "renaming original as $outImageDir/$imageName.gz~"
	    flush stdout
	}
	exec mv $outImageDir/$imageName.gz $outImageDir/$imageName.gz~
    }
    exec gzip $outImageDir/$imageName


    # delete any of the original copies of the fpBIN file (if so requested)...
    if {$keepOrig == 0} {
	if {$verbose > 0} {echo Deleting any of the original copies of $imageName}
	if {[file exists $fullImageName.orig]}    {exec rm $fullImageName.orig}
	if {[file exists $fullImageName.orig.R]}  {exec rm $fullImageName.orig.R}
	if {[file exists $fullImageName.orig.gz]} {exec rm $fullImageName.orig.gz}
    }


    # final clean up...

    handleDel $trans
    chainDel $smallChain
    genericChainDel $transChain
    regDel $region

    return 0

}

proc cleanBinnedFrameHeader { hdr {verbose 0} } {

#    # keywords to keep...
#    set keywordKeepList "TAI RA DEC SPA IPA IPARATE AZ ALT FOCUS DATE-OBS TAIHMS COMMENT \
#	    BUNITS ORIGIN TELESCOP RUN FRAME CCDLOC STRIPE STRIP FLAVOR SYS_SCN \
#	    EQNX_SCN NODE INCL XBORE YBORE FIELD OBJECT EXPTIME EQUINOX SYSTEM \
#	    CCDMODE C_OBS COLBIN ROWBIN NLINES DAVERS CAMCOL CAMROW FILTER AMPL \
#	    AMPR BADLINES SOFTBIAS BUNIT FILTER CAMCOL VERSION DERV_VER \
#	    ASTR_VER ASTRO_ID BIAS_ID FRAME_ID KO_VER PS_ID RADECSYS CTYPE1 \
#	    CTYPE2 CUNIT1 CUNIT2 CRPIX1 CRPIX2 CRVAL1 CRVAL2 CD1_1 CD1_2 CD2_1 CD2_2"


    # keywords to kill...
    set keywordKillList "CCDSECL OVERSECL BIASSECL DATASECL CCDSECR DATASECR BIASSECR \
	    OVERSECR AMPL AMPR NLINES"


    # loop backwards through the header, removing empty lines, lines with 
    #  keywords marked for deletion, and duplicate-keyword lines...

    set keywordFoundList ""

    set ntot [hdrGetLineTotal $hdr]

    loop i 0 $ntot {
	set j [expr $ntot - $i - 1]
	set line [hdrGetLineCont $hdr $j]
	set tmp_line [string trim $line]
	if { $tmp_line == "" } {
	    hdrDelByLine $hdr $j
	} else {
	    set keyword [string trim [string range $tmp_line 0 7]]
	    if { [lsearch $keywordKillList  $keyword] >= 0 } {
		if {$verbose > 0} {
		    echo $keyword is a bad keyword:  deleting line in header!
		    flush stdout
		}
		hdrDelByLine $hdr $j
	    } elseif { [lsearch $keywordFoundList $keyword] >= 0 } {
		if {$verbose > 0} {
		    echo $keyword is a duplicate keyword:  deleting line in header!
		    flush stdout
		}
		hdrDelByLine $hdr $j
	    }
	    if {$keyword != "COMMENT" && $keyword != "HISTORY"} {
		lappend keywordFoundList $keyword
	    }
	}
    }

    unset keywordKillList keywordFoundList

    return 0

}

