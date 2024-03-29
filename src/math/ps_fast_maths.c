#include "math/ps_fast_maths.h"

// Morten "Sparky" Mikkelsen's fast maths routines (From a post on the forums
// at www.playstation2-linux.com)

float Abs(const float x)
{
	float r;
	asm(" abs.s %0, %1 "
		: "=&f"(r)
		: "f"(x));
	return r;
}
float Sqrt(const float x)
{
	float r;
	asm(" sqrt.s %0, %1 "
		: "=&f"(r)
		: "f"(x));
	return r;
}
float Max(const float a, const float b)
{
	float r;
	asm(" max.s %0, %1, %2 "
		: "=&f"(r)
		: "f"(a), "f"(b));
	return r;
}
float Min(const float a, const float b)
{
	float r;
	asm(" min.s %0, %1, %2 "
		: "=&f"(r)
		: "f"(a), "f"(b));
	return r;
}
float ACos(float x) { return PIHALF - ASin(x); }
float Sin(float v) { return Cos(v - PIHALF); }

float DegToRad(float Deg)
{
	return (Deg / 180.0f) * PI;
}

float Mod(const float a, const float b)
{
	float r;

	asm __volatile__(
		".set push\n"
		".set noreorder \n"
		"div.s %0, %1, %2 \n"
		"mtc1 $0, $f8 \n"
		"mfc1 $10, %0 \n"
		"srl $8, $10, 23 \n"
		"addiu $9, $0, 127+23 \n"
		"andi $8, $8, 0xff \n"
		"addiu $12, $0, 1 \n"
		"addiu $11, $8, -127 \n"
		"subu $8, $9, $8 \n"
		"pmaxw $8, $8, $0 \n"
		"sllv $8, $12, $8 \n"
		"lui $9, 0x8000 \n"
		"negu $8, $8 \n"
		"srl $11, $11, 31 \n"
		"movz $9, $8, $11 \n"
		"and $10, $9, $10 \n"
		"mtc1 $10, %0 # trunc(fdiv) \n"
		"adda.s %1, $f8 \n"
		"msub.s %0, %2, %0 \n"
		".set pop \n"
		: "=&f"(r)
		: "f"(a), "f"(b)
		: "$8", "$9", "$10", "$11", "$12", "$f8");

	/*
	float fDiv = a / b
	int32 uDiv = MFC1(fDiv)
	int32 exp = (uDiv>>23)&0xff
	int32 exp_cen = exp-127
	int32 bits = max(127+23 - exp, 0)
	uint32 and_val = 0x80000000
	MOVZ(and_val, -(1<<bits), exp_cen>>31) // ~((1<<bits)-1)
	fTruncDiv = MTC1(uDiv&and_val)

	return a - fTruncDiv*b; // a = i*b + r
	*/

	return r;
}

float ASin(float x)
{
	float r;

	asm __volatile__(

		"# s0-s4, $f3-$f7 \n"
		"# offs, $f6 (tmp) \n"
		"# x^2, $f7 (tmp) \n"

		".set noreorder \n"
		".align 3 \n"
		"lui $8, 0x3f80 \n"
		"mtc1 $0, $f8 \n"
		"mtc1 $8, $f1 \n"
		"lui $8, 0x3f35 \n"
		"mfc1 $9, %1 \n"
		"ori $8, $8, 0x04f3 \n"
		"adda.s $f1, $f8 \n"
		"lui $10, 0x8000 \n"
		"msub.s $f2, %1, %1 \n"
		"not $11, $10 \n"
		"and $11, $9, $11 # $11, uAbsX \n"
		"subu $8, $8, $11 \n"
		"nop # don't remove this nop\n"
		"and $9, $9, $10 # $9, uSign \n"
		"sqrt.s $f2, $f2 \n"
		"srl $8, $8, 31 # $8, bFlip = ((BOUND_HEX - uAbsX) >> 31) \n"
		"abs.s %0, %1 \n"
		"lui $11, 0x3fc9 \n"
		"ori $11, 0x0fdb \n"
		"or $11, $9, $11 \n"
		"movz $11, $0, $8 # offs_hex \n"
		"xor $10, $9, $10 \n"
		"mtc1 $11, $f6 \n"
		"movz $10, $9, $8 # uSign \n"
		"min.s %0, $f2, %0 \n"
		"lui $9, 0x3e2a \n"
		"lui $8, 0x3f80 \n"
		"ori $9, $9, 0xaaab \n"
		"or $8, $10, $8 \n"
		"or $9, $10, $9 \n"
		"mtc1 $8, $f3 # upload S0 \n"
		"lui $8, 0x3d99 \n"
		"mul.s $f7, %0, %0 # x^2 \n"
		"ori $8, $8, 0x999a \n"
		"mtc1 $9, $f4 # upload S1 \n"
		"lui $9, 0x3d36 \n"
		"or $8, $10, $8 \n"
		"ori $9, $9, 0xdb6e \n"
		"mtc1 $8, $f5 # upload S2 \n"
		"or $9, $10, $9 \n"
		"mul.s $f1, %0, $f7 # x^3 \n"
		"lui $8, 0x3cf8 \n"
		"adda.s $f6, $f8 # acc = offs \n"
		"ori $8, $8, 0xe38e \n"
		"mul.s $f8, $f7, $f7 # x^4 \n"
		"or $8, $10, $8 \n"
		"madda.s $f3, %0 \n"
		"mul.s $f2, $f1, $f7 # x^5 \n"
		"madda.s $f4, $f1 \n"
		"mtc1 $9, $f6 # upload S3 to $f6\n"
		"mul.s $f1, $f1, $f8 # x^7 \n"
		"mul.s %0, $f2, $f8 # x^9 \n"
		"mtc1 $8, $f7 # upload S4 to $f7 \n"
		"madda.s $f5, $f2 \n"
		"madda.s $f6, $f1 \n"
		"madd.s %0, $f7, %0 \n"

		".set reorder \n"

		: "=&f"(r)
		: "f"(x)
		: "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8");

	/*
	// BOUND_HEX, 0x3f3504f3 (IEEE)
	// PIHALF_HEX, 0x3fc90fdb (IEEE)
	float r;

	const float Xc = Sqrt(1-x*x);
	const float Xs = Abs(x);

	float offs;
	const uint32 uX = *((uint32 *) &x);
	uint32 uSign = uX&0x80000000;
	const uint32 uAbsX = uX&0x7fffffff;
	*((uint32 *) &offs) = 0;

	// if Abs(x) > bound then manipulate
	// expression to do an acos taylor instead.
	// this is done for better precission
	bool bFlip = (bool) ((BOUND_HEX - uAbsX) >> 31); // bound = 1/sqrt(2)
	MovNZero((uint32 *) &offs, PIHALF_HEX | uSign, bFlip);
	MovNZero(&uSign, uSign^0x80000000, bFlip);

	x = Min(Xc, Xs);

	const float x2 = x*x;
	const float x3 = x*x2;
	const float x5 = x3*x2;
	const float x7 = x5*x2;
	const float x9 = x7*x2;

	const float s0 = MTC1(0x3f800000 | uSign); // 1
	const float s1 = MTC1(0x3e2aaaab | uSign); // 1/6
	const float s2 = MTC1(0x3d99999a | uSign); // 3/40
	const float s3 = MTC1(0x3d36db6e | uSign); // 5/112
	const float s4 = MTC1(0x3cf8e38e | uSign); // 35/1152

	r = offs+s0*x+s1*x3+s2*x5+s3*x7+s4*x9;


	return r;

	*/

	return r;
}

float Cos(float v)
{
	float r;

	asm __volatile__(

		"lui $9, 0x3f00 \n"

		".set noreorder \n"
		".align 3 \n"
		"abs.s %0, %1 \n"
		"lui $8, 0xbe22 \n"

		"mtc1 $9, $f1 # 0.5f \n"
		"ori $8, $8, 0xf983 \n"

		"mtc1 $8, $f8 # -1/TWOPI \n"
		"lui $9, 0x4b00 \n"

		"mtc1 $9, $f3 # bias \n"
		"lui $8, 0x3f80 \n"

		"mtc1 $8, $f2 # 1.0f \n"
		"mula.s %0, $f8 \n"
		"msuba.s $f3, $f2 \n"
		"madda.s $f3, $f2 \n"
		"lui $8, 0x40c9 \n"

		"msuba.s %0, $f8 \n"
		"ori $8, 0x0fdb \n"

		"msub.s %0, $f1, $f2 \n"
		"lui $9, 0xc225 \n"

		"abs.s %0, %0 \n"
		"lui $10, 0x3e80 \n"

		"mtc1 $10, $f7 \n"
		"ori $9, 0x5de1 \n"

		"sub.s %0, %0, $f7 \n"
		"lui $10, 0x42a3 \n"

		"mtc1 $8, $f3 \n"
		"ori $10, 0x3458 \n"

		"mtc1 $9, $f4 \n"
		"lui $8, 0xc299 \n"

		"mtc1 $10, $f5 \n"
		"ori $8, 0x2663 \n"

		"mul.s $f8, %0, %0 \n"
		"lui $9, 0x421e \n"

		"mtc1 $8, $f6 \n"
		"ori $9, 0xd7bb \n"

		"mtc1 $9, $f7 \n"
		"nop \n"
		"mul.s $f1, %0, $f8 # v^3 \n"
		"mul.s $f9, $f8, $f8 \n"
		"mula.s $f3, %0 \n"
		"mul.s $f2, $f1, $f8 # v^5 \n"
		"madda.s $f4, $f1 \n"
		"mul.s $f1, $f1, $f9 # v^7 \n"
		"mul.s %0, $f2, $f9 # v^9 \n"
		"madda.s $f5, $f2\n"
		"madda.s $f6, $f1 \n"
		"madd.s %0, $f7, %0 \n"
		".set reorder \n"

		: "=&f"(r)
		: "f"(v)
		: "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9", "$8", "$9", "$10");

	/*
	v = Abs(v);

	// using accumulator for speed
	float acc = v*(-1.0f/TWOPI); // be22f983
	float b; *((uint32 *) &b) = 0x4b000000;
	acc -= 1.0f*b;
	acc += 1.0f*b; // after this add all decimals will be removed leaving -int(v) due to controled rounding errors
	acc -= (v*(-1.0f/TWOPI)); // this will add v to -int(v) leaving just the decimals

	// v (in acc) is now in the range [0; 1)
	// cos/sin in this description is addapted to
	// the domain [0; 1) instead of [0; TWOPI)
	// cos(v)=-cos(v-0.5)=-cos(-(v-0.5)) = -cos(abs(v-0.5)) =
	// -sin(abs(v-0.5)+0.25) = sin(abs(v-0.5)-0.25)
	// since v was in the range [0; 1) then
	// v' = abs(v-0.5)-0.25 will be in the range [-0.25; 0.25)
	acc -= 1.0f*0.5f;
	v = Abs(acc) - 0.25f;

	float s1, s2, s3, s4, s5;
	s1 = MTC1(0x40c90fdb);
	s2 = MTC1(0xc2255de1);
	s3 = MTC1(0x42a33458);
	s4 = MTC1(0xc2992663);
	s5 = MTC1(0x421ed7bb);

	float v2 = v*v;
	float v3 = v*v2;
	float v5 = v3*v2;
	float v7 = v5*v2;
	float v9 = v7*v2;
	r = s1*v+s2*v3+s3*v5+s4*v7+s5*v9;

	*/

	return r;
}
