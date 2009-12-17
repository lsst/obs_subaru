# Create a Few test Schemas

typedef struct t { int *p; int q} T
typedef struct t1 { T* tptr} T1            
typedef struct t2 { T1* t1ptr} T2
typedef struct t3 { T2* t2ptr} T3
typedef struct t4 { T3* t3ptr} T4

typedef struct s1 {
	int	id;
	float	f;
	double	d;
	int	a[16];
}	SS;

typedef struct s2 {
	int	id;
	int	total;
	SS	ss[10][5];
}	SSS;

typedef struct s3 {
	int	id;
	double	value;
	SSS	sss;
	SSS	s2[10];
	SSS	*sssptr;
}	SSSS;

typedef struct s4 {
	REGION	r[10];
	int	n;
	int	a[10][10][10][10];
}	RR;

typedef struct s1 {
	float	f;
	int	id;
}	S1;

typedef struct s2 {
	int	id;
	S1	a[3][4];
        S1      b[10];
}	S2;

typedef struct { 
    char Label[80]; 
    char *name; 
    char mask[10];
} S_STR;

typedef struct {
    int i<32>;
    int j;
} WITH_ARRAY;


