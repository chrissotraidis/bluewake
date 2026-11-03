/* Included before every translated chunk on ELF targets (Linux, Steam Deck,
 * Android): see the note above its use in CMakeLists.txt.
 *
 * Order matters, because a symbol keeps the visibility of its first declaration:
 *   1. the system headers, so libc, libm and pthreads stay imports;
 *   2. the runtime's header, so its whole API keeps its own visibility (the host
 *      resolves some of it from the module, e.g. ppc_guest_alias_resolve);
 *   3. the runtime data the chunks touch on nearly every guest memory access,
 *      defined in this module and never exported, declared hidden;
 *   4. everything the generated chunk headers declare, hidden.
 * Hidden symbols are referenced directly instead of through the GOT. Leaving
 * something out of step 3 only costs speed; hiding an export would break the
 * host, which is why the runtime's header comes before the pragma. */
#ifndef BLUEWAKE_HIDDEN_EXTERNS_H
#define BLUEWAKE_HIDDEN_EXTERNS_H

#include <fenv.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include DOLRECOMP_CPU_HEADER

extern __attribute__((visibility("hidden"))) bool g_ppc_guest_aliases_overlap_mem1;
extern __attribute__((visibility("hidden"))) PPCMemWriteJournal g_mem_write_journal;
extern __attribute__((visibility("hidden"))) void* g_mem_write_journal_user;
#if defined(BW_GUEST_MEM1)
extern __attribute__((visibility("hidden"))) u8 BW_GUEST_MEM1[];
#endif

#pragma GCC visibility push(hidden)

#endif
