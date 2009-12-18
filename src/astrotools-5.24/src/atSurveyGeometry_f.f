      subroutine cobecp(
     c     longitude, latitude, coord, resolution, pixel)
      integer pixel
      character coord
      integer resolution
      real*4 longitude
      real*4 latitude

      real*4         VECTOR(3)     ! Temp. unit vector to center of pixel
      real*4         OVECTOR(3)    ! Unit vector to center of pixel
      integer*4      X,Y,FACE      ! Face x,y coordinates for quad-cube, 
                                   ! where 0,0 is in bottom right hand 
                                   ! corner of face. There are 6 faces.
      integer*4      ISTAT         ! Status
      integer*4      ll2UV  ! Routine convert lon/lat to unit vector

      istat=ll2uv(longitude, latitude, vector)

c Do this for GALACTIC output
      if (coord.eq."g") call CONV_G2E(vector,ovector)
c
c Do this for EQUATORIAL output
      if (coord.eq."q") call CONV_Q2E(vector,ovector)
c
c Do this for ECLIPTIC output
      if (coord.eq."e") then
      ovector(1)=vector(1)
      ovector(2)=vector(2)
      ovector(3)=vector(3)
      endif
      call pixel_number(ovector, resolution, pixel)
      return
      end

      subroutine cobepc(
     c     pixel, coord, resolution, longitude, latitude)

      integer pixel
      character coord
      integer*4 resolution
      real*4 longitude
      real*4 latitude

      real*4         VECTOR(3)     ! Temp. unit vector to center of pixel
      real*4         OVECTOR(3)    ! Unit vector to center of pixel
      integer*4      X,Y,FACE      ! Face x,y coordinates for quad-cube, 
                                   ! where 0,0 is in bottom right hand 
                                   ! corner of face. There are 6 faces.
      integer*4      ISTAT         ! Status
      integer*4      UV2ll         ! Routine convert unit vector to lon/lat

      call PIXEL_VECTOR(pixel,resolution,vector,x,y,face)
c

c Do this for GALACTIC output
      if (coord.eq."g") call CONV_E2G(vector,ovector)
c
c Do this for EQUATORIAL output
      if (coord.eq."q") call CONV_E2Q(vector,ovector)
c
c Do this for ECLIPTIC output
      if (coord.eq."e") then
      ovector(1)=vector(1)
      ovector(2)=vector(2)
      ovector(3)=vector(3)
      endif

      istat=UV2LL(ovector,longitude,latitude)
      
      return
      end



C*******************************************************************************
c*     UV2LL
c*
c*     Function: Convert from unit three-vector to decimal latitude and 
c*               longitude, in degrees.  Longitude is between 0 and 360.
c*
c*     PROLOGUE:
c*
c*     INPUT DATA:  three-vector
c*
c*     OUTPUT DATA: latitude and longitude in degrees
c*
c*     INVOCATION:  status=UV2LL(vector,longitude,latitude)
c*
c*     INPUT PARAMETERS:
c*
c*     Name                Type          Description
c*     ----                ----          -----------
c*
c*    vector               real*4(3)     coordinate vector
c*
c*     OUTPUT PARAMETERS:
c*
c*     Name                Type          Description
c*     ----                ----          -----------
c*
c*    longitude            real*4        in degrees
c*    latitude             real*4        in degrees
c*
c*     COMMON VARIABLES USED: none
c*
c*     SUBROUTINES CALLED:    none
c*
c*     FUNCTIONS CALLED:      none
c*
c*     PROCESSING METHOD:
c*
c*     BEGIN
c*
c*       Normalize input vector
c*       If pole:
c*           longitude = 0
c*         else
c*           calculate longitude
c*         endif
c*       Make sure longitude is positive
c*       Calculate latitude
c*       Return
c*
c*     END
c*******************************************************************************
c*
      Integer*4 Function UV2LL(vector,longitude,latitude)

c
c  Arguments:
c
      Real*4         VECTOR(3)    ! coordinate vector
      Real*4         LONGITUDE    ! in degrees
      Real*4         LATITUDE     ! in degrees
c
c  Variables:
c
      Real*4         V(3)         ! Normalized VECTOR
      Real*4         NORM_INV     ! 1 / VECTOR norm
c
c     Normalize input vector:
c
      norm_inv = sqrt(vector(1)**2 + vector(2)**2 + vector(3)**2)
      if (norm_inv .eq. 0) then
          print *,'Unit vector normalization is 0'
          uv2ll = 0
          return
        else
          norm_inv = 1.0 / norm_inv
        endif

      v(1) = vector(1) * norm_inv
      v(2) = vector(2) * norm_inv
      v(3) = vector(3) * norm_inv
c
c     Conversion:
c
      if (v(2) .eq. 0.0 .and. v(1) .eq. 0.0) then
          longitude = 0.0
        else
ccc          longitude = atan2d(v(2),v(1))
          longitude = (57.2957795130823230) * 
     c       atan2(v(2),v(1))
        endif
      if (longitude .lt. 0.0) longitude = longitude + 360.0

ccc      latitude = atan2d(v(3),sqrt(v(1)**2+v(2)**2))
      latitude = (57.2957795130823230) * 
     c     atan2(v(3),sqrt(v(1)**2+v(2)**2))
c
c     Done:
c
      uv2ll = 1
      return
      end

      Subroutine PIXEL_VECTOR(pixel,resolution,vector,jx,jy,face)
C
C    Routine to return unit vector pointing to center of pixel given pixel
C  number and resolution of the cube.  
C
C
C  Arguments:
C
      Integer*4      PIXEL         ! Pixel number
      Integer*4      RESOLUTION    ! Quad-cube resolution
      Real*4         VECTOR(3)     ! Unit vector to center of pixel
C
C  Variables:
C
      Integer*4      FACE             ! Face number
      Integer        JX,JY,IP,ID      ! Loop variables
      Real           X,Y              ! Face coordinates
      Real*4         SCALE              
      Integer*4      PIXELS_PER_FACE  ! Number of pixels on a single face
      Integer*4      FPIX             ! Pixel number within the face
      Real           XI,ETA           ! 'Database' coordinates
      Integer*4      ONE,TWO          ! Constants used to avoid integer
                                      !   overflow for large pixel numbers
      Integer*4      BIT_MASK         ! Bit mask to select the low order bit
C
ccc      Parameter (ONE = 1)
ccc      Parameter (TWO = 2)
ccc      Parameter (BIT_MASK = 1)
      one = 1
      two = 2
      bit_mask = 1

C
      scale = 2**(resolution-1) / 2.0
      pixels_per_face = two**(two*(resolution-one))
C
      face = pixel/pixels_per_face          ! Note: Integer division truncates
      fpix = pixel - face*pixels_per_face
C 
C  Break pixel number down into x and y bits:
C
      jx = 0      
      jy = 0
      ip = 1
      do while (fpix .ne. 0)
          id = iand(bit_mask,fpix)  !  = mod(fpix,2)

ccc          fpix = ishft(fpix,-1)     !  = fpix / 2
          fpix = fpix/2

          jx = id * ip + jx
          id = iand(bit_mask,fpix)

ccc          fpix = ishft(fpix,-1)
          fpix = fpix/2

          jy = id * ip + jy
          ip = ip * 2
        end do
C
      x = (jx - scale + 0.5) / scale
      y = (jy - scale + 0.5) / scale
      call FORWARD_CUBE(x,y,xi,eta)
      call XYAXIS(face,xi,eta,vector)
C
      return
      end
	SUBROUTINE FORWARD_CUBE(X,Y,XI,ETA)
	REAL*4 X,Y,XI,ETA
        real*4 xx, yy
C
C	INPUT: X,Y IN RANGE -1 TO +1 ARE DATABASE CO-ORDINATES
C
C	OUTPUT: XI, ETA IN RANGE -1 TO +1 ARE TANGENT PLANE CO-ORDINATES
C
C	BASED ON POLYNOMIAL FIT FOUND USING FCFIT.FOR
C
	REAL*4 P(28)
	DATA P/
	1	 -0.27292696, -0.07629969, -0.02819452, -0.22797056,
	2	 -0.01471565,  0.27058160,  0.54852384,  0.48051509,
	3	 -0.56800938, -0.60441560, -0.62930065, -1.74114454,
	4	  0.30803317,  1.50880086,  0.93412077,  0.25795794,
	5	  1.71547508,  0.98938102, -0.93678576, -1.41601920,
	6	 -0.63915306,  0.02584375, -0.53022337, -0.83180469,
	7	  0.08693841,  0.33887446,  0.52032238,  0.14381585/
	XX=X*X
	YY=Y*Y
	XI=X*(1.+(1.-XX)*(
	1 P(1)+XX*(P(2)+XX*(P(4)+XX*(P(7)+XX*(P(11)+XX*(P(16)+XX*P(22)))))) +
	2 YY*( P(3)+XX*(P(5)+XX*(P(8)+XX*(P(12)+XX*(P(17)+XX*P(23))))) +
	3 YY*( P(6)+XX*(P(9)+XX*(P(13)+XX*(P(18)+XX*P(24)))) +
	4 YY*( P(10)+XX*(P(14)+XX*(P(19)+XX*P(25))) + 
	5 YY*( P(15)+XX*(P(20)+XX*P(26)) +
	6 YY*( P(21)+XX*P(27) + YY*P(28))))) )))
	ETA=Y*(1.+(1.-YY)*(
	1 P(1)+YY*(P(2)+YY*(P(4)+YY*(P(7)+YY*(P(11)+YY*(P(16)+YY*P(22)))))) +
	2 XX*( P(3)+YY*(P(5)+YY*(P(8)+YY*(P(12)+YY*(P(17)+YY*P(23))))) +
	3 XX*( P(6)+YY*(P(9)+YY*(P(13)+YY*(P(18)+YY*P(24)))) +
	4 XX*( P(10)+YY*(P(14)+YY*(P(19)+YY*P(25))) + 
	5 XX*( P(15)+YY*(P(20)+YY*P(26)) +
	6 XX*( P(21)+YY*P(27) + XX*P(28))))) )))
	RETURN
	END
	SUBROUTINE XYAXIS(NFACE,XI,ETA,C)
C
C	CONVERTS FACE NUMBER NFACE (0-5) AND XI, ETA (-1. - +1.)
C	INTO A UNIT VECTOR C
C
	REAL*4 C(3),XI,ETA,XI1,ETA1,NORM
        integer*4 nface
C
C To preserve symmetry, the normalization sum must
C always have the same ordering (i.e. largest to smallest).
C
        XI1  = MAX(ABS(XI),ABS(ETA))
        ETA1 = MIN(ABS(XI),ABS(ETA))
        NORM = 1. / SQRT(1. + XI1**2 + ETA1**2)
C
	GO TO (200,210,220,230,240,250), NFACE+1
200	  C(3)=NORM				!Transform face to sphere
	  C(1)=-ETA*NORM
	  C(2)=XI*NORM
	  GO TO 300
210	  C(1)=NORM
	  C(3)=ETA*NORM
	  C(2)=XI*NORM
	  GO TO 300
220	  C(2)=NORM
	  C(3)=ETA*NORM
	  C(1)=-XI*NORM
	  GO TO 300
230	  C(1)=-NORM
	  C(3)=ETA*NORM
	  C(2)=-XI*NORM
	  GO TO 300
240	  C(2)=-NORM
	  C(3)=ETA*NORM
	  C(1)=XI*NORM
	  GO TO 300
250	  C(3)=-NORM
	  C(1)=ETA*NORM
	  C(2)=XI*NORM
	  GO TO 300
C
 300	CONTINUE
C
	RETURN
	END
      Subroutine conv_E2G(ivector,ovector)
C
C    Routine to convert ecliptic (celestial) coordinates at the given
C  epoch to galactic coordinates.  
C
C                          --------------
C
C  Galactic Coordinate System Definition (from Zombeck, "Handbook of
C  Space Astronomy and Astrophysics", page 71):
C
C         North Pole:  12:49       hours right ascension (1950.0)
C                     +27.4        degrees declination   (1950.0)
C
C    Reference Point:  17:42:26.603     hours right ascension (1950.0)
C                     -28 55' 00.45     degrees declination   (1950.0)
C
C                          --------------
C
C
C
C  Arguments:
C
      Real*4     IVECTOR(3)      ! Input coordinate vector
      Real*4     OVECTOR(3)      ! Output coordinate vector
C
C  Program variables:
C
      Real*8     T(3,3)          ! Galactic coordinate transformation matrix
      Integer*2  I,J             ! Loop variables
C 
      Data T / -0.054882486d0, -0.993821033d0, -0.096476249d0, ! 1st column
     +          0.494116468d0, -0.110993846d0,  0.862281440d0, ! 2nd column
     +         -0.867661702d0, -0.000346354d0,  0.497154957d0/ ! 3rd column
C
C     Multiply vector by transpose of transformation matrix:
C
      Do 200 I = 1,3
          ovector(i) = 0.0
          Do 100 J = 1,3
  100        ovector(i) = ovector(i) + ivector(j)*T(j,i)
  200   end do
C
      return
      end
      Subroutine conv_E2Q(ivector,ovector)
C
C    Routine to convert from ecliptic coordinates to equatorial (celestial)
C  coordinates at the given epoch.  
C
C
C  Arguments:
C
      Real*4      IVECTOR(3)     ! Input coordinate vector
      Real*4      OVECTOR(3)     ! Output coordinate vector
C
C  Program variables:
C
      Real*8      T              ! Julian centuries since 1900.0
      Real*4      EPOCH          ! Equatorial coordinate epoch
      Real*8      EPSILON        ! Obliquity of the ecliptic
C
C  Set-up:
C
      epoch=2000.
      T = (epoch - 1900.d0) / 100.d0
      epsilon = 23.452294d0 - 0.0130125d0*T - 1.63889d-6*T**2
     +           + 5.02778d-7*T**3
C
C  Conversion:
C

ccc Convert epsilon to radians:
      epsilon = epsilon / 57.2957795130823230
      ovector(1) = ivector(1)
ccc      ovector(2) = dcosd(epsilon)*ivector(2) - dsind(epsilon)*ivector(3)
ccc      ovector(3) = dcosd(epsilon)*ivector(3) + dsind(epsilon)*ivector(2)
      ovector(2) = cos(epsilon)*ivector(2) - sin(epsilon)*ivector(3)
      ovector(3) = cos(epsilon)*ivector(3) + sin(epsilon)*ivector(2)
C
      return
      end

      Subroutine CONV_G2E(ivector,ovector)
C
C    Routine to convert galactic coordinates to ecliptic (celestial) 
C  coordinates at the epoch=2000.  
C
C                          --------------
C
C  Galactic Coordinate System Definition (from Zombeck, "Handbook of
C  Space Astronomy and Astrophysics", page 71):
C
C         North Pole:  12:49       hours right ascension (1950.0)
C                     +27.4        degrees declination   (1950.0)
C
C    Reference Point:  17:42.6     hours right ascension (1950.0)
C                     -28 55'      degrees declination   (1950.0)
C
C                          --------------
C
c*     PROLOGUE:
c*
ch     Date    Version   SPR#   Programmer    Comments
ch     ----    -------   ----   ----------    --------
ch
ch                                            Pole:  12:49      
ch                                                  +27.4
ch                                          0 long:  17:42:26.603 
ch                                                  -28 55 00.45
ch
C
C
C  Arguments:
C
      Real*4     IVECTOR(3)      ! Input coordinate vector
      Real*4     OVECTOR(3)      ! Output coordinate vector
C
C  Program variables:
C
      Real*8     T(3,3)          ! Galactic coordinate transformation matrix
      Integer*2  I,J             ! Loop variables
C
      Data T / -0.054882486d0, -0.993821033d0, -0.096476249d0, ! 1st column
     +          0.494116468d0, -0.110993846d0,  0.862281440d0, ! 2nd column
     +         -0.867661702d0, -0.000346354d0,  0.497154957d0/ ! 3rd column
C
C     Multiply by transformation matrix:
C
      Do 200 I = 1,3
          ovector(i) = 0.0
          Do 100 J = 1,3
  100        ovector(i) = ovector(i) + ivector(j)*T(i,j)
  200   end do
C
      return
      end
      Subroutine CONV_Q2E(ivector,ovector)
C
C    Routine to convert equatorial coordinates at the given
C  epoch to ecliptic coordinates.  Adapted from the COBLIB routine by Dr.
C  E. Wright.
C
c*
c*     PROLOGUE:
c*
ch     Date    Version   SPR#   Programmer    Comments
ch     ----    -------   ----   ----------    --------
ch
C
C
C  Arguments:
C
      Real*4     IVECTOR(3)     ! Input coordinate vector
      Real*4     EPOCH          ! Epoch of equatorial coordinates
      Real*4     OVECTOR(3)     ! Output coordinate vector
C
C  Program variables:
C
      Real*8     T              ! Centuries since 1900.0
      Real*8     EPSILON        ! Obliquity of the ecliptic, in degrees
      Real*4     HVECTOR(3)     ! Temporary holding place for output vector
C
C  Set-up:
C
      epoch=2000.
      T = (epoch - 1900.d0) / 100.d0
      epsilon = 23.452294d0 - 0.0130125d0*T - 1.63889d-6*T**2
     +            + 5.02778d-7*T**3
C
C  Conversion:
C
ccc Convert epsilon to radians:
      epsilon = epsilon / 57.2957795130823230
      hvector(1) = ivector(1)
ccc      ovector(2) = dcosd(epsilon)*ivector(2) - dsind(epsilon)*ivector(3)
ccc      ovector(3) = dcosd(epsilon)*ivector(3) + dsind(epsilon)*ivector(2)
      hvector(2) = cos(epsilon)*ivector(2) + sin(epsilon)*ivector(3)
      hvector(3) = cos(epsilon)*ivector(3) - sin(epsilon)*ivector(2)

C
C  Move to output vector:
C
      ovector(1) = hvector(1)
      ovector(2) = hvector(2)
      ovector(3) = hvector(3)
C
      return
      end
      Subroutine PIXEL_NUMBER(vector,resolution,pixel)
C
C    Routine to return the pixel number corresponding to the input unit
C  vector in the given resolution.  Adapted from the SUPER_PIXNO routine
C  by E. Wright, this routine determines the pixel number in the maximum
C  resolution (15), then divides by the appropriate power of 4 to determine
C  the pixel number in the desired resolution.
c*
c*     PROLOGUE:
c*
ch     Date    Version   SPR#   Programmer    Comments
ch     ----    -------   ----   ----------    --------
ch
C
C
C  Arguments:
C
      Real*4        VECTOR(3)     ! Unit vector of position of interest
      Integer*4     RESOLUTION    ! Resolution of quad cube
      Integer*4     PIXEL         ! Pixel number (0 -> 6*((2**(res-1))**2)
C
C  Variables:
C
      Integer*2     FACE2             ! I*2 Cube face number (0-5)
      Integer*4     FACE4             ! I*4 Cube face number
      Real*4        X,Y               ! Coordinates on cube face
      Integer*4     IX,IY   ! Bit tables for calculating I,J
      Integer*4     I,J               ! Integer pixel coordinates
      Integer*4     IP,ID       
      Integer*4     FOUR              ! I*4 '4' - reuired to avoid integer
                                      !  overflow for large pixel numbers
      Integer*2     IL,IH,JL,JH       ! High and low 2-bytes of I & J
C
      integer*2     seven

      common/ixiy/ix(128),iy(128)

      Data IX(128) /0/
      data seven /7/



ccc      Parameter  TWO14 = 2**14, TWO28 = 2**28
ccc      Parameter  (FOUR = 4)
      integer*4 two14, two28
      two14 = 2**14
      two28 = 2**28
      four = 4

C

      if (ix(128) .eq. 0) call BIT_SET(ix,iy,128)
C
      call AXISXY(vector,face2,x,y)


      face4 = face2
      i = 2.**14 * x
      j = 2.**14 * y
      if (i .gt. 16383) i = 16383
      if (j .gt. 16383) j = 16383

ccc      ih = jishft(i,-7)
ccc      il = i - iishft(ih, seven)
ccc      jh = jishft(j,-7)
ccc      jl = j - iishft(jh, seven)
ccc      pixel = jishft(face4,28) + ix(il+1) + iy(jl+1) +
ccc     +             jishft((ix(ih+1) + iy(jh+1)),14)

      ih = ishft(i,-7)
      il = i - ishft(ih, seven)
      jh = ishft(j,-7)
      jl = j - ishft(jh, seven)
      pixel = ishft(face4,28) + ix(il+1) + iy(jl+1) +
     +             ishft((ix(ih+1) + iy(jh+1)),14)
      
C
C     'Pixel' now contains the pixel number for a resolution of 15.  To
C     convert to the desired resolution, (integer) divide by 4 to the power
C     of the difference between 15 and the given resolution:
C
ccc      pixel = jishft(pixel,-(30-2*resolution))  !  = pixel / 4**(15-resolution)
      pixel = ishft(pixel,-(30-2*resolution))  !  = pixel / 4**(15-resolution)
C
      return
      end

C*******************************************************************************
c*     ll2uv
c*
c*     Function: Convert from decimal latitude/longitude to unit three-vector
c*
c*     PROLOGUE:
c*
ch     Date    Version   SPR#   Programmer    Comments
ch     ----    -------   ----   ----------    --------
ch
c*
c*     INPUT DATA:  Decimal latitude and longitude in degrees
c*
c*     OUTPUT DATA: Unit three-vector equivalent
c*
c*     INVOCATION:  status=ll2uv(longitude,latitude,vector)
c*
c*     INPUT PARAMETERS:
c*
c*     Name                Type          Description
c*     ----                ----          -----------
c*
c*     longitude          real*4        decimal longitude
c*     latitude           real*4        decimal latitude
c*
c*     OUTPUT PARAMETERS:
c*
c*     Name                Type          Description
c*     ----                ----          -----------
c*
c*     vector             real*4(3)     output vector equivalent
c*
c*     COMMON VARIABLES USED: none
c*
c*     SUBROUTINES CALLED:    none
c*
c*     FUNCTIONS CALLED:      none
c*
c*     PROCESSING METHOD:
c*
c*     BEGIN
c*
c*       Apply standard conversion to input coordinates
c*       Return
c*
c*     END
c*******************************************************************************
c*
      Integer*4 Function ll2uv(longitude,latitude,vector)

c
c  Arguments:
c
      Real*4      LONGITUDE  ! longitude in degrees
      Real*4      LATITUDE   ! latitude in degrees
      Real*4      VECTOR(3)  ! output coordinate
      real*4      longrad, latrad ! in radians
c
c     Convert:
c
ccc Convert to radians:
      longrad = longitude / 57.2957795130823230
      latrad = latitude / 57.2957795130823230

      vector(1) = cos(latrad) * cos(longrad)
      vector(2) = cos(latrad) * sin(longrad)
      vector(3) = sin(latrad)
c
c     Done:
c
      ll2uv=1
      return
      end
      Subroutine BIT_SET(IX,IY,LENGTH)
C
C    Routine to set up the bit tables for use in the pixelization subroutines 
C  (extracted and generalized from existing routines).
C
C  Arguments:
C
      Integer*4   LENGTH       ! Number of elements in IX, IY
      Integer*4   IX(length)   ! X bits
      Integer*4   IY(length)   ! Y bits
C
C  Variables:
C
      Integer*4   i,j,k,ip,id  ! Loop variables
C
      Do 30 I = 1,length
          j = i - 1
          k = 0
          ip = 1
   10     if (j .eq. 0) then
              ix(i) = k
              iy(i) = 2*k
            else
              id = mod(j,2)
              j = j/2
              k = ip * id + k
              ip = ip * 4
              go to 10
            endif
   30   continue
C
      return
      end
	SUBROUTINE AXISXY(C,NFACE,X,Y)
C
C	CONVERTS UNIT VECTOR C INTO NFACE NUMBER (0-5) AND X,Y
C	IN RANGE 0-1
C
	integer*2 nface
	real*4    x,y
	REAL*4 C(3)
        real*4 ac1, ac2, ac3, eta, xi
	AC3=ABS(C(3))
	AC2=ABS(C(2))
	AC1=ABS(C(1))
	IF(AC3.GT.AC2) THEN
	   IF(AC3.GT.AC1) THEN
	      IF(C(3).GT.0.) GO TO 100
		GO TO 150
	   ELSE
	      IF(C(1).GT.0.) GO TO 110
		GO TO 130
	   ENDIF
	ELSE
	   IF(AC2.GT.AC1) THEN
	      IF(C(2).GT.0.) GO TO 120
		GO TO 140
	   ELSE
	      IF(C(1).GT.0.) GO TO 110
		GO TO 130
	   ENDIF
	ENDIF
100	NFACE=0
	ETA=-C(1)/C(3)
	XI=C(2)/C(3)
	GO TO 200
110	NFACE=1
	XI=C(2)/C(1)
	ETA=C(3)/C(1)
	GO TO 200
120	NFACE=2
	ETA=C(3)/C(2)
	XI=-C(1)/C(2)
	GO TO 200
130	NFACE=3
	ETA=-C(3)/C(1)
	XI=C(2)/C(1)
	GO TO 200
140	NFACE=4
	ETA=-C(3)/C(2)
	XI=-C(1)/C(2)
	GO TO 200
150	NFACE=5
	ETA=-C(1)/C(3)
	XI=-C(2)/C(3)
	GO TO 200
C
200	CALL INCUBE(XI,ETA,X,Y)
	X=(X+1.)/2.
	Y=(Y+1.)/2.
	RETURN
	END
	SUBROUTINE INCUBE(ALPHA,BETA,X,Y)
C
c*     PROLOGUE:
c*
ch     Date    Version   SPR#   Programmer    Comments
ch     ----    -------   ----   ----------    --------
ch
C
	REAL*4 ALPHA,BETA
        Real*4 GSTAR_1,M_G,C_COMB
        real*4 aa, bb, a4, b4, onmaa, onmbb, x, y

ccc	PARAMETER GSTAR=1.37484847732,G=-0.13161671474,
ccc	1   M=0.004869491981,W1=-0.159596235474,C00=0.141189631152,
ccc	2   C10=0.0809701286525,C01=-0.281528535557,C11=0.15384112876,
ccc	3   C20=-0.178251207466,C02=0.106959469314,D0=0.0759196200467,
ccc	4   D1=-0.0217762490699
ccc	PARAMETER R0=0.577350269
        real*4  gstar, g, m, w1, c00, c10, c01, c11, c20, c02, d0, d1

	GSTAR=1.37484847732
        G=-0.13161671474
	M=0.004869491981
        W1=-0.159596235474
        C00=0.141189631152
	C10=0.0809701286525
        C01=-0.281528535557
        C11=0.15384112876
	C20=-0.178251207466
        C02=0.106959469314
        D0=0.0759196200467
	D1=-0.0217762490699
ccc	R0=0.577350269

	AA=ALPHA**2
	BB=BETA**2
	A4=AA**2
	B4=BB**2
	ONMAA=1.-AA
	ONMBB=1.-BB

        gstar_1 = 1. - gstar
        m_g     = m - g
        c_comb  = c00 + c11*aa*bb

	X = ALPHA * 
     +       (GSTAR + 
     +        AA * gstar_1 + 
     +        ONMAA * (BB * (G + (m_g)*AA + 
     +	                     ONMBB * (c_comb + C10*AA + C01*BB +
     +                                C20*A4 + C02*B4)) +
     +	               AA * (W1 - ONMAA*(D0 + D1*AA))))

	Y = BETA * 
     +       (GSTAR + 
     +        BB * gstar_1 + 
     +        ONMBB * (AA * (G + (m_g)*BB +
     +                       ONMAA * (c_comb + C10*BB + C01*AA +
     +                                C20*B4+C02*A4)) + 
     +                 BB * (W1 - ONMBB*(D0 + D1*BB))))
	RETURN
	END
