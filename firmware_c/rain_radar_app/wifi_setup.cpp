
#include "wifi_setup.hpp"

#include <cmath>
#include <memory>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include "pimoroni_common.hpp"
#include "pico/cyw43_arch.h"
#include "secrets.h"
#include "rain_radar_common.hpp"


using namespace pimoroni;

namespace
{
  class NetworkLedController
  {
  public:
    NetworkLedController(std::shared_ptr<InkyFrame> frame, int speed_hz)
        : inky_frame(frame), pulse_speed_hz(speed_hz < 0 ? 1 : speed_hz) {
          };

    repeating_timer_t network_led_timer;
    std::shared_ptr<InkyFrame> const inky_frame;
    int const pulse_speed_hz;

    static bool network_led_callback_static(repeating_timer_t *rt)
    {
      auto self = reinterpret_cast<NetworkLedController *>(rt->user_data);
      // ms since boot
      double t_ms = (double)to_ms_since_boot(get_absolute_time());
      // angle = 2*pi * t_ms * freq / 1000
      double angle = 2.0 * M_PI * t_ms * (double)self->pulse_speed_hz / 1000.0;
      double brightness = (sin(angle) * 40.0) + 60.0; // -> range [20,100]
      self->inky_frame->led(InkyFrame::LED_CONNECTION, (uint8_t)brightness);
      return true; // keep repeating
    }

    void start_pulse_network_led()
    {
      add_repeating_timer_ms(50, network_led_callback_static, nullptr, &network_led_timer);
      // important that user_data is set after the timer is added
      network_led_timer.user_data = this;
    };

    void stop_pulse_network_led()
    {
      cancel_repeating_timer(&network_led_timer);
    };
  };
}

namespace wifi_setup
{
Err wifi_connect(InkyFrame &inky_frame)
{
  NetworkLedController led_controller(std::make_shared<InkyFrame>(inky_frame), 1);

  uint32_t timeout_ms = 10000;
  led_controller.start_pulse_network_led();
  sleep_ms(100); // let the LED start

  printf("Connecting to %s...\n", WIFI_SSID);

  if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK))
  {
    printf("failed to initialise\n");
    return Err::NOT_INITIALISED;
  }
  printf("initialised\n");

  cyw43_arch_enable_sta_mode();

  if (!cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK))
  {
    printf("Started connection attempt...\n");
  }
  else
  {
    printf("failed to start connection\n");
    return Err::ERROR;
  }

  uint32_t t_start = millis();

  sleep_ms(2000); // wait a bit before checking status

  while (millis() - t_start < timeout_ms)
  {

    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    switch (link_status)
    {
    case CYW43_LINK_DOWN:
      printf("Wifi status: LINK_DOWN\n");
      break;
    case CYW43_LINK_JOIN:
      printf("Wifi status: LINK_JOIN (associating)\n");
      printf("Connected!\n");
      led_controller.stop_pulse_network_led();
      inky_frame.led(InkyFrame::LED_CONNECTION, 100); // solid on
      return Err::OK;
      break;
    case CYW43_LINK_FAIL:
      printf("Wifi status: LINK_FAIL (connection failed)\n");
      break;
    case CYW43_LINK_NONET:
      printf("Wifi status: LINK_NONET (SSID not found)\n");
      break;
    case CYW43_LINK_BADAUTH:
      printf("Wifi status: LINK_BADAUTH (authentication failure)\n");
      break;
    default:
      if (link_status < 0)
      {
        printf("Wifi status: Unknown error %d\n", link_status);
      }
      break;
    }
    printf("Waiting to connect...\n");
    sleep_ms(1000);
  }
  led_controller.stop_pulse_network_led();
  inky_frame.led(InkyFrame::LED_CONNECTION, 0); // solid off
  return Err::TIMEOUT;
}

bool is_connected()
{
  int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
  return link_status == CYW43_LINK_JOIN;
}

void network_deinit()
{
  cyw43_arch_deinit();
}

} // namespace wifi_setup