
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
#include "network_utils.hpp"
#include "pico_wireless.hpp"
#include "pico/stdlib.h"
#include "pimoroni_common.hpp"
#include "secrets.h"


using namespace pimoroni;



static const std::string URL = "https://muse-hub.taile8f45.ts.net/";

void draw_error(InkyFrame &graphics, const std::string_view &msg) {
  graphics.set_pen(Inky73::RED);
  graphics.rectangle(Rect(graphics.width / 2, graphics.height -25, graphics.width / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(graphics.width / 2 + 5, graphics.height -22), graphics.width / 2, 2);
}

void draw_lower_left_text(InkyFrame &graphics, const std::string_view &msg) {
  graphics.set_pen(Inky73::BLACK);
  graphics.rectangle(Rect(0, graphics.height -25, graphics.width / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(5, graphics.height -22), graphics.width / 2, 2);
}

int main()
{
  stdio_init_all();
  sleep_ms(1000);

  InkyFrame inky_frame;
  inky_frame.init();

  for (int i = 0; i < 3; i++)
  {
    printf("Inky Frame init done %d\n", i);
    fflush(stdout);
    sleep_ms(1000);
  }

  if (!wifi_connect(inky_frame))
  {
    printf("Failed to connect to WiFi\n");
    fflush(stdout);

    draw_error(inky_frame, "Failed to connect to WiFi");
    inky_frame.update(true);
    return -1;
  }

  while (true)
  {
    printf("Connected!\n");
    sleep_ms(1000);
  }

  draw_error(inky_frame, "test error");
  draw_lower_left_text(inky_frame, "Pico Rain Radar");
  inky_frame.update(true);

  int i = 0;

  while (true)
  {
    printf("Pico Rain Radar\n");
    printf("Attempt %d to init display\n", i++);
    sleep_ms(1000);
  }

  printf("Done\n");

  network_deinit();
  return 0;
}