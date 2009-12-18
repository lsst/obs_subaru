c===============================================================================
c
c
c	Code sent by M. Bershady at Penn State
c
c	renamed from bces_regress.f to bces_regress_f.f
c	catted the files bootspbec.f and ran3.f to the end of this file.
c
c	The umma,ed top-level routine in bces_regress_f.f has been
c	rewritten in c in atLinearFit.c, as bcesFit.
c	That routine calls the subroutines in this file.
c	The top level routine has been commented out.
c
c					James Annis, June 1996
c
c        variables declared implicity now declared explicity to eliminate
c        compiler warnings and trouble with IRIX 6.x  Ron Kollgaard May 1997
c
c===============================================================================

c c     bces_regress.f   -  Bi-variate Correlated Errors and Scatter
c c
c c     Calculate linear regression model when both variables
c c     are subject to known errors, and there is intrinsic scatter.
c c     Errors may be correlated or UNcorrelated.
c c
c c     BCES = bivariate, correlate errors and scatter
c c
c c     Authors:
c c     Michael Akritas (Penn State) statistical analysis
c c     Tina Bird (Kansas) code written 
c c     Matthew A. Bershady (Penn State) code expanded/modified
c c     
c c     Modification History:
c c     Feb 2  1995   TB      Initial Release    
c c     Mar 26 1995   MAB     reformat; add orthogonal regression; add variances
c c                           for a3 and a4; generalize for off-diagonal
c c                           (covariant) errors (but no inputs yet);
c c                           all real-> double precision; add bootstrap errors;
c c                           add input of covariant errors - required.
c c     
c       parameter (nmax=1000,nmod=4)
c       implicit double precision (x,y,z,c)
c       dimension y1(nmax),y1err(nmax),y2(nmax),y2err(nmax),cerr(nmax)
c       dimension zeta(nmax,nmod),xi(nmax,nmod)
c       dimension y1sim(nmax),y1errsim(nmax),y2sim(nmax)
c       dimension y2errsim(nmax),cerrsim(nmax)
c       integer npts,nmax,nsim,iseed
c       double precision a(4),b(4),avar(4),bvar(4),bvar_ifab(4)
c       double precision asim(4),bsim(4),avarsim(4),bvarsim(4)
c       double precision asum(4),bsum(4),aavg(4),bavg(4)
c       double precision assum(4),bssum(4),sda(4),sdb(4)
c       double precision bvar_ifabsim(4),sdtest,rnsim
c       character infile*50,outfile*50,model(4)*15
c c
c       data model / '      BCES(Y|X)', '      BCES(X|Y)',
c      &      '  BCES Bisector', 'BCES Orthogonal' /
c c
c c     inputs
c c
c       write(6,*) 'Name of input file [x ex y ey exy]:'
c       read(5,*) infile
c       write(6,*) 'Name of output file:'
c       read(5,*) outfile
c       write(6,*) 'Number of bootstrap simulations: '
c       read(5,*) nsim
c       write(6,*) 'Integer seed for random number generator: '
c       read(5,*) iseed
c       open(unit=7,file=infile,status='old')
c       rewind(7)
c       open(unit=8,file=outfile,status='new')
c       rewind(8)
c c     
c c     read in data and confidence intervals
c c     
c       npts = 0
c       do 10 i=1,1000
c          read(7,*,end=10) y1(i),y1err(i),y2(i),y2err(i),cerr(i)
c          npts = npts +1
c  10   continue
c       write(6,*) 'Finished reading file.  NPTS =',npts
c c
c c     Calculate nominal fits
c c
c       call bess ( nmax,nmod,npts,y1,y1err,y2,y2err,cerr,
c      &     a,b,avar,bvar,bvar_ifab,xi,zeta )
c c
c c     Make Bootstrap simulated datasets, and compute averages
c C     and standard deviations of regression coefficients
c C
c       rnsim = dfloat(nsim)
c       do 20 i=1,4
c          asum(i) = 0.0
c          assum(i) = 0.0
c          bsum(i) = 0.0
c          bssum(i) = 0.0
c          sda(i) = 0.0
c          sdb(i) = 0.0
c  20   continue
c       do 30  i=1,nsim
c          if(i.eq.1) iseed = -1*iabs(iseed)
c          call bootspbec(nmax,npts,iseed,y1,y1err,y2,y2err,cerr,
c      &        y1sim,y1errsim,y2sim,y2errsim,cerrsim)
c          call bess ( nmax,nmod,npts,y1sim,y1errsim,y2sim,
c      &        y2errsim,cerrsim,asim,bsim,avarsim,bvarsim,
c      &        bvar_ifabsim,xi,zeta )
c          do 35 j=1,4
c             asum(j) = asum(j) + asim(j)
c             assum(j) = assum(j) + asim(j)**2
c             bsum(j) = bsum(j) + bsim(j)
c             bssum(j) = bssum(j) + bsim(j)**2
c  35      continue
c  30   continue
c       do 40 i=1,4
c          aavg(i) = asum(i)/rnsim
c          sdtest = assum(i) - rnsim*aavg(i)**2
c          if(sdtest.gt.0.0) sda(i) = dsqrt(sdtest/(rnsim-1.0))
c          bavg(i) = bsum(i)/rnsim
c          sdtest = bssum(i) - rnsim*bavg(i)**2
c          if(sdtest.gt.0.0) sdb(i) = dsqrt(sdtest/(rnsim-1.0))
c  40   continue
c c      
c c     write out the data
c c      
c       write(*,99) 'fit','B','err(B)','A','err(A)'
c       write(8,99) 'fit','B','err(B)','A','err(A)'
c       do 50 i=1,4
c          write(*,100) model(i),b(i),sqrt(bvar(i)),a(i),sqrt(avar(i))
c          write(*,100) 'bootstrap',bavg(i),sdb(i),aavg(i),sda(i)
c          write(*,*) ' '
c          write(8,100) model(i),b(i),sqrt(bvar(i)),a(i),sqrt(avar(i))
c          write(8,100) 'bootstrap',bavg(i),sdb(i),aavg(i),sda(i)
c          write(8,*) ' '
c  50   continue
c       write(8,*) ' '
c       write(8,*) 'Comparison of variances for B:'
c       write(8,*) ' '
c       write(8,99) 'fit','err(B)','err(B) IFAB'
c       write(8,100) 'OLS BISECTOR',sqrt(bvar(3)),
c      &     sqrt(bvar_ifab(3))
c       write(8,100) 'Orthogonal',sqrt(bvar(4)),
c      &     sqrt(bvar_ifab(4))
c  99   format (5a12)
c  100  format (a15,4e12.3)
c       close (7)
c       close (8)
c c     
c       stop
c       end
c c     
c -----------------------------------------------------------------------
      subroutine bess ( nmax,nmod,npts,y1,y1err,y2,y2err,cerr,
     &     a,b,avar,bvar,bvar_ifab,xi,zeta )
c -----------------------------------------------------------------------
c     Do the entire regression calculation for 4 slopes:
c     OLS(Y|X), OLS(X|Y), bisector, orthogonal
c
C****     implicit double precision (c,x,y,z)
      integer i,npts,nmax,nmod
c Chris S. made the next two lines be double precision
      double precision y1(nmax),y1err(nmax),y2(nmax),y2err(nmax)
      double precision cerr(nmax),xi(nmax,nmod),zeta(nmax,nmod)
      double precision a(4),b(4),avar(4),bvar(4),bvar_ifab(4)
      double precision y1av,y2av,y1var,y2var,y1stddev,y2stddev
      double precision sig11var,sig22var,sig12var,sign
      double precision zetaav(4),zetadisp(4),xidisp(4),xiav(4)
      double precision covar_y1y2,covb1b2
c     
c     calculate sigma's for datapoints using length of conf. intervals
c
      sig11var = 0.
      sig22var = 0.
      sig12var = 0.
      do 2 i=1,npts
         sig11var = sig11var + y1err(i)**2
         sig22var = sig22var + y2err(i)**2
         sig12var = sig12var + cerr(i)**2
 2    continue
      sig11var = sig11var/real(npts)
      sig22var = sig22var/real(npts)
      sig12var = sig12var/real(npts)
c     
c     calculate means and variances
c     
      call class(y1,npts,y1av,y1stddev)
      call class(y2,npts,y2av,y2stddev)
      y1var = y1stddev**2
      y2var = y2stddev**2
      covar_y1y2 = 0.
      do 5 i=1,npts
         covar_y1y2 = (y1(i)-y1av)*(y2(i)-y2av) + covar_y1y2
 5    continue
      covar_y1y2 = covar_y1y2/real(npts)
c      
c     compute the regression slopes for OLS(Y2|Y1), OLS(Y1|Y2),
c     bisector, and orthogonal.
c      
      b(1) = (covar_y1y2 - sig12var)/(y1var - sig11var)
      b(2) = (y2var - sig22var)/(covar_y1y2 - sig12var)
      b(3) = ( b(1)*b(2) - 1.0 
     &     + sqrt((1.0 + b(1)**2)*(1.0 + b(2)**2)) ) /
     &     (b(1)+b(2))
      if ( covar_y1y2.lt.0. ) then
         sign = -1.
      else
         sign = 1.
      endif
      b(4) = 0.5*((b(2)-(1./b(1))) 
     &     + sign * sqrt(4.+(b(2)-(1./b(1)))**2))
c      
c     compute intercepts for above 4 cases:
c      
      do 10 i=1,4
         a(i) = y2av - b(i)*y1av
 10   continue
c      
c     set up variables to calculate standard deviations of slope
c     and intercept (MAB renamed: chi -> xi, xi -> zeta to be consistent 
c     with text.)
c
      do 15 i=1,npts
         xi(i,1) = ( (y1(i)-y1av) * (y2(i)-b(1)*y1(i)-a(1)) + 
     &        b(1)*y1err(i)**2 ) / (y1var-sig11var)
         zeta(i,1) = y2(i) - b(1)*y1(i) - y1av*xi(i,1)
         xi(i,2) = ( (y2(i)-y2av) * (y2(i)-b(2)*y1(i)-a(2)) - 
     &        y2err(i)**2 ) / covar_y1y2
         zeta(i,2) = y2(i) - b(2)*y1(i) - y1av*xi(i,2)
         xi(i,3) = xi(i,1) * 
     &        (1.+b(2)**2)*b(3) / 
     &        ((b(1)+b(2))*sqrt((1.+b(1)**2)*(1.+b(2)**2))) + 
     &        xi(i,2) * 
     &        (1.+b(1)**2)*b(3) /
     &        ((b(1)+b(2))*sqrt((1.+b(1)**2)*(1.+b(2)**2)))
         zeta(i,3) = y2(i) - b(3)*y1(i) - y1av*xi(i,3)
         xi(i,4) = xi(i,1) * 
     &        b(4)/(b(1)**2*sqrt(4.+(b(2)-1./b(1))**2)) +
     &        xi(i,2)*b(4)/sqrt(4.+(b(2)-1./b(1))**2)
         zeta(i,4) = y2(i) - b(4)*y1(i) - y1av*xi(i,4)
 15   continue
c      
c     calculate variance for all a and b
c
      do 20 i=1,4
         call nclass(nmax,nmod,npts,i,xi,xiav,xidisp)
         call nclass(nmax,nmod,npts,i,zeta,zetaav,zetadisp)
         bvar(i) = xidisp(i)**2/real(npts)
         avar(i) = zetadisp(i)**2/real(npts)
 20   continue
c      
c     alternate slope variances for b3 and b4 via IFAB formulae;
c     requires calculating first covariance for b1,b2
c
      covb1b2 = xidisp(1)*xidisp(2)/real(npts)
      bvar_ifab(3) = ( b(3)**2 / 
     &     (((b(1)+b(2))**2)*(1.+b(1)**2)*(1.+b(2)**2)) ) *
     &     ( (1.+b(2)**2)**2*bvar(1) + (1.+b(1)**2)**2*bvar(2) + 
     &     2.*(1.+b(1)**2)*(1.+b(2)**2)*covb1b2 )
      bvar_ifab(4) = ( b(4)**2 / 
     &     (4.*b(1)**2 + (b(1)*b(2)-1.)**2 ) ) *
     &     ( bvar(1)/b(1)**2 + 2.*covb1b2 + b(1)**2*bvar(2) )
c
      return
      end
c -----------------------------------------------------------------------
      subroutine class (x,n,xav,xstddev)
c -----------------------------------------------------------------------
c     Calculate mean and standard deviation of the array X of N numbers.
c     
C*****      implicit double precision x
      integer n
      double precision x(n)
      integer i
      double precision xav,xstddev
c
      xav = 0.0
      xstddev = 0.0
      
      do 10 i=1,n
         xav = xav + x(i)
 10   continue
      xav = xav/dfloat(n)
c      
      do 20 i=1,n
         xstddev = (x(i) - xav)**2 + xstddev
 20   continue
      xstddev = sqrt(xstddev/dfloat(n))
c      
      return
      end
c -----------------------------------------------------------------------
      subroutine nclass (nmax,nmod,npts,i,x,xav,xstddev)
c -----------------------------------------------------------------------
c     Calculate mean and standard deviation of the array X of N numbers.
c     
C*****      implicit double precision (x)
      integer i,j,nmax,nmod,npts
      double precision x(nmax,nmod),xav(nmod),xstddev(nmod)
c
      xav(i) = 0.0
      xstddev(i) = 0.0
c
      do 10 j=1,npts
         xav(i) = xav(i) + x(j,i)
 10   continue
      xav(i) = xav(i)/dfloat(npts)
c      
      do 20 j=1,npts
         xstddev(i) = (x(j,i) - xav(i))**2 + xstddev(i)
 20   continue
      xstddev(i) = sqrt(xstddev(i)/dfloat(npts))
c
      return
      end

c ------------------------------------------------------------------------------
      subroutine bootspbec (nmax,n,iseed,x,xerr,y,yerr,cerr,
     &     xboot,xerrboot,yboot,yerrboot,cerrboot)
c ------------------------------------------------------------------------------
c     Constructs Monte Carlo simulated data set using the
c     Bootstrap algorithm.                               
c                                                        
c     Random number generator is real function ran3.f, from
c     Numerical Recipes (an adaptation of Knuth's algorithm).
c
C*****      implicit double precision (x,y,c)
      integer iseed
      integer i,j,n,nmax
      double precision x(nmax),xerr(nmax),y(nmax),yerr(nmax),cerr(nmax),
     &     xboot(nmax),xerrboot(nmax),yboot(nmax),yerrboot(nmax),
     &     cerrboot(nmax)
      real*4 ran3
      external ran3
c     
      do 10 i = 1,n
         j = int(ran3(iseed)*float(n) + 1.0)
         xboot(i) = x(j)
         xerrboot(i) = xerr(j)
         yboot(i) = y(j)
         yerrboot(i) = yerr(j)
         cerrboot(i) = cerr(j)
 10   continue
c     
      return
      end
      
c ------------------------------------------------------------------------------
      FUNCTION RAN3(IDUM)
C         IMPLICIT REAL*4(M)
C         PARAMETER (MBIG=4000000.,MSEED=1618033.,MZ=0.,FAC=2.5E-7)
c lower case are changes by jta to get this to compile
C***      implicit integer (i,k,m)
C***      implicit double precision (f)
C***      implicit real (r)
      integer i, k, iff, ii, idum, inext, inextp, ma, mj, mk 
      integer mbig, mseed, mz
      real ran3
      double precision fac
      PARAMETER (MBIG=1000000000,MSEED=161803398,MZ=0,FAC=1.E-9)
      DIMENSION MA(55)
c
      save
c
      DATA IFF /0/
      IF(IDUM.LT.0.OR.IFF.EQ.0)THEN
        IFF=1
        MJ=MSEED-IABS(IDUM)
        MJ=MOD(MJ,MBIG)
        MA(55)=MJ
        MK=1
        DO 11 I=1,54
          II=MOD(21*I,55)
          MA(II)=MK
          MK=MJ-MK
          IF(MK.LT.MZ)MK=MK+MBIG
          MJ=MA(II)
11      CONTINUE
        DO 13 K=1,4
          DO 12 I=1,55
            MA(I)=MA(I)-MA(1+MOD(I+30,55))
            IF(MA(I).LT.MZ)MA(I)=MA(I)+MBIG
12        CONTINUE
13      CONTINUE
        INEXT=0
        INEXTP=31
        IDUM=1
      ENDIF
      INEXT=INEXT+1
      IF(INEXT.EQ.56)INEXT=1
      INEXTP=INEXTP+1
      IF(INEXTP.EQ.56)INEXTP=1
      MJ=MA(INEXT)-MA(INEXTP)
      IF(MJ.LT.MZ)MJ=MJ+MBIG
      MA(INEXT)=MJ
      RAN3=MJ*FAC
      RETURN
      END

