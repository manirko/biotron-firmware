#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "debussy_mode.h"

enum { TRACE_BEATS = 192 };

static DebussyInput still_input(unsigned int beat) {
    DebussyInput input = {
        .plant_energy = (uint8_t)(beat < 24u ? 18u : 6u),
        .plant_direction = DEBUSSY_DIRECTION_STABLE,
        .gesture = DEBUSSY_GESTURE_STABLE,
        .light_level = (uint8_t)(48u + (beat / 48u)),
        .light_change = beat % 48u == 0u ? 4u : 0u,
    };
    return input;
}

static DebussyInput varied_input(unsigned int beat) {
    DebussyInput input = {
        .plant_energy = (uint8_t)((beat * 17u + 11u) % 96u),
        .plant_direction = beat % 13u < 5u ? DEBUSSY_DIRECTION_RISING
                                           : (beat % 13u < 9u
                                                  ? DEBUSSY_DIRECTION_STABLE
                                                  : DEBUSSY_DIRECTION_FALLING),
        .gesture = beat == 37u || beat == 101u || beat == 149u
                           ? DEBUSSY_GESTURE_TOUCH
                           : DEBUSSY_GESTURE_DRIFT,
        .light_level = (uint8_t)((beat * 3u + 24u) % 128u),
        .light_change = beat % 29u == 0u ? 24u : 3u,
    };
    return input;
}

static bool collection_contains(uint8_t collection, uint8_t root,
                                uint8_t note) {
    static const uint16_t masks[DEBUSSY_COLLECTION_COUNT] = {
        UINT16_C(0x555), /* whole tone: 0, 2, 4, 6, 8, 10 */
        UINT16_C(0x295), /* major pentatonic: 0, 2, 4, 7, 9 */
        UINT16_C(0x2a5), /* suspended pentatonic: 0, 2, 5, 7, 9 */
        UINT16_C(0xad5), /* Lydian: 0, 2, 4, 6, 7, 9, 11 */
        UINT16_C(0x6ad), /* Dorian: 0, 2, 3, 5, 7, 9, 10 */
    };
    assert(collection < DEBUSSY_COLLECTION_COUNT);
    const uint8_t pitch_class = (uint8_t)((note + 12u - root % 12u) % 12u);
    return (masks[collection] & (UINT16_C(1) << pitch_class)) != 0u;
}

static uint32_t hash_decision(uint32_t hash, const DebussyDecision *decision) {
    const uint8_t *bytes = (const uint8_t *)decision;
    for (size_t i = 0; i < sizeof(*decision); ++i) {
        hash ^= bytes[i];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

static void assert_decision_invariants(const DebussyDecision *decision) {
    assert(decision->scene < DEBUSSY_SCENE_COUNT);
    assert(decision->phase < DEBUSSY_PHASE_COUNT);
    assert(decision->collection < DEBUSSY_COLLECTION_COUNT);
    assert(decision->phrase_length == 8u || decision->phrase_length == 10u ||
           decision->phrase_length == 12u);
    assert(decision->beat_in_phrase < decision->phrase_length);
    assert(decision->active_voice_count <= 3u);
    assert(decision->pedal_note >= DEBUSSY_NOTE_MIN);
    assert(decision->pedal_note <= DEBUSSY_NOTE_MAX);
    assert(decision->colour_note >= DEBUSSY_NOTE_MIN);
    assert(decision->colour_note <= DEBUSSY_NOTE_MAX);
    assert(collection_contains(decision->collection, decision->root,
                               decision->pedal_note));
    assert(collection_contains(decision->collection, decision->root,
                               decision->colour_note));
    if (decision->melody_on) {
        assert(decision->melody_note >= DEBUSSY_NOTE_MIN);
        assert(decision->melody_note <= DEBUSSY_NOTE_MAX);
        assert(decision->melody_velocity > 0u);
        assert(decision->melody_duration_beats >= 1u);
        assert(collection_contains(decision->collection, decision->root,
                                   decision->melody_note));
    } else {
        assert(decision->melody_note == DEBUSSY_REST);
        assert(decision->melody_velocity == 0u);
        assert(decision->melody_duration_beats == 0u);
    }
}

static void test_state_is_fixed_and_small(void) {
    assert(sizeof(DebussyState) <= 512u);
}

static void test_fixed_seed_is_exact_and_repeatable(void) {
    DebussyState left;
    DebussyState right;
    debussy_init(&left, UINT32_C(0x4d595df4));
    debussy_init(&right, UINT32_C(0x4d595df4));

    uint32_t trace_hash = UINT32_C(2166136261);
    for (unsigned int beat = 0; beat < TRACE_BEATS; ++beat) {
        const DebussyInput input = varied_input(beat);
        const DebussyDecision a = debussy_step(&left, &input);
        const DebussyDecision b = debussy_step(&right, &input);
        assert(memcmp(&a, &b, sizeof(a)) == 0);
        assert_decision_invariants(&a);
        trace_hash = hash_decision(trace_hash, &a);
    }
    assert(memcmp(&left, &right, sizeof(left)) == 0);
    assert(trace_hash == UINT32_C(0x2541021d));
}

static void test_different_seed_changes_surface_not_rules(void) {
    DebussyState left;
    DebussyState right;
    debussy_init(&left, UINT32_C(0x4d595df4));
    debussy_init(&right, UINT32_C(0x9e3779b9));
    unsigned int differences = 0u;

    for (unsigned int beat = 0; beat < TRACE_BEATS; ++beat) {
        const DebussyInput input = varied_input(beat);
        const DebussyDecision a = debussy_step(&left, &input);
        const DebussyDecision b = debussy_step(&right, &input);
        assert_decision_invariants(&a);
        assert_decision_invariants(&b);
        if (memcmp(&a, &b, sizeof(a)) != 0) ++differences;
    }
    assert(differences > TRACE_BEATS / 4u);
}

static void test_collection_changes_only_on_phrase_boundaries(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    uint8_t previous_collection = DEBUSSY_COLLECTION_COUNT;
    unsigned int changes = 0u;

    for (unsigned int beat = 0; beat < TRACE_BEATS * 3u; ++beat) {
        const DebussyInput input = varied_input(beat);
        const DebussyDecision decision = debussy_step(&state, &input);
        if (previous_collection < DEBUSSY_COLLECTION_COUNT &&
            decision.collection != previous_collection) {
            assert(decision.phrase_boundary);
            assert(decision.beat_in_phrase == 0u);
            ++changes;
        }
        previous_collection = decision.collection;
    }
    assert(changes > 0u);
}

static void test_stable_input_converges_to_sparse_anchor(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    unsigned int late_onsets = 0u;
    unsigned int late_pitch_span_min = 128u;
    unsigned int late_pitch_span_max = 0u;

    for (unsigned int beat = 0; beat < TRACE_BEATS; ++beat) {
        const DebussyInput input = still_input(beat);
        const DebussyDecision decision = debussy_step(&state, &input);
        assert_decision_invariants(&decision);
        if (beat >= 64u && decision.melody_on) {
            ++late_onsets;
            if (decision.melody_note < late_pitch_span_min)
                late_pitch_span_min = decision.melody_note;
            if (decision.melody_note > late_pitch_span_max)
                late_pitch_span_max = decision.melody_note;
        }
    }
    assert(late_onsets >= 20u);
    assert(late_onsets <= 52u); /* 16-41% after settling. */
    assert(late_pitch_span_max - late_pitch_span_min <= 12u);
}

static void test_one_touch_advances_at_most_one_phase_and_accents_once(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    DebussyInput input = still_input(0u);
    DebussyDecision decision = debussy_step(&state, &input);

    for (unsigned int beat = 1; beat < 16u; ++beat) {
        input = still_input(beat);
        decision = debussy_step(&state, &input);
    }

    const uint8_t before = decision.phase;
    unsigned int accents = 0u;
    unsigned int phase_advances = 0u;
    uint8_t previous = before;
    for (unsigned int beat = 16u; beat < 24u; ++beat) {
        input = still_input(beat);
        input.gesture = DEBUSSY_GESTURE_TOUCH;
        input.plant_energy = 127u;
        decision = debussy_step(&state, &input);
        if (decision.accent) ++accents;
        if (decision.phase > previous) {
            assert(decision.phase == (uint8_t)(previous + 1u));
            ++phase_advances;
        }
        previous = decision.phase;
    }
    assert(accents == 1u);
    assert(phase_advances <= 1u);
    assert(previous <= (uint8_t)(before + 1u));
}

static void test_boundary_touch_does_not_skip_a_phase(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    DebussyInput input = still_input(0u);
    (void)debussy_step(&state, &input);
    while (state.beat_in_phrase != 0u) {
        input = still_input(state.beat_index);
        (void)debussy_step(&state, &input);
    }

    const uint8_t before = state.phase;
    input = still_input(state.beat_index);
    input.gesture = DEBUSSY_GESTURE_TOUCH;
    input.plant_energy = 127u;
    const DebussyDecision decision = debussy_step(&state, &input);
    const uint8_t expected = before == DEBUSSY_PHASE_RELEASE
                                 ? DEBUSSY_PHASE_CALM
                                 : (uint8_t)(before + 1u);
    assert(decision.phrase_boundary);
    assert(decision.accent);
    assert(decision.phase == expected);
}

static unsigned int count_phase_onsets(uint8_t phase) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x6a09e667));
    DebussyInput input = varied_input(1u);
    input.plant_energy = 40u;
    input.gesture = DEBUSSY_GESTURE_DRIFT;
    unsigned int onsets = 0u;
    (void)debussy_step(&state, &input);

    for (unsigned int beat = 0; beat < 1000u; ++beat) {
        state.phase = phase;
        state.phrase_length = 12u;
        state.beat_in_phrase = 1u; /* Keep the density sample off boundaries. */
        state.stable_beats = 0u;
        const DebussyDecision decision = debussy_step(&state, &input);
        if (decision.melody_on) ++onsets;
    }
    return onsets;
}

static void test_density_rises_from_calm_to_crest(void) {
    const unsigned int calm = count_phase_onsets(DEBUSSY_PHASE_CALM);
    const unsigned int grow = count_phase_onsets(DEBUSSY_PHASE_GROW);
    const unsigned int crest = count_phase_onsets(DEBUSSY_PHASE_CREST);
    assert(calm < grow);
    assert(grow < crest);
}

static void test_noisy_light_does_not_retrigger_colour(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x9e3779b9));
    unsigned int changes_after_initial_phrase = 0u;

    for (unsigned int beat = 0; beat < TRACE_BEATS; ++beat) {
        DebussyInput input = still_input(beat);
        input.light_level = (uint8_t)(62u + beat % 5u);
        input.light_change = (uint8_t)(beat % 3u);
        const DebussyDecision decision = debussy_step(&state, &input);
        if (beat >= 16u && decision.colour_changed)
            ++changes_after_initial_phrase;
    }
    assert(changes_after_initial_phrase == 0u);
}

static void test_melodic_repeats_and_leaps_are_bounded(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x9e3779b9));
    uint8_t previous_note = DEBUSSY_REST;
    unsigned int repeats = 0u;

    for (unsigned int beat = 0; beat < TRACE_BEATS * 3u; ++beat) {
        const DebussyInput input = varied_input(beat);
        const DebussyDecision decision = debussy_step(&state, &input);
        if (!decision.melody_on) continue;
        if (previous_note == decision.melody_note) {
            ++repeats;
            assert(repeats <= 2u);
        } else {
            if (previous_note != DEBUSSY_REST && !decision.accent &&
                decision.phase != DEBUSSY_PHASE_CREST) {
                const int leap = (int)decision.melody_note - (int)previous_note;
                assert(leap >= -7 && leap <= 7);
            }
            repeats = 0u;
        }
        previous_note = decision.melody_note;
    }
}

static uint32_t fuzz_next(uint32_t *state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static void test_ten_thousand_extreme_sensor_beats(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x243f6a88));
    uint32_t fuzz = UINT32_C(0xb7e15162);
    uint8_t previous_collection = DEBUSSY_COLLECTION_COUNT;
    uint8_t previous_note = DEBUSSY_REST;
    uint8_t previous_gesture = DEBUSSY_GESTURE_STABLE;
    unsigned int repeats = 0u;

    for (unsigned int beat = 0; beat < 10000u; ++beat) {
        const uint32_t bits = fuzz_next(&fuzz);
        DebussyInput input = {
            .plant_energy = (uint8_t)(bits & 0x7fu),
            .plant_direction = (int8_t)((int)((bits >> 8) % 3u) - 1),
            .gesture = bits % 97u == 0u
                           ? DEBUSSY_GESTURE_TOUCH
                           : ((bits & 0x7fu) < 20u ? DEBUSSY_GESTURE_STABLE
                                                  : DEBUSSY_GESTURE_DRIFT),
            .light_level = (uint8_t)((bits >> 16) & 0x7fu),
            .light_change = (uint8_t)((bits >> 24) & 0x7fu),
        };
        const uint8_t phase_before = state.phase;
        const DebussyDecision decision = debussy_step(&state, &input);
        assert_decision_invariants(&decision);
        if (previous_collection < DEBUSSY_COLLECTION_COUNT &&
            decision.collection != previous_collection) {
            assert(decision.phrase_boundary);
        }
        if (input.gesture == DEBUSSY_GESTURE_TOUCH &&
            previous_gesture != DEBUSSY_GESTURE_TOUCH) {
            const uint8_t expected = phase_before == DEBUSSY_PHASE_RELEASE
                                         ? DEBUSSY_PHASE_CALM
                                         : (uint8_t)(phase_before + 1u);
            assert(decision.accent);
            assert(decision.phase == expected);
        }
        if (decision.melody_on) {
            if (decision.melody_note == previous_note) {
                ++repeats;
                assert(repeats <= 2u);
            } else {
                if (previous_note != DEBUSSY_REST && !decision.accent &&
                    decision.phase != DEBUSSY_PHASE_CREST) {
                    const int leap =
                        (int)decision.melody_note - (int)previous_note;
                    assert(leap >= -7 && leap <= 7);
                }
                repeats = 0u;
            }
            previous_note = decision.melody_note;
        }
        previous_collection = decision.collection;
        previous_gesture = input.gesture;
    }
}

int main(void) {
    test_state_is_fixed_and_small();
    test_fixed_seed_is_exact_and_repeatable();
    test_different_seed_changes_surface_not_rules();
    test_collection_changes_only_on_phrase_boundaries();
    test_stable_input_converges_to_sparse_anchor();
    test_one_touch_advances_at_most_one_phase_and_accents_once();
    test_boundary_touch_does_not_skip_a_phase();
    test_density_rises_from_calm_to_crest();
    test_noisy_light_does_not_retrigger_colour();
    test_melodic_repeats_and_leaps_are_bounded();
    test_ten_thousand_extreme_sensor_beats();
    puts("debussy_mode: golden trace and musical invariants passed");
    return 0;
}
