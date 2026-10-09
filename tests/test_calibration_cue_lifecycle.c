/* Production calibration, settings actions, note lifecycle and MIDI encoding.
 * Sensor/time/USB are host mocks; no hardware throughput or timing is inferred. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/time.h"
#include "params.h"
#include "global.h"
#include "raw_plant.h"
#include "music.h"
#include "leds.h"

extern void set_channel_sys_ex(const uint8_t data[], uint8_t len);
extern void default_settings(void);

static bool single_active[16][128];
static unsigned repeated_active[16][128];
static unsigned note_ons[16][128];
static unsigned note_offs[16][128];
static uint32_t frequency = 1000;
static alarm_id_t next_alarm_id = 1;
bool LOGGER_FLAG = false;
uint32_t button_states[3] = {0};

uint64_t time_us_64(void) { return 1000000; }
uint32_t time_us_32(void) { return 1000000; }
uint32_t save_and_disable_interrupts(void) { return 0; }
void restore_interrupts(uint32_t state) { (void)state; }
bool plant_is_ready(void) { return true; }
uint32_t get_real_freq(void) { return frequency; }
uint16_t adc_read(void) { return 1600; }
alarm_id_t add_alarm_in_us(int64_t delay, alarm_callback_t callback,
                          void *data, bool fire_if_past) {
    (void)delay; (void)callback; (void)data; (void)fire_if_past;
    return next_alarm_id++;
}
bool cancel_alarm(alarm_id_t id) { (void)id; return true; }
void light_note_observer(void) {}
void led_music_beat(void) {}
void led_music_note_on(led_source_t source, uint8_t note, uint8_t velocity) {
    (void)source; (void)note; (void)velocity;
}
void plsdk_printf(const char *format, ...) { (void)format; }

bool print_pure(uint8_t cable, const uint8_t data[], uint8_t len) {
    if (len != 3) return true;
    assert(cable == 0 && data[1] < 128 && data[2] < 128);
    const uint8_t channel = data[0] & 15u;
    const uint8_t kind = data[0] & 0xf0u;
    const uint8_t note = data[1];
    if (kind == 0x90 && data[2] != 0) {
        single_active[channel][note] = true;
        ++repeated_active[channel][note];
        ++note_ons[channel][note];
        if (note == 64) assert(data[2] == 24);
    } else if (kind == 0x80 || (kind == 0x90 && data[2] == 0)) {
        single_active[channel][note] = false;
        if (repeated_active[channel][note] != 0) --repeated_active[channel][note];
        ++note_offs[channel][note];
    } else if (kind == 0xb0 && data[1] == 123) {
        memset(single_active[channel], 0, sizeof single_active[channel]);
        memset(repeated_active[channel], 0, sizeof repeated_active[channel]);
    }
    return true;
}

static void assert_receivers_empty(void) {
    for (unsigned channel = 0; channel < 16; ++channel) {
        for (unsigned note = 0; note < 128; ++note) {
            assert(!single_active[channel][note]);
            assert(repeated_active[channel][note] == 0);
        }
    }
}

static void sample(unsigned count, uint32_t value) {
    frequency = value;
    for (unsigned i = 0; i < count; ++i) status_loop();
}

enum setting_action { KEEP_CHANNEL, CHANGE_CHANNEL, RESET_SETTINGS };
enum cue_end { TIMED_RELEASE, RECALIBRATE, SENSOR_LOST };

static void test_cue_identity(bool prior_note, uint8_t channel,
                              enum setting_action action, enum cue_end end) {
    start_plant_calibration(41);
    assert_receivers_empty();
    settings = (Settings_t){0};
    settings.plant_channel = channel;
    settings.light_channel = 6;
    settings.middle_plant_note = 60;
    settings.fibPower = 0.5;
    settings.firstValue = 0.1;
    settings.fraction_note_off = 4;
    settings.light_note_range = 12;
    settings.BPM = 1000;
    settings.maxPlantVelocity = 100;
    active_status = Active;
    if (prior_note) {
        status = Active;
        settings.plant_channel = 4;
        last_freq = average_freq = 1000;
        average_delta_freq = 1;
        midi_plant(4000);
        const uint8_t request[] = {0, channel};
        set_channel_sys_ex(request, sizeof request);
        assert_receivers_empty();
    }
    start_plant_calibration(42);
    memset(note_ons, 0, sizeof note_ons);
    memset(note_offs, 0, sizeof note_offs);
    sample(STABILIZATION_COUNTER, 1000);
    assert(status == Stabilization);
    sample(3, 1000);
    assert(note_ons[channel][64] == 1 && repeated_active[channel][64] == 1);

    if (action == CHANGE_CHANNEL) {
        const uint8_t request[] = {0, channel == 15 ? 0 : 15};
        set_channel_sys_ex(request, sizeof request);
    } else if (action == RESET_SETTINGS) {
        default_settings();
    }
    const uint8_t current_channel = settings.plant_channel;
    if (end == RECALIBRATE) {
        start_plant_calibration(43);
        assert(status == Sleep);
    } else if (end == SENSOR_LOST) {
        sample(1, 0);
        assert(status == Sleep);
    } else {
        sample(3, 1000);
    }
    assert(note_offs[channel][64] == 1);
    if (current_channel != channel) assert(note_offs[current_channel][64] == 0);
    assert_receivers_empty();

    if (end == TIMED_RELEASE) {
        sample(2, 1000);
        assert(note_ons[current_channel][65] == 1);
        sample(3, 1000);
        assert(note_offs[current_channel][65] == 1);
        assert_receivers_empty();
    }
    start_plant_calibration(44);
    start_plant_calibration(45);
    assert(note_offs[channel][64] == 1);
    assert_receivers_empty();
}

int main(void) {
    const struct {
        bool prior_note;
        uint8_t channel;
        enum setting_action action;
        enum cue_end end;
    } cases[] = {
        {false, 5, KEEP_CHANNEL, TIMED_RELEASE},
        {false, 5, CHANGE_CHANNEL, TIMED_RELEASE},
        {false, 5, CHANGE_CHANNEL, RECALIBRATE},
        {false, 5, CHANGE_CHANNEL, SENSOR_LOST},
        {true, 5, CHANGE_CHANNEL, TIMED_RELEASE},
        {true, 5, CHANGE_CHANNEL, RECALIBRATE},
        {false, 5, RESET_SETTINGS, TIMED_RELEASE},
        {false, 5, RESET_SETTINGS, RECALIBRATE},
        {false, 0, CHANGE_CHANNEL, TIMED_RELEASE},
        {false, 15, CHANGE_CHANNEL, TIMED_RELEASE},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        test_cue_identity(cases[i].prior_note, cases[i].channel,
                          cases[i].action, cases[i].end);
    }
    puts("calibration_cue_lifecycle: exact releases, settings reset and both receivers passed");
    return 0;
}
