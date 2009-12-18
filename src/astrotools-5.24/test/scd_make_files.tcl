set uncompReg [regNew 256 256]
for {set i 0} {$i < 256} {incr i} {
for {set j 0} {$j < 256} {incr j} {
  regPixSetWithDbl $uncompReg $i $j [expr $i + $i * $j]
}}
regWriteAsFits $uncompReg uncompressable.fits

set nullReg [regNew 0 0]
regWriteAsFits $nullReg nullfile.fits
