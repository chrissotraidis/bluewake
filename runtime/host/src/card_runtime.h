#ifndef BLUEWAKE_CARD_RUNTIME_H
#define BLUEWAKE_CARD_RUNTIME_H

#include "core/cpu.h"
#include "gxruntime/memory_card.h"

#include <stdbool.h>

// Opens BlueWake's default-on slot-A card. An explicit path is intended for
// isolated tests; NULL selects the per-user macOS application-support path.
bool bluewake_card_runtime_open(const char* explicit_path);
void bluewake_card_runtime_close(void);
void bluewake_card_runtime_suspend_writes(bool suspend);
// Turns saving off for the rest of the run: the card answers as one that has
// been taken out, so the game says it cannot save and writes nothing. There is
// no way back on; the card file stays as it was. `why` is logged and must outlive it.
// With `cpu`, the game's no-card messages say so at once, and a save state taken
// from here on carries them: loading one in any later run turns saving off again.
void bluewake_card_runtime_refuse_writes(CPUState* cpu, const char* why);
bool bluewake_card_runtime_writes_refused(void);
// BLUEWAKE_STAGE_SELECT_SAVES=1: refusing does nothing, and saving stays on.
bool bluewake_card_runtime_saving_kept(void);
const char* bluewake_card_runtime_path(void);

// How much the guest has asked the card to write. The in-game save is the
// guest's own write of its own buffer, so these are the honest in-run signal
// that a save happened at all; the card file's own contents are checked
// against them after the run.
u64 bluewake_card_runtime_write_calls(void);
u64 bluewake_card_runtime_write_bytes(void);

// Services queued asynchronous completions and GZLE01's public CARD SDK
// entry points while leaving the retail synchronous wrappers in control.
void bluewake_card_runtime_service_callback(CPUState* cpu);
bool bluewake_card_runtime_intercepts(u32 address);
bool bluewake_card_runtime_dispatch(CPUState* cpu);

#endif
