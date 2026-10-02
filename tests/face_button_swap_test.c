#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "face_button_swap.h"
int main(void) {
    // Different default orders (standard, Switch and NSO); native sources stay fixed.
    uint32_t defaults[][4] = {{0x100,0x200,0x400,0x800},{0x200,0x100,0x800,0x400},{0x400,0x100,0x800,0x200}};
    for (unsigned table = 0; table < 3; ++table)
        for (unsigned i = 0; i < 4; ++i) {
            uint32_t value = defaults[table][i];
            assert(bw_swap_face_buttons(value, false, false) == value);
            assert(bw_swap_face_buttons(bw_swap_face_buttons(value, true, true), true, true) == value);
        }
    assert(bw_swap_face_buttons(0x100, true, false) == 0x200);
    assert(bw_swap_face_buttons(0x800, false, true) == 0x400);
    for (uint32_t value = 0; value < 65536; ++value)
        for (unsigned flags = 0; flags < 4; ++flags) {
            uint32_t swapped = bw_swap_face_buttons(value, flags & 1, flags & 2);
            assert((swapped & ~0xf00u) == (value & ~0xf00u));
            assert(bw_swap_face_buttons(swapped, flags & 1, flags & 2) == value);
        }
}
