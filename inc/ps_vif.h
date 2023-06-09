#ifndef VIFCOMM_H
#define VIFCOMM_H
#include <vif_codes.h>
#include "ps_global.h"
//#include "ps_vumanager.h"




void UploadProgramToVU1(u32 *cStart, u32 *cEnd, u32 dest, u32 packetSize, u32 programSize);
qword_t* add_unpack_data(qword_t* q, u32 dest_address, void *data, u32 qwSize, u8 use_top, u32 vif_pack);
qword_t * read_unpack_data(qword_t *q, u32 dest_address, u32 qwSize, u8 use_top, u32 vif_pack);
qword_t* add_start_program_vu1(qword_t *q, u32 address);
qword_t* set_alpha_registers(qword_t *q, blend_t *blend, int context);
void load_framebuffer_vif(unsigned char *pixels, framebuffer_t *frame, int width, int height, int psm);
qword_t* load_texture_vif(qword_t* q, Texture *tex, void *pixels, unsigned char *clut_buffer);
qword_t* set_tex_address_mode(qword_t*q, u32 mode, u32 context);
qword_t *VIFSetupScaleVector(qword_t*b);
qword_t* vif_setup_tex(qword_t *b, Texture *tex, u32 context);
qword_t* vif_setup_rgbaq(qword_t *b, color_t color);
qword_t* InitDoubleBufferingQWord(qword_t *q, u16 base, u16 offset);

#endif
