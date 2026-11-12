/* Exercise the unchanged production capacitive-button timer across us wrap. */
#include <assert.h>
#include <stdarg.h>
#include "hardware/gpio.h"
#include "pico/time.h"

#define GPIO_OUT true
#define GPIO_IRQ_EDGE_RISE 8u
static uint64_t now_us;
static bool pulse_high;
uint32_t time_us_32(void) { return (uint32_t)now_us; }
void gpio_put(uint pin, bool value) { (void)pin; pulse_high = value; }
void gpio_init(uint pin) { (void)pin; }
void gpio_set_dir(uint pin, bool output) { (void)pin; (void)output; }
void gpio_disable_pulls(uint pin) { (void)pin; }
void gpio_set_irq_enabled_with_callback(uint pin, uint32_t events, bool enabled,
                                       void (*callback)(uint, uint32_t)) {
    (void)pin; (void)events; (void)enabled; (void)callback;
}
void plsdk_printf(const char *format, ...) { (void)format; }

/* The locked SDK uses GNU initializers and has no trailing newline. */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
#pragma clang diagnostic ignored "-Wnewline-eof"
#pragma clang diagnostic ignored "-Wsign-compare"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#include "../PLSDK/include/PLSDK/cap_buttons.h"
#include "../PLSDK/src/cap_buttons.c"
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

static void measure_across(uint64_t start) {
    now_us = start;
    _outVal = 0;
    buttons_task();
    assert(pulse_high);
    now_us += 75;
    _buttons_int_cb(4, GPIO_IRQ_EDGE_RISE);
    assert(button_states[0] == 75);
    now_us = start + _measureInterval - 1;
    buttons_task();
    assert(pulse_high);
    ++now_us;
    buttons_task();
    assert(!pulse_high);
}

int main(void) {
    buttons_add_button(4, 50, NULL, NULL, NULL);
    measure_across(1000000);
    measure_across(UINT32_MAX - UINT64_C(50));
    puts("time_wrap: production button measurement/timeout before and after wrap passed");
    return 0;
}
