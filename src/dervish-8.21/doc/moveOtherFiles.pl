#!/usr/local/bin/perl

# move all of the files in the first directory, except those explicitly
# listed to the new www directory. The input list
# consists of -
#
#          the first argument is the directory to move from
#          the second argument is the directory to move to
#          all the rest are the files not to move.
#
$dir = $ARGV[0];
$newDir = $ARGV[1];
$exceptFile = $dir."CVS";
$inputFiles = join(' ', @ARGV);

open(FIND, "find $dir -print |") || die "Couldn't run find: $!\n";

while ($filename = <FIND>) {
  chop $filename;
  $_ = $inputFiles;
  if (! /$filename/) {             # if the file is not in the list
    $_ = $filename;
    if (! /$exceptFile/) {          # and if the file is not CVS
      system "cp $filename $newDir";
    }
  }
}


