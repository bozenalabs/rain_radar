#pragma once

#include <cmath>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include "pico_wireless.hpp"
#include "pimoroni_common.hpp"


using namespace pimoroni;

// Network LED PWM/timer state
static const uint LED_WIFI_PIN = 7;
static repeating_timer_t network_led_timer;
static bool network_led_timer_active = false;
static int network_led_pulse_speed_hz = 1;
static uint network_led_pwm_slice = 0;
static uint network_led_pwm_chan = 0;

// Initialize PWM on LED_WIFI_PIN at ~1kHz and 0 duty
static void network_led_init() {
  gpio_set_function(LED_WIFI_PIN, GPIO_FUNC_PWM);
  network_led_pwm_slice = pwm_gpio_to_slice_num(LED_WIFI_PIN);
  network_led_pwm_chan  = pwm_gpio_to_channel(LED_WIFI_PIN);

  // Use 16-bit wrap to mimic duty_u16()
  pwm_set_wrap(network_led_pwm_slice, 65535);

  // Calculate clkdiv so frequency ~= 1000 Hz: clk_hz / ((wrap+1) * clkdiv) = freq
  double clk_hz = (double)clock_get_hz(clk_sys);
  double clkdiv = clk_hz / ((65536.0) * 1000.0);
  if(clkdiv < 1.0) clkdiv = 1.0; // sane minimum
  pwm_set_clkdiv(network_led_pwm_slice, (float)clkdiv);

  pwm_set_chan_level(network_led_pwm_slice, network_led_pwm_chan, 0);
  pwm_set_enabled(network_led_pwm_slice, true);
}

// Set network LED brightness 0..100 (gamma-corrected, gamma=2.8)
static void network_led(int brightness) {
  if(brightness < 0) brightness = 0;
  if(brightness > 100) brightness = 100;
  double b = (double)brightness / 100.0;
  uint32_t value = (uint32_t)(pow(b, 2.8) * 65535.0 + 0.5);
  pwm_set_chan_level(network_led_pwm_slice, network_led_pwm_chan, value);
}

// Timer callback that pulses the network LED (runs in interrupt context)
static bool network_led_callback(repeating_timer_t *rt) {
  // ms since boot
  double t_ms = (double)to_ms_since_boot(get_absolute_time());
  // angle = 2*pi * t_ms * freq / 1000
  double angle = 2.0 * M_PI * t_ms * (double)network_led_pulse_speed_hz / 1000.0;
  double brightness = (sin(angle) * 40.0) + 60.0; // -> range [20,100]
  double b = brightness / 100.0;
  uint32_t value = (uint32_t)(pow(b, 2.8) * 65535.0 + 0.5);
  pwm_set_chan_level(network_led_pwm_slice, network_led_pwm_chan, value);
  return true; // keep repeating
}

// Start pulsing the network LED at speed_hz (default 1 Hz)
static void pulse_network_led(int speed_hz = 1) {
  network_led_pulse_speed_hz = speed_hz > 0 ? speed_hz : 1;
  if(network_led_timer_active) {
    cancel_repeating_timer(&network_led_timer);
    network_led_timer_active = false;
  }
  // period 50 ms like original
  add_repeating_timer_ms(50, network_led_callback, nullptr, &network_led_timer);
  network_led_timer_active = true;
}

// Stop any pulsing and turn the network LED off
static void stop_network_led() {
  if(network_led_timer_active) {
    cancel_repeating_timer(&network_led_timer);
    network_led_timer_active = false;
  }
  pwm_set_chan_level(network_led_pwm_slice, network_led_pwm_chan, 0);
}

bool wifi_connect(PicoWireless &wireless, uint32_t timeout_ms=10000) {
  printf("Connecting to %s...\n", WIFI_SSID);
  wireless.wifi_set_passphrase(WIFI_SSID, WIFI_PASSWORD);

  IPAddress dns_server = IPAddress(1,1,1,1);

  uint32_t t_start = millis();

  while(millis() - t_start < timeout_ms) {
    if(wireless.get_connection_status() == WL_CONNECTED) {
      printf("Connected!\n");
      wireless.set_dns(1, dns_server, 0);
      return true;
    }
    wireless.set_led(255, 0, 0);
    sleep_ms(500);
    wireless.set_led(0, 0, 0);
    sleep_ms(500);
    printf("...\n");
  }

  return false;
}