#!/usr/local/bin/perl

# Read in the unique search string that should signal if the bottomButtons
# have already been added to the file.   If the string is not present in the
# file, add the bottomButtons to the end.
#

# Do not change the value of the following string.
$string = "These are the buttons to be included at the bottom of each page.";

FILE:
while (<>) {
  if ($ARGV ne $oldargv) {
    $rtn = `fgrep -l "$string"  $ARGV`; # see if string is in file already
    chop($rtn);                       # remove return at end of string
    if ("$rtn" eq "$ARGV") {
      rename($ARGV, $ARGV . '.bak');  # create a backup of the original file
      open(ARGVOUT, ">$ARGV");
      select(ARGVOUT);                # write to a file of the same name
      $oldargv = $ARGV;
    } else {                          # file has string already, go to next 
      close(ARGV);
      next FILE;
    }
  } 
  if (m/$string/) {
    $buttonsFound = 1;            # found the beginning of the buttons
  } elsif ((m/^<\/html>/i) || (m/^<\/body>/i)) {
    $buttonsFound = 0;
  } 

  if (eof) {                        # reset in case have multiple input files
    close(ARGV);
    $buttonsFound = 0;
  }
}
continue {
  if ($buttonsFound == 0) {
    print;                          # print each line of input to output file
  }
}
select(STDOUT);                     # restore the default output filehandle


