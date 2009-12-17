proc makeText {} {
	echo <h2> <a name="hg">HG structure</a> </h2>
	dumpHelp {
			hgClear hgDefine hgDel hgFill hgFillFromAf hgFromAf 
			hgMean hgNew hgPlot hgPrint hgReg hgSigma
		}
	echo <h2> <a name="af">AF structure</a> </h2>
	dumpHelp {
			afClear afDefine afDel afExtreme 
			afFromAf afFromList afGetError 
			afGetValue afIndex afInvert afLimit afMaskSet
			afMaskSetFromChain afNew afOper afPlot 
			afPrint afSet afSetWithConstafSigmaClip afSize
			resizePlot 
		 }
	echo <h2> <a name="hgstate">PGSTATE structure </h2>
	dumpHelp {
			pgstateClose pgstateDefault pgstateDel pgstateNew 
			pgstateNextWindow pgstateOpen pgstatePrint pgstateSet
		 }
	echo <h2><a name="tlist"> Thing Lists </h2>
	dumpHelp {listFromPlot listToSao }
	echo <h2><a name="pt"> PT Commands </h2>
	dumpHelp {ptNew ptDefine ptDel }
	echo <h2><a name="odd"> Odd Commands </h2>
	dumpHelp {randGet regFluctuateAsSqrt seedSet }

}

proc dumpHelp {list} {
	echo <listing>
	foreach command $list {
		echo [help $command]
		echo . . . . . . . . . . . . . . . . . . . . . . . . . . . .
	}
	echo </listing>
}

makeText
