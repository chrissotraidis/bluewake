/* Synthetic vectors and host readiness; no game input. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_game_math.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE 0x400000u
static u8* ram;
static const u32 entries[] = {0x80245674,0x802456C4,0x80245714,0x8024A8E0,
    0x8000CD28,0x8000CDC8,0x8000CE68,0x8024AE3C,0x80256888,0x802569D0,0x802F072C,0x802F0954};
static CPUState state(void) {
    memset(ram,0,RAM_SIZE);
    for (unsigned i=0;i<3;++i) {
        write_be32(ram+0x2000+4*i,0x3f800000); /* 1 */
        write_be32(ram+0x3000+4*i,0x40000000); /* 2 */
    }
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;
    c.gpr[1]=GC_RAM_BASE+0x4000;c.gpr[3]=GC_RAM_BASE+0x1000;
    c.gpr[4]=GC_RAM_BASE+0x2000;c.gpr[5]=GC_RAM_BASE+0x3000;
    c.pc=entries[0];c.lr=0x80001003;c.msr=PPC_MSR_FP;c.hid2=PPC_HID2_LSQE;
    c.cycle_budget=100000;c.cycle_deadline_budget=100000;ppc_fpscr_updated(&c);return c;
}
static void decline(CPUState c,u32 entry) {
    CPUState before=c;u8* saved=malloc(RAM_SIZE);assert(saved);memcpy(saved,ram,RAM_SIZE);
    assert(!bluewake_native_game_math(&c,entry));
    assert(!memcmp(&c,&before,sizeof c));assert(!memcmp(saved,ram,RAM_SIZE));free(saved);
}
static unsigned queries,internal_queries;
static bool allow_entry,allow_internal;
static bool ready(void* user,const CPUState* c,u32 address) {
    (void)user;++queries;assert(c);
    if(address==0x80328F40) {
        ++internal_queries;assert(c->pc==address && c->lr==0x8024AEC8);
        assert(c->gpr[1]==GC_RAM_BASE+0x4000-512 && c->gpr[11]==c->gpr[1]+272);
        return allow_internal;
    }
    return allow_entry;
}
static void journal(u32 a,u32 n,void* p) { (void)a;(void)n;(void)p;assert(0); }
int main(void) {
    ram=calloc(1,RAM_SIZE);assert(ram);
    CPUState c=state(),before=c;
    assert(!bluewake_native_game_math_try(&c,entries[0]));
    assert(!bluewake_composite_native_game_math_v1(true,NULL,NULL));
    assert(bluewake_composite_native_game_math_v1(true,ready,NULL));
    assert(!bluewake_native_game_math_try(&c,entries[0]));assert(!memcmp(&c,&before,sizeof c));
    allow_entry=true;
    assert(bluewake_native_game_math_try(&c,entries[0]));assert(c.pc==0x80001000 && c.downcount==-30);
    for(unsigned i=0;i<3;++i)assert(read_be32(ram+0x1000+4*i)==0x40400000);
    c=state();before=c;
    u8* saved=malloc(RAM_SIZE);assert(saved);memcpy(saved,ram,RAM_SIZE);
    assert(!bluewake_native_game_math_try(&c,0x8024AE3C));assert(internal_queries==1);
    assert(!memcmp(&c,&before,sizeof c));assert(!memcmp(saved,ram,RAM_SIZE));free(saved);
    assert(!bluewake_composite_native_game_math_v1(false,ready,NULL));
    assert(!bluewake_native_game_math_try(&c,entries[0]));
    for(unsigned i=0;i<sizeof entries/sizeof entries[0];++i) {
        assert(!bluewake_native_game_math(NULL,entries[i]));
        assert(!bluewake_native_game_math_try(NULL,entries[i]));
        c=state();c.ram=NULL;decline(c,entries[i]);
    }
    c=state();decline(c,0);
    c=state();c.ram_size=1;decline(c,entries[0]);
    c=state();c.exception=1;decline(c,entries[0]);
    c=state();c.msr=0;decline(c,entries[0]);
    c=state();c.hid2=0;decline(c,entries[0]);
    c=state();c.fpscr=1;decline(c,entries[0]);
    c=state();c.gqr[0]=0x00040004;decline(c,entries[0]);
    c=state();c.cycle_budget=1;decline(c,entries[0]);
    c=state();c.cycle_deadline_budget=1;decline(c,entries[0]);
    c=state();c.gpr[3]=c.gpr[4];decline(c,entries[0]);
    c=state();g_mem_write_journal=journal;decline(c,entries[0]);g_mem_write_journal=NULL;
    c=state();g_ppc_guest_aliases_overlap_mem1=true;decline(c,entries[0]);g_ppc_guest_aliases_overlap_mem1=false;
    free(ram);puts("Game-math guards: arithmetic, entry/internal readiness and unchanged CPU/RAM fallback pass");return 0;
}
