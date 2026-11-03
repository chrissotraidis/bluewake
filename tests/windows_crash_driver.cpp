#include "../windows/src/win_crash.h"
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    bw_crash_install(argv[1]);
    bw_crash_test();
    return 0;
}
