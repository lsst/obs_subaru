# Run the test code

echo 
echo Run test code in testAll.tcl

if [info exists failed] { unset failed }
foreach test [list                  \
	testTransInverseApplyNowrap \
	testQuasar                  \
	testAberration              \
	testAperture                \
	testObject                  \
	testGalaxy                  \
	testRegSci                  \
	testSurveyGeometry          \
	testEphemeris               \
	testCali                    \
	testHgMath                  \
	testLinearFits              \
	testMatch                   \
	testSlalib                  \
	testTrans                   \
	testAirmass                 \
	testPMatch                  \
	testDustGetval              \
	testSmoothReg               \
	] {
   echo "====================================> Call $test"
   source $test.tcl
   if { [set ret [$test]] != 0} {
      echo "TEST-ERR: $test failed"
      set failed($test) $ret
   }
}
echo testing done
if {[array size failed] > 0} {
   error "TEST-ERR: [llength [array names failed]] tests failed"
}
