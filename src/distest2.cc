//--------------------------------------------------
//distest2.cc
//
//Last Update 2013/03/15
//--------------------------------------------------
#include<iostream>
#include<vector>
#include<cmath>

using namespace std;
std::vector< double > InvPROJECTION(std::vector< double >CRVAL,std::vector< double >POS);
std::vector< double >    PROJECTION(std::vector< double >CRVAL,std::vector< double >POS);
std::vector< double > CALC_IMPIX(std::vector< double >POS);
std::vector< double > CALC_IMWLD(std::vector< double >POS);
std::vector< double > CALC_IMPIX_SIM(std::vector< double >POS);
std::vector< double > CALC_IMWLD_SIM(std::vector< double >POS);
std::vector< double > MAKE_Vdouble(){
	std::vector< double > A(2);
	return A;
}
std::vector< std::vector< double > > MAKE_VVdouble(){
	std::vector< std::vector< double > > A;
	return A;
}
std::vector< std::vector< double >  > CALC_RADEC(std::vector< double > CRVAL,std::vector< double > CRPIX,std::vector< std::vector< double >  > POSITION){
	std::vector< std::vector< double >  > DIST_RADEC;
	std::vector< double > RADEC(2);
	std::vector< double > POS(2);
	std::vector< double > POS2(2);
	int i;
	for(i=0;i<POSITION.size();i++){
		POS[0]=POSITION[i][0]-CRPIX[0];
		POS[1]=POSITION[i][1]-CRPIX[1];
		POS2=CALC_IMWLD(POS);
		RADEC=InvPROJECTION(CRVAL,POS2);
		DIST_RADEC.push_back(RADEC);
	}

	return DIST_RADEC;
}
std::vector< std::vector< double >  > CALC_GLOBL(std::vector< double > CRVAL,std::vector< double > CRPIX,std::vector< std::vector< double >  > POSITION){
	std::vector< std::vector< double >  > DIST_GLOBL;
	std::vector< double > GLOBL(2);
	std::vector< double > POS(2);
	std::vector< double > POS2(2);
	int i;
	for(i=0;i<POSITION.size();i++){
		POS=PROJECTION(CRVAL,POSITION[i]);
		POS2=CALC_IMPIX(POS);
		GLOBL[0]=POS2[0]+CRPIX[0];
		GLOBL[1]=POS2[1]+CRPIX[1];
		DIST_GLOBL.push_back(GLOBL);
	}

	return DIST_GLOBL;
}
std::vector< std::vector< double >  > CALC_RADEC_SIM(std::vector< double > CRVAL,std::vector< double > CRPIX,std::vector< std::vector< double >  > POSITION){
	std::vector< std::vector< double >  > DIST_RADEC;
	std::vector< double > RADEC(2);
	std::vector< double > POS(2);
	std::vector< double > POS2(2);
	int i;
	for(i=0;i<POSITION.size();i++){
		POS[0]=POSITION[i][0]-CRPIX[0];
		POS[1]=POSITION[i][1]-CRPIX[1];
		POS2=CALC_IMWLD_SIM(POS);
		RADEC=InvPROJECTION(CRVAL,POS2);
		DIST_RADEC.push_back(RADEC);
	}

	return DIST_RADEC;
}
std::vector< std::vector< double >  > CALC_GLOBL_SIM(std::vector< double > CRVAL,std::vector< double > CRPIX,std::vector< std::vector< double >  > POSITION){
	std::vector< std::vector< double >  > DIST_GLOBL;
	std::vector< double > GLOBL(2);
	std::vector< double > POS(2);
	std::vector< double > POS2(2);
	int i;
	for(i=0;i<POSITION.size();i++){
		POS=PROJECTION(CRVAL,POSITION[i]);
		POS2=CALC_IMPIX_SIM(POS);
		GLOBL[0]=POS2[0]+CRPIX[0];
		GLOBL[1]=POS2[1]+CRPIX[1];
		DIST_GLOBL.push_back(GLOBL);
	}

	return DIST_GLOBL;
}
//------

#define PI (4*atan(1.0))
#define LP 180
std::vector< double > InvPROJECTION(std::vector< double >PPOINT,std::vector< double >Pdeg){
	std::vector< double >Cdeg(2);
    double NRAD[2];//native psi,theta (RAD)
	
    NRAD[0]=atan2(Pdeg[0],-Pdeg[1]);	
    NRAD[1]=atan(180/PI/sqrt(Pdeg[0]*Pdeg[0]+Pdeg[1]*Pdeg[1]));
    if(NRAD[0]<0)
    NRAD[0]+=2*PI;                                     
    if(NRAD[0]>2*PI)
    NRAD[0]-=2*PI;

    Cdeg[1]=180/PI*asin(sin(NRAD[1])*sin(PPOINT[1]/180*PI)+cos(NRAD[1])*cos(NRAD[0]-LP/180*PI)*cos(PPOINT[1]/180*PI));//-90deg - 90deg
    Cdeg[0]=180/PI*atan2(-cos(NRAD[1])*sin(NRAD[0]-LP/180*PI),sin(NRAD[1])*cos(PPOINT[1]/180*PI)-cos(NRAD[1])*cos(NRAD[0]-LP/180*PI)*sin(PPOINT[1]/180*PI))+PPOINT[0];

	return Cdeg;
}
std::vector< double > PROJECTION(std::vector< double >PPOINT,std::vector< double >Cdeg){
	std::vector< double >Pdeg(2);
    double NRAD[2];
	
    NRAD[1]=asin(sin(Cdeg[1]/180*PI)*sin(PPOINT[1]/180*PI)+cos(Cdeg[1]/180*PI)*cos(PPOINT[1]/180*PI)*cos(Cdeg[0]/180*PI-PPOINT[0]/180*PI));
    NRAD[0]=atan2(-cos(Cdeg[1]/180*PI)*sin(Cdeg[0]/180*PI-PPOINT[0]/180*PI),sin(Cdeg[1]/180*PI)*cos(PPOINT[1]/180*PI)-cos(Cdeg[1]/180*PI)*sin(PPOINT[1]/180*PI)*cos(Cdeg[0]/180*PI-PPOINT[0]/180*PI))+LP/180*PI;

    Pdeg[0]= 180/PI/tan(NRAD[1])*sin(NRAD[0]);//+PPOINT[0];
    Pdeg[1]=-180/PI/tan(NRAD[1])*cos(NRAD[0]);//+PPOINT[1];

	return Pdeg;
}
#undef LP
#undef PI
std::vector< double > CALC_IMPIX(std::vector< double >POS){
	std::vector< double > POS2(2);
	double COEF[2][10][10]={0};
COEF[0][ 0 ][ 0 ]= 0.00357457263523;
COEF[0][ 0 ][ 1 ]= -21330.9528185;
COEF[0][ 0 ][ 2 ]= 3.08718040412;
COEF[0][ 0 ][ 3 ]= -1013.77500367;
COEF[0][ 0 ][ 4 ]= -10.8343950476;
COEF[0][ 0 ][ 5 ]= -277.243768186;
COEF[0][ 0 ][ 6 ]= 21.0964905942;
COEF[0][ 0 ][ 7 ]= 9.88489720833;
COEF[0][ 0 ][ 8 ]= -5.72612584238;
COEF[0][ 0 ][ 9 ]= -250.922869767;
COEF[0][ 1 ][ 0 ]= 25.7532984084;
COEF[0][ 1 ][ 1 ]= -2.16988019757;
COEF[0][ 1 ][ 2 ]= 2.02295245558;
COEF[0][ 1 ][ 3 ]= 3.75735556815;
COEF[0][ 1 ][ 4 ]= -44.2382793466;
COEF[0][ 1 ][ 5 ]= 9.02247110084;
COEF[0][ 1 ][ 6 ]= 214.679970545;
COEF[0][ 1 ][ 7 ]= -6.75635797434;
COEF[0][ 1 ][ 8 ]= -239.864551239;
COEF[0][ 2 ][ 0 ]= 0.13257860942;
COEF[0][ 2 ][ 1 ]= -1069.00385308;
COEF[0][ 2 ][ 2 ]= 15.8525209771;
COEF[0][ 2 ][ 3 ]= -141.267330235;
COEF[0][ 2 ][ 4 ]= -6.00985679047;
COEF[0][ 2 ][ 5 ]= -1077.64407346;
COEF[0][ 2 ][ 6 ]= -19.394034333;
COEF[0][ 2 ][ 7 ]= -30.1809714841;
COEF[0][ 3 ][ 0 ]= 4.52289632003;
COEF[0][ 3 ][ 1 ]= 10.3038742469;
COEF[0][ 3 ][ 2 ]= -7.13546447828;
COEF[0][ 3 ][ 3 ]= -24.3263266108;
COEF[0][ 3 ][ 4 ]= -4.5525589894;
COEF[0][ 3 ][ 5 ]= -13.1607850362;
COEF[0][ 3 ][ 6 ]= -119.278439255;
COEF[0][ 4 ][ 0 ]= 4.71050726766;
COEF[0][ 4 ][ 1 ]= 82.1869681766;
COEF[0][ 4 ][ 2 ]= -111.923145271;
COEF[0][ 4 ][ 3 ]= -1748.53068979;
COEF[0][ 4 ][ 4 ]= 145.688876538;
COEF[0][ 4 ][ 5 ]= 779.482006784;
COEF[0][ 5 ][ 0 ]= -35.5883523795;
COEF[0][ 5 ][ 1 ]= -18.3776735173;
COEF[0][ 5 ][ 2 ]= 56.6704610735;
COEF[0][ 5 ][ 3 ]= 75.8898286658;
COEF[0][ 5 ][ 4 ]= 75.0114877288;
COEF[0][ 6 ][ 0 ]= -11.9280635128;
COEF[0][ 6 ][ 1 ]= -778.28864704;
COEF[0][ 6 ][ 2 ]= 114.732852;
COEF[0][ 6 ][ 3 ]= 893.535935697;
COEF[0][ 7 ][ 0 ]= 86.5069838486;
COEF[0][ 7 ][ 1 ]= 3.83060601673;
COEF[0][ 7 ][ 2 ]= -72.4873960965;
COEF[0][ 8 ][ 0 ]= 9.18899290553;
COEF[0][ 8 ][ 1 ]= 316.242800729;
COEF[0][ 9 ][ 0 ]= -64.084580894;
COEF[1][ 0 ][ 0 ]= -0.00235831460096;
COEF[1][ 0 ][ 1 ]= -26.4052259416;
COEF[1][ 0 ][ 2 ]= 0.73371695177;
COEF[1][ 0 ][ 3 ]= 14.6794968215;
COEF[1][ 0 ][ 4 ]= 4.14042441126;
COEF[1][ 0 ][ 5 ]= -78.1419088649;
COEF[1][ 0 ][ 6 ]= -5.86293689607;
COEF[1][ 0 ][ 7 ]= 131.97269948;
COEF[1][ 0 ][ 8 ]= 5.75971239845;
COEF[1][ 0 ][ 9 ]= -79.4870542457;
COEF[1][ 1 ][ 0 ]= -21329.6549608;
COEF[1][ 1 ][ 1 ]= -2.62154199772;
COEF[1][ 1 ][ 2 ]= -1051.73872845;
COEF[1][ 1 ][ 3 ]= 48.7841128949;
COEF[1][ 1 ][ 4 ]= -88.2890088963;
COEF[1][ 1 ][ 5 ]= -154.983086355;
COEF[1][ 1 ][ 6 ]= -415.653610738;
COEF[1][ 1 ][ 7 ]= 146.593551721;
COEF[1][ 1 ][ 8 ]= 100.33610647;
COEF[1][ 2 ][ 0 ]= -0.593182608153;
COEF[1][ 2 ][ 1 ]= -26.4596683867;
COEF[1][ 2 ][ 2 ]= -11.3732220688;
COEF[1][ 2 ][ 3 ]= 82.6717682811;
COEF[1][ 2 ][ 4 ]= 89.6191164596;
COEF[1][ 2 ][ 5 ]= -181.472568836;
COEF[1][ 2 ][ 6 ]= -119.82142646;
COEF[1][ 2 ][ 7 ]= 239.057374696;
COEF[1][ 3 ][ 0 ]= -1045.74410389;
COEF[1][ 3 ][ 1 ]= 18.2702974641;
COEF[1][ 3 ][ 2 ]= -193.461918882;
COEF[1][ 3 ][ 3 ]= -94.4649487377;
COEF[1][ 3 ][ 4 ]= -1001.3883232;
COEF[1][ 3 ][ 5 ]= 130.851765068;
COEF[1][ 3 ][ 6 ]= -13.8994011879;
COEF[1][ 4 ][ 0 ]= 9.41047196199;
COEF[1][ 4 ][ 1 ]= 174.945021985;
COEF[1][ 4 ][ 2 ]= 51.6651053708;
COEF[1][ 4 ][ 3 ]= -379.748903334;
COEF[1][ 4 ][ 4 ]= -90.0410413519;
COEF[1][ 4 ][ 5 ]= 130.581101026;
COEF[1][ 5 ][ 0 ]= -124.785370861;
COEF[1][ 5 ][ 1 ]= -22.5094209611;
COEF[1][ 5 ][ 2 ]= -1077.34056248;
COEF[1][ 5 ][ 3 ]= 72.9324401417;
COEF[1][ 5 ][ 4 ]= 68.2834257398;
COEF[1][ 6 ][ 0 ]= -15.2682159077;
COEF[1][ 6 ][ 1 ]= -421.870052276;
COEF[1][ 6 ][ 2 ]= -57.8334960011;
COEF[1][ 6 ][ 3 ]= 543.726109798;
COEF[1][ 7 ][ 0 ]= -309.039110797;
COEF[1][ 7 ][ 1 ]= 9.20219749691;
COEF[1][ 7 ][ 2 ]= 89.9577581542;
COEF[1][ 8 ][ 0 ]= 13.6126929861;
COEF[1][ 8 ][ 1 ]= 324.417923389;
COEF[1][ 9 ][ 0 ]= -7.26100172315;
	int i,j;
	POS2[0]=POS2[1]=0;
	for(i=0;i<10;i++)
	for(j=0;j<10;j++)
	if( i + j<10){
		POS2[0]+=COEF[0][i][j]*pow(POS[0],i)*pow(POS[1],j);
		POS2[1]+=COEF[1][i][j]*pow(POS[0],i)*pow(POS[1],j);
	}
	return POS2;
}
std::vector< double > CALC_IMWLD(std::vector< double >POS){
	std::vector< double > POS2(2);
	double COEF[2][10][10]={0};
COEF[0][ 0 ][ 0 ]= 1.19334688956e-09;
COEF[0][ 0 ][ 1 ]= -4.68829398468e-05;
COEF[0][ 0 ][ 2 ]= -6.60313272549e-14;
COEF[0][ 0 ][ 3 ]= 5.04837178619e-15;
COEF[0][ 0 ][ 4 ]= 2.0924188506e-21;
COEF[0][ 0 ][ 5 ]= -2.42752070917e-25;
COEF[0][ 0 ][ 6 ]= -7.36911043931e-30;
COEF[0][ 0 ][ 7 ]= 6.32123123211e-33;
COEF[0][ 0 ][ 8 ]= 1.19653678515e-38;
COEF[0][ 0 ][ 9 ]= -5.53185124363e-42;
COEF[0][ 1 ][ 0 ]= 5.81493687851e-08;
COEF[0][ 1 ][ 1 ]= -2.39378895833e-13;
COEF[0][ 1 ][ 2 ]= 9.72065709264e-17;
COEF[0][ 1 ][ 3 ]= 3.77394370895e-21;
COEF[0][ 1 ][ 4 ]= -1.47992134815e-24;
COEF[0][ 1 ][ 5 ]= -1.08901075482e-29;
COEF[0][ 1 ][ 6 ]= 7.22028563965e-33;
COEF[0][ 1 ][ 7 ]= 9.97446995627e-39;
COEF[0][ 1 ][ 8 ]= -1.12753398435e-41;
COEF[0][ 2 ][ 0 ]= 6.01387174447e-14;
COEF[0][ 2 ][ 1 ]= 5.07395056275e-15;
COEF[0][ 2 ][ 2 ]= -1.54808489365e-21;
COEF[0][ 2 ][ 3 ]= -1.03040020697e-24;
COEF[0][ 2 ][ 4 ]= 1.58812643918e-29;
COEF[0][ 2 ][ 5 ]= 2.20739503218e-32;
COEF[0][ 2 ][ 6 ]= -4.04578909184e-38;
COEF[0][ 2 ][ 7 ]= -2.72765287959e-41;
COEF[0][ 3 ][ 0 ]= -8.45308766225e-17;
COEF[0][ 3 ][ 1 ]= 9.56522394371e-21;
COEF[0][ 3 ][ 2 ]= -6.4299486774e-25;
COEF[0][ 3 ][ 3 ]= -4.07622664685e-29;
COEF[0][ 3 ][ 4 ]= 6.38550843751e-33;
COEF[0][ 3 ][ 5 ]= 6.62016952576e-38;
COEF[0][ 3 ][ 6 ]= -1.87438739899e-41;
COEF[0][ 4 ][ 0 ]= 1.1615097931e-21;
COEF[0][ 4 ][ 1 ]= -5.54616117963e-25;
COEF[0][ 4 ][ 2 ]= 3.05197628603e-29;
COEF[0][ 4 ][ 3 ]= 2.0680565281e-32;
COEF[0][ 4 ][ 4 ]= -6.58510732381e-38;
COEF[0][ 4 ][ 5 ]= -3.86578772558e-41;
COEF[0][ 5 ][ 0 ]= 8.40328570976e-25;
COEF[0][ 5 ][ 1 ]= -6.32950520485e-29;
COEF[0][ 5 ][ 2 ]= 2.58620424591e-33;
COEF[0][ 5 ][ 3 ]= 1.13682821642e-37;
COEF[0][ 5 ][ 4 ]= -4.31632075117e-42;
COEF[0][ 6 ][ 0 ]= -4.53664935288e-30;
COEF[0][ 6 ][ 1 ]= 8.20646252996e-33;
COEF[0][ 6 ][ 2 ]= -8.84405249438e-38;
COEF[0][ 6 ][ 3 ]= -2.34493944928e-41;
COEF[0][ 7 ][ 0 ]= -3.20185476421e-33;
COEF[0][ 7 ][ 1 ]= 1.23061232275e-37;
COEF[0][ 7 ][ 2 ]= -7.30124984562e-42;
COEF[0][ 8 ][ 0 ]= 8.37169822172e-39;
COEF[0][ 8 ][ 1 ]= -9.46744833237e-42;
COEF[0][ 9 ][ 0 ]= 4.31195364554e-42;
COEF[1][ 0 ][ 0 ]= 2.20651543069e-08;
COEF[1][ 0 ][ 1 ]= -5.66828078642e-08;
COEF[1][ 0 ][ 2 ]= 3.38290586096e-14;
COEF[1][ 0 ][ 3 ]= -5.16829279996e-18;
COEF[1][ 0 ][ 4 ]= 6.6345089916e-22;
COEF[1][ 0 ][ 5 ]= 2.93692667955e-25;
COEF[1][ 0 ][ 6 ]= -3.66979792657e-30;
COEF[1][ 0 ][ 7 ]= -1.48448596078e-33;
COEF[1][ 0 ][ 8 ]= 5.87682211253e-39;
COEF[1][ 0 ][ 9 ]= 2.28853616928e-42;
COEF[1][ 1 ][ 0 ]= -4.68801083133e-05;
COEF[1][ 1 ][ 1 ]= -2.18740673662e-13;
COEF[1][ 1 ][ 2 ]= 5.13879722084e-15;
COEF[1][ 1 ][ 3 ]= 2.21798456335e-21;
COEF[1][ 1 ][ 4 ]= -1.98548076196e-24;
COEF[1][ 1 ][ 5 ]= -8.81835376516e-30;
COEF[1][ 1 ][ 6 ]= 1.45933216677e-32;
COEF[1][ 1 ][ 7 ]= 5.72916682931e-39;
COEF[1][ 1 ][ 8 ]= -1.7379010676e-41;
COEF[1][ 2 ][ 0 ]= 3.24708569904e-13;
COEF[1][ 2 ][ 1 ]= 6.91515749463e-18;
COEF[1][ 2 ][ 2 ]= 2.44446694743e-21;
COEF[1][ 2 ][ 3 ]= 5.20826533446e-26;
COEF[1][ 2 ][ 4 ]= -4.15579190929e-29;
COEF[1][ 2 ][ 5 ]= -1.17309148294e-33;
COEF[1][ 2 ][ 6 ]= 8.83201922078e-38;
COEF[1][ 2 ][ 7 ]= 3.24603797537e-42;
COEF[1][ 3 ][ 0 ]= 4.90303818109e-15;
COEF[1][ 3 ][ 1 ]= 9.33451772393e-22;
COEF[1][ 3 ][ 2 ]= -1.26263828201e-24;
COEF[1][ 3 ][ 3 ]= -1.05909952537e-29;
COEF[1][ 3 ][ 4 ]= 3.29111047862e-32;
COEF[1][ 3 ][ 5 ]= 6.23697908767e-38;
COEF[1][ 3 ][ 6 ]= -5.51536976087e-41;
COEF[1][ 4 ][ 0 ]= -2.60973870705e-21;
COEF[1][ 4 ][ 1 ]= 3.09374339316e-25;
COEF[1][ 4 ][ 2 ]= 2.36766867219e-30;
COEF[1][ 4 ][ 3 ]= 4.85344009997e-34;
COEF[1][ 4 ][ 4 ]= 1.05969320362e-37;
COEF[1][ 4 ][ 5 ]= -2.54519142101e-42;
COEF[1][ 5 ][ 0 ]= 1.19182573538e-24;
COEF[1][ 5 ][ 1 ]= 2.64902670318e-30;
COEF[1][ 5 ][ 2 ]= 2.08666772249e-32;
COEF[1][ 5 ][ 3 ]= -1.30377962897e-38;
COEF[1][ 5 ][ 4 ]= -6.15475156737e-41;
COEF[1][ 6 ][ 0 ]= 1.1439366118e-29;
COEF[1][ 6 ][ 1 ]= -3.42275871639e-33;
COEF[1][ 6 ][ 2 ]= -3.0202189982e-38;
COEF[1][ 6 ][ 3 ]= 2.75546886553e-42;
COEF[1][ 7 ][ 0 ]= 1.97052522852e-34;
COEF[1][ 7 ][ 1 ]= -4.85308743016e-39;
COEF[1][ 7 ][ 2 ]= -2.11653537549e-41;
COEF[1][ 8 ][ 0 ]= -1.14523183474e-38;
COEF[1][ 8 ][ 1 ]= 8.21507352457e-42;
COEF[1][ 9 ][ 0 ]= 4.02751121909e-42;
	int i,j;
	POS2[0]=POS2[1]=0;
	for(i=0;i<10;i++)
	for(j=0;j<10;j++)
	if( i + j<10){
		POS2[0]+=COEF[0][i][j]*pow(POS[0],i)*pow(POS[1],j);
		POS2[1]+=COEF[1][i][j]*pow(POS[0],i)*pow(POS[1],j);
	}
	return POS2;
}
std::vector< double > CALC_IMPIX_SIM(std::vector< double >POS){
	std::vector< double > POS2(2);
	double COEF[2][10][10]={0};

	COEF[0][0][1]=1.0/(-4.69*pow(10,-5));
	COEF[1][1][0]=1.0/(-4.69*pow(10,-5));

	int i,j;
	POS2[0]=POS2[1]=0;
	for(i=0;i<10;i++)
	for(j=0;j<10;j++)
	if( i + j<10){
		POS2[0]+=COEF[0][i][j]*pow(POS[0],i)*pow(POS[1],j);
		POS2[1]+=COEF[1][i][j]*pow(POS[0],i)*pow(POS[1],j);
	}
	return POS2;
}
std::vector< double > CALC_IMWLD_SIM(std::vector< double >POS){
	std::vector< double > POS2(2);
	double COEF[2][10][10]={0};

	COEF[0][0][1]=-4.69*pow(10,-5);
	COEF[1][1][0]=-4.69*pow(10,-5);

	int i,j;
	POS2[0]=POS2[1]=0;
	for(i=0;i<10;i++)
	for(j=0;j<10;j++)
	if( i + j<10){
		POS2[0]+=COEF[0][i][j]*pow(POS[0],i)*pow(POS[1],j);
		POS2[1]+=COEF[1][i][j]*pow(POS[0],i)*pow(POS[1],j);
	}
	return POS2;
}

