
#include "network_utils.hpp"

#include <cmath>
#include <memory>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include "pico_wireless.hpp"
#include "pimoroni_common.hpp"

using namespace pimoroni;

namespace
{
class NetworkLedController {
public:
    NetworkLedController(std::shared_ptr<InkyFrame> frame, int speed_hz=1)
      : inky_frame(frame), pulse_speed_hz(speed_hz < 0? 1 : speed_hz) {
        network_led_timer_active = false;
      };


    repeating_timer_t network_led_timer;
    std::shared_ptr<InkyFrame> inky_frame;
    int pulse_speed_hz;
    bool network_led_timer_active;

    static bool network_led_callback_static(repeating_timer_t* rt) {

      auto self = reinterpret_cast<NetworkLedController*>(rt->user_data);

      // ms since boot
      double t_ms = (double)to_ms_since_boot(get_absolute_time());
      // angle = 2*pi * t_ms * freq / 1000
      double angle = 2.0 * M_PI * t_ms * (double)self->pulse_speed_hz / 1000.0;
      double brightness = (sin(angle) * 40.0) + 60.0; // -> range [20,100]

      self->inky_frame->led(InkyFrame::LED_CONNECTION, (uint8_t)brightness);

      return true; // keep repeating
    }

  // Start pulsing the network LED at speed_hz
  void start_pulse_network_led() {

    if (network_led_timer_active) {
        cancel_repeating_timer(&network_led_timer);
        network_led_timer_active = false;
    }

    add_repeating_timer_ms(50, network_led_callback_static, nullptr, &network_led_timer);
    network_led_timer_active = true;
  };


  void stop_pulse_network_led() {
    if (network_led_timer_active) {
        cancel_repeating_timer(&network_led_timer);
        network_led_timer_active = false;
    }
  };

};
}



bool wifi_connect(PicoWireless &wireless, InkyFrame &inky_frame)
{

  NetworkLedController led_controller(std::make_shared<InkyFrame>(inky_frame), 1);


  uint32_t timeout_ms = 10000;
  led_controller.start_pulse_network_led();
  sleep_ms(100); // let the LED start

  printf("Connecting to %s...\n", WIFI_SSID);
  wireless.wifi_set_passphrase(WIFI_SSID, WIFI_PASSWORD);

  IPAddress dns_server = IPAddress(1, 1, 1, 1);

  uint32_t t_start = millis();

  while (millis() - t_start < timeout_ms)
  {
    if (wireless.get_connection_status() == WL_CONNECTED)
    {
      printf("Connected!\n");
      wireless.set_dns(1, dns_server, 0);
      led_controller.stop_pulse_network_led();
      inky_frame.led(InkyFrame::LED_CONNECTION, 100); // solid on
      return true;
    }
    wireless.set_led(255, 0, 0);
    sleep_ms(500);
    wireless.set_led(0, 0, 0);
    sleep_ms(500);
    printf("...\n");
  }
  led_controller.stop_pulse_network_led();
  inky_frame.led(InkyFrame::LED_CONNECTION, 0); // solid on
  return false;
}