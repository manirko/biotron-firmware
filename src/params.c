#include <string.h>
#include <pico/stdlib.h>
#include "params.h"
#include "PLSDK/constants.h"
#include "global.h"
#include "PLSDK/commands.h"
#include "PLSDK/midi_diagnostics.h"
#include "PLSDK/midi_health.h"
#include "PLSDK/music.h"
#include "music.h"
#include "PLSDK.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/bootrom.h>
#include <pico/printf.h>
#include "tusb.h"
#include "runtime_safety.h"
#include "settings_storage.h"
#include "persistence_scheduler.h"
#include "settings_readback.h"

Settings_t settings;
bool isMutedByButton = false;
bool TestMode = false;
bool isTestModeGreen = true;

#define SETTINGS_SAVE_DEBOUNCE_US UINT64_C(1000000)

static Settings_t persisted_settings_snapshot;
static bool persisted_settings_snapshot_valid = false;
static persistence_scheduler_t settings_save_scheduler = {
        .pending = false,
        .last_change_us = 0,
        .debounce_us = SETTINGS_SAVE_DEBOUNCE_US,
};

// region presets
const Settings_t fast_role_preset = {
        .id = ID_FLASH,
        .BPM = BPM_TO_US(404),
        .lightBPM = 8,
        .fraction_note_off = 1,
        .fibPower = 0.5,
        .firstValue = 0.1,
        .filterPercent = 0,
        .scale = 3,
        .minPlantVelocity = 0,
        .maxPlantVelocity = 97,
        .minLightVelocity = 0,
        .maxLightVelocity = 68,
        .random_note = true,
        .same_note_plant = 0,
        .same_note_light = 0,
        .light_note_range = 12,
        .light_pitch_mode = 0,
        .isMutePlantVelocity = 0,
        .isMuteLightVelocity = 0,
        .isRandomPlantVelocity = true,
        .isRandomLightVelocity = true,
        .performance_mode = 0,
        .middle_plant_note = 60,
        .plant_channel = 1,
        .light_channel = 2,
        .swing_first_note_percent = 100,
        .is_mute_button_active = false,
};

const Settings_t the_performer_mode = {
        .id = ID_FLASH,
        .BPM = BPM_TO_US(404),
        .lightBPM = 2,
        .fraction_note_off = 2,
        .fibPower = 0.5,
        .firstValue = 0.1,
        .filterPercent = 0,
        .scale = 6,
        .minPlantVelocity = 8,
        .maxPlantVelocity = 97,
        .minLightVelocity = 0,
        .maxLightVelocity = 54,
        .random_note = false,
        .same_note_plant = 1,
        .same_note_light = 0,
        .light_note_range = 18,
        .light_pitch_mode = false,
        .isMutePlantVelocity = 0,
        .isMuteLightVelocity = true,
        .isRandomPlantVelocity = true,
        .isRandomLightVelocity = true,
        .performance_mode = true,
        .middle_plant_note = 60,
        .plant_channel = 1,
        .light_channel = 2,
        .swing_first_note_percent = 100,
        .is_mute_button_active = false,
};

const Settings_t in_discussion = {
        .id = ID_FLASH,
        .BPM = BPM_TO_US(404),
        .lightBPM = 2,
        .fraction_note_off = 1,
        .fibPower = 0.5,
        .firstValue = 0.1,
        .filterPercent = 0,
        .scale = 5,
        .minPlantVelocity = 44,
        .maxPlantVelocity = 97,
        .minLightVelocity = 0,
        .maxLightVelocity = 54,
        .random_note = false,
        .same_note_plant = 0,
        .same_note_light = 0,
        .light_note_range = 18,
        .light_pitch_mode = false,
        .isMutePlantVelocity = 0,
        .isMuteLightVelocity = 0,
        .isRandomPlantVelocity = true,
        .isRandomLightVelocity = true,
        .performance_mode = true,
        .middle_plant_note = 60,
        .plant_channel = 1,
        .light_channel = 2,
        .swing_first_note_percent = 100,
        .is_mute_button_active = false,
};

const Settings_t mixolyd = {
        .id = ID_FLASH,
        .BPM = BPM_TO_US(462),
        .lightBPM = 4,
        .fraction_note_off = 4,
        .fibPower = 0.5,
        .firstValue = 0.1,
        .filterPercent = 0,
        .scale = 4,
        .minPlantVelocity = 8,
        .maxPlantVelocity = 98,
        .minLightVelocity = 74,
        .maxLightVelocity = 75,
        .random_note = 0,
        .same_note_plant = 1,
        .same_note_light = 0,
        .light_note_range = 12,
        .light_pitch_mode = 0,
        .isMutePlantVelocity = 0,
        .isMuteLightVelocity = true,
        .isRandomPlantVelocity = true,
        .isRandomLightVelocity = 0,
        .performance_mode = true,
        .middle_plant_note = 60,
        .plant_channel = 1,
        .light_channel = 2,
        .swing_first_note_percent = 100,
        .is_mute_button_active = false,
};

#define COUNT_OF_PRESETS 4
const Settings_t * order_of_presets[COUNT_OF_PRESETS] = {
        &mixolyd,
        &fast_role_preset,
        &the_performer_mode,
        &in_discussion
};
// endregion

enum {
    SETTINGS_PROGRAM_BYTES = STORAGE_ROUND_UP(sizeof(Settings_t), FLASH_PAGE_SIZE),
    SETTINGS_ERASE_BYTES = STORAGE_ROUND_UP(SETTINGS_PROGRAM_BYTES, FLASH_SECTOR_SIZE),
};

_Static_assert(SETTINGS_PROGRAM_BYTES >= sizeof(Settings_t),
               "settings program buffer must contain Settings_t");
_Static_assert(SETTINGS_PROGRAM_BYTES % FLASH_PAGE_SIZE == 0,
               "settings program size must be page aligned");
_Static_assert(SETTINGS_ERASE_BYTES % FLASH_SECTOR_SIZE == 0,
               "settings erase size must be sector aligned");

void default_settings() {
    settings = *order_of_presets[0];
    reset_bpm();
}


void save_settings() {
    uint8_t program_data[SETTINGS_PROGRAM_BYTES];
    if (!settings_storage_pack(program_data, sizeof(program_data),
                               &settings, sizeof(settings))) return;

    const uint32_t save_started_us = time_us_32();
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, SETTINGS_ERASE_BYTES);
    flash_range_program(FLASH_TARGET_OFFSET, program_data, sizeof(program_data));
    restore_interrupts(interrupts);
    midi_diagnostics_settings_saved(time_us_32() - save_started_us);
    persisted_settings_snapshot = settings;
    persisted_settings_snapshot_valid = true;
    persistence_note_saved(&settings_save_scheduler);
}

void read_settings() {
    const uint8_t* flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(&settings, flash_target_contents, sizeof(settings));

    if (settings.id != ID_FLASH) {
        default_settings();
        save_settings();
        return;
    }
    const Settings_t stored_settings = settings;
    settings.fibPower = biotron_normalize_percent_setting(
            settings.fibPower, DEF_FIB_POW);
    settings.firstValue = biotron_normalize_percent_setting(
            settings.firstValue, DEF_FIB_FIRST);
    settings.filterPercent = biotron_normalize_percent_setting(
            settings.filterPercent, DEF_FILTER_PERCENT);
    if (memcmp(&settings, &stored_settings, sizeof(settings)) != 0) {
        save_settings();
        return;
    }
    persisted_settings_snapshot = settings;
    persisted_settings_snapshot_valid = true;
    persistence_note_saved(&settings_save_scheduler);
}

/* Compare values, not padding: Settings_t is a shipping flash ABI. */
static bool settings_equal(const Settings_t *left, const Settings_t *right) {
    return left->id == right->id &&
           left->BPM == right->BPM &&
           left->lightBPM == right->lightBPM &&
           left->fibPower == right->fibPower &&
           left->firstValue == right->firstValue &&
           left->filterPercent == right->filterPercent &&
           left->scale == right->scale &&
           left->isRandomPlantVelocity == right->isRandomPlantVelocity &&
           left->isMutePlantVelocity == right->isMutePlantVelocity &&
           left->minPlantVelocity == right->minPlantVelocity &&
           left->maxPlantVelocity == right->maxPlantVelocity &&
           left->isRandomLightVelocity == right->isRandomLightVelocity &&
           left->isMuteLightVelocity == right->isMuteLightVelocity &&
           left->minLightVelocity == right->minLightVelocity &&
           left->maxLightVelocity == right->maxLightVelocity &&
           left->random_note == right->random_note &&
           left->same_note_plant == right->same_note_plant &&
           left->same_note_light == right->same_note_light &&
           left->fraction_note_off == right->fraction_note_off &&
           left->light_note_range == right->light_note_range &&
           left->light_pitch_mode == right->light_pitch_mode &&
           left->performance_mode == right->performance_mode &&
           left->middle_plant_note == right->middle_plant_note &&
           left->plant_channel == right->plant_channel &&
           left->light_channel == right->light_channel &&
           left->swing_first_note_percent == right->swing_first_note_percent &&
           left->is_mute_button_active == right->is_mute_button_active;
}

static bool settings_differ_from_persisted(void) {
    return !persisted_settings_snapshot_valid ||
           !settings_equal(&settings, &persisted_settings_snapshot);
}

static void schedule_settings_save(void) {
    const bool dirty = settings_differ_from_persisted();
    midi_diagnostics_settings_changed(dirty);
    if (dirty) {
        persistence_note_change(&settings_save_scheduler, time_us_64());
    } else {
        persistence_note_saved(&settings_save_scheduler);
    }
}

static void save_pending_settings_now(void) {
    if (settings_differ_from_persisted()) save_settings();
    else persistence_note_saved(&settings_save_scheduler);
}

void service_settings_persistence(void) {
    /* Flash stalls sampling IRQs; keep an already-running measurement intact
     * and persist the pending value as soon as calibration leaves this state. */
    if (status == Stabilization) return;
    if (!persistence_is_due(&settings_save_scheduler, time_us_64())) return;
    save_pending_settings_now();
}

//region MIDI commands

void change_plant_bpm(uint16_t bpm) {
    if (bpm == 0) return;
    settings.BPM = BPM_TO_US(bpm);

    if (status == Active) {
        reset_bpm();
    }
}

void change_plant_bpm_sys_ex(const uint8_t data[], uint8_t len) {
    uint16_t bpm = 0;
    for (int i = 0; i < len; i++) {
        bpm += data[i];
    }
    change_plant_bpm(bpm);
}


void change_light_bpm_sys_ex(const uint8_t data[], uint8_t len) {
    settings.lightBPM = data[0];
}

void change_bpm_cc(uint8_t channel, uint8_t value) {
    switch (channel) {
        case 0:
            change_plant_bpm(value * 5);
            break;
        case 1:
            settings.lightBPM = value;
            break;
        default:
            break;
    }
}

void set_fib_power_sys_ex(const uint8_t data[], uint8_t len) {
    settings.fibPower = (double )data[0] / 100;
}

void set_fib_power_cc(uint8_t channel, uint8_t value) {
    settings.fibPower = (double )value / 127;
}

void set_fib_first_sys_ex(const uint8_t data[], uint8_t len) {
    settings.firstValue = (double )data[0] / 100;
}

void set_fib_first_cc(uint8_t channel, uint8_t value) {
    settings.firstValue = (double )value / 127;
}

void set_filter_sys_ex(const uint8_t data[], uint8_t len) {
    settings.filterPercent = (double )data[0] / 100;
}

void set_filter_cc(uint8_t channel, uint8_t value) {
    settings.filterPercent = (double )value / 127;
}

void set_scale_sys_ex(const uint8_t data[], uint8_t len) {
    settings.scale = data[0] % SCALES_COUNT;
}

void set_scale_cc(uint8_t channel, uint8_t value) {
    settings.scale = ((int)value * SCALES_COUNT) / 128;
}

void set_max_plant_vel_sys_ex(const uint8_t data[], uint8_t len) {
    settings.maxPlantVelocity = data[0];
}

void set_min_plant_vel_sys_ex(const uint8_t data[], uint8_t len) {
    settings.minPlantVelocity = data[0];
}

void set_random_plant_vel_sys_ex(const uint8_t data[], uint8_t len) {
    settings.isRandomPlantVelocity = data[0] > 0;
}

void set_max_light_vel_sys_ex(const uint8_t data[], uint8_t len) {
    settings.maxLightVelocity = data[0];
}

void set_min_light_vel_sys_ex(const uint8_t data[], uint8_t len) {
    settings.minLightVelocity = data[0];
}

void set_random_light_vel_sys_ex(const uint8_t data[], uint8_t len) {
    settings.isRandomLightVelocity = data[0] > 0;
}

void set_mute_plant_vel_sys_ex(const uint8_t data[], uint8_t len) {
    settings.isMutePlantVelocity = data[0] > 0;
    if (settings.isMutePlantVelocity) stop_plant_midi();
}

void set_mute_light_vel_sys_ex(const uint8_t data[], uint8_t len) {
    settings.isMuteLightVelocity = data[0] > 0;
    if (settings.isMuteLightVelocity) stop_light_midi();
}


void set_max_vel_cc(uint8_t channel, uint8_t value) {
    switch (channel) {
        case 0:
            settings.maxPlantVelocity = value;
            break;
        case 1:
            settings.maxLightVelocity = value;
            break;
        default:
            break;
    }
}

void set_min_vel_cc(uint8_t channel, uint8_t value) {
    switch (channel) {
        case 0:
            settings.minPlantVelocity = value;
            break;
        case 1:
            settings.minLightVelocity = value;
            break;
        default:
            break;
    }
}

void set_random_vel_cc(uint8_t channel, uint8_t value) {
    switch (channel) {
        case 0:
            settings.isRandomPlantVelocity = value >= 64;
            break;
        case 1:
            settings.isRandomLightVelocity = value >= 64;
            break;
        default:
            break;
    }
}


void set_mute_cc(uint8_t channel, uint8_t value) {
    switch (channel) {
        case 0:
            settings.isMutePlantVelocity = value >= 64;
            if (settings.isMutePlantVelocity) stop_plant_midi();
            break;
        case 1:
            settings.isMuteLightVelocity = value >= 64;
            if (settings.isMuteLightVelocity) stop_light_midi();
            break;
        default:
            break;
    }
}

void set_default_sys_ex(const uint8_t data[], uint8_t len) {
    default_settings();
    reset_bpm();
}

void set_random_note_sys_ex(const uint8_t data[], uint8_t len) {
    settings.random_note = data[0] > 0;
}

void set_random_note_cc(uint8_t channel, uint8_t value) {
    settings.random_note = value > 63;
}

void set_same_note_plant_sys_ex(const uint8_t data[], uint8_t len) {
    settings.same_note_plant = data[0];
}

void set_same_note_light_sys_ex(const uint8_t data[], uint8_t len) {
    settings.same_note_light = data[0];
}

void set_same_note_cc(uint8_t channel, uint8_t value) {
    switch (channel) {
        case 0:
            settings.same_note_plant = value;
            break;
        case 1:
            settings.same_note_light = value;
            break;
        default:
            break;
    }

}

#define LENGTH_POSSIBLE_NOTE_FRACTION 11
static const int POSSIBLE_NOTE_FRACTION[LENGTH_POSSIBLE_NOTE_FRACTION] = {
        64, 48, 32, 24, 16, 12, 8, 6, 4, 2, 1
};

void set_note_off_percent_sys_ex(const uint8_t data[], uint8_t len) {
    for (int i = 0; i < LENGTH_POSSIBLE_NOTE_FRACTION; i++) {
        if (data[0] == POSSIBLE_NOTE_FRACTION[i]) {
            settings.fraction_note_off = data[0];
            return;
        }
    }
}

void set_note_off_percent_cc(uint8_t channel, uint8_t value) {
    uint8_t id = MIN(LENGTH_POSSIBLE_NOTE_FRACTION - 1, value / (127 / LENGTH_POSSIBLE_NOTE_FRACTION));
    settings.fraction_note_off = POSSIBLE_NOTE_FRACTION[id];
}

void set_light_range_sys_ex(const uint8_t data[], uint8_t len) {
    settings.light_note_range = data[0];
}

void set_light_range_cc(const uint8_t channel, uint8_t value) {
    settings.light_note_range = value;
}

void set_light_pitch_mode_sys_ex(const uint8_t data[], uint8_t len) {
    const bool enabled = data[0] > 0;
    if (settings.light_pitch_mode != enabled) stop_light_midi();
    settings.light_pitch_mode = enabled;
    change_pitch(settings.plant_channel, 0, 64);
}

void set_light_pitch_mode_cc(uint8_t channel, uint8_t value) {
    const bool enabled = value > 63;
    if (settings.light_pitch_mode != enabled) stop_light_midi();
    settings.light_pitch_mode = enabled;
    change_pitch(settings.plant_channel, 0, 64);
}

void set_stuck_mode_sys_ex(const uint8_t data[], uint8_t len) {
    settings.performance_mode = data[0] > 0;
}

void set_stuck_mode_cc(uint8_t channel, uint8_t value) {
    settings.performance_mode = value > 63;
}

void set_middle_plant_note_sys_ex(const uint8_t data[], uint8_t len) {
    settings.middle_plant_note = data[0];
}

void set_middle_plant_note_cc(uint8_t channel, uint8_t value) {
    settings.middle_plant_note = value;
}

void set_swing_first_note_percent_sys_ex(const uint8_t data[], uint8_t len) {
    if (len < 1 || data[0] > 100) return;
    settings.swing_first_note_percent = MAX(1, data[0]);
    refresh_music_alarm_timing();
}

void set_swing_first_note_percent_cc(uint8_t channel, uint8_t value) {
    settings.swing_first_note_percent = MAX(1, value / 127.0 * 100);
    refresh_music_alarm_timing();
}

void set_channel_sys_ex(const uint8_t data[], uint8_t len) {
//    printf("%d %d %d\n", len, data[0], data[1]);
    if (len != 2)
        return;

    if (data[0] >= 2 || data[1] >= 16) {
        return;
    }

    if (data[0] == 0) {
        stop_plant_midi();
        settings.plant_channel = data[1];
    }
    else {
        stop_light_midi();
        settings.light_channel = data[1];
    }
}

void get_info_sys_ex(const uint8_t data[], uint8_t len) {
    if (len != 1) return;

    uint8_t sys_ex_info[] = {SYS_EX_START, PLAYTRONICA_SYS_KEY, 126, data[0],
                             MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, SYS_EX_END};
    print_pure(0, sys_ex_info, 8);
    print_pure(1, sys_ex_info, 8);
}

void get_health_sys_ex(const uint8_t data[], uint8_t len) {
    if (len != 1 || data[0] >= MIDI_HEALTH_PAGE_COUNT) return;
    midi_diagnostics_snapshot_t snapshot;
    uint8_t payload[MIDI_HEALTH_MAX_PAYLOAD_BYTES];
    midi_diagnostics_snapshot(&snapshot);
    const size_t payload_length = midi_health_encode_page(
            &snapshot, data[0], payload, sizeof payload);
    if (payload_length > 0 && payload_length <= UINT8_MAX) {
        print_sys_ex_reply(payload, (uint8_t)payload_length);
    }
}

void get_settings_sys_ex(const uint8_t data[], uint8_t len) {
    if (len != 2 || data[0] > BIOTRON_SETTINGS_SOURCE_PERSISTED) return;

    midi_diagnostics_snapshot_t snapshot;
    uint8_t payload[BIOTRON_SETTINGS_PAYLOAD_BYTES];
    const bool persisted = data[0] == BIOTRON_SETTINGS_SOURCE_PERSISTED;
    const bool source_valid = !persisted || persisted_settings_snapshot_valid;
    const Settings_t *source = persisted ? &persisted_settings_snapshot :
                                           &settings;
    midi_diagnostics_snapshot(&snapshot);
    const size_t payload_length = biotron_settings_encode(
            source, source_valid, settings_differ_from_persisted(), data[0],
            data[1], snapshot.settings_dirty_generation,
            snapshot.settings_persisted_generation, payload, sizeof payload);
    if (payload_length > 0 && payload_length <= UINT8_MAX) {
        print_sys_ex_reply(payload, (uint8_t)payload_length);
    }
}

void start_plant_calibration_sys_ex(const uint8_t data[], uint8_t len) {
    if (len == 1) {
        start_plant_calibration(data[0]);
        return;
    }
    if (len < 2) return;
    const uint8_t nonce = data[0];
    if (data[1] == BIOTRON_CALIBRATION_TELEMETRY && len == 2) {
        report_calibration_telemetry(nonce);
    } else if (data[1] == BIOTRON_CALIBRATION_SET_REFERENCE && len == 10) {
        uint32_t baseline = 0, noise = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            baseline |= (uint32_t)data[2 + i] << (i * 7u);
            noise |= (uint32_t)data[6 + i] << (i * 7u);
        }
        set_manual_calibration_reference(baseline, noise);
        report_calibration_telemetry(nonce);
    } else if (data[1] == BIOTRON_CALIBRATION_RESET_REFERENCE && len == 2) {
        reset_calibration_reference();
        report_calibration_telemetry(nonce);
    }
}


void set_button_mode_state_sys_ex(const uint8_t data[], uint8_t len) {
    if (len != 1) return;
    settings.is_mute_button_active = data[0];
    if (!settings.is_mute_button_active) {
        isMutedByButton = false;
    }

}

void set_button_mode_state_cc(uint8_t channel, uint8_t value) {
    settings.is_mute_button_active = value > 63;
    if (!settings.is_mute_button_active) {
        isMutedByButton = false;
    }
}
//endregion


void setup_commands() {
    add_sys_ex_com_len(change_plant_bpm_sys_ex, 0, 1);
    add_sys_ex_com_range(change_light_bpm_sys_ex, 9, 1, 1);
    add_CC(change_bpm_cc, 14);

    add_sys_ex_com_range(set_fib_power_sys_ex, 1, 1, 1);
    add_CC(set_fib_power_cc, 22);

    add_sys_ex_com_range(set_fib_first_sys_ex, 2, 1, 1);
    add_CC(set_fib_first_cc, 23);

    add_sys_ex_com_range(set_filter_sys_ex, 3, 1, 1);
    add_CC(set_filter_cc, 3);

    add_sys_ex_com_range(set_scale_sys_ex, 4, 1, 1);
    add_CC(set_scale_cc, 24);

    add_sys_ex_com_range(set_max_plant_vel_sys_ex, 5, 1, 1);
    add_sys_ex_com_range(set_max_light_vel_sys_ex, 6, 1, 1);
    add_sys_ex_com_range(set_min_plant_vel_sys_ex, 15, 1, 1);
    add_sys_ex_com_range(set_min_light_vel_sys_ex, 17, 1, 1);
    add_sys_ex_com_range(set_random_plant_vel_sys_ex, 16, 1, 1);
    add_sys_ex_com_range(set_random_light_vel_sys_ex, 18, 1, 1);
    add_sys_ex_com_range(set_mute_plant_vel_sys_ex, 22, 1, 1);
    add_sys_ex_com_range(set_mute_light_vel_sys_ex, 23, 1, 1);

    add_CC(set_max_vel_cc, 9);
    add_CC(set_min_vel_cc, 25);
    add_CC(set_random_vel_cc, 26);
    add_CC(set_mute_cc, 31);

    add_sys_ex_com_range(set_default_sys_ex, 7, 0, 0);

    add_sys_ex_com_range(set_random_note_sys_ex, 10, 1, 1);
    add_CC(set_random_note_cc, 15);

    add_sys_ex_com_range(set_same_note_plant_sys_ex, 11, 1, 1);
    add_sys_ex_com_range(set_same_note_light_sys_ex, 24, 1, 1);
    add_CC(set_same_note_cc, 20);

    add_sys_ex_com_range(set_note_off_percent_sys_ex, 12, 1, 1);
    add_CC(set_note_off_percent_cc, 21);

    add_sys_ex_com_range(set_light_range_sys_ex, 13, 1, 1);
    add_CC(set_light_range_cc, 28);

    add_sys_ex_com_range(set_light_pitch_mode_sys_ex, 19, 1, 1);
    add_CC(set_light_pitch_mode_cc, 27);

    add_sys_ex_com_range(set_stuck_mode_sys_ex, 21, 1, 1);
    add_CC(set_stuck_mode_cc, 30);

    add_sys_ex_com_range(set_middle_plant_note_sys_ex, 25, 1, 1);
    add_CC(set_middle_plant_note_cc, 85);

    add_sys_ex_com_range(set_swing_first_note_percent_sys_ex, 26, 1, 1);
    add_CC(set_swing_first_note_percent_cc, 86);

    add_sys_ex_com_range(set_button_mode_state_sys_ex, 27, 1, 1);
    add_CC(set_button_mode_state_cc, 87);

    add_sys_ex_com_range(set_channel_sys_ex, 127, 2, 2);
    // Runtime-only action. The non-persisting registration is intentional:
    // recalibration must never schedule a settings flash write.
    add_sys_ex_query_range(get_settings_sys_ex,
                         BIOTRON_SETTINGS_QUERY_ID, 2, 2);
    add_sys_ex_query_len(start_plant_calibration_sys_ex,
                         BIOTRON_RECALIBRATE_COMMAND, 1);
    add_sys_ex_query_range(get_health_sys_ex, 124, 1, 1);
    add_sys_ex_query_range(get_info_sys_ex, 126, 1, 1);
}


void get_sys_ex_and_behave() {
    midi_diagnostics_service(time_us_64(), tud_midi_available());
    for (uint8_t packet = 0; packet < 32; ++packet) {
        Settings_t before;
        memcpy(&before, &settings, sizeof before);
        const int sys_ex_status = read_sys_ex();
        if (sys_ex_status == UNKNOWN) return;

        switch (sys_ex_status) {
            case RESET_DEVICE:
                // Entering BOOT for an update must not erase user settings.
                save_pending_settings_now();
                reset_usb_boot(0, 0);
                return;
            case TEST_MODE_BLUE_ACTIVATE:
                TestMode = true;
                isTestModeGreen = false;
                break;
            case TEST_MODE_GREEN_ACTIVATE:
                TestMode = true;
                isTestModeGreen = true;
                break;
            case TEST_MODE_DEACTIVATE:
                TestMode = false;
                break;
            case LIST_OF_COMMANDS_ACTION:
                load_settings();
                break;
            case CUSTOM_COMMAND:
            case CUSTOM_CC_COMMAND:
                if (!settings_equal(&before, &settings)) schedule_settings_save();
                break;
            case CUSTOM_QUERY_COMMAND:
            case MIDI_PACKET_IGNORED:
            case BPM_CLOCK_INACTIVE:
                break;
            case BPM_CLOCK_PLAY:
                play_music_bpm_clock();
                break;
            case BPM_CLOCK_DEACTIVATE:
                bpm_clock_control(false);
                break;
            case BPM_CLOCK_ACTIVATE:
                bpm_clock_control(true);
                break;
            default:
                break;
        }
    }
}

void set_next_preset() {
    static uint counter = 0;
    settings = *order_of_presets[counter];
    midi_diagnostics_settings_changed(settings_differ_from_persisted());
    stop_bpm();
    counter = (counter + 1) % COUNT_OF_PRESETS;
    save_settings();

    for (int counter = 0; counter < 4; counter++) {
        uint note = calculate_note_by_scale(settings.middle_plant_note, counter, settings.scale);
        note_on(settings.plant_channel, note, 127);
        uint32_t time = time_us_32();
        while (time_us_32() - time < 500000) remind_midi();
        note_off(settings.plant_channel, note);
    }

    reset_bpm();
}
