proc knownobjTbl {} {
   set koTbl [schemaTransNew] 
   schemaTransEntryAdd $koTbl name CATALOG catalog string
   schemaTransEntryAdd $koTbl name ID id string
   schemaTransEntryAdd $koTbl name CLASS class enum
   schemaTransEntryAdd $koTbl name RA ra double
   schemaTransEntryAdd $koTbl name RAERR raErr double
   schemaTransEntryAdd $koTbl name DEC dec double
   schemaTransEntryAdd $koTbl name DECERR decErr double
   schemaTransEntryAdd $koTbl name MAG mag struct
   schemaTransEntryAdd $koTbl cont mag mag float
   schemaTransEntryAdd $koTbl name MAGERR mag struct
   schemaTransEntryAdd $koTbl cont mag magErr float
   schemaTransEntryAdd $koTbl name PASSBAND mag struct
   schemaTransEntryAdd $koTbl cont mag  passBand string
   schemaTransEntryAdd $koTbl name RASIZE raSize float
   schemaTransEntryAdd $koTbl name DECSIZE decSize float
   schemaTransEntryAdd $koTbl name AXRATIO axRatio float
   schemaTransEntryAdd $koTbl name ROW row float
   schemaTransEntryAdd $koTbl name ROWERR rowErr float
   schemaTransEntryAdd $koTbl name COL col float
   schemaTransEntryAdd $koTbl name COLERR colErr float
   schemaTransEntryAdd $koTbl name NROW nrow int
   schemaTransEntryAdd $koTbl name NCOL ncol int
   schemaTransEntryAdd $koTbl name OBJC_ID objc_id int
   return $koTbl
}


proc starcTbl {ncolors} {
   set starcTbl [schemaTransNew]

  schemaTransEntryAdd $starcTbl name ID id int
  schemaTransEntryAdd $starcTbl name RA ra double
  schemaTransEntryAdd $starcTbl name RAERR raErr double
  schemaTransEntryAdd $starcTbl name DEC dec double
  schemaTransEntryAdd $starcTbl name DECERR decErr double
  schemaTransEntryAdd $starcTbl name NCOLORS ncolors int

  schemaTransEntryAdd $starcTbl name MJD mjd double

   loop i 0 $ncolors {
    schemaTransEntryAdd $starcTbl name PASSBAND$i \
	mag\[$i\] struct -dimen $ncolors
      schemaTransEntryAdd $starcTbl cont mag\[$i\]\
        passBand char
    schemaTransEntryAdd $starcTbl name MAG$i \
        mag\[$i\] struct -dimen $ncolors
      schemaTransEntryAdd $starcTbl cont mag\[$i\] \
        mag float
    schemaTransEntryAdd $starcTbl name MAGERR$i \
        mag\[$i\] struct -dimen $ncolors
      schemaTransEntryAdd $starcTbl cont mag\[$i\] \
        magErr float
   }

  return $starcTbl
}




proc extinctionTbl {ncolors} {
  set extinctionTbl [schemaTransNew]

  schemaTransEntryAdd $extinctionTbl name NCOLORS ncolors int

  schemaTransEntryAdd $extinctionTbl name MJD mjd double

  loop i 0 $ncolors {
    schemaTransEntryAdd $extinctionTbl name PASSBAND$i \
        k\[$i\] struct -dimen $ncolors
    schemaTransEntryAdd $extinctionTbl cont k\[$i\] \
        passBand char
    schemaTransEntryAdd $extinctionTbl name MAG$i \
        k\[$i\] struct -dimen $ncolors 
    schemaTransEntryAdd $extinctionTbl cont k\[$i\] \
        mag float
    schemaTransEntryAdd $extinctionTbl name MAGERR$i \
        k\[$i\] struct -dimen $ncolors 
    schemaTransEntryAdd $extinctionTbl cont k\[$i\] \
        magErr float
  }

  return $extinctionTbl
}


proc frameinfoTbl {} {
  set frameinfoTbl [schemaTransNew] 
  schemaTransEntryAdd $frameinfoTbl name FRAMENUM frameNum int
  schemaTransEntryAdd $frameinfoTbl name AIRMASS airmass float
  
  schemaTransEntryAdd $frameinfoTbl name MJD mjd double

  return $frameinfoTbl
}  

proc fieldstatTbl {ncolor} {
  set fieldstatTbl [schemaTransNew]
  schemaTransEntryAdd $fieldstatTbl name FRAMENUM frameNum int
  schemaTransEntryAdd $fieldstatTbl name NOBJECTS nobjects int
  schemaTransEntryAdd $fieldstatTbl name NSTARS nstars int
  schemaTransEntryAdd $fieldstatTbl name NGALS ngals int
  schemaTransEntryAdd $fieldstatTbl name NCOLORS ncolor int
  loop i 0 $ncolor {
    schemaTransEntryAdd $fieldstatTbl name MINPIX$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] minpix float

    schemaTransEntryAdd $fieldstatTbl name MAXPIX$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] maxpix float

    schemaTransEntryAdd $fieldstatTbl name MEANPIX$i filter\[$i\] \
        struct -dimen  $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] meanpix float

    schemaTransEntryAdd $fieldstatTbl name SIGPIX$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] sigpix float

    schemaTransEntryAdd $fieldstatTbl name SKY$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] sky float

    schemaTransEntryAdd $fieldstatTbl name NBADPIX$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] nbadpix int

    schemaTransEntryAdd $fieldstatTbl name NBRIGHTOBJ$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] nbrightobj int

    schemaTransEntryAdd $fieldstatTbl name NFAINTOBJ$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] nfaintobj int

    schemaTransEntryAdd $fieldstatTbl name GAIN$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] gain float

    schemaTransEntryAdd $fieldstatTbl name DARK_VARIANCE$i filter\[$i\] \
        struct -dimen $ncolor
    schemaTransEntryAdd $fieldstatTbl cont filter\[$i\] dark_variance float
  }
  return $fieldstatTbl
}

# Schema to tblcol conversion table
proc psTbl {nrow ncol} {

  set psTbl [schemaTransNew]
  schemaTransEntryAdd $psTbl name SROW row0 int
  schemaTransEntryAdd $psTbl name SCOL col0 int
  schemaTransEntryAdd $psTbl name PIXVAL rows_u16 {unsigned short} \
    -dimen "$nrow x $ncol"
  return $psTbl
}

# Schema to tblcol conversion table for calib1bytime struct
proc calib1bytimeTbl {ncolors} {

  set calib1bytimeTbl [schemaTransNew]

  schemaTransEntryAdd $calib1bytimeTbl name MJD mjd double

  schemaTransEntryAdd $calib1bytimeTbl name NCOLORS ncolors int
  loop i 0 $ncolors {

    schemaTransEntryAdd $calib1bytimeTbl name FILTER$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1bytimeTbl cont calib\[$i\] filter SCHAR

    schemaTransEntryAdd $calib1bytimeTbl name FLUX20$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1bytimeTbl cont calib\[$i\] flux20 float

    schemaTransEntryAdd $calib1bytimeTbl name FLUX20ERR$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1bytimeTbl cont calib\[$i\] flux20Err float

  }
  return $calib1bytimeTbl
}


# Schema to tblcol conversion table
proc totransTbl {} {

  set totransTbl [schemaTransNew]
  schemaTransEntryAdd $totransTbl name FILTER filter char
  schemaTransEntryAdd $totransTbl name FRAME frame int
  schemaTransEntryAdd $totransTbl name A trans struct
  schemaTransEntryAdd $totransTbl cont trans a double
  schemaTransEntryAdd $totransTbl name B trans struct
  schemaTransEntryAdd $totransTbl cont trans b double
  schemaTransEntryAdd $totransTbl name C trans struct
  schemaTransEntryAdd $totransTbl cont trans c double
  schemaTransEntryAdd $totransTbl name D trans struct
  schemaTransEntryAdd $totransTbl cont trans d double
  schemaTransEntryAdd $totransTbl name E trans struct
  schemaTransEntryAdd $totransTbl cont trans e double
  schemaTransEntryAdd $totransTbl name F trans struct
  schemaTransEntryAdd $totransTbl cont trans f double
  return $totransTbl
}
# Schema to tblcol conversion table
proc transTbl {} {

  set transTbl [schemaTransNew]
  schemaTransEntryAdd $transTbl name A a double
  schemaTransEntryAdd $transTbl name B b double
  schemaTransEntryAdd $transTbl name C c double
  schemaTransEntryAdd $transTbl name D d double
  schemaTransEntryAdd $transTbl name E e double
  schemaTransEntryAdd $transTbl name F f double
  return $transTbl
}

# Schema to tblcol conversion table for calib1byframe struct
# without the trans structures.
proc calib1byframeTbl {ncolors} {

  set calib1byframeTbl [schemaTransNew]
  schemaTransEntryAdd $calib1byframeTbl name FRAMENUM frameNum int
  schemaTransEntryAdd $calib1byframeTbl name NCOLORS ncolors int
  loop i 0 $ncolors {

    schemaTransEntryAdd $calib1byframeTbl name FILTER$i calib\[$i\] \
	struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] filter SCHAR

    schemaTransEntryAdd $calib1byframeTbl name SKY$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] sky float

    schemaTransEntryAdd $calib1byframeTbl name SKYSLOPE$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] skyslope float

    schemaTransEntryAdd $calib1byframeTbl name LBIAS$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] lbias float

    schemaTransEntryAdd $calib1byframeTbl name RBIAS$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] rbias float

    schemaTransEntryAdd $calib1byframeTbl name FLUX20$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] flux20 float

    schemaTransEntryAdd $calib1byframeTbl name FLUX20ERR$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] flux20Err float

    schemaTransEntryAdd $calib1byframeTbl name PSFSIGX1$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf sigmax1 float

    schemaTransEntryAdd $calib1byframeTbl name PSFSIGX2$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf sigmax2 float

    schemaTransEntryAdd $calib1byframeTbl name PSFSIGY1$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf sigmay1 float

    schemaTransEntryAdd $calib1byframeTbl name PSFSIGY2$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf sigmay2 float

    schemaTransEntryAdd $calib1byframeTbl name PSFB$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf b float

  }
  return $calib1byframeTbl
}



# Schema to tblcol conversion table for calib1byframe struct
# including the trans structures.
proc cframeTbl {ncolors} {

  set calib1byframeTbl [schemaTransNew]
  schemaTransEntryAdd $calib1byframeTbl name FRAMENUM frameNum int
  schemaTransEntryAdd $calib1byframeTbl name NCOLORS ncolors int
  loop i 0 $ncolors {

    schemaTransEntryAdd $calib1byframeTbl name FILTER$i calib\[$i\] \
	struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] filter SCHAR

    schemaTransEntryAdd $calib1byframeTbl name SKY$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] sky float

    schemaTransEntryAdd $calib1byframeTbl name SKYSLOPE$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] skyslope float

    schemaTransEntryAdd $calib1byframeTbl name LBIAS$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] lbias float

    schemaTransEntryAdd $calib1byframeTbl name RBIAS$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] rbias float

    schemaTransEntryAdd $calib1byframeTbl name FLUX20$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] flux20 float

    schemaTransEntryAdd $calib1byframeTbl name FLUX20ERR$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] flux20Err float

    schemaTransEntryAdd $calib1byframeTbl name PSFSIGX1$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf sigmax1 float

    schemaTransEntryAdd $calib1byframeTbl name PSFSIGX2$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf sigmax2 float

    schemaTransEntryAdd $calib1byframeTbl name PSFSIGY1$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf sigmay1 float

    schemaTransEntryAdd $calib1byframeTbl name PSFSIGY2$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf sigmay2 float

    schemaTransEntryAdd $calib1byframeTbl name PSFB$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] psf struct \
      -proc dgpsfNew
    schemaTransEntryAdd $calib1byframeTbl cont psf b float

#    schemaTransEntryAdd $calib1byframeTbl name TOREFCOLORA$i calib\[$i\] \
#      struct -dimen $ncolors
#    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toRefcolor struct \
#      -proc transNew
#    schemaTransEntryAdd $calib1byframeTbl cont toRefcolor a double
#
#    schemaTransEntryAdd $calib1byframeTbl name TOREFCOLORB$i calib\[$i\] \
#      struct -dimen $ncolors
#    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toRefcolor struct \
#      -proc transNew
#    schemaTransEntryAdd $calib1byframeTbl cont toRefcolor b double
#
#    schemaTransEntryAdd $calib1byframeTbl name TOREFCOLORC$i calib\[$i\] \
#      struct -dimen $ncolors
#    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toRefcolor struct \
#      -proc transNew
#    schemaTransEntryAdd $calib1byframeTbl cont toRefcolor c double
#
#    schemaTransEntryAdd $calib1byframeTbl name TOREFCOLORD$i calib\[$i\] \
#      struct -dimen $ncolors
#    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toRefcolor struct \
#      -proc transNew
#    schemaTransEntryAdd $calib1byframeTbl cont toRefcolor d double
#
#    schemaTransEntryAdd $calib1byframeTbl name TOREFCOLORE$i calib\[$i\] \
#      struct -dimen $ncolors
#    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toRefcolor struct \
#      -proc transNew
#    schemaTransEntryAdd $calib1byframeTbl cont toRefcolor e double
#
#    schemaTransEntryAdd $calib1byframeTbl name TOREFCOLORF$i calib\[$i\] \
#      struct -dimen $ncolors
#    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toRefcolor struct \
#      -proc transNew
#    schemaTransEntryAdd $calib1byframeTbl cont toRefcolor f double

    schemaTransEntryAdd $calib1byframeTbl name NODE$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] node float

    schemaTransEntryAdd $calib1byframeTbl name INCL$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] incl float

    schemaTransEntryAdd $calib1byframeTbl name TOGCCA$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toGCC struct \
      -proc transNew
    schemaTransEntryAdd $calib1byframeTbl cont toGCC a double

    schemaTransEntryAdd $calib1byframeTbl name TOGCCB$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toGCC struct \
      -proc transNew
    schemaTransEntryAdd $calib1byframeTbl cont toGCC b double

    schemaTransEntryAdd $calib1byframeTbl name TOGCCC$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toGCC struct \
      -proc transNew
    schemaTransEntryAdd $calib1byframeTbl cont toGCC c double

    schemaTransEntryAdd $calib1byframeTbl name TOGCCD$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toGCC struct \
      -proc transNew
    schemaTransEntryAdd $calib1byframeTbl cont toGCC d double

    schemaTransEntryAdd $calib1byframeTbl name TOGCCE$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toGCC struct \
      -proc transNew
    schemaTransEntryAdd $calib1byframeTbl cont toGCC e double

    schemaTransEntryAdd $calib1byframeTbl name TOGCCF$i calib\[$i\] \
      struct -dimen $ncolors
    schemaTransEntryAdd $calib1byframeTbl cont calib\[$i\] toGCC struct \
      -proc transNew
    schemaTransEntryAdd $calib1byframeTbl cont toGCC f double


  }
  return $calib1byframeTbl
}
