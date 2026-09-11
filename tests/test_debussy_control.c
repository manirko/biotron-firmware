#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "debussy_control.h"
#include "debussy_mode.h"

static void test_safe_session_defaults(void) {
    DebussyControl control;
    debussy_control_default(&control);
    assert(sizeof control == 10u);
    assert(control.mode == DEBUSSY_MODE_CLASSIC);
    assert(control.seed_variant == 0u);
    assert(control.texture == 96u);
    assert(control.register_band == DEBUSSY_REGISTER_MIDDLE);
    assert(control.colour_level == 48u);
    assert(control.sensitivity == 64u);
    assert(control.touch_threshold == 80u);
    assert(control.light_influence == 127u);
    assert(control.pedal_level == 34u);
    assert(control.melody_level == 100u);
}

static void test_values_are_bounded(void) {
    DebussyControl control;
    debussy_control_default(&control);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_MODE, 127u));
    assert(control.mode == DEBUSSY_MODE_ENABLED);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_SEED, 127u));
    assert(control.seed_variant == 1u);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_TEXTURE, 255u));
    assert(control.texture == 127u);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_REGISTER, 127u));
    assert(control.register_band == DEBUSSY_REGISTER_HIGH);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_COLOUR, 255u));
    assert(control.colour_level == 127u);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_SENSITIVITY, 91u));
    assert(control.sensitivity == 91u);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_TOUCH_THRESHOLD, 0u));
    assert(control.touch_threshold == 16u);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_LIGHT_INFLUENCE, 255u));
    assert(control.light_influence == 127u);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_PEDAL, 255u));
    assert(control.pedal_level == 127u);
    assert(debussy_control_set(&control, DEBUSSY_COMMAND_MELODY, 255u));
    assert(control.melody_level == 127u);
    assert(!debussy_control_set(&control, 27u, 1u));
    assert(!debussy_control_set(&control, 38u, 1u));
}

static void test_sensor_mapping_is_bounded_and_audible(void) {
    DebussyControl control;
    debussy_control_default(&control);
    assert(debussy_control_scale_energy(&control, 32u) == 32u);
    control.sensitivity = 127u;
    assert(debussy_control_scale_energy(&control, 64u) == 127u);
    control.sensitivity = 0u;
    assert(debussy_control_scale_energy(&control, 127u) == 0u);

    control.light_influence = 0u;
    assert(debussy_control_scale_light(&control, 0u) == 64u);
    assert(debussy_control_scale_light(&control, 127u) == 64u);
    control.light_influence = 127u;
    assert(debussy_control_scale_light(&control, 0u) == 0u);
    assert(debussy_control_scale_light(&control, 127u) == 127u);

    control.touch_threshold = 80u;
    assert(debussy_control_gesture(&control, 15u) == DEBUSSY_GESTURE_STABLE);
    assert(debussy_control_gesture(&control, 16u) == DEBUSSY_GESTURE_DRIFT);
    assert(debussy_control_gesture(&control, 80u) == DEBUSSY_GESTURE_TOUCH);
}

static void test_query_payload_is_exact(void) {
    DebussyControl control = {1u, 1u, 73u, 0u, 41u, 92u, 77u, 63u, 29u, 111u};
    uint8_t payload[DEBUSSY_CONTROL_PAYLOAD_BYTES] = {0};
    assert(debussy_control_encode(&control, 91u, payload, sizeof payload) ==
           DEBUSSY_CONTROL_PAYLOAD_BYTES);
    const uint8_t expected[] = {
        DEBUSSY_QUERY_ID, DEBUSSY_PROTOCOL_VERSION,
        DEBUSSY_SCHEMA_VERSION, 91u, 1u, 1u, 73u, 0u, 41u,
        92u, 77u, 63u, 29u, 111u,
    };
    for (unsigned int i = 0; i < sizeof expected; ++i)
        assert(payload[i] == expected[i]);
    assert(debussy_control_encode(&control, 91u, payload,
                                  sizeof payload - 1u) == 0u);
}

int main(void) {
    test_safe_session_defaults();
    test_values_are_bounded();
    test_sensor_mapping_is_bounded_and_audible();
    test_query_payload_is_exact();
    puts("debussy_control: safe defaults, bounds and protocol passed");
    return 0;
}
