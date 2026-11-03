#ifndef BLUEWAKE_DELIVERY_DIGEST_H
#define BLUEWAKE_DELIVERY_DIGEST_H

#include "core/cpu.h"

#include <stdbool.h>

#define BLUEWAKE_DELIVERY_DSP_SAMPLE_CAPACITY 16u
#define BLUEWAKE_DELIVERY_EXTERNAL_HISTORY_CAPACITY 1024u

// The FNV-1a offset basis every digest starts from. A digest that has recorded
// nothing reads this value, so callers and tests can name it instead of
// repeating the constant.
#define BLUEWAKE_DELIVERY_FNV64_OFFSET 14695981039346656037ull

typedef struct BluewakeDeliveryPoint {
    u64 ordinal;
    u64 cycle;
    u64 prefix_hash;
    u32 cause;
    u32 pc;
    u32 context;
} BluewakeDeliveryPoint;

typedef struct BluewakeDeliveryDigest {
    u64 hash;
    // Hash over (cause, pc, context) only. The delivery *identity* is a
    // property of the guest route; the delivery *cycle* is a property of the
    // host turn schedule, so a route invariant that must hold across cycle
    // caps belongs here and not in ->hash.
    u64 hash_no_cycle;
    u64 cycle_sum;
    u64 external_count;
    u32 external_history_count;
    u64 external_history_overflow;
    BluewakeDeliveryPoint
        external_history[BLUEWAKE_DELIVERY_EXTERNAL_HISTORY_CAPACITY];
    bool first_dsp_valid;
    u64 first_dsp_cycle;
    u32 first_dsp_cause;
    u32 first_dsp_pc;
    u32 first_dsp_context;
    u64 dsp_count;
    u32 dsp_sample_count;
    BluewakeDeliveryPoint dsp_samples[BLUEWAKE_DELIVERY_DSP_SAMPLE_CAPACITY];
    // Play-scene accumulators. The recorded history above is the first 1024
    // deliveries and covers cycles 6,689,063-44,638,873 of a 114,210,000,002
    // cycle route, so it is structurally blind to the play scene. These fields
    // are gated on the host play-scene milestone instead, which is the phase
    // that owns playability.
    u64 play_hash;
    u64 play_hash_no_cycle;
    u64 play_cycle_sum;
    u64 play_count;
    u64 play_first_cycle;
    u64 play_last_cycle;
} BluewakeDeliveryDigest;

void bluewake_delivery_digest_init(BluewakeDeliveryDigest* digest);
void bluewake_delivery_digest_record_external(
    BluewakeDeliveryDigest* digest, u64 cycle, u32 cause, u32 pc,
    u32 context, bool includes_dsp, bool in_play);

#endif
