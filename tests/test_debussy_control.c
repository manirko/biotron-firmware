#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "debussy_control.h"

static void test_safe_session_defaults(void) {
    DebussyControl control;
    debussy_control_default(&control);
    assert(sizeof control == 5u);
    assert(control.mode == DEBUSSY_MODE_CLASSIC);
    assert(control.seed_variant == 0u);
    assert(control.texture == 96u);
    assert(control.register_band == DEBUSSY_REGISTER_MIDDLE);
    assert(control.colour_level == 48u);
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
    assert(!debussy_control_set(&control, 27u, 1u));
    assert(!debussy_control_set(&control, 33u, 1u));
}

static void test_query_payload_is_exact(void) {
    DebussyControl control = {1u, 1u, 73u, 0u, 41u};
    uint8_t payload[DEBUSSY_CONTROL_PAYLOAD_BYTES] = {0};
    assert(debussy_control_encode(&control, 91u, payload, sizeof payload) ==
           DEBUSSY_CONTROL_PAYLOAD_BYTES);
    const uint8_t expected[] = {
        DEBUSSY_QUERY_ID, DEBUSSY_PROTOCOL_VERSION,
        DEBUSSY_SCHEMA_VERSION, 91u, 1u, 1u, 73u, 0u, 41u,
    };
    for (unsigned int i = 0; i < sizeof expected; ++i)
        assert(payload[i] == expected[i]);
    assert(debussy_control_encode(&control, 91u, payload,
                                  sizeof payload - 1u) == 0u);
}

int main(void) {
    test_safe_session_defaults();
    test_values_are_bounded();
    test_query_payload_is_exact();
    puts("debussy_control: safe defaults, bounds and protocol passed");
    return 0;
}
