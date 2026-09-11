#include "debussy_control.h"
#include "debussy_mode.h"

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
        .sensitivity = 64u,
        .touch_threshold = 80u,
        .light_influence = 127u,
        .pedal_level = 34u,
        .melody_level = 100u,
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
        case DEBUSSY_COMMAND_SENSITIVITY:
            control->sensitivity = clamp_u7(value);
            return true;
        case DEBUSSY_COMMAND_TOUCH_THRESHOLD:
            control->touch_threshold = clamp_u7(value);
            if (control->touch_threshold < 16u)
                control->touch_threshold = 16u;
            return true;
        case DEBUSSY_COMMAND_LIGHT_INFLUENCE:
            control->light_influence = clamp_u7(value);
            return true;
        case DEBUSSY_COMMAND_PEDAL:
            control->pedal_level = clamp_u7(value);
            return true;
        case DEBUSSY_COMMAND_MELODY:
            control->melody_level = clamp_u7(value);
            return true;
        default:
            return false;
    }
}

uint8_t debussy_control_scale_energy(const DebussyControl *control,
                                     uint8_t raw_energy) {
    const uint16_t scaled = (uint16_t)raw_energy * control->sensitivity + 32u;
    const uint16_t result = scaled / 64u;
    return result > 127u ? 127u : (uint8_t)result;
}

uint8_t debussy_control_scale_light(const DebussyControl *control,
                                    uint8_t raw_light) {
    const int16_t offset = (int16_t)raw_light - 64;
    int16_t result = 64 + (int16_t)((offset * control->light_influence) / 127);
    if (result < 0) result = 0;
    if (result > 127) result = 127;
    return (uint8_t)result;
}

uint8_t debussy_control_gesture(const DebussyControl *control,
                                uint8_t energy) {
    if (energy >= control->touch_threshold) return DEBUSSY_GESTURE_TOUCH;
    uint8_t drift_threshold = control->touch_threshold / 5u;
    if (drift_threshold < 8u) drift_threshold = 8u;
    return energy >= drift_threshold ? DEBUSSY_GESTURE_DRIFT :
                                       DEBUSSY_GESTURE_STABLE;
}

size_t debussy_control_encode(const DebussyControl *control,
                              uint8_t request_id, uint8_t *output,
                              size_t capacity) {
    if (capacity < DEBUSSY_CONTROL_PAYLOAD_BYTES) return 0u;
    const uint8_t payload[DEBUSSY_CONTROL_PAYLOAD_BYTES] = {
        DEBUSSY_QUERY_ID, DEBUSSY_PROTOCOL_VERSION, DEBUSSY_SCHEMA_VERSION,
        (uint8_t)(request_id & 0x7fu), control->mode, control->seed_variant,
        control->texture, control->register_band, control->colour_level,
        control->sensitivity, control->touch_threshold,
        control->light_influence, control->pedal_level, control->melody_level,
    };
    for (size_t index = 0; index < sizeof payload; ++index)
        output[index] = payload[index];
    return sizeof payload;
}
