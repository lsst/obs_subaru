#!/usr/local/bin/perl

# move all of the input files to the new www directory. The input list
# consists of -
#
#          the first argument is the directory to move to
#          all the rest are the files to move.
#
$dir = $ARGV[0];

foreach $i (1 .. $#ARGV) {
  $file = @ARGV[$i];
  system "cp $file $dir";
}
