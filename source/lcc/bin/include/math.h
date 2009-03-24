/* math.h -- Definitions for the math floating point package.  */

#ifndef  _MATH_H_INCLUDED
#define  _MATH_H_INCLUDED
#ifndef HUGE_VAL

#define HUGE_VAL (*HUGE_dll)
extern double *HUGE_dll;

#endif /* ! defined (HUGE_VAL) */

/* Reentrant ANSI C functions.  */

extern double atan(double);
#if __LCCOPTIMLEVEL > 0
extern double _stdcall _fcos(double);
extern double _stdcall _fsin(double);
#define cos _fcos
#define sin _fsin
#else
extern double cos(double);
extern double sin(double);
#endif
extern double tan(double);
extern double tanh(double);
extern double frexp(double, int *);
extern double modf(double, double *);
extern double ceil(double);
extern double cbrt(double);
#if __LCCOPTIMLEVEL > 0
extern double _stdcall _fabs(double);
#define fabs _fabs
#else
extern double fabs(double);
#endif
extern double floor(double);

extern double acos (double);
extern double asin (double);
extern double atan2 (double, double);
extern double cosh (double);
extern double sinh (double);
extern double exp(double);
extern double ldexp(double, int);
extern double log(double);
extern double log10(double);
extern double pow(double, double);
extern double scalbn(double,int);
#if __LCCOPTIMLEVEL > 0
extern double _stdcall _fsqrt(double);
#define sqrt _fsqrt
#else
extern double sqrt(double);
#endif
extern double fmod(double, double);
extern double atof(const char *);


/* Non ANSI double precision functions.  */

extern double infinity(void);
extern int isnan (double);
extern int finite (double);
#define isnormal(a) (isnan(a) ? 0 : (a == 0.0) ? 0 : 1)
extern double copysign(double, double);
int ilogb(double);
double logb(double);
double erf(double);
double erfc(double);
#define FP_ILOGB0 0x80000000
#ifdef __LCC__
extern int _stdcall _signbit(double);
#define signbit _signbit
#endif
extern double asinh (double);
extern double nextafter (double, double);

extern double acosh (double);
extern double atanh(double);
extern double lgamma (double);
extern double y0 (double);
extern double y1 (double);
extern double yn(int, double);
extern double j0(double);
extern double j1 (double);
extern double jn (int, double);
#define log2(x) (log (x) / M_LOG2_E)

extern double hypot (double, double);
#define _hypot hypot
extern int _stdcall rint(double);
#define rint _rint
extern int isinf(double);
#define isfinite(d) ((isinf(d)||isnan(d))?0:1)
#ifndef __COMPLEX_DEFINED__
struct _complex {
	double x,y;
};
#define __COMPLEX_DEFINED
#endif

extern double cabs(struct _complex);


/* The exception structure passed to the matherr routine.  */

struct exception 
{
  int type;
  char *name;
  double arg1;
  double arg2;
  double retval;
  int err;
};

extern int matherr (struct exception *e);

/* Values for the type field of struct exception.  */

#define DOMAIN 1
#define SING 2
#define OVERFLOW 3
#define UNDERFLOW 4
#define TLOSS 5
#define PLOSS 6

/* Useful constants.  */

#define M_E		2.7182818284590452354
#define M_LOG2E		1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2		0.69314718055994530942
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_TWOPI         (M_PI * 2.0)
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.78539816339744830962
#define M_3PI_4		2.3561944901923448370E0
#define M_SQRTPI        1.77245385090551602792981
#define M_1_PI		0.31830988618379067154
#define M_2_PI		0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440
#define M_LN2LO         1.9082149292705877000E-10
#define M_LN2HI         6.9314718036912381649E-1
#define M_SQRT3   	1.73205080756887719000
#define M_IVLN10        0.43429448190325182765 /* 1 / log(10) */
#define M_LOG2_E        0.693147180559945309417
#define M_INVLN2        1.4426950408889633870E0  /* 1 / log(2) */

#endif /* _MATH_H_ */
