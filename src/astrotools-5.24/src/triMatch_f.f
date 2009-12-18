	subroutine smtm(n1,x1,y1,varxy1,n2,x2,y2,varxy2,a,b,c,d,e,f,nmatch,idb)
c       
c       ***   Given list of n1 reference coords and n2 comparison coords,
c       calculate transform coefficients (a-f) from comp to ref system
c       
c       Xr = a*Xc + b*Yc + c
c       Yr = d*Yc + e*Xc + f
c       
c       varxy1 & varxy2 are variances in coords for each list
c       
        integer maxvert, maxtri
	parameter (maxvert = 50)
	parameter (MAXTRI= MAXVERT * (MAXVERT-1) * (MAXVERT - 2) / 6)
	integer*2 idata1(24,maxtri),idata2(24,maxtri),
	1    tmatch1(maxtri),tmatch2(maxtri)
	logical debug
	integer*4 dbglun,nmatch,match1(maxvert),match2(maxvert)
	real*4    xw1(maxvert),yw1(maxvert),xw2(maxvert),yw2(maxvert),
	1    a,b,c,d,e,f
	real*8   x1(maxvert),y1(maxvert),x2(maxvert),y2(maxvert),varxy1,varxy2,
	1    rlimit1,rlimit2,data1(6,maxtri),data2(6,maxtri),
	2    work(6,maxtri)
	integer*4 n1, n2, i1, i2, imat, idb
	
	equivalence (data1,idata1),(data2,idata2)
	
	rlimit1 = 10
	rlimit2 = 10
	
	if (idb.eq.1) then
	   debug = .true.
	else
	   debug = .false.
	end if


	if (debug) then
	   dbglun = 1
	   open(unit=dbglun,file='triMatch.out',form='formatted',
     +		status='UNKNOWN')
	endif
	
	call CDMTRIANG(DEBUG,DBGLUN,
	1    N1,X1,Y1,VARXY1,RLIMIT1,
	2    N2,X2,Y2,VARXY2,RLIMIT2,
	3    NMATCH,MATCH1,MATCH2,
	4    DATA1,IDATA1,DATA2,IDATA2,
	5    WORK,TMATCH1,TMATCH2)
	
c	type *, 'In tm:  ', nmatch,' points matched'
	imat = 0
	do i1 = 1,n1
	   i2 = match1(i1)
	   if (i2.gt.0) then
	      imat = imat + 1
	      xw1(imat) = x1(i1)
	      yw1(imat) = y1(i1)
	      xw2(imat) = x2(i2)
	      yw2(imat) = y2(i2)
	   endif
	enddo
	call plate(xw2,yw2,xw1,yw1,nmatch,a,b,c,d,e,f)
	
	if (debug) then
	   close(unit=dbglun)
	end if
	
	end
	
	SUBROUTINE CDMTRIANG(DEBUG,DBGLUN,
	1    N1,X1,Y1,VARXY1,RLIMIT1,
	2    N2,X2,Y2,VARXY2,RLIMIT2,
	3    NMATCH,MATCH1,MATCH2,
	4    DATA1,IDATA1,DATA2,IDATA2,
	5    WORK,TMATCH1,TMATCH2)
C       
C       
C       
C	This subroutine is used by the coordinate match package
C	to match coordinates when there are only a few coordinates
C	in each list.  The algorithm used here is to look at all
C	triangles in each list and try to match them.
C       
C	DEBUG (L*4 input) is true if debug output is to be produced
C       
C	DBGLUN (I*4 input) is the logical unit number for debug output.
C	This should be a carriagecontrol=list file.
C       
C	N1 (I*4 input) is the number of coordinate pairs in the first
C	list.  This must be 3 or greater and must be less than
C	MAXVERT (see below).  These constraints are not checked.
C
C	X1 and Y1 (R*8 input arrays of length N1) contain the coordinates
C	in the first list.
C
C	VARXY1 (R*8 input) is the variance in the X and Y coordinates
C	in the X1, Y1 arrays.
C
C	RLIMIT1 (R*8 input) is the maximum ratio of long to short
C	side for a triangle.  Triangles that exceed this are discarded.
C
C	N2, X2, Y2, VARXY2 and RLIMIT2 are the same for the second list.
C
C	NMATCH (I*4 output) is the number of points found to match.
C
C	MATCH1 (I*4 output array of length N1) is such that
C	MATCH1(I) is the sequence number of the point in the second
C	list which matches point I in the first list.  MATCH1(I)=0
C	if no point is found to match point I.
C
C	MATCH2 is the same for the second list.
C
C	The remaining arguments are work arrays.  The main program
C	should define a parameter MAXVERT, the maximum number of
C	vertices in each list.  Then MAXTRI=
C	MAXVERT * (MAXVERT-1) * (MAXVERT - 2) / 6 is the maximum
C	number of triangles.  Note that MAXVERT must be between
C	3 and 59.  The lower limit comes from the fact that we
C	must have at least 3 vertices to get one triangle and the
C	upper limit from the fact that with 60 vertices, we would
C	get more than 32767 triangles.
C
C	DATA1, DATA2, and WORK are REAL*8 work arrays dimensioned
C	at least (6,MAXTRI).  IDATA1 should be the same as DATA1
C	in the calling program.  This is so the array can be referenced
C	both as a REAL*8 and an INTEGER*2 array by this routine.
C	Similarly, IDATA2 should be the same array as DATA2.
C	Finally, TMATCH1 and TMATCH2 are INTEGER*2 work arrays
C	dimensioned (MAXTRI).
C
C
C
C	Edward J. Groth   October, 1984
C
C	Modified 10-Dec-84 - EJG - to add debug option
C	and triangle ratio discard option
C
C
C
C
	IMPLICIT REAL*8 (A-H,O-Z)
C       
	LOGICAL*4 DEBUG
	INTEGER*4 DBGLUN
C       
	DIMENSION X1(*),Y1(*)
	DIMENSION X2(*),Y2(*)
	DIMENSION MATCH1(*),MATCH2(*)
C       
	DIMENSION DATA1(6,*)
	INTEGER*2 IDATA1(24,*)
	DIMENSION DATA2(6,*)
	INTEGER*2 IDATA2(24,*)
C       
	INTEGER*2 WORK(*)
C       
	INTEGER*2 TMATCH1(*)
	INTEGER*2 TMATCH2(*)
	
	integer   n1,n2,nmatch,match1,match2,i,j,nt1,nt2,nmatpos,nmatneg
C       
C	The DATA arrays hold information about each triangle.
C	Each triangle is ordered so vertex 1 is the vertex between
C	the shortest and longest side, vertex 2 between the shortest
C	and middle side, and vertex 3 between the middle and longest side.
C       
C	DATAx(1,I) holds the cosine of the angle at vertex 1.
C	DATAx(2,I) holds the variance of the cosine.
C	DATAx(3,I) holds the ratio of the longest to the shortest side.
C	DATAx(4,I) holds the variance in the ratio.
C	DATAx(5,I) holds the log of the perimeter of the triangle.
C	IDATAx(21,I) holds the sequence number of the coordinates at vertex 1
C	IDATAx(22,I) holds the sequence number of the coordinates at vertex 2
C	IDATAx(23,I) holds the sequence number of the coordinates at vertex 3
C	IDATAx(24,I) is 1 if vertices 1,2,3 are traversed CCW, 0 if CW.
C	The arrays are sorted in order from smallest ratio to largest ratio.
C	A single array is used in order to facilitate the sorting.
C       
C	The TMATCHx arrays keep track of which triangles match which
C	triangles in the other list.
C       
C       
	LOGICAL SAMESENSE
C       
C       
C       
C       
C	Zero the output match arrays
	DO I=1,N1
	   MATCH1(I)=0
	ENDDO
	DO I=1,N2
	   MATCH2(I)=0
	ENDDO
C       
C	Generate the sorted lists of triangles.
	IF(DEBUG) THEN
	   WRITE(DBGLUN,100)
 100	   FORMAT('Generating triangles for the first list...')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
	CALL CDMTRIGEN(N1,X1,Y1,VARXY1,RLIMIT1,NT1,DATA1,IDATA1,WORK)
	IF(DEBUG) THEN
	   WRITE(DBGLUN,101) NT1
 101	   FORMAT('First list triangles complete with ',I6,' triangles')
	   CALL CDMSTAMP(DBGLUN)
	   WRITE(DBGLUN,102)
 102	   FORMAT('   Seq     Cos        Tol        Rat   ',
	1	'     Tol         log p    1    2    3  O')
	   WRITE(DBGLUN,103) (I,DATA1(1,I),DATA1(2,I),DATA1(3,I),
	1	DATA1(4,I),DATA1(5,I),IDATA1(21,I),
	2	IDATA1(22,I),IDATA1(23,I),IDATA1(24,I),
	3	I=1,NT1)
 103	   FORMAT(I6,2F11.7,2F11.6,F11.5,3I5,I3)
	ENDIF
	IF(DEBUG) THEN
	   WRITE(DBGLUN,104)
 104	   FORMAT('Generating triangles for the second list...')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
	CALL CDMTRIGEN(N2,X2,Y2,VARXY2,RLIMIT2,NT2,DATA2,IDATA2,WORK)
	IF(DEBUG) THEN
	   WRITE(DBGLUN,105) NT2
 105	   FORMAT('Second list triangles complete with ',I6,' triangles')
	   CALL CDMSTAMP(DBGLUN)
	   WRITE(DBGLUN,102)
	   WRITE(DBGLUN,103) (I,DATA2(1,I),DATA2(2,I),DATA2(3,I),
	1	DATA2(4,I),DATA2(5,I),IDATA2(21,I),
	2	IDATA2(22,I),IDATA2(23,I),IDATA2(24,I),
	3	I=1,NT2)
	ENDIF
C       
C	Match the triangles.
	IF(DEBUG) THEN
	   WRITE(DBGLUN,106)
 106	   FORMAT('Starting actual triangle matching...')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
	CALL CDMTRIMAT(NT1,DATA1,IDATA1,NT2,DATA2,IDATA2,
	1    NMATPOS,NMATNEG,TMATCH1,TMATCH2)
	IF(DEBUG) THEN
	   WRITE(DBGLUN,107) NMATPOS,NMATNEG
 107	   FORMAT('Actual triangle matching complete with ',
	1	I6,' positive and ',I6,' negative matches')
	   CALL CDMSTAMP(DBGLUN)
	   WRITE(DBGLUN,108)
 108	   FORMAT('Matches for list 1:'/' Seq 1  Seq 2')
	   WRITE(DBGLUN,109) (I,TMATCH1(I),I=1,NT1)
 109	   FORMAT(I6,I7)
	   WRITE(DBGLUN,110)
 110	   FORMAT('Matches for list 2:'/' Seq 2  Seq 1')
	   WRITE(DBGLUN,109) (I,TMATCH2(I),I=1,NT2)
	ENDIF
C       
C	If we didn't find any triangles, give up
	IF(NMATPOS.LE.0.AND.NMATNEG.LE.0) THEN
	   NMATCH=0
	   RETURN
	ENDIF
C       
C	Now use scale factor to discard matches
	IF(DEBUG) THEN
	   WRITE(DBGLUN,111)
 111	   FORMAT('Log scale factor for all matches:'/
	1	' Seq 1 Seq 2     Log S')
	   DO I=1,NT1
	      IF(TMATCH1(I).NE.0) THEN
		 J=ABS(TMATCH1(I))
		 XXX=DATA2(5,J)-DATA1(5,I)
		 WRITE(DBGLUN,112) I,J,XXX
 112		 FORMAT(2I6,F12.5)
	      ENDIF
	   ENDDO
	   WRITE(DBGLUN,113)
 113	   FORMAT('Starting scale factor discard...')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
	CALL CDMTRIDSM(NT1,DATA1,DATA2,
	1    TMATCH1,TMATCH2,NMATPOS,NMATNEG)
	IF(DEBUG) THEN
	   WRITE(DBGLUN,114) NMATPOS,NMATNEG
 114	   FORMAT('Scale factor discard complete with ',
	1	I6,' positive and ',I6,' negative matches')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
C       
C	If none left, punt
	IF(NMATPOS.LE.0.AND.NMATNEG.LE.0) THEN
	   NMATCH=0
	   RETURN
	ELSEIF(NMATPOS.GE.NMATNEG) THEN
	   NMATCH=NMATPOS
	   SAMESENSE=.TRUE.
	ELSE
	   NMATCH=NMATNEG
	   SAMESENSE=.FALSE.
	ENDIF
C       
C	Get rid of the wrong sense matches
	IF(DEBUG) THEN
	   WRITE(DBGLUN,115)
 115	   FORMAT('Starting wrong sense discard...')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
	CALL CDMTRIDWS(NT1,TMATCH1,TMATCH2,SAMESENSE)
	IF(DEBUG) THEN
	   WRITE(DBGLUN,116) NMATCH
 116	   FORMAT('Wrong sense discard complete with ',I6,' matches')
	   CALL CDMSTAMP(DBGLUN)
	   WRITE(DBGLUN,111)
	   DO I=1,NT1
	      IF(TMATCH1(I).NE.0) THEN
		 J=ABS(TMATCH1(I))
		 XXX=DATA2(5,J)-DATA1(5,I)
		 WRITE(DBGLUN,112) I,J,XXX
	      ENDIF
	   ENDDO
	ENDIF
C       
C	Assign point matches based on triangle matches
	IF(DEBUG) THEN
	   WRITE(DBGLUN,117)
 117	   FORMAT('Starting vote...')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
	CALL CDMTRIASM(DEBUG,DBGLUN,N1,N2,IDATA1,IDATA2,NT1,TMATCH1,
	1    WORK,WORK(3*N1*N2+1),NMATCH,MATCH1,MATCH2)
	IF(DEBUG) THEN
	   WRITE(DBGLUN,118) NMATCH
 118	   FORMAT('Vote complete with ',I6,' matches')
	   CALL CDMSTAMP(DBGLUN)
	   WRITE(DBGLUN,119)
 119	   FORMAT('Point matches:'/' Seq 1 Seq 2')
	   DO I=1,N1
	      IF(MATCH1(I).GT.0) THEN
		 WRITE(DBGLUN,120) I,MATCH1(I)
 120		 FORMAT(2I6)
	      ENDIF
	   ENDDO
	ENDIF
C       
C	Done
	RETURN
	END
	
	

	SUBROUTINE CDMTRIASM(DEBUG,DBGLUN,
	1                    N1,N2,IDATA1,IDATA2,
	2                    NT1,TMATCH1,VOTE1,VOTE2,
	3                    NMATCH,MATCH1,MATCH2)
C
C	This subroutine is used by the triangle coordinate
C	matching package to convert triangle matches to
C	point matches.
C
C	DEBUG (L*4 input) true if debug output should be produced.
C
C	DBGLUN (I*4 input) specifies the logical unit number for
C	debug output.  This file should be opened as a
C	carriagecontrol=list file.
C
C	N1 and N2 (I*4 input) are the number of points
C	in the first and second lists.
C
C	IDATA1 and IDATA2 (I*2 input arrays) contain information
C	about the triangles.  Each array is dimensioned
C	(24,number of triangles).  The elements of interest
C	are IDATAx(21,x), IDATAx(22,x), IDATAx(23,x) which
C	contain the sequence numbers of the vertices of each
C	triangle.
C
C	NT1 (I*4 input) is the number of triangles in list 1.
C
C	TMATCH1 (I*2 input array of length NT1) gives for each
C	triangle in list 1, the matching triangle in list 2,
C	or 0, if the triangle does not match any triangle in
C	list 2.
C
C	VOTE1 and VOTE2 are I*4 work arrays of length 3*N1*N2.
C
C	NMATCH (I*4 output) is the number of matching points found.
C
C	MATCH1 and MATCH2 (I*4 output arrays of length N1 and N2)
C	are such that MATCH1(I) is the sequence number of the point
C	in list 2 which matches point I in list 1 and vice versa.
C	It is assumed that these arrays have previously been set
C	to 0.
C
C	The algorithm is as follows:  each matched triangle defines
C	three pairs of points which match.  For each such pair the
C	appropriate location in the VOTE1 array is incremented.
C	This is VOTE1(1,(I1-1)*N2+I2)).  VOTE1(2,...) = I1 and
C	VOTE1(3,...) = I2.    Then the VOTE1 array is sorted
C	in order of decreasing vote.  (The VOTE2 array is used to
C	facilitate the sorting.)  The VOTE1 array is examined
C	and each pair of points is assigned as a matched pair
C	in the MATCH1 and MATCH2 arrays and NMATCH is incremented.
C	This continues until one of the following happens:
C	The vote reaches 0.  An attempt is made to assign a
C	point that has already been assigned.  The number of
C	votes for the current pair is less than half the number
C	of votes for the previous pair.
C
C	If at the end, less than 3 pairs have been matched,
C	then, not even a whole triangle matched, so NMATCH
C	is set to 0.
C
C
C	Edward J. Groth   November, 1984
C
C	Modified 10-Dec-84 - EJG - to add debug option
C       Modified 15-Feb-94 - JL  - make VOTE arrays I*4 and use CERNLIB SORTI
C
C
C
C
	IMPLICIT INTEGER (A-Z)
C
	INTEGER*2 IDATA1(24,*)
	INTEGER*2 IDATA2(24,*)
	INTEGER*2 TMATCH1(*)
	INTEGER*4 VOTE1(3,*)
	INTEGER*4 VOTE2(3,*)
	INTEGER*4 MATCH1(*)
	INTEGER*4 MATCH2(*)
	logical   DEBUG
C
C
C	Sort array - says sort by element starting 0 bytes from
C	beginning of record, I*2 element, descending order.
	DIMENSION IS(3)
	DATA IS/0,1,.FALSE./
C       
	IF(DEBUG) THEN
	   WRITE(DBGLUN,100)
 100	   FORMAT('In vote subroutine...')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
C       
C	Initialize total pairs, NMATCH, and VOTE array
	NP=N1*N2
	NMATCH=0
	LOC=1
	DO I=1,N1
	   DO J=1,N2
	      VOTE1(1,LOC)=0
	      VOTE1(2,LOC)=I
	      VOTE1(3,LOC)=J
	      LOC=LOC+1
	   ENDDO
	ENDDO
C       
C	Cast the ballots
	DO I=1,NT1
	   J=TMATCH1(I)
	   IF(J.GT.0) THEN
	      I1=IDATA1(21,I)-1
	      I2=IDATA1(22,I)-1
	      I3=IDATA1(23,I)-1
	      J1=IDATA2(21,J)
	      J2=IDATA2(22,J)
	      J3=IDATA2(23,J)
	      K1=I1*N2+J1
	      K2=I2*N2+J2
	      K3=I3*N2+J3
	      VOTE1(1,K1)=VOTE1(1,K1)+1
	      VOTE1(1,K2)=VOTE1(1,K2)+1
	      VOTE1(1,K3)=VOTE1(1,K3)+1
	   ENDIF
	ENDDO
C       
C	Now sort 
C	CALL SORTMEM(IERROR,VOTE1,VOTE2,6,NP,1,IS)
	call sorti(vote1,3,np,-1)
C       
	IF(DEBUG) THEN
	   WRITE(DBGLUN,101)
 101	   FORMAT('Voting complete')
	   CALL CDMSTAMP(DBGLUN)
	   WRITE(DBGLUN,102)
 102	   FORMAT('Tally:'/'   Seq Seq 1 Seq 2  Vote')
	   WRITE(DBGLUN,103) (I,VOTE1(2,I),VOTE1(3,I),VOTE1(1,I),I=1,NP)
 103	   FORMAT(4I6)
	ENDIF
C       
C	Go through the array and assign matches until
C	we find 0 votes, an inconsistent match, or less than
C	1/2 the previous number of votes
	IF(DEBUG) THEN
	   WRITE(DBGLUN,104)
 104	   FORMAT('Starting assignments...')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
	NOLD=0
	DO I=1,NP
	   IF(VOTE1(1,I).LT.1.OR.2*VOTE1(1,I).LT.NOLD) GOTO 1
	   J1=VOTE1(2,I)
	   J2=VOTE1(3,I)
	   IF(MATCH1(J1).NE.0.OR.MATCH2(J2).NE.0) GOTO 1
	   NMATCH=NMATCH+1
	   MATCH1(J1)=J2
	   MATCH2(J2)=J1
	   NOLD=VOTE1(1,I)
	ENDDO
C       
C	If NMATCH<3, then not even one whole triangle matched, so
C	set no matches
 1	IF(NMATCH.LT.3) THEN
	   NMATCH=0
	   DO I=1,N1
	      MATCH1(I)=0
	   ENDDO
	   DO I=1,N2
	      MATCH2(I)=0
	   ENDDO
	ENDIF
C       
	IF(DEBUG) THEN
	   WRITE(DBGLUN,105)
 105	   FORMAT('Assignments complete')
	   CALL CDMSTAMP(DBGLUN)
	ENDIF
C       
C	Done
	RETURN
	END
	



	SUBROUTINE CDMTRIDSM(NT1,DATA1,DATA2,TMATCH1,TMATCH2,
	1                    NMATPOS,NMATNEG)
C
C
C	This subroutine is used by the coordinate matching program
C	to examine a list of matched triangles and throw out matches
C	based on disagreement between the scales of the triangles.
C
C	NT1 (I*4 input) is the number of triangles in list 1.
C
C	DATA1 (R*8 input array dimensioned 6 by NT1) contains data
C	for the triangles in list 1.  The only item that is used by
C	this routine is DATA1(5,...) which is the log of the perimeter
C	of the triangle.
C
C	DATA2 is the same for list 2 (except its dimensions are
C	6 by NT2 where NT2 does not appear in the calling sequence).
C
C	TMATCH1 (I*2 input/output array of length NT1) is such that
C	TMATCH1(I) gives the sequence number of the triangle in list
C	2 which matches triangle I in list 1.
C
C	TMATCH2 is the same for list 2 except its dimension is NT2
C	where NT2 does not appear in the calling sequence.
C
C	For both TMATCH1 and TMATCH2, if the match is an opposite sense
C	match, the pointer is negative.
C
C	NMATPOS and NMATNEG (I*4 input/output) are the number of
C	triangles which match in the same and opposite senses.
C
C	This routine looks at all matched triangles and computes
C	the log of the ratio of the size of the triangle in list 2 to that
C	in list 1.  It computes the average and standard deviation
C	of this log.  It then repeats this computation, but
C	declares that triangles whose log is more than
C	RMSLIM times the standard deviation from the mean are not
C	real matches and updates the match arrays and NMATPOS and NMATNEG
C	to reflect this fact.  Iterations are repeated a maximum
C	of ITMAX times or until no matches are thrown out.
C	ITMAX is set in the parameter statement below.
C
C	RMSLIM varies from 1 sigma to 3 sigma.  As an estimate of the
C	number of triangles to keep, take NKEEP=ABS(NMATPOS-NMATNEG)
C	as an estimate of the number to throw out take
C	NOUT=2*MIN(NMATPOS,NMATNEG).  Then if NOUT>NKEEP, which means we
C	want to throw out more than half the triangles, use 1 sigma
C	(which would throw out 1/3 for a normal dist).  If
C	.1*NKEEP<NOUT<=NKEEP which means we want to throw out at least
C	10%, use 2 sigma (throw out ~ 5% with normal dist) and if
C	NOUT<.1*NKEEP, use 3 sigma.
C
C
C
C
C	Edward J. Groth   October, 1984
C
C
C
	IMPLICIT REAL*8 (A-H,O-Z)
C
	DIMENSION DATA1(6,*)
	DIMENSION DATA2(6,*)
	INTEGER*2 TMATCH1(*)
	INTEGER*2 TMATCH2(*)

	integer   itmax,nt1,nmatpos,nmatneg,i,j,nmatch,it,nkill,nkeep,nout
C
	PARAMETER (ITMAX=40)
C
C	If less than 3 matches, obviously nothing to do
	IF(NMATPOS+NMATNEG.LT.3) RETURN
C       
C	Compute initial scale and standard deviation
	S=0.0D0
	SS=0.0D0
	NMATPOS=0
	NMATNEG=0
	DO I=1,NT1
	   IF(TMATCH1(I).NE.0) THEN
	      J=ABS(TMATCH1(I))
	      XX=DATA2(5,J)-DATA1(5,I)
	      S=S+XX
	      SS=SS+XX**2
	      IF(TMATCH1(I).GT.0) THEN
		 NMATPOS=NMATPOS+1
	      ELSE
		 NMATNEG=NMATNEG+1
	      ENDIF
	   ENDIF
	ENDDO
	NMATCH=NMATPOS+NMATNEG
	SAVE=S/NMATCH
	SDEV=SQRT(MAX(0.0D0,(SS-NMATCH*SAVE**2)/(NMATCH-1)))
C       
C	Loop over iterations
	DO IT=1,ITMAX
	   NKILL=0
	   S=0.0D0
	   SS=0.0D0
	   NKEEP=ABS(NMATPOS-NMATNEG)
	   NOUT=2*MIN(NMATPOS,NMATNEG)
	   IF(NOUT.GT.NKEEP) THEN
	      RMSLIM=1.0D0
	   ELSEIF(NOUT.GT.0.1*NKEEP) THEN
	      RMSLIM=2.0D0
	   ELSE
	      RMSLIM=3.0D0
	   ENDIF
	   STEST=RMSLIM*SDEV
	   NMATPOS=0
	   NMATNEG=0
	   DO I=1,NT1
	      IF(TMATCH1(I).NE.0) THEN
		 J=ABS(TMATCH1(I))
		 XX=DATA2(5,J)-DATA1(5,I)
		 IF(ABS(XX-SAVE).GT.STEST) THEN
		    NKILL=NKILL+1
		    TMATCH1(I)=0
		    TMATCH2(J)=0
		 ELSE
		    S=S+XX
		    SS=SS+XX**2
		    IF(TMATCH1(I).GT.0) THEN
		       NMATPOS=NMATPOS+1
		    ELSE
		       NMATNEG=NMATNEG+1
		    ENDIF
		 ENDIF
	      ENDIF
	   ENDDO
	   NMATCH=NMATPOS+NMATNEG
C       
C       If none thrown out or too few left, then punt
	   IF(NKILL.LE.0.OR.NMATCH.LT.3) RETURN
	   SAVE=S/NMATCH
	   SDEV=SQRT(MAX(0.0D0,(SS-NMATCH*SAVE**2)/(NMATCH-1)))
	ENDDO
	RETURN
	END
	
	
	
	
	SUBROUTINE CDMTRIDWS(NT1,TMATCH1,TMATCH2,SAMESENSE)
C       
C
C	This subroutine is used by the coordinate matching package
C	to go through two triangle match lists and throw out the
C	matches that have the wrong sense.
C
C
C	NT1 (I*4 input) is the number of triangles in list 1
C
C	TMATCH1 (I*2 input/output array) contains the sequence
C	numbers of the triangles in list 2 which match those in list 1.
C	This array is NT1 long.
C
C	TMATCH2 (I*2 input/output array) contains the sequence
C	numbers of the triangles in list 1 which match those in list 2.
C	This array is NT2 long, but NT2 is not needed in the calling sequence.
C
C	SAMESENSE (L*4 input) is true if same sense matches are to be kept
C	and false if opposite sense matches are to be kept.
C
C	Note that the match arrays contain 0 for unmatched triangles,
C	negative sequence numbers for opposite sense matches and 
C	positive sequence numbers for same sense matches.  This routine
C	sets all the wrong sense matches to zero and makes all the
C	correct sense matches posistive (if necessary).
C
C
C
C	Edward J. Groth   October, 1984
C
C
C
C
	INTEGER*2 TMATCH1(*)
	INTEGER*2 TMATCH2(*)
	LOGICAL SAMESENSE
	integer nt1,i,j
C       
C       
C	Split processing according to SAMESENSE
	IF(SAMESENSE) THEN
C       
C       Here we keep positive sequence numbers and zap negative ones
	   DO I=1,NT1
	      IF(TMATCH1(I).LT.0) THEN
		 J=-TMATCH1(I)
		 TMATCH1(I)=0
		 TMATCH2(J)=0
	      ENDIF
	   ENDDO
	ELSE
C       
C       Here we keep negatives (changing positives) and zap positives.
	   DO I=1,NT1
	      IF(TMATCH1(I).GT.0) THEN
		 J=TMATCH1(I)
		 TMATCH1(I)=0
		 TMATCH2(J)=0
	      ELSEIF(TMATCH1(I).LT.0) THEN
		 J=-TMATCH1(I)
		 TMATCH1(I)=J
		 TMATCH2(J)=I
	      ENDIF
	   ENDDO
	ENDIF
C       
C	Done
	RETURN
	END
	


	SUBROUTINE CDMTRIGEN(N,X,Y,VARXY,RLIMIT,NT,DATA,IDATA,WORK)
C
C
C	This subroutine is used by the coordinate match package
C	to generate a list of triangles from a list of coordinates.
C
C
C	N (I*4 input) is the number of coordinates in the list
C
C	X and Y (R*8 input arrays of length N) contain the
C	coordinates of the points.
C
C	VARXY (R*8 input) contains the variance of the X and Y
C	coordinates in the X and Y arrays.
C
C	RLIMIT (R*8 input) is the upper limit on the ratio
C	of the long side to the short side.  Triangles with
C	bigger ratios than this are not kept.
C
C	NT (I*4 output) is set to the number of triangles
C	generated <= N * (N-1) * (N-2) / 6
C
C	DATA (R*8 output array dimensioned 6 by >= NT) contains
C	the R*8 output data for the triangles.  The vertices of
C	the triangles are numbered as follows:
C	1 longest side - shortest side
C	2 shortest side - middle side
C	3 middle side - longest side
C	DATA(1,I) contains the cosine of the angle at vertex 1
C	DATA(2,I) contains the variance of the cosine
C	DATA(3,I) contains the ratio of the longest side to the shortest side.
C	DATA(4,I) contains the variance in the ratio
C	DATA(5,I) contains the log of the perimeter of the triangle.
C	DATA(6,I) is not used for R*8 data.  It is assumed that the calling
C	program has equivalenced DATA and IDATA, so DATA(6,I) corresponds
C	to IDATA(21,I) - IDATA(24,I)
C
C	IDATA (I*2 output array dimensioned 24 by >= NT) contains
C	the I*2 output data for the triangles.  IDATA(1,I) thru
C	IDATA(20,I) are not used for I*2 data.  Instead, the calling
C	program must equivalence DATA and IDATA, so these correspond
C	to DATA(1,I) thru DATA(5,I).
C	DATA(21,I) contains the sequence number of vertex 1.
C	DATA(22,I) contains the sequence number of vertex 2.
C	DATA(23,I) contains the sequence number of vertex 3.
C	DATA(24,I) contains 1 if vertices 1,2,3 are oriented CCW, and 0 for CW.
C
C	WORK is a scratch array of the same length as DATA used
C	for sorting.
C
C	Note that the output arrays are sorted in order of increasing
C	DATA(3,I) (ratio).  For the sorting to work, DATA and IDATA
C	MUST be equivalenced by the calling program.
C
C
C
C	Edward J. Groth   October, 1984
C
C	Modified 10-Dec-84 - EJG - to add ratio discard option
C       Modified 15-Feb-94 - JL  - use CERNLIB SORTD
C
C
C
	IMPLICIT REAL*8 (A-H,O-Z)
C
	DIMENSION X(*),Y(*)
	DIMENSION DATA(6,*)
	INTEGER*2 IDATA(24,*)
	DIMENSION WORK(*)
C       The following does not go through on Linux 
C       (maybe because of the IMPLICIT REAL above ?)
C	DIMENSION IS(3)
C	DATA IS/16,4,.TRUE./
	integer n,nt,i,j,k
C       
C       
C	Loop over triangles
	NT=0
	DO I=1,N-2
	   X1=X(I)
	   Y1=Y(I)
	   DO J=I+1,N-1
	      X2=X(J)
	      Y2=Y(J)
	      DX21=X2-X1
	      DY21=Y2-Y1
	      R21=SQRT(DX21**2+DY21**2)
	      DO K=J+1,N
		 NT=NT+1
		 X3=X(K)
		 Y3=Y(K)
		 DX32=X3-X2
		 DY32=Y3-Y2
		 R32=SQRT(DX32**2+DY32**2)
		 DX13=X1-X3
		 DY13=Y1-Y3
		 R13=SQRT(DX13**2+DY13**2)
C       
C       Now know the lengths of all three sides, so can
C       figure out short, middle, long - will get six cases.
		 IF(R13.GE.R32.AND.R13.GE.R21) THEN
		    IF(R32.GE.R21) THEN
C       
C       13=long, 32=middle, 21=short
		       DXL=DX13
		       DYL=DY13
		       RL=R13
		       DXS=DX21
		       DYS=DY21
		       RS=R21
		       IDATA(21,NT)=I
		       IDATA(22,NT)=J
		       IDATA(23,NT)=K
		    ELSE
C       
C       13=long, 21=middle, 32=short
		       DXL=-DX13
		       DYL=-DY13
		       RL=R13
		       DXS=-DX32
		       DYS=-DY32
		       RS=R32
		       IDATA(21,NT)=K
		       IDATA(22,NT)=J
		       IDATA(23,NT)=I
		    ENDIF
		 ELSEIF(R32.GT.R13.AND.R32.GE.R21) THEN
		    IF(R13.GE.R21) THEN
C       
C       32=long, 13=middle, 21=short
		       DXL=-DX32
		       DYL=-DY32
		       RL=R32
		       DXS=-DX21
		       DYS=-DY21
		       RS=R21
		       IDATA(21,NT)=J
		       IDATA(22,NT)=I
		       IDATA(23,NT)=K
		    ELSE
C       
C       32=long, 21=middle, 13=short
		       DXL=DX32
		       DYL=DY32
		       RL=R32
		       DXS=DX13
		       DYS=DY13
		       RS=R13
		       IDATA(21,NT)=K
		       IDATA(22,NT)=I
		       IDATA(23,NT)=J
		    ENDIF
		 ELSE
		    IF(R13.GE.R32) THEN
C       
C       21=long, 13=middle, 32=short
		       DXL=DX21
		       DYL=DY21
		       RL=R21
		       DXS=DX32
		       DYS=DY32
		       RS=R32
		       IDATA(21,NT)=J
		       IDATA(22,NT)=K
		       IDATA(23,NT)=I
		    ELSE
C       
C       21=long, 32=middle, 13=short
		       DXL=-DX21
		       DYL=-DY21
		       RL=R21
		       DXS=-DX13
		       DYS=-DY13
		       RS=R13
		       IDATA(21,NT)=I
		       IDATA(22,NT)=K
		       IDATA(23,NT)=J
		    ENDIF
		 ENDIF
C       
C       Compute ratio and keep triangle only if less than limit
		 RATTMP=RL/RS
		 IF(RATTMP.LT.RLIMIT) THEN
C       
C       Now we have the short vector which points away from vertex 1
C       and the long vector which points towards vertex 1, so compute
C       the cosine
		    COSTMP=-(DXL*DXS+DYL*DYS)/(RL*RS)
		    COSTMP=MAX(-1.0D0,MIN(1.0D0,COSTMP))
C       
C       Compute cosine squared, sine squared, and variance factor
		    COSTMP2=COSTMP*COSTMP
		    SINTMP2=1.0D0-COSTMP2
		    FACTMP=VARXY*(1.0D0/RL**2+1.0D0/RS**2-COSTMP/(RL*RS))
C       
C       Fill in array - cosine
		    DATA(1,NT)=COSTMP
C       Variance
		    DATA(2,NT)=2.0D0*SINTMP2*FACTMP+3.0D0*COSTMP2*FACTMP**2
C       Ratio
		    DATA(3,NT)=RATTMP
C       Variance
		    DATA(4,NT)=2.0D0*RATTMP**2*FACTMP
C       Perimeter
		    DATA(5,NT)=LOG(R21+R32+R13)
C       
C       Now take cross product of short into long - if negative, then
C       positive orientation (remember that short points away and
C       long points toward vertex 1).
		    CP=DXS*DYL-DXL*DYS
		    IF(CP.LE.0.0D0) THEN
		       IDATA(24,NT)=1
		    ELSE
		       IDATA(24,NT)=0
		    ENDIF
C       
C       Ratio too big, so discard
		 ELSE
		    NT=NT-1
		 ENDIF
C       
C       End of triangle loops
	      ENDDO
	   ENDDO
	ENDDO
C       
C       
C	Now sort the triangles in order of increasing ratio
C	CALL SORTMEM(IERROR,DATA,WORK,48,NT,1,IS)
	call sortd(data,6,nt,3)
C       
C	Done
	RETURN
	END
	
	
	
	SUBROUTINE CDMTRIMAT(NT1,DATA1,IDATA1,NT2,DATA2,IDATA2,
	1    NMATPOS,NMATNEG,TMATCH1,TMATCH2)
C       
C       
C       
C	This subroutine is used to match two lists of triangles.
C       
C	NT1 (I*4 input) is the number of triangles in the first list
C       
C	DATA1 (R*8 input array dimensioned 6 by NT1 and equivalenced
C	to IDATA1 by the calling program) contains the following:
C	DATA1(1,I)=cos of the angle at vertex 1 of the triangle
C	DATA1(2,I)=variance in above
C	DATA1(3,I)=ratio of longest to shortest side of the triangle
C	DATA1(4,I)=variance in above
C	DATA1(5,I)=perimeter of triangle (not used)
C	DATA1(6,I) is equivalenced to the integer variables used.
C       
C	IDATA1 (I*2 input array dimensioned 24 by NT1 and equivalenced
C	to DATA1 by the calling program) contains the following:
C	IDATA1(1,I) thru IDATA1(20,I) are equivalenced to the floating data.
C	IDATA1(21,I) thru IDATA1(23,I) contain the sequence numbers of the
C	vertices and are not used by this routine.
C	IDATA1(24,I) contains 1 for a CCW triangle and 0 for a CW triangle.
C       
C	Note that DATA1 is sorted in increasing order of DATA1(3,...)
C       
C	NT2, DATA2, and IDATA2 are the same for the second list of triangles
C       
C	NMATPOS (I*4 output) is the number of triangles found to match in
C	the same sense.
C       
C	NMATNEG (I*4 output) is the number of triangles found to match in the
C	opposite sense.
C       
C	TMATCH1 (I*2 output array of length NT1) contains 0 for triangles
C	that do not match.  For triangles that do match TMATCH1(I) contains
C	the sequence number of the triangle in list 2 which matches triangle
C	I in list 1.  The sequence number is positive if both triangles
C	have the same sense and negative if they have the opposite sense.
C       
C	TMATCH2 is the same for the second list.
C       
C       
C       
C	Edward J. Groth   October, 1984
C       
C       
C       
	IMPLICIT REAL*8 (A-H,O-Z)
C       
C       
	DIMENSION DATA1(6,*)
	DIMENSION DATA2(6,*)
	INTEGER*2 IDATA1(24,*)
	INTEGER*2 IDATA2(24,*)
	INTEGER*2 TMATCH1(*)
	INTEGER*2 TMATCH2(*)
	LOGICAL CONT,MATCH
	
	integer   nt1,nt2,nmatpos,nmatneg,i,js,is1,j,is,ismin,jmin
C       
C       
C	Go through the lists and set match arrays to zero.  Also
C	find largest ratio variance in each list.
	VMAX1=-1.0D0
	VMAX2=-1.0D0
	DO I=1,NT1
	   VMAX1=MAX(VMAX1,DATA1(4,I))
	   TMATCH1(I)=0
	ENDDO
	DO I=1,NT2
	   VMAX2=MAX(VMAX2,DATA2(4,I))
	   TMATCH2(I)=0
	ENDDO
C       
C	Zero the match counters
	NMATPOS=0
	NMATNEG=0
C       
C	Set up the maximum forward or backward we'll look in the ratio list
	RATTST=SQRT(VMAX1+VMAX2)
C       
C	Now we are going to step through list 1 and compare to elements in
C	list 2.  For each element in list 1, we start on the last element
C	in list 2 known to be within range and work along list 2 until
C	we hit an element out of range.
C	I=index in list 1
C	J=index in list 2
C	JS=starting index in list 2
C	Initialize loops
	I=1
	JS=1
	DO WHILE(I.LE.NT1.AND.JS.LE.NT2)
	   C1=DATA1(1,I)
	   VC1=DATA1(2,I)
	   R1=DATA1(3,I)
	   VR1=DATA1(4,I)
	   IS1=IDATA1(24,I)
C       
C       Is the starting element within range?
	   IF(R1-DATA2(3,JS).GT.RATTST) THEN
C       
C       No - increment starting element
	      JS=JS+1
	   ELSE
C       
C       Yes, initialize inner loop - index over second list,
C       continue, and set no match
	      J=JS
	      CONT=.TRUE.
	      MATCH=.FALSE.
C       
C       Loop over triangles in second list
	      DO WHILE (J.LE.NT2.AND.CONT)
C       
C       Are we so far along in the second list that we can't have
C       any more matches?
		 IF(DATA2(3,J)-R1.GT.RATTST) THEN
C       
C       Yes, reset continue flag
		    CONT=.FALSE.
		 ELSE
C       
C       No, see if this element will match (but only if not already)
		    IF(TMATCH2(J).EQ.0) THEN
		       COSDEL=(C1-DATA2(1,J))**2
		       COSTOL=VC1+DATA2(2,J)
		       IF(COSDEL.LE.COSTOL) THEN
			  RATDEL=(R1-DATA2(3,J))**2
			  RATTOL=VR1+DATA2(4,J)
			  IF(RATDEL.LE.RATTOL) THEN
			     DIST=COSDEL/COSTOL+RATDEL/RATTOL
			     IS=IS1+IDATA2(24,J)
			     IF(IS.EQ.2) IS=0
			     IF(MATCH) THEN
				IF(DIST.LT.DISTMIN) THEN
				   DISTMIN=DIST
				   ISMIN=IS
				   JMIN=J
				ENDIF
			     ELSE
				MATCH=.TRUE.
				DISTMIN=DIST
				ISMIN=IS
				JMIN=J
			     ENDIF
			  ENDIF
		       ENDIF
		    ENDIF
C       
C       Go to next triangle in second list
		    J=J+1
		 ENDIF
	      ENDDO
C       
C       Now we've gone through all the possible elements in the
C       second list, if we found a match, then store it
	      IF(MATCH) THEN
		 IF(ISMIN.EQ.0) THEN
C       
C       Here is a same sense match
		    NMATPOS=NMATPOS+1
		    TMATCH1(I)=JMIN
		    TMATCH2(JMIN)=I
		 ELSE
C       
C       Here is an opposite sense match
		    NMATNEG=NMATNEG+1
		    TMATCH1(I)=-JMIN
		    TMATCH2(JMIN)=-I
		 ENDIF
	      ENDIF
C       
C       Now we have finished with this first list element, so increment
	      I=I+1
C       
C       End of if statement to decide whether to advance JS or look for
C       matches with I
	   ENDIF
C       
C	End of loop over first list
	ENDDO
C       
C	Done
	RETURN
	END

	subroutine CDMSTAMP(DBGLUN)
c
c ***   Dummy routine (supposed to write time to file unit dbglun)
c
	integer*4 dbglun

	end

	subroutine plate(x1,y1,x2,y2,npnt,a,b,c,d,e,f)
	dimension x1(*),y1(*),x2(*),y2(*),xmatx(25,25),vect(25)
c       *** PLATE  fits constants between coordinates x1,y1 and x2,y2
c       ***        using normal ls solution.  x2,y2 are reference set.
	real*4 x1,y1,x2,y2,a,b,c,d,e,f,xmatx,vect
	integer npnt,i
	real*4 fn, sx1sq, sx1, sy1sq, sy1, sx1y1, sx1x2, sy1x2
	real*4 sx2, sy1y2, sx1y2, sy2
	real*4 xx1, xx2,yy1,yy2
	fn=float(npnt)
	sx1sq=0.0
	sx1=0.0
	sy1sq=0.0
	sy1=0.0
	sx1y1=0.0
	sx1x2=0.0
	sy1x2=0.0
	sx2=0.0
	sy1y2=0.0
	sx1y2=0.0
	sy2=0.0
c       *** cumulate sums
	do i=1,npnt
	   xx1=x1(i)
	   xx2=x2(i)
	   yy1=y1(i)
	   yy2=y2(i)
	   sx1sq=sx1sq+xx1**2
	   sx1=sx1+xx1
	   sy1sq=sy1sq+yy1**2
	   sy1=sy1+yy1
	   sx1y1=sx1y1+xx1*yy1
	   sx1x2=sx1x2+xx1*xx2
	   sy1x2=sy1x2+yy1*xx2
	   sx2=sx2+xx2
	   sy1y2=sy1y2+yy1*yy2
	   sx1y2=sx1y2+xx1*yy2
	   sy2=sy2+yy2
	end do
c       *** fill in matrix-vector for a b c
	xmatx(1,1)=sx1sq
	xmatx(1,2)=sx1y1
	xmatx(1,3)=sx1
	xmatx(2,1)=sx1y1
	xmatx(2,2)=sy1sq
	xmatx(2,3)=sy1
	xmatx(3,1)=sx1
	xmatx(3,2)=sy1
	xmatx(3,3)=fn
	vect(1)=sx1x2
	vect(2)=sy1x2
	vect(3)=sx2
	call solve(xmatx,vect,3)
	a=vect(1)
	b=vect(2)
	c=vect(3)
c       *** fill in matrix-vector for d e f
	xmatx(1,1)=sy1sq
	xmatx(1,2)=sx1y1
	xmatx(1,3)=sy1
	xmatx(2,1)=sx1y1
	xmatx(2,2)=sx1sq
	xmatx(2,3)=sx1
	xmatx(3,1)=sy1
	xmatx(3,2)=sx1
	xmatx(3,3)=fn
	vect(1)=sy1y2
	vect(2)=sx1y2
	vect(3)=sy2
	call solve(xmatx,vect,3)
	d=vect(1)
	e=vect(2)
	f=vect(3)
	return
	end
	
	subroutine solve(a, b, m)
c       *** gauss elimination to solve ax=b
	dimension a(25, 25), b(25)
	real*4 a,b,big,rmax,temp,pivot
	integer m,i,iu,k,ib,j,jl,ir,l
	iu = m - 1
c       *** find largest remaining term in ith column for pivot
	do i = 1, iu
	   big = 0.0
	   do 50 k = i, m
	      rmax = abs(a(k,i))
	      if (rmax - big) 50, 50, 30
 30	      big = rmax
	      l = k
c       *** check for non-zero term
 50	   continue
	   if (big .eq. 0.0) then
	      do ib = 1, m
		 b(ib) = 0.0
	      end do
	      write(unit=*, fmt=*) ' Zero determinant from SOLVE'
	      return 
	   end if
c       *** switch rows
	   if (i .eq. l) goto 120
	   do j = 1, m
	      temp = a(i,j)
	      a(i,j) = a(l,j)
 	      a(l,j) = temp
	   end do
	   temp = b(i)
	   b(i) = b(l)
c       *** pivotal reduction
	   b(l) = temp
 120	   continue
	   pivot = a(i,i)
	   jl = i + 1
	   do j = jl, m
	      temp = a(j,i) / pivot
	      b(j) = b(j) - (temp * b(i))
	      do k = i, m
c       *** back substitution for solution
		 a(j,k) = a(j,k) - (temp * a(i,k))
	      end do
	   end do
	end do
	do 500 i = 1, m
	   ir = (m + 1) - i
	   if (a(ir,ir) .eq. 0.0) goto 490
	   temp = b(ir)
	   if (ir .eq. m) goto 480
	   do  j = 2, i
	      k = (m + 2) - j
 	      temp = temp - (a(ir,k) * b(k))
	   end do
 480	   b(ir) = temp / a(ir,ir)
	   goto 500
 490	   b(ir) = 0.0
 500	continue
	return 
	end
