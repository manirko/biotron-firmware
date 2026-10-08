
/* Offline source review. USB/time/flash/BOOT are local mocks; no device I/O. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "params.h"
#include "global.h"
#include "PLSDK/commands.h"
#include "PLSDK/midi_diagnostics.h"
#include "tusb.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/bootrom.h"
bool LOGGER_FLAG = false;
enum Status status = Active;
enum Status active_status = Active;
static uint64_t now_us;
static uint32_t erase_calls, program_calls, boot_calls;
static uint8_t flash_memory[FLASH_SECTOR_SIZE];
#undef XIP_BASE
#define XIP_BASE ((uintptr_t)flash_memory - FLASH_TARGET_OFFSET)
static uint8_t queue[128][4];
static size_t queue_read, queue_write;
uint64_t time_us_64(void) { return now_us; }
uint32_t time_us_32(void) { return (uint32_t)now_us; }
uint32_t save_and_disable_interrupts(void) { return 0; }
void restore_interrupts(uint32_t ignored) { (void)ignored; }
void flash_range_erase(uint32_t offset,size_t count) {
    assert(offset==FLASH_TARGET_OFFSET && count<=sizeof flash_memory);
    memset(flash_memory,0xff,count);++erase_calls;
}
void flash_range_program(uint32_t offset,const uint8_t *data,size_t count) {
    assert(offset==FLASH_TARGET_OFFSET && count<=sizeof flash_memory);
    memcpy(flash_memory,data,count);++program_calls;
}
void reset_usb_boot(uint32_t mask,uint32_t interfaces) {
    assert(mask==0 && interfaces==0);++boot_calls;
}
void reset_bpm(void) { }
void load_settings(void) { }
void bpm_clock_control(bool enabled) { (void)enabled; }
void play_music_bpm_clock(void) { }
void remind_midi(void) { }
void plsdk_printf(const char *format,...) { (void)format; }
bool tud_midi_packet_read(uint8_t packet[4]) {
    if(queue_read==queue_write) return false;
    memcpy(packet,queue[queue_read++],4);return true;
}
uint32_t tud_midi_available(void) { return (uint32_t)((queue_write-queue_read)*4); }
uint32_t tud_midi_stream_write(uint8_t cable,const uint8_t *data,uint32_t length) {
    (void)cable;(void)data;return length;
}
#include "../src/params.c"
static void enqueue(uint8_t header,uint8_t a,uint8_t b,uint8_t c) {
    assert(queue_write<128);
    uint8_t packet[4]={header,a,b,c}; memcpy(queue[queue_write++],packet,4);
}
static void send_filter_cc(uint8_t value) {
    enqueue(0x0b,0xbf,3,value);get_sys_ex_and_behave();
}
static void enqueue_sysex(const uint8_t *message,size_t length) {
    size_t at=0;
    while(at<length) {
        size_t remaining=length-at;
        uint8_t cin=remaining>3 ? 4 : (uint8_t)(4+remaining);
        uint8_t packet[4]={(uint8_t)(0x10|cin),0,0,0};
        size_t count=remaining>3 ? 3 : remaining;
        memcpy(packet+1,message+at,count);
        enqueue(packet[0],packet[1],packet[2],packet[3]);at+=count;
    }
}

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "persistence_scheduler.h"

static void test_scheduler(void) {
    persistence_scheduler_t scheduler = {
        .pending = false,
        .last_change_us = 0,
        .debounce_us = UINT64_C(1000000),
    };
    persistence_note_change(&scheduler, 100);
    assert(!persistence_is_due(&scheduler, 1000099));
    assert(persistence_is_due(&scheduler, 1000100));
    persistence_note_saved(&scheduler);
    assert(!persistence_is_due(&scheduler, 2000000));

    persistence_note_change(&scheduler, 3000000);
    persistence_note_change(&scheduler, 3500000);
    assert(!persistence_is_due(&scheduler, 4499999));
    assert(persistence_is_due(&scheduler, 4500000));

    scheduler.debounce_us = 20;
    persistence_note_change(&scheduler, UINT64_MAX - 9);
    assert(!persistence_is_due(&scheduler, 9));
    assert(persistence_is_due(&scheduler, 10));
    puts("persistence_scheduler: debounce, coalescing and wrap passed");
    return;
}

int main(void) {
    test_scheduler();
    midi_diagnostics_reset();
    settings = mixolyd;
    save_settings();
    const uint32_t initial_saves = program_calls;
    add_CC(set_filter_cc, 3);
    add_sys_ex_com_range(set_fib_power_sys_ex, 1, 1, 1);
    /* F04: repeated dirty no-ops never postpone the original deadline. */
    send_filter_cc(64);
    assert(settings_save_scheduler.last_change_us == 0);
    now_us = 900000;
    send_filter_cc(64);
    assert(settings_save_scheduler.last_change_us == 0);
    now_us = 1000000;
    service_settings_persistence();
    assert(program_calls == initial_saves + 1);
    assert(!settings_save_scheduler.pending);
    for (int i = 0; i < 20; ++i) send_filter_cc(64);
    service_settings_persistence();
    assert(program_calls == initial_saves + 1);
    midi_diagnostics_snapshot_t diagnostics;
    midi_diagnostics_snapshot(&diagnostics);
    assert(diagnostics.settings_dirty_generation == 1);
    /* Padding is not a setting and must not create a dirty save. */
    ((uint8_t *)&settings)[12] ^= 0xff;
    assert(!settings_differ_from_persisted());
    send_filter_cc(65);
    now_us = 1500000;
    send_filter_cc(66);
    assert(settings_save_scheduler.last_change_us == now_us);
    status = Stabilization;
    now_us = 2500000;
    service_settings_persistence();
    assert(program_calls == initial_saves + 1);
    status = Active;
    service_settings_persistence();
    assert(program_calls == initial_saves + 2);
    /* Missing/extra payload and malformed bytes do not advance pending save. */
    send_filter_cc(67);
    const uint64_t deadline = settings_save_scheduler.last_change_us;
    now_us += 500000;
    const uint8_t invalid[][7] = {
        {0xf0,0x14,0x0d,1,0xf7},
        {0xf0,0x14,0x0d,1,50,1,0xf7},
        {0xf0,0x14,0x0d,1,0xff,0xf7}
    };
    const size_t lengths[] = {5,7,6};
    for (size_t i = 0; i < 3; ++i) {
        Settings_t before = settings;
        enqueue_sysex(invalid[i], lengths[i]);
        get_sys_ex_and_behave();
        assert(settings_equal(&before, &settings));
        assert(settings_save_scheduler.last_change_us == deadline);
    }
    now_us = deadline + SETTINGS_SAVE_DEBOUNCE_US;
    service_settings_persistence();
    /* F03: invalid percent never mutates/schedules; endpoints roundtrip.
     * Red if the old unchecked percent setter is restored. */
    for (uint16_t value = 101; value <= 127; ++value) {
        const uint8_t message[] = {0xf0,0x14,0x0d,1,(uint8_t)value,0xf7};
        Settings_t before = settings;
        enqueue_sysex(message, sizeof message);
        get_sys_ex_and_behave();
        assert(settings_equal(&before, &settings));
        assert(!settings_save_scheduler.pending);
    }
    void (*percent_sysex[])(const uint8_t[], uint8_t) = {
        set_fib_power_sys_ex, set_fib_first_sys_ex
    };
    void (*percent_cc[])(uint8_t, uint8_t) = {
        set_fib_power_cc, set_fib_first_cc
    };
    for (size_t field = 0; field < 2; ++field) {
        for (uint16_t value = 0; value <= 100; ++value) {
            const uint8_t data[] = {(uint8_t)value};
            percent_sysex[field](data, 1);
            assert((field == 0 ? settings.fibPower : settings.firstValue) == value / 100.0);
            save_settings();
            Settings_t before = settings;
            read_settings();
            assert(settings_equal(&before, &settings));
        }
        for (uint16_t value = 0; value <= 127; ++value) {
            percent_cc[field](0, (uint8_t)value);
            assert((field == 0 ? settings.fibPower : settings.firstValue) == value / 127.0);
            save_settings();
            Settings_t before = settings;
            read_settings();
            assert(settings_equal(&before, &settings));
        }
        Settings_t before = settings;
        for (uint16_t value = 128; value <= 255; ++value) {
            percent_cc[field](0, (uint8_t)value);
            assert(settings_equal(&before, &settings));
        }
    }
    Settings_t legacy = mixolyd;
    legacy.fibPower = 50;
    legacy.firstValue = 10;
    memcpy(flash_memory, &legacy, sizeof legacy);
    read_settings();
    assert(settings.fibPower == 0.5 && settings.firstValue == 0.1);
    const uint32_t migrated_saves = program_calls;
    read_settings();
    assert(program_calls == migrated_saves);
    /* F05: valid golden presets preserve every byte and do not write back. */
    for (size_t preset = 0; preset < COUNT_OF_PRESETS; ++preset) {
        memcpy(flash_memory, order_of_presets[preset], sizeof settings);
        const uint32_t saves = program_calls;
        read_settings();
        assert(memcmp(&settings, order_of_presets[preset], sizeof settings) == 0);
        assert(program_calls == saves && persisted_settings_snapshot_valid);
    }
    const size_t bool_offsets[] = {
        offsetof(Settings_t,isRandomPlantVelocity), offsetof(Settings_t,isMutePlantVelocity),
        offsetof(Settings_t,isRandomLightVelocity), offsetof(Settings_t,isMuteLightVelocity),
        offsetof(Settings_t,random_note), offsetof(Settings_t,light_pitch_mode),
        offsetof(Settings_t,performance_mode), offsetof(Settings_t,is_mute_button_active)
    };
    for (size_t i = 0; i < sizeof bool_offsets / sizeof bool_offsets[0]; ++i) {
        for (unsigned byte = 2; byte <= 255; ++byte) {
            memcpy(flash_memory, &mixolyd, sizeof settings);
            flash_memory[bool_offsets[i]] = (uint8_t)byte;
            const uint32_t saves = program_calls;
            read_settings();
            assert(settings_equal(&settings, &mixolyd));
            assert(!persisted_settings_snapshot_valid && !settings_save_scheduler.pending);
            assert(program_calls == saves);
        }
    }
    const size_t integer_offsets[] = {
        offsetof(Settings_t,BPM), offsetof(Settings_t,lightBPM), offsetof(Settings_t,scale),
        offsetof(Settings_t,minPlantVelocity), offsetof(Settings_t,maxPlantVelocity),
        offsetof(Settings_t,minLightVelocity), offsetof(Settings_t,maxLightVelocity),
        offsetof(Settings_t,same_note_plant), offsetof(Settings_t,same_note_light),
        offsetof(Settings_t,fraction_note_off), offsetof(Settings_t,light_note_range),
        offsetof(Settings_t,middle_plant_note), offsetof(Settings_t,plant_channel),
        offsetof(Settings_t,light_channel), offsetof(Settings_t,swing_first_note_percent)
    };
    const int invalid_ints[] = {INT_MIN,-1,INT_MAX};
    for (size_t i = 0; i < sizeof integer_offsets / sizeof integer_offsets[0]; ++i) {
        for (size_t v = 0; v < sizeof invalid_ints / sizeof invalid_ints[0]; ++v) {
            memcpy(flash_memory, &mixolyd, sizeof settings);
            memcpy(flash_memory + integer_offsets[i], &invalid_ints[v], sizeof(int));
            const uint32_t saves = program_calls;
            read_settings();
            assert(settings_equal(&settings, &mixolyd) && !persisted_settings_snapshot_valid);
            assert(program_calls == saves);
        }
    }
    const size_t percent_offsets[] = {offsetof(Settings_t,fibPower),
        offsetof(Settings_t,firstValue),offsetof(Settings_t,filterPercent)};
    const double invalid_doubles[] = {NAN,INFINITY,-INFINITY,-0.1,100.1};
    for (size_t i = 0; i < 3; ++i) {
        for (size_t v = 0; v < sizeof invalid_doubles / sizeof invalid_doubles[0]; ++v) {
            memcpy(flash_memory, &mixolyd, sizeof settings);
            memcpy(flash_memory + percent_offsets[i], &invalid_doubles[v], sizeof(double));
            const uint32_t saves = program_calls;
            read_settings();
            assert(settings_equal(&settings, &mixolyd) && !persisted_settings_snapshot_valid);
            assert(program_calls == saves);
        }
    }
    puts("load_production: golden ABI, raw bool bytes, integer/percent domains and RAM fallback passed");
    puts("percent_production: SysEx0-100, CC0-127, invalid input and legacy migration passed");
    puts("persistence_production: real changes, dirty no-ops, padding and invalid input passed");
    return 0;
}
