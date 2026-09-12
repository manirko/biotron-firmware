#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debussy_mode.h"

enum { FIXTURE_BEATS = 180 };

static uint8_t absolute_u8_difference(uint8_t left, uint8_t right) {
    return left >= right ? (uint8_t)(left - right) : (uint8_t)(right - left);
}

static DebussyInput still_plant(unsigned int beat) {
    DebussyInput input = {
        .plant_energy = (uint8_t)(beat < 24u ? 18u : 6u),
        .plant_direction = DEBUSSY_DIRECTION_STABLE,
        .gesture = DEBUSSY_GESTURE_STABLE,
        .light_level = (uint8_t)(54u - beat / 60u),
        .light_change = beat % 60u == 0u ? 3u : 0u,
    };
    return input;
}

static DebussyInput touch_arc(unsigned int beat) {
    DebussyInput input = {
        .plant_energy = 8u,
        .plant_direction = DEBUSSY_DIRECTION_STABLE,
        .gesture = DEBUSSY_GESTURE_STABLE,
        .light_level = 64u,
        .light_change = 0u,
    };
    static const unsigned int touches[] = {42u, 66u, 90u};
    for (unsigned int index = 0; index < 3u; ++index) {
        const unsigned int touch = touches[index];
        if (beat == touch) {
            input.plant_energy = 120u;
            input.plant_direction = DEBUSSY_DIRECTION_RISING;
            input.gesture = DEBUSSY_GESTURE_TOUCH;
        } else if (beat > touch && beat <= touch + 7u) {
            input.plant_energy = (uint8_t)(68u - (beat - touch) * 7u);
            input.plant_direction = DEBUSSY_DIRECTION_FALLING;
            input.gesture = DEBUSSY_GESTURE_DRIFT;
        } else if (beat + 4u >= touch && beat < touch) {
            input.plant_energy = (uint8_t)(24u + (beat + 4u - touch) * 9u);
            input.plant_direction = DEBUSSY_DIRECTION_RISING;
            input.gesture = DEBUSSY_GESTURE_DRIFT;
        }
    }
    return input;
}

static uint8_t light_arc_level(unsigned int beat) {
    if (beat < 42u) return 24u;
    if (beat < 82u) return (uint8_t)(24u + (beat - 42u) * 2u);
    if (beat < 116u) return 104u;
    if (beat < 156u) return (uint8_t)(104u - (beat - 116u) * 2u);
    return 24u;
}

static DebussyInput light_arc(unsigned int beat) {
    const uint8_t level = light_arc_level(beat);
    const uint8_t previous = beat == 0u ? level : light_arc_level(beat - 1u);
    DebussyInput input = {
        .plant_energy = 8u,
        .plant_direction = DEBUSSY_DIRECTION_STABLE,
        .gesture = DEBUSSY_GESTURE_STABLE,
        .light_level = level,
        .light_change = absolute_u8_difference(level, previous),
    };
    return input;
}

static const char *scene_name(uint8_t value) {
    static const char *names[] = {"veils", "pagodas", "cathedral"};
    return names[value];
}

static const char *phase_name(uint8_t value) {
    static const char *names[] = {"calm", "grow", "crest", "release"};
    return names[value];
}

static const char *collection_name(uint8_t value) {
    static const char *names[] = {
        "whole_tone", "major_pentatonic", "suspended_pentatonic",
        "lydian", "dorian",
    };
    return names[value];
}

static const char *gesture_name(uint8_t value) {
    static const char *names[] = {"stable", "drift", "touch"};
    return names[value];
}

static const char *arc_stage_name(uint8_t value) {
    static const char *names[] = {"listen", "promise", "reveal", "return"};
    return names[value];
}

static void write_row(unsigned int beat, const DebussyInput *input,
                      const DebussyDecision *decision) {
    printf("%u,%u,%d,%s,%u,%u,%s,%s,%s,%u,%u,%u,", beat,
           input->plant_energy, input->plant_direction,
           gesture_name(input->gesture), input->light_level,
           input->light_change, scene_name(decision->scene),
           phase_name(decision->phase),
           collection_name(decision->collection), decision->root,
           decision->phrase_length, decision->beat_in_phrase);
    if (decision->melody_on) printf("%u", decision->melody_note);
    printf(",%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%s,%u\n",
           decision->melody_velocity,
           decision->melody_duration_beats, decision->pedal_note,
           decision->colour_note, decision->active_voice_count,
           decision->phrase_boundary ? 1u : 0u,
           decision->accent ? 1u : 0u,
           decision->colour_changed ? 1u : 0u,
           decision->structural_response ? 1u : 0u,
           decision->surprise ? 1u : 0u,
           arc_stage_name(decision->arc_stage), decision->landmark_note);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s FIXTURE SEED\n", argv[0]);
        return 2;
    }
    const char *fixture = argv[1];
    char *seed_end = NULL;
    const unsigned long parsed_seed = strtoul(argv[2], &seed_end, 0);
    if (seed_end == argv[2] || *seed_end != '\0' || parsed_seed > UINT32_MAX) {
        fputs("seed must fit uint32_t\n", stderr);
        return 2;
    }

    DebussyInput (*fixture_input)(unsigned int) = NULL;
    unsigned int source_start = 0u;
    unsigned int output_beats = FIXTURE_BEATS;
    if (strcmp(fixture, "still_plant_3m") == 0)
        fixture_input = still_plant;
    else if (strcmp(fixture, "touch_arc_3m") == 0)
        fixture_input = touch_arc;
    else if (strcmp(fixture, "light_arc_3m") == 0)
        fixture_input = light_arc;
    else if (strcmp(fixture, "viral_touch_15s") == 0) {
        fixture_input = touch_arc;
        source_start = 40u;
        output_beats = 15u;
    }
    else {
        fputs("unknown fixture\n", stderr);
        return 2;
    }

    DebussyState state;
    debussy_init(&state, (uint32_t)parsed_seed);
    for (unsigned int beat = 0; beat < source_start; ++beat) {
        const DebussyInput input = fixture_input(beat);
        (void)debussy_step(&state, &input);
    }
    puts("beat,plant_energy,plant_direction,gesture,light_level,light_change,scene,phase,collection,root,phrase_length,beat_in_phrase,melody_note,velocity,duration_beats,pedal_note,colour_note,active_voices,phrase_boundary,accent,colour_changed,structural_response,surprise,arc_stage,landmark_note");
    for (unsigned int beat = 0; beat < output_beats; ++beat) {
        const DebussyInput input = fixture_input(source_start + beat);
        const DebussyDecision decision = debussy_step(&state, &input);
        write_row(beat, &input, &decision);
    }
    return 0;
}
