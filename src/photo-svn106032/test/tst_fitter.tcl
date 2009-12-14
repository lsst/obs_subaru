#
# Basic test of fitter
#
proc testFitModel {type ampl size aratio pa psf} {
    set ret 0

    echo "Testing $type model"

    set skysig 1
    set tsize -1;			# not used
    if { $type == "deV" } {
       set reg [makeDevModel $ampl $size $aratio $pa $psf]
    } elseif { $type == "exp" } {
       set reg [makeExpModel $ampl $size $aratio $pa $psf]
    } else {
       error "Unknown model type: $type"
    }
    echo "model $type is [exprGet $reg.nrow]x[exprGet $reg.ncol]"

    set levels [vectorExprEval 10]
    set br [binregionNewFromConst 0]
    set objects [regFinder $reg $levels $br]
    set objs [objectToObject1ChainConvert $objects 0 $reg $br]
    set obj [chainElementRemByPos $objs HEAD]
    set dgpsf [dgpsfNew -sigmax1 $seeing]

    fitPSFModel $obj $reg $dgpsf $skysig
    fitDevModel $obj $reg $seeing $skysig
    fitExpModel $obj $reg $seeing $skysig

    if { [check_it $obj  r_$type $size 2.5] != 0} {
	incr ret
    }
    if { [check_it $obj  I_$type $aratio .14] != 0} {
	incr ret
    }
    if { [check_it $obj  ab_$type $ampl 200] != 0} {
	incr ret
    }
    if { [check_it $obj  phi_$type $pa 4] != 0} {
	incr ret
    }

    set mask [handleBindFromHandle [handleNew] *$reg.mask];
    handleSet $reg.mask 0x0;

    object1Del $obj; dgpsfDel $dgpsf
    spanmaskDel $mask; regDel $reg;
    objectChainDel $objects; chainDel $objs;
    binregionDel $br; vectorExprDel $levels; handleDel $levels

    return $ret
}

#
# Run all the tests
#
set ampl 10000;				# central amplitude
set size 10;				# characteristic size of models
set aratio 0.7;				# axis ratio
set pa 30;				# position angle of major axis
set psf [dgpsfNew -sigmax1 1 -sigmay1 1 -sigmax2 3 -sigmay2 3 -b 1]
set seeing 1;				# seeing in some units

set catalog "rgalprof.dat"
if {![file readable $catalog]} {
   echo "Creating Catalogues"
   makeProfCat $catalog
}

initFitobj $catalog
initProfileExtract

set failed 0

if {[set ret [testFitModel "deV" $ampl $size $aratio $pa $psf]] != 0} {
   echo "TEST-ERR: Failed fitting deV galaxy (size = $size)"
   set failed [expr $failed+$ret]
}

set size 17;				# characteristic size of models

if {[set ret [testFitModel "exp" $ampl $size $aratio $pa $psf]] != 0} {
   echo "TEST-ERR: Failed fitting exponential galaxy (size = $size)"
   set failed [expr $failed+$ret]
}

finiProfileExtract
finiFitobj

if $failed {
   echo "TEST-ERR: Failed $failed tests"
}

return $failed;
