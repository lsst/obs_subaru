
##########################################################################
#
# hg2Save 
#
##########################################################################

set hg2SaveArgs {
  { hg2Save "save the hg2 in a file, use hg2Read to read it back\nreturns name of the region" }
  { <hg2>      STRING "" hg2 "the hg2" }
  { <fileName> STRING "" fileName "file where hg2 is to be saved" }
}

ftclHelpDefine astrotools hg2Save [shTclGetHelpInfo hg2Save $hg2SaveArgs]

proc hg2Save { args } {
#
# access help and parse inputs
#
    upvar #0 hg2SaveArgs formal_list

    if { [shTclParseArg $args $formal_list hg2Save] == 0 } { return }

#
# start
#
    set reg [handleBind [handleNew] [exprGet $hg2.reg] REGION]
    hdrInsertLine $reg.hdr 0 \
	    [hdrMakeLineWithDbl XMIN [exprGet $hg2.xmin]]
    hdrInsertLine $reg.hdr 0 \
	    [hdrMakeLineWithDbl XMAX [exprGet $hg2.xmax]]
    hdrInsertLine $reg.hdr 0 \
	    [hdrMakeLineWithDbl DELX [exprGet $hg2.delx]]
    hdrInsertLine $reg.hdr 0 \
	    [hdrMakeLineWithDbl YMIN [exprGet $hg2.ymin]]
    hdrInsertLine $reg.hdr 0 \
	    [hdrMakeLineWithDbl YMAX [exprGet $hg2.ymax]]
    hdrInsertLine $reg.hdr 0 \
	    [hdrMakeLineWithDbl DELY [exprGet $hg2.dely]]
    hdrInsertLine $reg.hdr 0 \
	    [hdrMakeLineWithInt NX [exprGet $hg2.nx]]
    hdrInsertLine $reg.hdr 0 \
	    [hdrMakeLineWithInt NY [exprGet $hg2.ny]]
    regWriteAsFits $reg $fileName
    return $reg
}

##########################################################################
#
# hg2Read
#
##########################################################################

set hg2ReadArgs {
  { hg2Read "read an hg2 from a file, use hg2Save to save it to a file\noutput is hg2 handle" }
  { <fileName> STRING "" fileName "file to read hg2" }
}

ftclHelpDefine astrotools hg2Read [shTclGetHelpInfo hg2Read $hg2ReadArgs ]

proc hg2Read { args } {
#
# access help and parse inputs
#
    upvar #0 hg2ReadArgs formal_list

    if { [shTclParseArg $args $formal_list hg2Read] == 0 } { return }
#
# start
#
    global reg
    set hg2 [genericNew HG2]
    set reg [regReadAsFits [regNew] $fileName]
    handleSet $hg2.reg [handlePtr $reg]
    #handleBindFromHandle $hg2.reg $reg
    handleSet $hg2.xmin [hdrGetAsDbl $reg.hdr XMIN]
    handleSet $hg2.xmax [hdrGetAsDbl $reg.hdr XMAX]
    handleSet $hg2.delx [hdrGetAsDbl $reg.hdr DELX]
    handleSet $hg2.ymin [hdrGetAsDbl $reg.hdr YMIN]
    handleSet $hg2.ymax [hdrGetAsDbl $reg.hdr YMAX]
    handleSet $hg2.dely [hdrGetAsDbl $reg.hdr DELY]
    handleSet $hg2.nx   [hdrGetAsInt $reg.hdr NX]
    handleSet $hg2.ny   [hdrGetAsInt $reg.hdr NY]
    return $hg2
}

proc atRegHistInit {reg min max nbin} {
    global hg1 hg2
    set hg1 [hgNew]
    hgDefine $hg1 -min $min -max $max -nbin $nbin
    set hg2 [hgNew]
    hgDefine $hg2 -min $min -max $max -nbin $nbin

    hgReg $hg1 $reg
    
    set accum 0
    loop ibin 0 [exprGet $hg1.nbin] {
	set accum [expr $accum + [exprGet $hg1.contents<$ibin>]]
	handleSet $hg2.contents<$ibin> $accum
    }
    echo accum=$accum
    loop ibin 0 [exprGet $hg1.nbin] {
	handleSet $hg2.contents<$ibin> \
		[expr [exprGet $hg2.contents<$ibin>]/[double $accum]]
    }
    return $hg2
}
