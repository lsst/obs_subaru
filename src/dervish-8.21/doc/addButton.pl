#!/usr/local/bin/perl

# Read in the unique search string that should signal if the bottomButtons
# have already been added to the file.   If the string is not present in the
# file, add the bottomButtons to the end.
#

$bfile = "bottomButtons.html";
$buttons = `cat $bfile`;            # get the bottom buttons

# Do not change the value of the following string.
$string = "These are the buttons to be included at the bottom of each page.";

FILE:
while (<>) {
  if ($ARGV ne $oldargv) {
    $rtn = `fgrep -l "$string"  $ARGV`; # see if string is in file already
    chop($rtn);                       # remove return at end of string
    if ("$rtn" ne "$ARGV") {
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
    $buttonsFound = 1;              # the buttons have already been added
  } elsif ((m/^<\/html>/i) || (m/^<\/body>/i)) {
    if (($buttonsFound == 0) && ($matchMade == 0)) {
      $matchMade = 1;               # only input the special lines once
      print $buttons;               # add buttons to the bottom of the file
    }
  }
  if (eof) {                        # reset in case have multiple input files
# Make sure that the lines have been added, in case there is no /html line.
    if (($buttonsFound == 0) && ($matchMade == 0)) {
      print $buttons;
    }
    close(ARGV);
    $matchMade = 0;
    $butonsFound = 0;
  }
}
continue {
  print;                            # print each line of input to output file
}
select(STDOUT);                     # restore the default output filehandle


