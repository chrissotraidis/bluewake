#ifndef BLUEWAKE_BMG_PATCH_H
#define BLUEWAKE_BMG_PATCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*BluewakeBmgWriteByte)(void* user, size_t offset, uint8_t value);

static inline uint32_t bluewake_bmg_u32(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static inline uint16_t bluewake_bmg_u16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

/* Validate the complete loaded message table before changing any byte. The
 * caller supplies the remaining RAM span, not just the file's claimed size,
 * and writes through its guest-memory accessor to retain write journaling. */
static inline bool bluewake_bmg_patch(const uint8_t* data, size_t available,
                                     BluewakeBmgWriteByte write_byte, void* user,
                                     uint32_t* messages_out, uint32_t* waits_out) {
    if (data == NULL || available < 32 || write_byte == NULL ||
        messages_out == NULL || waits_out == NULL ||
        bluewake_bmg_u32(data) != 0x4D455347u ||
        bluewake_bmg_u32(data + 4) != 0x626D6731u)
        return false;
    const uint32_t blocks = bluewake_bmg_u32(data + 8);
    const uint32_t sections = bluewake_bmg_u32(data + 12);
    if (blocks < 2 || blocks > 0x00400000u / 32u ||
        sections == 0 || sections > 16 || data[16] == 2)
        return false;
    const size_t size = (size_t)blocks * 32;
    if (size > available)
        return false;
    size_t inf = 0, inf_size = 0, dat = 0, dat_size = 0, at = 32;
    for (uint32_t s = 0; s < sections; ++s) {
        if (at > size || size - at < 8)
            return false;
        const uint32_t magic = bluewake_bmg_u32(data + at);
        const size_t section_size = bluewake_bmg_u32(data + at + 4);
        if (section_size < 8 || section_size > size - at)
            return false;
        if (magic == 0x494E4631u) {
            if (inf != 0 || section_size < 16)
                return false;
            inf = at;
            inf_size = section_size;
        } else if (magic == 0x44415431u) {
            if (dat != 0)
                return false;
            dat = at;
            dat_size = section_size;
        }
        at += section_size;
    }
    if (inf == 0 || dat == 0)
        return false;
    const uint32_t messages = bluewake_bmg_u16(data + inf + 8);
    const size_t entry = bluewake_bmg_u16(data + inf + 10);
    if (entry < 14 || messages > (inf_size - 16) / entry)
        return false;
    const size_t text_start = dat + 8, text_end = dat + dat_size;
    uint32_t waits = 0;
    /* Pass zero rejects malformed later entries before pass one writes draw
     * types or commands. Writes change neither lengths nor string offsets. */
    for (unsigned pass = 0; pass < 2; ++pass) {
        for (uint32_t m = 0; m < messages; ++m) {
            const size_t e = inf + 16 + (size_t)m * entry;
            const size_t offset = bluewake_bmg_u32(data + e);
            if (offset >= dat_size - 8)
                return false;
            if (pass != 0)
                write_byte(user, e + 13, 1);
            size_t p = text_start + offset;
            while (p < text_end && data[p] != 0) {
                if (data[p] != 0x1A) {
                    ++p;
                    continue;
                }
                if (text_end - p < 2)
                    return false;
                const size_t length = data[p + 1];
                if (length < 2 || length > text_end - p)
                    return false;
                if (pass != 0 && length == 7 && data[p + 2] == 0 && data[p + 3] == 0) {
                    const uint8_t command = data[p + 4];
                    if ((command == 7 || command == 3) &&
                        (command != 7 || data[p + 5] != 0 || data[p + 6] != 0)) {
                        ++waits;
                        write_byte(user, p + 4, 7);
                        write_byte(user, p + 5, 0);
                        write_byte(user, p + 6, 0);
                    }
                }
                p += length;
            }
            if (p == text_end)
                return false; // no terminating NUL within DAT1
        }
    }
    *messages_out = messages;
    *waits_out = waits;
    return true;
}

#endif
