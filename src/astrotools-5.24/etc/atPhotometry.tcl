# Procedures to do color transformations

set atUbvriToUgrizArgs {
  { atUbvriToUgriz "convert UBVRI colors to ugriz" }
  { <U> STRING "" U "U" }
  { <B> STRING "" B "B" }
  { <V> STRING "" V "V" }
  { <R> STRING "" R "R" }
  { <I> STRING "" I "I" }
}

ftclHelpDefine astrotools atUbvriToUgriz [shTclGetHelpInfo atUbvriToUgriz $atUbvriToUgrizArgs]
  
proc atUbvriToUgriz { args } {

#
# access help and parse inputs
#
    upvar #0 atUbvriToUgrizArgs formal_list

    if { [shTclParseArg $args $formal_list atUbvriToUgriz] == 0 } { return }
#
# convert colors
#
    set g [expr "$V + 0.57*($B-$V)-0.12"]
    set r [expr "$V - 0.84*($V-$R)+0.13"]
    set u [expr "$g + 1.38*($U-$B) + 1.14"]
    if {[expr $R-$I] < 1.15} {
	set i [expr "$r - 0.98*($R-$I) + 0.24"]
    } else {
	set i [expr "$r - 1.40*($R-$I) + 0.72"]
    }
    if {[expr $R-$I] < 1.7} {
	set z [expr "$r - 1.61*($R-$I) + 0.41"]
    } else {
	set z [expr "$r - 2.67*($R-$I) + 2.19"]
    }
    return [format "%6.3f %6.3f %6.3f %6.3f %6.3f" $u $g $r $i $z]
}
