#
# Test code for template verbs
#
# This file should produce no output unless a test fails, in which case
# it should signal an error and return a helpful string
#
# add
#
if {[contrib_add 1 2] != 3} {
   error "Adding 2 integers"
}

if {[contrib_add 1.1 2.3] != 3.4} {
   error "Adding 2 floats"
}
#
# subtract
#
if {[contrib_subtract 1.2 5.2] != 4} {
   error "subtracting 2 floats"

}
#
# multiply
#
if {[contrib_multiply 10 3.14] != 31.4} {
   error "multiplying by a positive integer"
}

if {[contrib_multiply -10 3.14] != -31.4} {
   error "multiplying by a negative integer"
}

if {[contrib_multiply 0 3.14] != 0} {
   error "multiplying by a zero"
}

if {[catch { contrib_multiply 0.1 10 }] != 1} {
   error "failed to diagnose multiplying by a non-integer"
}
