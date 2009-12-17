#
# FILE: 
# -----
#    nextFrameProc.tcl
#
# DESCRIPTION:
# ------------
#    The TCL procedures in this file provide support for reading a 
#    sequence of FITS files into a "frame".  A frame is defined as 
#    one FITS file (or possibly more than one) plus some overlap 
#    appended from the start of the next FITS file in the sequence.    
#
#    The procedures of interest are:
#        nextFrameInit - Initialize and setup state information.
#        nextFrameGet  - Read next FITS file and overlap into the frame.
#        nextEOF       - Check if all FITS files have been read.
#
#    NOTE:
#      - The sequence is complete when the second-to-last FITS file is
#        read (i.e., nextEOF returns 1).  The last file is _not_ read
#        in completely - only enough to fill the overlap. 
#
#      - For general use, the <frameRows> parameter to nextFrameInit
#        should be equal to <overlapRows> plus the number of rows in
#        each FITS file.
#  
#
# EXAMPLE USAGE:
# --------------
#    Assume there is a sequence of ten FITS files, each containing 100 rows 
#    and 100 columns.  They are located in the current directory and named 
#    as follows:  
#        test-1.fits 
#        test-2.fits
#        ...
#        test-10.fits
#
#    If you would like to process this sequence of files with an overlap of
#    10 rows, then the frame size would be 110 (i.e., number of rows in FITS 
#    file plus number of overlap rows).  The following TCL line would 
#    initialize the state: 
#
#        dervish> set state [nextFrameInit . test- 1 10 .fits 10 110 0]
#
#    Then the nextFrameGet and nextEOF proc's could be used to read each 
#    FITS file in the sequence and determine when the last file has been read:
#
#        dervish> set region [keylget state frameHandle]
#        dervish> set state [nextFrameGet $state]
#        dervish> if { ![nextEOF state] } then {...do something with $region...}
# 
#    A loop in a TCL script to do this might look like:
#        
#        set state [nextFrameInit . test- 1 10 .fits 10 110 0]
#        set region [keylget state frameHandle]  # Save frame region handle
#        set state [nextFrameGet $state]         # Get first frame
#        while { ![nextEOF $state] } {           # Loop over FITS file sequence
#          ...do something with $region...
#          set state [nextFrameGet $state]       # Get next frame
#        }
#
#
#-----------------------------------------------------------------------------

#------------------------------------------------------------------------------
#
# PROC: nextFrameInit
#
ftclHelpDefine contrib nextFrameInit \
"USAGE: nextFrameInit <dirName> <baseName> <beginFile> <endFile> <suffixName> <overlapRows> <frameRows> <nfDebug>
Initializes the keyed list that controls nextFrame."

proc nextFrameInit {dirName baseName beginFile endFile suffixName overlapRows frameRows nfDebug} {
    global thisFileArray
    set state ""
    keylset state nfDebug $nfDebug
    keylset state dirName $dirName
    keylset state baseName $baseName
    keylset state beginFile $beginFile
    keylset state endOfTape 0
    keylset state endFile $endFile
    keylset state suffixName $suffixName
    set fni $dirName/$baseName$suffixName
    set thisFileArray($fni) $beginFile
    if {$nfDebug>0} {
	echo nextFrameInit:  fni is $fni
	echo nextFrameInit:  thisFileArray is $thisFileArray($fni)
    }
    # keylset state maskSuffixName $maskSuffixName
    # keylset state bufferHandle [set bufferHandle [regNew -mask -type FL32] ]
    keylset state bufferHandle [set bufferHandle [regNew -type FL32] ]
    keylset state bufferFirstPt 0
    set state [bufferFill $state]
    keylset state bufferFirstPt \
	    [set bufferFirstPt [keylget state bufferFirstPt]]
    set temp [regInfoGet $bufferHandle]
    keylset state bufferSize [set bufferSize [keylget temp nrow]]
    keylset state ncol [set ncol [keylget temp ncol]]
    keylset state overlapHandle \
	    [set overlapHandle [regNew $overlapRows $ncol -type FL32]]
    # keylset state overlapHandle \
	    #[set overlapHandle [regNew $overlapRows $ncol -mask -type FL32]]
    keylset state overlapLastPt [set overlapLastPt -1]
    keylset state overlapRows $overlapRows
    keylset state frameHandle [set frameHandle \
	[regNew $frameRows $ncol -type FL32 -mask ]]
    #    keylset state frameHandle [set frameHandle \
	    #	[regNew $frameRows $ncol -mask -type FL32]]
    keylset state frameRows	$frameRows
    if {$nfDebug>1} then {
	echo --- nextFrameInit:  current state:
	echo $state
    }
    
    
    return $state
}

#------------------------------------------------------------------------------
#
# PROC: bufferFill
#
ftclHelpDefine contrib bufferFill \
"USAGE:  bufferFill <state>
Loads the next file into the buffer, as defined in state.  This is called by
nextFrameInit and nextFrameGet."

proc bufferFill {state} {
    global thisFileArray
    set nfDebug [keylget state nfDebug]
    set dirName [keylget state dirName]
    set baseName [keylget state baseName]
    set beginFile [keylget state beginFile]
    set endFile   [keylget state endFile]
    set suffixName [keylget state suffixName]
    set thisFile  $thisFileArray($dirName/$baseName$suffixName)
    #    set maskSuffixName [keylget state maskSuffixName]
    set bufferHandle [keylget state bufferHandle]
    ### set bufferFirstPt [keylget state bufferFirstPt]
    if {$nfDebug>0} then {
	echo --- bufferFill: thisFile=$thisFile endFile=$endFile
    }
    if {$thisFile<=$endFile} then {
	set fullFileName $dirName/$baseName[format %06d $thisFile]$suffixName
	if {$nfDebug>0} then {
	    echo --- bufferFill: Reading $fullFileName
	}
	hdrFree $bufferHandle.hdr
	set check [ catch {regReadAsFits $bufferHandle $fullFileName}]
	if {$check} then {
	    if {$nfDebug>0} then {
		echo --- bufferFill: No more files on disk
	    }
	    keylset state endOfTape 1
	    return $state
	}

	if {$nfDebug>0} then {
	}
	# get the mask, too
	#	set mask [maskNew]
	#	maskReadAsFits $mask $dirName/$baseName$thisFile$maskSuffixName
	#	maskCopy $bufferHandle $mask
	#	maskDel $mask

	# done getting the mask
	set fni $dirName/$baseName$suffixName
        incr thisFileArray($fni)

	keylset state bufferFirstPt 0
    } else {
	if {$nfDebug>0} then {
	    echo --- bufferFill: End of requested files
	}
	keylset state endOfTape 1
    }
    
    return $state
}

#------------------------------------------------------------------------------
#
# PROC: nextFrameGet
#
ftclHelpDefine contrib nextFrameGet \
	"USAGE:  set state \[nextFrameGet \$state\]

Loads up the frame with the next set of rows."

proc nextFrameGet {state} {
    
    set nfDebug       [keylget state nfDebug]
    ### set dirName       [keylget state dirName]
    ### set baseName      [keylget state baseName]
    set beginFile     [keylget state beginFile]
    ### set endFile       [keylget state endFile]
    ### set suffixName    [keylget state suffixName]
    set bufferHandle  [keylget state bufferHandle]
    set bufferFirstPt [keylget state bufferFirstPt]
    set bufferSize    [keylget state bufferSize]
    set ncol          [keylget state ncol]
    set overlapHandle [keylget state overlapHandle]
    set overlapLastPt [keylget state overlapLastPt]
    set overlapRows   [keylget state overlapRows]
    set frameHandle   [keylget state frameHandle]
    set frameRows     [keylget state frameRows]

    # Copy the overlap to the frame
    if {$overlapLastPt > -1} then {
	copyRows $overlapHandle 0 $overlapLastPt $frameHandle 0 \
		$overlapLastPt $ncol
	set frameFirstPt [expr $overlapLastPt+1]
    } else {
	set frameFirstPt 0
    }


    # Copy the buffer to the frame

    while {$frameFirstPt < $frameRows} {
	if {$bufferFirstPt > [expr $bufferSize-1]} then {
	    set state [bufferFill $state]
	    if { [nextEOF $state]} then {
		return $state
	    }
	    set bufferFirstPt 0
	}
	set nLeftInBuffer [expr $bufferSize-$bufferFirstPt]
	set nLeftInFrame  [expr $frameRows-$frameFirstPt]
	set nToCopy [min $nLeftInBuffer $nLeftInFrame]
	set bufferLastPt [expr $bufferFirstPt+$nToCopy-1]
	set frameLastPt  [expr $frameFirstPt+$nToCopy-1]

	copyRows $bufferHandle $bufferFirstPt $bufferLastPt \
		$frameHandle  $frameFirstPt  $frameLastPt $ncol


	set bufferFirstPt [expr $bufferLastPt+1]
	set frameFirstPt  [expr $frameLastPt+1]
    }
    keylset state bufferFirstPt $bufferFirstPt
    
    # Set aside the last few rows for the next time
    set frameTailBegin [expr $frameRows-$overlapRows]
    set frameTailEnd   [expr $frameRows-1]
    set overlapTailBegin 0
    set overlapTailEnd [expr $overlapRows-1]
    copyRows $frameHandle   $frameTailBegin   $frameTailEnd \
	    $overlapHandle $overlapTailBegin $overlapTailEnd $ncol
    set overlapLastPt [expr $overlapRows-1]
    keylset state overlapLastPt $overlapLastPt
    if {$nfDebug>1} then {
	echo --- nextFrame: bufferFirstPt=$bufferFirstPt 
	echo --- nextFrame: overlapLastPt=$overlapLastPt
    }
    # Do the header bookkeeping
    hdrCopy $bufferHandle.hdr $frameHandle.hdr
    if {$nfDebug>2} {
	echo --- nextFrame: hdrPrint of bufferHandle.hdr:
	hdrPrint $bufferHandle.hdr
	echo --- nextFrame: hdrPrint of frameHandle.hdr:
	hdrPrint $frameHandle.hdr
    }
    return $state
}

#------------------------------------------------------------------------------
#
# PROC: nextEOF
#
ftclHelpDefine contrib nextEOF \
"USAGE:  nextEOF state

returns 1 if state says it is out of input files; 0 otherwise."

proc nextEOF {state} {
  set nfDebug [keylget state nfDebug]
  set endOfTape [keylget state endOfTape]
  if {$endOfTape == 1} then {
	if {$nfDebug>0} then {
	  echo --- nextEOF: EOF found
	}
	return 1
  } else {
	if {$nfDebug>0} then {
	  echo --- nextEOF: No EOF yet
	}
	return 0
  }
}

#------------------------------------------------------------------------------
#
# PROC: copyRows
#
ftclHelpDefine contrib copyRows \
"USAGE:  copyRows fromHandle fromFirst fromLast toHandle toFirst toLast ncol

Copy the pixel values from fromHandle to toHandle, from *First to *Last 
rows inclusive, for all of the columns."

proc copyRows {fromHandle fromFirst fromLast toHandle toFirst toLast ncol} {
    #  echo in copyRows:
    #  echo FROM: first=$fromFirst
    #  echo FROM:  last=$fromLast
    #  echo FROM:   reg=$fromHandle
    #  echo   TO: first=$toFirst
    #  echo   TO:  last=$toLast
    #  echo   TO:   reg=$toHandle
    set fromNrow [expr $fromLast-$fromFirst+1]
    # set fromSubReg [subRegNew $fromHandle $fromNrow $ncol $fromFirst 0 -mask]
    set fromSubReg [subRegNew $fromHandle $fromNrow $ncol $fromFirst 0]
    set toNrow [expr $toLast-$toFirst+1]
    #  set toSubReg [subRegNew $toHandle $toNrow $ncol $toFirst 0 -mask]
    set toSubReg [subRegNew $toHandle $toNrow $ncol $toFirst 0]
    #  echo fromSubReg $fromSubReg
    #  echo toSubReg $toSubReg
    regPixCopy $fromSubReg $toSubReg
    #  regMaskCopy $fromSubReg $toSubReg
    regDel $fromSubReg
    regDel $toSubReg
    return
}

ftclHelpDefine contrib nextFieldInit \
"
USAGE:  nextFieldInit <filters> <dirName> <baseName> <beginFile> <endFile> <suffixName> <overlapRows> <frameRows> <nfDebug>
Initialize the list of keyed lists that controls nextFieldGet
"
proc nextFieldInit {filters dirName baseName beginFile endFile suffixName overlapRows frameRows nfDebug} {
    set fsr \{$filters\}
    foreach filter $filters {
	lappend fsr [nextFrameInit \
		$dirName $baseName $beginFile $endFile $suffixName \
		$overlapRows $frameRows $nfDebug]
    }
    return $fsr
}

ftclHelpDefine contrib nextFieldGet \
"
USAGE: set fieldState \[nextFieldGet \$fieldState\]

Loads up the frames with the next set of rows
"
proc nextFieldGet {fieldState } {
    global filterList

    set filterList  [lindex $fieldState 0]
    set fsr \{$filterList\}

    set iFilter 1
    foreach  filter $filterList {
	lappend fsr [nextFrameGet [lindex $fieldState $iFilter]]
	incr iFilter
    }
    return $fsr
}

ftclHelpDefine contrib regFromFilter \
"
USAGE:  regFromFilter filter fieldState
Return the region handle for the frame in fieldState that corresponds to the filter
"
proc regFromFilter {filter fieldState} {
    set filterList [lindex $fieldState 0]
    if {[set index [lsearch $filterList $filter]] < 0} {
	return h0
    }
    set state [lindex $fieldState [expr 1+$index]]
    return [keylget state frameHandle]
}








