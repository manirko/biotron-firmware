#include "debussy_mode.h"

#include <stddef.h>

enum {
    LIGHT_DARK = 0,
    LIGHT_MIDDLE,
    LIGHT_BRIGHT,
    LIGHT_UNSET = 255,
};

typedef struct {
    uint8_t length;
    uint8_t semitones[7];
} Collection;

static const Collection collections[DEBUSSY_COLLECTION_COUNT] = {
    {6, {0, 2, 4, 6, 8, 10, 0}},
    {5, {0, 2, 4, 7, 9, 0, 0}},
    {5, {0, 2, 5, 7, 9, 0, 0}},
    {7, {0, 2, 4, 6, 7, 9, 11}},
    {7, {0, 2, 3, 5, 7, 9, 10}},
};

static uint32_t random_next(DebussyState *state) {
    uint32_t value = state->random_state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    state->random_state = value;
    return value;
}

static uint8_t clamp_u8(int value, uint8_t minimum, uint8_t maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return (uint8_t)value;
}

static int absolute_int(int value) { return value < 0 ? -value : value; }

static bool note_in_collection(uint8_t collection, uint8_t root, int note) {
    const uint8_t pitch_class = (uint8_t)((note + 120 - root) % 12);
    const Collection *scale = &collections[collection];
    for (uint8_t index = 0; index < scale->length; ++index) {
        if (scale->semitones[index] == pitch_class) return true;
    }
    return false;
}

static int degree_note(uint8_t collection, uint8_t root, int degree,
                       int octave_base) {
    const Collection *scale = &collections[collection];
    int octave = 0;
    while (degree < 0) {
        degree += scale->length;
        --octave;
    }
    while (degree >= scale->length) {
        degree -= scale->length;
        ++octave;
    }
    return octave_base + root + scale->semitones[degree] + octave * 12;
}

static uint8_t nearest_note(uint8_t collection, uint8_t root, int target,
                            uint8_t previous, int maximum_leap) {
    int best_note = DEBUSSY_NOTE_MIN;
    int best_cost = 10000;
    for (int note = DEBUSSY_NOTE_MIN; note <= DEBUSSY_NOTE_MAX; ++note) {
        if (!note_in_collection(collection, root, note)) continue;
        if (previous != DEBUSSY_REST &&
            absolute_int(note - previous) > maximum_leap) {
            continue;
        }
        const int voice_leading = previous == DEBUSSY_REST
                                      ? 0
                                      : absolute_int(note - previous);
        const int cost = absolute_int(note - target) * 3 + voice_leading;
        if (cost < best_cost) {
            best_cost = cost;
            best_note = note;
        }
    }
    return (uint8_t)best_note;
}

static uint8_t directed_nearest_note(uint8_t collection, uint8_t root,
                                     int target, uint8_t previous,
                                     int direction) {
    for (int distance = 0; distance <= 12; ++distance) {
        const int preferred = target + direction * distance;
        if (preferred >= DEBUSSY_NOTE_MIN && preferred <= DEBUSSY_NOTE_MAX &&
            note_in_collection(collection, root, preferred) &&
            (previous == DEBUSSY_REST ||
             absolute_int(preferred - previous) <= 7)) {
            return (uint8_t)preferred;
        }
        const int alternate = target - direction * distance;
        if (alternate >= DEBUSSY_NOTE_MIN && alternate <= DEBUSSY_NOTE_MAX &&
            note_in_collection(collection, root, alternate) &&
            (previous == DEBUSSY_REST ||
             absolute_int(alternate - previous) <= 7)) {
            return (uint8_t)alternate;
        }
    }
    return previous;
}

static uint8_t neighbouring_note(uint8_t collection, uint8_t root,
                                 uint8_t previous, int direction,
                                 int maximum_leap) {
    for (int distance = 1; distance <= maximum_leap; ++distance) {
        const int preferred = previous + direction * distance;
        if (preferred >= DEBUSSY_NOTE_MIN && preferred <= DEBUSSY_NOTE_MAX &&
            note_in_collection(collection, root, preferred)) {
            return (uint8_t)preferred;
        }
        const int alternate = previous - direction * distance;
        if (alternate >= DEBUSSY_NOTE_MIN && alternate <= DEBUSSY_NOTE_MAX &&
            note_in_collection(collection, root, alternate)) {
            return (uint8_t)alternate;
        }
    }
    return previous;
}

static uint8_t touch_arc_stage(const DebussyState *state) {
    if (!state->touch_arc_active) return DEBUSSY_ARC_LISTEN;
    if (state->touch_arc_beat <= 2u) return DEBUSSY_ARC_PROMISE;
    if (state->touch_arc_beat <= 5u) return DEBUSSY_ARC_REVEAL;
    return DEBUSSY_ARC_RETURN;
}

static void start_touch_arc(DebussyState *state, const DebussyInput *input) {
    const int origin_target = state->last_melody_note == DEBUSSY_REST
                                  ? 64
                                  : state->last_melody_note;
    const uint8_t origin = nearest_note(state->collection, state->root,
                                        origin_target, DEBUSSY_REST, 127);
    int direction = input->plant_direction;
    if (direction == DEBUSSY_DIRECTION_STABLE) direction = 1;
    state->touch_arc_direction = (int8_t)direction;
    const int target = (int)origin + direction * 7;
    state->touch_origin_note = origin;
    state->landmark_note = nearest_note(state->collection, state->root,
                                        target, origin, 7);
    state->touch_arc_active = 1u;
    state->touch_arc_beat = 0u;
    state->surprise_cooldown = 16u;
}

static bool touch_arc_onset(const DebussyState *state) {
    return state->touch_arc_active && state->touch_arc_beat <= 12u;
}

static int touch_arc_target(const DebussyState *state) {
    /* Two tiny authored identities: repetition makes the gesture memorable;
       the sensor chooses their direction, never their individual notes. */
    static const int8_t themes[2][13] = {
        {0, 2, 5, 2, 7, 5, 2, 5, 0, 2, 5, 2, 0},
        {0, -2, 3, 0, 7, 3, 0, -2, 0, 3, -2, 3, 0},
    };
    return (int)state->touch_origin_note +
           state->touch_arc_direction *
               themes[state->touch_theme][state->touch_arc_beat];
}

static void advance_touch_arc(DebussyState *state) {
    if (!state->touch_arc_active) return;
    if (state->touch_arc_beat >= 12u) {
        state->touch_arc_active = 0u;
        state->touch_arc_beat = 0u;
        state->motif_index = 0u;
        state->current_degree = (int8_t)state->motif[0];
        return;
    }
    ++state->touch_arc_beat;
}

static uint8_t observed_light_band(uint8_t level) {
    if (level < 40u) return LIGHT_DARK;
    if (level > 88u) return LIGHT_BRIGHT;
    return LIGHT_MIDDLE;
}

static bool observe_light(DebussyState *state, const DebussyInput *input) {
    const uint8_t observed = observed_light_band(input->light_level);
    if (input->light_muted) {
        state->light_was_muted = 1u;
        state->light_motion_latched = 0u;
        state->pending_light_band = LIGHT_UNSET;
        return false;
    }
    if (state->light_was_muted) {
        state->light_was_muted = 0u;
        state->light_band = observed;
        state->light_candidate_band = observed;
        state->light_candidate_beats = 1u;
        state->pending_light_band = observed;
        return false;
    }
    if (state->light_band == LIGHT_UNSET) {
        state->light_band = observed;
        state->light_candidate_band = observed;
        state->light_candidate_beats = 1u;
        state->pending_light_band = LIGHT_UNSET;
        return false;
    }

    const bool band_edge = observed != state->light_band &&
                           observed != state->light_candidate_band;
    bool motion_response = false;
    if ((band_edge || input->light_change >= 8u) &&
        !state->light_motion_latched) {
        state->light_motion_latched = 1u;
        motion_response = true;
    } else if (input->light_change < 4u) {
        state->light_motion_latched = 0u;
    }

    if (observed != state->light_candidate_band) {
        state->light_candidate_band = observed;
        state->light_candidate_beats = 1u;
    } else if (state->light_candidate_beats < UINT8_MAX) {
        ++state->light_candidate_beats;
    }

    const uint8_t required_beats = input->light_change >= 20u ? 3u : 8u;
    if (observed != state->light_band &&
        state->light_candidate_beats >= required_beats) {
        state->pending_light_band = observed;
    }
    return motion_response;
}

static uint8_t scene_for_light(uint8_t band) {
    if (band == LIGHT_DARK) return DEBUSSY_SCENE_VEILS;
    if (band == LIGHT_BRIGHT) return DEBUSSY_SCENE_PAGODAS;
    return DEBUSSY_SCENE_CATHEDRAL;
}

static uint8_t collection_for_scene(uint8_t scene, uint8_t phrase_count) {
    if (scene == DEBUSSY_SCENE_VEILS) {
        return phrase_count % 4u == 3u
                   ? DEBUSSY_COLLECTION_SUSPENDED_PENTATONIC
                   : DEBUSSY_COLLECTION_WHOLE_TONE;
    }
    if (scene == DEBUSSY_SCENE_PAGODAS) {
        return phrase_count % 4u == 3u ? DEBUSSY_COLLECTION_LYDIAN
                                       : DEBUSSY_COLLECTION_MAJOR_PENTATONIC;
    }
    return phrase_count % 3u == 2u ? DEBUSSY_COLLECTION_DORIAN
                                    : DEBUSSY_COLLECTION_SUSPENDED_PENTATONIC;
}

static void generate_motif(DebussyState *state) {
    const uint8_t scale_length = collections[state->collection].length;
    state->motif_length = (uint8_t)(3u + random_next(state) % 3u);
    int degree = (int)(random_next(state) % scale_length);
    state->motif[0] = (uint8_t)degree;
    for (uint8_t index = 1; index < state->motif_length; ++index) {
        const uint32_t roll = random_next(state) % 100u;
        int distance = roll < 8u ? 0 : (roll < 62u ? 1 : (roll < 86u ? 2 : 3));
        if ((random_next(state) & 1u) == 0u) distance = -distance;
        degree += distance;
        if (degree < 0) degree = -degree;
        if (degree >= scale_length)
            degree = (int)scale_length - 1 - (degree - (int)scale_length + 1);
        if (degree < 0) degree = 0;
        state->motif[index] = (uint8_t)degree;
    }
    state->motif_index = 0u;
    state->current_degree = (int8_t)state->motif[0];
}

static void select_harmony(DebussyState *state) {
    const int target = state->light_band == LIGHT_DARK
                           ? 43
                           : (state->light_band == LIGHT_BRIGHT ? 55 : 48);
    state->pedal_note = nearest_note(state->collection, state->root, target,
                                     DEBUSSY_REST, 127);
    static const uint8_t intervals[] = {7, 5, 9, 14, 2};
    state->colour_note = state->pedal_note;
    for (size_t index = 0; index < sizeof(intervals); ++index) {
        const int note = state->pedal_note + intervals[index];
        if (note <= DEBUSSY_NOTE_MAX &&
            note_in_collection(state->collection, state->root, note)) {
            state->colour_note = (uint8_t)note;
            break;
        }
    }
    if (state->colour_note == state->pedal_note) {
        state->colour_note = nearest_note(state->collection, state->root,
                                          state->pedal_note + 7,
                                          DEBUSSY_REST, 127);
    }
}

static uint8_t colour_note_for_melody(const DebussyState *state,
                                      uint8_t melody) {
    /* Light is a harmonising shadow, not a second unrelated melody. Its slow
       band still selects the colour interval, while every melodic move gives
       the companion voice a nearby note in the same collection. */
    const int preferred_interval = state->light_band == LIGHT_DARK
                                       ? 7
                                       : (state->light_band == LIGHT_BRIGHT
                                              ? 3
                                              : 5);
    uint8_t best = state->colour_note;
    int best_cost = 10000;
    for (int interval = 2; interval <= 9; ++interval) {
        const int note = (int)melody - interval;
        if (note < DEBUSSY_NOTE_MIN ||
            note == state->pedal_note ||
            !note_in_collection(state->collection, state->root, note)) {
            continue;
        }
        const int cost = absolute_int(interval - preferred_interval) * 4 +
                         absolute_int(note - state->colour_note);
        if (cost < best_cost) {
            best = (uint8_t)note;
            best_cost = cost;
        }
    }
    return best;
}

static uint8_t next_phase(uint8_t phase) {
    if (phase == DEBUSSY_PHASE_RELEASE) return DEBUSSY_PHASE_CALM;
    return (uint8_t)(phase + 1u);
}

static void update_phase_at_boundary(DebussyState *state,
                                     const DebussyInput *input) {
    if (state->beat_index == 0u) return;
    /* A boundary-aligned touch is handled exactly once by the touch latch. */
    if (input->gesture == DEBUSSY_GESTURE_TOUCH) return;
    if (state->stable_beats >= 20u && input->plant_energy < 20u) {
        state->phase = DEBUSSY_PHASE_CALM;
        return;
    }
    if (input->plant_energy >= 72u) {
        state->phase = next_phase(state->phase);
        return;
    }
    if (state->phase == DEBUSSY_PHASE_CREST) {
        state->phase = DEBUSSY_PHASE_RELEASE;
    } else if (state->phase == DEBUSSY_PHASE_RELEASE) {
        state->phase = DEBUSSY_PHASE_CALM;
    } else if (input->plant_energy >= 32u && state->phrase_count % 2u == 0u) {
        state->phase = next_phase(state->phase);
    }
}

static bool begin_phrase(DebussyState *state, const DebussyInput *input) {
    static const uint8_t phrase_lengths[] = {8, 10, 12};
    bool harmony_changed = false;
    update_phase_at_boundary(state, input);

    if (state->beat_index > 0u) ++state->phrase_count;
    state->phrase_length = phrase_lengths[random_next(state) % 3u];

    /* A promised landmark owns its harmonic field until it has returned. */
    if (state->touch_arc_active) return false;

    if (state->beat_index == 0u) {
        state->scene = scene_for_light(state->light_band);
        harmony_changed = true;
    }

    if (state->pending_light_band != LIGHT_UNSET) {
        state->light_band = state->pending_light_band;
        state->pending_light_band = LIGHT_UNSET;
        state->scene = scene_for_light(state->light_band);
        harmony_changed = true;
    }

    const uint8_t collection = state->stable_beats >= 20u && !harmony_changed
                                   ? state->collection
                                   : collection_for_scene(state->scene,
                                                          state->phrase_count);
    if (collection != state->collection) {
        state->collection = collection;
        /* The root is the common tone bridging contrasting collections. */
        harmony_changed = true;
    }

    if (state->beat_index == 0u) {
        generate_motif(state);
        harmony_changed = true;
    } else if (!harmony_changed && state->phrase_count % 4u == 0u) {
        generate_motif(state);
    }
    state->motif_index = 0u;

    if (harmony_changed) select_harmony(state);
    return harmony_changed;
}

static int transformed_motif_degree(const DebussyState *state) {
    uint8_t index = state->motif_index;
    const uint8_t transform = state->phrase_count % 4u;
    if (transform == 1u)
        index = (uint8_t)((index + 1u) % state->motif_length);
    else if (transform == 2u)
        index = (uint8_t)(state->motif_length - 1u - index);

    int degree = state->motif[index];
    if (transform == 3u) ++degree;
    return degree;
}

static uint8_t melody_note(DebussyState *state, const DebussyInput *input) {
    if (state->touch_arc_active) {
        const int target = touch_arc_target(state);
        const int direction = target >= state->touch_origin_note ? 1 : -1;
        uint8_t note = directed_nearest_note(
            state->collection, state->root, target,
            state->last_melody_note, direction);
        if (state->touch_arc_beat == 4u) note = state->landmark_note;
        if (state->touch_arc_beat == 8u || state->touch_arc_beat == 12u)
            note = state->touch_origin_note;

        if (note == state->last_melody_note) {
            if (state->repeated_notes < UINT8_MAX) ++state->repeated_notes;
        } else {
            state->repeated_notes = 0u;
        }
        state->last_melody_note = note;
        return note;
    }

    int target_degree = transformed_motif_degree(state);
    if (input->gesture == DEBUSSY_GESTURE_DRIFT &&
        state->motif_index % 3u == 2u) {
        target_degree += input->plant_direction;
    }

    int movement = target_degree - state->current_degree;
    if (movement > 2) movement = 2;
    if (movement < -2) movement = -2;
    state->current_degree = (int8_t)(state->current_degree + movement);

    const int octave_base = state->light_band == LIGHT_DARK
                                ? 48
                                : (state->light_band == LIGHT_BRIGHT ? 60 : 55);
    int target = degree_note(state->collection, state->root,
                             state->current_degree, octave_base);
    if (state->stable_beats >= 20u &&
        state->stable_anchor_note != DEBUSSY_REST) {
        const int lower = state->stable_anchor_note - 5;
        const int upper = state->stable_anchor_note + 5;
        if (target < lower) target = lower;
        if (target > upper) target = upper;
    }
    const int maximum_leap = 7;
    uint8_t note = nearest_note(state->collection, state->root, target,
                                state->last_melody_note, maximum_leap);

    if (note == state->last_melody_note && state->repeated_notes >= 2u) {
        note = neighbouring_note(state->collection, state->root, note,
                                 target >= note ? 1 : -1, maximum_leap);
    }

    if (note == state->last_melody_note) {
        if (state->repeated_notes < UINT8_MAX) ++state->repeated_notes;
    } else {
        state->repeated_notes = 0u;
    }
    state->last_melody_note = note;
    state->motif_index = (uint8_t)((state->motif_index + 1u) %
                                   state->motif_length);
    return note;
}

static bool rhythmic_onset(const DebussyState *state) {
    static const uint16_t masks[3][DEBUSSY_PHASE_COUNT] = {
        {UINT16_C(0x049), UINT16_C(0x06d), UINT16_C(0x0ef), UINT16_C(0x049)},
        {UINT16_C(0x129), UINT16_C(0x1ad), UINT16_C(0x1bb), UINT16_C(0x089)},
        {UINT16_C(0x111), UINT16_C(0x555), UINT16_C(0x6db), UINT16_C(0x511)},
    };
    uint8_t length_index = 0u;
    if (state->phrase_length == 10u) length_index = 1u;
    if (state->phrase_length == 12u) length_index = 2u;
    if (state->phase == DEBUSSY_PHASE_CALM && state->stable_beats >= 20u) {
        return state->beat_in_phrase == 0u ||
               state->beat_in_phrase == state->phrase_length / 2u;
    }
    return (masks[length_index][state->phase] &
            (UINT16_C(1) << state->beat_in_phrase)) != 0u;
}

void debussy_init(DebussyState *state, uint32_t seed) {
    *state = (DebussyState){0};
    state->random_state = seed == 0u ? UINT32_C(0x6d2b79f5) : seed;
    state->scene = DEBUSSY_SCENE_CATHEDRAL;
    state->phase = DEBUSSY_PHASE_CALM;
    state->phrase_length = 8u;
    state->collection = DEBUSSY_COLLECTION_SUSPENDED_PENTATONIC;
    state->root = (uint8_t)((seed >> 4) % 12u);
    state->light_band = LIGHT_UNSET;
    state->light_candidate_band = LIGHT_UNSET;
    state->pending_light_band = LIGHT_UNSET;
    state->pedal_note = DEBUSSY_NOTE_MIN;
    state->colour_note = DEBUSSY_NOTE_MIN;
    state->last_melody_note = DEBUSSY_REST;
    state->stable_anchor_note = DEBUSSY_REST;
    state->touch_origin_note = DEBUSSY_REST;
    state->landmark_note = DEBUSSY_REST;
    state->touch_theme = (uint8_t)(seed & 1u);
    state->touch_arc_direction = DEBUSSY_DIRECTION_RISING;
}

DebussyDecision debussy_step(DebussyState *state, const DebussyInput *input) {
    DebussyDecision decision = {0};
    if (state->surprise_cooldown > 0u) --state->surprise_cooldown;
    const bool light_response = observe_light(state, input);

    const bool phrase_boundary = state->beat_in_phrase == 0u;
    bool colour_changed = false;
    if (phrase_boundary) colour_changed = begin_phrase(state, input);

    const uint8_t quiet_beats_before = state->stable_beats;
    if (input->gesture == DEBUSSY_GESTURE_STABLE &&
        input->plant_energy < 20u) {
        if (state->stable_beats < UINT8_MAX) ++state->stable_beats;
        if (state->stable_beats == 20u) {
            state->stable_anchor_note =
                state->last_melody_note == DEBUSSY_REST
                    ? nearest_note(state->collection, state->root, 64,
                                   DEBUSSY_REST, 127)
                    : state->last_melody_note;
        }
    } else {
        state->stable_beats = 0u;
        state->stable_anchor_note = DEBUSSY_REST;
    }

    bool accent = false;
    bool structural_response = false;
    if (input->gesture == DEBUSSY_GESTURE_TOUCH) {
        if (!state->touch_latched) {
            state->touch_latched = 1u;
            accent = true;
            state->engagement_source = DEBUSSY_RESPONSE_PLANT;
            if (!state->touch_arc_active && state->surprise_cooldown == 0u) {
                start_touch_arc(state, input);
                structural_response = true;
                state->phase = next_phase(state->phase);
            }
        }
    } else {
        state->touch_latched = 0u;
    }

    const bool plant_motion_response =
        input->gesture == DEBUSSY_GESTURE_DRIFT &&
        state->previous_gesture == DEBUSSY_GESTURE_STABLE &&
        quiet_beats_before >= 3u &&
        input->plant_energy >= 32u;
    if (light_response || plant_motion_response) {
        structural_response = true;
        state->engagement_beats_left = 8u;
        state->engagement_source = plant_motion_response
                                       ? DEBUSSY_RESPONSE_PLANT
                                       : DEBUSSY_RESPONSE_LIGHT;
        state->motif_index = 0u;
        state->current_degree = (int8_t)state->motif[0];
    }
    if (input->light_muted &&
        state->engagement_source == DEBUSSY_RESPONSE_LIGHT) {
        state->engagement_beats_left = 0u;
        state->engagement_source = DEBUSSY_RESPONSE_NONE;
    }

    const uint8_t arc_stage = touch_arc_stage(state);
    const bool engaged = state->touch_arc_active ||
                         state->engagement_beats_left > 0u;
    const bool melody_on = engaged &&
                           (accent || structural_response ||
                            (state->touch_arc_active
                                 ? touch_arc_onset(state)
                                 : rhythmic_onset(state)));

    decision.scene = state->scene;
    decision.phase = state->phase;
    decision.collection = state->collection;
    decision.root = state->root;
    decision.phrase_length = state->phrase_length;
    decision.beat_in_phrase = state->beat_in_phrase;
    decision.melody_note = DEBUSSY_REST;
    decision.pedal_note = state->pedal_note;
    decision.colour_note = state->colour_note;
    decision.active_voice_count = engaged
                                      ? (uint8_t)(2u + (melody_on ? 1u : 0u))
                                      : 0u;
    decision.phrase_boundary = phrase_boundary;
    decision.melody_on = melody_on;
    decision.accent = accent;
    decision.colour_changed = colour_changed;
    decision.structural_response = structural_response;
    decision.surprise = melody_on && state->touch_arc_active &&
                        state->touch_arc_beat == 4u;
    decision.arc_stage = arc_stage;
    decision.landmark_note = state->landmark_note;

    if (melody_on) {
        decision.melody_note = melody_note(state, input);
        const uint8_t previous_colour = state->colour_note;
        state->colour_note = colour_note_for_melody(
            state, decision.melody_note);
        decision.colour_note = state->colour_note;
        colour_changed = colour_changed ||
                         state->colour_note != previous_colour;
        decision.colour_changed = colour_changed;
        decision.melody_velocity = structural_response
                                       ? 112u
                                       : (accent
                                              ? 84u
                                              : clamp_u8(
                                                    42 + input->plant_energy / 2,
                                                    42u, 104u));
        decision.melody_duration_beats =
            state->phase == DEBUSSY_PHASE_CALM ? 2u : 1u;
    }

    ++state->beat_index;
    ++state->beat_in_phrase;
    if (state->beat_in_phrase >= state->phrase_length)
        state->beat_in_phrase = 0u;
    advance_touch_arc(state);
    if (state->engagement_beats_left > 0u)
        --state->engagement_beats_left;
    if (state->engagement_beats_left == 0u && !state->touch_arc_active)
        state->engagement_source = DEBUSSY_RESPONSE_NONE;
    state->previous_gesture = input->gesture;
    return decision;
}
