/**
 * Battery monitoring implementation for Raspberry Pi Pico W
 */

#include "battery.hpp"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cmath>

void Battery::init() {
    // ADC initialization
    printf("Initializing ADC for battery monitoring\n");
    adc_init();
    printf("ADC initialized\n");
    
    // Initialize VSYS pin for ADC reading
    #ifdef PICO_VSYS_PIN
    printf("Initializing VSYS pin %d for ADC\n", PICO_VSYS_PIN);
    adc_gpio_init(PICO_VSYS_PIN);
    #endif
}

float Battery::read_vsys_voltage() {
    #ifndef PICO_VSYS_PIN
    return -1.0f;
    #else
    printf("Reading VSYS voltage\n");
    #if CYW43_USES_VSYS_PIN
    cyw43_thread_enter();
    // Make sure cyw43 is awake by reading VBUS pin
    cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
    #endif

    // Setup ADC for VSYS reading
    adc_select_input(PICO_VSYS_PIN - PICO_FIRST_ADC_PIN);
    printf("ADC input selected for VSYS\n");
    adc_fifo_setup(true, false, 0, false, false);
    adc_run(true);

    // Discard initial readings which tend to be low
    int ignore_count = SAMPLE_COUNT;
    printf("here");
    while (!adc_fifo_is_empty() || ignore_count-- > 0) {
        (void)adc_fifo_get_blocking();
    }
    printf("here2");

    // Take multiple samples and average them
    uint32_t vsys_sum = 0;
    for(int i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t val = adc_fifo_get_blocking();
        vsys_sum += val;
    }
    printf("here3");

    adc_run(false);
    adc_fifo_drain();
    printf("here4");

    uint32_t vsys_avg = vsys_sum / SAMPLE_COUNT;
    
    #if CYW43_USES_VSYS_PIN
    cyw43_thread_exit();
    #endif
    
    // Convert to voltage
    // VSYS is connected through a 3:1 voltage divider, so multiply by 3
    const float conversion_factor = 3.3f / (1 << 12);  // 12-bit ADC
    float voltage = vsys_avg * 3.0f * conversion_factor;
    printf("here54");
    
    return voltage;
    #endif
}

float Battery::get_voltage() {
    return read_vsys_voltage();
}

int Battery::get_battery_percentage() {
    float voltage = get_voltage();
    if (voltage < 0) {
        return -1;
    }
    
    // Only calculate percentage if on battery power
    if (is_usb_powered()) {
        return 100; // Assume full when on USB
    }
    
    // Clamp voltage to battery range
    if (voltage < MIN_BATTERY_VOLTAGE) voltage = MIN_BATTERY_VOLTAGE;
    if (voltage > MAX_BATTERY_VOLTAGE) voltage = MAX_BATTERY_VOLTAGE;
    
    // Calculate percentage
    float percentage = ((voltage - MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE)) * 100.0f;
    return (int)roundf(percentage);
}

bool Battery::is_usb_powered() {
    printf("Checking if USB powered\n");
    #if defined CYW43_WL_GPIO_VBUS_PIN
    // For Pico W, use CYW43 GPIO to check VBUS
    return cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
    #elif defined PICO_VBUS_PIN
    // For regular Pico, check VBUS pin directly
    gpio_set_function(PICO_VBUS_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(PICO_VBUS_PIN, GPIO_IN);
    return gpio_get(PICO_VBUS_PIN);
    #else
    // Unable to determine, assume battery
    return false;
    #endif
}

const char* Battery::get_status_string() {
    printf("Generating battery status string\n");
    float voltage = get_voltage();
    
    bool usb_powered = is_usb_powered();
    
    if (voltage < 0) {
        snprintf(status_buffer, sizeof(status_buffer), "ERR");
        return status_buffer;
    }
    
    if (usb_powered) {
        snprintf(status_buffer, sizeof(status_buffer), "%.1fV USB", voltage);
    } else {
        int percentage = get_battery_percentage();
        if (percentage >= 0) {
            snprintf(status_buffer, sizeof(status_buffer), "%.1fV %d%%", voltage, percentage);
        } else {
            snprintf(status_buffer, sizeof(status_buffer), "%.1fV BAT", voltage);
        }
    }
    
    return status_buffer;
}