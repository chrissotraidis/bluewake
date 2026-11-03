#ifndef BLUEWAKE_PAD_WIRE_H
#define BLUEWAKE_PAD_WIRE_H

#include "gxruntime/headless_backend.h"

void bluewake_pad_wire_encode(const DolPadState* pad, u16 buttons,
                              u32* data0_out, u32* data1_out);
void bluewake_pad_merge(const DolPadState* configured,
                        const DolPadState* live, DolPadState* merged_out);

#endif
