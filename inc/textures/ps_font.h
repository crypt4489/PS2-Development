#ifndef PS_FONT_H
#define PS_FONT_H



#include "ps_global.h"





void LoadFontWidths(Font *font_struct, const char * filePath);
int WidthOfString(Font *font_struct, const char *text);
qword_t *RenderL(qword_t* q, Font *font_struct, int x, int y, const char *text, int context);
unsigned char * RewriteAlphaClutBuffer(unsigned char *buffer);

void PrintText(Font *fontStruct, const char *text, int x, int y);

Font *CreateFontStruct(const char* fontName, const char *fontData, int read_type);

void CleanFontStruct(Font *font);

void CreateFontWidthsFromFile(void* object, void*, u8 *buffer, u32 bufferLen);
#endif
