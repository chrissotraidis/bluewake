#ifdef NDEBUG
#undef NDEBUG
#endif
// EXI SRAM device (runtime/host/src/ipl_sram.c) driven by the SDK's own
// register sequence (ref/tww src/dolphin/os/OSRtc.c, exi/EXIBios.c).
#include "ipl_sram.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static u8 g_ram[0x100000];
static u8 rd(void* u, u32 a) { (void)u; return g_ram[a % sizeof g_ram]; }
static void wr(void* u, u32 a, u8 v) { (void)u; g_ram[a % sizeof g_ram] = v; }

#define EXI0 0xCC006800u

static void sel(BluewakeIplSram* d, u32 dev) {  // EXISelect(0, dev, 3)
    u32 cpr = bluewake_ipl_sram_read(d, EXI0) & 0x405u;
    cpr |= ((1u << dev) << 7) | (3u * 0x10u);
    bluewake_ipl_sram_write(d, EXI0, cpr, rd, wr, NULL);
}
static void desel(BluewakeIplSram* d) {
    bluewake_ipl_sram_write(d, EXI0, bluewake_ipl_sram_read(d, EXI0) & 0x405u, rd, wr, NULL);
}
static void imm(BluewakeIplSram* d, const u8* buf, u32 len, u32 type) {  // EXIImm + EXISync
    if (type != 0u) {
        u32 data = 0;
        for (u32 i = 0; i < len; i++) data |= (u32)buf[i] << ((3 - i) * 8);
        bluewake_ipl_sram_write(d, EXI0 + 0x10, data, rd, wr, NULL);
    }
    bluewake_ipl_sram_write(d, EXI0 + 0x0C, (type << 2) | 1u | ((len - 1u) << 4), rd, wr, NULL);
    assert((bluewake_ipl_sram_read(d, EXI0 + 0x0C) & 1u) == 0u);  // EXISync
}
static void dma_read(BluewakeIplSram* d, u32 addr, u32 len) {
    bluewake_ipl_sram_write(d, EXI0 + 0x04, addr, rd, wr, NULL);
    bluewake_ipl_sram_write(d, EXI0 + 0x08, len, rd, wr, NULL);
    bluewake_ipl_sram_write(d, EXI0 + 0x0C, 3u, rd, wr, NULL);
    assert((bluewake_ipl_sram_read(d, EXI0 + 0x0C) & 1u) == 0u);
}
static void read_sram(BluewakeIplSram* d, u32 addr) {  // ReadSram
    const u8 cmd[4] = {0x20, 0x00, 0x01, 0x00};
    sel(d, 1);
    imm(d, cmd, 4, 1);
    dma_read(d, addr, 64);
    desel(d);
}

int main(void) {
    BluewakeIplSram d;

    // Off: the recorded behaviour, nothing claimed.
    bluewake_ipl_sram_init(&d, NULL);
    assert(!bluewake_ipl_sram_contains(&d, EXI0));
    bluewake_ipl_sram_init(&d, "0");
    assert(!d.enabled);

    // Defaults: Dolphin's SRAM, stereo, checksums valid.
    bluewake_ipl_sram_init(&d, "default");
    assert(bluewake_ipl_sram_contains(&d, EXI0) && !bluewake_ipl_sram_contains(&d, EXI0 + 0x14));
    memset(g_ram, 0xEE, sizeof g_ram);
    read_sram(&d, 0x3E0);
    const u8* s = g_ram + 0x3E0;
    assert(s[0] == 0x00 && s[1] == 0x2C && s[2] == 0xFF && s[3] == 0xD0);
    assert(s[19] == 0x2C && (s[19] & 0x04));  // OSGetSoundMode: stereo
    assert(memcmp(s + 20, "DOLPHINSLOTA", 12) == 0);
    assert(g_ram[0x3E0 + 64] == 0xEE);  // exactly 64 bytes

    // A probe of device 0 (the memory card) still sees no card and no data.
    sel(&d, 0);
    assert((bluewake_ipl_sram_read(&d, EXI0) & 0x1000u) == 0u);
    const u8 id_cmd[2] = {0, 0};
    imm(&d, id_cmd, 2, 1);
    u8 dummy[4] = {0};
    imm(&d, dummy, 4, 0);
    assert(bluewake_ipl_sram_read(&d, EXI0 + 0x10) == 0u);
    desel(&d);

    // WriteSram(buffer + 19, 19, 45) after OSSetSoundMode(mono), persisted.
    const char* path = "ipl_sram_test.bin";
    remove(path);
    bluewake_ipl_sram_init(&d, path);
    FILE* f = fopen(path, "rb");
    assert(f != NULL);
    fclose(f);
    u8 image[64];
    memcpy(image, d.sram, 64);
    image[19] &= (u8)~0x04u;
    const u32 offset = 19u;
    const u32 cmd = 0xA0000100u + (offset << 6);
    const u8 cb[4] = {(u8)(cmd >> 24), (u8)(cmd >> 16), (u8)(cmd >> 8), (u8)cmd};
    sel(&d, 1);
    imm(&d, cb, 4, 1);
    for (u32 left = 64u - offset, at = offset; left > 0;) {  // EXIImmEx
        const u32 n = left < 4u ? left : 4u;
        imm(&d, image + at, n, 1);
        at += n;
        left -= n;
    }
    desel(&d);
    assert(memcmp(d.sram, image, 64) == 0);
    BluewakeIplSram again;
    bluewake_ipl_sram_init(&again, path);
    assert(memcmp(again.sram, image, 64) == 0 && (again.sram[19] & 0x04u) == 0u);
    memset(g_ram, 0, sizeof g_ram);
    read_sram(&again, 0x1000);
    assert(g_ram[0x1000 + 19] == image[19]);
    remove(path);

    printf("ipl_sram_test: ok\n");
    return 0;
}
