#ifndef BIOTRON_DEBUSSY_CONTROL_H
#define BIOTRON_DEBUSSY_CONTROL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DEBUSSY_QUERY_ID = 122,
    DEBUSSY_PROTOCOL_VERSION = 1,
    DEBUSSY_SCHEMA_VERSION = 1,
    DEBUSSY_CONTROL_PAYLOAD_BYTES = 9,
    DEBUSSY_COMMAND_MODE = 28,
    DEBUSSY_COMMAND_SEED = 29,
    DEBUSSY_COMMAND_TEXTURE = 30,
    DEBUSSY_COMMAND_REGISTER = 31,
    DEBUSSY_COMMAND_COLOUR = 32,
};

typedef enum {
    DEBUSSY_MODE_CLASSIC = 0,
    DEBUSSY_MODE_ENABLED = 1,
} DebussyMode;

typedef enum {
    DEBUSSY_REGISTER_LOW = 0,
    DEBUSSY_REGISTER_MIDDLE = 1,
    DEBUSSY_REGISTER_HIGH = 2,
} DebussyRegister;

typedef struct {
    uint8_t mode;
    uint8_t seed_variant;
    uint8_t texture;
    uint8_t register_band;
    uint8_t colour_level;
} DebussyControl;

extern DebussyControl debussy_control;

void debussy_control_default(DebussyControl *control);
bool debussy_control_set(DebussyControl *control, uint8_t command,
                         uint8_t value);
size_t debussy_control_encode(const DebussyControl *control,
                              uint8_t request_id, uint8_t *output,
                              size_t capacity);

#endif
