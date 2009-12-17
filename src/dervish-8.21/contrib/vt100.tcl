set tc \
{xterm|vs100|xterm terminal emulator (X Window System):\
        :AL=\E[%dL:DC=\E[%dP:DL=\E[%dM:DO=\E[%dB:IC=\E[%d@:UP=\E[%dA:\
        :al=\E[L:am:\
        :bs:cd=\E[J:ce=\E[K:cl=\E[H\E[2J:cm=\E[%i%d;%dH:co#80:\
        :cs=\E[%i%d;%dr:ct=\E[3k:\
        :dc=\E[P:dl=\E[M:\
        :im=\E[4h:ei=\E[4l:mi:\
        :ho=\E[H:\
        :is=\E[r\E[m\E[2J\E[H\E[?7h\E[?1;3;4;6l\E[4l:\
        :rs=\E[r\E[m\E[2J\E[H\E[?7h\E[?1;3;4;6l\E[4l\E<:\
        :k1=\EOP:k2=\EOQ:k3=\EOR:k4=\EOS:kb=^H:kd=\EOB:ke=\E[?1l\E>:\
        :kl=\EOD:km:kn#4:kr=\EOC:ks=\E[?1h\E=:ku=\EOA:\
        :li=\E[2m:md=\E[1m:me=\E[m:mr=\E[7m:ms:nd=\E[C:pt:\
        :sc=\E7:rc=\E8:sf=\n:so=\E[7m:se=\E[m:sr=\EM:\
        :te=\E[2J\E[?47l\E8:ti=\E7\E[?47h:\
        :up=\E[A:us=\E[4m:ue=\E[m:xn:
}

proc vtLine {} {
    puts stdout \033\[2m nonewline
    return
}

#Put a string at x,y then return cursor to lower left corner
proc vtPutxy {x y char} {
   puts stdout \033\[${x}\;${y}H${char} nonewline
#   puts stdout \033\[24\;0H nonewline
   }

proc vtMovexy {x y} {
   puts stdout \033\[${x}\;${y}H nonewline
   }

#Set scroll region from n1 to n2
proc vtScroll {n1 n2} {
   puts stdout \033\[${n1}\;${n2}r nonewline
   puts stdout \033\[${n2}\;0H nonewline
   return
   }

proc vtBold {} {
   puts stdout \033\[1m nonewline
   return
   }

proc vtReverse {} {
   puts stdout \033\[7m nonewline
   return
   }

proc vtNormal {} {
   puts stdout \033\[m nonewline
   return
   }

proc vtSaveCursor {} {
   puts stdout \0337 nonewline
   return
   }

proc vtRestoreCursor {} {
   puts stdout \0338 nonewline
   return
   }

proc vtInsertLine {} {
   puts stdout \033\[L nonewline
   return
   }

proc vtClearToDisplayEnd {} {
   puts stdout \033\[J nonewline
   return
   }

proc vtClearToLineEnd {} {
   puts stdout \033\[K nonewline
   return
   }

proc vtClearScreen {} {
   puts stdout \033\[H\033\[2J nonewline
   return
   }

proc screen {} {
   vtClearScreen
   vtMovexy 24 1
   return
   }

proc vtDelChar {} {
   puts stdout \033\[P nonewline
   return
   }

proc vtDelLine {} {
   puts stdout \033\[M nonewline
   return
   }

proc vtHome {} {
   puts stdout \033\[H nonewline
   return
   }

proc vtReset {} {
# :is=\E[r\E[m\E[2J\E[H\E[?7h\E[?1;3;4;6l\E[4l:\
   puts stdout \033\[r nonewline
   puts stdout \033\[m nonewline
   puts stdout \033\[2J nonewline
   puts stdout \033\[H nonewline
   puts stdout \033\[7h nonewline
   puts stdout \033\[1 nonewline
   puts stdout \033\[3 nonewline
   puts stdout \033\[4 nonewline
   puts stdout \033\[5 nonewline
   puts stdout \033\[4l
   return
   }

proc vtUnderline {} {
   puts stdout \033\[4m nonewline
   return
   }

proc vtBell {} {
   puts stdout [format %c 7] nonewline 
   return
   }

#The following is a template for printing lots of vt100 special symbols
#char can be a-z and some punctuation marks.  Unused chars are echoed normally.

proc vtBox {} {
   set char a
   puts stdout [format \033\[0m%c${char}\033\[0m%c 14 15] nonewline
   }

proc vtAlt {line {nonewline ""}} {
   if {$nonewline == ""} then {
	puts stdout [format \033\[0m%c${line}\033\[0m%c 14 15]
   } else {
	puts stdout [format \033\[0m%c${line}\033\[0m%c 14 15] nonewline
	}
   }

#Use normal char set
proc vtSet0 {} {
   puts stdout [format \033\[0m%c 15] nonewline
   }

#Use alternate char set
proc vtSet1 {} {
   puts stdout [format \033\[0m%c 14] nonewline
   }
