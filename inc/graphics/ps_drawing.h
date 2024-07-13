#ifndef PS_DRAWING_H
#define PS_DRAWING_H
#include "ps_global.h"

void ShaderProgram(int shader);

void BeginCommand();

void BeginCommandSet(qword_t *drawBuffer);

qword_t* EndCommand();


void DepthTest(int enable, int method);

void DestinationAlphaTest(int enable, int method);

void SourceAlphaTest(int framebuffer, int method, int reference);

void PrimitiveType(int primitive);

void VertexType(int vertextype);

void FrameBufferMask(int red, int green, int blue, int alpha);

void DepthBufferMask(int enable);

void PushQWord(void *q, int offset);

void PushMatrix(float *mat, int offset, int size);

void PushInteger(int num, int vuoffset, int vecoffset);

void PushFloat(float num, int vuoffset, int vecoffset);

void DrawCount(int num);

void DrawVector(VECTOR v);

void DrawVertices();

void AllocateShaderSpace(int size, int offset);

void PushColor(int r, int g, int b, int a, int vuoffset);

void ShaderHeaderLocation(int location);

void PushPairU64(u64 a, u64 b, u32 memoffset);

void FrameBufferMaskWord(u32 mask);

void WritePairU64(u64 a, u64 b);

#endif
