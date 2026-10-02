#include "native_math.h"
#include "StaticRecompABI.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "module_cpu_contract.h"
#include "native_work_pool.h"
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <math.h>
#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

static u32 seed=0x36E21A57;
static u32 random_u32(void) { seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed; }
static float random_float(void) {
    u32 b=random_u32();
    // Include cancellation, both signed zeros and a broad ordinary exponent
    // range, without spending most of the test on rejected NaN/denorm inputs.
    b=(b&0x807FFFFFu) | ((100+(b%50))<<23);
    if ((b&31)==0) b&=0x80000000;
    float f;memcpy(&f,&b,4);return f;
}
static void write_float(CPUState* c,u32 p,float f) {
    u32 b;memcpy(&b,&f,4);mem_write32(c,p,b);
}
static CPUState initial(u8* ram,u32 pc) {
    CPUState c={0};c.ram=ram;c.ram_size=GC_MAIN_RAM_SIZE;
    c.pc=pc;c.lr=0xFFFFFFFC;c.msr=PPC_MSR_FP;c.hid2=PPC_HID2_LSQE;
    c.cycle_budget=16384;c.cycle_deadline_budget=1000;c.downcount=-17;
    c.fpscr=(random_u32()&0xFFFFF000u); // sticky flags must be retained
    for(unsigned i=0;i<32;++i) {
        c.gpr[i]=random_u32();c.fpr[i]=(double)random_float();c.ps1[i]=(double)random_float();
    }
    c.gpr[1]=0x80004100;c.gpr[3]=0x80001000;c.gpr[4]=0x80002000;c.gpr[5]=0x80003000;
    c.reserve_valid=true;c.reserve_addr=c.gpr[5];
    mem_write32(&c,0x803F66F0,0);mem_write32(&c,0x803F66F4,0x3F800000);
    for(unsigned i=0;i<12;++i) {
        write_float(&c,c.gpr[3]+4*i,random_float());
        write_float(&c,c.gpr[4]+4*i,random_float());
    }
    return c;
}
static void unchanged(CPUState* c,u32 pc) {
    CPUState saved=*c;
    assert(!bluewake_native_math(c,pc));assert(memcmp(c,&saved,sizeof saved)==0);
}
typedef struct Module {
    void* library;
    bool owns_ram;
    const StaticRecompModuleDesc* desc;
    CPUState fallback;
    BlueWakeModuleStorage storage;
    int (*native_math)(bool, BluewakeNativeMathReady, void*);
    void (*report)(void);
} Module;

static int load_module(const char* path, Module* module) {
#if defined(_WIN32)
    HMODULE lib = LoadLibraryA(path);
#define SYMBOL(name) ((void*)GetProcAddress(lib, name))
#else
    void* lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#define SYMBOL(name) dlsym(lib, name)
#endif
    module->library = lib;
    if (!lib) { fprintf(stderr, "cannot load %s\n", path); return 0; }
    StaticRecompGetModuleFn get = (StaticRecompGetModuleFn)SYMBOL(STATICRECOMP_GET_MODULE_SYMBOL);
    if (!get) return 0;
    module->desc = get();
    module->native_math = (int (*)(bool, BluewakeNativeMathReady, void*))SYMBOL("bluewake_composite_native_math_v1");
    module->report = (void (*)(void))SYMBOL("bluewake_native_math_report");
    const char* error = bw_module_select_storage(module->desc,
        (BlueWakeModuleCPUFn)SYMBOL("bluewake_composite_guest_cpu"),
        (BlueWakeModuleMEM1Fn)SYMBOL("bluewake_composite_guest_mem1"),
        &module->fallback, &module->storage);
    if (error) { fprintf(stderr, "%s\n", error); return 0; }
    if (module->storage.mem1 == NULL) {
        module->owns_ram = true;
        module->storage.mem1 = calloc(1, GC_MAIN_RAM_SIZE);
        module->storage.mem1_size = GC_MAIN_RAM_SIZE;
    }
    return module->storage.mem1 != NULL;
#undef SYMBOL
}

static unsigned routed_queries;
static bool routed_ready(void* user, const CPUState* cpu, u32 address) {
    (void)user; (void)cpu; (void)address;
    ++routed_queries;
    return true;
}

static int dispatch(Module* module, CPUState* state, u32 entry) {
    CPUState* guest = module->storage.cpu;
    *guest = *state;
    module->desc->on_state_loaded(guest);
    int result = module->desc->dispatch(guest, entry);
    *state = *guest;
    return result;
}
static void unload(Module* module) {
    if (module->owns_ram) free(module->storage.mem1);
#if defined(_WIN32)
    assert(FreeLibrary((HMODULE)module->library));
#else
    assert(dlclose(module->library) == 0);
#endif
}
int main(int argc,char**argv) {
    if (argc != 1 && argc != 2 && !(argc == 4 && strcmp(argv[2], "--candidate") == 0)) return 2;
    u8* a=calloc(1,GC_MAIN_RAM_SIZE);u8* b=calloc(1,GC_MAIN_RAM_SIZE);assert(a&&b);
    CPUState c=initial(a,0x8030D0FC);
    c.fpscr|=1;unchanged(&c,c.pc);c.fpscr&=~3u;
    c.cycle_deadline_budget=67;unchanged(&c,c.pc);c.cycle_deadline_budget=1000;
    c.gqr[0]=4;unchanged(&c,c.pc);c.gqr[0]=0;
    c.msr=0;unchanged(&c,c.pc);c.msr=PPC_MSR_FP;
    write_float(&c,c.gpr[3],NAN);unchanged(&c,c.pc);
    write_float(&c,c.gpr[3],0x1p-120f);unchanged(&c,c.pc);
    write_float(&c,c.gpr[3],1);c.gpr[5]=0xCC000000;unchanged(&c,c.pc);
    c.gpr[5]=c.gpr[1]-48;unchanged(&c,c.pc);
    c.gpr[5]=0x80003000;c.gpr[1]=0x803F6720;unchanged(&c,c.pc);
    c=initial(a,0x8030DA98);c.gpr[6]=0;unchanged(&c,c.pc);
    c.gpr[6]=1;unchanged(&c,c.pc);
    c.gpr[6]=GC_MAIN_RAM_SIZE/12u+1;unchanged(&c,c.pc);
    c.gpr[6]=4;c.gpr[5]=c.gpr[4]+4;unchanged(&c,c.pc);
    c.gpr[5]=0x817FFFFC;unchanged(&c,c.pc);
    c.gpr[5]=0x80003000;c.cycle_budget=20;unchanged(&c,c.pc);
    for(unsigned scenario=0;scenario<5;++scenario) {
        c=initial(a,0x80328F04);c.gpr[11]=0x80005000;
        if(scenario==0) c.gpr[11]++;
        if(scenario==1) c.gpr[11]=0xCC000000;
        if(scenario==2) c.cycle_budget=20;
        if(scenario==3) c.cycle_deadline_budget=20;
        if(scenario==4) c.exception=1; // any pending exception keeps the original path
        CPUState saved=c;
        assert(!bluewake_native_gpr(&c,c.pc));assert(memcmp(&c,&saved,sizeof c)==0);
    }
    puts("native math: fallback leaves state unchanged");
    if(argc<2) {free(a);free(b);return 0;}
    Module original = {0}, candidate_module = {0};
    assert(load_module(argv[1], &original));
    assert(strcmp(original.desc->game_id, "GZLE01") == 0);
    free(b); b = original.storage.mem1;
    const bool candidate = argc == 4;
    if (candidate) {
        assert(load_module(argv[3], &candidate_module));
        assert(candidate_module.native_math && candidate_module.native_math(true, routed_ready, NULL));
        free(a); a = candidate_module.storage.mem1;
    }
    const u32 entries[]={0x8030D0C8,0x8030D0FC,0x8030DA44};
    for(unsigned fn=0;fn<3;++fn) for(unsigned i=0;i<4000;++i) {
        if (candidate) candidate_module.native_math(i % 31u != 0, routed_ready, NULL);
        memset(a,0,0x10000);c=initial(a,entries[fn]);
        if(fn==0 && i%2) c.gpr[4]=c.gpr[3];
        if(fn==1 && i%3) c.gpr[5]=c.gpr[3+i%3-1];
        if(fn==2 && i%2) c.gpr[5]=c.gpr[4];
        if(i%4==0) c.cycle_deadline_budget=0;
        if(i%4==1) c.cycle_deadline_budget=17+(fn==0?13:fn==1?51:21);
        if(fn==2 && i%7==0) c.ps1[6]=c.ps1[12]=NAN;
        if(candidate && i%13==0) c.cycle_deadline_budget=18;
        if(candidate && i%17==0) c.gqr[0]=0x040004;
        if(candidate && i%19==0) c.msr=0;
        if(candidate && i%23==0) write_float(&c,c.gpr[3],NAN);
        memcpy(b,a,0x10000);memcpy(b+0x3F66F0,a+0x3F66F0,8);
        CPUState reference=c;reference.ram=b;
        assert(dispatch(&original, &reference, reference.pc));
        ppc_fpscr_updated(&c);
        if(candidate) assert(dispatch(&candidate_module, &c, c.pc));
        else assert(bluewake_native_math(&c,c.pc));
        reference.ram=a;
        if(memcmp(&c,&reference,sizeof c) || memcmp(a,b,0x10000)) {
            fprintf(stderr,"mismatch function %08x case %u seed %08x\n",entries[fn],i,seed);
            for(unsigned j=0;j<sizeof c;++j) if(((u8*)&c)[j]!=((u8*)&reference)[j])
                fprintf(stderr,"CPU byte %u got %02x want %02x\n",j,((u8*)&c)[j],((u8*)&reference)[j]);
            for(unsigned j=0;j<0x10000;++j) if(a[j]!=b[j])
                fprintf(stderr,"RAM %08x got %02x want %02x\n",0x80000000+j,a[j],b[j]);
            return 1;
        }
    }
    puts("native math: 12000 complete CPU and 64 KiB RAM-area comparisons against personal module passed");
    const unsigned counts[]={2,3,17,127,1024,4096};
    for(unsigned i=0;i<360;++i) {
        if (candidate) candidate_module.native_math(i % 31u != 0, routed_ready, NULL);
        memset(a,0,0x40000);c=initial(a,0x8030DA98);
        unsigned count=counts[i%6];
        c.gpr[4]=0x80010000;c.gpr[5]=i%2?c.gpr[4]:0x80020000;
        c.gpr[6]=count;c.cycle_budget=100000;c.cycle_deadline_budget=100000;
        c.reserve_valid=true;c.reserve_addr=c.gpr[5]+(i%count)*12;
        for(unsigned j=0;j<count*3;++j) write_float(&c,c.gpr[4]+4*j,random_float());
        if(candidate && i%13==0) c.cycle_deadline_budget=18;
        if(candidate && i%17==0) c.gqr[0]=0x040004;
        if(candidate && i%19==0) c.cycle_budget=40;
        if(candidate && i%23==0) write_float(&c,c.gpr[4]+12*(count-1),NAN);
        memcpy(b,a,0x40000);memcpy(b+0x3F66F0,a+0x3F66F0,8);
        CPUState reference=c;reference.ram=b;
        assert(dispatch(&original, &reference, reference.pc));
        ppc_fpscr_updated(&c);
        if(candidate) assert(dispatch(&candidate_module, &c, c.pc));
        else assert(bluewake_native_math(&c,c.pc));
        reference.ram=a;
        if(memcmp(&c,&reference,sizeof c) || memcmp(a,b,0x40000)) {
            fprintf(stderr,"array mismatch case %u count %u seed %08x\n",i,count,seed);
            for(unsigned j=0;j<sizeof c;++j) if(((u8*)&c)[j]!=((u8*)&reference)[j])
                fprintf(stderr,"CPU byte %u got %02x want %02x\n",j,((u8*)&c)[j],((u8*)&reference)[j]);
            unsigned differences=0;
            for(unsigned j=0;j<0x40000 && differences<32;++j) if(a[j]!=b[j]) {
                fprintf(stderr,"RAM %08x got %02x want %02x\n",0x80000000+j,a[j],b[j]);++differences;
            }
            return 1;
        }
    }
    puts("native arrays: 360 complete CPU and 256 KiB RAM-area comparisons, including in-place and worker batches, passed");
    for(unsigned fn=0;fn<36;++fn) for(unsigned i=0;i<80;++i) {
        u32 entry=(fn<18?0x80328F04:0x80328F50)+4*(fn%18);
        memset(a,0,0x10000);c=initial(a,entry);c.gpr[11]=0x80005000;
        c.reserve_valid=true;c.reserve_addr=c.gpr[11]-4;
        c.msr=0; // integer-only helper must not require FP availability
        for(unsigned j=1;j<=18;++j) mem_write32(&c,c.gpr[11]-4*j,random_u32());
        memcpy(b,a,0x10000);memcpy(b+0x3F66F0,a+0x3F66F0,8);
        CPUState reference=c;reference.ram=b;
        assert(dispatch(&original, &reference, entry));
        ppc_fpscr_updated(&c);assert(bluewake_native_gpr(&c,entry));
        reference.ram=a;
        if(memcmp(&c,&reference,sizeof c) || memcmp(a,b,0x10000)) {
            fprintf(stderr,"GPR mismatch entry %08x case %u\n",entry,i);
            for(unsigned j=0;j<sizeof c;++j) if(((u8*)&c)[j]!=((u8*)&reference)[j])
                fprintf(stderr,"CPU byte %u got %02x want %02x\n",j,((u8*)&c)[j],((u8*)&reference)[j]);
            return 1;
        }
    }
    puts("native GPR: 2880 complete-state comparisons across all 36 entry points passed");
#if !defined(_WIN32)
    const char* workers = getenv("BLUEWAKE_NATIVE_WORKERS");
    if (!candidate && workers && atoi(workers) > 0) assert(bluewake_parallel_batches() > 0);
#endif
    bluewake_native_math_report();
    if (candidate) {
        assert(routed_queries > 0);
        printf("native matrix routing: %u readiness queries\n", routed_queries);
        if (candidate_module.report) candidate_module.report();
        unload(&candidate_module);
    } else free(a);
    unload(&original);
    return 0;
}
