#include "trig.h"

/* 128 sine values * 256 */
static int sine_array[] = {
	0, 12, 25, 37, 49, 62, 74, 86, 97, 109, 120, 131, 142, 152, 162, 171, 181, 189, 197, 205, 212,
	219, 225, 231, 236, 241, 244, 248, 251, 253, 254, 255, 256, 255, 254, 253, 251, 248, 244, 241,
	236, 231, 225, 219, 212, 205, 197, 189, 181, 171, 162, 152, 142, 131, 120, 109, 97, 86, 74, 62,
	49, 37, 25, 12, 0, -12, -25, -37, -49, -62, -74, -86, -97, -109, -120, -131, -142, -152, -162,
	-171, -181, -189, -197, -205, -212, -219, -225, -231, -236, -241, -244, -248, -251, -253, -254,
	-255, -256, -255, -254, -253, -251, -248, -244, -241, -236, -231, -225, -219, -212, -205, -197,
	-189, -181, -171, -162, -152, -142, -131, -120, -109, -97, -86, -74, -62, -49, -37, -25, -12,
};

short sine(int a)
{
	return sine_array[a];
}

short cosine(int a)
{
	return sine_array[(a + 32) & 127];
}