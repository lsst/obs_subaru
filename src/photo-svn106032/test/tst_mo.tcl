#
# Test the Measure Objects module
#
source [envscan \$PHOTO_DIR]/etc/photoStartup.tcl
source [envscan \$PHOTO_DIR]/test/tst_procs.tcl

set pi 3.141592654

# We do NOT want to use a random seed! These tests should be deterministic.
# Note that they are not quite; the gaussian random number generator keeps a
# single value cached so it is possible for these tests to have two outcomes
# if run more than once
#set seed [getclock]; echo Seed is $seed
set seed 8190645
set rand [phRandomNew 200000 -seed $seed -type 2]

#
# create regions, using the given sky, and add data values for objects
#    "object" on top; then measure them and see if we get them right
#
# We also set the correct answers. N.b:
#   only the zero'th element of profRatio contains a valid value
#   only the zero'th element of profAngle contains a valid value
#
# n.b. The region for test "foo" is created by proc make_region_foo
#
proc make_region_const {sky _vals} {
   upvar $_vals vals
   global fieldparams fiber_rad pi
   set regval 1000;			# value of region above sky

   set gain [phGain $fieldparams.frame<0> 0 0]
   set dvar [phDarkVariance $fieldparams.frame<0> 0 0]

   set sigx1 [exprGet $fieldparams.frame<0>.psf->sigmax1];# PSF parameters
   set sigy1 [exprGet $fieldparams.frame<0>.psf->sigmay1]
   set sigx2 [exprGet $fieldparams.frame<0>.psf->sigmax2]
   set sigy2 [exprGet $fieldparams.frame<0>.psf->sigmay2]
   set b [exprGet $fieldparams.frame<0>.psf->b]
   set neff [expr {4*$pi*pow($sigx1*$sigy1+$b*$sigx2*$sigy2,2)/\
		       ($sigx1*$sigy1+\
			4*$b*$sigx1*$sigy1*$sigx2*$sigy2/($sigx1*$sigy1+$sigx2*$sigy2)+\
			    $b*$b*$sigx2*$sigy2)}];# n effective for dgpsf
   # set the correct answers
   set vals [list \
		 {sky 100} \
		 {Q 0} \
		 {U 0} \
		 {nprof 9} \
		 {profMean<0> 1001} \
		 {profMean<1> 1000} \
		 {profMean<2> 1000} \
		 {profMean<3> 1000} \
		 {profMean<4> 1000} \
		 {profMean<5> 1000} \
		 {profMean<6> 1000} \
		 {profMean<7> 1000} \
		 {profMean<8> 1000} \
		 ]
   lappend vals [list fiberCounts [expr $regval*$pi*pow($fiber_rad,2)+1]]
   lappend vals [list fiberCountsErr \
	       [expr sqrt($pi*pow($fiber_rad,2)*(($regval+$sky)/$gain+$dvar))]]
   lappend vals [list psfCounts [expr $regval*$neff+1]]
   lappend vals [list psfCountsErr \
		     [expr sqrt($neff*(($regval+$sky)/$gain+$dvar))]]
		    
   set reg [regNew 81 81]

   regIntSetVal $reg [expr $sky+$regval]
   regPixSetWithDbl $reg 40 45 [expr $sky+$regval+1];	# define centre

   return $reg;
}

proc make_region_noise {sky _vals} {
   upvar $_vals vals
   global fieldparams fiber_rad pi rand
   set regval 500;			# value of region above sky

   set gain [phGain $fieldparams.frame<0> 0 0]
   set dvar [phDarkVariance $fieldparams.frame<0> 0 0]

   set sigx1 [exprGet $fieldparams.frame<0>.psf->sigmax1];# PSF parameters
   set sigy1 [exprGet $fieldparams.frame<0>.psf->sigmay1]
   set sigx2 [exprGet $fieldparams.frame<0>.psf->sigmax2]
   set sigy2 [exprGet $fieldparams.frame<0>.psf->sigmay2]
   set b [exprGet $fieldparams.frame<0>.psf->b]
   set neff [expr {4*$pi*pow($sigx1*$sigy1+$b*$sigx2*$sigy2,2)/\
		       ($sigx1*$sigy1+\
			4*$b*$sigx1*$sigy1*$sigx2*$sigy2/($sigx1*$sigy1+$sigx2*$sigy2)+\
			    $b*$b*$sigx2*$sigy2)}];# n effective for dgpsf

   # set the correct answers
   set vals [list \
		 {colc 30.54} \
		 {rowc 30.47} \
		 {sky 100} \
		 {Q 0} \
		 {U 0} \
		 ]
   lappend vals [list fiberCounts [expr $regval*$pi*pow($fiber_rad,2)+1]]
   lappend vals [list fiberCountsErr \
	       [expr sqrt($pi*pow($fiber_rad,2)*(($regval+$sky)/$gain+$dvar))]]
   lappend vals [list psfCounts [expr $regval*$neff+1]]
   lappend vals [list psfCountsErr \
		     [expr sqrt($neff*(($regval+$sky)/$gain+$dvar))]]
		    
   set reg [regNew -name "noise" 61 61]

   regClear $reg
   regIntGaussianAdd $reg $rand [expr $sky+$regval] 3
   regPixSetWithDbl $reg 30 30 [expr $sky+$regval+50]
   return $reg;
}

proc make_region_psf {sky _vals} {
   upvar $_vals vals
   global fieldparams pi
   set amp 1000
   set psf [handleBindFromHandle [handleNew] *$fieldparams.frame<0>.psf] 

   set sigx1 [exprGet $psf.sigmax1]
   set sigy1 [exprGet $psf.sigmay1]
   set sigx2 [exprGet $psf.sigmax2]
   set sigy2 [exprGet $psf.sigmay2]
   set b [exprGet $psf.b]

   set psfCounts [expr $amp*2*$pi*($sigx1*$sigy1 + $b*$sigx2*$sigy2)]

   # set the correct answers
   set vals [list \
		 {rowc 28.50} \
		 {colc 29.50} \
		 {sky 100} \
		 {U 0} \
		 ]
   lappend vals [list psfCounts $psfCounts]
   
   set reg [regNew 60 60]

   regIntSetVal $reg $sky;
   dgpsfAddToReg $reg $psf 28.5 29.5 -a $amp

   handleDel $psf

   return $reg;
}

proc make_region_m1 { sky _vals } {
   upvar $_vals vals			
   # set the correct answers
   set vals [list \
		 {rowc 39.50} \
		 {colc 41.50} \
		 {sky 100} \
		 {npix 1} \
                 {fiberCounts 128} \
		 {Q 0} \
		 {U 0} \
		]
   
  set reg [regNew 80 80]
  regIntSetVal $reg $sky
  add_to_reg $reg 38 40 " 8 16  8"
  add_to_reg $reg 39 40 "16 32 16"
  add_to_reg $reg 40 40 " 8 16  8"

  return $reg
}

proc make_region_0 { sky _vals } {
   upvar $_vals vals			
   # set the correct answers
   set vals [list \
		 {rowc 29.50} \
		 {colc 31.50} \
		 {sky 100} \
		 {npix 9} \
                 {fiberCounts 415.09} \
		 {psfCounts 392} \
		 {profMean<0> 30.1} \
		 {profMean<1> 19.4} \
		 {profMean<2> 9.8} \
		 {profMean<3> 4.0} \
		 {profMed<0> 32} \
		 {profMed<1> 20} \
		 {profMed<2> 9.3} \
		 {profMed<3> 3.8} \
		 {Q -0.216} \
		 {U 0.345} \
		 {majaxis 1.06} \
		 {aratio 0.407} \
		 ]
   
  set reg [regNew 60 60]
  regIntSetVal $reg $sky
  add_to_reg $reg 22 25 "0  0  0  0  0  0  0  0  0  0  0  0  0"
  add_to_reg $reg 23 25 "0  1  1  1  1  1  0  0  0  0  0  0  0"
  add_to_reg $reg 24 25 "0  1  2  2  2  2  1  0  0  0  0  0  0"
  add_to_reg $reg 25 25 "0  0  4  6  4  6  4  1  0  0  0  0  0"
  add_to_reg $reg 26 25 "0  0  2  8 12 12  8  2  0  0  0  0  0"
  add_to_reg $reg 27 25 "0  0  1  6 16 20 12  6  1  0  0  0  0"
  add_to_reg $reg 28 25 "0  0  0  2 12 24 24 12  2  0  0  0  0"
  add_to_reg $reg 29 25 "0  0  0  1  4 20 32 20  4  1  0  0  0"
  add_to_reg $reg 30 25 "0  0  0  0  2 12 24 24 12  2  0  0  0"
  add_to_reg $reg 31 25 "0  0  0  0  1  6 12 20 16  6  1  0  0"
  add_to_reg $reg 32 25 "0  0  0  0  0  2  8 12 12  8  2  0  0"
  add_to_reg $reg 33 25 "0  0  0  0  0  1  4  6  4  6  4  0  0"
  add_to_reg $reg 34 25 "0  0  0  0  0  0  1  2  2  2  2  1  0"
  add_to_reg $reg 35 25 "0  0  0  0  0  0  0  1  1  1  1  1  0"
  add_to_reg $reg 36 25 "0  0  0  0  0  0  0  0  0  0  0  0  0"

  return $reg
}

#############################################################################
# create a region corresponding to the star at (1222, 268) in frames_4,
#    field 2, r-band.  The sky value is 106, but I reset that to $sky
#

proc make_region_1 { sky _vals } {
   upvar $_vals vals			

   # set the correct answers
   set vals [list \
		 {rowc 32.2169} \
		 {colc 33.5956} \
		 {sky 100} \
		 {npix 217} \
		 {Q 0} \
		 {U 0} \
		 {nprof 8} \
		 {fiberCounts 92025.2} \
		 {psfCounts 115132} \
		 {profMean<0> 10745.1} \
		 {profMean<1> 5776} \
		 {profMean<2> 1714.3} \
		 {profMean<3> 530.7} \
		 {profMean<4> 123.3} \
		 {profMean<5> 10.76} \
		 {profMean<6> 1.10} \
		 {profMed<0> 11651} \
		 {profMed<1> 5830.38} \
		 {profMed<2> 1628.5} \
		 {profMed<3> 489.275} \
		 {profMed<4> 95.70} \
		 {profMed<5> 9.13} \
		 {profMed<6> 0.2} \
		 ]

  set reg [regNew 67 67]
  regIntSetVal $reg 106
  set sreg [subRegNew $reg 27 27 20 20]; regIntSetVal $sreg 0; regDel $sreg
  add_to_reg $reg 20 20 "112 108 116 109 111 114 107 115 113 106 112 120 104 \
       103 117 100 110 103 110 103 105 107 108 112 105 107 109 "
  add_to_reg $reg 21 20 "118 110 100 108 109 114 109 103 109 108 114 102 112 \
       113 109 115 118 108 115 105 109 107 105 110 103 105 107 "
  add_to_reg $reg 22 20 "112 108 108 107 114 110 107 112 114 110 119 113 105 \
       123 117 108 103 110 106 113 111 106 109 113 110 101 103 "
  add_to_reg $reg 23 20 "118 107 102 109 106 106 107 113 114 113 118 113 115 \
       123 127 120 122 118 110 109 116 104 105 112 112 111 105 "
  add_to_reg $reg 24 20 "102 108 110 110 106 110 118 113 121 110 121 131 136 \
       137 137 130 131 117 116 125 107 114 116 112 109 110 111 "
  add_to_reg $reg 25 20 "113 114 113 106 111 114 107 119 120 130 147 159 181 \
       175 175 161 141 130 120 116 118 111 109 102 112 107 104 "
  add_to_reg $reg 26 20 " 110 106 114 113 112 112 114 124 132 165 180 202 233 \
       251 238 214 194 151 137 135 121 107 114 111 107 109 110 "
  add_to_reg $reg 27 20 " 103 110 112 110 118 112 118 134 168 198 254 313 362 \
       384 351 333 259 206 168 147 118 118 119 108 115 104 102 "
  add_to_reg $reg 28 20 " 102 110 106 113 105 122 131 156 197 257 355 448 533 \
       557 561 454 371 267 207 157 131 124 116 113 111 112 107 "
  add_to_reg $reg 29 20 " 110 109 114 111 114 128 142 167 244 334 482 625 840 \
       945 880 683 502 376 249 187 150 132 118 110 120 112 101 "
  add_to_reg $reg 30 20 " 109 110 108 113 121 128 152 183 285 425 612 959 \
       1975 3050 2304 1126 670 451 309 205 161 136 117 109 114 116 114 "
  add_to_reg $reg 31 20 " 117 112 113 111 122 130 158 207 307 463 724 1616 \
       5157 9170 6211 2017 804 503 345 229 167 123 115 119 106 112 108 "
  add_to_reg $reg 32 20 " 100 114 110 110 117 134 150 214 320 473 779 1827 \
       6321 11264 7632 2347 834 515 348 210 169 135 118 105 111 119 112 "
  add_to_reg $reg 33 20 " 102 107 120 111 115 120 146 209 289 436 670 1239 \
       3218 5338 3776 1486 726 485 311 206 165 130 117 105 112 114 109 "
  add_to_reg $reg 34 20 " 104 113 113 107 108 128 137 190 254 364 512 762 \
       1134 1498 1213 823 581 405 271 189 143 126 114 115 112 114 105" 
  add_to_reg $reg 35 20 " 104 100 115 110 111 121 129 158 210 299 409 521 632 \
       692 648 557 422 328 222 162 142 122 115 109 102 113 110 "
  add_to_reg $reg 36 20 " 104 108 104 108 118 120 121 139 175 219 294 368 435 \
       466 438 383 313 241 181 149 125 110 109 99 110 111 104 "
  add_to_reg $reg 37 20 " 104 106 115 114 107 113 124 127 142 184 204 244 285 \
       307 292 262 220 179 156 129 125 111 105 114 108 101 110 "
  add_to_reg $reg 38 20 " 99 106 102 102 107 112 119 120 141 136 160 180 198 \
       194 200 195 159 138 132 112 120 104 104 110 107 118 110 "
  add_to_reg $reg 39 20 " 102 107 112 115 115 109 109 108 117 125 126 138 136 \
       161 157 137 124 123 119 114 112 107 107 118 108 105 104 "
  add_to_reg $reg 40 20 " 115 109 101 115 112 102 107 113 118 124 118 118 131 \
       136 135 121 120 115 108 111 103 112 112 109 103 106 109 "
  add_to_reg $reg 41 20 " 97 100 108 112 112 118 111 102 112 120 117 111 112 \
       125 117 106 115 117 107 113 109 104 109 105 115 110 103 "
  add_to_reg $reg 42 20 " 97 109 113 106 101 118 111 111 114 117 107 120 113 \
       125 119 113 106 105 104 109 120 103 102 109 110 106 111 "
  add_to_reg $reg 43 20 " 112 106 98 114 107 111 102 104 108 107 115 111 103 \
       116 104 108 116 109 112 111 110 105 115 105 112 106 108 "
  add_to_reg $reg 44 20 " 105 105 108 102 99 119 111 110 108 113 102 102 106 \
       114 105 113 103 98 113 109 105 104 111 109 100 109 97 "
  add_to_reg $reg 45 20 " 100 102 108 110 110 109 105 112 106 108 102 97 112 \
       112 106 110 108 112 101 107 109 111 115 99 104 104 115 "
  add_to_reg $reg 46 20 " 113 101 115 100 106 106 108 109 101 105 114 114 107 \
       105 122 108 114 99 112 112 107 107 107 111 111 110 107 "
#
# change the sky value to $sky
#
   regIntConstAdd $reg [expr $sky-106]

  return $reg
}

#############################################################################
# create a region corresponding to the galaxy at (1127, 512) in frames_4,
# field 2, r-band. The sky value is 106, but I reset that to $sky
#

proc make_region_2 { sky _vals } {
   upvar $_vals vals			
   # set the correct answers
   set vals [list \
		 {rowc 34.807} \
		 {colc 35.913} \
		 {sky 100} \
		 {npix 123} \
		 {Q -0.379} \
		 {U -0.211} \
		 {fiberCounts 2250} \
		 {psfCounts 1644} \
		 {majaxis -1.31} \
		 {profMean<0> 95.52} \
		 {profMean<1> 73.34} \
		 {profMean<2> 53.2} \
		 {profMean<3> 36.0} \
		 {profMean<4> 16.7} \
		 {profMean<5> 6.4} \
		 {profMed<0> 100} \
		 {profMed<1> 73.6} \
		 {profMed<2> 53.0} \
		 {profMed<3> 36.237} \
		 {profMed<4> 16.46} \
		 {profMed<5> 6.37} \
		 ]

   set bigreg [regNew 73 69]
   regIntSetVal $bigreg 106
   set reg [subRegNew $bigreg 33 29 20 20]
   regIntSetVal $reg 0

  add_to_reg $reg  0 0 "114 104 109 115 108 109 103 102 106 106 106 102 113 \
          108 104 105 114 115 120 112 113 117 115 110 107 108 117 107 112 "
  add_to_reg $reg  1 0 "107 100 103 109 102 103 104 109 110 102 101 110 111 \
          103 107 114 114 107 112 113 118 105 109 110 102 108 107 101 102 "
  add_to_reg $reg  2 0 "112 110 105 102 102 101 104 111 109 107 108 106 114 \
          106 105 114 116 119 118 113 122 111 115 104 107 103 115 108 112 "
  add_to_reg $reg  3 0 "110 109 103 103 103 111 110 106 99 100 103 106 104 \
          108 118 109 110 118 119 124 118 115 117 111 113 112 100 108 111 "
  add_to_reg $reg  4 0 "110 105 108 100 98 107 102 103 103 108 107 110 108 \
          112 119 118 121 122 131 120 119 118 111 115 113 106 108 109 103 "
  add_to_reg $reg  5 0 "108 113 104 110 103 113 107 105 114 112 101 103 111 \
          119 115 127 116 136 134 131 130 118 114 104 119 105 108 107 104 "
  add_to_reg $reg  6 0 "106 102 110 109 103 106 108 105 107 106 107 108 116 \
          112 110 125 128 132 134 126 120 120 112 106 104 106 105 110 109 "
  add_to_reg $reg  7 0 "113 107 105 103 101 106 106 107 113 104 108 100 103 \
          114 124 123 128 137 142 125 123 110 114 112 113 102 113 105 102 "
  add_to_reg $reg  8 0 "113 105 108 100 99 108 102 101 109 108 110 114 114 \
          107 122 132 143 141 142 130 128 121 111 103 109 112 105 101 101 "
  add_to_reg $reg  9 0 "104 113 104 102 109 103 110 99 106 110 118 113 113 \
          117 136 133 143 157 149 132 123 121 114 116 105 105 106 102 105 "
  add_to_reg $reg 10 0 " 104 103 100 102 112 104 104 105 108 110 110 116 \
          111 122 127 145 163 151 150 131 121 122 111 108 106 100 110 103 107 "
  add_to_reg $reg 11 0 " 100 114 108 111 108 106 100 100 113 108 105 114 117 \
          124 140 146 169 163 145 131 121 123 103 102 105 112 105 100 111" 
  add_to_reg $reg 12 0 " 106 109 107 98 105 117 104 111 105 113 117 120 116 \
          131 147 166 174 161 152 133 113 111 113 111 107 111 99 102 99 "
  add_to_reg $reg 13 0 " 103 112 106 101 112 104 103 103 115 107 118 111 129 \
          135 150 177 183 172 152 132 114 112 109 115 111 108 104 103 111 "
  add_to_reg $reg 14 0 " 104 99 104 100 108 108 107 107 108 114 121 114 132 \
          136 167 200 189 159 144 130 118 114 107 109 116 113 104 102 106 "
  add_to_reg $reg 15 0 " 96 103 118 108 106 103 111 105 119 102 113 129 133 \
          136 170 190 193 153 138 122 124 111 106 112 103 101 112 110 104 "
  add_to_reg $reg 16 0 " 107 108 115 111 107 111 107 111 103 119 117 118 133 \
          144 167 175 171 149 140 126 113 113 108 105 109 106 111 111 111 "
  add_to_reg $reg 17 0 " 102 104 109 100 103 108 105 106 104 104 116 125 131 \
          152 163 183 169 142 128 114 119 104 110 112 101 101 97 104 108 "
  add_to_reg $reg 18 0 " 96 110 96 106 107 105 101 103 112 115 118 128 135 \
          156 164 162 144 143 122 119 122 115 115 107 107 111 106 106 106 "
  add_to_reg $reg 19 0 " 109 106 116 95 98 115 101 112 104 119 114 123 137 \
          141 151 156 139 128 119 107 115 111 106 113 107 107 101 104 112 "
  add_to_reg $reg 20 0 " 106 102 103 103 103 109 109 110 103 120 119 126 127 \
          150 142 140 134 128 123 114 107 105 106 109 117 105 111 109 111 "
  add_to_reg $reg 21 0 " 107 106 116 108 105 106 111 108 118 110 110 121 129 \
          136 143 142 134 123 114 106 103 112 107 117 107 112 104 113 108 "
  add_to_reg $reg 22 0 " 107 102 103 105 107 103 108 110 119 111 109 132 121 \
          131 134 135 124 110 120 117 113 110 108 103 109 102 108 106 109 "
  add_to_reg $reg 23 0 " 114 103 115 103 106 100 99 106 112 107 118 127 126 \
          139 123 128 118 120 118 106 99 107 103 104 103 105 102 105 104 "
  add_to_reg $reg 24 0 " 100 100 102 103 106 108 97 102 117 112 121 123 133 \
          129 127 128 119 119 107 114 113 111 104 108 106 113 105 112 104 "
  add_to_reg $reg 25 0 " 99 105 111 104 103 113 110 106 105 113 123 121 124 \
          131 118 107 121 103 109 108 108 103 108 107 107 107 100 108 103 "
  add_to_reg $reg 26 0 " 105 98 104 102 116 109 99 111 110 116 108 118 126 \
          116 119 113 114 108 107 103 107 112 106 110 105 106 101 101 113 "
  add_to_reg $reg 27 0 " 115 107 102 101 112 105 107 108 109 113 116 117 \
          123 127 118 108 110 113 105 110 116 104 104 104 103 102 103 104 104 "
  add_to_reg $reg 28 0 " 115 107 105 110 104 106 109 112 109 118 115 112 \
          122 109 115 114 108 105 102 110 107 108 100 105 113 115 111 112 104 "
  add_to_reg $reg 29 0 " 98 99 105 108 104 103 108 115 119 120 116 118 125 \
          111 112 107 112 117 101 103 107 114 101 108 108 103 107 108 111 "
  add_to_reg $reg 30 0 " 104 111 111 101 106 107 115 116 105 106 107 117 118 \
          114 115 106 101 108 110 118 106 108 93 103 107 107 109 108 104 "
  add_to_reg $reg 31 0 " 109 105 100 104 111 102 109 101 110 108 112 118 107 \
          104 118 106 105 112 112 103 106 108 105 106 101 105 110 107 104 "
  add_to_reg $reg 32 0 " 101 104 108 99 102 101 112 103 107 112 110 115 106 \
          110 113 107 105 117 104 114 114 103 106 111 105 101 98 98 106 "
   regDel $reg; set reg $bigreg
#
# change the sky value to $sky
#
   regIntConstAdd $reg [expr $sky-106]

  return $reg
}

################################################################
#
# given a region, and a sky value, run the object finder to find all
# pixels above sky, and use this to generate an OBJC, which is then returned
#
proc find_objc {reg sky soft_bias} {
   global MASK_TYPE id fparams OBJECT1 OBJECT3

   set objc [objcNew 1]

   set psf_filt_size 11; set psfsigma 1
   set hpsf_filt_size [expr int($psf_filt_size/2)]
   set nrow [exprGet $reg.nrow]
   set ncol [exprGet $reg.ncol]

   if 0 {				# smooth image
      set sreg [regNew $nrow $ncol]; set scr [regNew $nrow $ncol]
      convolveWithGaussian $sreg $reg $scr $psf_filt_size $psfsigma
      regDel $scr
   
      handleSetFromHandle $sreg.mask $reg.mask; # set bits in $reg.mask
   } else {
      set sreg $reg
   }

   set lev [vectorExprEval $soft_bias+20]
   set bsky [binregionNewFromConst $sky]
   set objects [regFinder $reg $lev $fparams \
		    -row0 $hpsf_filt_size  -col0 $hpsf_filt_size \
		    -row1 -$hpsf_filt_size -col1 -$hpsf_filt_size];
   
   if {$sreg != $reg} {
      handleSet (long)$sreg.mask 0;
      regDel $sreg
   }
   vectorExprDel $lev
#   
# set master_mask
#
   set mask [spanmaskNew]
   spanmaskSetFromObjectChain $objects $mask 3 0
   set mm [chainElementRemByPos *$mask.masks<3> HEAD]
   handleSetFromHandle $objc.aimage->master_mask &$mm
   handleDel $mm; spanmaskDel $mask
#
# get our OBJC from the OBJECT chain; choose the brightest if there's
# more than one (we don't smooth, remember)
#
   set objs [objectToObject1ChainConvert $objects 0 $reg $bsky]
   spanmaskSetFromObject1Chain $objs *$reg.mask $MASK_TYPE(OBJECT)
   object1ChainFlagsSet $objs $OBJECT1(BINNED1)

   set our_obj [object1New]
   
   set curs [chainCursorNew $objs]
   while {[chainWalk $objs $curs] != ""} {
      set obj [chainElementRemByPos $objs HEAD]
      if {[exprGet (int)$our_obj.peaks] == 0 || \
	      ([exprGet $obj.peaks->peaks<0>->peak] > \
		   [exprGet $our_obj.peaks->peaks<0>->peak])} {
	 object1Del $our_obj; set our_obj $obj
      } else {
	 object1Del $obj
      }
   }

   chainCursorDel $objs $curs
   handleSet $our_obj.id [incr id]
   handleSetFromHandle $objc.color<0> &$our_obj; handleDel $our_obj
   chainDel $objs
   binregionDel $bsky
#
# Set the OBJC centre
#
   foreach el "rowc rowcErr colc colcErr" {
      handleSetFromHandle $objc.$el $objc.color<0>->$el
   }
   handleSet $objc.flags3 $OBJECT3(HAS_CENTER)

   chainDel $objects
#
# Set the PSF width (adaptive moments; real calculation needs PSF_BASIS
#
   handleSet $objc.color<0>->M_rr_cc_psf 100

   return $objc
}


#################################################################
# check to see that various fields in the given OBJC->color<0>
#   the values we expect.  return 0 if all okay, or 1 if not.
#
proc check_objc { objc vals } {
   global errors
   set errs 0

   if {[exprGet $objc.color<0>->aratio] < 2e-3} {
      handleSet $objc.color<0>->majaxis 0
   }

   foreach elem $vals {
      set name [lindex $elem 0]
      set val [lindex $elem 1]
      set err [keylget errors $name]

      if { [check_it $objc.color<0> $name $val $err] != 0 } {
	 incr errs
      }
   }

   return $errs;
}

#
# this is an array of permissible errors for OBJECT1 structure elements
# An error may be specified as (e.g.) 0.1%
#
# You can specify more than one criterion, (e.g. 2|0.5%); in this case only
# if the measurement fails all the criteria is it said to fail.
#
set errors [list \
		{rowc 0.01} \
		{colc 0.01} \
		{sky 0} \
		{npix 0} \
		{Q 0.01} \
		{U 0.01} \
		{fiberCounts 0.5%} \
		{fiberCountsErr 0.5%} \
		{psfCounts 0.5%} \
		{psfCountsErr 0.5%} \
		{petroRad 0.2} \
		{petroCounts 1%} \
		{petroR50 0.2} \
		{petroR90 0.2} \
		{majaxis 0.01} \
		{aratio 0.01} \
		{nprof 0} \
		{profMean<0> 0.1|0.1%} \
		{profMean<1> 0.1|0.1%} \
		{profMean<2> 0.1|0.1%} \
		{profMean<3> 0.1|0.1%} \
		{profMean<4> 0.1|0.1%} \
		{profMean<5> 0.1} \
		{profMean<6> 0.1} \
		{profMean<7> 0.1} \
		{profMean<8> 0.1} \
		{profMean<9> 0.1} \
		{profMed<0> 0.1} \
		{profMed<1> 0.1} \
		{profMed<2> 0.1} \
		{profMed<3> 0.1} \
		{profMed<4> 0.1} \
		{profMed<5> 0.1} \
		{profMed<6> 0.1} \
	   ]
keylset errors psfCounts "11%";		# while I think about things. RHL
keylset errors psfCountsErr "6%"

################################################################
# the main routine -- create a new region, create all auxiliary
#   data, then call measureObj on the region
#
set soft_param_file [envscan \$PHOTO_DIR]/test/tst_params
inipar $soft_param_file

initProfileExtract
set pixscale [getparf fp_pixscale]
set mo_fiber_dia [getparf mo_fiber_dia]
set sky 100
set skysig 1.0
set flux20 1000

#####
# the filters we use
set filterlist [list r]
set nfilter [llength $filterlist]

#####
# more setup
set sblist ""
foreach f $filterlist {
   lappend sblist [getparf mo_sbellip_$f]
}
set fiber_rad [expr 0.5*($mo_fiber_dia/$pixscale)]

#####
# create CALIB1BYFRAME structure
set calibs [calib1byframeNew $filterlist]
loop i 0 $nfilter {
   set cal1 [calib1New [lindex $filterlist $i]]
   handleSet $cal1.sky $sky
   handleSet $cal1.flux20 $flux20
   handleSet $cal1.node 0.0
   handleSet $cal1.incl 0.0
   handleSet $cal1.gain [getpari gain]
   handleSet $cal1.dark_variance [getpari dark_variance]

   set psf [dgpsfNew -sigmax1 0.94 -sigmax2 2.70 -sigmay1 0.84 \
	       -sigmay2 2.90 -b 0.11]
   handleSet $psf.sigma1_2G \
       [expr 0.5*([exprGet $psf.sigmax1]+[exprGet $psf.sigmay1])]
   handleSet $psf.sigma2_2G \
       [expr 0.5*([exprGet $psf.sigmax2]+[exprGet $psf.sigmay2])]
   handleSet $psf.b_2G [exprGet $psf.b]
   handleSet $psf.width [expr [exprGet $psf.sigma1_2G]]
   handleSetFromHandle $cal1.psf &$psf; handleDel $psf
   
   handleSetFromHandle $calibs.calib<$i> &$cal1
   handleDel $cal1
}

#####
# create FRAMESTAT structure
set fieldstat [fieldstatNew]

set fieldparams [fieldparamsNew $filterlist]
handleSet $fieldparams.smooth_profs [getpari mo_smooth_profs]
set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<0>]

set soft_bias [softBiasGet]
handleSet $fparams.bkgd 0
handleSetFromHandle $fparams.gain &[binregionNewFromConst  [getpari gain] -shift $PHOTO_CONSTS(MAX_U16)]
handleSetFromHandle $fparams.dark_variance &[binregionNewFromConst [getpari dark_variance] -shift $PHOTO_CONSTS(MAX_U16)]
handleSetFromHandle $fparams.psf $calibs.calib<0>->psf
handleSet $fparams.colBinning 1

#####
# create an ASCII file with the decision tree we'll use for classification
#    this is just a placeholder -- it doesn't really classify accurately
set treefile "tree.ascii"
exec echo ".      no_type      0    13.4675  0.542      0" > $treefile
exec echo ".l     Galaxy      -1     0.0000  0.042    172" >> $treefile
exec echo ".r     Star        -1     0.0000  0.000    133" >> $treefile

set retval 0;				# success until proven a failure
#
# Do the real work
#
# call the Init Measure Obj module
   
handleSet $fieldparams.pixscale $pixscale
handleSet $fieldparams.petro_f1 [getparf petro_f1]
handleSet $fieldparams.petro_f2 [getparf petro_f2]
handleSet $fieldparams.petro_f4 [getparf petro_f4]
handleSet $fieldparams.petro_f5 [getparf petro_f5]
handleSet $fieldparams.fiber_rad $fiber_rad
handleSet $fieldparams.deblend_psf_Lmin 0.5
handleSet $fieldparams.ref_band_index \
    [lsearch $filterlist [getpars mo_fiber_color]]

set catalog "$env(PHOTO_DIR)/lib/rgalprof.dat"
set cellcat "$env(PHOTO_DIR)/lib/cellprof.dat"

set failed 0
foreach f "catalog cellcat" {
   set file [set $f]
   if {![file readable $file]} {
      echo "TEST-ERR: you must make $file before running tests"
      incr failed
   }
}
if $failed {
   error "TEST-ERR: required input files are missing (run \"make datafiles\")"
}

initFitobj $catalog
initCellFitobj $cellcat 1
initMeasureObj $fieldparams $nfilter $sblist $rand

if {![info exists show_failures]} {
   set show_failures 0;			# if true, display failures
}

set id 1;				# ID number for objects
#
# Finally, we run the tests
#
if {![info exists tests]} {
   set set_tests 1
   set tests [list const noise psf m1 0 1 2]
}
foreach test $tests {
   echo "measuring object $test"
#
# create data
#
   eval "set reg \[make_region_$test $sky values\]"
   add_mask $reg
#
# Now we have a region, set colors for measure objects; note that one
# of the arguments is the data region
#
   set sky_reg [binregionNewFromConst $sky]
   set skysig_reg [binregionNewFromConst $skysig]

   foreach filter $filterlist {
      set ifilter [lsearch $filterlist $filter]

      set prof [compositeProfileNew]; handleSet $prof.psfCounts 1e4
      handleSetFromHandle $calibs.calib<$ifilter>->prof &$prof
      handleDel $prof

      set trans [transNew]
      handleSetFromHandle $calibs.calib<$ifilter>->toGCC &$trans
      handleDel $trans

      measureObjColorSet $fieldparams $fieldstat $ifilter \
	  *$calibs.calib<$ifilter> $reg \
	  $sky_reg $skysig_reg ""

      set psf [handleBindFromHandle [handleNew] *$calibs.calib<$ifilter>->psf]
      tst_set_psf $psf $ifilter
      handleDel $psf
   }
   handleDel $sky_reg; handleDel $skysig_reg
#
# sky subtract and create an OBJC from the data
#
   regIntConstAdd $reg [expr $soft_bias-$sky]

   set objc [find_objc $reg $sky $soft_bias]

   measureObjc $objc $fieldparams $fieldstat
#
# See if we got it right
#
   if {[set nerr [check_objc $objc $values]] > 0} {
      echo "TEST-ERR: measureObjChain failed on object $test"
      set retval [expr $retval+$nerr]

      if $show_failures {
	 mtv $reg;
	 puts -nonewline "RHL Test $test; hit <CR> to proceed "; gets stdin
      }

      if {[info exists abort_on_error] && $abort_on_error} {
	 set objc $objc; set reg $reg
	 error "Set \$objc and \$reg"
      }
   }
#
# cleanup object
#
   set obj1 [handleBindNew [exprGet $objc.color<0>] OBJECT1]
   measureObjUnset $fieldparams
   objmaskDel [handleBindNew [exprGet $obj1.mask] OBJMASK]
   handleSet $obj1.mask 0
   handleDel $obj1

   objcDel $objc

   spanmaskDel *$reg.mask
   handleSet (long)$reg.mask 0
   regDel $reg
}

#####
# clean up
if [info exists set_tests] {
   unset tests
}

if 0 {
   echo Not cleaning up;		# useful for debugging
} else {
   finiProfileExtract
   finiMeasureObj
   finiCellFitobj
   
   unlink -nocomplain $treefile
   calib1byframeDel $calibs
   fieldstatDel $fieldstat
   handleDel $fparams
   fieldparamsDel $fieldparams
}

if $retval {
   error "Failed $retval tests"
}
