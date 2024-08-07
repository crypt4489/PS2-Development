#ifndef PS_DRAWING_H
#define PS_DRAWING_H
#include "ps_global.h"

void ShaderProgram(int shader);

void BeginCommand();

void BeginCommandSet(qword_t *drawBuffer);

void EnterCommand(qword_t *q);

void SetVIFHeaderUpload(qword_t *q);

qword_t *GetDrawPtr();

qword_t* EndCommand();

void DepthTest(bool enable, int method);

void DestinationAlphaTest(bool enable, int method);

void SourceAlphaTest(int framebuffer, int method, int reference);

void PrimitiveType(u64 primitive);

void PrimitiveTypeStruct(prim_t prim);

void VertexType(int vertextype);

void FrameBufferMask(int red, int green, int blue, int alpha);

void DepthBufferMask(bool enable);

void BlendingEquation(blend_t *blend);

void PrimitiveColor(Color c);

void PushQWord(void *q, int offset);

void PushMatrix(float *mat, int offset, int size);

void PushInteger(int num, int vuoffset, int vecoffset);

void PushFloat(float num, int vuoffset, int vecoffset);

void DrawCount(int num, int vertexMemberCount, bool toVU);

void DrawVector(VECTOR v);

void DrawColor(Color c);

void StartVertexShader();

void AllocateShaderSpace(int size, int offset);

void PushColor(int r, int g, int b, int a, int vuoffset);

void ShaderHeaderLocation(int location);

void PushPairU64(u64 a, u64 b, u32 memoffset);

void FrameBufferMaskWord(u32 mask);

void WritePairU64(u64 a, u64 b);

void PushScaleVector();

void DrawVectorFloat(float x, float y, float z, float w);

void SetRegSizeAndType(u64 size, u64 type);

void BindTexture(Texture *tex, bool end);

void EndVifImmediate();

void PrintOut();

#endif
