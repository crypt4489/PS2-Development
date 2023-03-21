#ifndef PERLIN_H
#define PERLIN_H

#include "psxgte.h"
#include "irx_imports.h"
int Fade(int t);
int lerp(int x, int y, int w);
int p_noise(int x, int y);
void generatePerlin(unsigned char *iop_addr, int buffer_size, int height, int channels, int context);
#endif
