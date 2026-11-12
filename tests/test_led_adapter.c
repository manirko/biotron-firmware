#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/pwm.h"
#include "buttons.h"
#include "global.h"
#include "leds.h"
#include "music.h"
#include "params.h"

Settings_t settings;
enum Status status = Active;
enum Status active_status = Active;
bool isMutedByButton = false;
bool TestMode = false;
bool isTestModeGreen = false;
bool button_finger_pressed = false;
bool button_bottom_pressed = false;
bool button_top_pressed = false;
bool LOGGER_FLAG = false;
uint8_t last_note_plant = MIDDLE_NOTE;

static uint64_t now_us = 0;
static uint16_t pwm_levels[32];

uint64_t time_us_64(void) { return now_us; }
uint32_t time_us_32(void) { return now_us; }
void sleep_ms(uint32_t milliseconds) { now_us += milliseconds * 1000u; }
void remind_midi(void) {}
void note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    (void)channel;
    (void)note;
    (void)velocity;
}
void note_off(uint8_t channel, uint8_t note) {
    (void)channel;
    (void)note;
}

void gpio_init_mask(uint32_t mask) { (void)mask; }
void gpio_set_function(uint gpio, uint function) {
    (void)gpio;
    (void)function;
}
uint pwm_gpio_to_slice_num(uint gpio) { return gpio / 2u; }
pwm_config pwm_get_default_config(void) {
    const pwm_config config = {0};
    return config;
}
void pwm_init(uint slice, const pwm_config *config, bool start) {
    (void)slice;
    (void)config;
    (void)start;
}
void pwm_set_gpio_level(uint gpio, uint16_t level) {
    assert(gpio < sizeof(pwm_levels) / sizeof(pwm_levels[0]));
    pwm_levels[gpio] = level;
}

#if BIOTRON_LED_MUSIC_PULSE
static uint16_t brightness(uint gpio) {
    return (uint16_t)(MAX_LIGHT - pwm_levels[gpio]);
}
#endif

static void reset_fixture(void) {
    memset(&settings, 0, sizeof(settings));
    settings.middle_plant_note = MIDDLE_NOTE;
    settings.light_note_range = 12;
    status = Active;
    isMutedByButton = false;
    TestMode = false;
    isTestModeGreen = false;
    button_finger_pressed = false;
    button_bottom_pressed = false;
    button_top_pressed = false;
    now_us = 0;
    memset(pwm_levels, 0, sizeof(pwm_levels));
    init_leds();
    led_loop();
}

#if BIOTRON_LED_MUSIC_PULSE
static void test_spatial_pin_map_and_polarity(void) {
    reset_fixture();
    led_music_note_on(LED_SOURCE_LIGHT, 24, 127);
    led_loop();
    assert(brightness(3) > 0);
    assert(brightness(3) < LED_ENGINE_MAX_LEVEL);
    assert(brightness(4) == 0);
    assert(brightness(9) == 0);

    reset_fixture();
    led_music_note_on(LED_SOURCE_PLANT, 97, 127);
    led_loop();
    assert(brightness(14) > 0);
    assert(brightness(14) < LED_ENGINE_MAX_LEVEL);
    assert(brightness(10) == 0);
    assert(brightness(11) == 0);
}

static void test_beat_and_mute_domains(void) {
    reset_fixture();
    led_music_note_on(LED_SOURCE_PLANT, 60, 127);
    led_music_beat();
    led_loop();
    const uint16_t beat = brightness(0);
    const uint16_t note = brightness(11);
    assert(beat > 0 && beat < LED_ENGINE_MAX_LEVEL);
    assert(note > 0 && note < LED_ENGINE_MAX_LEVEL);
    assert(brightness(1) == beat);
    assert(brightness(2) == beat);

    isMutedByButton = true;
    led_loop();
    assert(brightness(11) == 0);
    assert(brightness(0) == beat);
}

static void test_note_reaches_all_physical_zones(void) {
    reset_fixture();
    led_music_note_on(LED_SOURCE_PLANT, 48, 127);
    now_us = 32000;
    led_loop();
    const uint8_t green_pins[] = {3, 4, 9, 10, 11, 14};
    for (size_t pin = 0; pin < sizeof(green_pins); ++pin) {
        assert(brightness(green_pins[pin]) > 0);
    }
    assert(brightness(0) > 0);
    assert(brightness(1) > 0);
    assert(brightness(2) > 0);
}

static void music_frame_after(uint64_t start, uint16_t frame[32]) {
    reset_fixture();
    now_us = start;
    init_leds();
    led_music_note_on(LED_SOURCE_PLANT, 48, 127);
    led_music_beat();
    led_loop();
    now_us += 32000;
    led_loop();
    memcpy(frame, pwm_levels, sizeof pwm_levels);
}

static void test_music_wrap_matches_normal_decay(void) {
    uint16_t normal[32], wrapped[32];
    music_frame_after(1000000, normal);
    music_frame_after(UINT64_C(4294960000), wrapped);
    assert(memcmp(normal, wrapped, sizeof normal) == 0);
    /* The uint32 millisecond engine also crosses its own 49-day wrap. */
    music_frame_after((UINT64_C(1) << 32) * 1000 - 8000, wrapped);
    assert(memcmp(normal, wrapped, sizeof normal) == 0);
}
#endif

extern void active_led_loop_light_bpm_sync(void);

static uint16_t legacy_frame_after(uint64_t start, uint64_t elapsed) {
    reset_fixture();
    settings.BPM = 100000;
    settings.lightBPM = 1;
    now_us = start;
    light_note_observer();
    memset(pwm_levels, 0, sizeof pwm_levels);
    now_us += elapsed;
    active_led_loop_light_bpm_sync();
    return pwm_levels[FIRST_GROUP_GREEN_LED_1];
}

static void test_legacy_wrap_and_expired_pulse(void) {
    const uint16_t normal = legacy_frame_after(1000000, 32000);
    assert(normal > 0);
    assert(legacy_frame_after(UINT32_MAX - UINT64_C(16000), 32000) == normal);
    assert(legacy_frame_after(1000000, (UINT64_C(1) << 32) + 32000) == 0);
}

int main(void) {
#if BIOTRON_LED_MUSIC_PULSE
    test_spatial_pin_map_and_polarity();
    test_beat_and_mute_domains();
    test_note_reaches_all_physical_zones();
    test_music_wrap_matches_normal_decay();
#endif
    test_legacy_wrap_and_expired_pulse();
    puts("led_adapter: pins, envelopes and microsecond/millisecond wrap passed");
    return 0;
}
