#ifndef PS_PAD_H
#define PS_PAD_H

#include "ps_global.h"
#include "libpad.h"

typedef struct controller_pad_t
{
    int port;
    int slot;
    char padBuf[256] __attribute__((aligned(64)));
    struct padButtonStatus buttons;
    u16 currentButtonsPressed;
    u16 previousButtonsPressed;
    u16 buttonsToggledDown;
    u16 buttonsToggledUp;
} Controller;

void LoadInitLibPad();
void InitializeController(Controller *controller, int port, int slot);
int GetPadWhenReady(Controller *controller);
int ReadPad(Controller *controller);
#endif
