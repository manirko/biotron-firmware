#include <stdlib.h>
#include <pico/time.h>
#include <hardware/adc.h>
#include <pico/printf.h>
#include "music.h"
#include "pico/stdio.h"
#include "params.h"
#include "global.h"
#include "PLSDK/music.h"
#include "PLSDK/commands.h"
#include "leds.h"
#include "PLSDK.h"
#include "runtime_safety.h"
#include "midi_note_lifecycle.h"
#include "debussy_mode.h"
#include "debussy_control.h"

uint8_t last_note_plant = MIDDLE_NOTE;
uint8_t last_note_light = 0;
static uint8_t last_note_plant_channel = 1;
static uint8_t last_note_light_channel = 2;
static uintptr_t active_plant_note_identity = 0;
static bool light_note_active = false;
static bool light_sensor_counter_valid = false;
static int last_light_sensor_counter = 0;
static volatile uintptr_t due_plant_note_identity = 0;

DebussyControl debussy_control = {
    .mode = DEBUSSY_MODE_CLASSIC,
    .seed_variant = 0u,
    .texture = 96u,
    .register_band = DEBUSSY_REGISTER_MIDDLE,
    .colour_level = 48u,
    .sensitivity = 64u,
    .touch_threshold = 80u,
    .light_influence = 127u,
    .pedal_level = 34u,
    .melody_level = 100u,
};

static DebussyState debussy_state;
static bool debussy_initialized = false;
static bool debussy_pedal_active = false;
static bool debussy_colour_active = false;
static bool debussy_melody_active = false;
static uint8_t debussy_pedal_note = 0u;
static uint8_t debussy_colour_note = 0u;
static uint8_t debussy_melody_note = 0u;
static uint8_t debussy_channel = 1u;
static uint8_t debussy_melody_beats_left = 0u;
static uint16_t debussy_last_light = MAX_OF_LIGHT / 2u;


alarm_id_t note_off_alarm_id = -1;


int64_t plant_note_off(alarm_id_t id, void *user_data) {
    const uintptr_t identity = (uintptr_t)user_data;
    if (note_off_alarm_id != id) return 0;
    // Pico alarm callbacks run in IRQ context. Publish the exact note identity;
    // TinyUSB transmission is serviced from the main loop.
    due_plant_note_identity = identity;
    note_off_alarm_id = -1;
    return 0;
}

void service_midi_note_lifecycle(void) {
    const uintptr_t identity = due_plant_note_identity;
    if (identity == 0) return;
    due_plant_note_identity = 0;
    note_off(midi_note_identity_channel(identity),
             midi_note_identity_note(identity));
    if (active_plant_note_identity == identity) {
        active_plant_note_identity = 0;
    }
}

void reset_plant_note_off() {
    if (note_off_alarm_id >= 0) {
        cancel_alarm(note_off_alarm_id);
        note_off_alarm_id = -1;
    }
    due_plant_note_identity = 0;
    if (active_plant_note_identity != 0) {
        note_off(midi_note_identity_channel(active_plant_note_identity),
                 midi_note_identity_note(active_plant_note_identity));
        active_plant_note_identity = 0;
    }
}

static uint8_t debussy_register_note(uint8_t note) {
    const int shift = ((int)debussy_control.register_band -
                       (int)DEBUSSY_REGISTER_MIDDLE) * 12;
    int shifted = (int)note + shift;
    if (shifted < 0) shifted = 0;
    if (shifted > 127) shifted = 127;
    return (uint8_t)shifted;
}

static void debussy_note_off_if_active(bool *active, uint8_t note) {
    if (!*active) return;
    note_off(debussy_channel, note);
    *active = false;
}

static void stop_debussy_midi(void) {
    debussy_note_off_if_active(&debussy_melody_active, debussy_melody_note);
    debussy_note_off_if_active(&debussy_colour_active, debussy_colour_note);
    debussy_note_off_if_active(&debussy_pedal_active, debussy_pedal_note);
    debussy_melody_beats_left = 0u;
}

void debussy_runtime_reset(void) {
    stop_debussy_midi();
    debussy_initialized = false;
    debussy_last_light = MAX_OF_LIGHT / 2u;
}

static DebussyInput debussy_sensor_input(void) {
    const uint32_t delta = biotron_abs_diff_u32(last_freq, average_freq);
    const uint32_t noise = average_delta_freq > 0u ? average_delta_freq : 1u;
    const uint64_t scaled = (uint64_t)delta * 127u;
    const uint64_t denominator = (uint64_t)noise * 8u;
    const uint8_t raw_energy = scaled >= denominator * 127u ? 127u :
                               (uint8_t)(scaled / denominator);
    const uint8_t energy = debussy_control_scale_energy(&debussy_control,
                                                        raw_energy);
    int8_t direction = DEBUSSY_DIRECTION_STABLE;
    if (delta > noise) direction = last_freq > average_freq ?
            DEBUSSY_DIRECTION_RISING : DEBUSSY_DIRECTION_FALLING;

    const uint16_t light = MIN(adc_read(), MAX_OF_LIGHT);
    const uint16_t light_delta = light > debussy_last_light ?
            light - debussy_last_light : debussy_last_light - light;
    debussy_last_light = light;
    const uint8_t raw_light = (uint8_t)(((uint32_t)light * 127u) /
                                        MAX_OF_LIGHT);
    const uint8_t raw_light_change = (uint8_t)MIN(127u,
            ((uint32_t)light_delta * 127u) / MAX_OF_LIGHT);
    const uint8_t light_change = (uint8_t)(((uint16_t)raw_light_change *
            debussy_control.light_influence + 63u) / 127u);
    return (DebussyInput){
        .plant_energy = energy,
        .plant_direction = direction,
        .gesture = debussy_control_gesture(&debussy_control, energy),
        .light_level = debussy_control_scale_light(&debussy_control,
                                                   raw_light),
        .light_change = light_change,
    };
}

static void debussy_replace_voice(bool *active, uint8_t *active_note,
                                  uint8_t note, uint8_t velocity) {
    if (*active && *active_note == note) return;
    debussy_note_off_if_active(active, *active_note);
    if (isMutedByButton || velocity == 0u) return;
    note_on(debussy_channel, note, velocity);
    *active_note = note;
    *active = true;
}

static void play_debussy(void) {
    if (!debussy_initialized) {
        const uint32_t seed = debussy_control.seed_variant == 0u ?
                UINT32_C(0x4d595df4) : UINT32_C(0x9e3779b9);
        debussy_init(&debussy_state, seed);
        debussy_channel = biotron_midi_channel(settings.plant_channel);
        debussy_initialized = true;
    }

    if (debussy_melody_active && debussy_melody_beats_left > 0u) {
        --debussy_melody_beats_left;
        if (debussy_melody_beats_left == 0u)
            debussy_note_off_if_active(&debussy_melody_active,
                                       debussy_melody_note);
    }

    const DebussyInput input = debussy_sensor_input();
    const DebussyDecision decision = debussy_step(&debussy_state, &input);
    if (decision.phrase_boundary || !debussy_pedal_active) {
        debussy_replace_voice(&debussy_pedal_active, &debussy_pedal_note,
                debussy_register_note(decision.pedal_note),
                debussy_control.pedal_level);
        debussy_replace_voice(&debussy_colour_active, &debussy_colour_note,
                debussy_register_note(decision.colour_note),
                debussy_control.colour_level);
    }

    const uint32_t texture_sample =
            (debussy_state.beat_index * 29u +
             (uint32_t)debussy_control.seed_variant * 53u) & 0x7fu;
    if (decision.melody_on && texture_sample < debussy_control.texture) {
        debussy_note_off_if_active(&debussy_melody_active,
                                   debussy_melody_note);
        const uint16_t scaled_velocity =
                (uint16_t)decision.melody_velocity *
                debussy_control.melody_level + 50u;
        const uint8_t melody_velocity = (uint8_t)MIN(127u,
                                                     scaled_velocity / 100u);
        if (!isMutedByButton && melody_velocity > 0u) {
            const uint8_t note = debussy_register_note(decision.melody_note);
            note_on(debussy_channel, note, melody_velocity);
#if BIOTRON_LED_MUSIC_PULSE
            led_music_note_on(LED_SOURCE_PLANT, note,
                              melody_velocity);
#endif
            debussy_melody_note = note;
            debussy_melody_active = true;
            debussy_melody_beats_left = decision.melody_duration_beats;
            last_note_plant = note;
        }
    }
}

uint8_t get_CC(int counter) {
//    static int lastCC = 127;
//    uint8_t buff = abs(MIDDLE_NOTE - counter);
//    uint8_t target_CC;
//    if (counter > MIDDLE_NOTE) {
//        target_CC = (1 - ((double )buff / HIGHEST_NOTE_RANGE)) * 127;
//    }
//    else {
//        target_CC = (1 - ((double )buff / LOWEST_NOTE_RANGE)) * 127;
//    }
//
//    if (target_CC > lastCC) {
//        lastCC += (target_CC - lastCC) / 2;
//    }
//    else {
//        lastCC -= (lastCC - target_CC) / 2;
//    }

    return biotron_midi_7bit(63 + counter);
}


int get_plant_counter() {
    static uint64_t last_change_time = 0;
    static uint32_t last_val = 0;
    static int8_t extra_counter = 0;
    const uint8_t a = 10, b = 30;


    const bool minus = average_freq < last_freq;
    const uint32_t diff_magnitude = biotron_abs_diff_u32(
            average_freq, last_freq);
    int diff = diff_magnitude > (uint32_t)INT_MAX ? INT_MAX :
            (int)diff_magnitude;

    if (((uint64_t)biotron_abs_diff_u32(last_val, last_freq) >=
         (uint64_t)average_delta_freq * 3u) || settings.performance_mode) {
        last_change_time = time_us_64();
        last_val = last_freq;
        extra_counter = 0;
    }

    if (time_us_64() - last_change_time > 90000000) {
        int chance = rand() % 101;
        if (chance <= a * ((extra_counter + 10.0) / 10.0)) {
            extra_counter = MAX(-10, extra_counter - 2);
        } else if (chance <= a * ((extra_counter + 10.0) / 10.0) + b * ((extra_counter + 10.0) / 10.0)) {
            extra_counter = MAX(-10, extra_counter - 1);
        } else if (chance <= a * ((extra_counter + 10.0) / 10.0) + b * ((extra_counter + 10.0) / 10.0) + 20) {

        } else if (chance <= a * ((extra_counter + 10.0) / 10.0) + b * 2 + 20) {
            extra_counter = MIN(10, extra_counter + 1);
        } else {
            extra_counter = MIN(10, extra_counter + 2);
        }
    }

    if (average_delta_freq <= (uint32_t)INT_MAX &&
        diff >= (int)average_delta_freq) {
        diff -= (int)average_delta_freq;
    } else {
        // Released integer promotions made this path return one scale step.
        // Preserve that observable behaviour while avoiding underflow.
        diff = 0;
    }

    const int i = biotron_fibonacci_counter(
            diff, average_delta_freq, settings.fibPower, settings.firstValue);

    if (minus) {
        return biotron_clamp_int(-i + extra_counter, -127, 127);
    }
    return biotron_clamp_int(i + extra_counter, -127, 127);
}


void midi_plant(int64_t to_the_next_beat_us) {
    int plant_counter = get_plant_counter();

    uint8_t currentNote = MAX(settings.middle_plant_note - LOWEST_NOTE_RANGE,
                              MIN(settings.middle_plant_note + HIGHEST_NOTE_RANGE,
                                  calculate_note_by_scale(settings.middle_plant_note,
                                                          plant_counter, settings.scale)));

    if (!isMutedByButton) {
        if (active_status == BPMClockActive) {
            if (!settings.isMutePlantVelocity) reset_plant_note_off();
        }
        if (!settings.isMutePlantVelocity &&
            abs((int)currentNote - (int)last_note_plant) <
                    settings.same_note_plant) {
            return;
        }

        if (!settings.isMutePlantVelocity && active_status == Active &&
            active_plant_note_identity != 0) {
            reset_plant_note_off();
        }

        uint8_t velocity = settings.isRandomPlantVelocity ?
                biotron_random_velocity((uint32_t)rand(),
                        settings.minPlantVelocity, settings.maxPlantVelocity) :
                biotron_midi_7bit(settings.maxPlantVelocity);

        if (!settings.isMutePlantVelocity) {
            note_on(settings.plant_channel, currentNote, velocity);
        }
#if BIOTRON_LED_MUSIC_PULSE
        led_music_note_on(LED_SOURCE_PLANT, currentNote, velocity);
#endif
        if (!settings.isMutePlantVelocity) {
            last_note_plant_channel = biotron_midi_channel(settings.plant_channel);
            active_plant_note_identity = midi_note_identity_pack(
                    last_note_plant_channel, currentNote);
        }

        if (!settings.isMutePlantVelocity && active_status == Active) {
            note_off_alarm_id = add_alarm_in_us(
                    MAX(1, to_the_next_beat_us /
                            MAX(1, settings.fraction_note_off)),
                    plant_note_off, (void *)active_plant_note_identity, false);
            if (note_off_alarm_id < 0) {
                note_off(last_note_plant_channel, currentNote);
                active_plant_note_identity = 0;
            }
        }
    }

    uint8_t note_cc[3] = {0xB0 | settings.plant_channel, 90, get_CC(plant_counter)};
    print_pure(0, note_cc, 3);

    last_note_plant = currentNote;
}


void midi_light() {
    uint16_t adc = MIN(adc_read(), MAX_OF_LIGHT);
    const uint8_t light_note_range = biotron_effective_light_range(
            settings.light_note_range);
    uint16_t step = MAX_OF_LIGHT / (light_note_range * 2);

    int counter = abs(MAX_OF_LIGHT / 2 - (int)adc) / step;
    if (adc > MAX_OF_LIGHT / 2) {
        counter = -counter;
    }


    uint8_t current_note = MAX(settings.middle_plant_note - LIGHT_DIFFERENCE - light_note_range,
                               MIN(settings.middle_plant_note - LIGHT_DIFFERENCE + light_note_range,
                                   calculate_note_by_scale(settings.middle_plant_note - LIGHT_DIFFERENCE, counter,
                                                           settings.scale)));

    if (light_note_active) {
        note_off(last_note_light_channel, last_note_light);
        light_note_active = false;
    }

    if (isMutedByButton || settings.isMuteLightVelocity) return;

    const int sensor_change = light_sensor_counter_valid ?
            abs(counter - last_light_sensor_counter) : INT_MAX;
    if (sensor_change <= MAX(0, settings.same_note_light)) {
        return;
    }
    last_light_sensor_counter = counter;
    light_sensor_counter_valid = true;

    uint8_t vel = settings.isRandomLightVelocity ?
            biotron_random_velocity((uint32_t)rand(),
                    settings.minLightVelocity, settings.maxLightVelocity) :
            biotron_midi_7bit(settings.maxLightVelocity);
    note_on(settings.light_channel, current_note, vel);
#if BIOTRON_LED_MUSIC_PULSE
    led_music_note_on(LED_SOURCE_LIGHT, current_note, vel);
#endif
    last_note_light_channel = biotron_midi_channel(settings.light_channel);
    light_note_active = true;

    last_note_light = current_note;
}

void midi_light_pitch() {
    const uint16_t bend = biotron_pitch_from_adc(
            MIN(adc_read(), MAX_OF_LIGHT), MAX_OF_LIGHT);
    change_pitch(settings.plant_channel, biotron_pitch_lsb(bend),
                 biotron_pitch_msb(bend));
}

void stop_midi() {
    stop_debussy_midi();
    stop_plant_midi();
    stop_light_midi();
}

void stop_plant_midi(void) {
    reset_plant_note_off();
    stop_all_notes(last_note_plant_channel);
    change_pitch(last_note_plant_channel, 0, 64);
}

void stop_light_midi(void) {
    if (light_note_active) {
        note_off(last_note_light_channel, last_note_light);
        light_note_active = false;
    }
    light_sensor_counter_valid = false;
    stop_all_notes(last_note_light_channel);
}

void play_music(int64_t to_the_next_beat) {
    static uint64_t time_log = 0;
    static uint8_t counter = 1;
    const uint8_t light_every = biotron_effective_light_bpm(settings.lightBPM);

#if BIOTRON_LED_MUSIC_PULSE
    led_music_beat();
#endif

    if (debussy_control.mode == DEBUSSY_MODE_ENABLED) {
        play_debussy();
        return;
    }

    midi_plant(to_the_next_beat);

    if (settings.light_pitch_mode) midi_light_pitch();
    else if (counter++ >= light_every) {
        midi_light();
        light_note_observer();
        counter = 1;
    }

    if (time_log == 0) {
        time_log = time_us_64();
    }
    if (time_us_64() - time_log > 100000) {
        plsdk_printf("{\"AverageFreq\": %d, \"Freq\": %d, \"PlantNote\": %d, \"LightNote\": %d }\n",
                     average_freq, last_freq, last_note_plant, last_note_light);
        time_log = time_us_64();
    }
}

void play_music_bpm_clock() {
    if (status != BPMClockActive) return;
    play_music(0);
}
