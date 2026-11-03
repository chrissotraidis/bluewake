/* Synthetic one-joint identity model; no disc or translated game source. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_skin.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE 0x400000u
static u8* ram;
static void put(u32 offset, u32 value) { write_be32(ram + offset, value); }
static CPUState state(void) {
    memset(ram, 0, RAM_SIZE);
    put(0x1004, GC_RAM_BASE + 0x2000);
    put(0x1084, GC_RAM_BASE + 0x7000); /* scale flags */
    put(0x1088, GC_RAM_BASE + 0x8000); /* envelope flags */
    put(0x108c, GC_RAM_BASE + 0x9000); /* nodes */
    put(0x1090, GC_RAM_BASE + 0xa000); /* output */
    write_be16(ram + 0x2030, 1);
    put(0x2034, GC_RAM_BASE + 0x3000); /* joint counts */
    put(0x2038, GC_RAM_BASE + 0x4000); /* indices */
    put(0x203c, GC_RAM_BASE + 0x5000); /* weights */
    put(0x2040, GC_RAM_BASE + 0x6000); /* inverse */
    ram[0x3000] = 1; ram[0x7000] = 1;
    put(0x5000, 0x3f800000);
    for (unsigned i=0; i<12; ++i) {
        put(0x6000 + 4*i, (i==0 || i==5 || i==10) ? 0x3f800000 : 0);
        put(0x9000 + 4*i, (i==0 || i==5 || i==10) ? 0x3f800000 : 0);
    }
    put(0x2f85cc, 0x3f800000); /* r13-31288: unit 0,1 */
    CPUState c = {0}; c.ram=ram; c.ram_size=RAM_SIZE;
    c.gpr[1]=GC_RAM_BASE+0x201000; c.gpr[3]=GC_RAM_BASE+0x1000;
    c.gpr[13]=GC_RAM_BASE+0x300000; c.pc=BLUEWAKE_NATIVE_SKIN_ENTRY;
    c.lr=0x80001003; c.msr=PPC_MSR_FP; c.hid2=PPC_HID2_LSQE;
    c.cycle_budget=100000; c.cycle_deadline_budget=100000;
    ppc_fpscr_updated(&c); return c;
}
static void decline(CPUState c) {
    CPUState before=c; u8* saved=malloc(RAM_SIZE); assert(saved);
    memcpy(saved,ram,RAM_SIZE);
    assert(!bluewake_native_skin(&c));
    assert(!memcmp(&c,&before,sizeof c));
    assert(!memcmp(ram,saved,RAM_SIZE)); free(saved);
}
static void journal(u32 a,u32 n,void* p) { (void)a;(void)n;(void)p;assert(0); }
static bool ready(void* p,const CPUState* c,u32 entry) {
    assert(c && entry==BLUEWAKE_NATIVE_SKIN_ENTRY); return *(bool*)p;
}
int main(void) {
    ram=calloc(1,RAM_SIZE);assert(ram);
    CPUState c=state(), before=c;
    assert(!bluewake_native_skin_try(&c)); assert(!memcmp(&c,&before,sizeof c));
    assert(!bluewake_composite_native_skin_v1(true,NULL,NULL));
    bool allow=false;
    assert(bluewake_composite_native_skin_v1(true,ready,&allow));
    assert(!bluewake_native_skin_try(&c)); assert(!memcmp(&c,&before,sizeof c));
    allow=true;assert(bluewake_native_skin_try(&c));
    assert(c.pc==0x80001000 && c.downcount==-141);
    assert(!memcmp(ram+0xa000,ram+0x9000,48));assert(ram[0x8000]==1);
    assert(!bluewake_composite_native_skin_v1(false,ready,&allow));
    c=state();assert(!bluewake_native_skin_try(&c));
    assert(!bluewake_native_skin(NULL));assert(!bluewake_native_skin_try(NULL));
    c=state();c.ram=NULL;decline(c);
    c=state();c.ram_size=1;decline(c);
    c=state();c.exception=1;decline(c);
    c=state();c.msr=0;decline(c);
    c=state();c.hid2=0;decline(c);
    c=state();c.gqr[0]=0x00040004;decline(c);
    c=state();c.fpscr=1;decline(c);
    c=state();c.cycle_budget=1;decline(c);
    c=state();c.cycle_deadline_budget=1;decline(c);
    c=state();c.gpr[3]=0xcc000000;decline(c);
    c=state();put(0x1090,GC_RAM_BASE+0x9000);decline(c); /* output aliases nodes */
    c=state();put(0x5000,0x7f800000);decline(c);
    c=state();g_mem_write_journal=journal;decline(c);g_mem_write_journal=NULL;
    free(ram);puts("Skin guards: identity, handshake and unchanged CPU/RAM fallback pass");return 0;
}
