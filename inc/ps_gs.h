#ifndef PS_GS_H
#define PS_GS_H

#include "ps_global.h"
#include <gs_psm.h>
#include <gs_privileged.h>
#include <gs_gp.h>


#define CreateGSScreenCoordinates(input, operand)   ((2048 operand input) << 4)


void InitGS(GameManager *manager, framebuffer_t *frame, zbuffer_t *z, int context);
void InitFramebuffer(framebuffer_t *frame, int width, int height, int psm);
void load_framebuffer(framebuffer_t *frame, unsigned char *pixels, int width, int height, int psm);

void CreateTexBuf(Texture *texture, int width, int psm);
void load_texture(Texture *texture, unsigned char *pixels, int width, int height, int psm, u32 components);
void load_texture_clut(Texture *texture, clutbuffer_t* clut, unsigned char *pixels, int width, int height, int psm, u32 components);
void load_texture_32(Texture *texture, u32 *pixels);
void init_tex_structs(Texture *texture, clutbuffer_t* clut, int width, int height, int psm, u32 components);
qword_t* setup_texture(Texture *texture, qword_t *q);
qword_t* CreateGSSetTag(qword_t *q, u32 count, u32 eop, u32 type, u32 nreg, u32 regaddr);

void init_drawing_environment(framebuffer_t *frame, zbuffer_t *z, int hheight, int hwidth, int context, int waitFinish);
void copy_vram_from_vram(int srcAd, int srcH, int srcW, int dstAd, int dstH, int dstW, int psm);
void copy_vram_to_memory(int address, int width, int height, int x, int y, int psm, u32* buffer);

void CreateClutBuf(clutbuffer_t *clut, int width, int psm);
void load_clut_buffer(clutbuffer_t *clut, unsigned char *pixels, int width, int height, int psm);
void init_zbuffer(zbuffer_t *z, int width, int height, int zsm);
void CreateClutStructs(Texture *tex, int width, int psm);
void CreateTexStructs(Texture *tex, int width, int psm, u32 components, u32 function, u32 texfilter);
void SetupRenderTarget(RenderTarget *target, int context, int wait);
RenderTarget *CreateRenderTarget(int height, int width);
Texture *CreateTextureFromRenderTarget(RenderTarget *target, u32 filter, u32 function);
RenderTarget *allocRenderTarget();
void DestroyRenderTarget(RenderTarget *target);


void ClearScreen(RenderTarget *target, int context, int r, int g, int b, int a);

qword_t *draw_clear_alpha(qword_t *q, int context, float x, float y, float width, float height, int r, int g, int b, int a);

qword_t *draw_enable_tests_alpha(qword_t *q, int context, int alpha, u32 method);
qword_t *draw_disable_tests_alpha(qword_t *q, int context, int alpha);
qword_t *SetupZTestGS(qword_t* q, int z_test_method, int z_test_enable, char alphaValue, char alpha_test_method, char frameBufferTest, char alpha_test_enable, char alpha_test, int context);
qword_t* SetupRGBAQGS(qword_t *b, color_t color);
qword_t* SetupAlphaGS(qword_t *q, blend_t *blend, int context);
#endif
//
