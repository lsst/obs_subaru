if(!$?OBJ1_SATUR) {
   define HOME :
   RESTORE "$!HOME/photo/misc/utils.sav"
}

macro class_colour 22 {
   # Usage: class_colour type logical
   local define type "$!1"
   local define ll "$!2"

   local set rat = atan2(exp(exp_lnL2-deV_lnL2),1)*2/pi + 0.01*(0.5 - random(dimen(exp_L2)))

   window 1 -10 1 2:9

   lim (<0 5>) (rat>0?rat:0)
   #lim (<0 2.3>) (<0 0.2>)
   box
   lt 1 rel 2.3 $fy1 draw 2.3 $fy2 lt 0

   photoL 2
   define l "objc_type == 'galaxy' && good && !blended && $ll"
   local set col = ($type""Mag0-$type""Mag2)

   #local set pt = id if($l)  ptype pt
   poi col rat if($l)
   pt 4 1
   
   yla (2/\pi) atan(L_{exp,r'}/L_{deV,r'}) + 0.05*random()
   #
   # Now marginal distributions
   #
   local set x = $fx1,$fx2,0.1
   set h local

   local set zi = 0.2
   lt 1
   rel $fx1 $(zi) draw $fx2 $(zi)
   rel $fx1 $(1-zi) draw $fx2 $(1-zi)
   lt 0

   window 1 -10 1 1:1
   set h=col if($l && rat < zi) set h = histogram(h:x)
   lim 0 0 (x == $fx1 || x == $fx2 ? 0 : h) box
   histogram x h 

   window 1 -10 1 10:10
   set h=col if($l && rat > 1 - zi) set h = histogram(h:x)
   lim 0 0 (x == $fx1 || x == $fx2 ? 0 : h) box
   histogram x h 
   
   
   window 1 1 1 1

   xla \2u'_{$type} - r'_{$type}
   toplabel $l ($(sum($l)) objects)
}

macro class_concentration 22 {
   # Usage: class_concentration type logical
   local define type "$!1"
   local define ll "$!2"

   local set rat = atan2(exp_L2,deV_L2)*2/pi + 0.05*(0.5 - random(dimen(exp_L2)))

   window 1 -10 1 2:9

   lim (<0.2 0.65>) (rat>0?rat:0)
   #lim (<0 2.3>) (<0 0.2>)
   box
   lt 1 rel 2.3 $fy1 draw 2.3 $fy2 lt 0

   photoL 2
   define l "objc_type == 'galaxy' && good && !blended && $ll"
   local set conc = petroR502/petroR902 

   #local set pt = id if($l)  ptype pt
   poi conc rat if($l)
   pt 4 1
   
   yla (2/\pi) atan(L_{exp,r'}/L_{deV,r'}) + 0.05*random()
   #
   # Now marginal distributions
   #
   local set x = $fx1,$fx2,0.01
   set h local

   local set zi = 0.2
   lt 1
   rel $fx1 $(zi) draw $fx2 $(zi)
   rel $fx1 $(1-zi) draw $fx2 $(1-zi)
   lt 0

   window 1 -10 1 1:1
   set h=conc if($l && rat < zi) set h = histogram(h:x)
   lim 0 0 (x == $fx1 || x == $fx2 ? 0 : h) box
   histogram x h 

   window 1 -10 1 10:10
   set h=conc if($l && rat > 1 - zi) set h = histogram(h:x)
   lim 0 0 (x == $fx1 || x == $fx2 ? 0 : h) box
   histogram x h 
   
   
   window 1 1 1 1

   xla \2R_{50}/R_{90}
   toplabel $l ($(sum($l)) objects)
}


macro class_colour_concentration 14 {
   # Usage: class_colour_concentration type [logical] [errors] [ratio-coded]
   #
   # Plot R90/R50 agaoinst u-r, possibly colour-coded by type or Lexp/LdeV
   #
   local define type "$!1"
   if(!$?2) { define 2 1 }
   local define ll "$!2"
   if(!$?3) { define 3 0 }
   local define errors $3
   if(!$?4) { define 4 0 }
   local define use_rat $4

   photoL 2

   local set col = ($type""Mag0-$type""Mag2)
   local set conc = petroR902/petroR502
   set x local  set y local
   if($errors) {
      local set colErr = sqrt($type""MagErr0**2 + $type""MagErr2**2)
      local set concErr = conc*sqrt((petroR50Err2/petroR502)**2 + (petroR90Err2/petroR902)**2)
      set dx local  set dy local
   }

   define l "objc_type == 'galaxy' && good && !blended && $ll"

   lim -0.25 4.5 1 5 box

   local define labels 0
   if($?_ix && $?_iy) {
      if($_ix == 1 && $_iy == 1) {
         define labels 1
         
         xla u' - r' ($type)
         yla R50/R90
      }
   }
   if('$ll' != '1') {
      rel $($fx1 + 0.1*($fx2 - $fx1)) $($fy1 + 0.1*($fy2 - $fy1))
      label $ll
   }

   OVERLOAD exp 0
   define v local define j local  set _l local
   local define nmod 50
   #define nmod 1
   local set rand = $nmod*random(dimen(type))

   if(sum(index(ctype(string), 'purple') == 0) {
      add_ctype purple 100 0 255
   }

   if($use_rat) {
      local set rat = atan2(EXP(exp_lnL2 - deV_lnL2), 1)*2/pi
      set _rat local

      local set dc =   {0.2 0.4    0.5   0.6  0.8  1.0}
      local set cols = {red yellow green cyan blue purple}

      if($labels) {
         rel $fx1 $($fy1 + 0.9*($fy2-$fy1)) putl 6  \2L_{exp}/L_{deV}
         do v=0,dimen(dc) - 1 {
            rel $($fx1 + (0.2 + $v/dimen(dc)*0.7)*($fx2-$fx1)) $($fy1 + 0.9*($fy2-$fy1))
            ct $(cols[$v])
            label $(sprintf('%.1f', dc[$v]))
            ct 0
         }
      }
         
      set dc = 0.0 concat dc  set cols = 'default' concat cols
   }

   do j=0,$nmod - 1 {
      set _l = $l && (rand%$nmod == $j)
      if($use_rat) {                    # plot by Lratio
         set x = col if(_l)
         set y = conc if(_l)
         set _rat = rat if(_l)
         do v=1,dimen(dc) - 1{
            ct $(cols[$v])
            if(dimen(_rat) > 0) {
               poi x y if(_rat > dc[$v-1] && _rat < dc[$v])
            }
         }
         ct 0
      } else {
         foreach v {exp deV galaxy} {
            ctype_from_type $v
            set x = col if(_l && type == '$v')
            set y = conc if(_l && type == '$v')
            
            if(!$errors) {
               poi x y
            } else {
               set dx = colErr if(_l && type == '$v')
               set dy = concErr if(_l && type == '$v')
               error_x x y dx
               error_y x y dy
            }
         }
      }
   }
   ct 0
}


macro morph_colour 56 {
   OVERLOAD exp 1
   local define t1 $1
   local define t2 $2
   local define c1 $3
   local define c2 $4
   local define l "$!5"
   if(!$?6) { define 6 1 }

   lim (<-.5 4>) ($6*<-1 1>)
   box

   define t local
   set xvec local  set pt local
   #expand 0.5
   foreach t {exp deV} {
      ctype_from_type $t

      set xvec = $t1""Mag$c1 - $t1""Mag$c2
      set xvec = modelMag$c1 - modelMag$c2
      #set pt = id if($l && type == '$t')  ptype pt
      points xvec ($t2""Mag$c1 - $t2""Mag$c2 - xvec) \
       if($l&&type=='$t')
      ptype 4 1
      ct 0
   }
   expand 1

   #xla ($t1""Mag$c1 - $t1""Mag$c2)
   xla modelMag$c1 - modelMag$c2
   #yla ($t2""Mag$c1 - $t2""Mag$c2) - ($t1""Mag$c1 - $t1""Mag$c2)
   yla \2 $t1 - $t2 : (Mag$c1 - Mag$c2)
   toplabel $l

   rel $fx1 $fy2
   if($c2 < $c1) {
      putl 7 $t1 is Redder
   } else {
      putl 7 $t1 is Bluer
   }
}

macro number_counts 24 {
   # Plot a luminosity function in magnitude type $1, band $2, if $3 is true.
   # Only plot objects of type $4 (SG: plot stars/galaxies, or star, deV, etc)
   #
   photoL $2
   
   if($?3) {
      local set l = $3
   } else {
      local set l = 1
   }

   if(!$?4) {
      define 4 none
   }

   local set x=do(13, 24, 0.25) + 0.25/2
   local set y = $1Mag$2 if(l)
   local set type = type if(l)

   lim x (0 concat lg(sum(l)))
   ti 0 0 -1 0  notation 0 0 30 30
   bo
   ti 0 0 0 0 notation 0 0 0 0

   set lf local
   if('$4' == 'none') {
      set lf = histogram(y : x)
      set lf = (lf == 0) ? -1 : lg(lf)
      histogram x lf
   } else {
      if('$4' == 'SG') {
         local define types {galaxy class deV exp star}
      } else {
         local define types <$4>
      }
      define v local
      overload exp 1
      foreach v <$types> {
         overload exp 0
         ctype_from_type $v
         if('$v' == 'galaxy') {
            set lf = y if(type == 'deV' || type == 'exp' || type == 'galaxy')
         } else {if('$v' == 'class') {
            set lf = y if(type == 'deV' || type == 'exp')
         } else {
            set lf = y if(type == '$v')
         }}

         set lf = histogram(lf : x)
         set lf = (lf == 0) ? -1 : lg(lf)
         histogram x lf
      }
      ct 0
   }
   if($?_iy) {
      if($_iy == 1) {
         xla $1
      }
   }
   if($?_ix) {
      if($_ix == 1) {
         yla \phi
      }
   }
   
   relocate $($fx1+0.1*($fx2-$fx1))  $($fy1+0.9*($fy2-$fy1))
   putl 6 Band: $2
}

macro read_psField 56 {
   # Usage: read_psField root run[:rerun] camCol field0 field1 [composite profile?]
   # E.g. read_psField $root/testbed 745:666 4 450 470
   local define root "$!1"
   local define run "$!2"
   define col $3
   define f0 $4
   define f1 $5
   if(!$?6) { define 6 0 }
   local define read_profiles $6

   define rerun -1
   define run "$!run"
   if(index('$run', ':') >= 0) {
      define rerun   ( substr('$run', index('$run', ':') + 1, 0) )
      define run     ( substr('$run', 0, index('$run', ':')) )
   }
   
   set dimen(PSP_status)=$($f1 + 1)
   define i local  define j local
   do i=0,$NFILTER - 1 {
      set dimen(PSP_status$i)=$($f1 + 1)
      set dimen(PSP_sky$i)=$($f1 + 1)
      set dimen(PSF_width$i)=$($f1 + 1)
      set dimen(PSF_ap_corrErr$i)=$($f1 + 1)
      set dimen(PSF_sigma1$i)=$($f1 + 1)
      set dimen(PSF_sigma2$i)=$($f1 + 1)
      set dimen(PSF_b$i)=$($f1 + 1)
      set dimen(ap_corr$i)=$($f1 + 1)
      set dimen(ap_corrErr$i) = $($f1 + 1)
   }

   set psp_status local
   do i=0,$NFILTER - 1 {
      set status$i local
      set psf_width$i local
      set psf_ap_correctionErr$i local
      set sky$i local
   }

   if($read_profiles) {
      do i=0,$NFILTER - 1 {
         set prof_nprof$i local
      }
      do i=0,74 {
         set prof_mean$i local
      }
   }

   define f local
   do f=$f0,$f1 {
      set_run $root $run $col 0 $rerun

      table $(1 + $NFILTER) \
          "$!photodata/psField-$!run-$!col-$!(sprintf('%04d',$!f)).fit"
      read table {psp_status status[0-4] psf_width[0-4] \
             psf_sigma1_2G[0-4] psf_sigma2_2G[0-4] psf_b_2G[0-4] \
             psf_ap_correctionErr[0-4] sky[0-4] }
      if($read_profiles) {
         read table { prof_nprof[0-4] prof_mean[0-74] }
         do i=0,$NFILTER - 1 {
            set dimen(profMean$f""_$i) = $(prof_nprof$i)
            do j = 0, prof_nprof$i - 1 {
               set profMean$f""_$i[$j] = prof_mean$(15*$i + $j)
            }
         }
      }
      set PSP_status[$f] = psp_status
      do i=0,$NFILTER - 1 {
         set PSP_status$i[$f] = status$i
         set PSP_sky$i[$f] = sky$i
         set PSF_width$i[$f] = psf_width$i
         set PSF_ap_corrErr$i[$f] = psf_ap_correctionErr$i
         set PSF_sigma1$i[$f] = psf_sigma1_2G$i
         set PSF_sigma2$i[$f] = psf_sigma2_2G$i
         set PSF_b$i[$f] = psf_b_2G$i
      }
      
      table $(3 + $NFILTER)
      read table < ap_corr_run ap_corr_runErr >
      do i=0,$NFILTER - 1 {
         set ap_corr$i[$f] = ap_corr_run[$i]
         set ap_corrErr$i[$f] = ap_corr_runErr[$i]
      }
   }
   set PSP_field = do(0,$f1) set PSP_field = (PSP_field < $f0) ? -1 : PSP_field

   set I1 local  set I2 local set neff local
   do i=0,$NFILTER - 1 {
      #set PSF_b$i = 0
      set I1 = 2*pi*(PSF_sigma1$i**2 + PSF_b$i*PSF_sigma2$i**2)
      set I2 = 2*pi*(PSF_sigma1$i**2/2 + \
          2*PSF_b$i*(PSF_sigma1$i**2*PSF_sigma2$i**2)/(PSF_sigma1$i**2 + PSF_sigma2$i**2) + \
          PSF_b$i**2*PSF_sigma2$i**2/2)
      set neff = I1**2/I2
      set PSF_width_2G$i = 0.400*2*sqrt(2*ln(2))*sqrt(neff/(4*pi))
   }
}

macro plot_psField 35 {
   # Usage: what ylim1 ylim2 [windows?] [error-vec]
   local define what "$!1"
   if(!$?4) { define 4 0 }
   local define windows $4

   if(!$?5) {
      define 5 "none"
   }
   local define errvec $5

   lim (<$f0 $f1>) (<$2 $3>)
   if($2 == $3) {
      lim 0 0 \
     ($what""0 concat $what""1 concat $what""2 concat $what""3 concat $what""4)
   }

   if($windows) {
      set_window 1 -$NFILTER $($NFILTER - 1)
   } else {
      box
   }
   do i=0,$NFILTER - 1 {
      if($windows) {
         set_window 1 -$NFILTER
         box
      }
      lt 1 rel $fx1 0 draw $fx2 0 lt 0
      ctype_band $i
      con PSP_field $what""$i if(PSP_field > 0)
      if('$errvec' != 'none') {
         error_y PSP_field $what""$i $errvec""$i
      }
      ct 0
   }
   if($windows) {
      set_window 1 1
   }
   ct 0
   xla Field
   yla $(quote_TeX('$what'))
   toplabel Column $col, fields $f0--$f1
   if(!$windows) {
      do i=0,4 {
         ctype_band $i label " $!(filter_name($i))"
      }
      ctype 0
   }
}

macro plot_profMean 23 {
   local define f $1                    # field
   local define b $2                    # band
   if(!$?3) { define 3 0 }		# flags
   local define flags ( $3 )
   local define log ( $flags & 0x1 )
   local define area ( $flags & 0x2 )
   
   local set p = profMean$f""_$b
   local set radii = profileRadii()
   local set r = radii if(do(1,dimen(radii)) <= dimen(p))

   local set plim = {1 1e-6}

   if($area) {
      local set area = pi*((r concat r[dimen(r)-1])**2 - (0 concat r)**2) \
       if(do(1,dimen(r) + 1) <= dimen(p))
      local set flux = sum(p*area)
      set p = p/flux*2e3
   }

   if($log) {
      set p = lg(p)
      set r = lg(r)
      lim r (3 + lg(plim))
      lim -0.5 1.8 0 0

      local define xla "lg(r/pixels)"
      local define yla "lg(profMean)"
   } else {
      lim r (1000*plim)
      if($area) { set p = p*5 }
      local define xla "r/pixels"
      local define yla "profMean"
   }

   box
   ct red
   poi r p
   ctype_band $b
   connect r p
   ct 0

   xla $xla
   yla $yla
}

macro read_blends 11 {
   da $1
   read { run 1 camCol 2 field 3 id 4 rowc 5 colc 6 dr 8 \
          m0_1 11 m0_2 12 m0_3 13 m1_1 17 m1_2 18 m1_3 19 m2_1 23 m2_2 24 m2_3 25}
   set id = string(run) + '-' + string(camCol) + '-' + string(field) + ':' + string(id)
   set dr=0.400*dr                      # arcsec
}

macro analyse_blends 25 {
   # Usage: analyse_blends rotate maglim [dr0] [dr1] [ascii_file]
   local define rotate $1
   local set maglim = $2
   local set dr0 = 0 local set dr1 = 0
   if($?3) { set dr0 = $3 }
   if($?4) { set dr1 = $4 }
   if(!$?5) { define 5 none }

   if('$5' != 'none') {
      read_blends $5
   }

   local set l = (\
       m1_1 < maglim && m2_1 < maglim && \
       m1_2 < maglim && m2_2 < maglim && \
       m1_3 < maglim && m2_3 < maglim) ? 1 : 0
       
   set l = l && (dr >= dr0 && (dr1 == 0 || dr <= dr1)) ? 1 : 0

   #set l = (l && camCol == 4) ? l : 0
          
   define i local
   do i=0,2 {
      set c$i""_12 = m$i""_1 - m$i""_2 if(l)
      set c$i""_23 = m$i""_2 - m$i""_3 if(l)
   }
   
   set rx local  set ry local
   
   if(0) {
      set x=do(-1,1,.1) - 0.05
      rotate_gri c1_12 c1_23 rx ry1 $rotate
      rotate_gri c2_12 c2_23 rx ry2 $rotate
      
      set h=histogram(ry1 - ry2:x)
      lim x h box
      hi x h
      xla r-i_{brighter} - r-i_{fainter}
      ct 0
   } else {
      set gr_min = 0.2                  # max. allowed g-r
      set gr_max = 1.15                 # max. allowed g-r
      set ri_min = 0.05                 # max. allowed r-i
      set ri_max = 0.75                 # max. allowed r-i
      
      window -10 1 1:6 1
      define xla "g - r"\n define yla "r - i"
      if($rotate) {
         lim -0.4 2 ({-0.25 0.75})
         define yla "$!yla  (sheared)"
      } else {
         lim -1 2 -0.5 1.5
      }
      box
      xla $xla
      yla $yla
      
      if(0 && $rotate) {                # draw g-r and r-i axes (broken)
         set xx=($fx2-$fx1)*<0.0 0.0 0.1>
         set yy=($fy2-$fy1)*<0.1 0.0 0.0>
         rotate_gri xx yy rx ry $rotate
         con rx ry
      }
      
      if(dimen(c0_12) > 200) {
         pt 4 0
         expand 0.5
      }
      
      set hx = $fy1,$fy2,0.02
      
      define i 0
      rotate_gri c$i""_12 c$i""_23 rx ry $rotate
      poi c$i""_12 ry
      
      set hy$i = ry if(c$i""_12 >= gr_min && c$i""_12 < gr_max && \
          c$i""_23 >= ri_min && c$i""_23 < ri_max) 
      set hy$i = histogram(hy$i : hx)
          
      ct blue
      define i 1
      rotate_gri c$i""_12 c$i""_23 rx ry $rotate
      poi c$i""_12 ry
      
      set hy$i = ry if(c$i""_12 >= gr_min && c$i""_12 < gr_max && \
          c$i""_23 >= ri_min && c$i""_23 < ri_max) 
      set hy$i = histogram(hy$i: hx)
          
      ct red
      define i 2
      rotate_gri c$i""_12 c$i""_23 rx ry $rotate
         
      #set pt = '\\-4' + substr(id,4,0) if(l)  ptype pt
      poi c$i""_12 ry
         
      set hy$i = ry if(c$i""_12 >= gr_min && c$i""_12 < gr_max && \
             c$i""_23 >= ri_min && c$i""_23 < ri_max) 
      set hy$i = histogram(hy$i : hx)
      
      ct 0
      pt 4 1 expand 1
      
      if($rotate) {
         define ymin $fy1  define ymax $fy2
      } else {
         define ymin (ri_min)  define ymax (ri_max)
      }
      lt 1
      con (gr_min concat gr_min concat gr_max concat gr_max concat gr_min) \
          ($ymin concat $ymax concat $ymax concat $ymin concat $ymin)
      lt 0
          
      window -10 1 7:9 1
      lim (hy0 concat hy1 concat hy2) 0 0
      BOX 1 0 0 2
      xla N
      histogram hy0 hx
      ct blue
      histogram hy1 hx
      ct red
      histogram hy2 hx
      ct 0
      
      window 1 1 1 1
      toplabel maglim: $(maglim)  $(dr0)'' \le{} \Delta r
      if(dr1 > 0) {
         label  \le{} $(dr1)''
      }
   }
}

macro analyse_separations 14 {
   # Usage: analyse_separations maglim [normalise] [errors] [ascii_file]
   # normalise:
   #   0 : plot N(r)
   #   1 : plot N(r)/r
   #   2 : plot N(r)_tot/N(r)_blue
   local set maglim = $1
   define i local do i=2,4 {
      if(!$?$i) { define $i 0 }
   }
   local define normalise $2
   local define errors $3
   if('$4' == '0') { define 4 none }\n  define file "$!4"

   if('$file' != 'none') {
      read_blends $file
   }

   local set l = (\
       m1_1 < maglim && m2_1 < maglim && \
       m1_2 < maglim && m2_2 < maglim) ? 1 : 0
       
   local set x = 0,15,1  set x = x - 0.5*(x[1] - x[0])
   local set dcolor = (m2_1 - m2_2) - (m1_1 - m1_2) # < 0 => fainter is bluer
   local set cut = -0.5

   local set hR = dr if(l && dcolor > cut)
   set hR = histogram(hR:x) local set hRErr = sqrt(hR)

   local set hB = dr if(l && dcolor <= cut)
   set hB = histogram(hB:x) local set hBErr = sqrt(hB)

   if(1) {
      local set foo = sprintf('%.2f  ', dr) + id + sprintf('  (%.2f,',rowc) + \
       sprintf('%.2f)',colc) if(l && dr < 3)
      local set goo = m0_2 if(l && dr < 3) 
      print { foo goo }
   }

   if($normalise == 0) {
      
   } else { if($normalise == 1) {
      foreach i (B R) {
         set h$i = h$i/x
         set h$i""Err = h$i""Err/x
      }
   } else { if($normalise == 2) {
      set hRErr = sqrt((hRErr/hB)**2 + (hR*hBErr/hB**2)**2)
      set hR = (hR + hB)/hB
      local set fac = sum(hR)/sum(hB/x)/5
      set hB = hB/x*fac
      set hBErr = hBErr/x*fac
   }}}
   
   lim x (hR concat hB)
   box
   ct red
   hi x hR
   if($errors) {
      error_y x hR hRErr
   }

   ct cyan
   hi (x+0.1*(x[1]-x[0])) hB
   if($errors) {
      error_y x hB hBErr
   }
   ct 0
   
   xla separation (arcsec)
   if($normalise == 0) {
      yla N(r)
   } else { if($normalise == 1) {
      yla N(r)/r
   } else {
      yla N(r)_{tot,corrected} and N(r)_{blue}
   }}

   toplabel $(sum(l)) binaries g',r' \le{} $(maglim)
}

macro rotate_gri 45 {
   #
   # LSQ line to blue part of gri stellar locus
   # set xx=-1,3 set yy = $b + $a*xx con xx yy
   #
   if(!$?5) { define 5 0 }
   
   if($5) {
      local define a 0.3859660026 define b 0.05377124172
   } else {
      local define a 0
   }
   
   local set theta = atan2(-$a, 1)  
   set $3 = cos(theta)*$1 - sin(theta)*$2
   set $4 = sin(theta)*$1 + cos(theta)*$2
}

macro colour_mag 35 {
   # Usage: colour_mag rotate? type logical [ctype] [histogram-vec]
   overload exp 1
   local define rotate $1
   local define t $2
   local define l "$!3"
   if($?4) { define ct $4 } else { define ct none }
   if($?5) { define hist $5 } else { define hist none }

   local define c1 1  local define c2 2  local define c3 3
   photoL $c1

   local set c1 = $t""Mag$c1 - $t""Mag$c2
   local set c2 = $t""Mag$c2 - $t""Mag$c3

   lim ({-0.25 2.0}-0.0) ({-0.5 2.0})
   if($c1 == 0) {                        # u
      lim ({-0.5 3.0}-0.0) 0 0
   }
   if($rotate) {
      lim -0.4 2 -0.1 0.2
      lim 0 0 ({-0.25 0.75})
   }

   if('$ct' != 'none') {
      ctype $ct
   } else {
      box

      define xla <$(filter_name($c1)) - $(filter_name($c2)) ({\-1$t})>
      define yla <$(filter_name($c2)) - $(filter_name($c3)) ({\-1$t})>
      if($rotate) {
         define yla "$!yla  (sheared)"
      }
      
      xla $xla\n yla $yla
      if(sum($l) != 0) { toplabel $l }
   }

   if(sum($l) == 0) {
      return
   }

   if($rotate) {
      set rx local  set ry local

      rotate_gri c1 c2 rx ry $rotate
   } else {
      local set ry = c2
   }
   poi c1 ry if($l)
   ct 0

   if('$hist' != 'none') {
      local define gr_lim 0.95
      lt 1 rel $gr_lim $fy1  draw $gr_lim $fy2 lt 0
      set $0 = ry if($l && modelMag1 - modelMag2 < $gr_lim)
      set $0 = histogram($0 : $hist)
   } else {
      set $0 = 0
   }
}

macro colour_mag_range 11 {
   define rotate $1
   local set x = -0.3,0.3,0.01
   if($rotate) {
      window -10 1 1:6 1
   }
   colour_mag 1 psf 0
   local define l "type=='star'&&good&&!blended"
   local set h1 = colour_mag($rotate, psf, \
       ($l)&&psfMag1<16, cyan, x)  set h1=h1/sum(h1)
   local set h2 = colour_mag($rotate, psf, \
       ($l)&&psfMag1>18&&psfMag1<18.5, green, x) set h2=h2/sum(h2)
   local set h3 = colour_mag($rotate, psf, \
           ($l)&&psfMag1>19.5&&psfMag1<20, red, x) set h3=h3/sum(h3)
   if($rotate) {
      window -10 1 7:9 1
      lim (x concat x <= $fy1 || x concat x >= $fy2 ? 0 : h1 concat h2) 0 0
      box
      ct cyan  hi h1 x  define mh1 (sprintf('%.3f', sum(x*h1)/sum(h1)))
      ct green hi h2 x  define mh2 (sprintf('%.3f', sum(x*h2)/sum(h2)))
      ct red   hi h3 x  define mh3 (sprintf('%.3f', sum(x*h3)/sum(h3)))
      ct 0
      window 1 1 1 1
   }
   toplabel cyan: Mag1<16 green: 18<Mag1<18.5  red: 19.5<Mag1<20
   toplabel \raise-2000{Means: $mh1 $mh2 $mh3}
}

macro apply_command 129 {
   # Apply command $2..$9 to ranges of field in a run; $1 per plot
   local define df $1
   
   define i local
   do i=2, 9 {
      if(!$?$i) { define $i " " }
   }

   define f0 local  define f1 local
   vecminmax field f0 f1
   local define nfield ( $f1 - $f0 + 1 )
   if($df < 0) {
      local define nw (-$df)
   } else {
      local define nw ( int($nfield/$df) )
      if($nw != int($nw)) { define nw ( $nw + 1 ) }
   }

   if($df < 0) {
      define df ( int($nfield/$nw) )
   }
   
   local define nwx ( int(sqrt($nw)) )  local define nwy $nwx
   while {$nwx*$nwy < $nw} {
      define nwy ( $nwy + 1 )
   }

   #echo $f0 $f1 $nfield  $nw $nwx $nwy

   do i=0,$nwx*$nwy-1 {
      set_window -$nwx -$nwy
      
      if($($f0+($i+1)*$df) <= $f1) {
         $2 $3 $4 $5 $6 $7 $8 $9 l&&field>=$($f0+$i*$df)&&field<$($f0+($i+1)*$df)
         
         #ct magenta draw_arrow $($fx1+1) $($fy2+1)  $($fx1+1 +1) $($fy2+1+ 1.16/0.31) ct 0
         ltype ERASE
         shade 0 ($fx1+($fx2-$fx1)*{0.05 0.25 0.25 0.05}) \
             ($fy1+($fy2-$fy1)*{0.01 0.01 0.11 0.11})
         ltype 0
         con ($fx1+($fx2-$fx1)*{0.05 0.25 0.25 0.05 0.05}) \
             ($fy1+($fy2-$fy1)*{0.01 0.01 0.11 0.11 0.01})
         rel $($fx1+0.15*($fx2-$fx1)) $($fy1+0.05*($fy2-$fy1))
         putl 5 \-1$(int($f0+($i+0.5)*$df))
   }
   }
   set_window
}

macro CMs 11 {
   local set l =  good&&!blended&&type=='star'
   apply_command $1 colours psf psf 1 2
}

#
# Astrometry
#
macro astromErr 36 {
   # Usage: astromErr {row,col} iband magType [logical] [type] [logErr]
   overload ROW 1

   local define rc $1  local define iband $2
   local define mtype $3
   if(!$?4) {
      define 4 1
   }
   local define l "$4"
   if(!$?5) {
      define 5 "none"
   }
   local define what $5
   if($?6) {
      local define logErr $6
   } else {
      local define logErr 0
   }
   

   if('$rc' != 'row' && '$rc' != 'col') {
      user abort "Please specify row or col, not $!rc"
      return
   }

   photoL $iband

   local define astrom_floor 0.08
   #define astrom_floor 0
    
   local set x = $mtype""Mag$iband
   local set y = sqrt($rc""cErr$iband**2 - $astrom_floor**2)
   if(dimen($l) != 1) {
      set x = x if($l)
      set y = y if($l)
      local set flags2$iband = flags2$iband if($l)
      if('$what' == 'ID') {
         local set id = id if($l)
      } else { if('$what' == 'SG') {
         local set type = type if($l)
      }}
   }

   lim x y 
   lim ({14 23}) ({-0.001 1.5})

   if($logErr) {
      set y = lg(y)
      lim 0 0  -3 1
   }

   #lim 17 19 -2.5 -1.4                  # RHL
   #lim x y

   box

   if('$what' == 'ID') {
      ptype id
      poi x y
      ptype 4 1
   } else { 
      local set bc = flags2$iband & (1<<$OBJ2_BINNED_CENTER)
      define v local
      if('$what' == 'SG') {
         foreach v {star exp deV galaxy} {
            ctype_from_type $v

            ptype 4 0 
            poi x y if(type == '$v' && bc)
            ptype 4 1 
            poi x y if(type == '$v' && !bc)
         }
         ct 0
      } else {
         ptype 4 0 
         if('$what' == 'BINNED_CENTER') {
            ct magenta
         }
         poi x y if(bc)
         ptype 4 1 
         if('$what' == 'BINNED_CENTER') {
            ct green
         }
         poi x y if(!bc)
         ct 0
   }}

   xla $band""_{\2$mtype}
   if($logErr) {
      yla lg($rc""cErr) (pixels)
   } else {
      yla $rc""cErr (pixels)
   } 
   local define top "Frames $!frames  $!(dimen(x)) objects"
   if(dimen($l) != 1) {
      define top "$!top  ($!l)"
   }
   toplabel $top
}

###############################################################################

macro astrom_bias 24 {
   #Usage: astrom_bias band scale [logical] [remove_mean]
   local define b $1
   local define sc $2
   if(!$?3) {
      local define l 1
   } else {
      local define l "$!3"
   }
   if(!$?4) {
      local define remove_mean 0
   } else {
      local define remove_mean $4
   }
   
   lim objc_colc objc_rowc 
   #lim objc_colc objc_colc

   box
   if($_iy == 1) {
      xla Colc
   }
   if($_ix == 1) {
      yla Rowc
   }

   photoL $b
   ctype_band $b

   local set l = !bright && good && !(flags2$b & (1<<$OBJ2_BINNED_CENTER)) && ($l)

   local set r=objc_rowc if(l)
   local set c=objc_colc if(l)
   #local set dr=$sc*($fy2 - $fy1)/($fx2 - $fx1)*row_bias$b if(l)
   local set dr=$sc*row_bias$b if(l)
   local set dc=$sc*col_bias$b if(l)
   local set id=id if(l)

   if($remove_mean) {
      set dr = dr - sum(dr)/dimen(dr)
      set dc = dc - sum(dc)/dimen(dc)
   }

   vfield4 c r dc dr
   ct 0

   local set len = sum(sqrt(dr**2 + dc**2))/($sc*dimen(dr))
   #local set len = sum(abs(dr))/($sc*dimen(dr))
   #p dr
   
   echo RMS displacement == $(sprintf('%.3f', len)) pixels \
 == $(sprintf('%.1f', 400*len))mas

   if(0) {
      ct red
      set l = abs(dr) > 10 ? 1 : 0
      poi c r if(l)
      set id = id if(l)
      p id
      ct 0
   }
}

###############################################################################
#
# Central-surface brightness
#
macro central_SB 23 {
   #Usage: central_SB band type [logical]
   local define b $1
   local define type "$!2"
   if(!$?3) {
      local define l 1
   } else {
      local define l "$!3"
   }

   photoL $b

   local define pixelScale 0.400

   if('$type' == 'fiber') {
      local set r = 1.5
      local set mag = fiberMag$b
   } else { if('$type' == 'petro' || '$type' == 'petroR') {
      local set r = $pixelScale*petroRad$b*2
      local set mag = petroMag$b
   } else { if('$type' == 'petroR50') {
      local set r = $pixelScale*petroR50$b 
      local set mag = petroMag$b + 2.5*lg(0.50)
   } else { if('$type' == 'petroR90') {
      local set r = $pixelScale*petroR90$b 
      local set mag = petroMag$b + 2.5*lg(0.90)
   }}}}
     
   local set SB = mag + 2.5*lg(pi*r**2) if($l)
   local set x = 17,26,.5
   local set h = histogram(SB : x)
   lim x h
   box
   hi x h
}

macro petroRatio 0 {
   limits r ({0 1})

   define i local
   do i = 0,4 {
      set_window -2 -3\n bo\n
      ctype_band $i
      con r PR$i if(PR$i >= 0)
      ct 0
      lt 1 rel $fx1 0.2 draw $fx2 $uyp lt 0
      rel $($fx1 + 0.9*($fx2 - $fx1))  $($fy1 + 0.9*($fy2 - $fy1))
      putl 5 \2$(filter_name($i))
   }
   set_window
   xla Radius/pixels
   yla {\sc R}_{Petrosian}
}

macro dmag_rowc 34 {
   # Usage: dmag_rowc magtype1 magtype2 maglim [l]
   #
   # Plot psf - model mag v. rowc
   # if logical l is not provided, use a per-band magnitude limit
   #
   #v 6
   overload exp 1
   local define m1 $1
   local define m2 $2
   overload exp 0
   local define maglim $3

   local define coord rowc

   if(!$?4) {
      define 4 1
   }

   #v 0
   
   define b local  define v local
   set dmag local  set sdmag local  set x local  set l local
   
   do b=0,$NFILTER-1 {
      set_window 1 -$NFILTER
      photoL $b
      set dmag = $m1""Mag$b - $m2""Mag$b 
      set x=objc_$coord

      set l = abs(dmag) < 0.15 && modelMag$b < $maglim && \
          psp_status==0&&good&&!blended&&$4

      foreach v (dmag x) { set $v = $v if(l) }
      lim x dmag
      lim x ((('$m1' == 'psf' || '$m2' == 'psf') ? 0.12 : 0.25)*{-1 1})
      bo

      poi x dmag

      if(0) {
         sort { x dmag }
         smooth dmag sdmag 100
         set l = dmag - sdmag < 0.05
         set x = x if(l)  set dmag = dmag if(l)
         smooth dmag sdmag 30
         ctype red
         connect x sdmag
         ct 0
      }

      if($b == 0) {
         xla $coord
      }
      yla $(filter_name($b))_{$m1} - $(filter_name($b))_{$m2}
   }
   set_window
}

macro dcolor_rowc 46 {
   # Usage: dcolor_rowc magtype1 magtype2 band1 band2 [flags] [l]
   #
   # Plot e.g.
   #     dcolor_rowc psf aper 2 3 0x2 objc_type=='star'
   #            (g-r)_psf - (g-r)_aper v. rowc for stars
   #            (r-i)_psf - (r-i)_aper v. rowc for stars
   # or
   #     dcolor_rowc psf aper 1 2 0x8 objc_type=='star'
   #            (g_psf - g_aper) v. colc for stars
   #            (r_psf - r_aper) v. colc for stars
   #
   # Use bands $band1..$band2, with colours u-g, g-r, r-i, etc.
   # if logical l is not provided, use a per-band magnitude limit
   #
   local define m1 $1
   local define m2 $2
   local define b0 $3
   local define b1 $4
   if(!$?5) { local define flags 0 } else { local define flags ($5) }
   if(!$?6) { define 6 1 }\n local define l "$!6"

   local define smooth  ($flags & 0x1)  # smooth plots v. coord. Used $nsmooth
   					# if defined
   local define color   ($flags & 0x2)  # plot difference in colour as
                                        # measured two ways, not mag difference
   local define hist    ($flags & 0x4)  # plot histograms, not v. coord
   local define by_colc ($flags & 0x8)  # plot as a function of colc, not field

   if($by_colc) {
      local define coord colc
   } else {
      local define coord field
   }
   if($smooth) {
      if($hist) {
         user abort "Please don't specify smooth and histograms"
      }
      if(!$?nsmooth) {
         local define nsmooth 20        # how many fields to smooth other
      }
   }

   if('$coord' == 'field') {
      local set objc_field = field[0] + objc_rowc/1361
   }

   define b local  define c local  define v local
   set dmag local  set sdmag local  set x local  set l local

   if($hist || $smooth) {
      local set camCols = uniq(sort(atof(id)))
      local set cts = {white red yellow green cyan blue magenta white}
      set camCol local  set tmp local  set tmpx local
      set nsmooth local
   }

   local define lim (('$m1' == 'psf' || '$m2' == 'psf') ? 0.12 : 0.25)
   if($hist) {
      local set hx = $lim*do(-1,1,0.01)
      set hy local
   }

   define xla local  define yla local
   set_window
   do b=$b0, $b1 {
      set_window 1 -$($b1 - $b0 + 1)

      photoL $b

      if($color) {
         set dmag = ($m1""Mag$($b-1) - $m2""Mag$($b-1)) - \
          ($m1""Mag$b - $m2""Mag$b)
      } else {
         set dmag = ($m1""Mag$b - $m2""Mag$b)
      }
      set x=objc_$coord
      set camCol = atof(id)

      set l = abs(dmag) < 0.5 && psp_status==0 && good && !blended && $l

      foreach v (dmag x camCol) { set $v = $v if(l) }

      if($hist) {
         set hy = histogram(dmag:hx)
         lim hx hy
         bo
         histogram hx hy

         do c=0, dimen(camCols) - 1 {
            set tmp = dmag if(camCol == camCols[$c])
            if(dimen(tmp) > 0) {
               set hy = histogram(tmp:hx)

               ctype $(cts[camCols[$c]])
               histogram hx hy
               ct 0
            }
         }
      } else {
         lim x ($lim*{-1 1})
         bo
         poi x dmag
      }

      if(sum(l) > 0 && $smooth) {
         sort { x dmag camCol }

         do c=0, dimen(camCols) - 1 {
            set tmpx = x set tmp = dmag
            set l = camCol == camCols[$c]

            foreach v (tmpx tmp) { set $v = $v if(l) }
            if(dimen(tmp) > 0) {
               set nsmooth = sort(field)
               set nsmooth = int($nsmooth*dimen(tmp)/\
                   (nsmooth[dimen(nsmooth)-1] - nsmooth[0] + 1))

               set sdmag = vsmooth(tmp, 3*nsmooth)
               set l = abs(tmp - sdmag) < 0.2
               set tmpx = tmpx if(l)  set tmp = tmp if(l) # clip

               if(0) {
                  set sdmag = vsmooth(tmp, 3*nsmooth)
                  set l = abs(tmp - sdmag) < 0.1
                  set tmpx = tmpx if(l)  set tmp = tmp if(l) # clip
               }

               ctype $(cts[camCols[$c]])
               connect tmpx (vsmooth(tmp, nsmooth))
               ct 0
            }
         }
      }

      if($color) {
         define yla <\2($(filter_name($b-1)) - $(filter_name($b)))_{($m1 - $m2)}>
      } else {
         define yla <\2$(filter_name($b)) ($m1 - $m2)>
      }
      if($hist) {
         define xla <($m1 - $m2)>
         define yla <N($(filter_name($b)))>
      } else {
         define xla $coord
      }

      if($b == 0) {
         xla $xla
      }
      yla $yla
   }

   toplabel Run $run Fields $frames
   set_window
}

###############################################################################

macro re_field 23 {
   # Usage: re_field band maglim [type]
   #
   # Plot re against field number for all 6 chips
   #
   local define b $1
   local set maglim = $2
   overload exp 1
   if($?3) {
      define type $3
   } else {
      define type deV
   }
   if('$type' == 'fmodel') {
      local set r_fmodel$b = fracPSF$b*r_deV$b + (1 - fracPSF$b)*r_exp$b
   }

   local set ffield = field[0] + objc_rowc/1361
   #set ffield = objc_colc
   photoL $b   
   lim ffield -2.5 2
   local set camCols = uniq(sort(atof(id)))  local define ncol (dimen(camCols))
   define c local
   do c=camCols[0],camCols[$ncol - 1] {
      #define c ($c + 394)
      set_window 1 -$ncol
      box
      ctype green
      poi ffield (lg(0.400*r_$type""$b)) if(good && (objc_type == 'star') && psfMag$b<maglim && atof(id) == $c)
      ctype magenta
      poi ffield (lg(0.400*r_$type""$b)) if(good && (objc_type == 'galaxy') && psfMag$b<maglim && atof(id) == $c)
      ct 0
      #poi ffield (0.400*r_$type""$b) if(good && objc_type == 'star' && psfMag$b<maglim && field == $c)
   }
   set_window

   xla Field
   yla \1lg({\1r_e}/arcsec)_{$type}
}

macro re_frequency 35 {
   # Usage: re_frequency band maglimL maglimU [objc_type] [re_type]
   #
   # Plot histograms of re for all 6 chips
   #
   local define b $1
   local set maglimL = $2
   local set maglimU = $3
   overload exp 1

   if($?4) {
      local define objc_type $4
   } else {
      local define objc_type all
   }

   if($?5) {
      define type $5
   } else {
      define type deV
   }
   if('$type' == 'fmodel') {
      local set r_fmodel$b = fracPSF$b*r_deV$b + (1 - fracPSF$b)*r_exp$b
   }

   photoL $b  
   local set x=do(-1.5,-0.5001,0.10) concat do(-0.5,2,0.05)

   define t local
   if('$objc_type' == 'all') {
      local set types = {star galaxy}
   } else {
      local set types == <$objc_type>
   }

   local set l = good && modelMag$b >= maglimL && modelMag$b < maglimU
   local set lre_all = lg(0.400*r_$type""$b) if(l)
   local set id = id if(l)
   local set objc_type = objc_type if(l)
   set lre local

   define c local
   local set camCols = uniq(sort(atof(id)))  local define ncol (dimen(camCols))
   do c=camCols[0],camCols[$ncol - 1] {
      if($ncol >= 5) {
         set_window -2 -3
      } else { if($ncol >= 3) {
         set_window -2 -2
      } else { if($ncol >= 2) {
         set_window -2 -1
      } else {
         set_window 1 1
      }}}

      set l = (atof(id) == $c) ? 1 : 0

      set lre = lre_all if(l)
      set h = histogram(lre : x)
      if(0) {
         lim x h
      } else {
         if($c == 1) {
            set i=0,dimen(x)-1 set hh = -h sort { hh i } 
            set hh = h set hh[i[0]] = 0
            lim x hh
            
            lim 0 0 $fy1 $(1.2*$fy2)
         }
      }
      ti -1 0 0 0
      bo
      ti 0 0 0 0

      if('$objc_type' == 'all') {
         hi x h if(h > -100)
      }

      foreach t types {
         set lre = lre_all if(l && objc_type == '$t')
      
         set h = histogram(lre : x)
         #set h = (h > 0) ? lg(h) : -1000 lim 0 0 0 3
         ctype_from_type $t
         hi x h if(h > -100)
         ct 0
         
         rel $($fx1 + 0.8*($fx2 - $fx1))  $($fy1 + 0.9*($fy2 - $fy1))
         putl 5 C$c
      }
   }
   set_window
   xla \1({\1r_e}/arcsec)_{$type}\n yla N
   toplabel Frames $frames  $(filter_name($b)) ($(maglimL) < Mag_{model} < $(maglimU)) $objc_type
}

#
# Plot a g-r CM diagram from g band data only
#
macro plot_mono_CM {
   local set l = good && type == 'star'
   local set x = psfMag1 - aperMag1 if(l)
   local set y = psfMag1 if(l)

   lim -0.05 0.05 20 13
   box
   poi x y
}

macro plot_aper_color {
   photoL 1

   local define l "expMag1 < 20"
   local set l = good && type == 'star' && $l
   local set x = expMag1 - expMag2 if(l)
   local set y = psfMag1 - expMag1 if(l)

   lim 0 1.6 -0.05 0.05
   box
   poi x y

   xla g-r
   yla g_{psf - exp}
   toplabel $l
}

macro colour_field 47 {
   # Plot colour $1Mag$3 - $2Mag$4 v. (floating) field number, only
   # plotting points for which $5 is true (if provided)
   # If $6&0x1,   plot errors
   #    $6&0x2,   connect points
   #    $6&0x4,   subtract median of $1Mag$3 - $2Mag$4 before plotting
   #    $6&0x8,   plot PSF sigma in separate panel at top
   #    $6&0x10,  plot field boundaries
   #    $6&0x20,  plot v. column not row
   #    $6&0x40,  overplot PSP sky level
   #    $6&0x80,  Show (object - KL) sigma
   #    $6&0x100, Show PSP aperture corrections
   # If $7, smooth with a boxcar filter of $7 points
   local define t1 $1  local define t2 $2
   local define b1 $3  local define b2 $4

   photoL $b1

   local set field0 = field[0]
   local set field = (field0 + objc_rowc/1361) # floating field
   
   if($?5) {
      local set l = good&&!blended&&$5
   } else {
      local set l = good&&!blended
   }

   if(!$?6) {
      define 6 0
   }
   local set flags = $6
   if(!$?7) {
      define 7 0
   }
   local set filtsize = int($7)
   if(filtsize%2 == 0) { set filtsize = filtsize + 1 }

   if(flags & 0x20) {
      local set camCol = atof(id)
      set flags = flags & (0xfffff - 0x10)
      local set gutter = 800
      local set x = (objc_colc + (2048 + gutter)*(camCol - 1)) if(l)
      set camCol = camCol if(l)
   } else {
      local set x = field if(l)
   }
   if($b1 == $b2 || '$t1' == '$t2') {
      local set y = ($t1""Mag$b1 - $t2""Mag$b2) if(l)
   } else {
      local set y = ($t1""Mag$b1 - $t1""Mag$b2) - ($t2""Mag$b1 - $t2""Mag$b2) if(l)
   }

   define med local  define siqr local
   if(flags & 0x4) {
      stats_med y med siqr
      echo $(filter_name($b1)): $med +- $(0.741*2*$siqr)
      set y = y - $med
   }
   #
   # which-window variables (set by window)
   #
   if(!$?_ix) {
      local define _ix 1
   } 
   if(!$?_iy) {
      local define _iy 1
   }
   
   define i local  define j local
   set dy local  set dimen(dy) = 0
   if(flags & 0x1) {
      if(is_vector($t1""MagErr$b1)) {
         if(is_vector($t2""MagErr$b2)) {
            set dy = sqrt($t1""MagErr$b1**2 + $t2""MagErr$b2**2) if(l)
         } else {
            set dy = $t1""MagErr$b1 if(l)
         }
      } else {
         if(is_vector($t2""MagErr$b2)) {
            set dy = $t2""MagErr$b2 if(l)
         }
      }
   }
   
   lim x -0.1 0.1
   lim x -0.2 0.2
   lim x -0.1 0.1
   if('$t1' == 'fiber' || '$t2' == 'fiber') {
      lim 0 0 -0.5 0.2
   }
   #lim x -0.3 0.1

   if(flags & 0x8) {
      window 1 -20 1 1:15
      local set sigma = sqrt(M_rr_cc$b1/2) if(l)
      local set sigma_psf = sqrt(M_rr_cc_psf$b1/2) if(l)
   }

   if(flags & 0x40) {                   # plot sky
      box 1 2 0 3
   } else {
      box
   }
   ct red rel $fx1 0 draw $fx2 0 ct 0
   #
   # Which vectors are we using?
   #
   local define vecs < x y >
   if(dimen(dy) > 0) {
      define vecs <$vecs dy>
   }
   if(flags & 0x8) {
      define vecs <$vecs sigma sigma_psf>
   }
   
   if(filtsize > 1) {
      local define svecs " "
      foreach i <$vecs> {
         if('$i' != 'x') {
            define svecs <$svecs $i>
         }
      }
      sort < x $svecs >
      set tmp local  set itmp local
      foreach i <$svecs> {
         set itmp = $i
         set dimen($i) = 0
         do j=1,6 {
            set tmp = itmp if(camCol == $j)
            set $i = $i concat vsmooth(tmp, $(filtsize))
         }
      }
   }

   if(flags & 0x2) {
      define conn_poi "connect "
      sort < $vecs >
   } else {
      define conn_poi "points "
   }
   #
   # Main panel
   #
   $conn_poi x y
   
   if(dimen(dy) > 0) {
      error_y x y dy
   }

   if(flags & 0x100) {                  # PSP aperture corrections
      ct green
      connect PSP_field ap_corr$b1
      error_y PSP_field ap_corr$b1 ap_corrErr$b1
      ct 0
   }

   if(substr('$_iy',0,1) == '1') {
      if(flags & 0x20) {
         xla Column
      } else {
         xla Field
      }
   }
   if(substr('$_ix',0,1) == '1') {
      if($b1 == $b2) {
         yla ($t1 - $t2)_{$(filter_name($b1))}
      } else { if('$t1' == '$t2') {
         yla ($(filter_name($b1)) - $(filter_name($b2)))_{$t1}
      } else {
         yla ($(filter_name($b1)) - $(filter_name($b2)))($t1 - $t2)
         #yla $t1_{$(filter_name($b1))} - $t2_{$(filter_name($b2))}
      }}
   }
   #
   # Supplementary seeing panel if requested
   #
   if(flags & 0x8) {
      window 1 -20 1 16:20
      
      if(flags & 0x80) {
         stats_med sigma med siqr

         set dy = (sigma - sigma_psf)
         lim 0 0 dy
         #lim 0 0 (0.1*{-1 1})
         box
         $conn_poi x dy
         yla \sigma - \sigma_{KL}
      } else {
         pt 4 0
         foreach i (sigma sigma_psf) {
            if('$i' == 'sigma') {
               stats_med $i med siqr
               
               lim 0 0 ($med concat $med - 6*$siqr concat $med + 6*$siqr)
               box
               #rel $($fx1+0.1*($fx2-$fx1)) $($fy1+1.1*($fy2-$fy1))
               #putl 6 \-1 PSF (\point41: KL, \point40 Object)
               yla \sigma_{PSF}
            } else {
               ct cyan
               if(flags & 0x2) {
                  lt 1
               } else {
                  pt 4 1
               }
            }
            
            $conn_poi x $i
         }
      }

      lt 0  pt 4 1  ct 0
   }


   if(flags & 0x40) {                   # plot sky
      if(flags & 0x8) {
         window 1 -20 1 1:15
      }
      range 0 10
      lim 0 0 PSP_sky$b1
      range 0 0
      
      BOX 3 3 3 2
      ct green
      connect PSP_field PSP_sky$b1  
      ct 0
   }

   window 1 1 1 1
   #
   # Frame boundaries?
   #
   if(flags & 0x10) {
      lt 1
      do i=int($fx1), int($fx2) + 1 {
         rel $i $fy1  draw $i $fy2
      }
      lt 0
   }

   toplabel Run $(sprintf('%d', $run))($rerun) Camcol:Fields $frames
}

###############################################################################

macro pr2771 23 {
   #
   # Make plots relevant to PR2771 (exp - psf varies with g-r colour of stars)
   # Usage: pr2771 band logical [flags]
   # Plot band $1 (e.g. 1) [and maybe band $1 + 1 for colours]
   # Only consider points for which $2 is true
   # flags:
   # If flags&0x1,     plot stars
   #    flags&0x2,     plot galaxies
   #    flags&0x4,     x = psf_{band} - psf_{band+1}
   #             else  x = exp_{band} - psf_{band}
   #    flags&0x8,     y = psf_{band} - psf_{band+1}
   #             else  y = sigma_{object} - sigma_{KL psf}
   #    flags&0x10,    Plot band+1's PSF width
   #    flags&0x20,    Correct (exp - psf) using (sigma - sigma_KL)
   #    flags&0x40,    Use aperture, not psf, magnitudes
   #
   # E.g. compare plots:
   #   pr2771 $b good&&modelMag$b<19 0x1|0x8
   #   pr2771 $b good&&modelMag$b<19 0x1|0x8|0x20
   #
   local define b1 $1
   photoL $b1
   local set l = $2
   if(!$?3) {
      define 3 0x0
   }

   local define stars         (($3) &  0x1 ? 1 : 0)
   local define galaxies      (($3) &  0x2 ? 1 : 0)
   local define x_color       (($3) &  0x4 ? 1 : 0)
   local define y_color       (($3) &  0x8 ? 1 : 0)
   local define bandp1        (($3) & 0x10 ? 1 : 0)
   local define correct       (($3) & 0x20 ? 1 : 0)
   local define aperture      (($3) & 0x40 ? 1 : 0)

   if($aperture) {
      local define mag "aper"
   } else {
      local define mag "psf"
   }
   
   local define types < >
   if($stars) {
      define types <$types star>
   }
   if($galaxies) {
      define types <$types galaxy>
   }

   if(!$stars && !$galaxies) {
      set l = l && type$b1 == 'star'
      if('$types' == ' ') {
         define types "star"
      }
   }

   lim -.2 .2 -0.25 0.25

   if($x_color) {
      local set x = $mag""Mag$b1 - $mag""Mag$($b1 + 1)
      local define xla <($(filter_name($b1)) - $(filter_name($b1+1)))_{$mag}>

      lim ({-0.5 2.0}) 0 0
   } else {
      local set x = expMag$b1 - $mag""Mag$b1
      local define xla <(exp - $mag)_{$(filter_name($b1))}>
   }
   set x = x if(l)
   
   if($y_color) {
      local set y = $mag""Mag$b1 - $mag""Mag$($b1 + 1)
      local define yla <($(filter_name($b1)) - $(filter_name($b1+1)))_{$mag}>

      lim 0 0 ({-0.5 2.0})
   } else {
      if($bandp1) {
         local define bp ( $b1 + 1 )
      } else {
         local define bp $b1
      }
      
      if(0) {
         local set y = sqrt(M_rr_cc_psf$bp/2) - sqrt(M_rr_cc$bp/2)
         local define yla <(\sigma_{KL} - \sigma_{\1\point52})_{\2$(filter_name($bp))}>
      } else {
         local set y = (M_rr_cc_psf$bp/2 - M_rr_cc$bp/2)/2
         local define yla <{\-2\frac{1}{2}}(\sigma_{KL}^2 - \sigma_{\1\point52}^2)_{\2$(filter_name($bp))}>
         #set y = (y < 0) ? -sqrt(-y) : sqrt(y)
         #lim 0 0 (0.3*{-1 1})
      }
   }
   set y = y if(l)

   local set type = type$b1 if(l)

   if($correct) {                  # fix chromatic seeing variation
      if($x_color) {
         user abort "flag bit 0x20 only makes sense if 0x4 isn't set"
      }
      local set dsigma = sqrt(M_rr_cc$b1/2) - sqrt(M_rr_cc_psf$b1/2) if(l)
      set x = x - (-0.004083344713 + -0.6810624003*dsigma)
   }

   box

   define t local
   foreach t ($types) {
      ctype_from_type $t
      if('$t' == 'star') {
         ptype 6 1
      } else {
         ptype 4 0
      }

      poi x y if(type == '$t')
   }
   ct 0  pt 4 1

   if(!($galaxies || $x_color || $y_color)) { # no galaxies; exp-$mag v. dsigma
      define a local  define b local
      
      local set clip = (x < $fx1 || x > $fx2 || y < $fy1 || y > $fy2)
      local set xx = x if(!clip)
      local set yy = y if(!clip)

      lsq2 yy xx 1000 \n                  # sigma_y^2 = 1000*sigma_x^2
      if(0) {
         ct red
         con ($b + $a*<$fy1 $fy2>) (<$fy1 $fy2>)

         lsq2 yy xx 1e-10
         ct cyan
         con ($b + $a*<$fy1 $fy2>) (<$fy1 $fy2>)

         lsq2 yy xx 1e5
         ct blue
         con ($b + $a*<$fy1 $fy2>) (<$fy1 $fy2>)
         ct 0
      }
      #
      # Clip that fit
      #
      if(0) {
         set clip = (abs($b + $a*xx - yy) > 0.15) ? 1 : 0
         set xx = xx if(!clip)
         set yy = yy if(!clip)
         lsq yy xx
      }
      
      con ($b + $a*<$fy1 $fy2>) (<$fy1 $fy2>)
      toplabel $yla = $b + $a*$xla
      echo x = $b + $a*y
   }

   xla $xla
   yla $yla

   lt 1  ct cyan
   rel $fx1 0 draw $fx2 $uyp
   rel 0 $fy1 draw $uxp $fy2
   lt 0  ct 0
}

###############################################################################
#
# Plot deltaMag as a function of image quality for good stars
#
macro deltaMag_seeing 34 {
   # Usage: deltaMag_seeing band type1 type2 [logical]
   # e.g. define b 3 photoL $b  deltaMag_seeing $b psf fiber psfMag$b<21
   overload exp 1
   local define band $1
   local define t1 $2
   local define t2 $3
   if($?4) {
      local define l "$!4"
      local set l = $4
   } else {
      local define l ""
      local set l = 1
   }

   local set sigma = sqrt(M_rr_cc_psf$band/2)
   set l = l & good && !blended && objc_type=='star' && sigma > 0

   set sigma = sigma if(l)
   set y = $t1""Mag$band - $t2""Mag$band if(l)
   
   range 0 0.5
   lim sigma y
   range 0 0
   box
   if(0) {
      poi sigma y
   } else {
      local set camCol = atof(id) if(l)
      local set ctypes = {default red blue green cyan magenta yellow}
      local define i 0
      do i=1,6 {
         ctype $(ctypes[$i])
         poi sigma y if(camCol == $i)
         ct 0
      }
   }

   xla ((\sigma_{rr}^2 + \sigma_{cc}^2)/2)^{1/2}
   yla ($t1 - $t2)_{$(filter_name($band))}
   local define top "Frames $!frames  $!(dimen(y)) objects"
   if($?l) {
      define top "$!top  ($!l)"
   }
   toplabel $top
}

###############################################################################
#
# Return an object's full ID
#
macro full_id 0 {
   set $0 = '$($run)-' + sprintf('%-15s', id) + \
    sprintf(' %15s ', '(' + sprintf('%.1f',objc_rowc-1361*(field-field[0])) + \
    ',' + sprintf('%.1f',objc_colc) + ')')
}

###############################################################################
#
# Plot e1/e2
#
macro amoment 12 {
   # Usage: amoment band [logical]
   photoL $1  local define band $1
   if($?2) {
      local define l "$!2"
      local set l = $2
   } else {
      local define l ""
      local set l = 1
   }

   local set flags = 0x1 | 0x2 | 0x4

   if(flags & 0x1) {
      set x = objc_rowc if(l) set dimen(xErr) = 0
      local define xla "rowc"
   } else {
      local set x = M_e1$band if(l)
      local set xErr = M_e1e1Err$band if(l)
      local define xla "E_1"
   }
   local set y = M_e2$band if(l)
   local set yErr = M_e2e2Err$band if(l)
   local define yla "E_2"

   if(flags & 0x2) {
      local set smoothed = vsmooth(y, 10)  set y = y - smoothed
      define yla "$!yla - <$!yla>"
      if(flags & 0x4) {
         set y = y/yErr
         define yla "($!yla)/\sigma_{$!yla}"

         local set foo = y if(abs(y) < 10)
         echo <chi^2> = $(sum(foo**2)/dimen(foo))
      }
   }


   lim x y

   if(flags & 0x4) {
      lim 0 0 ({-10 10})
   }

   box
   poi x y
   if(dimen(xErr) > 0) {
      error_x x y xErr
   }
   if(dimen(yErr) > 0) {
      error_y x y yErr
   }

   xla $xla
   yla $yla
}

###############################################################################

macro frelocate 22 {
   # Usage: frelocate frac-x frac-y
   # Like relocate, but positions are in units of the current window. E.g.
   #   frelocate 0.1 0.9  putlabel 5 Top Left Corner
   RELOCATE $($fx1 + $1*($fx2 - $fx1))  $($fy1 + $2*($fy2 - $fy1))
}

macro fputlabel 33 {
   # Usage: fputlabel frac-x  frac-y  putlabel-n  label
   # Like putlabel, but first position within the current window. E.g.
   #   fputlabel 0.1 0.9 5 Top Left Corner
   frelocate $1 $2
   PUTLABEL $3}

###############################################################################

macro histogram_velocity 12 {
   # Usage: histogram_velocity x-vec [logical]
   # E.g.
   #  histogram_velocity do(-2,2,0.025) good&&!blended&&objc_type=='star'
   local set x=$1
   if(!$?2) {
      local set l = 1 + 0*x
   } else {
      local set l=$2
   }
   set h local
   define m local define s local define k local

   define v local  
   foreach v {row col} {
      set_window 1 -2
      set h=$v""v/$v""vErr if(l)
      stats_med h m s  define s (0.741*2*$s)
      set h=histogram(h:x) lim x h bo
      hi x h
      con x (gauss(x,$m,$s)*sum(h)*(x[1]-x[0]))
      fputlabel 0.1 0.9 6 $(sprintf('%.3f', float($m))) \pm{} $(sprintf('%.3f', float($s)))
      ylabel N($v)
   }
   set_window
   xla \chi
}

###############################################################################
#
# Read Zeljko's dump_match outputs
#
macro read_dump_match 11 {
   # Usage: read_dump_match dumpfile
   #
   # Read the output of dump_match, generating vectors with names such as
   # petroCounts3_1 and petroCounts3_2 for the i band matches.
   DATA "$!1"
   define vec local 
   define j local

   local define i 3
   while {1} {
      define i ($i + 1)
      define vec read $i 1

      if(!$?vec || $i > 15) {
         local define vline -1
         break
      }
      if('$vec' == 'objc_type') {
         local define vline ( $i + 1 )
         break
      }
   }

   if($vline < 0) {
      user abort "Cannot find list of vectors"
   }

   set vecs local set dimen(vecs) = 0.s
   define i 0
   while {1} {
      define i ($i + 1)
      define vec read $vline $($i)

      if(!$?vec || $i > 100) {
         break
      }

      define j (index('$vec', '<'))
      if($j > 0) {
         define b ( index('ugriz', substr('$vec', $j+1, 1)) )
         define vec ( substr('$vec', 0, $j) + '$b')
      }
      set vecs = vecs concat '$vec'
   }

   do j=0,5 {
      local define vecs$j < >
   }

   do i=0,dimen(vecs)-1 {
      define j ( int($i/25) )
      set v$i local
      define vecs$j <$vecs$j v1$i v2$i>
   }

   lin $($vline+1) 0
   #lin $($vline+1) 1000
   read '' < $vecs0 $vecs1 $vecs2 $vecs3 $vecs4 $vecs5>

   do i=0,dimen(vecs)-1 {
      set $(vecs[$i])_1 = v1$i
      set $(vecs[$i])_2 = v2$i
   }
}

macro compare 46 {
   # Usage: compare mag what band flags [logical] [ylim]
   # Compare the measurements for <what> in <band> for two runs, as read
   # using read_dump_match. The x-axis is <mag>
   #
   # If logical is provided, only use those points
   #
   set foo local
   define i local

   local define x $1  local define v $2  local define b $3
   local define good_errors (($4) &  0x1 ? 1 : 0)
   local define chi         (($4) &  0x2 ? 1 : 0)
   local define fractional  (($4) &  0x4 ? 1 : 0)
   local define xlog        (($4) &  0x8 ? 1 : 0)
   local define hist        (($4) & 0x10 ? 1 : 0)
   local define gauss       (($4) & 0x20 ? 1 : 0)
   local set x = ($x""$b""_1 + $x""$b""_2)/2 # may be used in l
   if($?5) {
      local set l = $5
   } else {
      local set l = 1
   }
   if(dimen(l) == 1) {
      set l = l + 0*$x""$b""_1
   }
   
   if($?6) {
      local define ylim $6
   } else {
      local define ylim 0
   }
   #
   # Handle special case of concentration parameter, C
   #
   if('$v' == 'petroC') {
      do i=1,2 {
         local set $v""$b""_$i = petroR90$b""_$i/petroR50$b""_$i
         local set covar_$i = 1*petroR50Err$b""_$i*petroR90Err$b""_$i
         local set $v""Err$b""_$i = \
             (petroR90Err$b""_$i/petroR50$b""_$i)**2 - \
             2*covar_$i*petroR90$b""_$i/petroR50$b""_$i**3 + \
             (petroR90$b""_$i*petroR50Err$b""_$i/petroR50$b""_$i**2)**2
         set $v""Err$b""_$i = $v""Err$b""_$i < 0 || \
                 petroR50Err$b""_$i < 0 || petroR90Err$b""_$i < 0 \
                 ? -1000 : sqrt($v""Err$b""_$i)
      }
   }

   if($chi) {
      if(!$good_errors) {
         echo Defining good_errors (0x1) for chi (0x2)
         define good_errors 1
      }
   }

   if($good_errors) {
      set l = l && ($v""Err$b""_1 >= 0 && $v""Err$b""_2 >= 0)
   }
   if($xlog) {
      set l = l && ($x""$b""_1 > 0)
   }

   set x = x if(l)
   if($xlog) {
      set x = lg(x)
   }
   
   define med local   define sig local
   if($chi) {
      if($fractional) {
         echo "Ignoring fractional as chi is set"
      }
      local set y = ($v""$b""_1 - $v""$b""_2)
      set foo = y if(l) sort { foo }
      stats_med foo med sig
      set y = (y - $med)/sqrt($v""Err$b""_1**2 + $v""Err$b""_2**2)
   } else { if($fractional) {
      local set y = ($v""$b""_1 - $v""$b""_2)/(($v""$b""_1 + $v""$b""_2)/2)
   } else {
      local set y = ($v""$b""_1 - $v""$b""_2)
   }}
   set y = y if(l)

   if($chi || $hist) {
      stats_med y med sig
      define sig ( 0.741*2*$sig )
   }

   if($hist) {
      if($ylim != 0) {
         local define min ( -$ylim )
         local define max ( $ylim )
         set x = $ylim*(2*do(0,1,1/50) - 1)
      } else {
         local set hfoo = y sort { hfoo }
         local define min ( hfoo[int(0.01*dimen(hfoo))] )
         local define max ( hfoo[int(0.99*dimen(hfoo))] )
      }
      set y = histogram(y:x)
   }

   if($hist) {
      lim x (1.15*y)
   } else {
      if(index('$x', 'Counts') != -1 || index('$v', 'Mag') == -1) {
         lim x 0 0
      } else {
         lim $(14<$fx1?$fx1:14) $(23>$fx2?$fx2:23) 0 0
      }
   }

   if($ylim != 0) {
      if($hist) {
         lim (<-$ylim $ylim>) 0 0
      } else {
         lim 0 0 $(-$ylim) $($ylim)
      }
   } else { 
      if(!$hist) {if($chi) {
         lim 0 0 (1.5*{-1 1})
      } else {
         lim 0 0 y
      }}
   }

   bo
   
   if($hist) {
      hi x y
      if($gauss) {
         ct cyan lt 2
         con x (sum(y)*(x[1]-x[0])*gauss(x, 0, $sig))
         ct 0 lt 0
      }
   } else {
      poi x y
   }

   fputlabel 0.8 0.9 6 $(filter_name($b))

   define xla ( quote_TeX('$x') )
   if($xlog) {
      define xla <lg($xla)>
   }
   
   if($chi) {
      local define yla <\chi_{\1$(quote_TeX('$v'))}>
      if($hist) {
         fputlabel 0.1 0.9 6 $(sprintf('%.3f', float($med))) \pm{} $(sprintf('%.2f', float($sig)))
      }
   } else { if($fractional) {
      local define yla <\Delta $(quote_TeX('$v/$v'))>
   } else {
      local define yla <\Delta $(quote_TeX('$v'))>
   }}

   if($hist) {
      define xla <$yla>
      define yla N
   }
   #
   # Actually label axes
   #
   if(!$?_iy) { 
      local define _iy 1
   }
   if($_iy == 1) {
      xla $xla
   }
   if(!$?_ix) {
      local define _ix 1
   }
   if($_ix == 1) {
      yla $yla
   }

   if(0) {
      set foo = sprintf('%g-', camCol_1) + sprintf('%g:', field_1) + sprintf('%.1f,', rowc2_1) + sprintf('%.1f', colc2_1) if(l)
      if($hist) {
         if(0) {
            set $v""Err$b""_1 = $v""Err$b""_1 if(l)
            set foo = foo if($v""Err$b""_1 > 0)
         } else {
            set foo = $v""Err$b""_1 if(l)
         }
      } else {
         set foo = foo if(y <1e-2 && x > 20)
      }
      if(dimen(foo) > 0) {
         calc foo
      }
   }
   #
}

macro compareAll 35 {
   if(!$?4) { define 4 1 }
   if(!$?5) { define 5 0 }
   define i local
   set good local

   local set OBJ_FLAGS = 0
   foreach i (BRIGHT SATUR BLENDED) {
      set OBJ_FLAGS = OBJ_FLAGS | (1<<$OBJ1_$i)
   }
   if(index('$2', 'petro') >= 0) {
      foreach i (NOPETRO PETROFAINT NOPETRO_BIG) {
         set OBJ_FLAGS = OBJ_FLAGS | (1<<$OBJ1_$i)
      }
   }

   do i=0,4 {
      set good = !((flags$i""_1|flags$i""_2) & OBJ_FLAGS)
      set_window -2 -3
      compare $1 $2 $i $3 $4&&good $5
   }

   set_window -2 -3
   fputlabel 0.5 0.5 5 good && $(quote_TeX('$4'))

   set_window
}

###############################################################################
#
# Show centroids from psFang and fpObjc files
#
macro show_centroids 01 {
   if(!$?1) { define 1 0 }
   define show_matched ($1)
   
   lim 0 2037 0 1360
   #lim 0 1000 0 1000
   box

   define i local
   do i=0,4 {
      ctype_band $i
      photoL $i
      
      expand 5

      poi ps_colc$i ps_rowc$i if(astromGood$i)
      if($show_matched) {
         pt 4 0
         poi frames_colc$i frames_rowc$i
      } else {
         pt 1 1
         poi colc$i rowc$i if(good)
      }
      pt 4 1  expand 1   ct 0
   } 
}

#
# Match centroids from psFang and fpObjc files
#
macro match_centroids 12 {
   # Usage: match_centroids band [max-distance]
   local define b $1
   if(!$?2) {
      define 2 1                        # 1 pixel
   }
   define max ( $2**2 )                 # delta == distance**2
   
   photoL $b
   local set l = good && !blended && !(objc_flags & (1<<$OBJ1_CHILD))
   local set I = do(0,dimen(good)-1) if(l)
   local set R = rowc$b if(l)
   local set C = colc$b if(l)

   set delta local  set J local
   set match = 0*ps_rowc$b              # index of matched stars in e.g. rowc2
   define i local
   do i=0,dimen(ps_rowc$b)-1 {
      set delta = (ps_rowc$b[$i] - R)**2 + (ps_colc$b[$i] - C)**2
      set J = I
      sort { delta J }
      set match[$i] = delta[0] <= $max ? J[0] : -J[0]
   }

   set frames_rowc$b = (match >= 0 && astromGood$b) ? rowc$b[abs(match)] : -1
   set frames_colc$b = (match >= 0 && astromGood$b) ? colc$b[abs(match)] : -1
   set frames_mag$b = (match >= 0 && astromGood$b) ? psfMag$b[abs(match)] : -1
   set match_id$b = (match >= 0 && astromGood$b) ? id[abs(match)] : '-1'
   set match_id$b = string(match_id$b) + ':' + ps_id$b
}

macro show_match 11 {
   # Usage: show_match <band>
   local define b $1
   
   local set r = frames_rowc$b
   local set c = frames_colc$b
   local set dr = 1000*(frames_rowc$b - ps_rowc$b)
   local set dc = 1000*(frames_colc$b - ps_colc$b)
   local set mid = match_id$b
   local set pid = int(atof(substr(mid, index(mid, '.') + 1, 0)))
   set pid = pid < 0 ? 0 : pid
   local set l = atof(mid) > 0
   define v local
   foreach v (mid r c dr dc pid) { set $v = $v if(l) }
   
   local set objc_type = objc_type
   set objc_type = objc_type[pid]

   print  '%-25s %6.1f %6.1f %9.4f %9.4f %-s\n' \
       { mid r c dr dc objc_type }
}

##
## E.g.
## define run 745   define rerun 668  define c 1  define f 460
## read_psFang $root/testbed   $run:$rerun $c $f
## Lconcat $root/testbed       $run:$rerun $c $c $f $f 0 4
## 
## do i=0,4 { match_centroids $i }
## 
## #ERASE show_centroids 1
## ERASE centroid_psp_frames col -1 0x0
##
macro centroid_psp_frames 23 {
   #
   # Plot psp v. frames centroids
   #
   # Usage:  centroid_psp_frames row|col|both band [flags]
   # if "both", plot row and column
   # If band < 0, plot all bands
   overload row 1

   local define rc $1
   local define b $2
   if(!$?3) { define 3 0 }
   define flags ($3)
   local define id   (($flags) & 0x1)
   local define mag  (!(($flags) & 0x2))
   #
   # Handle set_windows commands
   #
   if(!$?_nx) {
      if('$rc' == 'both') {
         local define _nx 2
      } else {
         local define _nx 1
      }
   }
   if(!$?_ny) {
      if($b < 0) {
         local define _ny 5
      } else {
         local define _ny 1
      }
   }
   if(!$?_ix) { local define _ix 1 }
   if(!$?_iy) { local define _iy 1 }

   define i local

   if($b < 0) {
      do i=0,4 {
         set_window -$(abs($_nx)) -5
         centroid_psp_frames $rc $i $flags
         frelocate 0.1 0.8  putl 5 $(filter_name($i))
      }
      if($_ix == abs($_nx) && $_iy == abs($_ny)) {
         set_window 1 1
      }
      yla $rc, psFang - Frames (mpixel)
      
      return
   }

   if('$rc' == 'both') {
      foreach i (row col) {
         if('$i' == 'col' || $_ny == 1) {
            set_window -2 -$(abs($_ny))
         }
         centroid_psp_frames $i $b $flags
         frelocate 0.1 0.8  putl 6 $i, $(filter_name($b))
      }

      if($_ix == abs($_nx) && $_iy == abs($_ny)) {
         set_window 1 1
      }
      
      return
   }

   if($mag) {
      local set x = frames_mag$b

      lim 14 21 0 0
   } else {
      lim 0 2048 0 0
      local set x = frames_colc$b
   }

   lim 0 0 (22*{-1 1}) 
   box
   if($id) {
      set pt = string(match_id$b) if(frames_$rc""c$b >= 0) 
      pt pt
   }
   poi x (1e3*(ps_$rc""c$b - frames_$rc""c$b)) \
       if(frames_$rc""c$b >= 0)

   if(0) {                              # save differences as foo_rcb?
      local set goo = (1e3*(ps_$rc""c$b - frames_$rc""c$b)) if(frames_$rc""c$b >= 0 && frames_mag$b > 18 && frames_mag$b < 18.5)
      if(!is_vector(foo_$rc""$b)) { set dimen(foo_$rc""$b) = 0 }
      set foo_$rc""$b = foo_$rc""$b concat goo
   }
   pt 4 1

   if($_iy == 1) {
      if($mag) {
         xla Magnitude
      } else {
         xla colc
      }
   }
   if($_nx == 1) {
      yla $rc, psFang - Frames (mpixel)
   }
}

###############################################################################

macro good_trails 105 {
   # Usage: good_trails [band [band ...]]
   # Set `good' including saturated stars with saturation-trail photometry;
   # check only <bands> (default: gri).  <bands> can be "reset"
   #
   define i local

   if(!$?1) {
      local define bands "1 2 3"
   } else {
      local define bands "$!1"
      do i=2,5 {
         if($?$i) { define bands <$!bands $!$!i> }
      }
   }

   set $0 = !is_set(objc_flags,$OBJ1_BRIGHT)&&detected&&!edge

   if('$bands' == 'reset') {
      set $0 = $0 && !saturated
   } else {
      #echo $0 Init $(sum($0))
      foreach i <$bands> {
         set $0 = $0 && \
          (!(flags$i & (1<<$OBJ1_SATUR)) || (flags2$i&(1<<$OBJ2_HAS_SATUR_DN)))
          #echo $0 Band $i: $(sum($0))
      }
   }
}
macro write_velocity 22 {
   # Write velocities in the same format as velocity.sql
   # Usage: write_velocity filename maglim
   local define file <"$!1">
   local define maglim "$!2"

   define v local
   photoL 2

   local define OBJ1_BINNED1	28
   local define OBJ2_DEBLEND_NOPEAK      14

   local set l = good && !blended && psfMag2 < $maglim && objc_type == 'star'
   set l = \
       (flags1 & (1<<$OBJ1_BINNED1)) + (flags2 & (1<<$OBJ1_BINNED1)) +\
       (flags3 & (1<<$OBJ1_BINNED1)) == 3*(1<<$OBJ1_BINNED1) && \
       (((flags21 | flags22 | flags23) & (1<<$OBJ2_DEBLEND_NOPEAK)) == 0) && \
       l
   
   foreach v (objc_rowc objc_colc  id objc_flags \
       psfMag1 psfMag2 psfMag3 rowv colv rowvErr colvErr psp_status ) {
      local set $v = $v if(l)
   }
   local set run=runs if(l)
   set objc_rowc = (run == runs[0]) ? objc_rowc : int(objc_rowc)%1361
   
   local set rerun = 0*objc_flags + $rerun
   local set camCol = atof(id)
   local set field = int(atof(substr(id,2,0)))
         set id = atof(substr(id,strlen(string(field))+3,0))
   local set ok_run = 0*objc_flags + 10
   local set nmatch = 0*objc_flags + 0
   local set dmin = 0*objc_flags + 0
   #
   # Convert pix/frame to deg/day
   #
   local set deg_day = 0.400/((54.5/24)*(1361.0/2048))

   foreach v (rowv colv rowvErr colvErr) {
      set $v = ($v == -9999) ? $v : $v*deg_day
   }

   define noclobber local
   print "$!file" { \
          run rerun camCol field  objc_rowc objc_colc  id objc_flags \
          psfMag1 psfMag2 psfMag3 rowv colv rowvErr colvErr ok_run psp_status \
          nmatch dmin \
   }
}

###############################################################################
#
macro cond3_fac2 23 {
   local define file "$!1"
   local define flags ($2)
   if(!$?3) { define 3 1 }\n local define desired "$!3"
      

   local define c3       ($flags & 0x1)
   local define scatter  ($flags & 0x2)
   local define byrun    ($flags & 0x4)
   local define byband   ($flags & 0x8)
   local define canonical ($flags & 0x10)

   da $file

   local define header read 1
   local define filterlist ( substr('$header', index('$header', ':') + 3, 0) )
   define header ( substr('$header', 0, index('$header', ':') + 2) )
   
   local define vecs <run rerun camCol field>
   define v local
   foreach v ($filterlist) {
      define vecs <$vecs w_$v c3_$v>
   }

   foreach v <$vecs> {
      set $v local
   }
   
   read '' <$vecs>

   local set desired = 0*run + $desired
   if(sum(desired) != dimen(desired)) {
      foreach v <$vecs> {
         set $v = $v if(desired)
      }
   }
   local define pt <$ptype>
   if(sum(desired) > 1000) {
      ptype 1 1
   }

   if(!$?_iy) { local define _iy 1 }
   if(!$?_ix) { local define _ix 1 }

   local set c3_x = 0.3,1.1,.01
   local set w_x=0.6,2.2,0.05

   if($c3) {
      local define prefix "c3"
   } else {
      local define prefix "w"
   }
   local set x = $prefix""_x

   foreach f ($filterlist) {
      set c3_$f = c3_$f + 0.02*(random(dimen(w_$f)) - 0.5)
   }

   if($byrun) {
      local set choose = run
   } else {
      local set choose = camCol
   } 

   local set chosen = uniq(sort(choose))
   define i local  local set c = { red yellow green cyan blue magenta }

   define f local
   if($scatter) {
      lim c3_x w_x
      set l local
      foreach f ($filterlist) {
         if($byband) {
            set_window -2 -3
            rel $($fx1 + 0.1*($fx2 - $fx1)) $($fy1 + 0.8*($fy2 - $fy1))
            putl 5 $f
         }
         bo
         do i=0, dimen(chosen) - 1 {
            ctype $(c[$i%dimen(c)])
            set l = (choose == chosen[$i]) ? 1 : 0
            
            poi c3_$f w_$f if(l)
         }
         ct 0

         if($canonical) {
            rel 0.6 $fy1  draw $uxp $fy2
         }
      }
      
      if($byband) {
         set_window -2 -3
      }
      
      rel $($fx1 + 0.1*($fx2 - $fx1)) $($fy1 + 0.9*($fy2 - $fy1))
      do i=0, dimen(chosen) - 1 {
         ctype $(c[$i%dimen(c)])
         
         label "\-2$!(chosen[$!i]) "
         ct 0
      }

      set_window

      xla cond3_2  $header
      yla psf width
   } else {
      set h local
      foreach f ($filterlist) {
         set_window 1 -$(dimen(<$filterlist>))
         set h=histogram($prefix""_$f:x)
         lim x h
         bo

         if($canonical) {
            rel 0.6 $fy1  draw $uxp $fy2
         }

         hi x h
         rel $($fx1 + 0.1*($fx2 - $fx1)) $($fy1 + 0.8*($fy2 - $fy1))
         putl 5 $f

         label "  "
         do i=0, dimen(chosen) - 1 {
            ctype $(c[$i%dimen(c)])
            
            label "\-2$!(chosen[$!i]) "
            ct 0
         }

         do i=0, dimen(chosen) - 1 {
            ctype $(c[$i%dimen(c)])
            set l = (choose == chosen[$i]) ? 1 : 0
            
            set h=$prefix""_$f if(l)
            set h=histogram(h:x)
            hi x h if(h > 0)
         }
         ct 0

      }
      set_window
      
      if($c3) {
         local define xla cond3_2
      } else {
         local define xla width
      }
      xla $xla
      # $header
   }
   #toplabel $header

   ptype $pt
}

if(0) {
#dev blackpostfile /home/s1/rhl/photo/c32.ps
#dev blackppm /home/s1/rhl/photo/c32.ppm
#ERASE cond3_fac2 /home/s1/rhl/photo/756.dat 0x2|0x8|0x0 \n#camCol==3

ERASE
if(0) {
#   cond3_fac2 /home/s1/rhl/photo/745.dat 0x2|0x8|0x10
#   cond3_fac2 /home/s1/rhl/photo/756.dat 0x2|0x8|0x10
#   cond3_fac2 /home/s1/rhl/photo/1033.dat 0x2|0x8|0x10
#   cond3_fac2 /home/s1/rhl/photo/1894.dat 0x2|0x8|0x10
#   cond3_fac2 /home/s1/rhl/photo/2579.dat 0x2|0x8|0x10
   cond3_fac2 /home/s1/rhl/photo/2583.dat 0x2|0x8|0x10
#   cond3_fac2 /home/s1/rhl/photo/2662.dat 0x2|0x8|0x10
#   cond3_fac2 /home/s1/rhl/photo/2709.dat 0x2|0x8|0x10 
#   cond3_fac2 /home/s1/rhl/photo/2728.dat 0x2|0x8|0x10 
} else {
   # cat {745,756,1033,1894,2579,2583,2662,2709,2728}.dat > c32.dat
   cond3_fac2 /home/s1/rhl/photo/c32.dat 0x2|0x4|0x8|0x10
}
}

macro plot_sky 14 {
   # Usage: plot_sky band [nrow] [yrange] [flags]
   # Plot sky in band $1 as a function on rowc/colc,
   # coloured in sets of $2 rows.  If $?3, plot that range of sky values
   # flags:
   #   0x1    Adjust each nrow section to have the same median as whole run
   #   0x2    Plot errors
   #
   # E.g. ERASE plot_sky 3 1000 10
   #
   local define band $1

   if(!$?2) {
      define 2 0
   }
   if($2 == 0) {
      local define drcen 0
      local set rcen = -1
   } else {
      local define drcen $2
      local set rcen = $drcen/2,1489*(field[dimen(field)-1]-field[0]+1),$drcen
   }

   if($?3) {
      local define yrange $3
   } else {
      local define yrange 5
   }

   if($?4) {
      local define flags ( $4 )
   } else {
      local define flags $4
   }
   local define normalise  ( $flags & 0x1 )
   local define errors     ( $flags & 0x2 )
   #
   # Fix all drcen-length blocks to have the same median
   #
   set g local
   define rcen local
   if(dimen(rcen) > 1 && $normalise) {
      set foo local
      local set sky$band = sky$band
      set g = !(flags$band & (1<<$OBJ1_CHILD))
      set foo = sky$band if(g)
      sort { foo }  local define med ( foo[int(dimen(foo)/2)] )
      foreach rcen rcen {
         set foo = sky$band if(g && abs(objc_rowc - $rcen) < $drcen/2)
         sort { foo }
         if(dimen(foo) > 0) {
            set sky$band = (g && abs(objc_rowc - $rcen) < $drcen/2) ? \
             sky$band + $med - foo[int(dimen(foo)/2)] : sky$band

             set foo = sky$band if(g && abs(objc_rowc - $rcen) < $drcen/2)
             sort { foo }
         }
      }
   }

   local set objc_colc = objc_colc + (atof(id) - 1)*(2048 + 500)

   define v local  
   set l local
   foreach rcen rcen {
      set g = !(flags$band & (1<<$OBJ1_CHILD))
      if(dimen(rcen) > 1) {
         set g = g && abs(objc_rowc - $rcen) < $drcen/2
      }
      set l = (objc_flags & (1<<$OBJ1_BRIGHT))
      foreach v (rowc colc) {
         set_window 1 -2
         range 0 $yrange
         lim objc_$v sky$band
         if($normalise && $?med) {
            rel $fx1 $med draw $fx2 $med
         }
         range 0 0
         #lim 0 0 163.5 165
         box
         if(dimen(rcen) > 1) {
            ctype_band $((($rcen-$drcen/2)/$drcen)%5)
         }
         if(0) {
            #ct red
            poi objc_$v sky$band if(g && l)
            if($errors) {
               error_y objc_$v (g&&l?sky$band:-1000) (g&&l?skyErr$band:0)
            }
            #ct 0
         }
         poi objc_$v sky$band if(g && !l)
         if($errors) {
            error_y objc_$v (g&&!l?sky$band:-1000) (g&&!l?skyErr$band:0)
         }
         ct 0
      }
      set_window
   }
}

###############################################################################
#
# 
#
macro rowc "02 " {
   # Compare objc_rowc and rowc.  Usage: rowc [flags] [logical]
   if(!$?1) { define 1 0 }\n  local define flags ($1)
   if(!$?2) { define 2 1 }\n  local define desired "$!2"

   local define colc 1  local define rowc 0
   define colc     ( $flags & 0x1 )
   define rowc     ( $flags & 0x2 )

   photoL 2
   local set desired = ($desired)

   if($colc) {
      if($rowc) {
         user abort Please choose rowc OR colc
      }
      set objc_x = objc_colc if(desired)
      set x = colc2 if(desired)
   }
   if($rowc) {
      set objc_x = objc_rowc if(desired)
      set x = rowc2 if(desired)
   }

   lim x (objc_x - x)
   lim x (0.1*{-1 1})
   box
   poi x (objc_x - x)
}

#ERASE rowc 0x2 good&&nchild==0&&!(flags2&(1<<$OBJ1_CHILD))&&psfMag2<117&&objc_type=='star'

###############################################################################
#
# Ivan Baldry writes:
# 
# I have now quantified "u-band sky-subtraction errors that are correlated
# with camera position". These are amount to about 27.6 mags per square
# arcsec.
# 
# 
# Details: 
# 
# Following from sdss-galaxies/msg.417 where I summarized some u-band
# problems, I looked further in to the issue of what is causing u_Petro to
# vary systematically with position in the camera?
# 
# To do this, I divided the selection of galaxies (14<r<18.5, \theta_50<23)  
# into u-band surface brightness bins where
# 
# u_SB = u_model - r_model + r_Petro + 2.5 log(\pi * petrorad_r^2)
# 
# is a robust estimate of the un-extinction-corrected u-band SB if you do
# not trust u_Petro. To quantify sky-subtraction errors, I measured the
# average of (u-r)_Petro - (u-r)_model with camera position. An example is
# shown here:
# 
# http://mrhanky.pha.jhu.edu/~baldry/sdss/test-u-SB-25-26.ps
# 
# where asterisks represents means and the squares represent medians. The
# dashed lines define the 90% range in the (u-r) means. Some of this will be
# cosmic variance but mostly it is due to the systematic errors with camera
# column. If I take various measures of this 90% range and subtract an
# estimate for cosmic and other variances that are not correlated with
# camera column, I can plot the systematic error as a function of u_SB:
# 
# http://mrhanky.pha.jhu.edu/~baldry/sdss/u-band-usb-var.ps
# 
# In particular, the triangles representing the errors in (u-r)_Petro - 
# (u-r)_model must be due to sky-subtraction errors. The solid line 
# represents a scenario where the errors are due to sky-subtraction errors 
# of 27.6 mag/arcsec^2 (also, effectively 90% range). 
# 
# This does not sound like much but for galaxies of u_sb = 25 mag/arcsec^2,
# this amounts to close to 10% errors. The u-r observed-frame colors in MAIN
# are typically 1-4. So that relatively typical galaxies of 22-23 mag per
# arcsec^2 in r can be dominated by sky-subtraction errors in the u-band.
# Note that half of MAIN galaxies are fainter than 22 in r_SB (petroradius 
# definition). 
# 
# These u-band sky-sub problems will probably not be fixed by improved flat
# fields (5.3) since the problems are probably caused by the scattered light
# directly rather than indirectly through errors in the flat fields.  Thus
# model u magnitudes will still be the best bet ... effectively they are
# weighted towards higher SB parts of the galaxy. Thinking about it, the
# best estimate of u_Petro would be to use g_Petro as the template (closer 
# in morphology):
# 
# u_pseudo_Petro = u_model - g_model + g_Petro - u_redden

macro baldry 24 {
   # Usage: baldry band flags [condition] [smooth]
   # For example:
   #    define b 1  photoL $b  set gal = good && objc_type=='galaxy'
   #    baldry $b 0x1|0x2|0x4|0x8 gal&&cmodelMag$b<19&&abs(SB-24.5)<.5 1
   local define b $1                    # band
   local define flags ($2)
   if(!$?3) { define 3 1 }\n local define desired "$!3"
   if(!$?4) { define 4 1 }\n local define smooth $4

   local define connect_points  ($flags & 0x1)
   local define chip_boundaries ($flags & 0x2)
   local define limit_range     ($flags & 0x4)
   local define color           ($flags & 0x8) # plot e.g. (u - g)_model 

   local define bref 2                  # reference band
   local define scale 0.4               # pixel size, arcsec

   if(1) {                              # petroRad
      local set SB = (cmodelMag$b - cmodelMag$bref) + petroMag$bref + \
       2.5*lg(pi*(petroRad2*$scale)**2)
   } else {                             # petroR50
      local set SB = (cmodelMag$b - cmodelMag$bref) + \
       petroMag$bref + 2.5*lg(2) + 2.5*lg(pi*(petroR502*$scale)**2)
   }
   local set desired = $desired
   set SB = SB if(desired)

   if($color) {
      local set dcol = (cmodelMag$b - cmodelMag$bref) if(desired)
   } else {
      local set dcol = (petroMag$b - petroMag$bref) - \
       (cmodelMag$b - cmodelMag$bref) if(desired)
   }
    
   local set camCol = atof(id)
   local set gutter = 500
   if($smooth > 1) {
      set gutter = $smooth*(int(gutter/$smooth) + (gutter%$smooth ? 1 : 0))
   }
   local set x = (objc_colc + (2048 + gutter)*(camCol - 1)) if(desired)

   if($smooth > 1) {
      set x = float(int(x/$smooth))
      local set bins = x sort {bins}
      #
      # Add points between the chips
      #
      set bins = uniq(bins) concat \
          (-gutter/2 + (2048 + gutter)*do(1,5))/$smooth
      sort { bins }

      define i local
      local set mean = 0*bins - 1000  local set median = mean
      set tmp local  set n local
      do i=0,dimen(bins) - 1 {
         set tmp = dcol if(x == bins[$i])
         set n = dimen(tmp)
         if(n > 0) {
            set mean[$i] = sum(tmp)/n
            sort { tmp }
            
            set median[$i] = (n%1 == 1) ? \
                tmp[int(n/2)]: (tmp[n/2 - 1] + tmp[n/2])/2
         }
      }
      local set is_mean = 1 + 0*bins concat 0*bins
      set x = $smooth*(bins + 0.5)  set x = x concat x
      set dcol = mean concat median
      #
      # We added points between the chips to make the plotting nicer;
      # process them
      #
      set is_mean = (dcol < -100) ? -1000 : is_mean
      set dcol = (dcol < -100) ? 0 : dcol
   } else {
      local set is_mean = 1 + 0*x
   }

   if($limit_range) {
      range 0 1
   }
   lim x dcol
   range 0 0
   notation -100 100 0 0
   box
   notation 0 0 0 0
   ct magenta
   if($connect_points) {
      local define verb "connect "
      sort { x dcol is_mean }
   } else {
      local define verb "points "
   }
   $verb x dcol if(is_mean == 1)
   ct green
   $verb x dcol if(is_mean == 0)
   ct 0
   #
   # Draw chip boundaries?
   #
   if($chip_boundaries) {
      lt 2
      set x = (2048 + gutter)*do(1,5)
      rel 0 $fy1  draw $uxp $fy2
      foreach i x {
         rel ($i-gutter) $fy1  draw $uxp $fy2
         rel $i $fy1  draw $uxp $fy2
      }
      rel ($uxp+2048) $fy1  draw $uxp $fy2
      lt 0
   }

   local define bn (filter_name($b))
   local define brefn (filter_name($bref))
   
   if($color) {
      local define yla "($!bn - $!brefn)_{\2cmodel}"
   } else {
      local define yla "($!bn - $!brefn)_{\2Petro} - ($!bn - $!brefn)_{\2cmodel}"
   }
   xla colc
   yla $yla

   toplabel "Frames $!frames  $!(dimen(SB)) objects"
   if(dimen(desired) != 1) {
      toplabel \move-15000 -1000{$desired}
   }
}

#14<r<18.5, \theta_50<23

if(0) {
   define b 0  photoL $b  set gal = good && objc_type=='galaxy'
   ERASE baldry $b 0x0|0x2|0x4|0x8 gal&&cmodelMag$b<20&&abs(0.4*petroRad2-10)<5&&abs(SB-24.5)<.5 5
}

macro fix_photometry {
   # Fix up the photometry for all the data currently read, so that
   # the bright-end stellar aperture magnitudes match the PSF magnitudes

   local set camCol = atof(id)
   local set camCols = uniq(sort(camCol))

   set offset local  set tmp local
   define c local  define i local  define v local
   do i=0, $NFILTER - 1 {
      photoL $i
      foreach c camCols {
         set offset = aperMag$i - psfMag$i \
             if(camCol == $c && good && objc_type == 'star' && \
          (psfMagErr0 + psfMagErr1 + psfMagErr2 + psfMagErr3 + psfMagErr4)/5 < 0.02)
         sort { offset }
         set offset = offset[int(dimen(offset)/2)]

         echo camCol $c  band $i  <aper - psf> = $(offset)

         foreach v {deV exp fiber model petro psf} {
            set $v""Mag$i = $v""Mag$i + offset
         }
      }
   }
}
#fix_photometry
macro psfModel 2 {
   local set radii = $1
   local define beta $2

   set $0 = 0*radii
   set radii = 0 concat radii

   set r1 local  set r2 local

   define i local
   do i=0, dimen(radii) - 2 {
      set r1 = radii[$i]
      set r2 = radii[$i + 1]
      set $0[$i] = 2*(r1**(2 - $beta) - r2**(2 - $beta))/(($beta - 2)*(r2*r2 - r1*r1))
   }
}

macro fit_profile 4 {
   local define flags ($1)
   local define b $2
   local define iprofMin $3
   local define beta $4

   local define nprof -1
   define i local
   do i=0, 14 {
      if (cprof$b[$i] == 0 && $nprof < 0) {
         define nprof $i
      }
   }

   local set radii = profileRadii()     # definitions of radii
   local set model = psfModel(radii, $beta)
   set sigma2 local
   
   local set sum_pm = 0.0               # sum (profile * model)/sigma^2
   local set sum_mm = 0.0               # sum (model   * model)/sigma^2

   do i=$iprofMin, $nprof - 1 {
      set sigma2 = cprofErr$b[$i]**2

      if (sigma2 > 0) {
         set sum_pm = sum_pm + cprof$b[$i]*model[$i]/sigma2
         set sum_mm = sum_mm +   model[$i]*model[$i]/sigma2
      }
   }

   if (sum_mm == 0.0) {
      set $0 = 0.0
   } else {
      set $0 = sum_pm/sum_mm            # intensity of model */
   }

   lim (cprofErr$b>=0?lg(radii+0.01):0) (cprof$b <= 0 ? 0 : lg(cprof$b)) box con (lg(radii)) (lg(cprof$b))
   ctype red
   con (lg(radii)) (lg($0*model)) if(radii > 1)
   ctype 0

   #print { radii model }
   
}
