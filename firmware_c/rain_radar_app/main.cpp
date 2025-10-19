#include <cstdio>
#include <math.h>
#include <string>

#include <stdio.h>
#include "drivers/psram_display/psram_display.hpp"
#include "drivers/inky73/inky73.hpp"
#include "pimoroni_common.hpp"
#include "pico_wireless.hpp"

#include "secrets.h"
#include "network.h"

using namespace pimoroni;

uint LED_PIN = 8;
uint LED_WARN_PIN = 6;
uint DISPLAY_WIDTH = 800;
uint DISPLAY_HEIGHT = 480;



static const std::string URL = "https://muse-hub.taile8f45.ts.net/";

void draw_error(PicoGraphics_PenInky7 &graphics, const std::string_view &msg) {
  graphics.set_pen(Inky73::RED);
  graphics.rectangle(Rect(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT -25, DISPLAY_WIDTH / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(DISPLAY_WIDTH / 2 + 5, DISPLAY_HEIGHT -22), DISPLAY_WIDTH / 2, 2);
}

void draw_lower_left_text(PicoGraphics_PenInky7 &graphics, const std::string_view &msg) {
  graphics.set_pen(Inky73::BLACK);
  graphics.rectangle(Rect(0, DISPLAY_HEIGHT -25, DISPLAY_WIDTH / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(5, DISPLAY_HEIGHT -22), DISPLAY_WIDTH / 2, 2);
}

int main() {
  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_function(LED_PIN, GPIO_FUNC_SIO);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  PSRamDisplay ramDisplay(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  PicoGraphics_PenInky7 graphics(DISPLAY_WIDTH, DISPLAY_HEIGHT, ramDisplay);
  Inky73 inky7(DISPLAY_WIDTH,DISPLAY_HEIGHT);
  PicoWireless wireless;

  network_led_init();
  pulse_network_led(1); // 1Hz pulse

  draw_error(graphics, "Initialising wireless...");
  draw_lower_left_text(graphics, "Pico Rain Radar");
  inky7.update(&graphics);
  sleep_ms(40000);



  if(! wifi_connect(wireless) ) {
    draw_error(graphics, "Failed to connect to WiFi");
    inky7.update(&graphics);
    sleep_ms(40000);
    return -1;
  }



  // while (true) {
  //   while(!inky7.is_pressed(Inky73::BUTTON_A)) {
  //     sleep_ms(10);
  //   }
  //   graphics.set_pen(1);
  //   graphics.clear();

  //   for(int i =0 ; i < 100 ; i++)
  //   {
  //     uint size = 25 + (rand() % 50);
  //     uint x = rand() % graphics.bounds.w;
  //     uint y = rand() % graphics.bounds.h;

  //     graphics.set_pen(0);
  //     graphics.circle(Point(x, y), size);

  //     graphics.set_pen(2+(i%5));
  //     graphics.circle(Point(x, y), size-2);
  //   }
  
  //   gpio_put(LED_PIN, 1);
  //   inky7.update(&graphics);
  //   gpio_put(LED_PIN, 0);
  // }

  return 0;
}