#include "delivery_digest.h"

#include <stddef.h>

#define BLUEWAKE_FNV64_PRIME 1099511628211ull

static void mix_byte(u64* hash, u8 value) {
    *hash ^= value;
    *hash *= BLUEWAKE_FNV64_PRIME;
}

static void mix_u32(u64* hash, u32 value) {
    for (unsigned shift = 24u;; shift -= 8u) {
        mix_byte(hash, (u8)(value >> shift));
        if (shift == 0u)
            break;
    }
}

static void mix_u64(u64* hash, u64 value) {
    for (unsigned shift = 56u;; shift -= 8u) {
        mix_byte(hash, (u8)(value >> shift));
        if (shift == 0u)
            break;
    }
}

void bluewake_delivery_digest_init(BluewakeDeliveryDigest* digest) {
    if (digest == NULL)
        return;
    *digest = (BluewakeDeliveryDigest){0};
    digest->hash = BLUEWAKE_DELIVERY_FNV64_OFFSET;
    digest->hash_no_cycle = BLUEWAKE_DELIVERY_FNV64_OFFSET;
    digest->play_hash = BLUEWAKE_DELIVERY_FNV64_OFFSET;
    digest->play_hash_no_cycle = BLUEWAKE_DELIVERY_FNV64_OFFSET;
}

void bluewake_delivery_digest_record_external(
    BluewakeDeliveryDigest* digest, u64 cycle, u32 cause, u32 pc,
    u32 context, bool includes_dsp, bool in_play) {
    if (digest == NULL)
        return;

    mix_u64(&digest->hash, cycle);
    mix_u32(&digest->hash, cause);
    mix_u32(&digest->hash, pc);
    mix_u32(&digest->hash, context);

    mix_u32(&digest->hash_no_cycle, cause);
    mix_u32(&digest->hash_no_cycle, pc);
    mix_u32(&digest->hash_no_cycle, context);
    digest->cycle_sum += cycle;
    digest->external_count++;

    if (digest->external_history_count <
        BLUEWAKE_DELIVERY_EXTERNAL_HISTORY_CAPACITY) {
        BluewakeDeliveryPoint* point =
            &digest->external_history[digest->external_history_count++];
        point->ordinal = digest->external_count;
        point->cycle = cycle;
        point->prefix_hash = digest->hash;
        point->cause = cause;
        point->pc = pc;
        point->context = context;
    } else {
        digest->external_history_overflow++;
    }

    if (includes_dsp) {
        digest->dsp_count++;
        if (digest->dsp_sample_count <
            BLUEWAKE_DELIVERY_DSP_SAMPLE_CAPACITY) {
            BluewakeDeliveryPoint* point =
                &digest->dsp_samples[digest->dsp_sample_count++];
            point->ordinal = digest->external_count;
            point->cycle = cycle;
            point->prefix_hash = digest->hash;
            point->cause = cause;
            point->pc = pc;
            point->context = context;
        }
    }

    if (includes_dsp && !digest->first_dsp_valid) {
        digest->first_dsp_valid = true;
        digest->first_dsp_cycle = cycle;
        digest->first_dsp_cause = cause;
        digest->first_dsp_pc = pc;
        digest->first_dsp_context = context;
    }

    if (in_play) {
        mix_u64(&digest->play_hash, cycle);
        mix_u32(&digest->play_hash, cause);
        mix_u32(&digest->play_hash, pc);
        mix_u32(&digest->play_hash, context);
        mix_u32(&digest->play_hash_no_cycle, cause);
        mix_u32(&digest->play_hash_no_cycle, pc);
        mix_u32(&digest->play_hash_no_cycle, context);
        if (digest->play_count == 0u)
            digest->play_first_cycle = cycle;
        digest->play_last_cycle = cycle;
        digest->play_cycle_sum += cycle;
        digest->play_count++;
    }
}
