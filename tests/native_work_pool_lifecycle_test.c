#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>

typedef int (*FixtureRun)(uint32_t salt);

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    for (unsigned iteration = 0; iteration < 100; ++iteration) {
        void *library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
        if (!library) {
            fprintf(stderr, "load %u failed: %s\n", iteration, dlerror());
            return 1;
        }
        FixtureRun run = (FixtureRun)dlsym(library, "bluewake_pool_fixture_run");
        const char *symbol_error = dlerror();
        if (symbol_error || !run) {
            fprintf(stderr, "symbol %u failed: %s\n", iteration,
                    symbol_error ? symbol_error : "missing export");
            return 1;
        }
        int result = run(UINT32_C(0x9e3779b9) ^ iteration);
        if (result) {
            fprintf(stderr, "run %u failed: %d\n", iteration, result);
            return 1;
        }
        if (dlclose(library) != 0) {
            fprintf(stderr, "unload %u failed: %s\n", iteration, dlerror());
            return 1;
        }
    }
    return 0;
}
