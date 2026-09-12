#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "pico/time.h"
#include "global.h"
#include "midi_note_lifecycle.h"
#include "music.h"
#include "leds.h"
#include "params.h"
#include "PLSDK/music.h"
#include "debussy_control.h"

typedef struct {
    uint8_t kind;
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
} midi_log_entry_t;

enum {
    LOG_NOTE_ON = 1,
    LOG_NOTE_OFF = 2,
    LOG_ALL_NOTES_OFF = 3,
    LOG_PITCH = 4,
};

Settings_t settings;
bool isMutedByButton = false;
bool TestMode = false;
bool isTestModeGreen = false;
bool LOGGER_FLAG = false;
enum Status status = Active;
enum Status active_status = Active;
uint32_t last_freq = 100;
uint32_t average_freq = 100;
uint32_t average_delta_freq = 1;

static midi_log_entry_t midi_log[64];
static size_t midi_log_len = 0;
static uint8_t calculated_note = 64;
static alarm_callback_t scheduled_callback = NULL;
static void *scheduled_user_data = NULL;
static alarm_id_t scheduled_id = 7;
static bool fail_next_alarm = false;
static size_t led_note_count = 0;
static size_t led_beat_count = 0;
static led_source_t last_led_source = LED_SOURCE_LIGHT;
static uint8_t last_led_note = 0;
static uint8_t last_led_velocity = 0;
static uint16_t light_adc = 1600;

static void log_midi(uint8_t kind, uint8_t channel, uint8_t note,
                     uint8_t velocity) {
    assert(midi_log_len < sizeof midi_log / sizeof midi_log[0]);
    midi_log[midi_log_len++] =
            (midi_log_entry_t){kind, channel, note, velocity};
}

static size_t count_event(uint8_t kind, uint8_t channel, uint8_t note) {
    size_t count = 0;
    for (size_t i = 0; i < midi_log_len; ++i) {
        if (midi_log[i].kind == kind && midi_log[i].channel == channel &&
            midi_log[i].note == note) {
            ++count;
        }
    }
    return count;
}

uint64_t time_us_64(void) { return 1000; }
uint32_t time_us_32(void) { return 1000; }
uint16_t adc_read(void) { return light_adc; }
void light_note_observer(void) {}
void led_music_note_on(led_source_t source, uint8_t note, uint8_t velocity) {
    ++led_note_count;
    last_led_source = source;
    last_led_note = note;
    last_led_velocity = velocity;
}
void led_music_beat(void) { ++led_beat_count; }
void plsdk_printf(const char *format, ...) { (void)format; }
bool print_pure(uint8_t cable, const uint8_t data[], uint8_t len) {
    (void)cable;
    (void)data;
    (void)len;
    return true;
}

alarm_id_t add_alarm_in_us(int64_t delay_us, alarm_callback_t callback,
                           void *user_data, bool fire_if_past) {
    (void)delay_us;
    (void)fire_if_past;
    scheduled_callback = callback;
    scheduled_user_data = user_data;
    if (fail_next_alarm) {
        fail_next_alarm = false;
        return -1;
    }
    return scheduled_id;
}

bool cancel_alarm(alarm_id_t alarm_id) {
    return alarm_id == scheduled_id;
}

int calculate_note_by_scale(uint8_t start_note, int counter,
                            ScaleNums_t scale) {
    (void)scale;
    if (start_note != settings.middle_plant_note) return start_note + counter;
    return calculated_note;
}

void note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    log_midi(LOG_NOTE_ON, channel, note, velocity);
}

void note_off(uint8_t channel, uint8_t note) {
    log_midi(LOG_NOTE_OFF, channel, note, 0u);
}

void change_pitch(uint8_t channel, uint8_t lsb, uint8_t msb) {
    (void)lsb;
    log_midi(LOG_PITCH, channel, msb, 0u);
}

void stop_all_notes(uint8_t channel) {
    log_midi(LOG_ALL_NOTES_OFF, channel, 0, 0u);
}

static void reset_fixture(void) {
    settings = (Settings_t){0};
    settings.fibPower = 0.5;
    settings.firstValue = 0.1;
    settings.scale = SCALE_MAJOR;
    settings.middle_plant_note = 60;
    settings.plant_channel = 5;
    settings.light_channel = 6;
    settings.maxPlantVelocity = 100;
    settings.maxLightVelocity = 100;
    settings.fraction_note_off = 4;
    settings.same_note_plant = 1;
    settings.same_note_light = 1;
    settings.light_note_range = 12;
    settings.lightBPM = 1;
    status = Active;
    active_status = Active;
    isMutedByButton = false;
    midi_log_len = 0;
    scheduled_callback = NULL;
    scheduled_user_data = NULL;
    scheduled_id = 7;
    fail_next_alarm = false;
    last_note_plant = 60;
    reset_plant_note_off();
    stop_light_midi();
    midi_log_len = 0;
    led_note_count = 0;
    led_beat_count = 0;
    light_adc = 1600;
    debussy_control_default(&debussy_control);
    debussy_runtime_reset();
}

static void test_identity_round_trip(void) {
    for (uint8_t channel = 0; channel < 16; ++channel) {
        for (uint8_t note = 0; note < 128; ++note) {
            const uintptr_t identity = midi_note_identity_pack(channel, note);
            assert(identity != 0);
            assert(midi_note_identity_channel(identity) == channel);
            assert(midi_note_identity_note(identity) == note);
        }
    }
}

static void test_alarm_keeps_exact_note_identity(void) {
    reset_fixture();
    calculated_note = 64;
    midi_plant(4000);
    assert(count_event(LOG_NOTE_ON, 5, 64) == 1);
    assert(scheduled_callback != NULL);

    scheduled_callback(scheduled_id, scheduled_user_data);
    settings.plant_channel = 9;
    last_note_plant = 70;
    assert(count_event(LOG_NOTE_OFF, 5, 64) == 0);
    service_midi_note_lifecycle();
    assert(count_event(LOG_NOTE_OFF, 5, 64) == 1);
    assert(count_event(LOG_NOTE_OFF, 9, 70) == 0);
}

static void test_replacement_and_clock_same_note_do_not_stick(void) {
    reset_fixture();
    calculated_note = 64;
    midi_plant(4000);

    calculated_note = 65;
    midi_plant(4000);
    assert(count_event(LOG_NOTE_OFF, 5, 64) == 1);
    assert(count_event(LOG_NOTE_ON, 5, 65) == 1);

    active_status = BPMClockActive;
    midi_plant(0);
    assert(count_event(LOG_NOTE_OFF, 5, 65) == 1);
    assert(count_event(LOG_NOTE_ON, 5, 65) == 1);
}

static void test_alarm_failure_fails_closed(void) {
    reset_fixture();
    calculated_note = 67;
    fail_next_alarm = true;
    midi_plant(4000);
    assert(count_event(LOG_NOTE_ON, 5, 67) == 1);
    assert(count_event(LOG_NOTE_OFF, 5, 67) == 1);
}

static void test_cancelled_alarm_is_ignored(void) {
    reset_fixture();
    calculated_note = 68;
    midi_plant(4000);
    alarm_callback_t old_callback = scheduled_callback;
    void *old_identity = scheduled_user_data;
    reset_plant_note_off();
    const size_t off_count = count_event(LOG_NOTE_OFF, 5, 68);
    old_callback(scheduled_id, old_identity);
    service_midi_note_lifecycle();
    assert(count_event(LOG_NOTE_OFF, 5, 68) == off_count);
}

static void test_led_events_follow_emitted_notes_and_beats(void) {
    reset_fixture();
    calculated_note = 64;
    midi_plant(4000);
    assert(led_note_count == 1);
    assert(last_led_source == LED_SOURCE_PLANT);
    assert(last_led_note == 64);
    assert(last_led_velocity == 100);

    reset_fixture();
    settings.same_note_light = 0;
    midi_light();
    assert(led_note_count == 1);
    assert(last_led_source == LED_SOURCE_LIGHT);
    assert(last_led_note == 36);
    assert(last_led_velocity == 100);

    reset_fixture();
    isMutedByButton = true;
    midi_plant(4000);
    assert(led_note_count == 0);

    reset_fixture();
    calculated_note = 64;
    play_music(4000);
    assert(led_beat_count == 1);
}

static void test_light_requires_sensor_motion(void) {
    reset_fixture();
    settings.same_note_light = 0;
    midi_light();
    assert(count_event(LOG_NOTE_ON, 6, 36) == 1);

    /* A timer tick, key change or scale change is not a light-sensor event. */
    midi_light();
    settings.middle_plant_note = 72;
    settings.scale = SCALE_MINOR;
    settings.swing_first_note_percent = 60;
    midi_light();
    assert(count_event(LOG_NOTE_ON, 6, 36) == 1);
    assert(count_event(LOG_NOTE_ON, 6, 48) == 0);

    light_adc = 1800;
    midi_light();
    assert(count_event(LOG_NOTE_ON, 6, 47) == 1);
}

static void test_light_pitch_targets_the_plant_channel(void) {
    reset_fixture();
    midi_light_pitch();
    assert(count_event(LOG_PITCH, 5, 63) == 1);
    assert(count_event(LOG_PITCH, 6, 63) == 0);
}

static void test_plant_mute_keeps_visual_feedback(void) {
    reset_fixture();
    calculated_note = 64;
    last_note_plant = 64;
    settings.isMutePlantVelocity = true;
    midi_plant(4000);
    assert(count_event(LOG_NOTE_ON, 5, 64) == 0);
    assert(led_note_count == 1);
    assert(last_led_source == LED_SOURCE_PLANT);
    assert(last_led_note == 64);
}

static void test_debussy_adapter_is_bounded_and_stops_cleanly(void) {
    reset_fixture();
    debussy_control.mode = DEBUSSY_MODE_ENABLED;
    debussy_runtime_reset();
    play_music(4000);

    size_t note_ons = 0;
    for (size_t i = 0; i < midi_log_len; ++i)
        if (midi_log[i].kind == LOG_NOTE_ON) ++note_ons;
    assert(note_ons == 0u);

    light_adc = 400u;
    play_music(4000);
    for (size_t i = 0; i < midi_log_len; ++i)
        if (midi_log[i].kind == LOG_NOTE_ON) ++note_ons;
    assert(note_ons == 3u);

    stop_midi();
    size_t note_offs = 0;
    for (size_t i = 0; i < midi_log_len; ++i)
        if (midi_log[i].kind == LOG_NOTE_OFF) ++note_offs;
    assert(note_offs >= note_ons);
}

static void test_debussy_gesture_phrase_returns_to_midi_silence(void) {
    reset_fixture();
    debussy_control.mode = DEBUSSY_MODE_ENABLED;
    debussy_runtime_reset();
    play_music(4000); /* establish the sensor baseline without sounding */
    light_adc = 400u;
    play_music(4000); /* physical movement opens the eight-beat response */
    for (unsigned int beat = 1u; beat <= 8u; ++beat) play_music(4000);

    size_t note_ons = 0u;
    size_t note_offs = 0u;
    for (size_t i = 0; i < midi_log_len; ++i) {
        if (midi_log[i].kind == LOG_NOTE_ON) ++note_ons;
        if (midi_log[i].kind == LOG_NOTE_OFF) ++note_offs;
    }
    assert(note_ons > 0u);
    assert(note_offs == note_ons);

    const size_t events_at_rest = midi_log_len;
    play_music(4000);
    assert(midi_log_len == events_at_rest);
}

static void test_debussy_mix_controls_reach_midi_adapter(void) {
    reset_fixture();
    debussy_control.mode = DEBUSSY_MODE_ENABLED;
    debussy_control.texture = 0u;
    debussy_control.pedal_level = 25u;
    debussy_control.colour_level = 0u;
    debussy_runtime_reset();
    play_music(4000);
    light_adc = 400u;
    play_music(4000);

    size_t note_ons = 0u;
    for (size_t i = 0; i < midi_log_len; ++i) {
        if (midi_log[i].kind != LOG_NOTE_ON) continue;
        ++note_ons;
        if (midi_log[i].channel == settings.light_channel)
            assert(midi_log[i].velocity == 25u);
    }
    assert(note_ons == 2u); /* pedal plus the causal melody response */
}

static void test_debussy_light_mute_survives_runtime_restart(void) {
    reset_fixture();
    debussy_control.mode = DEBUSSY_MODE_ENABLED;
    settings.isMuteLightVelocity = true;
    debussy_runtime_reset();
    play_music(4000);
    light_adc = 400u;
    play_music(4000);

    assert(count_event(LOG_NOTE_ON, settings.light_channel, 36) == 0u);
    for (size_t i = 0; i < midi_log_len; ++i) {
        if (midi_log[i].kind == LOG_NOTE_ON)
            assert(midi_log[i].channel != settings.light_channel);
    }

    /* Every Listening Lab edit restarts the composition runtime. A user's
       light mute remains authoritative across that restart. */
    debussy_control.texture = 127u;
    debussy_runtime_reset();
    midi_log_len = 0u;
    play_music(4000);
    light_adc = 2800u;
    play_music(4000);
    for (size_t i = 0; i < midi_log_len; ++i) {
        if (midi_log[i].kind == LOG_NOTE_ON)
            assert(midi_log[i].channel != settings.light_channel);
    }
}

static void test_debussy_uses_separate_plant_and_light_channels(void) {
    reset_fixture();
    debussy_control.mode = DEBUSSY_MODE_ENABLED;
    debussy_runtime_reset();
    play_music(4000);
    light_adc = 400u;
    play_music(4000);

    unsigned int plant_notes = 0u;
    unsigned int light_notes = 0u;
    for (size_t i = 0; i < midi_log_len; ++i) {
        if (midi_log[i].kind != LOG_NOTE_ON) continue;
        if (midi_log[i].channel == settings.plant_channel) ++plant_notes;
        if (midi_log[i].channel == settings.light_channel) ++light_notes;
    }
    assert(plant_notes == 1u);
    assert(light_notes == 2u);
}

static void test_debussy_gesture_bypasses_sparse_texture(void) {
    reset_fixture();
    debussy_control.mode = DEBUSSY_MODE_ENABLED;
    debussy_control.texture = 0u;
    debussy_control.pedal_level = 32u;
    debussy_control.colour_level = 48u;
    debussy_runtime_reset();

    play_music(4000);
    light_adc = 400u;
    for (unsigned int beat = 0u; beat < 7u; ++beat) play_music(4000);

    unsigned int light_notes = 0u;
    unsigned int plant_notes = 0u;
    for (size_t i = 0; i < midi_log_len; ++i) {
        if (midi_log[i].kind != LOG_NOTE_ON) continue;
        if (midi_log[i].channel == settings.light_channel) ++light_notes;
        if (midi_log[i].channel == settings.plant_channel) ++plant_notes;
    }
    assert(plant_notes == 1u); /* the user's gesture is never texture-gated */
    assert(light_notes == 2u); /* one pedal + one colour after the gesture */
}

int main(void) {
    test_identity_round_trip();
    test_alarm_keeps_exact_note_identity();
    test_replacement_and_clock_same_note_do_not_stick();
    test_alarm_failure_fails_closed();
    test_cancelled_alarm_is_ignored();
    test_led_events_follow_emitted_notes_and_beats();
    test_light_requires_sensor_motion();
    test_plant_mute_keeps_visual_feedback();
    test_light_pitch_targets_the_plant_channel();
    test_debussy_adapter_is_bounded_and_stops_cleanly();
    test_debussy_gesture_phrase_returns_to_midi_silence();
    test_debussy_mix_controls_reach_midi_adapter();
    test_debussy_light_mute_survives_runtime_restart();
    test_debussy_uses_separate_plant_and_light_channels();
    test_debussy_gesture_bypasses_sparse_texture();
    puts("note_lifecycle: identity, replacement, Clock, LED and failure passed");
    return 0;
}
