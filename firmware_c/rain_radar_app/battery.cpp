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
    // ADC initialization - pin setup is done in read_vsys_voltage()
    adc_init();
}

float Battery::read_vsys_voltage() {
    #ifndef PICO_VSYS_PIN
    return -1.0f;
    #else
    
    #if CYW43_USES_VSYS_PIN
    cyw43_thread_enter();
    // Make sure cyw43 is awake by reading VBUS pin
    cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
    #endif

    // Setup ADC for VSYS reading (must be done each time on Pico W)
    adc_gpio_init(PICO_VSYS_PIN);
    adc_select_input(PICO_VSYS_PIN - PICO_FIRST_ADC_PIN);
    
    adc_fifo_setup(true, false, 0, false, false);
    adc_run(true);

    // Discard initial readings which tend to be low
    int ignore_count = SAMPLE_COUNT;
    while (!adc_fifo_is_empty() || ignore_count-- > 0) {
        (void)adc_fifo_get_blocking();
    }

    // Take multiple samples and average them
    uint32_t vsys_sum = 0;
    for(int i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t val = adc_fifo_get_blocking();
        printf("ADC sample %d: %d\n", i, val);  // Debug print
        vsys_sum += val;
    }

    adc_run(false);
    adc_fifo_drain();

    uint32_t vsys_avg = vsys_sum / SAMPLE_COUNT;
    
    #if CYW43_USES_VSYS_PIN
    cyw43_thread_exit();
    #endif
    
    // Convert to voltage
    // VSYS is connected through a 3:1 voltage divider, so multiply by 3
    const float conversion_factor = 3.3f / (1 << 12);  // 12-bit ADC
    float voltage = vsys_avg * 3.0f * conversion_factor;
    
    printf("ADC avg: %lu, voltage: %.3f\n", vsys_avg, voltage);  // Debug print
    
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