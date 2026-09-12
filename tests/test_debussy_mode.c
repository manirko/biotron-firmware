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
        const int colour_delta = (int)decision->melody_note -
                                 (int)decision->colour_note;
        const int colour_interval = colour_delta < 0 ? -colour_delta
                                                     : colour_delta;
        assert(colour_interval >= 2 && colour_interval <= 9);
        assert(decision->colour_note != decision->pedal_note);
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
    assert(trace_hash == UINT32_C(0x81911f13));
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

static void test_stable_input_stays_silent(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));

    for (unsigned int beat = 0; beat < TRACE_BEATS; ++beat) {
        const DebussyInput input = still_input(beat);
        const DebussyDecision decision = debussy_step(&state, &input);
        assert_decision_invariants(&decision);
        assert(!decision.melody_on);
        assert(decision.active_voice_count == 0u);
    }
}

static void test_light_motion_answers_immediately_then_returns_to_rest(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));

    for (unsigned int beat = 0; beat < 12u; ++beat) {
        const DebussyInput input = still_input(beat);
        const DebussyDecision decision = debussy_step(&state, &input);
        assert(decision.active_voice_count == 0u);
    }

    DebussyInput input = still_input(12u);
    input.light_level = 18u;
    input.light_change = 36u;
    DebussyDecision decision = debussy_step(&state, &input);
    assert(decision.structural_response);
    assert(decision.melody_on);
    assert(decision.active_voice_count == 3u);

    unsigned int sounding_beats = 1u;
    for (unsigned int beat = 13u; beat < 40u; ++beat) {
        input = still_input(beat);
        input.light_level = 18u;
        input.light_change = 0u;
        decision = debussy_step(&state, &input);
        if (decision.active_voice_count > 0u) ++sounding_beats;
    }
    assert(sounding_beats == 8u);
    assert(decision.active_voice_count == 0u);
}

static void test_plant_drift_requires_a_quiet_baseline(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    DebussyInput input = still_input(0u);
    input.gesture = DEBUSSY_GESTURE_DRIFT;
    input.plant_energy = 64u;

    for (unsigned int beat = 0u; beat < 8u; ++beat) {
        const DebussyDecision decision = debussy_step(&state, &input);
        assert(decision.active_voice_count == 0u);
    }

    for (unsigned int beat = 0u; beat < 4u; ++beat) {
        input = still_input(beat);
        (void)debussy_step(&state, &input);
    }
    input.gesture = DEBUSSY_GESTURE_DRIFT;
    input.plant_energy = 64u;
    const DebussyDecision response = debussy_step(&state, &input);
    assert(response.structural_response);
    assert(response.melody_on);
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

static void advance_to_touch_test(DebussyState *state, unsigned int beats) {
    for (unsigned int beat = 0; beat < beats; ++beat) {
        const DebussyInput input = still_input(beat);
        (void)debussy_step(state, &input);
    }
}

static void test_touch_creates_one_complete_short_form_arc(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    assert(state.touch_theme == 0u);
    advance_to_touch_test(&state, 24u);
    assert(state.touch_theme == 0u);

    uint8_t origin = DEBUSSY_REST;
    unsigned int onsets = 0u;
    unsigned int surprises = 0u;
    unsigned int surprise_offset = UINT32_MAX;
    uint8_t reveal_note = DEBUSSY_REST;
    uint8_t closing_note = DEBUSSY_REST;

    for (unsigned int offset = 0; offset <= 12u; ++offset) {
        DebussyInput input = still_input(24u + offset);
        if (offset == 0u) {
            input.gesture = DEBUSSY_GESTURE_TOUCH;
            input.plant_energy = 120u;
            input.plant_direction = DEBUSSY_DIRECTION_RISING;
        } else if (offset < 5u) {
            input.gesture = DEBUSSY_GESTURE_DRIFT;
            input.plant_energy = 48u;
            input.plant_direction = DEBUSSY_DIRECTION_RISING;
        }

        const DebussyDecision decision = debussy_step(&state, &input);
        assert_decision_invariants(&decision);
        if (offset == 0u) {
            origin = state.touch_origin_note;
            assert(decision.accent);
            assert(decision.melody_on); /* proof of control is immediate */
            assert(decision.landmark_note > origin);
        }
        if (decision.melody_on) {
            ++onsets;
            if (decision.arc_stage == DEBUSSY_ARC_RETURN)
                closing_note = decision.melody_note;
        }
        if (decision.surprise) {
            ++surprises;
            surprise_offset = offset;
            reveal_note = decision.melody_note;
            assert(decision.arc_stage == DEBUSSY_ARC_REVEAL);
        }
    }

    /* A hook must expose a learnable phrase, not a few isolated landmarks. */
    assert(onsets == 13u);
    assert(surprises == 1u);
    assert(surprise_offset >= 3u && surprise_offset <= 6u);
    assert(reveal_note >= state.touch_origin_note);
    assert(reveal_note == state.landmark_note);
    assert(closing_note != DEBUSSY_REST);
    assert((int)closing_note - (int)origin <= 3);
    assert((int)closing_note - (int)origin >= -3);
}

static void test_touch_arc_states_an_authored_four_note_identity(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    advance_to_touch_test(&state, 24u);

    uint8_t notes[5] = {0};
    for (unsigned int offset = 0; offset < 5u; ++offset) {
        DebussyInput input = still_input(24u + offset);
        input.gesture = offset == 0u ? DEBUSSY_GESTURE_TOUCH
                                     : DEBUSSY_GESTURE_DRIFT;
        input.plant_energy = 80u;
        input.plant_direction = DEBUSSY_DIRECTION_RISING;
        const DebussyDecision decision = debussy_step(&state, &input);
        assert(decision.melody_on);
        notes[offset] = decision.melody_note;
    }

    /* 0-a-b-a is heard before the fifth-note reveal. */
    assert(notes[1] > notes[0]);
    assert(notes[2] > notes[1]);
    assert(notes[3] == notes[1]);
    assert(notes[4] >= notes[2]);
}

static void test_touch_direction_changes_the_promised_landmark(void) {
    DebussyState rising;
    DebussyState falling;
    debussy_init(&rising, UINT32_C(0x9e3779b9));
    debussy_init(&falling, UINT32_C(0x9e3779b9));
    advance_to_touch_test(&rising, 18u);
    advance_to_touch_test(&falling, 18u);
    assert(rising.last_melody_note == DEBUSSY_REST);
    assert(falling.last_melody_note == DEBUSSY_REST);

    DebussyInput up = still_input(18u);
    up.gesture = DEBUSSY_GESTURE_TOUCH;
    up.plant_energy = 120u;
    up.plant_direction = DEBUSSY_DIRECTION_RISING;
    DebussyInput down = up;
    down.plant_direction = DEBUSSY_DIRECTION_FALLING;

    const DebussyDecision up_decision = debussy_step(&rising, &up);
    const DebussyDecision down_decision = debussy_step(&falling, &down);
    const uint8_t origin = rising.touch_origin_note;
    assert(origin == falling.touch_origin_note);
    assert(up_decision.landmark_note > origin);
    assert(down_decision.landmark_note < origin);
    assert(up_decision.landmark_note != down_decision.landmark_note);
}

static void test_repeated_touches_cannot_spam_surprises(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x243f6a88));
    advance_to_touch_test(&state, 20u);
    unsigned int surprises = 0u;
    unsigned int structural_responses = 0u;

    for (unsigned int offset = 0; offset < 16u; ++offset) {
        DebussyInput input = still_input(20u + offset);
        if (offset % 2u == 0u) {
            input.gesture = DEBUSSY_GESTURE_TOUCH;
            input.plant_energy = 120u;
            input.plant_direction = DEBUSSY_DIRECTION_RISING;
        }
        const DebussyDecision decision = debussy_step(&state, &input);
        if (decision.surprise) ++surprises;
        if (decision.structural_response) ++structural_responses;
    }
    assert(surprises == 1u);
    assert(structural_responses == 1u);
}

static unsigned int count_phase_onsets(uint8_t phase) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x6a09e667));
    DebussyInput input = varied_input(1u);
    input.plant_energy = 40u;
    input.gesture = DEBUSSY_GESTURE_DRIFT;
    unsigned int onsets = 0u;
    (void)debussy_step(&state, &input);
    state.engagement_beats_left = 12u;
    for (unsigned int beat = 1u; beat < 12u; ++beat) {
        state.phase = phase;
        state.phrase_length = 12u;
        state.beat_in_phrase = (uint8_t)beat;
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

static void test_randomness_never_decides_inside_a_phrase(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    DebussyInput input = varied_input(0u);
    (void)debussy_step(&state, &input);
    const uint32_t phrase_random_state = state.random_state;

    while (state.beat_in_phrase != 0u) {
        input = varied_input(state.beat_index);
        (void)debussy_step(&state, &input);
        assert(state.random_state == phrase_random_state);
    }
}

static void test_each_phrase_returns_to_the_motif_beginning(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    DebussyInput input = still_input(0u);
    (void)debussy_step(&state, &input);
    state.motif_index = 3u;
    state.beat_in_phrase = 0u;
    state.beat_index = state.phrase_length;
    (void)debussy_step(&state, &input);
    assert(state.motif_index <= 1u);
}

static void test_collection_contrast_keeps_the_common_root(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x4d595df4));
    DebussyInput input = still_input(0u);
    (void)debussy_step(&state, &input);
    const uint8_t root = state.root;
    state.phrase_count = 1u;
    state.beat_in_phrase = 0u;
    state.beat_index = state.phrase_length;
    (void)debussy_step(&state, &input);
    assert(state.collection != DEBUSSY_COLLECTION_SUSPENDED_PENTATONIC);
    assert(state.root == root);
}

static void test_noisy_light_does_not_retrigger_scene(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x9e3779b9));
    uint8_t settled_scene = DEBUSSY_SCENE_COUNT;
    unsigned int scene_changes = 0u;

    for (unsigned int beat = 0; beat < TRACE_BEATS; ++beat) {
        DebussyInput input = still_input(beat);
        input.light_level = (uint8_t)(62u + beat % 5u);
        input.light_change = (uint8_t)(beat % 3u);
        const DebussyDecision decision = debussy_step(&state, &input);
        if (beat == 15u) settled_scene = decision.scene;
        if (beat >= 16u && decision.scene != settled_scene) ++scene_changes;
    }
    assert(scene_changes == 0u);
}

static void test_colour_voice_shadows_the_melody(void) {
    DebussyState state;
    debussy_init(&state, UINT32_C(0x9e3779b9));
    uint8_t previous_melody = DEBUSSY_REST;
    uint8_t previous_colour = DEBUSSY_REST;
    unsigned int melody_moves = 0u;
    unsigned int parallel_moves = 0u;
    unsigned int colour_moves = 0u;

    for (unsigned int beat = 0; beat < TRACE_BEATS * 3u; ++beat) {
        const DebussyInput input = varied_input(beat);
        const DebussyDecision decision = debussy_step(&state, &input);
        if (previous_colour != DEBUSSY_REST) {
            if (decision.colour_note != previous_colour)
                assert(decision.colour_changed);
            if (decision.colour_changed && !decision.phrase_boundary)
                assert(decision.colour_note != previous_colour);
        }
        if (!decision.melody_on) {
            previous_colour = decision.colour_note;
            continue;
        }

        const int interval = (int)decision.melody_note -
                             (int)decision.colour_note;
        assert(interval >= 2 && interval <= 9);
        assert(decision.colour_note != decision.pedal_note);
        if (previous_melody != DEBUSSY_REST &&
            decision.melody_note != previous_melody) {
            ++melody_moves;
            const int melody_direction = (int)decision.melody_note -
                                         (int)previous_melody;
            const int colour_direction = (int)decision.colour_note -
                                         (int)previous_colour;
            if (colour_direction != 0 &&
                (melody_direction < 0) == (colour_direction < 0)) {
                ++parallel_moves;
            }
        }
        if (previous_colour != DEBUSSY_REST &&
            decision.colour_note != previous_colour) {
            ++colour_moves;
        }
        previous_melody = decision.melody_note;
        previous_colour = decision.colour_note;
    }

    assert(melody_moves > 20u);
    assert(colour_moves > 20u);
    assert(parallel_moves * 4u >= melody_moves * 3u);
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
            assert(decision.accent);
            if (decision.structural_response &&
                decision.arc_stage == DEBUSSY_ARC_PROMISE) {
                const uint8_t expected =
                    phase_before == DEBUSSY_PHASE_RELEASE
                        ? DEBUSSY_PHASE_CALM
                        : (uint8_t)(phase_before + 1u);
                assert(decision.phase == expected);
            } else if (!decision.phrase_boundary) {
                assert(decision.phase == phase_before);
            }
        }
        if (decision.melody_on) {
            if (decision.structural_response) {
                /* A re-articulated origin is the user's downbeat, not drift. */
                repeats = 0u;
            } else if (decision.melody_note == previous_note) {
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
    test_stable_input_stays_silent();
    test_light_motion_answers_immediately_then_returns_to_rest();
    test_plant_drift_requires_a_quiet_baseline();
    test_one_touch_advances_at_most_one_phase_and_accents_once();
    test_boundary_touch_does_not_skip_a_phase();
    test_touch_creates_one_complete_short_form_arc();
    test_touch_arc_states_an_authored_four_note_identity();
    test_touch_direction_changes_the_promised_landmark();
    test_repeated_touches_cannot_spam_surprises();
    test_density_rises_from_calm_to_crest();
    test_randomness_never_decides_inside_a_phrase();
    test_each_phrase_returns_to_the_motif_beginning();
    test_collection_contrast_keeps_the_common_root();
    test_noisy_light_does_not_retrigger_scene();
    test_colour_voice_shadows_the_melody();
    test_melodic_repeats_and_leaps_are_bounded();
    test_ten_thousand_extreme_sensor_beats();
    puts("debussy_mode: golden trace and musical invariants passed");
    return 0;
}
