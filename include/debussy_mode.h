#ifndef BIOTRON_DEBUSSY_MODE_H
#define BIOTRON_DEBUSSY_MODE_H

#include <stdbool.h>
#include <stdint.h>

enum {
    DEBUSSY_NOTE_MIN = 36,
    DEBUSSY_NOTE_MAX = 96,
    DEBUSSY_REST = 255,
    DEBUSSY_MOTIF_CAPACITY = 5,
};

typedef enum {
    DEBUSSY_SCENE_VEILS = 0,
    DEBUSSY_SCENE_PAGODAS,
    DEBUSSY_SCENE_CATHEDRAL,
    DEBUSSY_SCENE_COUNT,
} DebussyScene;

typedef enum {
    DEBUSSY_PHASE_CALM = 0,
    DEBUSSY_PHASE_GROW,
    DEBUSSY_PHASE_CREST,
    DEBUSSY_PHASE_RELEASE,
    DEBUSSY_PHASE_COUNT,
} DebussyPhase;

typedef enum {
    DEBUSSY_COLLECTION_WHOLE_TONE = 0,
    DEBUSSY_COLLECTION_MAJOR_PENTATONIC,
    DEBUSSY_COLLECTION_SUSPENDED_PENTATONIC,
    DEBUSSY_COLLECTION_LYDIAN,
    DEBUSSY_COLLECTION_DORIAN,
    DEBUSSY_COLLECTION_COUNT,
} DebussyCollection;

typedef enum {
    DEBUSSY_DIRECTION_FALLING = -1,
    DEBUSSY_DIRECTION_STABLE = 0,
    DEBUSSY_DIRECTION_RISING = 1,
} DebussyDirection;

typedef enum {
    DEBUSSY_GESTURE_STABLE = 0,
    DEBUSSY_GESTURE_DRIFT,
    DEBUSSY_GESTURE_TOUCH,
} DebussyGesture;

/* Normalized sensor features supplied once per musical beat. */
typedef struct {
    uint8_t plant_energy;
    int8_t plant_direction;
    uint8_t gesture;
    uint8_t light_level;
    uint8_t light_change;
} DebussyInput;

/* Pure musical intention. MIDI lifecycle remains the adapter's concern. */
typedef struct {
    uint8_t scene;
    uint8_t phase;
    uint8_t collection;
    uint8_t root;
    uint8_t phrase_length;
    uint8_t beat_in_phrase;
    uint8_t melody_note;
    uint8_t melody_velocity;
    uint8_t melody_duration_beats;
    uint8_t pedal_note;
    uint8_t colour_note;
    uint8_t active_voice_count;
    bool phrase_boundary;
    bool melody_on;
    bool accent;
    bool colour_changed;
} DebussyDecision;

/* All engine memory is caller-owned and fixed-size. */
typedef struct {
    uint32_t random_state;
    uint32_t beat_index;
    uint8_t scene;
    uint8_t phase;
    uint8_t phrase_length;
    uint8_t beat_in_phrase;
    uint8_t collection;
    uint8_t root;
    uint8_t motif[DEBUSSY_MOTIF_CAPACITY];
    uint8_t motif_length;
    uint8_t motif_index;
    uint8_t phrase_count;
    uint8_t stable_beats;
    uint8_t touch_latched;
    uint8_t light_band;
    uint8_t light_candidate_band;
    uint8_t light_candidate_beats;
    uint8_t pending_light_band;
    uint8_t pedal_note;
    uint8_t colour_note;
    uint8_t last_melody_note;
    uint8_t stable_anchor_note;
    uint8_t repeated_notes;
    int8_t current_degree;
} DebussyState;

void debussy_init(DebussyState *state, uint32_t seed);
DebussyDecision debussy_step(DebussyState *state, const DebussyInput *input);

#endif
