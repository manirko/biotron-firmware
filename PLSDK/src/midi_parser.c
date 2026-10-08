#include <string.h>

#include "PLSDK/midi_parser.h"

static uint8_t cin_payload_length(uint8_t cin) {
    static const uint8_t lengths[16] = {
        0, 0, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1
    };
    return lengths[cin & 0x0f];
}

void midi_parser_init(midi_parser_t *parser) {
    memset(parser, 0, sizeof(*parser));
}

static midi_event_kind_t malformed(midi_parser_t *parser, midi_event_t *event,
                                   midi_parser_error_t error) {
    parser->in_sysex = false;
    parser->sysex_len = 0;
    event->kind = MIDI_EVENT_MALFORMED;
    event->error = error;
    event->len = 0;
    return event->kind;
}

midi_event_kind_t midi_parser_feed_usb_packet(midi_parser_t *parser,
                                               const uint8_t packet[4],
                                               midi_event_t *event) {
    const uint8_t cin = packet[0] & 0x0f;
    const uint8_t payload_len = cin_payload_length(cin);
    event->kind = MIDI_EVENT_NONE;
    event->error = MIDI_PARSER_ERROR_NONE;
    event->sysex_aborted = false;
    event->len = 0;

    if (payload_len == 0) {
        return malformed(parser, event, MIDI_PARSER_ERROR_MALFORMED);
    }

    /* Check only active payload bytes; USB padding is not MIDI data. */
    if (cin >= 0x8 && cin <= 0xe) {
        if ((packet[1] >> 4) != cin) {
            return malformed(parser, event, MIDI_PARSER_ERROR_MALFORMED);
        }
    } else if ((cin == 0x2 && packet[1] != 0xf1 && packet[1] != 0xf3) ||
               (cin == 0x3 && packet[1] != 0xf2)) {
        return malformed(parser, event, MIDI_PARSER_ERROR_MALFORMED);
    }
    if (cin == 0x2 || cin == 0x3 || (cin >= 0x8 && cin <= 0xe)) {
        for (uint8_t i = 2; i <= payload_len; ++i) {
            if (packet[i] >= 0x80) {
                return malformed(parser, event, MIDI_PARSER_ERROR_MALFORMED);
            }
        }
    }

    /* CIN 5 also carries one-byte System Common (Tune Request), not SysEx. */
    if (cin >= 0x4 && cin <= 0x7 && !(cin == 0x5 && packet[1] == 0xf6)) {
        if (!parser->in_sysex) {
            if (packet[1] != 0xf0) {
                return malformed(parser, event, MIDI_PARSER_ERROR_MALFORMED);
            }
            parser->in_sysex = true;
            parser->sysex_len = 0;
        }
        for (uint8_t i = 1; i <= payload_len; ++i) {
            const bool start = parser->sysex_len == 0 && i == 1;
            const bool end = cin != 0x4 && i == payload_len;
            if ((start && packet[i] != 0xf0) ||
                (end && packet[i] != 0xf7) ||
                (!start && !end && packet[i] >= 0x80)) {
                return malformed(parser, event, MIDI_PARSER_ERROR_MALFORMED);
            }
        }
        if (parser->sysex_len + payload_len > MIDI_PARSER_SYSEX_CAPACITY) {
            return malformed(parser, event,
                             MIDI_PARSER_ERROR_SYSEX_OVERFLOW);
        }
        memcpy(&parser->sysex[parser->sysex_len], &packet[1], payload_len);
        parser->sysex_len += payload_len;

        if (cin == 0x4) return MIDI_EVENT_NONE;
        if (parser->sysex[parser->sysex_len - 1] != 0xf7) {
            return malformed(parser, event, MIDI_PARSER_ERROR_MALFORMED);
        }
        event->kind = MIDI_EVENT_SYSEX;
        event->len = parser->sysex_len;
        memcpy(event->data, parser->sysex, event->len);
        parser->in_sysex = false;
        parser->sysex_len = 0;
        return event->kind;
    }

    /* MIDI realtime may legally be interleaved with a SysEx stream. */
    if (cin == 0x0f && packet[1] >= 0xf8) {
        event->kind = MIDI_EVENT_REALTIME;
        event->len = 1;
        event->data[0] = packet[1];
        return event->kind;
    }

    if (parser->in_sysex) {
        event->sysex_aborted = true;
        parser->in_sysex = false;
        parser->sysex_len = 0;
    }
    event->kind = MIDI_EVENT_CHANNEL;
    event->len = payload_len;
    memcpy(event->data, &packet[1], payload_len);
    return event->kind;
}
