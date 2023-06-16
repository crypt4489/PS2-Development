#ifndef CHECKBOARD_H
#define CHECKBOARD_H
#include "psxmath.h"
#include "irx_imports.h"
int CheckerBoard(int x, int y, int context);
void generateBoard(unsigned char *iop_addr, int buffer_size, int height, int channels, int context, int angle);
#endif
