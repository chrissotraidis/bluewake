#ifdef NDEBUG
#undef NDEBUG
#endif
#include "delivery_digest.h"

#include <assert.h>

int main(void) {
    BluewakeDeliveryDigest first;
    BluewakeDeliveryDigest second;
    bluewake_delivery_digest_init(&first);
    bluewake_delivery_digest_init(&second);
    assert(first.hash == second.hash);
    assert(first.external_count == 0u);
    assert(first.external_history_count == 0u);
    assert(first.external_history_overflow == 0u);
    assert(!first.first_dsp_valid);

    bluewake_delivery_digest_record_external(
        &first, 12600u, 0x140u, 0x8028E988u, 0x803A2960u, true, false);
    bluewake_delivery_digest_record_external(
        &second, 12600u, 0x140u, 0x8028E988u, 0x803A2960u, true, false);
    assert(first.hash == second.hash);
    assert(first.external_count == 1u);
    assert(first.external_history_count == 1u);
    assert(first.external_history[0].ordinal == 1u);
    assert(first.external_history[0].cycle == 12600u);
    assert(first.external_history[0].prefix_hash == first.hash);
    assert(first.external_history[0].cause == 0x140u);
    assert(first.external_history[0].pc == 0x8028E988u);
    assert(first.external_history[0].context == 0x803A2960u);
    assert(first.first_dsp_valid);
    assert(first.first_dsp_cycle == 12600u);
    assert(first.first_dsp_cause == 0x140u);
    assert(first.first_dsp_pc == 0x8028E988u);
    assert(first.first_dsp_context == 0x803A2960u);
    assert(first.dsp_count == 1u);
    assert(first.dsp_sample_count == 1u);
    assert(first.dsp_samples[0].ordinal == 1u);
    assert(first.dsp_samples[0].cycle == 12600u);
    assert(first.dsp_samples[0].prefix_hash == first.hash);
    assert(first.dsp_samples[0].pc == 0x8028E988u);

    const u64 prior_hash = first.hash;
    bluewake_delivery_digest_record_external(
        &first, 25200u, 0x100u, 0x80307EF4u, 0x803F06D0u, false, false);
    assert(first.external_count == 2u);
    assert(first.external_history_count == 2u);
    assert(first.external_history[1].ordinal == 2u);
    assert(first.external_history[1].cycle == 25200u);
    assert(first.external_history[1].prefix_hash == first.hash);
    assert(first.external_history[1].cause == 0x100u);
    assert(first.external_history[1].pc == 0x80307EF4u);
    assert(first.external_history[1].context == 0x803F06D0u);
    assert(first.hash != prior_hash);
    assert(first.first_dsp_cycle == 12600u);
    assert(first.dsp_count == 1u);
    assert(first.dsp_sample_count == 1u);

    // The play-scene accumulators are gated on the host's play-scene
    // milestone rather than on the certified route prefix, so deliveries
    // before that milestone must leave them untouched and deliveries after
    // it must move them without disturbing the route-wide history.
    assert(first.play_count == 0u);
    assert(first.play_hash == BLUEWAKE_DELIVERY_FNV64_OFFSET);

    BluewakeDeliveryDigest play_only;
    bluewake_delivery_digest_init(&play_only);
    const u64 route_hash_before_play = first.hash;
    bluewake_delivery_digest_record_external(
        &first, 3900000000ull, 0x100u, 0x80307EF4u, 0x803F06D0u, false,
        true);
    assert(first.play_count == 1u);
    assert(first.play_first_cycle == 3900000000ull);
    assert(first.play_last_cycle == 3900000000ull);
    assert(first.play_cycle_sum == 3900000000ull);
    assert(first.play_hash != BLUEWAKE_DELIVERY_FNV64_OFFSET);
    assert(first.play_hash_no_cycle != BLUEWAKE_DELIVERY_FNV64_OFFSET);
    bluewake_delivery_digest_record_external(
        &play_only, first.play_last_cycle, 0x100u, 0x80307EF4u, 0x803F06D0u,
        false, true);
    assert(first.play_hash == play_only.hash);
    assert(first.play_hash_no_cycle == play_only.hash_no_cycle);
    assert(first.play_count == play_only.external_count);
    assert(first.play_hash != first.hash); // route-wide hash includes pre-play deliveries
    assert(first.hash != route_hash_before_play);

    // A second play delivery must accumulate rather than restart.
    bluewake_delivery_digest_record_external(
        &first, 3900000100ull, 0x100u, 0x80307EF4u, 0x803F06D0u, false,
        true);
    assert(first.play_count == 2u);
    assert(first.play_first_cycle == 3900000000ull);
    assert(first.play_last_cycle == 3900000100ull);
    assert(first.play_cycle_sum == 7800000100ull);
    bluewake_delivery_digest_record_external(
        &play_only, first.play_last_cycle, 0x100u, 0x80307EF4u, 0x803F06D0u,
        false, true);
    assert(first.play_hash == play_only.hash);
    assert(first.play_hash_no_cycle == play_only.hash_no_cycle);
    assert(first.play_count == play_only.external_count);
    assert(first.play_hash != first.hash); // route-wide hash includes pre-play deliveries

    BluewakeDeliveryDigest bounded;
    bluewake_delivery_digest_init(&bounded);
    for (u32 index = 0u;
         index < BLUEWAKE_DELIVERY_EXTERNAL_HISTORY_CAPACITY + 1u; index++) {
        bluewake_delivery_digest_record_external(
            &bounded, index, 0x100u, 0x80000000u + index, 0u, false, false);
    }
    assert(bounded.external_count ==
           BLUEWAKE_DELIVERY_EXTERNAL_HISTORY_CAPACITY + 1u);
    assert(bounded.external_history_count ==
           BLUEWAKE_DELIVERY_EXTERNAL_HISTORY_CAPACITY);
    assert(bounded.external_history_overflow == 1u);
    assert(bounded.external_history[0].ordinal == 1u);
    assert(bounded.external_history[
               BLUEWAKE_DELIVERY_EXTERNAL_HISTORY_CAPACITY - 1u]
               .ordinal == BLUEWAKE_DELIVERY_EXTERNAL_HISTORY_CAPACITY);
    return 0;
}
