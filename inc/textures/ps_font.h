#ifndef PS_FONT_H
#define PS_FONT_H



#include "ps_global.h"

typedef enum  {
    LEFT,
    RIGHT,
    CENTER
} TextAlign;



void LoadFontWidths(Font *font_struct, const char * filePath);
int WidthOfString(Font *font_struct, const char *text);
int RenderL(Font *font_struct, int x, int y, const char *text);
unsigned char *RewriteAlphaClutBuffer(unsigned char *buffer);

void PrintText(Font *fontStruct, const char *text, int x, int y, TextAlign alignment);

Font *CreateFontStruct(const char* fontName, const char *fontData, int read_type);
Font *CreateFontStructFromBuffer(const char *fontName, u8 *fontPic, 
                                 u8 *fontData, int read_type, 
                                 u32 picSize, u32 dataSize);

void CleanFontStruct(Font *font);

void CreateFontWidthsFromFile(void *object, void*, u8 *buffer, u32 bufferLen);
#endif
