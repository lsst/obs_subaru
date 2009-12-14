#
# Create the catalog data files for photo
#
set catalog "rgalprof.dat"
set cellcat "cellprof.dat"

set rand [phRandomNew "10000:3"]
initProfileExtract

if {![file readable $catalog]} {
   echo "Creating Catalogues"
   makeProfCat $catalog -show
}

initFitobj $catalog

if {![file readable $cellcat]} {
   echo "Creating Cell Catalogues"
   set nsigma 10
   set sigma0 0.75
   set dsigma 0.25
   set sigma_ratio 3
   set b 0.1111

   echo [format "%d seeings (%.2f .. %.2f), N(0,s) and N(0,s) + %.3fN(0,%gs)" \
	     $nsigma $sigma0 [expr $sigma0 + ($nsigma - 1)*$dsigma] \
	     $b $sigma_ratio]
   makeCellProfCat $cellcat -show $nsigma $sigma0 $dsigma  $sigma_ratio $b
}

phRandomDel $rand
finiFitobj
finiProfileExtract
