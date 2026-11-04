# Native comparison tests: Linux port (for #179) — done, but blocked by stale v1 gate

## What was built

tests/posix/bw_posix_test_shim.h — POSIX shim for the Windows loader surface
the native_* tests use (LoadLibraryA->dlopen, GetProcAddress->dlsym,
VirtualAlloc->mmap, VirtualProtect->mprotect, QPC->CLOCK_MONOTONIC_RAW,
_putenv_s->setenv). tests/posix/windows.h resolves the tests' `#include
<windows.h>` to it, so the test sources compile UNMODIFIED on Linux.

tests/posix/build_native_tests.sh — builds native_search_test and
native_mtxcalc_test against build/linux-native/composite/gGZLE01_recomp.so,
linking cmake/composite/native_{search,mtxcalc}.c + direct_calls.c +
libgxruntime.a.

## Result

Both tests compile and load the .so. The .so (glibc 2.38) runs in this
container (glibc 2.41). The natives now REPORT (vs 0/15 certified on the
Windows 0.6.0 build) — the .so exports all needed symbols
(staticrecomp_get_module, bluewake_composite_guest_cpu/mem1,
bluewake_composite_direct_calls[_v2], bluewake_composite_edge_filter,
ppc_guest_alias_add_shared, bluewake_set_edge_service).

BUT the tests gate on the v1 handshake:
  direct(true, &clear, &clear, &zero, &zero)  -> bluewake_composite_direct_calls
  filter(true)                                 -> bluewake_composite_edge_filter

In HEAD, bluewake_composite_direct_calls (v1) is a hard stub that ALWAYS
returns 0 (the donor handshake can't describe BlueWake's extra host
observations). Only bluewake_composite_direct_calls_v2 (with a can_skip
callback) works. So the tests print "the module's direct calls or edge
filter are unavailable" and exit 1 before running any cases.

## Root cause (independent of #179's hash question)

The tests were written at 552ce1f, before 68b1ff6 "Integrate direct calls and
guarded register inlining with BlueWake host boundaries" introduced v2 and
stubbed v1. The tests were never updated to call v2 with a can_skip callback.

## What this means for #179

Two separate fixes are needed, not one:
1. (already known) Regenerate the certified hashes against a BlueWake module.
2. (new) Update native_search_test.c / native_mtxcalc_test.c (and any other
   native_* test that calls bluewake_composite_direct_calls) to use the v2
   handshake with a can_skip callback, matching direct_calls.h.

Both are test-side, not .so-side. The .so needs no change.

## Repro

  bash tests/posix/build_native_tests.sh build/linux-native/composite/gGZLE01_recomp.so
  ./build/linux-native/native-tests/native_search_test build/linux-native/composite/gGZLE01_recomp.so
