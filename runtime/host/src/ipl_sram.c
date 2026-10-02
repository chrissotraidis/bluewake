// See ipl_sram.h.
#include "ipl_sram.h"
#include "atomic_file.h"

#include <stdio.h>
#include <string.h>

#define EXI0_BASE 0xCC006800u
#define STATUS_CS_MASK 0x380u
#define STATUS_CS_DEVICE1 0x100u
#define CONTROL_TSTART 0x1u
#define CONTROL_DMA 0x2u
#define SRAM_ADDRESS 0x800004u  // (0x20000100 & 0x7FFFFFFF) >> 6

void bluewake_ipl_sram_defaults(u8 s[BLUEWAKE_IPL_SRAM_SIZE]) {
    memset(s, 0, BLUEWAKE_IPL_SRAM_SIZE);
    // OSSram: checkSum, checkSumInv, ead0, ead1, counterBias, displayOffsetH,
    // ntd, language (0 English), flags (0x20 | setup done 0x08 | stereo 0x04).
    s[19] = 0x2C;
    u16 sum = 0, inv = 0;
    for (u32 i = 12; i < 20; i += 2) {
        const u16 w = (u16)((s[i] << 8) | s[i + 1]);
        sum = (u16)(sum + w);
        inv = (u16)(inv + (u16)~w);
    }
    s[0] = (u8)(sum >> 8), s[1] = (u8)sum, s[2] = (u8)(inv >> 8), s[3] = (u8)inv;
    // OSSramEx: Dolphin's placeholder flash ids and their checksums.
    memcpy(s + 20, "DOLPHINSLOTA", 12);
    memcpy(s + 32, "DOLPHINSLOTB", 12);
    s[58] = 0x6E;
    s[59] = 0x6D;
}

static void persist(const BluewakeIplSram* d) {
    if (d->path == NULL)
        return;
    char* pending = bw_atomic_path(d->path);
    FILE* f = pending != NULL ? fopen(pending, "wb") : NULL;
    bool ok = false;
    if (f != NULL) {
        ok = fwrite(d->sram, 1, BLUEWAKE_IPL_SRAM_SIZE, f) == BLUEWAKE_IPL_SRAM_SIZE;
        ok = bw_atomic_finish(f, pending, d->path, ok);
    }
    if (!ok) fprintf(stderr, "[sram] could not save %s; previous file kept\n", d->path);
    free(pending);
}

void bluewake_ipl_sram_init(BluewakeIplSram* d, const char* spec) {
    memset(d, 0, sizeof(*d));
    if (spec == NULL || spec[0] == '\0' || strcmp(spec, "0") == 0)
        return;
    d->enabled = true;
    bluewake_ipl_sram_defaults(d->sram);
    if (strcmp(spec, "default") == 0)
        return;
    d->path = spec;
    FILE* f = fopen(spec, "rb");
    u8 loaded[BLUEWAKE_IPL_SRAM_SIZE];
    if (f != NULL) {
        const size_t n = fread(loaded, 1, sizeof loaded, f);
        fclose(f);
        if (n == sizeof loaded) {
            memcpy(d->sram, loaded, sizeof loaded);
            return;
        }
    }
    persist(d);
}

bool bluewake_ipl_sram_contains(const BluewakeIplSram* d, u32 address) {
    return d->enabled && address >= EXI0_BASE && address < EXI0_BASE + 0x14u;
}

u32 bluewake_ipl_sram_read(const BluewakeIplSram* d, u32 address) {
    switch (address - EXI0_BASE) {
    case 0x00: return d->status;
    case 0x04: return d->dma_address;
    case 0x08: return d->dma_length;
    case 0x0C: return d->control & ~CONTROL_TSTART;  // transfers finish at once
    case 0x10: return d->data;
    default: return 0u;
    }
}

static u8 sram_byte(const BluewakeIplSram* d, u32 cursor) {
    return d->sram_region && cursor < BLUEWAKE_IPL_SRAM_SIZE ? d->sram[cursor] : 0u;
}

static void store_byte(BluewakeIplSram* d, u32 cursor, u8 value) {
    if (d->sram_region && cursor < BLUEWAKE_IPL_SRAM_SIZE)
        d->sram[cursor] = value;
}

static void transfer(BluewakeIplSram* d, BluewakeIplRead8 read8,
                     BluewakeIplWrite8 write8, void* user) {
    const bool device1 = (d->status & STATUS_CS_MASK) == STATUS_CS_DEVICE1;
    const bool dma = (d->control & CONTROL_DMA) != 0u;
    const u32 type = (d->control >> 2) & 3u;  // 0 read, 1 write
    const u32 length = dma ? d->dma_length : ((d->control >> 4) & 3u) + 1u;
    if (!device1) {
        if (!dma && type == 0u)
            d->data = 0u;  // no other device answers
        return;
    }
    if (!d->command_latched) {
        if (!dma && type == 1u && length == 4u) {
            const u32 command = d->data;
            const u32 address = (command & 0x7FFFFFFFu) >> 6;
            d->command_latched = true;
            d->write = (command & 0x80000000u) != 0u;
            d->sram_region = address >= SRAM_ADDRESS &&
                             address < SRAM_ADDRESS + BLUEWAKE_IPL_SRAM_SIZE;
            d->cursor = d->sram_region ? address - SRAM_ADDRESS : 0u;
        }
        return;
    }
    bool changed = false;
    if (dma) {
        for (u32 i = 0; i < length; i++) {
            const u32 at = (d->dma_address + i) & 0x01FFFFFFu;
            if (type == 0u) {
                write8(user, at, sram_byte(d, d->cursor + i));
            } else if (d->write) {
                store_byte(d, d->cursor + i, read8(user, at));
                changed = true;
            }
        }
    } else if (type == 0u) {
        u32 value = 0u;
        for (u32 i = 0; i < length; i++)
            value |= (u32)sram_byte(d, d->cursor + i) << ((3u - i) * 8u);
        d->data = value;
    } else if (d->write) {
        for (u32 i = 0; i < length; i++)
            store_byte(d, d->cursor + i, (u8)(d->data >> ((3u - i) * 8u)));
        changed = true;
    }
    d->cursor += length;
    if (changed && d->sram_region) {
        d->writes++;
        persist(d);
    }
}

void bluewake_ipl_sram_write(BluewakeIplSram* d, u32 address, u32 value,
                             BluewakeIplRead8 read8, BluewakeIplWrite8 write8,
                             void* user) {
    switch (address - EXI0_BASE) {
    case 0x00: {
        // Chip select, clock and masks are the guest's; interrupt flags are
        // write-one-to-clear and never raised here, and EXT (a card in slot A)
        // stays clear.
        d->status = value & (0x7F5u | 0x2000u);
        if ((d->status & STATUS_CS_MASK) == 0u)
            d->command_latched = false;  // deselect ends the command
        break;
    }
    case 0x04: d->dma_address = value & 0x03FFFFE0u; break;
    case 0x08: d->dma_length = value & 0x03FFFFE0u; break;
    case 0x0C:
        d->control = value;
        if ((value & CONTROL_TSTART) != 0u)
            transfer(d, read8, write8, user);
        break;
    case 0x10: d->data = value; break;
    default: break;
    }
}
