#include "debussy_control.h"

static uint8_t clamp_u7(uint8_t value) {
    return value > 127u ? 127u : value;
}

void debussy_control_default(DebussyControl *control) {
    *control = (DebussyControl){
        .mode = DEBUSSY_MODE_CLASSIC,
        .seed_variant = 0u,
        .texture = 96u,
        .register_band = DEBUSSY_REGISTER_MIDDLE,
        .colour_level = 48u,
    };
}

bool debussy_control_set(DebussyControl *control, uint8_t command,
                         uint8_t value) {
    switch (command) {
        case DEBUSSY_COMMAND_MODE:
            control->mode = value == 0u ? DEBUSSY_MODE_CLASSIC :
                                          DEBUSSY_MODE_ENABLED;
            return true;
        case DEBUSSY_COMMAND_SEED:
            control->seed_variant = value == 0u ? 0u : 1u;
            return true;
        case DEBUSSY_COMMAND_TEXTURE:
            control->texture = clamp_u7(value);
            return true;
        case DEBUSSY_COMMAND_REGISTER:
            control->register_band = value > DEBUSSY_REGISTER_HIGH ?
                                     DEBUSSY_REGISTER_HIGH : value;
            return true;
        case DEBUSSY_COMMAND_COLOUR:
            control->colour_level = clamp_u7(value);
            return true;
        default:
            return false;
    }
}

size_t debussy_control_encode(const DebussyControl *control,
                              uint8_t request_id, uint8_t *output,
                              size_t capacity) {
    if (capacity < DEBUSSY_CONTROL_PAYLOAD_BYTES) return 0u;
    const uint8_t payload[DEBUSSY_CONTROL_PAYLOAD_BYTES] = {
        DEBUSSY_QUERY_ID, DEBUSSY_PROTOCOL_VERSION, DEBUSSY_SCHEMA_VERSION,
        (uint8_t)(request_id & 0x7fu), control->mode, control->seed_variant,
        control->texture, control->register_band, control->colour_level,
    };
    for (size_t index = 0; index < sizeof payload; ++index)
        output[index] = payload[index];
    return sizeof payload;
}
