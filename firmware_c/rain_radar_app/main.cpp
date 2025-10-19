
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <string>

#include "drivers/inky73/inky73.hpp"
#include "drivers/psram_display/psram_display.hpp"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "inky_frame_7.hpp"
#include "wifi_setup.hpp"
#include "pico/stdlib.h"
#include "pimoroni_common.hpp"
#include "secrets.h"
#include "data_fetching.hpp"
#include "rain_radar_common.hpp"

using namespace pimoroni;


void draw_error(InkyFrame &graphics, const std::string_view &msg)
{

  graphics.set_pen(Inky73::RED);
  graphics.rectangle(Rect(graphics.width / 2, graphics.height - 25, graphics.width / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(graphics.width / 2 + 5, graphics.height - 22), graphics.width / 2, 2);
}

void draw_lower_left_text(InkyFrame &graphics, const std::string_view &msg)
{
  graphics.set_pen(Inky73::BLACK);
  graphics.rectangle(Rect(0, graphics.height - 25, graphics.width , 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(5, graphics.height - 22), graphics.width /2, 2);
}

int main()
{
  stdio_init_all();
  sleep_ms(500);

  InkyFrame inky_frame;
  inky_frame.init();

  if (wifi_setup::wifi_connect(inky_frame) != Err::OK)
  {
    printf("Failed to connect to WiFi\n");
    draw_error(inky_frame, "Failed to connect to WiFi");
    inky_frame.update(true);
    return -1;
  }

  ResultOr<data_fetching::ImageInfo> info = data_fetching::fetch_image_info();
  if (info.ok()) {

    draw_lower_left_text(inky_frame, info.unwrap().image_text);
    inky_frame.update(true);
  } else  {
    printf("Test fetch failed\n");
    draw_error(inky_frame, "Test fetch failed");
    inky_frame.update(true);
    return -1;
  }


  printf("Done\n");

  wifi_setup::network_deinit();
  return 0;
}